//
// Copyright(C) 2003 Tim Stump
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

#ifndef R_CLIPPER_H
#define R_CLIPPER_H

boolean     RB_Clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void        RB_Clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void        RB_Clipper_Clear(void);

#endif