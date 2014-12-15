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

#ifndef __RB_SKY_H__
#define __RB_SKY_H__

#include "r_sky.h"

void RB_InitSky(void);
void RB_DeleteSkyTextures(void);
void RB_DrawClouds(const int lump);
void RB_CloudTicker(void);
void RB_SetupSkyData(void);
void RB_DrawSky(void);
void RB_DrawSkyDome(int tiles, float rows, int height, int radius, float offset, float topoffs,
                    unsigned int c1, unsigned int c2);

#endif
