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

#ifndef __RB_PATCH_H__
#define __RB_PATCH_H__

#include "v_patch.h"

void RB_PatchBufferInit(void);
void RB_SetPatchBufferPalette(void);
void RB_PatchBufferShutdown(void);
void RB_BlitPatch(int x, int y, patch_t *patch, byte alpha);
void RB_BlitBlock(int x, int y, int width, int height, byte *data, byte alpha);
void RB_DrawPatchBuffer(void);

#endif
