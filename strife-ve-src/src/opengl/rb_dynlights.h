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

#ifndef __RB_DYNLIGHTS_H__
#define __RB_DYNLIGHTS_H__

#define MAX_DYNLIGHTS   32

typedef struct
{
    mobj_t      *thing;
    float       x;
    float       y;
    float       z;
    float       radius;
    byte        rgb[3];
    fixed_t     bbox[4];
} rbDynLight_t;

void RB_InitDynLights(void);
void RB_InitLightMarks(void);
const int RB_GetDynLightCount(void);
void RB_AddDynLights(void);
rbDynLight_t *RB_GetDynLight(const int num);
unsigned int RB_SubsectorMarked(const int num);

#endif
