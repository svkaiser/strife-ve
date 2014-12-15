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
//    Sky rendering for OpenGL
//

#include "doomstat.h"
#include "rb_draw.h"
#include "rb_view.h"
#include "rb_data.h"
#include "rb_texture.h"
#include "rb_sky.h"
#include "rb_config.h"
#include "w_wad.h"
#include "deh_str.h"
#include "rb_wallshade.h"

#define CLOUD_SIZE          768
#define CLOUD_OUTER_SIZE    (CLOUD_SIZE * 0.0625f)
#define CLOUD_HEIGHT        80
#define CLOUD_TILE          8.0f
#define CLOUD_TILE_INNER    (CLOUD_TILE * 0.46875f)
#define CLOUD_TILE_OUTER    (CLOUD_TILE * 0.53125f)

static float sky_cloudpan1 = 0;
static float sky_cloudpan2 = 0;

// cloud lumps
static int cloudlump1;
static int cloudlump2;
static int currentCloudLump;

// cloud color stuff
static float clearColor[4];
static unsigned int skyDomeColor1;
static unsigned int skyDomeColor2;

//
// RB_InitSky
//

void RB_InitSky(void)
{
    cloudlump1 = W_GetNumForName(DEH_String("CLOUD1"));
    cloudlump2 = W_GetNumForName(DEH_String("CLOUD2"));
}

//
// RB_DeleteSkyTextures
//

void RB_DeleteSkyTextures(void)
{
    rbTexture_t *t1 = RB_GetTexture(RDT_PATCH, cloudlump1, 0);
    rbTexture_t *t2 = RB_GetTexture(RDT_PATCH, cloudlump2, 0);

    RB_DeleteTexture(t1);
    RB_DeleteTexture(t2);
}

//
// RB_DrawSkyDome
//
// Draws a cylinder-ish dome with upper/lower color gradients
//

void RB_DrawSkyDome(int tiles, float rows, int height,
                    int radius, float offset, float topoffs,
                    unsigned int c1, unsigned int c2)
{
    fixed_t x, y, z;
    fixed_t lx, ly;
    int i;
    angle_t an;
    float tu1, tu2;
    int r;
    vtx_t *vtx;
    int count;

    lx = ly = count = 0;

#define NUM_SKY_DOME_FACES  32

    //
    // front faces are drawn here, so cull the back faces
    //
    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_BACK);
    RB_SetState(GLSTATE_DEPTHTEST, true);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, false);

    r = radius / (NUM_SKY_DOME_FACES / 4);

    //
    // set pointer for the main vertex list
    //
    RB_BindDrawPointers(drawVertex);
    vtx = drawVertex;

    // excuse the mess......
#define SKYDOME_VERTEX() vtx->x = FIXED2FLOAT(x); vtx->y = FIXED2FLOAT(y); vtx->z = FIXED2FLOAT(z)
#define SKYDOME_UV(u, v) vtx->tu = u; vtx->tv = v
#define SKYDOME_LEFT(v, h)                      \
    x = lx;                                     \
    y = ly;                                     \
    z = (h<<FRACBITS);                          \
    SKYDOME_UV(tu1, v);                        \
    SKYDOME_VERTEX();                           \
    vtx++

#define SKYDOME_RIGHT(v, h)                     \
    x = lx + FixedMul((r<<FRACBITS), finecosine[angle >> ANGLETOFINESHIFT]);    \
    y = ly + FixedMul((r<<FRACBITS), finesine[angle >> ANGLETOFINESHIFT]);      \
    z = (h<<FRACBITS);                          \
    SKYDOME_UV((tu2 * (i + 1)), v);            \
    SKYDOME_VERTEX();                           \
    vtx++

    tu1 = 0;
    tu2 = ((float)tiles / (float)NUM_SKY_DOME_FACES) * 0.5f;
    an = (ANG_MAX / NUM_SKY_DOME_FACES);

    //
    // setup vertex data
    //
    for(i = 0; i < NUM_SKY_DOME_FACES; ++i)
    {
        angle_t angle = an * i;

        *(unsigned int*)&vtx[0].r = c2;
        *(unsigned int*)&vtx[1].r = c1;
        *(unsigned int*)&vtx[2].r = c1;
        *(unsigned int*)&vtx[3].r = c2;

        SKYDOME_LEFT(rows, -height);
        SKYDOME_LEFT(topoffs, height);
        SKYDOME_RIGHT(topoffs, height);
        SKYDOME_RIGHT(rows, -height);

        lx = x;
        ly = y;

        RB_AddTriangle(0+count, 1+count, 2+count);
        RB_AddTriangle(3+count, 0+count, 2+count);
        count += 4;

        tu1 += tu2;
    }

    for(i = 0; i < NUM_SKY_DOME_FACES * 4; i++)
    {
        drawVertex[i].x += -((float)radius / ((float)NUM_SKY_DOME_FACES / 2.0f));
        drawVertex[i].y += -((float)radius / (M_PI / 2));
        drawVertex[i].z += -offset;
    }

    //
    // draw sky dome
    //
    RB_DrawElements();
    RB_ResetElements();

    RB_SetCull(GLCULL_FRONT);

#undef SKYDOME_RIGHT
#undef SKYDOME_LEFT
#undef SKYDOME_UV
#undef SKYDOME_VERTEX
#undef NUM_SKY_DOME_FACES
}

//
// RB_DrawClouds
//

void RB_DrawClouds(const int lump)
{
    vtx_t *v;
    int i;
    rbTexture_t *texture;

    RB_SetCull(GLCULL_BACK);

    texture = RB_GetTexture(RDT_PATCH, lump, 0);
    RB_BindTexture(texture);
    RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);

    RB_BindDrawPointers(drawVertex);
    v = drawVertex;

    /*
    --------------------------
    |\                     / |
    |  \                 /   |
    |    \             /     |
    |      \ _______ /       |
    |       |       |        |
    |       |       |        |
    |       |       |        |
    |       |_______|        |
    |      /         \       |
    |    /             \     |
    |  /                 \   |
    |/                     \ |
    --------------------------
    */

    v[0].x = -CLOUD_OUTER_SIZE; v[0].y =  CLOUD_OUTER_SIZE; v[0].z = CLOUD_HEIGHT;
    v[1].x =  CLOUD_OUTER_SIZE; v[1].y =  CLOUD_OUTER_SIZE; v[1].z = CLOUD_HEIGHT;
    v[2].x =  CLOUD_OUTER_SIZE; v[2].y = -CLOUD_OUTER_SIZE; v[2].z = CLOUD_HEIGHT;
    v[3].x = -CLOUD_SIZE;       v[3].y = -CLOUD_SIZE;       v[3].z = CLOUD_HEIGHT;
    v[4].x = -CLOUD_SIZE;       v[4].y =  CLOUD_SIZE;       v[4].z = CLOUD_HEIGHT;
    v[5].x =  CLOUD_SIZE;       v[5].y = -CLOUD_SIZE;       v[5].z = CLOUD_HEIGHT;
    v[6].x = -CLOUD_OUTER_SIZE; v[6].y = -CLOUD_OUTER_SIZE; v[6].z = CLOUD_HEIGHT;
    v[7].x =  CLOUD_SIZE;       v[7].y =  CLOUD_SIZE;       v[7].z = CLOUD_HEIGHT;

    v[0].tu = CLOUD_TILE_INNER; v[0].tv = CLOUD_TILE_OUTER;
    v[1].tu = CLOUD_TILE_INNER; v[1].tv = CLOUD_TILE_INNER;
    v[2].tu = CLOUD_TILE_OUTER; v[2].tv = CLOUD_TILE_INNER;
    v[3].tu = CLOUD_TILE;       v[3].tv = CLOUD_TILE;
    v[4].tu = 0.0f;             v[4].tv = CLOUD_TILE;
    v[5].tu = CLOUD_TILE;       v[5].tv = 0.0f;
    v[6].tu = CLOUD_TILE_OUTER; v[6].tv = CLOUD_TILE_OUTER;
    v[7].tu = 0.0f;             v[7].tv = 0.0f;

    for(i = 0; i < 8; ++i)
    {
        v[i].r = 255;
        v[i].g = 255;
        v[i].b = 255;
        v[i].a = 100;
        v[i].tu += sky_cloudpan1;
        v[i].tv += sky_cloudpan2;
    }

    // zero out the outer portion of the geometry so it blends into the sky dome
    v[3].a = v[4].a = v[5].a = v[7].a = 0;
    v[3].r = v[4].r = v[5].r = v[7].r = 0;
    v[3].g = v[4].g = v[5].g = v[7].g = 0;
    v[3].b = v[4].b = v[5].b = v[7].b = 0;

    RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(4, 0, 3);
    RB_AddTriangle(5, 3, 6);
    RB_AddTriangle(7, 5, 2);
    RB_AddTriangle(4, 7, 1);
    RB_AddTriangle(6, 0, 2);
    RB_AddTriangle(3, 0, 6);
    RB_AddTriangle(2, 5, 6);
    RB_AddTriangle(1, 7, 2);
    RB_AddTriangle(0, 4, 1);
    RB_DrawElements();
    RB_ResetElements();
}

//
// RB_CloudTicker
//

void RB_CloudTicker(void)
{
    sky_cloudpan1 += 0.0005625f;
    sky_cloudpan2 += 0.0001875f;
    
    if(sky_cloudpan1 > 1) sky_cloudpan1 -= 1;
    if(sky_cloudpan2 > 1) sky_cloudpan2 -= 1;
}

//
// RB_SetupSkyData
//

void RB_SetupSkyData(void)
{
    if(((gamemap >= 9 && gamemap < 32) || gamemap == 35) ||
       (!deathmatch && players[0].weaponowned[wp_sigil]))
    {
        clearColor[0] = 0.25f;
        clearColor[1] = clearColor[2] = 0;
        clearColor[3] = 1;
        currentCloudLump = cloudlump2;
        skyDomeColor1 = D_RGBA(64, 0, 0, 0xff);
        skyDomeColor2 = D_RGBA(160, 8, 8, 0xff);
        RB_SetSkyShade(255, 236, 227);
    }
    else
    {
        clearColor[0] = clearColor[1] = 0.14f;
        clearColor[2] = 0.235f;
        clearColor[3] = 1;
        currentCloudLump = cloudlump1;
        skyDomeColor1 = D_RGBA(36, 36, 60, 0xff);
        skyDomeColor2 = D_RGBA(224, 224, 255, 0xff);
        RB_SetSkyShade(255, 252, 227);
    }

    dglClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
}

//
// RB_DrawSky
//

void RB_DrawSky(void)
{
    if(!skyvisible)
    {
        // don't draw skybox if no sky flats are visible
        return;
    }
    
    dglMatrixMode(GL_MODELVIEW);
    dglLoadMatrixf(rbPlayerView.rotation);
    
    RB_BindTexture(&whiteTexture);
    
    RB_DrawSkyDome(4, 1, 320, 2048, -256, 0, skyDomeColor1, skyDomeColor2);
    RB_DrawClouds(currentCloudLump);
    
    skyvisible = false;
}
