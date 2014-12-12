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

#ifndef _R_DRAWLIST_H_
#define _R_DRAWLIST_H_

#include "doomtype.h"
#include "rb_main.h"

typedef enum
{
    DLF_CEILING = BIT(0)
} drawlistflag_e;

typedef enum
{
    DLT_WALL,
    DLT_MASKEDWALL,
    DLT_TRANSWALL,
    DLT_BRIGHT,
    DLT_BRIGHTMASKED,
    DLT_FLAT,
    DLT_SPRITE,
    DLT_SPRITEALPHA,
    DLT_SPRITEBRIGHT,
    DLT_SPRITEOUTLINE,
    DLT_AMAP,
    DLT_SKY,
    DLT_CLIPLINE,
    DLT_DECAL,
    DLT_LIGHTMAP,
    DLT_DYNLIGHT,
    NUMDRAWLISTS
} drawlisttag_e;

typedef struct vtxlist_s
{
    void            *data;
    boolean         (*procfunc)(struct vtxlist_s*, int*);
    boolean         (*preprocess)(struct vtxlist_s*);
    void            (*postprocess)(struct vtxlist_s*, int*);
    dtexture        texid;
    int             flags;
    int             params;
    float           fparams;
    drawlisttag_e   drawTag;
} vtxlist_t;

typedef struct
{
    vtxlist_t       *list;
    int             index;
    int             max;
    drawlisttag_e   drawTag;
} drawlist_t;

extern drawlist_t drawlist[NUMDRAWLISTS];

vtxlist_t *DL_AddVertexList(drawlist_t *dl);
int DL_GetDrawListSize(int tag);
void DL_BeginDrawList(void);
void DL_ProcessDrawList(int tag);
void DL_RenderDrawList(void);
void DL_Reset(int tag);
void DL_Init(void);

#endif
