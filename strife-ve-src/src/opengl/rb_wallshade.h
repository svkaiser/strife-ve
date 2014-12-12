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

#ifndef __RB_WALLSHADE_H__
#define __RB_WALLSHADE_H__

#include "rb_main.h"
#include "rb_geom.h"
#include "r_defs.h"
#include "d_player.h"

void RB_InitWallShades(void);
void RB_SetSectorShades(sector_t *sec);
void RB_SetSkyShade(byte r, byte g, byte b);
void RB_SetThingShade(mobj_t *thing, vtx_t *vtx, int lightlevel);
void RB_SetPspriteShade(player_t *player, vtx_t *vtx, int lightlevel);
boolean RB_SetWallShade(vtx_t *vtx, int lightlevel, seg_t *seg, rbWallSide_t side);
boolean RB_GetFloorShade(sector_t *sector, byte *r, byte *g, byte *b);
boolean RB_GetCeilingShade(sector_t *sector, byte *r, byte *g, byte *b);

#endif
