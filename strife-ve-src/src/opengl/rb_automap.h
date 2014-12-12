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

#ifndef __RB_AUTOMAP_H__
#define __RB_AUTOMAP_H__

void RB_BeginAutomapDraw(void);
void RB_DrawAutomapSectors(boolean cheating);
void RB_EndAutomapDraw(void);
void RB_DrawObjectiveMarker(fixed_t x, fixed_t y);
void RB_DrawMark(fixed_t x, fixed_t y, int marknum);
void RB_DrawAutomapLine(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int color);

#endif
