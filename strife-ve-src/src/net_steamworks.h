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
// Steamworks Networking Interface
//

#ifndef NET_STEAMWORKS_H__
#define NET_STEAMWORKS_H__

#include "i_social.h"

#ifdef I_APPSERVICES_NETWORKING
extern net_module_t net_steamworks_module;
#endif

enum net_steamnode_e
{
    NET_STEAM_CLIENT,
    NET_STEAM_SERVER
};

extern boolean  net_SteamGame;     // a Steam-negotiated netgame is under way
extern int      net_SteamNodeType; // either NET_STEAM_CLIENT or NET_STEAM_SERVER
extern int      net_SteamNumNodes; // number of expected nodes
extern char    *net_SteamServerID; // ID of server if we are a client

#endif

// EOF

