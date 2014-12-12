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

#ifndef __RB_LIGHTGRID_H__
#define __RB_LIGHTGRID_H__

int RB_GetLightGridIndex(fixed_t x, fixed_t y, fixed_t z);
void RB_ApplyLightGridRGB(vtx_t *vtx, int index, boolean insky);
void RB_DrawLightGridCell(int index);

#endif
