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

#ifndef __RB_DECAL_H__
#define __RB_DECAL_H__

#include "r_defs.h"
#include "info.h"

#define NUM_DECAL_POINTS    16

typedef struct
{
    char        *lumpname;
    mobjtype_t  mobjtype;
    int         lifetime;
    int         fadetime;
    byte        startingAlpha;
    boolean     randRotate;
    float       scale;
    float       randScaleFactor;
    boolean     noterrain;
    boolean     isgore;
    int         count;
    int         lumpnum;
} rbDecalDef_t;

typedef struct
{
    float x;
    float y;
    float z;
    float tu;
    float tv;
} rbDecalVertex_t;

typedef enum
{
    DCT_FLOOR   = 0,
    DCT_CEILING,
    DCT_WALL,
    DCT_UPPERWALL,
    DCT_LOWERWALL,
    NUMDECALTYPES
} rbDecalType_t;

typedef struct rbDecal_s
{
    int                 tics;
    int                 fadetime;
    int                 lump;
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;
    float               rotation;
    float               scale;
    int                 offset;
    float               alpha;
    rbDecalVertex_t     points[NUM_DECAL_POINTS];
    int                 numpoints;
    struct sector_s     *stickSector;
    fixed_t             initialStickZ;
    rbDecalType_t       type;
    rbDecalDef_t        *def;
    struct subsector_s  *ssect;
    struct rbDecal_s    *prev;
    struct rbDecal_s    *next;
    struct rbDecal_s    *sprev;
    struct rbDecal_s    *snext;
} rbDecal_t;

void RB_InitDecals(void);
void RB_UpdateDecals(void);
void RB_ClearDecalLinks(void);
void RB_AddDecals(struct subsector_s *sub);
void RB_SpawnWallDecal(mobj_t *mobj);
void RB_SpawnFloorDecal(mobj_t *mobj, boolean floor);

#endif

