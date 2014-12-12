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

#ifndef __RB_THINGS_H__
#define __RB_THINGS_H__

#include "r_defs.h"
#include "r_state.h"
#include "r_things.h"
#include "rb_matrix.h"
#include "rb_main.h"

typedef struct
{
    mobj_t *spr;
    fixed_t dist;
    int     indiceStart;
    float   x;
    float   y;
    float   z;
} rbVisSprite_t;

extern matrix rbSpriteViewMatrix;
extern matrix rbSpriteViewBillboardMatrix; // for very tall sprites

void RB_ClearSprites(void);
void RB_AddSprites(subsector_t *sub);
void RB_SetupSprites(void);
void RB_SetSpriteCellColor(vtx_t *v, fixed_t x, fixed_t y, fixed_t z, sector_t *sector);

#endif
