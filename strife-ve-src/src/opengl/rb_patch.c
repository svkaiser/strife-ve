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
//    Patch buffer rendering
//

#include "rb_config.h"
#include "rb_data.h"
#include "rb_draw.h"
#include "rb_patch.h"
#include "doomstat.h"
#include "i_video.h"
#include "i_swap.h"
#include "r_defs.h"
#include "r_state.h"
#include "w_wad.h"
#include "z_zone.h"
#include "deh_str.h"

// haleyjd 20141111: patch palette
static unsigned int rbPalette[256];
static int playpallump;

// patch canvas
static rbTexture_t patchTexture;
static byte *patchBuffer;

//
// RB_SetPatchBufferPalette
//
// haleyjd 20141111: Precache converted palette
//

void RB_SetPatchBufferPalette(void)
{
    int i;
    byte *paldata = W_CacheLumpNum(playpallump, PU_CACHE);
    byte tempcol[3];
    
    for(i = 0; i < 256; i++)
    {
        RB_GetPaletteRGB(tempcol, paldata, i, 0);
        rbPalette[i] = D_RGBA(gammatable[usegamma][tempcol[0]],
                              gammatable[usegamma][tempcol[1]],
                              gammatable[usegamma][tempcol[2]], 0);
    }
}

//
// RB_PatchBufferInit
//

void RB_PatchBufferInit(void)
{
    patchTexture.colorMode = TCR_RGBA;
    patchTexture.origwidth = SCREENWIDTH;
    patchTexture.origheight = SCREENHEIGHT;
    
    patchTexture.width = RB_RoundPowerOfTwo(patchTexture.origwidth);
    patchTexture.height = patchTexture.origheight;
    
    patchBuffer = (byte*)Z_Calloc(1, (patchTexture.width * patchTexture.height) * 4, PU_STATIC, 0);
    RB_UploadTexture(&patchTexture, patchBuffer, TC_CLAMP_BORDER, TF_NEAREST);
    
    playpallump = W_GetNumForName(DEH_String("PLAYPAL"));
    RB_SetPatchBufferPalette();
}

//
// RB_PatchBufferShutdown
//

void RB_PatchBufferShutdown(void)
{
    RB_DeleteTexture(&patchTexture);
}

//
// RB_BlitPatch
//

void RB_BlitPatch(int x, int y, patch_t *patch, byte alpha)
{
    int col;
    unsigned int *desttop;
    int w;
    int h;
    byte *colData, ctd;
    column_t *column;
    
    col = 0;
    desttop = (unsigned int*)&patchBuffer[((patchTexture.width * y) + x) * 4];
    
	const int left_overhang  = x < 0 ? -x : 0;
	const int right_overhang = x + SHORT(patch->width) > patchTexture.width ? x + SHORT(patch->width) - patchTexture.width : 0;

    for(w = left_overhang; w < SHORT(patch->width) - right_overhang; ++w)
    {
        column = (column_t*)((byte*)patch + LONG(patch->columnofs[w]));
        while((ctd = column->topdelta) != 0xff)
        {
            colData = (byte*)column + 3;
            
            for(h = 0; h < column->length; ++h)
            {
				const int index = patchTexture.width * (ctd + h) + w;
				if(desttop + index >= (unsigned int*)patchBuffer + patchTexture.height * patchTexture.width)
					break; // we're not drawing anything else
				else if(desttop + index >= (unsigned int*)patchBuffer)
					desttop[index] = rbPalette[colData[h]] | (alpha << 24);
            }
            
            column = (column_t*)((byte*)column + column->length + 4);
        }
    }
}

//
// RB_BlitBlock
//

void RB_BlitBlock(int x, int y, int width, int height, byte *data, byte alpha)
{
    unsigned int *desttop;
    int w;
    int h;
    byte *dp;
    
    desttop = (unsigned int*)&patchBuffer[((patchTexture.width * y) + x) * 4];
    dp = data;
    
    for(h = 0; h < height; ++h)
    {
        for(w = 0; w < width; ++w)
        {
            desttop[patchTexture.width * h + w] = rbPalette[*dp] | (alpha << 24);
            dp++;
        }
    }
}

//
// RB_DrawPatchBuffer
//

void RB_DrawPatchBuffer(void)
{
    vtx_t v[4];
    float tx;

    if (patchTexture.texid == 0)
    {
        RB_UploadTexture(&patchTexture, patchBuffer, TC_CLAMP_BORDER, TF_NEAREST);
    }
    
    RB_SetOrtho();
    
    RB_BindTexture(&patchTexture);
    RB_UpdateTexture(&patchTexture, patchBuffer);
    RB_ChangeTexParameters(&patchTexture, TC_REPEAT, TEXFILTER);
    
    tx = (float)patchTexture.origwidth / (float)patchTexture.width;
    
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;
    
    // haleyjd 20141007: [SVE] support all aspect ratios properly
    RB_SetQuadAspectDimentions(v, 0, 0, SCREENWIDTH, SCREENHEIGHT);
    
    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = tx;
    v[2].tv = v[3].tv = 1;
    
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
    
    memset(patchBuffer, 0, (patchTexture.width * patchTexture.height) * 4);
}
