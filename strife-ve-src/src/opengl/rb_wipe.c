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
//    OpenGL version of the screen wipe effect
//

#include "doomstat.h"
#include "rb_draw.h"
#include "rb_wipe.h"
#include "rb_draw.h"
#include "rb_texture.h"
#include "i_video.h"

static rbTexture_t wipeTexture;
static rbTexture_t destTexture;
static int wipeAlpha = 0xff;

boolean rbInWipe = false;

//
// RB_StartWipe
//

void RB_StartWipe(void)
{
    RB_BindFrameBuffer(&wipeTexture);
    wipeAlpha = 0xff;
    rbInWipe = true;
}

//
// RB_StartDestWipe
//

void RB_StartDestWipe(void)
{
    RB_DrawExtraHudPics();
    RB_DrawPatchBuffer();
    RB_BindFrameBuffer(&destTexture);
}

//
// RB_EndWipe
//

void RB_EndWipe(void)
{
    RB_DeleteTexture(&wipeTexture);
    RB_DeleteTexture(&destTexture);
    rbInWipe = false;
}

//
// RB_DrawWipe
//

boolean RB_DrawWipe(void)
{
    vtx_t v[4];

    RB_ClearBuffer(GLCB_ALL);
    RB_BindTexture(&destTexture);

    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;

    v[0].x = v[2].x = 0;
    v[0].y = v[1].y = 0;
    v[1].x = v[3].x = SCREENWIDTH;
    v[2].y = v[3].y = SCREENHEIGHT;

    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 1;
    v[1].tu = v[3].tu = 1;
    v[2].tv = v[3].tv = 0;

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);
    RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);

    // render
    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);
    RB_DrawElements();

    RB_BindTexture(&wipeTexture);
    RB_SetVertexColor(v, wipeAlpha, wipeAlpha, wipeAlpha, wipeAlpha, 4);
    RB_DrawElements();
    RB_ResetElements();

    wipeAlpha -= 0x1F;

    return (wipeAlpha < 0);
}
