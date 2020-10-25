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
// DESCRIPTION:
//    Doom data to OpenGL texture management
//

#include "rb_data.h"
#include "rb_texture.h"
#include "rb_decal.h"
#include "rb_hudtext.h"
#include "rb_draw.h"
#include "rb_sky.h"
#include "r_data.h"
#include "r_draw.h"
#include "w_wad.h"
#include "z_zone.h"
#include "deh_str.h"
#include "i_swap.h"
#include "p_local.h"

rbTexture_t *lightmapTextures;
int lightmapCount;

typedef struct
{
    rbTexture_t     texture;
    rbTexture_t     brightmap;
    rbTexture_t     outline;
    unsigned int    flags;
} rbTextureData_t;

static rbTextureData_t  *colTextures;
static rbTextureData_t  *flatTextures;
static rbTextureData_t  *spriteTextures[8];
static rbTextureData_t  *patchTextures;

static int playpallump;
static boolean bInitialized = false;

extern SDL_Window *windowscreen;

//
// RB_GetScreenBufferData
//

byte *RB_GetScreenBufferData(void)
{
    SDL_Surface *screen = SDL_GetWindowSurface(windowscreen);
    int col;
    int pack;
    int i, j;
    int offset1;
    int offset2;
    byte *buffer;
    byte *tmpBuffer;

    buffer = (byte*)Z_Calloc(1, (screen->w * screen->h) * 3, PU_STATIC, 0);

    col = screen->w * 3;
    tmpBuffer = (byte*)Z_Calloc(1, col, PU_STATIC, 0);

    dglGetIntegerv(GL_PACK_ALIGNMENT, &pack);
    dglPixelStorei(GL_PACK_ALIGNMENT, 1);
    dglFlush();
    dglReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    dglPixelStorei(GL_PACK_ALIGNMENT, pack);

    // flip image vertically
    for(i = 0; i < screen->h / 2; ++i)
    {
        for(j = 0; j < col; ++j)
        {
            offset1 = (i * col) + j;
            offset2 = ((screen->h - (i + 1)) * col) + j;

            tmpBuffer[j] = buffer[offset1];
            buffer[offset1] = buffer[offset2];
            buffer[offset2] = tmpBuffer[j];
        }
    }

    Z_Free(tmpBuffer);
    return buffer;
}

//
// RB_DataInitialized
//

boolean RB_DataInitialized(void)
{
    return bInitialized;
}

//
// RB_InitData
//

void RB_InitData(void)
{
    int i;

    colTextures     = (rbTextureData_t*)Z_Calloc(1, sizeof(rbTextureData_t) * numtextures, PU_STATIC, 0);
    flatTextures    = (rbTextureData_t*)Z_Calloc(1, sizeof(rbTextureData_t) * numflats, PU_STATIC, 0);
    patchTextures   = (rbTextureData_t*)Z_Calloc(1,  sizeof(rbTextureData_t) * numlumps, PU_STATIC, 0);

    for(i = 0; i < 8; i++)
    {
        spriteTextures[i] = (rbTextureData_t*)Z_Calloc(1, sizeof(rbTextureData_t) * numspritelumps, PU_STATIC, 0);
    }

    playpallump = W_GetNumForName(DEH_String("PLAYPAL"));
    bInitialized = true;

    RB_InitDecals();
    RB_HudTextInit();
    RB_InitExtraHudTextures();
    RB_InitSky();

    // bind a dummy texture
    RB_BindTexture(&whiteTexture);
}

//
// RB_DeleteTextureData
//

void RB_DeleteTextureData(rbTextureData_t *texData)
{
    rbTexture_t *rbTexture;

    rbTexture = &texData->texture;
    RB_DeleteTexture(rbTexture);

    rbTexture = &texData->brightmap;
    RB_DeleteTexture(rbTexture);

    rbTexture = &texData->outline;
    RB_DeleteTexture(rbTexture);
}

//
// RB_DeleteDoomData
//

void RB_DeleteDoomData(void)
{
    int i;

    for(i = 0; i < numtextures; ++i)
    {
        RB_DeleteTextureData(&colTextures[texturetranslation[i]]);
    }

    for(i = 0; i < numflats; ++i)
    {
        RB_DeleteTextureData(&flatTextures[i]);
    }

    for(i = 0; i < 8; ++i)
    {
        int j;

        for(j = 0; j < numspritelumps; ++j)
        {
            RB_DeleteTextureData(&spriteTextures[i][j]);
        }
    }

    RB_DeleteExtraHudTextures();
    RB_DeleteSkyTextures();
}

//
// RB_DeleteData
//

void RB_DeleteData(void)
{
    RB_DeleteDoomData();

    if(patchTextures != NULL)
    {
        int i;

        for(i = 0; i < numlumps; ++i)
        {
            if(patchTextures[i].texture.texid > 0)
            {
                RB_DeleteTexture(&patchTextures[i].texture);
            }
        }
    }

    RB_FreeLightmapTextures();
}

//
// RB_CreateBrightMap1
//

void RB_CreateBrightMap1(rbTextureData_t *texdata, byte *data, byte *paldata,
                         patch_t *patch, const int translation)
{
    int w;
    int h;
    rbTexture_t *brightmap;
    rbTexture_t *texture;
    column_t *column;
    byte *colData;
    byte  rgb[256][3];
    byte  rgbf[32];

    memset(rgbf, 0, sizeof(rgbf));

    brightmap = &texdata->brightmap;

    if(brightmap->texid != 0)
    {
        return;
    }

    texdata->flags |= TDF_BRIGHTMAP;

    texture = &texdata->texture;

    memcpy(brightmap, texture, sizeof(rbTexture_t));
    brightmap->texid = 0;

    for(w = 0; w < texture->origwidth; ++w)
    {
        column = (column_t*)((byte*)patch + LONG(patch->columnofs[w]));
        while(column->topdelta != 0xff)
        {
            colData = (byte*)column + 3;

            for(h = 0; h < column->length; ++h)
            {
                int ch;
                byte p = colData[h];
                int bytenum = p >> 3;
                int bitnum  = 1 << (p & 7);

                if(!(rgbf[bytenum] & bitnum))
                {
                    RB_GetPaletteRGB(rgb[p], paldata, p, translation);
                    rgbf[bytenum] |= bitnum;
                }

                ch = column->topdelta + h;

                if(p < 224)
                {
                    data[((texture->width * ch) + w) * 4 + 0] = 0;
                    data[((texture->width * ch) + w) * 4 + 1] = 0;
                    data[((texture->width * ch) + w) * 4 + 2] = 0;
                    data[((texture->width * ch) + w) * 4 + 3] = 0;
                }
                else
                {
                    data[((texture->width * ch) + w) * 4 + 0] = rgb[p][0];
                    data[((texture->width * ch) + w) * 4 + 1] = rgb[p][1];
                    data[((texture->width * ch) + w) * 4 + 2] = rgb[p][2];
                    data[((texture->width * ch) + w) * 4 + 3] = 0xff;
                }
            }

            column = (column_t*)((byte*)column + column->length + 4);
        }
    }

    RB_UploadTexture(brightmap, data, TC_REPEAT, TF_NEAREST);
}

//
// RB_CreateBrightMap2
//

void RB_CreateBrightMap2(rbTextureData_t *texdata, byte *data,
                         texture_t *texture, byte *paldata, const int idx)
{
    byte *colData;
    rbTexture_t *brightmap;
    rbTexture_t *rbTexture;
    int w;
    int h;
    byte rgb[256][3];
    byte rgbf[32];

    memset(rgbf, 0, sizeof(rgbf));

    brightmap = &texdata->brightmap;

    if(brightmap->texid != 0)
    {
        return;
    }

    texdata->flags |= TDF_BRIGHTMAP;

    rbTexture = &texdata->texture;

    memcpy(brightmap, rbTexture, sizeof(rbTexture_t));
    brightmap->texid = 0;

    for(w = 0; w < texture->width; ++w)
    {
        colData = R_GetColumn(idx, w);
        for(h = 0; h < texture->height; ++h)
        {
            byte p = colData[h];
            int bytenum = p >> 3;
            int bitnum  = 1 << (p & 7);

            if(!(rgbf[bytenum] & bitnum))
            {
               RB_GetPaletteRGB(rgb[p], paldata, p, 0);
               rgbf[bytenum] |= bitnum;
            }

            if(colData[h] < 224)
            {
                data[((rbTexture->width * h) + w) * 4 + 0] = 0;
                data[((rbTexture->width * h) + w) * 4 + 1] = 0;
                data[((rbTexture->width * h) + w) * 4 + 2] = 0;
                data[((rbTexture->width * h) + w) * 4 + 3] = 0;
            }
            else
            {
                data[((rbTexture->width * h) + w) * 4 + 0] = rgb[p][0];
                data[((rbTexture->width * h) + w) * 4 + 1] = rgb[p][1];
                data[((rbTexture->width * h) + w) * 4 + 2] = rgb[p][2];
                data[((rbTexture->width * h) + w) * 4 + 3] = 0xff;
            }
        }
    }

    RB_UploadTexture(brightmap, data, TC_REPEAT, TF_NEAREST);
}

//
// RB_ReadTextureData
// Width and height for rbTexture should already be set
//

void RB_ReadTextureData(rbTextureData_t *texdata, texture_t *texture, byte *paldata, const int idx)
{
    byte *data;
    byte *colData;
    rbTexture_t *rbTexture;
    byte bMakeBrightmap;
    int w;
    int h;
    byte rgb[256][3];
    byte rgbf[32];

    memset(rgbf, 0, sizeof(rgbf));

    rbTexture = &texdata->texture;

    data = (byte*)malloc((rbTexture->width * rbTexture->height) * 4);
    memset(data, 0, (rbTexture->width * rbTexture->height) * 4);

    bMakeBrightmap = 0;

    rbTexture->colorMode = TCR_RGBA;

    for(w = 0; w < texture->width; ++w)
    {
        colData = R_GetColumn(idx, w);
        for(h = 0; h < texture->height; ++h)
        {
            byte p = colData[h];
            int bytenum = p >> 3;
            int bitnum  = 1 << (p & 7);

            if(!(rgbf[bytenum] & bitnum))
            {
                bMakeBrightmap |= RB_GetPaletteRGB(rgb[p], paldata, p, 0);
                rgbf[bytenum] |= bitnum;
            }

            data[((rbTexture->width * h) + w) * 4 + 0] = rgb[p][0];
            data[((rbTexture->width * h) + w) * 4 + 1] = rgb[p][1];
            data[((rbTexture->width * h) + w) * 4 + 2] = rgb[p][2];
            data[((rbTexture->width * h) + w) * 4 + 3] = 0xff;
        }
    }

    RB_UploadTexture(rbTexture, data, TC_REPEAT, TF_NEAREST);

    if(bMakeBrightmap)
    {
        RB_CreateBrightMap2(texdata, data, texture, paldata, idx);
    }

    free(data);
}

//
// RB_ReadPatchData
// Width and height for rbTexture should already be set
//

void RB_ReadPatchData(rbTextureData_t *texdata, byte *paldata, patch_t *patch,
                      const int translation, const int index, rbDataType_t type)
{
    int w;
    int h;
    byte bMakeBrightmap;
    rbTexture_t *rbTexture;
    column_t *column;
    byte *data;
    byte *colData;
    byte rgb[256][3];
    byte rgbf[32];

    memset(rgbf, 0, sizeof(rgbf));

    rbTexture = (type == RDT_SPRITEOUTLINE) ? &texdata->outline : &texdata->texture;

    data = (byte*)malloc((rbTexture->width * rbTexture->height) * 4);
    memset(data, 0, (rbTexture->width * rbTexture->height) * 4);

    bMakeBrightmap = 0;

    for(w = 0; w < rbTexture->origwidth; ++w)
    {
        column = (column_t*)((byte*)patch + LONG(patch->columnofs[w]));

        if(column->length != rbTexture->origheight)
        {
            texdata->flags |= TDF_MASKED;
        }

        while(column->topdelta != 0xff)
        {
            colData = (byte*)column + 3;

            for(h = 0; h < column->length; ++h)
            {
                int ch;
                byte p = colData[h];
                int bytenum = p >> 3;
                int bitnum  = 1 << (p & 7);

                if(type == RDT_SPRITEOUTLINE)
                {
                    rgb[p][0] = rgb[p][1] = rgb[p][2] = 0xff;
                }
                else
                {
                    if(!(rgbf[bytenum] & bitnum))
                    {
                        bMakeBrightmap |= RB_GetPaletteRGB(rgb[p], paldata, p, translation);
                        rgbf[bytenum] |= bitnum;
                    }
                }

                ch = column->topdelta + h;

                data[((rbTexture->width * ch) + w) * 4 + 0] = rgb[p][0];
                data[((rbTexture->width * ch) + w) * 4 + 1] = rgb[p][1];
                data[((rbTexture->width * ch) + w) * 4 + 2] = rgb[p][2];
                data[((rbTexture->width * ch) + w) * 4 + 3] = 0xff;
            }

            column = (column_t*)((byte*)column + column->length + 4);
        }
    }

    rbTexture->colorMode = TCR_RGBA;

    RB_UploadTexture(rbTexture, data, TC_REPEAT, TF_NEAREST);

    if(bMakeBrightmap)
    {
        RB_CreateBrightMap1(texdata, data, paldata, patch, translation);
    }

    free(data);
}

//
// RB_CreateColTexture
//

rbTextureData_t *RB_CreateColTexture(const int index)
{
    int             idx;
    texture_t       *texture;
    rbTextureData_t *texdata;
    rbTexture_t     *rbTexture;
    byte            *paldata;

    if(index == 0 || !bInitialized)
    {
        // blank texture?
        return NULL;
    }

    idx = texturetranslation[index];
    texdata = &colTextures[idx];
    rbTexture = &texdata->texture;

    if(rbTexture->texid != 0)
    {
        return texdata;
    }

    texture = textures[idx];
    paldata = (byte*)W_CacheLumpNum(playpallump, PU_CACHE);

    rbTexture->origwidth = texture->width;
    rbTexture->origheight = texture->height;

    rbTexture->width = RB_RoundPowerOfTwo(texture->width);
    // we should be good on height. we only care about width being in powers of 2
    rbTexture->height = texture->height;

    // as far as I know, textures with multiple patches are never masked so
    // the length per column should always be known
    if(texture->patchcount > 1)
    {
        RB_ReadTextureData(texdata, texture, paldata, index);
    }
    else
    {
        patch_t *patch = (patch_t*)W_CacheLumpNum(texture->patches[0].patch, PU_CACHE);

        // texture could be masked, so walk through each column and check
        RB_ReadPatchData(texdata, paldata, patch, 0, index, RDT_COLUMN);
    }

    return texdata;
}

//
// RB_CreateFlatTexture
//

rbTextureData_t *RB_CreateFlatTexture(const int index)
{
    int             idx;
    int             i;
    int             size;
    rbTextureData_t *texdata;
    rbTexture_t     *rbTexture;
    byte            *paldata;
    byte            *data;
    byte            *flatData;
    byte            bMakeBrightmap;
    byte            rgb[256][3];
    byte            rgbf[32];

    if(index == 0 || !bInitialized)
    {
        // blank texture?
        return NULL;
    }

    idx = flattranslation[index];
    texdata = &flatTextures[idx];
    rbTexture = &texdata->texture;

    if(rbTexture->texid != 0)
    {
        return texdata;
    }

    paldata = (byte*)W_CacheLumpNum(playpallump, PU_CACHE);
    size = lumpinfo[firstflat + idx].size;

    bMakeBrightmap = 0;

    rbTexture->origwidth = 64;
    rbTexture->origheight = 64;
    rbTexture->width = 64;
    rbTexture->height = 64;
    rbTexture->colorMode = TCR_RGBA;

    data = (byte*)malloc((rbTexture->width * rbTexture->height) * 4);
    memset(data, 0, (rbTexture->width * rbTexture->height) * 4);

    flatData = (byte*)W_CacheLumpNum(firstflat + idx, PU_CACHE);

    memset(rgbf, 0, sizeof(rgbf));

    for(i = 0; i < size; i++)
    {
        byte p = flatData[i];
        int bytenum = p >> 3;
        int bitnum = 1 << (p & 7);

        if(!(rgbf[bytenum] & bitnum))
        {
            bMakeBrightmap |= RB_GetPaletteRGB(rgb[p], paldata, p, 0);
            rgbf[bytenum] |= bitnum;
        }

        data[i * 4 + 0] = rgb[p][0];
        data[i * 4 + 1] = rgb[p][1];
        data[i * 4 + 2] = rgb[p][2];
        data[i * 4 + 3] = 0xff;
    }

    RB_UploadTexture(rbTexture, data, TC_REPEAT, TF_NEAREST);

    if(bMakeBrightmap)
    {
        rbTexture_t *brightmap = &texdata->brightmap;

        if(brightmap->texid == 0)
        {
            memcpy(brightmap, rbTexture, sizeof(rbTexture_t));
            brightmap->texid = 0;

            for(i = 0; i < size; i++)
            {
                byte p = flatData[i];

                if(p < 224)
                {
                    data[i * 4 + 0] = 0;
                    data[i * 4 + 1] = 0;
                    data[i * 4 + 2] = 0;
                    data[i * 4 + 3] = 0;
                }
                else
                {
                    data[i * 4 + 0] = rgb[p][0];
                    data[i * 4 + 1] = rgb[p][1];
                    data[i * 4 + 2] = rgb[p][2];
                    data[i * 4 + 3] = 0xff;
                }
            }

            texdata->flags |= TDF_BRIGHTMAP;
            RB_UploadTexture(brightmap, data, TC_REPEAT, TF_NEAREST);
        }
    }

    free(data);

    return texdata;
}

//
// RB_CreateSpriteTexture
//

rbTextureData_t *RB_CreateSpriteTexture(const int index, const int translation, boolean outline)
{
    rbTextureData_t *texdata;
    rbTexture_t     *rbTexture;
    byte            *paldata;
    patch_t         *patch;

    if(index < 0 || !bInitialized)
    {
        // blank texture?
        return NULL;
    }

    texdata = &spriteTextures[translation][index];
    rbTexture = outline ? &texdata->outline : &texdata->texture;

    if(rbTexture->texid != 0)
    {
        return texdata;
    }

    paldata = (byte*)W_CacheLumpNum(playpallump, PU_CACHE);
    patch = (patch_t*)W_CacheLumpNum(firstspritelump + index, PU_CACHE);

    rbTexture->colorMode = TCR_RGBA;
    rbTexture->origwidth = SHORT(patch->width);
    rbTexture->origheight = SHORT(patch->height);

    rbTexture->width = RB_RoundPowerOfTwo(rbTexture->origwidth);
    rbTexture->height = RB_RoundPowerOfTwo(rbTexture->origheight);

    RB_ReadPatchData(texdata, paldata, patch, translation, index,
        outline ? RDT_SPRITEOUTLINE : RDT_SPRITE);
    return texdata;
}

//
// RB_CreatePatchTexture
//

rbTextureData_t *RB_CreatePatchTexture(const int index)
{
    rbTextureData_t *texdata;
    rbTexture_t     *rbTexture;
    byte            *paldata;
    patch_t         *patch;

    if(index == 0 || !bInitialized)
    {
        // blank texture?
        return NULL;
    }

    texdata = &patchTextures[index];
    rbTexture = &texdata->texture;

    if(rbTexture->texid != 0)
    {
        return texdata;
    }

    paldata = (byte*)W_CacheLumpNum(playpallump, PU_CACHE);
    patch = (patch_t*)W_CacheLumpNum(index, PU_CACHE);

    rbTexture->colorMode = TCR_RGBA;
    rbTexture->origwidth = SHORT(patch->width);
    rbTexture->origheight = SHORT(patch->height);

    rbTexture->width = RB_RoundPowerOfTwo(rbTexture->origwidth);
    rbTexture->height = RB_RoundPowerOfTwo(rbTexture->origheight);

    RB_ReadPatchData(texdata, paldata, patch, 0, index, RDT_PATCH);
    return texdata;
}

//
// RB_GetTextureFlags
//

unsigned int RB_GetTextureFlags(const rbDataType_t type, const int index, const int translation)
{
    switch(type)
    {
    case RDT_COLUMN:
        return colTextures[texturetranslation[index]].flags;

    case RDT_FLAT:
        return flatTextures[flattranslation[index]].flags;

    case RDT_SPRITE:
        return spriteTextures[translation][index].flags;

    case RDT_PATCH:
        return patchTextures[index].flags;

    default:
        break;
    }

    return 0;
}

//
// RB_GetTexture
//

rbTexture_t *RB_GetTexture(const rbDataType_t type, const int index, const int translation)
{
    rbTextureData_t *texdata = NULL;

    switch(type)
    {
    case RDT_COLUMN:
        texdata = RB_CreateColTexture(index);
        break;

    case RDT_FLAT:
        texdata = RB_CreateFlatTexture(index);
        break;

    case RDT_SPRITE:
        texdata = RB_CreateSpriteTexture(index, translation, false);
        break;

    case RDT_PATCH:
        texdata = RB_CreatePatchTexture(index);
        break;

    default:
        break;
    }

    if(texdata)
    {
        return &texdata->texture;
    }

    return NULL;
}

//
// RB_GetBrightmap
//

rbTexture_t *RB_GetBrightmap(const rbDataType_t type, const int index, const int translation)
{
    switch(type)
    {
    case RDT_COLUMN:
        if(colTextures[texturetranslation[index]].brightmap.texid != 0)
        {
            return &colTextures[texturetranslation[index]].brightmap;
        }
        break;

    case RDT_FLAT:
        if(flatTextures[flattranslation[index]].brightmap.texid != 0)
        {
            return &flatTextures[flattranslation[index]].brightmap;
        }
        break;

    case RDT_SPRITE:
        if(spriteTextures[translation][index].brightmap.texid != 0)
        {
            return &spriteTextures[translation][index].brightmap;
        }
        break;

    default:
        break;
    }

    return NULL;
}

//
// RB_GetSpriteOutlineTexture
//

rbTexture_t *RB_GetSpriteOutlineTexture(const int index)
{
    rbTextureData_t *texdata = RB_CreateSpriteTexture(index, 0, true);

    if(texdata)
    {
        return &texdata->outline;
    }

    return NULL;
}

//
// RB_InitLightmapTextures
//

void RB_InitLightmapTextures(byte *data, int count, int width, int height)
{
    int i;
    byte *textureData;

    lightmapTextures = (rbTexture_t*)Z_Calloc(1, sizeof(rbTexture_t) * count, PU_LEVEL, 0);
    lightmapCount = count;

    for(i = 0; i < count; ++i)
    {
        lightmapTextures[i].colorMode = TCR_RGB;
        lightmapTextures[i].origwidth = width;
        lightmapTextures[i].origheight = height;
        lightmapTextures[i].width = width;
        lightmapTextures[i].height = height;

        textureData = &data[(width * height * 3) * i];
        RB_UploadTexture(&lightmapTextures[i], textureData, TC_CLAMP_BORDER, TF_LINEAR);
    }
}

//
// RB_FreeLightmapTextures
//

void RB_FreeLightmapTextures(void)
{
    int i;

    if(lightmapTextures)
    {
        for(i = 0; i < lightmapCount; ++i)
        {
            RB_DeleteTexture(&lightmapTextures[i]);
        }
    }
}

//
// RB_PrecacheLevel
//

void RB_PrecacheLevel(void)
{
    char *present;
    int i, j, k;
    thinker_t *th;
    anim_t *anim;
    
    present = (char*)Z_Calloc(1, numflats, PU_STATIC, 0);
    
    for(i = 0; i < numsectors; ++i)
    {
        present[sectors[i].floorpic] = 1;
        present[sectors[i].ceilingpic] = 1;
        
        for(anim = anims; anim < lastanim; ++anim)
        {
            if(!anim->istexture)
            {
                if(anim->basepic == sectors[i].floorpic)
                {
                    for(k = 0; k < anim->numpics; ++k)
                    {
                        present[anim->basepic + k] = 1;
                    }
                }
            }
        }
    }
    
    for(i = 0; i < numflats; ++i)
    {
        if(present[i])
        {
            RB_CreateFlatTexture(i);
        }
    }
    
    Z_Free(present);
    
    present = (char*)Z_Calloc(1, numtextures, PU_STATIC, 0);
    
    for(i = 0; i < numsides; ++i)
    {
        present[sides[i].toptexture] = 1;
        present[sides[i].midtexture] = 1;
        present[sides[i].bottomtexture] = 1;
        
        for(anim = anims; anim < lastanim; ++anim)
        {
            if(anim->istexture)
            {
                if(anim->basepic == sides[i].toptexture ||
                   anim->basepic == sides[i].midtexture ||
                   anim->basepic == sides[i].bottomtexture)
                {
                    for(k = 0; k < anim->numpics; ++k)
                    {
                        present[anim->basepic + k] = 1;
                    }
                }
            }
        }
    }
    
    for(i = 0; i < numtextures; ++i)
    {
        if(present[i])
        {
            RB_CreateColTexture(i);
        }
    }
    
    Z_Free(present);
    
    present = (char*)Z_Calloc(1, numsprites, PU_STATIC, 0);
    
    for(th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if(th->function.acp1 == (actionf_p1)P_MobjThinker)
        {
            present[((mobj_t*)th)->sprite] = 1;
        }
    }
    
    for(i = 0; i < numsprites; ++i)
    {
        if(present[i])
        {
            for(j = 0; j < sprites[i].numframes; ++j)
            {
                spriteframe_t *sf = &sprites[i].spriteframes[j];
                
                for(k = 0; k < 8; ++k)
                {
                    RB_CreateSpriteTexture(sf->lump[k], 0, false);
                }
            }
        }
    }
    
    Z_Free(present);
}
