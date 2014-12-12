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

#ifndef __RB_LEVEL_H__
#define __RB_LEVEL_H__

#include "doomtype.h"

#define gNd2    0x32644E67

#define GL_VERT_OFFSET 4

enum
{
    ML_GL_LABEL = 0,    // A separator name, GL_ExMx or GL_MAPxx
    ML_GL_VERTS,        // Extra Vertices
    ML_GL_SEGS,         // Segs, from linedefs & minisegs
    ML_GL_SSECT,        // SubSectors, list of segs
    ML_GL_NODES,        // GL BSP nodes
    ML_GL_PVS           // PVS Portals
};

enum
{
    ML_LM_LABEL = 0,
    ML_LM_CELLS,
    ML_LM_SUN,
    ML_LM_SURFS,
    ML_LM_TXCRD,
    ML_LM_LMAPS
};

typedef struct
{
    int         x;
    int         y;
} PACKEDATTR glVert_t;

typedef struct
{
    word        v1;
    word        v2;      
    word        linedef;
    int16_t     side;
    word        partner;
} PACKEDATTR glSeg_t;

typedef enum 
{
    SFT_UNKNOWN     = 0,
    SFT_MIDDLESEG,
    SFT_UPPERSEG,
    SFT_LOWERSEG,
    SFT_CEILING,
    SFT_FLOOR
} surfaceType_t;

typedef struct
{
    int16_t     type;
    int16_t     typeIndex;
    int16_t     lightmapNum;
    int16_t     numCoords;
    int32_t     coordOffset;
} PACKEDATTR mapSurface_t;

typedef struct
{
    int32_t     count;
    int16_t     min[3];
    int16_t     max[3];
    int16_t     gridSize[3];
    int16_t     blockSize[3];
} PACKEDATTR mapLightGrid_t;

#endif
