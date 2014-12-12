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

#ifndef __RB_GEOM_H__
#define __RB_GEOM_H__

#include "r_defs.h"
#include "rb_drawlist.h"

typedef enum
{
    WS_MIDDLE   = 0,
    WS_UPPER,
    WS_LOWER,
    WS_MIDDLEBACK,
    NUMWALLSIDES
} rbWallSide_t;

void RB_GetSideTopBottom(sector_t *sector, float *top, float *bottom);

boolean RB_GenerateLowerSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateUpperSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateMiddleSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateSubSectors(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateSpritePlane(vtxlist_t* vl, int* drawcount);
boolean RB_GenerateClipLine(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateSkyLine(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateLightmapLowerSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateLightmapUpperSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateLightmapMiddleSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateLightMapFlat(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateDynLightUpperSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateDynLightLowerSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateDynLightMiddleSeg(vtxlist_t *vl, int *drawcount);
boolean RB_GenerateDynLightFlat(vtxlist_t *vl, int *drawcount);

void RB_DynLightPostProcess(vtxlist_t *vl, int *drawcount);

boolean RB_PreProcessSeg(vtxlist_t* vl);
boolean RB_PreProcessSubsector(vtxlist_t* vl);
boolean RB_PreProcessSprite(vtxlist_t* vl);
boolean RB_PreProcessClipLine(vtxlist_t* vl);
boolean RB_PreProcessLightmapSeg(vtxlist_t* vl);
boolean RB_PreProcessLightMapFlat(vtxlist_t* vl);

#endif
