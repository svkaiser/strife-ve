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

#ifndef __RB_TEXTURE_H__
#define __RB_TEXTURE_H__

#include "rb_main.h"

typedef struct
{
    int x;
    int y;
    int w;
    int h;
} atlas_t;

typedef enum
{
    TC_CLAMP    = 0,
    TC_CLAMP_BORDER,
    TC_REPEAT,
    TC_MIRRORED
} texClampMode_t;

typedef enum
{
    TF_LINEAR   = 0,
    TF_NEAREST
} texFilterMode_t;

typedef enum {
    TCR_RGB     = 0,
    TCR_RGBA
} texColorMode_t;

typedef struct {
    int                 width;
    int                 height;
    int                 origwidth;
    int                 origheight;
    texClampMode_t      clampMode;
    texFilterMode_t     filterMode;
    texColorMode_t      colorMode;
    dtexture            texid;
} rbTexture_t;

#define TEXFILTER   (rbLinearFiltering == false) ? TF_NEAREST : TF_LINEAR

int RB_RoundPowerOfTwo(int x);
void RB_UploadTexture(rbTexture_t *rbTexture, byte *data, texClampMode_t clamp, texFilterMode_t filter);
void RB_SetTexParameters(rbTexture_t *rbTexture);
void RB_ChangeTexParameters(rbTexture_t *rbTexture, const texClampMode_t clamp, const texFilterMode_t filter);
void RB_BindTexture(rbTexture_t *rbTexture);
void RB_UnbindTexture(void);
void RB_DeleteTexture(rbTexture_t *rbTexture);
void RB_UpdateTexture(rbTexture_t *rbTexture, byte *data);
void RB_BindFrameBuffer(rbTexture_t *rbTexture);
void RB_BindDepthBuffer(rbTexture_t *rbTexture);

#endif
