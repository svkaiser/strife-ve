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
//    OpenGL version of hud text display
//

#include "rb_config.h"
#include "rb_texture.h"
#include "rb_data.h"
#include "rb_draw.h"
#include "rb_matrix.h"
#include "deh_str.h"
#include "i_video.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "v_patch.h"
#include "w_wad.h"

static rbTexture_t *rbYFont[HU_FONTSIZE];

//
// RB_HudTextInit
//

void RB_HudTextInit(void)
{
    int i, j;
    char buffer[9];

    j = HU_FONTSTART;

    for(i = 0; i < HU_FONTSIZE; i++)
    {
        DEH_snprintf(buffer, 9, "STBFN%.3d", j++);
        rbYFont[i] = RB_GetTexture(RDT_PATCH, W_GetNumForName(buffer), 0);
    }
}

//
// RB_HudTextShutdown
//

void RB_HudTextShutdown(void)
{
    int i;

    for(i = 0; i < HU_FONTSIZE; i++)
    {
        RB_DeleteTexture(rbYFont[i]);
    }
}

//
// RB_HudTextWidth
//

float RB_HudTextWidth(const char *text, const float scale)
{
    float width;
    float textscale;

    width = (float)HUlib_yellowTextWidth(text);
    textscale = ((float)screen_width / (float)SCREENWIDTH) * scale;

    return width * textscale;
}

//
// RB_HudTextDraw
//

void RB_HudTextDraw(int x, int y, const float scale, const int alpha, const char *text)
{
    matrix mtx;
    int start_x = x;
    float textscale;
    const char *rover = text;
    char c;
    vtx_t v[4];

    if(x > screen_width || y > screen_height)
    {
        return;
    }

    // push a custom modelview matrix
    dglMatrixMode(GL_MODELVIEW);
    dglPushMatrix();

    MTX_SetOrtho(mtx, 0, (float)screen_width, (float)screen_height, 0, -1, 1);
    dglLoadMatrixf(mtx);

    // setup vertices
    RB_SetVertexColor(v, 0xff, 0xff, 0xff, alpha, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;

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

    textscale = ((float)screen_width / (float)SCREENWIDTH) * scale;

    while((c = *rover++))
    {
        if(c == '\n')
        {
            x = start_x;
            y += (12 * textscale);
            continue;
        }

        c = toupper(c) - HU_FONTSTART;

        if(c >= 0 && c < HU_FONTSIZE)
        {
            rbTexture_t *texture = rbYFont[(int)c];

            if(!texture)
            {
                continue;
            }

            v[0].x = v[2].x = x;
            v[0].y = v[1].y = y;
            v[1].x = v[3].x = x + texture->width * textscale;
            v[2].y = v[3].y = y + texture->height * textscale;

            RB_BindTexture(texture);
            RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);

            // render
            RB_DrawVtxQuadImmediate(v);

            x = x + texture->origwidth * textscale;
        }
        else
        {
            x += (4 * textscale);
        }
    }

    // pop modelview matrix
    dglPopMatrix();
}
