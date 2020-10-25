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
// DESCRIPTION: Social/software deployment platform interface
// AUTHORS: James Haley, Samuel Villarreal
//

#include "i_social.h"
#include "i_noappservices.h"

// Global application services provider pointer
IAppServices *gAppServices;

//
// Call at startup as early as possible to set the app service provider
// to the proper instance for this build.
//
void I_InitAppServices(void)
{
#if defined(_USE_STEAM_)
    // Steam build
    gAppServices = &SteamAppServices;
#elif defined(GOG_RELEASE)
    // GOG Galaxy build
    gAppServices = &GalaxyAppServices;
#elif defined(SVE_PLAT_SWITCH)
    gAppServices = &SwitchAppServices;
#else
    // Free build
    // Use dummy provider
    gAppServices = &noAppServices;
#endif
}

// EOF

