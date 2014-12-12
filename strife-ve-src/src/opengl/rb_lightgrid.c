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
//    Light grid utilities
//

#include <math.h>

#include "doomstat.h"
#include "rb_main.h"
#include "rb_lightgrid.h"
#include "rb_draw.h"
#include "r_defs.h"
#include "r_state.h"

//
// RB_GetLightGridIndex
//

int RB_GetLightGridIndex(fixed_t x, fixed_t y, fixed_t z)
{
    int idx;
    int i;
    int origin[3];
    int pos[3];
    float size[3];

    for(i = 0; i < 3; ++i)
    {
        size[i] = MAX(floorf(((float)lightgrid.blockSize[i] / (float)lightgrid.gridSize[i]) * 2.0f), 1);
    }

    // try to convert the origin to local coordinates
    origin[0] = (x >> FRACBITS) - (lightgrid.min[0] - (lightgrid.blockSize[0] / size[0]));
    origin[1] = (y >> FRACBITS) - (lightgrid.min[1] - (lightgrid.blockSize[1] / size[1]));
    origin[2] = (z >> FRACBITS) - (lightgrid.min[2] - (lightgrid.blockSize[2] / size[2]));

    for(i = 0; i < 3; ++i)
    {
        // determine what grid we're standing in
        pos[i] = MIN(MAX(floorf(origin[i] * lightgrid.gridUnit[i]), 0), lightgrid.blockSize[i]-1);
    }

    // convert to grid cell index
    idx = pos[0] +
          pos[1] * lightgrid.blockSize[0] +
          pos[2] * (lightgrid.blockSize[0] * lightgrid.blockSize[1]);

    if((idx < 0 || idx >= lightgrid.count) || lightgrid.bits[idx] == 0)
    {
        return -1;
    }

    return idx;
}

//
// RB_OverrideNeighborCell
//

static int RB_OverrideNeighborCell(int index, rbLightGridType_t checkType)
{
    // check west cell
    if(index + 1 < lightgrid.count &&
       lightgrid.types[index + 1] == checkType)
    {
        return index + 1;
    }
    // check east cell
    else if(index - 1 >= 0 &&
        lightgrid.types[index - 1] == checkType)
    {
        return index - 1;
    }
    // check north cell
    else if(index + lightgrid.blockSize[0] < lightgrid.count &&
        lightgrid.types[index + lightgrid.blockSize[0]] == checkType)
    {
        return index + lightgrid.blockSize[0];
    }
    // check south cell
    else if(index - lightgrid.blockSize[0] >= 0 &&
        lightgrid.types[index - lightgrid.blockSize[0]] == checkType)
    {
        return index - lightgrid.blockSize[0];
    }
    
    return index;
}

//
// RB_ApplyLightGridRGB
//

void RB_ApplyLightGridRGB(vtx_t *vtx, int index, boolean insky)
{
    byte *rgb;
    float r, g, b;
    float r1, r2;
    float g1, g2;
    float b1, b2;

    if(index <= -1)
    {
        return;
    }
    
    // some cells that aren't in shade may still overlap sectors with
    // a sky flat, which results in sudden 'flickering' of
    // the lightlevel brightness when moving between a shaded cell and
    // a normal cell. this checks for surrounding cells that are in shade
    // so we can appropriately avoid this glitch
    if(insky && lightgrid.types[index] == LGT_NONE)
    {
        // if standing in a sky sector but cell isn't shaded, then
        // look for neighbor cells that is a shade type
        index = RB_OverrideNeighborCell(index, LGT_SUNSHADE);
    }
    else if(!insky && lightgrid.types[index] == LGT_SUNSHADE)
    {
        // if not standing in a sky sector but cell is shaded, then
        // look for neighbor cells that isn't a shade type
        index = RB_OverrideNeighborCell(index, LGT_NONE);
    }
    
    rgb = lightgrid.rgb + (index * 3);
    
    r1 = (float)vtx->r / 255.0f;
    g1 = (float)vtx->g / 255.0f;
    b1 = (float)vtx->b / 255.0f;
    
    r2 = (float)rgb[0] / 255.0f;
    g2 = (float)rgb[1] / 255.0f;
    b2 = (float)rgb[2] / 255.0f;

    if(lightgrid.types[index] == LGT_SUNSHADE)
    {
        // sunshade types lerps color to a slightly darker shade
        r1 = (((vtx->r>>1) / 255.0f) - r1) * 0.635f + r1;
        g1 = (((vtx->g>>1) / 255.0f) - g1) * 0.635f + g1;
        b1 = (((vtx->b>>1) / 255.0f) - b1) * 0.635f + b1;
    }
    else if(lightgrid.types[index] == LGT_SUN)
    {
        // sun types lerps half the sun color to the vertex color
        vtx->r = (byte)(((r2 - r1) * 0.5f + r1) * 255.0f);
        vtx->g = (byte)(((g2 - g1) * 0.5f + g1) * 255.0f);
        vtx->b = (byte)(((b2 - b1) * 0.5f + b1) * 255.0f);
        return;
    }

    // mix colors together
    r = MIN((r1 + ((MIN((r1 * r2) + r2, 1))) / 2), 1);
    g = MIN((g1 + ((MIN((g1 * g2) + g2, 1))) / 2), 1);
    b = MIN((b1 + ((MIN((b1 * b2) + b2, 1))) / 2), 1);

    vtx->r = (byte)(r * 255.0f);
    vtx->g = (byte)(g * 255.0f);
    vtx->b = (byte)(b * 255.0f);
}

//
// RB_DrawLightGridCell
//

void RB_DrawLightGridCell(int index)
{
    float   bbox[6];
    byte    *rgb;
    float   fx, fy, fz;
    int     x, y, z;
    int     mod;
    int     gx = lightgrid.blockSize[0];
    int     gy = lightgrid.blockSize[1];

    if(index <= -1 || lightgrid.count <= 0)
    {
        return;
    }

#define DRAWBBOXPOLY(b1, b2, b3) \
    dglVertex3f(bbox[b1], bbox[b2], bbox[b3])

#define DRAWBBOXSIDE1(z) \
    dglBegin(GL_POLYGON); \
    DRAWBBOXPOLY(2, 1, z); \
    DRAWBBOXPOLY(2, 0, z); \
    DRAWBBOXPOLY(3, 0, z); \
    DRAWBBOXPOLY(3, 1, z); \
    dglEnd()

#define DRAWBBOXSIDE2(b3) \
    dglBegin(GL_POLYGON); \
    DRAWBBOXPOLY(2, b3, 4); \
    DRAWBBOXPOLY(3, b3, 4); \
    DRAWBBOXPOLY(3, b3, 5); \
    DRAWBBOXPOLY(2, b3, 5); \
    dglEnd()

#define DRAWBBOXSIDE3(b1) \
    dglBegin(GL_POLYGON); \
    DRAWBBOXPOLY(b1, 1, 4); \
    DRAWBBOXPOLY(b1, 1, 5); \
    DRAWBBOXPOLY(b1, 0, 5); \
    DRAWBBOXPOLY(b1, 0, 4); \
    dglEnd()

    // convert grid id to xyz coordinates
    mod = index;
    z = mod / (gx * gy);
    mod -= z * (gx * gy);

    y = mod / gx;
    mod -= y * gx;

    x = mod;

    fx = lightgrid.min[0] + x * lightgrid.gridSize[0];
    fy = lightgrid.min[1] + y * lightgrid.gridSize[1];
    fz = lightgrid.min[2] + z * lightgrid.gridSize[2];

    bbox[0] = fy + (lightgrid.gridSize[1] / 2);
    bbox[1] = fy - (lightgrid.gridSize[1] / 2);
    bbox[2] = fx + (lightgrid.gridSize[0] / 2);
    bbox[3] = fx - (lightgrid.gridSize[0] / 2);
    bbox[4] = fz - (lightgrid.gridSize[2] / 2);
    bbox[5] = fz + (lightgrid.gridSize[2] / 2);

    rgb = lightgrid.rgb + (index * 3);

    RB_BindTexture(&whiteTexture);
    RB_SetState(GLSTATE_CULL, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);

    RB_SetDepthMask(0);

    dglColor4ub(rgb[0], rgb[1], rgb[2], 64);
    dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    DRAWBBOXSIDE1(4);
    DRAWBBOXSIDE1(5);
    DRAWBBOXSIDE2(1);
    DRAWBBOXSIDE2(0);
    DRAWBBOXSIDE3(3);
    DRAWBBOXSIDE3(2);

    RB_SetDepthMask(1);

    if(lightgrid.types[index] == LGT_SUN)
    {
        dglColor4ub(255, 255, 0, 255);
    }
    else if(lightgrid.types[index] == LGT_SUNSHADE)
    {
        dglColor4ub(0, 255, 0, 255);
    }
    else
    {
        dglColor4ub(255, 255, 255, 255);
    }
    
    dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    DRAWBBOXSIDE1(4);
    DRAWBBOXSIDE1(5);
    DRAWBBOXSIDE2(1);
    DRAWBBOXSIDE2(0);
    DRAWBBOXSIDE3(3);
    DRAWBBOXSIDE3(2);

    dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    RB_SetState(GLSTATE_CULL, true);
}
