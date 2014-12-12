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

#ifndef __RB_VIEW_H__
#define __RB_VIEW_H__

#include "d_player.h"
#include "rb_matrix.h"

typedef enum
{
    FP_RIGHT    = 0,
    FP_LEFT,
    FP_BOTTOM,
    FP_TOP,
    FP_FAR,
    FP_NEAR,
    NUMFRUSTUMPLANES
} frustumPlane_t;

typedef struct
{
    float s;
    float c;
} rbViewRotation_t;

typedef struct
{
    float x;
    float y;
    float z;
    rbViewRotation_t rotyaw;
    rbViewRotation_t rotpitch;
    float viewyaw;
    float viewpitch;
    matrix projection;
    matrix rotation;
    matrix modelview;
    int extralight;
    float frustum[NUMFRUSTUMPLANES][4];
} rbView_t;

extern rbView_t rbPlayerView;

void RB_SetupFrameForView(rbView_t *view, const float fov);
boolean RB_CheckPointsInView(rbView_t *view, vtx_t *vertex, int count);
boolean RB_CheckSphereInView(rbView_t *view, const float x, const float y, const float z, const float radius);
boolean RB_CheckBoxInView(rbView_t *view, fixed_t *box, const fixed_t z1, const fixed_t z2);
void RB_ProjectPointToView(rbView_t *view, fixed_t fx, fixed_t fy, fixed_t fz, float *out_x, float *out_y);
void RB_RenderPlayerView(player_t *player);

#endif
