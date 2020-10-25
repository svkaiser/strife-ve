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
//      System-specific Platform System behaviour
//


#ifndef __I_PLATSYSTEM__
#define __I_PLATSYSTEM__

int I_SetupPlatformQuit(void);
int I_DoPlatformQuit(void);
const char* I_TempSaveDir(void);
void I_FlushSaves(void);

#endif /* #ifndef __I_PLATSYSTEM__ */

