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

#ifndef __RB_DRAW_H__
#define __RB_DRAW_H__

#include "rb_main.h"
#include "rb_fbo.h"
#include "rb_texture.h"
#include "rb_patch.h"
#include "d_player.h"

#define MAXDLDRAWCOUNT  0x10000
extern vtx_t drawVertex[MAXDLDRAWCOUNT];
extern byte rbSectorLightTable[256];
extern rbTexture_t whiteTexture;
extern rbTexture_t frameBufferTexture;
extern rbTexture_t depthBufferTexture;
extern rbfbo_t spriteFBO;
extern boolean skyvisible;

void RB_InitDrawer(void);
void RB_InitExtraHudTextures(void);
void RB_ShutdownDrawer(void);
void RB_CheckReInitDrawer(void);
void RB_DeleteExtraHudTextures(void);
void RB_SetQuadAspectDimentions(vtx_t *v, const int x, const int y,
                                const int width, const int height);
void RB_DrawTexture(rbTexture_t *texture, const float x, const float y,
                    const int fixedwidth, const int fixedheight, byte alpha);
void RB_DrawTextureForName(const char *pic, const float x, const float y,
                           const int fixedwidth, const int fixedheight, byte alpha);
void RB_DrawScreenTexture(rbTexture_t *texture, const int width, const int height);
void RB_DrawStretchPic(const char *pic, const float x, const float y, const int width, const int height);
void RB_DrawMouseCursor(const int x, const int y);
void RB_BindDrawPointers(vtx_t *vtx);
void RB_AddTriangle(int v0, int v1, int v2);
void RB_DrawElements(void);
void RB_ResetElements(void);
void RB_RenderPlayerSprites(player_t *player);
void RB_DrawPlayerFlash(player_t *player);
void RB_DrawDamageMarkers(player_t *player);
void RB_DrawExtraHudPics(void);
void RB_DrawPlayerNames(void);
void RB_DrawScene(void);
void RB_RenderMotionBlur(void);
void RB_RenderFXAA(void);
void RB_RenderBloom(void);

//
// RB_DrawVtxQuadImmediate
//
 
static dinline void RB_DrawVtxQuadImmediate(vtx_t *v)
{
    dglBegin(GL_QUADS);
    dglColor4ubv(&v[0].r);
    dglTexCoord2fv(&v[0].tu); dglVertex3fv(&v[0].x);
    dglTexCoord2fv(&v[1].tu); dglVertex3fv(&v[1].x);
    dglTexCoord2fv(&v[3].tu); dglVertex3fv(&v[3].x);
    dglTexCoord2fv(&v[2].tu); dglVertex3fv(&v[2].x);
    dglEnd();
}

//
// RB_SetVertexColor
//

static dinline void RB_SetVertexColor(vtx_t *v, byte r, byte g, byte b, byte a, int count)
{
    int i;
    
    for(i = 0; i < count; ++i)
    {
        v[i].r = r;
        v[i].g = g;
        v[i].b = b;
        v[i].a = a;
    }
}

#endif
