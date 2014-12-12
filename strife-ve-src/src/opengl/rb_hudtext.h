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

#ifndef __RB_HUDTEXT_H__
#define __RB_HUDTEXT_H__

void RB_HudTextInit(void);
void RB_HudTextShutdown(void);
float RB_HudTextWidth(const char *text, const float scale);
void RB_HudTextDraw(int x, int y, const float scale, const int alpha, const char *text);

#endif
