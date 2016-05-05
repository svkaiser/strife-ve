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
// DESCRIPTION: GOG Galaxy platform services implementation
// AUTHORS: James Haley, Samuel Villarreal
//

#ifndef I_GALAXYSERVICES_H__
#define I_GALAXYSERVICES_H__

#ifdef GOG_RELEASE

// Networking is supported
#define I_APPSERVICES_NETWORKING 1

// Platform name
#define I_APPSERVICES_PLATFORMNAME "GOG Galaxy"

extern IAppServices GalaxyAppServices;

#endif

#endif

// EOF

