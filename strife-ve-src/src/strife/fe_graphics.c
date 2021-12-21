//
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
//    Configuration and netgame negotiation frontend for
//    Strife: Veteran Edition
//
// AUTHORS:
//    James Haley
//    Samuel Villarreal (Hi-Res BG code, mouse pointer)
//

#include "rb_config.h"
#include "rb_main.h"
#include "rb_draw.h"
#include "rb_texture.h"

#include "z_zone.h"

#include "f_wipe.h"
#include "hu_lib.h"
#include "m_menu.h"
#include "v_video.h"
#include "w_wad.h"

#include "fe_frontend.h"
#include "fe_graphics.h"

//=============================================================================
//
// General Graphics
//

// sigil cursor graphics
static char *feCursorName[8] = 
{
    "M_CURS1", "M_CURS2", "M_CURS3", "M_CURS4", 
    "M_CURS5", "M_CURS6", "M_CURS7", "M_CURS8" 
};

// laser cursor graphics
static char *feLaserName[2] =
{
    "SHT1A0", "SHT1B0"
};

//
// Write a centered string in the custom SVE big font.
//
void FE_WriteBigTextCentered(int y, const char *str)
{
    V_WriteBigText(str, 160 - V_BigFontStringWidth(str)/2, y);
}

//
// Write a centered string in the HUD/menu font.
//
void FE_WriteSmallTextCentered(int y, const char *str)
{
    M_WriteTextEx(160 - M_StringWidth(str)/2, y, str, false);
}

//
// Write yellow text centered
//
void FE_WriteYellowTextCentered(int y, const char *str)
{
    HUlib_drawYellowText(160 - HUlib_yellowTextWidth(str)/2, y, str, true);
}

//
// Clear the screen to black.
//
void FE_ClearScreen(void)
{
    V_DrawFilledBox(0, 0, SCREENWIDTH, SCREENHEIGHT, 0);
}

//
// Draw spinning Sigil cursor. Shamelessly duplicated from the menu system.
//
void FE_DrawSigilCursor(int x, int y)
{
    V_DrawPatch(x - 28, y - 5, 
        W_CacheLumpName(feCursorName[frontend_sigil], PU_CACHE));
}

//
// Draw blinking laser cursor.
//
void FE_DrawLaserCursor(int x, int y)
{
    V_DrawPatch(x - 8, y + 7, 
        W_CacheLumpName(feLaserName[frontend_laser], PU_CACHE));
}

//
// Render wipe effect. Also shamelessly duplicated, but out of D_Display.
// Call wipe_StartScreen before rendering a frame that will be followed with
// this.
//
void FE_DoWipe(void)
{
    boolean done = false;
    int     wipestart = I_GetTime() - 1;
    int     tics, nowtime;

    wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    do
    {
        do
        {
            nowtime = I_GetTime();
            tics = nowtime - wipestart;
            I_Sleep(1);
        }
        while(tics < 2);
        wipestart = nowtime;
        done = wipe_ScreenWipe(wipe_ColorXForm, 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
        I_FinishUpdate();
    }
    while(!done);

    // svillarreal - wait a frame
    frontend_waitframe = true;
    
    // svillarreal - clear buffer for 3d render mode
    if(use3drenderer)
    {
        RB_ClearBuffer(GLCB_ALL);
    }
}

//
// Draw box
//
void FE_DrawBox(int left, int top, int w, int h)
{
    int x, y;

    if(left + w > SCREENWIDTH || top + h > SCREENHEIGHT)
        return;

    if(w > SCREENWIDTH)
        w = SCREENWIDTH;
    if(h > 64)
        h = 64;

    for(x = left; x <= left+w-64; x += 64)
        V_DrawBlock(x, top, 64, h, W_CacheLumpName("F_PAVE02", PU_CACHE));
    if(x < left+w)
        V_DrawBlock(x, top, left+w-x, h, W_CacheLumpName("F_PAVE02", PU_CACHE));

    for(x = left; x <= left+w-8; x += 8)
    {
        V_DrawPatch(x, top-4,   W_CacheLumpName("BRDR_T", PU_CACHE));
        V_DrawPatch(x, top+h-4, W_CacheLumpName("BRDR_B", PU_CACHE));
    }
    if(x < left+w)
    {
        V_DrawPatch(left+w-8, top-4,   W_CacheLumpName("BRDR_T", PU_CACHE));
        V_DrawPatch(left+w-8, top+h-4, W_CacheLumpName("BRDR_B", PU_CACHE));
    }

    for(y = top; y <= top+h-8; y += 8)
    {
        V_DrawPatch(left-4,   y, W_CacheLumpName("BRDR_L", PU_CACHE));
        V_DrawPatch(left+w-4, y, W_CacheLumpName("BRDR_R", PU_CACHE));
    }
    if(y < top+h)
    {
        V_DrawPatch(left-4,   top+h-8, W_CacheLumpName("BRDR_L", PU_CACHE));
        V_DrawPatch(left+w-4, top+h-8, W_CacheLumpName("BRDR_R", PU_CACHE));
    }

    V_DrawPatch(left-4,   top-4,   W_CacheLumpName("BRDR_TL", PU_CACHE));
    V_DrawPatch(left+w-4, top-4,   W_CacheLumpName("BRDR_TR", PU_CACHE));
    V_DrawPatch(left-4,   top+h-4, W_CacheLumpName("BRDR_BL", PU_CACHE));
    V_DrawPatch(left+w-4, top+h-4, W_CacheLumpName("BRDR_BR", PU_CACHE));
}

//=============================================================================
//
// Hi-Resolution Drawing
//

typedef struct febackground_s
{
    const char *lowname;
    const char *highname;
    rbTexture_t texture;
} febackground_t;

static febackground_t backgrounds[FE_NUM_BGS] =
{
    { "FESIGIL",  "FHESIGIL"  },
    { "FERSKULL", "FHERSKULL" },
    { "FETSKULL", "FHETSKULL" }
};

//
// I_InitFrontEndTexture
//
static void FE_InitTexture(rbTexture_t *texture, const char *pic)
{
    int lumpnum = W_CheckNumForName(pic);
    byte *data = (byte*)W_CacheLumpNum(lumpnum, PU_STATIC);
    int w, h;
    
    texture->colorMode  = TCR_RGB;
    texture->origwidth  = (W_LumpLength(lumpnum) / SCREENHEIGHT) / 3;
    texture->origheight = SCREENHEIGHT;
    texture->width      = texture->origwidth;
    texture->height     = SCREENHEIGHT;

    for(h = 0; h < SCREENHEIGHT; ++h)
    {
        for(w = 0; w < texture->origwidth; ++w)
        {
            byte r = data[(h * texture->origwidth + w) * 3 + 0];
            byte g = data[(h * texture->origwidth + w) * 3 + 1];
            byte b = data[(h * texture->origwidth + w) * 3 + 2];

            data[(h * texture->origwidth + w) * 3 + 0] = gammatable[usegamma][r];
            data[(h * texture->origwidth + w) * 3 + 1] = gammatable[usegamma][g];
            data[(h * texture->origwidth + w) * 3 + 2] = gammatable[usegamma][b];
        }
    }
    
    RB_UploadTexture(texture, data, TC_CLAMP, TF_NEAREST);

    Z_Free(data);
}

//
// Initialize backgrounds
//
void FE_InitBackgrounds(void)
{
    int i;

    if(!use3drenderer)
        return;

    for(i = 0; i < FE_NUM_BGS; i++)
        FE_InitTexture(&backgrounds[i].texture, backgrounds[i].highname);
}

//
// Destroy backgrounds
//
void FE_DestroyBackgrounds(void)
{
    int i;

    if(!use3drenderer)
        return;

    for(i = 0; i < FE_NUM_BGS; i++)
        RB_DeleteTexture(&backgrounds[i].texture);
}

//
// FE_RefreshBackgrounds
//

void FE_RefreshBackgrounds(void)
{
    if(!use3drenderer)
        return;

    FE_DestroyBackgrounds();
    FE_InitBackgrounds();
}

//
// I_DrawFrontEndTexture
//
static void FE_DrawTexture(rbTexture_t *texture)
{
    vtx_t v[4];
    int i;
    float tx;

    RB_SetOrtho();
    RB_BindTexture(texture);
    RB_ChangeTexParameters(texture, TC_CLAMP, TEXFILTER);

    tx = (float)texture->origwidth / (float)texture->width;

    for(i = 0; i < 4; ++i)
    {
        v[i].r = v[i].g = v[i].b = v[i].a = 0xff;
        v[i].z = 0;
    }

    const int xoff = -((texture->origwidth - SCREENWIDTH) / 2);
    
    RB_SetQuadAspectDimentions(v, xoff, 0, texture->origwidth, SCREENHEIGHT);

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
// Render a background, using the hi-res version if available in the currently
// active rendering engine, and falling back to a patch otherwise.
//
void FE_DrawBackground(int bgnum)
{
    febackground_t *bg = &backgrounds[bgnum];

    if(use3drenderer)
        FE_DrawTexture(&bg->texture);
    else
    {
        int lumpnum;
        if((lumpnum = W_CheckNumForName((char*)bg->lowname)) >= 0)
            V_DrawPatch(0, 0, W_CacheLumpNum(lumpnum, PU_CACHE));
        else
            FE_ClearScreen();
    }
}

// EOF

