//
// Copyright(C) 2016 Night Dive Studios, Inc.
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
// DESCRIPTION: Steam platform services implementation
// AUTHORS: James Haley, Samuel Villarreal
//

#ifndef I_STEAMSERVICES_H__
#define I_STEAMSERVICES_H__

#ifdef _USE_STEAM_

// Supports screenshots
#define I_APPSERVICES_SCREENSHOTS 1

// Networking is supported
#define I_APPSERVICES_NETWORKING 1

// Has broken old configuration dir
#define I_APPSERVICES_OLDCFGDIR 1

// Platform name
#define I_APPSERVICES_PLATFORMNAME "Steam"

extern IAppServices SteamAppServices;

#endif

#endif

// EOF

