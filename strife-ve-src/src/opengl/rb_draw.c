//
// Copyright(C) 2007-2014 Samuel Villarreal
// Copyright(C) 2014 Night Dive Studios, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//    Drawing routines for OpenGL
//

#include <math.h>

#include "doomstat.h"
#include "rb_draw.h"
#include "rb_drawlist.h"
#include "rb_view.h"
#include "rb_data.h"
#include "rb_texture.h"
#include "rb_sky.h"
#include "rb_config.h"
#include "rb_shader.h"
#include "rb_wallshade.h"
#include "rb_lightgrid.h"
#include "rb_wipe.h"
#include "rb_hudtext.h"
#include "rb_things.h"
#include "fe_frontend.h"
#include "m_argv.h"
#include "i_system.h"
#include "i_video.h"
#include "i_swap.h"
#include "w_wad.h"
#include "hu_lib.h"
#include "p_pspr.h"
#include "p_local.h"
#include "st_stuff.h"
#include "deh_str.h"
#include "z_zone.h"

//=============================================================================
//
// Globals
//
//=============================================================================

#define MAXINDICES  0x10000

vtx_t drawVertex[MAXDLDRAWCOUNT];
byte rbSectorLightTable[256];

rbTexture_t whiteTexture;
rbTexture_t frameBufferTexture;
rbTexture_t depthBufferTexture;
rbfbo_t spriteFBO;

boolean skyvisible = false;

extern SDL_Window *windowscreen;

//=============================================================================
//
// Locals
//
//=============================================================================

static boolean bShowLightCells;
static int lightGridIndex;

// draw indices
static word indicecnt = 0;
static word drawIndices[MAXINDICES];

//=============================================================================
//
// FBOs, shaders and special textures
//
//=============================================================================

// fxaa
static rbfbo_t fxaaFBO;
static rbShader_t fxaaShader;

// bloom
static rbfbo_t bloomFBO;
static rbShader_t bloomShader;

// blur
static rbfbo_t blurFBO[2];
static rbShader_t blurShader;

// motion blur
extern float rendertic_msec;
extern unsigned int rendertic_step;

static matrix prevMVMatrix;
static rbShader_t motionBlurShader;

// dynamic lights
static rbTexture_t lightPointTexture;

// mouse cursor
static rbTexture_t mouseCursorTexture;

// extra hud pics
static rbTexture_t *extraHudTextures[3];

//=============================================================================
//
// Initialization and shutdown routines
//
//=============================================================================

//
// RB_InitSectorLightTable
//

static void RB_InitSectorLightTable(void)
{
    int i;
    float l;
    const float f = 0.95f;
    const float max = 255.0f / (expf(powf(1, f)) - 1);

    for(i = 0; i < 256; i++)
    {
        l = (expf(powf(i / 255.0f, f)) - 1) * max;

        if(l <   0) l = 0;
        if(l > 255) l = 255;

        rbSectorLightTable[i] = (byte)l;
    }
}

//
// RB_InitWhiteTexture
//

static void RB_InitWhiteTexture(void)
{
    byte rgb[48]; // 4x4 RGB texture
    memset(rgb, 0xff, 48);

    whiteTexture.colorMode = TCR_RGB;
    whiteTexture.origwidth = 4;
    whiteTexture.origheight = 4;
    whiteTexture.width = 4;
    whiteTexture.height = 4;

    RB_UploadTexture(&whiteTexture, rgb, TC_CLAMP, TF_NEAREST);
}

//
// RB_InitLightPointTexture
//

static void RB_InitLightPointTexture(void)
{
    byte *data = (byte*)W_CacheLumpName(DEH_String("LIGHT"), PU_STATIC);
    
    lightPointTexture.colorMode = TCR_RGB;
    lightPointTexture.origwidth = 64;
    lightPointTexture.origheight = 64;
    lightPointTexture.width = 64;
    lightPointTexture.height = 64;
    
    RB_UploadTexture(&lightPointTexture, data, TC_CLAMP, TF_LINEAR);

    Z_Free(data);
}

//
// RB_InitMouseCursor
//

static void RB_InitMouseCursor(void)
{
    byte *data = (byte*)W_CacheLumpName(DEH_String("CURSOR"), PU_STATIC);
    
    mouseCursorTexture.colorMode = TCR_RGBA;
    mouseCursorTexture.origwidth = 32;
    mouseCursorTexture.origheight = 32;
    mouseCursorTexture.width = 32;
    mouseCursorTexture.height = 32;
    
    RB_UploadTexture(&mouseCursorTexture, data, TC_CLAMP, TF_LINEAR);

    Z_Free(data);
}

//
// RB_InitExtraHudTextures
//

void RB_InitExtraHudTextures(void)
{
    extraHudTextures[0] = RB_GetTexture(RDT_PATCH, W_GetNumForName(DEH_String("INVB_WL")), 0);
    extraHudTextures[1] = RB_GetTexture(RDT_PATCH, W_GetNumForName(DEH_String("INVB_WR")), 0);
    extraHudTextures[2] = RB_GetTexture(RDT_PATCH, W_GetNumForName(DEH_String("INVB_54")), 0);
}

//
// RB_DeleteExtraHudTextures
//

void RB_DeleteExtraHudTextures(void)
{
    RB_DeleteTexture(extraHudTextures[0]);
    RB_DeleteTexture(extraHudTextures[1]);
    RB_DeleteTexture(extraHudTextures[2]);
}

//
// RB_InitDrawer
//

void RB_InitDrawer(void)
{
    int w;
    int h;

    RB_PatchBufferInit();
    
    RB_InitSectorLightTable();
    RB_InitWhiteTexture();
    RB_InitLightPointTexture();
    RB_InitMouseCursor();

	SDL_GetWindowSize(windowscreen, &w, &h);

    FBO_InitColorAttachment(&spriteFBO, 0, w, h);
    FBO_InitColorAttachment(&fxaaFBO, 0, w, h);
    FBO_InitColorAttachment(&bloomFBO, 0, w, h);
    FBO_InitColorAttachment(&bloomFBO, 0, w, h);

    FBO_InitColorAttachment(&blurFBO[0], 0, w >> 1, h >> 1);
    FBO_InitColorAttachment(&blurFBO[1], 0, w >> 3, h >> 3);

    SP_LoadProgram(&motionBlurShader, "MBLUR");
    SP_LoadProgram(&fxaaShader, "FXAA");
    SP_LoadProgram(&blurShader, "BLUR");
    SP_LoadProgram(&bloomShader, "BLOOM");

    bShowLightCells = M_CheckParm("-showlightcells");
}

//
// RB_ShutdownDrawer
//

void RB_ShutdownDrawer(void)
{
    SP_Delete(&fxaaShader);
    SP_Delete(&blurShader);
    SP_Delete(&bloomShader);
    SP_Delete(&motionBlurShader);

    FBO_Delete(&spriteFBO);
    FBO_Delete(&blurFBO[0]);
    FBO_Delete(&blurFBO[1]);
    FBO_Delete(&bloomFBO);

    RB_DeleteTexture(&whiteTexture);
    RB_DeleteTexture(&lightPointTexture);
    RB_DeleteTexture(&frameBufferTexture);
    RB_DeleteTexture(&depthBufferTexture);
    RB_DeleteTexture(&mouseCursorTexture);
    
    RB_PatchBufferShutdown();
}

//
// RB_CheckReInitDrawer
//

void RB_CheckReInitDrawer(void)
{
    int w;
    int h;

    SDL_GetWindowSize(windowscreen, &w, &h);

    if (!spriteFBO.bLoaded || (w == spriteFBO.fboWidth && h == spriteFBO.fboHeight))
    {
        return;
    }

    FBO_Delete(&spriteFBO);
    FBO_Delete(&blurFBO[0]);
    FBO_Delete(&blurFBO[1]);
    FBO_Delete(&bloomFBO);

    RB_DeleteTexture(&frameBufferTexture);
    RB_DeleteTexture(&depthBufferTexture);

    screen_width = w;
    screen_height = h;

    FBO_InitColorAttachment(&spriteFBO, 0, w, h);
    FBO_InitColorAttachment(&fxaaFBO, 0, w, h);
    FBO_InitColorAttachment(&bloomFBO, 0, w, h);
    FBO_InitColorAttachment(&bloomFBO, 0, w, h);

    FBO_InitColorAttachment(&blurFBO[0], 0, w >> 1, h >> 1);
    FBO_InitColorAttachment(&blurFBO[1], 0, w >> 3, h >> 3);

    // reset projection
    dglPushAttrib(GL_VIEWPORT_BIT);
    dglViewport(0, 0, screen_width, screen_height);
}

//=============================================================================
//
// Common drawing routines
//
//=============================================================================

//
// RB_SetQuadAspectDimentions
//
// Sets up a quad that confines to the aspect ratio
// Assumes 4 vertices passed in for the first param
//

void RB_SetQuadAspectDimentions(vtx_t *v, const int x, const int y, const int width, const int height)
{
    int aspect;
    float fratio;
    int ratio;
    int aspectwidth;
    
    aspect = (screen_width * FRACUNIT) / screen_height;
    
    if(aspect == 4 * FRACUNIT / 3) // nominal
    {
        v[0].x = v[2].x = x;
        v[0].y = v[1].y = y;
        v[1].x = v[3].x = x + width;
        v[2].y = v[3].y = y + height;
    }
    else if(aspect > 4 * FRACUNIT / 3) // widescreen (pillarboxed)
    {
        float fw;
        
        ratio = FixedDiv(4*FRACUNIT, 3 * aspect);
        fratio = FIXED2FLOAT(ratio);
        fw = width * fratio;
        aspectwidth = (SCREENWIDTH - ((SCREENWIDTH * ratio) >> 16)) / 2;
        
        v[0].x = v[2].x = (x * fratio) + aspectwidth;
        v[0].y = v[1].y = y;
        v[1].x = v[3].x = (x * fratio) + aspectwidth + fw;
        v[2].y = v[3].y = y + height;
    }
    else // narrow (letterboxed)
    {
        float top, bottom;
        
        ratio = FixedDiv(3*FRACUNIT, 4 * aspect);
        aspectwidth = ((SCREENWIDTH * ratio) >> 16);
        fratio = (float)aspectwidth / (float)SCREENHEIGHT;
        top = y * fratio + ((SCREENHEIGHT - aspectwidth) / 2);
        bottom = (float)height * fratio;
        
        v[0].x = v[2].x = x;
        v[0].y = v[1].y = top;
        v[1].x = v[3].x = x + width;
        v[2].y = v[3].y = top + bottom;
    }
}

//
// RB_DrawTexture
//
// Draws a textured quad that's confined to the aspect ratio
//

void RB_DrawTexture(rbTexture_t *texture, const float x, const float y,
                    const int fixedwidth, const int fixedheight, byte alpha)
{
    vtx_t v[4];
    int texwidth;
    int texheight;
    
    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = 1;
    v[2].tv = v[3].tv = 1;
    
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, alpha, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;
    
    texwidth = fixedwidth > 0 ? fixedwidth : texture->width;
    texheight = fixedheight > 0 ? fixedheight : texture->height;
    
    RB_SetQuadAspectDimentions(v, x, y, texwidth, texheight);
    RB_BindTexture(texture);
    RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);
    
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, false);
    
    // render
    RB_DrawVtxQuadImmediate(v);
}

//
// RB_DrawTextureForName
//

void RB_DrawTextureForName(const char *pic, const float x, const float y,
                           const int fixedwidth, const int fixedheight, byte alpha)
{
    rbTexture_t *texture;

    if(!(texture = RB_GetTexture(RDT_PATCH, W_GetNumForName((char*)pic), 0)))
    {
        return;
    }
    
    RB_DrawTexture(texture, x, y, fixedwidth, fixedheight, alpha);
}

//
// RB_DrawScreenTexture
//
// Draws a full screen texture that fits within the
// texture's aspect ratio
//

void RB_DrawScreenTexture(rbTexture_t *texture, const int width, const int height)
{
    vtx_t v[4];
    float tx;
    int i;
    SDL_Surface *screen;
    int wi, hi, ws, hs;
    float ri, rs;
    int scalew, scaleh;
    int xoffs = 0, yoffs = 0;

    RB_BindTexture(texture);

    screen = SDL_GetWindowSurface(windowscreen);
    ws = screen->w;
    hs = screen->h;

    tx = (float)texture->origwidth / (float)texture->width;

    for(i = 0; i < 4; ++i)
    {
        v[i].r = v[i].g = v[i].b = v[i].a = 0xff;
        v[i].z = 0;
    }

    wi = width;
    hi = height;

    rs = (float)ws / hs;
    ri = (float)wi / hi;

    if(rs > ri)
    {
        scalew = wi * hs / hi;
        scaleh = hs;
    }
    else
    {
        scalew = ws;
        scaleh = hi * ws / wi;
    }

    if(scalew < ws)
    {
        xoffs = (ws - scalew) / 2;
    }
    if(scaleh < hs)
    {
        yoffs = (hs - scaleh) / 2;
    }
    
    v[0].x = v[2].x = xoffs;
    v[0].y = v[1].y = yoffs;
    v[1].x = v[3].x = xoffs + scalew;
    v[2].y = v[3].y = yoffs + scaleh;

    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = tx;
    v[2].tv = v[3].tv = 1;

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);

    // render
    RB_DrawVtxQuadImmediate(v);
}

//
// RB_DrawStretchPic
//
// Simply draws a textured quad without being
// resized to match the aspect ratio
//

void RB_DrawStretchPic(const char *pic, const float x, const float y, const int width, const int height)
{
    rbTexture_t *texture;
    vtx_t v[4];
    
    if(!(texture = RB_GetTexture(RDT_PATCH, W_GetNumForName((char*)pic), 0)))
    {
        return;
    }
    
    RB_BindTexture(texture);
    
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;
    
    v[0].x = v[2].x = x;
    v[0].y = v[1].y = y;
    v[1].x = v[3].x = x + width;
    v[2].y = v[3].y = y + height;
    
    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = (float)texture->origwidth / (float)texture->width;
    v[2].tv = v[3].tv = (float)texture->origheight / (float)texture->height;
    
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, false);
    
    RB_DrawVtxQuadImmediate(v);
}

//
// RB_DrawMouseCursor
//

void RB_DrawMouseCursor(const int x, const int y)
{
    vtx_t v[4];
    float scale;
    float yscale;
    matrix mtx;

    // push a custom modelview matrix
    dglMatrixMode(GL_MODELVIEW);
    dglPushMatrix();

    int w;
    int h;

    SDL_GetWindowSize(windowscreen, &w, &h);

    MTX_SetOrtho(mtx, 0, (float)w, (float)h, 0, -1, 1);
    dglLoadMatrixf(mtx);

    // bind texture
    RB_BindTexture(&mouseCursorTexture);

    // setup vertices
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;
    
    scale = (float)w / (float)SCREENWIDTH;
    yscale = (float)h / (float)SCREENHEIGHT;

    v[0].x = v[2].x = x;
    v[0].y = v[1].y = y - (3.95f * yscale);
    v[1].x = v[3].x = x + ((mouseCursorTexture.width * 0.75f) * scale);
    v[2].y = v[3].y = (y - (3.95f * yscale)) + ((mouseCursorTexture.height * 0.75f) * scale);

    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = 1;
    v[2].tv = v[3].tv = 1;

    // set states
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);

    // render
    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);
    RB_DrawElements();
    RB_ResetElements();

    // pop modelview matrix
    dglPopMatrix();
}

//
// RB_BindDrawPointers
//
// Sets the pointer to the vertex data on the
// client side. A better approach would to use
// vertex buffer objects instead so everything
// can be done on the GPU side.
//
// Having witness numerous issues with ATI cards
// and seeing how vertex data will always be
// changing/moving, I just don't see it being worth
// while to support it
//

void RB_BindDrawPointers(vtx_t *vtx)
{
    static vtx_t *prevpointer = NULL;

    if(prevpointer == vtx)
    {
        return;
    }

    prevpointer = vtx;

    dglTexCoordPointer(2, GL_FLOAT, sizeof(vtx_t), &vtx->tu);
    dglVertexPointer(3, GL_FLOAT, sizeof(vtx_t), vtx);
    dglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vtx_t), &vtx->r);
}

//
// RB_AddTriangle
//

void RB_AddTriangle(int v0, int v1, int v2)
{
    if(indicecnt + 3 >= MAXINDICES)
    {
        fprintf(stderr, "RB_AddTriangle: Triangle indice overflow");
        return;
    }

    drawIndices[indicecnt++] = v0;
    drawIndices[indicecnt++] = v1;
    drawIndices[indicecnt++] = v2;
}

//
// RB_DrawElements
//

void RB_DrawElements(void)
{
    dglDrawElements(GL_TRIANGLES, indicecnt, GL_UNSIGNED_SHORT, drawIndices);
}

//
// RB_ResetElements
//

void RB_ResetElements(void)
{
    indicecnt = 0;
}

//=============================================================================
//
// Player sprite rendering
//
//=============================================================================

//
// RB_DrawPSprite
//

void RB_DrawPSprite(pspdef_t *psp, sector_t *sector, player_t *player)
{
    spritedef_t     *sprdef;
    spriteframe_t   *sprframe;
    int             spritenum;
    int             flip;
    byte            alpha = 0xff;
    float           x;
    float           y;
    float           width;
    float           height;
    float           u1;
    float           u2;
    float           v1;
    float           v2;
    float           tx;
    float           ty;
    vtx_t           v[4];
    rbTexture_t     *texture;
    short           lightlevel;

    // get sprite frame/defs
    sprdef = &sprites[psp->state->sprite];
    sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

    if(player->mo->flags & (MF_SHADOW|MF_MVIS))
    {
        if(player->powers[pw_invisibility] > 4*32
            || (player->powers[pw_invisibility] & 8))
        {
            alpha = (player->mo->flags & MF_MVIS) ? 24 : 64;
        }
        else if(player->powers[pw_invisibility] & 4)
        {
            alpha = (player->mo->flags & MF_MVIS) ? 64 : 192;
        }
    }
    
    lightlevel = sector->lightlevel + 64; // haleyjd: slightly brighter
    
    if(lightlevel > 0xff)
    {
        lightlevel = 0xff;
    }
    
    v[0].z = v[1].z = v[2].z = v[3].z = 0;
    
    if(psp->state->frame & FF_FULLBRIGHT)
    {
        RB_SetVertexColor(v, 0xff, 0xff, 0xff, alpha, 4);
    }
    else
    {
        RB_SetVertexColor(v, rbSectorLightTable[lightlevel],
                             rbSectorLightTable[lightlevel],
                             rbSectorLightTable[lightlevel],
                             alpha, 4);
    }
    
    if(rbWallShades && !(psp->state->frame & FF_FULLBRIGHT))
    {
        RB_SetPspriteShade(player, v, rbSectorLightTable[lightlevel]);
    }

    if(!(psp->state->frame & FF_FULLBRIGHT))
    {
        RB_SetSpriteCellColor(v, viewx, viewy, player->mo->z + (64*FRACUNIT), sector);
    }
    
    spritenum = sprframe->lump[0];
    flip = flipparm;
    
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, true);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);

    if(alpha < 0xff)
    {
        RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
    }

    // setup vertex data
    x = FIXED2FLOAT(psp->sx) - FIXED2FLOAT(spriteoffset[spritenum]);
    y = FIXED2FLOAT(psp->sy) - FIXED2FLOAT(spritetopoffset[spritenum]);

    width = FIXED2FLOAT(spritewidth[spritenum]);
    height = FIXED2FLOAT(spriteheight[spritenum]);
    
    if(!(texture = RB_GetTexture(RDT_SPRITE, spritenum, 0)))
    {
        return;
    }

    RB_BindTexture(texture);
    RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);

    tx = (float)texture->origwidth / (float)texture->width;
    ty = (float)texture->origheight / (float)texture->height;

    u1 = (float)flip;
    u2 = (float)tx-flip;
    v1 = (float)flip;
    v2 = (float)ty-flip;
    
    RB_SetQuadAspectDimentions(v, x, y, (int)width, (int)height);

    v[0].tu = v[2].tu = u1;
    v[0].tv = v[1].tv = v1;
    v[1].tu = v[3].tu = u2;
    v[2].tv = v[3].tv = v2;

    // render
    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);
    RB_DrawElements();

    if(!(player->mo->flags & (MF_SHADOW|MF_MVIS)) &&
        (texture = RB_GetBrightmap(RDT_SPRITE, spritenum, 0)))
    {
        RB_SetVertexColor(v, 0xff, 0xff, 0xff, alpha, 4);

        RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
        RB_SetDepth(GLFUNC_EQUAL);
        RB_BindTexture(texture);
        RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);
        RB_DrawElements();
        RB_SetDepth(GLFUNC_LEQUAL);
    }

    RB_ResetElements();
}

//
// RB_RenderPlayerSprites
//

void RB_RenderPlayerSprites(player_t *player)
{
    pspdef_t *psp;

    psp = &player->psprites[ps_weapon];
    for(psp = player->psprites; psp < &player->psprites[NUMPSPRITES]; psp++)
    {
        if(psp->state)
        {
            RB_DrawPSprite(psp, player->mo->subsector->sector, player);
        }
    }
}

//=============================================================================
//
// Player hud drawing
//
//=============================================================================

//
// RB_DrawPlayerFlash
//
// Renders a fullscreen transparent overlay to
// simulate player hud flashes
//

void RB_DrawPlayerFlash(player_t *player)
{
    vtx_t v[4];

    // inverse sigil shock
    if(player->fixedcolormap == INVERSECOLORMAP)
    {
        RB_SetVertexColor(v, 208, 208, 208, 255, 4);
    }
    // red flash for damage
    else if(player->damagecount)
    {
        int r1 = player->damagecount << 3;

        if(r1)
        {
            if(r1 > 128)
            {
                r1 = 128;
            }
        }

        RB_SetVertexColor(v, 255, 0, 0, r1, 4);
    }
    // yellow flash (for item pickups)
    else if(player->bonuscount)
    {
        int bc = (player->bonuscount + 8) << 1;

        if(bc > 192)
        {
            bc = 192;
        }

        RB_SetVertexColor(v, 255, 255, 0, bc, 4);
    }
    // green flash (for nukage effects)
    else if(player->nukagecount > 16*TICRATE || (player->nukagecount & 8))
    {
        RB_SetVertexColor(v, 0, 208, 16, 32, 4);
    }
    // berserk
    else if(player->powers[pw_strength] > 1)
    {
        int r2 = 12 - (player->powers[pw_strength] >> 6);

        if(r2 < 0)
            return;

        r2 <<= 3;

        if(r2 == 1)
        {
            r2 = 0;
        }
        else if(r2 > 40)
        {
            r2 = 40;
        }

        RB_SetVertexColor(v, 255, 0, 0, r2, 4);
    }
    else
    {
        // no effects are active
        return;
    }

    v[0].x = v[2].x = 0;
    v[0].y = v[1].y = 0;
    v[1].x = v[3].x = SCREENWIDTH;
    v[2].y = v[3].y = SCREENHEIGHT;

    v[0].z = v[1].z = v[2].z = v[3].z = 0;

    RB_BindTexture(&whiteTexture);

    if(player->fixedcolormap == INVERSECOLORMAP)
    {
        RB_SetBlend(GLSRC_ONE_MINUS_DST_COLOR, GLDST_ZERO);
    }
    else
    {
        RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    }

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);

    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);
    RB_DrawElements();
    RB_ResetElements();
}

//
// RB_DrawExtraHudPics
//
// Draws special hud graphic components that's not
// usually drawn on to the patch buffer. All are
// rendered as orthographic quads
//

void RB_DrawExtraHudPics(void)
{
    if(gamestate != GS_LEVEL || !gametic || FE_InOptionsMenu())
    {
        return;
    }

    // I am aware that ortho matrix may have already been set but
    // this is to just insure that we properly draw the pics during
    // screen wipe phases
    RB_SetOrtho();

    if(!automapactive && rbCrosshair)
    {
        player_t *player = &players[displayplayer];

        if(!(!classicmode &&
            (player->readyweapon == wp_torpedo ||
            player->readyweapon == wp_fist)) &&
            player->powers[pw_targeter] <= 1)
        {
            RB_DrawTextureForName(DEH_String("XHAIR"), 155,
                (viewheight != SCREENHEIGHT) ? 92 : 108, 15, 9, 0x7F);
        }
    }

    if(viewheight != SCREENHEIGHT || automapactive)
    {
        if(((screen_width * FRACUNIT) / screen_height) != (4 * FRACUNIT / 3))
        {
            // draw side hud pics for non-4:3 aspects
            RB_DrawTexture(extraHudTextures[0], -64, 161, 0, 0, 0xff);
            RB_DrawTexture(extraHudTextures[1], 320, 160, 0, 0, 0xff);
            RB_DrawTexture(extraHudTextures[2], 0, 200, 0, 0, 0xff);
        }
    }
}

//
// RB_DrawPlayerNames
//
// Displays the player's name on to the screen
//

void RB_DrawPlayerNames(void)
{
    int i;
    angle_t an;
    float alpha;
    float fangle;
    float x, y;
    float dist;
    char *name;
    mobj_t *mo;
    spritepos_t pos;

    for(i = 0; i < MAXPLAYERS; ++i)
    {
        if(i == displayplayer || !playeringame[i])
        {
            // don't display self or if not in game
            continue;
        }

        mo = players[i].mo;

        dist = FIXED2FLOAT(P_AproxDistance(viewx - mo->x, viewy - mo->y));
        if(dist > 1500.0f)
        {
            // too far away
            continue;
        }

        // normalize distance
        dist = (1500.0f - dist) / 1500.0f;

        an = viewangle - RB_PointToAngle(viewx - mo->x, viewy - mo->y);

        if(an > ANG180)
        {
            // wrap around
            an = ANG_MAX - an;
        }

        fangle = 180.0f - TRUEANGLES(an);

        if(fangle > 70.0f)
        {
            // not within range
            continue;
        }

        if(!P_CheckSight(players[displayplayer].mo, mo))
        {
            // something is obstructing this player
            continue;
        }

        // alpha is determined by how far the view is and how far off to
        // the side of the screen
        alpha = (255.0f - (255.0f * (fangle / 70.0f))) * dist;

        // fade the name alpha if invisible
        if(mo->player->powers[pw_invisibility] > 4*32 ||
            (mo->player->powers[pw_invisibility] & 8))
        {
            if(mo->flags & MF_MVIS) // almost totally invisible
                alpha *= 0.0941f;
            else
                alpha *= 0.376f;
        }
        else if(mo->player->powers[pw_invisibility] & 4)
        {
            alpha *= 0.125f;
        }

        // get formated name (either from Steam or internal)
        name = HUlib_makePrettyPlayerName(i);

        // get the lerped position and project coordinates to screen
        R_interpolateThingPosition(mo, &pos);
        RB_ProjectPointToView(&rbPlayerView, pos.x, pos.y, pos.z + mo->info->height, &x, &y);

        // fudge the text so it's centered
        x -= (RB_HudTextWidth(name, 0.5f) * 0.5f);

        // draw name on screen
        RB_HudTextDraw((int)x, (int)y, 0.5f, (int)alpha, name);

        Z_Free(name);
    }
}

//
// RB_DrawDamageMarkers
//
// Displays special arrow indictators that points
// to the source of where the damage came from
//

void RB_DrawDamageMarkers(player_t *player)
{
    damagemarker_t* dmgmarker;
    vtx_t v[3];
    angle_t an;
    
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);
    RB_SetState(GLSTATE_CULL, true);

    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    RB_BindTexture(&whiteTexture);
    
    an = player->mo->angle;
    
    v[0].x = -8; v[1].x = 8; v[2].x = 0;
    v[0].y =  0; v[1].y = 0; v[2].y = 4;
    v[0].r =  0; v[1].r = 0; v[2].r = 255;
    
    v[0].g = v[1].g = v[2].g = 0;
    v[0].b = v[1].b = v[2].b = 0;
    v[0].z = v[1].z = v[2].z = 0;
    
    for(dmgmarker = dmgmarkers.next; dmgmarker != &dmgmarkers; dmgmarker = dmgmarker->next)
    {
        float angle;
        byte alpha;
        
        alpha = (dmgmarker->tics << 3);
        
        if(alpha < 0)
        {
            alpha = 0;
        }
        
        v[0].a = alpha;
        v[1].a = alpha;
        v[2].a = alpha;
        
        angle = (float)TRUEANGLES(an - RB_PointToAngle(player->mo->x - dmgmarker->source->x,
                                                       player->mo->y - dmgmarker->source->y));
        
        dglPushMatrix();
        dglTranslatef(160, viewheight != SCREENHEIGHT ? 88 : 120, 0);
        dglRotatef(angle, 0.0f, 0.0f, 1.0f);
        dglTranslatef(0, 16, 0);
        RB_BindDrawPointers(v);
        RB_AddTriangle(0, 1, 2);
        RB_DrawElements();
        RB_ResetElements();
        dglPopMatrix();
    }
}

//=============================================================================
//
// Post processing
//
// These effects all display a full screen quad while applying the
// appropriate shader
//
//=============================================================================

//
// RB_RenderFXAA
//

void RB_RenderFXAA(void)
{
    if(!has_GL_ARB_shader_objects       ||
       !has_GL_ARB_framebuffer_object   ||
       !rbEnableFXAA)
    {
        return;
    }

    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);

#if 1
	int w;
	int h;
	SDL_GetWindowSize(windowscreen, &w, &h);
	FBO_CopyBackBuffer(&fxaaFBO, 0, 0, w, h);
#else
    FBO_CopyBackBuffer(&fxaaFBO, 0, 0, SDL_GetWindowSurface(windowscreen)->w, SDL_GetWindowSurface(windowscreen)->h);
#endif
    SP_Enable(&fxaaShader);

    SP_SetUniform1i(&fxaaShader, "uDiffuse", 0);

    SP_SetUniform1f(&fxaaShader, "uViewWidth", (float)w);
    SP_SetUniform1f(&fxaaShader, "uViewHeight", (float)h);

    SP_SetUniform1f(&fxaaShader, "uMaxSpan", 8.0f);
    SP_SetUniform1f(&fxaaShader, "uReduceMax", 8.0f);
    SP_SetUniform1f(&fxaaShader, "uReduceMin", 128.0f);

    FBO_Draw(&fxaaFBO, true);
    RB_DisableShaders();
}

//
// RB_RenderBloom
//

void RB_RenderBloom(void)
{
    static float bloomThreshold;
    int i, w, h;
    short threshold;
    float curthreshold;
    int vp[4];
    vtx_t v[4];

    if(!has_GL_ARB_shader_objects       ||
       !has_GL_ARB_framebuffer_object   ||
       !rbEnableBloom)
    {
        return;
    }

    v[0].x = v[2].x = 0;
    v[0].y = v[1].y = 0;
    v[1].x = v[3].x = SCREENWIDTH;
    v[2].y = v[3].y = SCREENHEIGHT;

    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 1;
    v[1].tu = v[3].tu = 1;
    v[2].tv = v[3].tv = 0;
    
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;

    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);

    // handle threshold overrides in sectors
    threshold = players[displayplayer].mo->subsector->sector->bloomthreshold;
    curthreshold = rbBloomThreshold;

    if(threshold != -1)
    {
        curthreshold = (float)threshold / 255.0f;

        if(threshold == 0 || rbBloomThreshold > curthreshold)
        {
            curthreshold = rbBloomThreshold;
        }
    }

    // lerp the active threshold
    bloomThreshold = (curthreshold - bloomThreshold) * rendertic_msec + bloomThreshold;

    // clamp down the threshold
    if(bloomThreshold < 0.5f) bloomThreshold = 0.5f;
    if(bloomThreshold > 1.0f) bloomThreshold = 1.0f;

    // pass 1: bloom
    RB_BindFrameBuffer(&frameBufferTexture);
    FBO_Bind(&bloomFBO);
    SP_Enable(&bloomShader);
    SP_SetUniform1i(&bloomShader, "uDiffuse", 0);
    SP_SetUniform1f(&bloomShader, "uBloomThreshold", bloomThreshold);

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, false);

    RB_DrawElements();

    FBO_UnBind(&bloomFBO);

    SP_Enable(&blurShader);
    SP_SetUniform1i(&blurShader, "uDiffuse", 0);
    SP_SetUniform1f(&blurShader, "uBlurRadius", 1.0f);

#if 1
	SDL_GetWindowSize(windowscreen, &w, &h);
#else
    w = SDL_GetWindowSurface(windowscreen)->w;
    h = SDL_GetWindowSurface(windowscreen)->h;
#endif

    // pass 2: blur
    for(i = 0; i < 2; ++i)
    {
        // horizonal
        FBO_CopyFrameBuffer(&blurFBO[i], &bloomFBO, w, h);
        FBO_Bind(&bloomFBO);
        FBO_BindImage(&blurFBO[i]);
        SP_SetUniform1f(&blurShader, "uSize", (float)blurFBO[i].fboWidth);
        SP_SetUniform1i(&blurShader, "uDirection", 1);

        RB_DrawElements();

        FBO_UnBind(&bloomFBO);

        // vertical
        FBO_CopyFrameBuffer(&blurFBO[i], &bloomFBO, w, h);
        FBO_Bind(&bloomFBO);
        FBO_BindImage(&blurFBO[i]);
        SP_SetUniform1f(&blurShader, "uSize", (float)blurFBO[i].fboHeight);
        SP_SetUniform1i(&blurShader, "uDirection", 0);
        
        RB_DrawElements();

        FBO_UnBind(&bloomFBO);
    }

    RB_ResetElements();
    RB_DisableShaders();

    dglGetIntegerv(GL_VIEWPORT, vp);

    // resize viewport to account for FBO dimentions
    dglPushAttrib(GL_VIEWPORT_BIT);
    dglViewport(0, 0, bloomFBO.fboWidth, bloomFBO.fboHeight);
    
    RB_SetBlend(GLSRC_ONE_MINUS_DST_COLOR, GLDST_ONE);
    FBO_Draw(&bloomFBO, true);

    dglPopAttrib();
}

//
// RB_RenderMotionBlur
//

void RB_RenderMotionBlur(void)
{
    matrix inverseMat;
    matrix motionMat;
    vtx_t v[4];
    int samples;
    float rampSpeed;
    float targetTimeMS;
    float currentTimeMS;
    float deltaTime;
    float velocity;
    
    if(!has_GL_ARB_shader_objects || !rbEnableMotionBlur)
    {
        return;
    }

    targetTimeMS = rendertic_msec * 1000.0f;
    currentTimeMS = (float)rendertic_step;

    if(currentTimeMS <= 0)
    {
        // this should never happen but just in case....
        return;
    }

    if(rbInWipe || MTX_IsUninitialized(prevMVMatrix))
    {
        MTX_Copy(prevMVMatrix, rbPlayerView.rotation);
    }

    // compute the pixel's current world space position using the
    // inverse of the current modelview matrix and multiply with
    // the previous modelview matrix
    MTX_Invert(inverseMat, rbPlayerView.rotation);
    MTX_Multiply(motionMat, inverseMat, prevMVMatrix);

    // determine the ramp speed in which to blur
    rampSpeed = (rbMotionBlurRampSpeed * 256.0f) * targetTimeMS;

    if(rampSpeed <= 0.0f)
    {
        velocity = 1.0f;
    }
    else
    {
        float quat1[4];
        float quat2[4];
        
        rampSpeed = 1.0f / (rampSpeed / currentTimeMS);
        
        // find the scalar between quaternions to determine
        // the velocity of the blur. assumes that the
        // quaternions are normalized
        MTX_ToQuaternion(rbPlayerView.rotation, quat1);
        MTX_ToQuaternion(prevMVMatrix, quat2);
        
        velocity = fabs(1.0f - (quat1[0] * quat2[0] +
                                quat1[1] * quat2[1] +
                                quat1[2] * quat2[2] +
                                quat1[3] * quat2[3]));

        // huh... wonder why this happens...
        // keep it within the [0.0, 1.0] range
        if(velocity > 1.0f)
        {
            velocity = 2.0f - velocity;
        }

        velocity /= rampSpeed;
        
        if(velocity < 0.0) velocity = 0.0;
        if(velocity > 1.0) velocity = 1.0;
    }

    // determine the time between blurs
    deltaTime = targetTimeMS / currentTimeMS;
    
    if(deltaTime < 0.0) deltaTime = 0.0;
    if(deltaTime > 1.0) deltaTime = 1.0;

    // save current rotation
    MTX_Copy(prevMVMatrix, rbPlayerView.rotation);

    samples = rbMotionBlurSamples;
    
    // setup shader
    SP_Enable(&motionBlurShader);
    SP_SetUniformMat4(&motionBlurShader, "uMVMPrevious", motionMat, false);
    SP_SetUniform1i(&motionBlurShader, "frameBuf", 0);
    SP_SetUniform1i(&motionBlurShader, "depthBuf", 1);
    SP_SetUniform1i(&motionBlurShader, "uSamples", samples);
    SP_SetUniform1f(&motionBlurShader, "uVelocity", velocity);
    SP_SetUniform1f(&motionBlurShader, "uDeltaTime", deltaTime);
    
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    
    // bind depth buffer to texture 1
    RB_SetState(GLSTATE_TEXTURE1, true);
    RB_BindDepthBuffer(&depthBufferTexture);
    
    // bind frame buffer to texture 0
    RB_SetTextureUnit(0);
    RB_BindFrameBuffer(&frameBufferTexture);
    
    // setup quad vertices
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;
    
    v[0].x = v[2].x = 0;
    v[0].y = v[1].y = 0;
    v[1].x = v[3].x = SCREENWIDTH;
    v[2].y = v[3].y = SCREENHEIGHT;
    
    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = 1;
    v[2].tv = v[3].tv = 1;
    
    // render
    RB_DrawVtxQuadImmediate(v);
    
    RB_SetState(GLSTATE_TEXTURE1, false);
    RB_SetTextureUnit(0);
    RB_DisableShaders();
}

//=============================================================================
//
// Scene rendering
//
// The bulk of the rendering will be done here
//
//=============================================================================

//
// RB_DrawSprites
//

static void RB_DrawSprites(void)
{
    // draw special outline sprites
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    DL_ProcessDrawList(DLT_SPRITEOUTLINE);
    RB_SetState(GLSTATE_DEPTHTEST, true);

    // this blend function works better for opaque sprites
    RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
    DL_ProcessDrawList(DLT_SPRITE);

    // redraw sprites with brightmaps
    RB_SetDepth(GLFUNC_EQUAL);
    DL_ProcessDrawList(DLT_SPRITEBRIGHT);
    RB_SetDepth(GLFUNC_LEQUAL);

    // draw transparent sprites
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    DL_ProcessDrawList(DLT_SPRITEALPHA);
}

//
// RB_FixSpriteClippingPreProcess
//
// Emulates sprite software rendering. Visible linedefs
// are added to an alternate draw list that's drawn into
// the depth buffer. Sprites are then clipped against it
// and saved into a framebuffer object. Masked/transparent
// textures must also be included in the framebuffer as well,
// otherwise sprites would be drawing on top of them
//

static void RB_FixSpriteClippingPreProcess(void)
{
    // bind framebuffer
    FBO_Bind(&spriteFBO);

    // clear everything, including alpha
    dglClearColor(0, 0, 0, 0);
    RB_ClearBuffer(GLCB_ALL);

    // don't write to color buffer
    RB_SetColorMask(0);

    // we do not want any culling enabled
    RB_SetState(GLSTATE_CULL, false);

    // draw occlusion lines
    DL_ProcessDrawList(DLT_CLIPLINE);

    // turn culling back on and color mask
    RB_SetState(GLSTATE_CULL, true);
    RB_SetColorMask(1);

    // draw sprites and then masked/transparent walls
    RB_DrawSprites();
    DL_ProcessDrawList(DLT_MASKEDWALL);
    
    RB_SetDepth(GLFUNC_EQUAL);
    DL_ProcessDrawList(DLT_BRIGHTMASKED);
    RB_SetDepth(GLFUNC_LEQUAL);
    
    DL_ProcessDrawList(DLT_TRANSWALL);

    // unbind framebuffer
    FBO_UnBind(&spriteFBO);
}

//
// RB_DrawScene
//
// Draws the actual scene processed by BSP traversal.
//
// Geometry may/will be redrawn to account for special
// effects such as dynamic lights and lightmaps in which
// are blended on top of the base geometry.
//
// Normally, multitexturing should be used instead for
// these sort of operations but I've seen a lot of issues
// with certain video cards and OpenGL extensions in general.
//
// The vertex/geometry overhead may be higher than usual, but
// I am willing to live with that
//

void RB_DrawScene(void)
{
    RB_SetupSkyData();
    RB_ClearBuffer(GLCB_ALL);

    // load projection
    dglMatrixMode(GL_PROJECTION);
    dglLoadMatrixf(rbPlayerView.projection);

    RB_DrawSky();
    RB_ClearBuffer(GLCB_DEPTH);

    // set opengl states
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, true);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);

    // load modelview
    dglMatrixMode(GL_MODELVIEW);
    dglLoadMatrixf(rbPlayerView.modelview);

    // only draw the sky geometry into the depth buffer
    RB_SetColorMask(0);
    DL_ProcessDrawList(DLT_SKY);
    RB_SetColorMask(1);

    // fix sprite clipping
    if(rbFixSpriteClipping && has_GL_ARB_framebuffer_object)
    {
        RB_FixSpriteClippingPreProcess();
    }

    // draw walls and flats
    DL_ProcessDrawList(DLT_WALL);
    DL_ProcessDrawList(DLT_FLAT);
    
    // draw brightmaps
    RB_SetDepth(GLFUNC_EQUAL);
    DL_ProcessDrawList(DLT_BRIGHT);
    RB_SetDepth(GLFUNC_LEQUAL);

    // redraw segs and leafs with lightmap textures
    if(drawlist[DLT_LIGHTMAP].index > 0)
    {
        // in a typical scenario, lightmaps are responsible for
        // lighting the entire scene, however sector lighting is still
        // the dominant source of lights for the given scene so lightmaps
        // are treated as a secondary source of light, so because of this,
        // modulated blending is out of the question. instead the blending
        // equation "(source*dest) + (dest*1)" is used. unfortunately,
        // the only drawback to this approach is that the lightmaps will
        // become much less noticeable in sectors with darker light levels.
        RB_SetBlend(GLSRC_DST_COLOR, GLDST_ONE);
        RB_SetDepth(GLFUNC_EQUAL);

        // draw geometry
        DL_ProcessDrawList(DLT_LIGHTMAP);

        // reset depth function and set RGB scale
        RB_SetDepth(GLFUNC_LEQUAL);
    }

    if(!rbFixSpriteClipping || !has_GL_ARB_framebuffer_object)
    {
        // draw sprites
        RB_DrawSprites();
    }

    // draw masked walls
    // note that this may get drawn twice if sprite clip fix is enabled
    // this is to ensure that they render properly when viewed behind
    // transparent geometry
    DL_ProcessDrawList(DLT_MASKEDWALL);

    RB_SetDepth(GLFUNC_EQUAL);
    DL_ProcessDrawList(DLT_BRIGHTMASKED);
    RB_SetDepth(GLFUNC_LEQUAL);

    if(!rbFixSpriteClipping)
    {
        // draw transparent walls
        DL_ProcessDrawList(DLT_TRANSWALL);
    }

    // set polygon offset for decals
    dglEnable(GL_POLYGON_OFFSET_FILL);
    dglPolygonOffset(-2, -2);

    // draw decals
    RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
    DL_ProcessDrawList(DLT_DECAL);

    // disable polygon offset
    dglDisable(GL_POLYGON_OFFSET_FILL);
    dglPolygonOffset(0, 0);

    // redraw geometry affected by dynamic lights
    if(rbDynamicLights)
    {
        RB_SetBlend((rbDynamicLightFastBlend ? GLSRC_SRC_ALPHA :
                                               GLSRC_DST_COLOR), GLDST_ONE);
        RB_SetDepth(GLFUNC_EQUAL);
        
        RB_BindTexture(&lightPointTexture);
        DL_ProcessDrawList(DLT_DYNLIGHT);
        
        RB_SetDepth(GLFUNC_LEQUAL);
    }
    
    DL_Reset(DLT_WALL);
    DL_Reset(DLT_MASKEDWALL);
    DL_Reset(DLT_TRANSWALL);
    DL_Reset(DLT_BRIGHT);
    DL_Reset(DLT_BRIGHTMASKED);
    DL_Reset(DLT_FLAT);
    DL_Reset(DLT_SPRITE);
    DL_Reset(DLT_SPRITEALPHA);
    DL_Reset(DLT_SPRITEBRIGHT);
    DL_Reset(DLT_SPRITEOUTLINE);
    DL_Reset(DLT_SKY);
    DL_Reset(DLT_CLIPLINE);
    DL_Reset(DLT_DECAL);
    DL_Reset(DLT_LIGHTMAP);
    DL_Reset(DLT_DYNLIGHT);

    if(bShowLightCells)
    {
        // visually render what light grid cell the player view is in (debugging)
        RB_DrawLightGridCell(lightGridIndex);
    }
}

//
// RB_PageDrawer
//
// Draws a widescreen compatible page
//
void RB_PageDrawer(const char* szPagename, const int xoff)
{
    static rbTexture_t* rbPageTexture = NULL;
    static const char* szLastPage = NULL;
    if (rbPageTexture != NULL && szLastPage != szPagename)
    {
        RB_DeleteTexture(rbPageTexture);
        szLastPage = NULL;
        rbPageTexture = NULL;
    }

    if (rbPageTexture == NULL)
    {
        rbPageTexture = RB_GetTexture(RDT_PATCH, W_GetNumForName(szPagename), 0);
        szLastPage = szPagename;
    }

    RB_DrawTexture(rbPageTexture, xoff, 0, 0, 0, 0xff);
}
