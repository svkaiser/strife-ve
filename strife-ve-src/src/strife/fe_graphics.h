//
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
//    Configuration and netgame negotiation frontend for
//    Strife: Veteran Edition
//
// AUTHORS:
//    James Haley
//    Samuel Villarreal (Hi-Res BG code, mouse pointer)
//

#ifndef FE_GRAPHICS_H_
#define FE_GRAPHICS_H_

void FE_WriteBigTextCentered(int y, const char *str);
void FE_WriteSmallTextCentered(int y, const char *str);
void FE_WriteYellowTextCentered(int y, const char *str);
void FE_ClearScreen(void);
void FE_DrawSigilCursor(int x, int y);
void FE_DrawLaserCursor(int x, int y);
void FE_DoWipe(void);
void FE_DrawBox(int left, int top, int w, int h);

enum
{
    FE_BG_SIGIL,
    FE_BG_RSKULL,
    FE_BG_TSKULL,
    FE_NUM_BGS
};

enum
{
    FE_SLIDER_LEFT,
    FE_SLIDER_RIGHT,
    FE_SLIDER_MIDDLE,
    FE_SLIDER_GEM,
    FE_SLIDER_NUMGFX
};

void FE_InitBackgrounds(void);
void FE_DestroyBackgrounds(void);
void FE_RefreshBackgrounds(void);
void FE_DrawBackground(int bgnum);

#endif

// EOF

