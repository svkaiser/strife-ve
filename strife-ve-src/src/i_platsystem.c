//
// Copyright(C) 2005-2014 Simon Howard
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
//       System Platform code.
//

#include "i_platsystem.h"

int I_SetupPlatformQuit(void)
{
    return 0;
}

int I_DoPlatformQuit(void)
{
    return 0;
}

void I_FlushSaves(void)
{
}

const char* I_TempSaveDir(void)
{
    return "";
}
