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

#ifndef __RB_DATA_H__
#define __RB_DATA_H__

#include "rb_common.h"
#include "rb_main.h"
#include "rb_texture.h"

typedef enum
{
    RDT_INVALID = 0,
    RDT_COLUMN,
    RDT_FLAT,
    RDT_SPRITE,
    RDT_SPRITEOUTLINE,
    RDT_PATCH,
    RDT_NUMTYPES
} rbDataType_t;

typedef enum
{
    TDF_MASKED      = BIT(0),
    TDF_BRIGHTMAP   = BIT(1)
} rbTexDataFlags_t;

extern rbTexture_t  *lightmapTextures;
extern int          lightmapCount;

byte *RB_GetScreenBufferData(void);
void RB_InitData(void);
void RB_DeleteDoomData(void);
void RB_DeleteData(void);
boolean RB_DataInitialized(void);
unsigned int RB_GetTextureFlags(const rbDataType_t type, const int index, const int translation);
rbTexture_t *RB_GetTexture(const rbDataType_t type, const int index, const int translation);
rbTexture_t *RB_GetBrightmap(const rbDataType_t type, const int index, const int translation);
rbTexture_t *RB_GetSpriteOutlineTexture(const int index);
void RB_InitLightmapTextures(byte *data, int count, int width, int height);
void RB_FreeLightmapTextures(void);
void RB_PrecacheLevel(void);

static dinline boolean RB_GetPaletteRGB(byte *rgb, byte *paldata, byte index, const int translation)
{
   extern int   usegamma;
   extern byte *translationtables;
   int transrc = index;

    if(translation-1 >= 0)
    {
        byte *trantable = translationtables + (256 * (translation-1));
        transrc = trantable[index];
    }

    if(rgb)
    {
        rgb[0] = gammatable[usegamma][paldata[transrc * 3 + 0]] & ~3;
        rgb[1] = gammatable[usegamma][paldata[transrc * 3 + 1]] & ~3;
        rgb[2] = gammatable[usegamma][paldata[transrc * 3 + 2]] & ~3;
    }

    return (transrc >= 224);
}

#endif
