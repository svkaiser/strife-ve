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

#ifdef GOG_RELEASE

#include "SDL.h"

#include "i_social.h"
#include "gogService.h"

//
// The GOG Galaxy overlay does not filter some SDL event types properly,
// allowing them to pass on through to the application even after the overlay
// processes them. We need to instruct the game to ignore them when the
// overlay is active.
//
static int I_GalaxyOverlayEventFilter(unsigned int event)
{
    if(I_GalaxyOverlayActive())
    {
        switch(event)
        {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEMOTION:
            // ignore any mouse events that make it through
            return 1;
        default:
            break;
        }
    }
    return 0;
}

//
// Shift+tab is a mess with Galaxy. Without special treatment, keys will get
// stuck and shift tracking state will become inverted.
//
static int I_GalaxyOverlayEatsShiftTab(void)
{
    return 1;
}

IAppServices GalaxyAppServices =
{
    // Core
    I_GalaxyInit,
    I_GalaxyShutdown,
    I_GalaxyCheckForRestart,
    I_GalaxyUpdate,

    // Achievements
    I_GalaxySetAchievement,
    I_GalaxySetAchievementProgress,
    I_GalaxyHasAchievement,
   
    // P2P network driver
    I_GalaxySendPacket,
    I_GalaxyRecvPacket,
    I_GalaxyResolveAddress,
    
    // Lobby join listener
    I_GalaxyBeginLobbyJoinListener,
    I_GalaxyDestroyLobbyJoinListener,
    I_GalaxyLobbyJoinRequested,

    // Lobby
    I_GalaxyLobbyPollState,
    I_GalaxyCreateNewLobby,
    I_GalaxyJoinLobbyFromStartup,
    I_GalaxyJoinLobbyFromInvite,
    I_GalaxyLobbyCleanUp,
    I_GalaxyLeaveLobby,
    I_GalaxyLockLobby,
    I_GalaxyLobbyInviteFriends,
    I_GalaxyLobbyStartGame,
    I_GalaxyLobbyGetReady,
    I_GalaxyLobbyChangeReady,
    I_GalaxyLobbyGetUserReadyAt,
    I_GalaxyLobbyGetTeam,
    I_GalaxyLobbyChangeTeam,
    I_GalaxyLobbyGetTeamAt,
    I_GalaxyLobbyUserIsOwner,
    I_GalaxyLobbyGetNumMembers,
    I_GalaxyLobbyGetUserNameAt,
    I_GalaxyLobbyGetName,
    I_GalaxyLobbyUpdateName,
    I_GalaxyLobbyCheckStartGame,

    // Lobby manager
    I_GalaxyLobbyMgrRefresh,
    I_GalaxyLobbyMgrInRequest,
    I_GalaxyLobbyMgrGetCount,
    I_GalaxyLobbyMgrGetNameAt,
    I_GalaxyLobbyMgrJoinLobbyAt,

    // Server component
    I_GalaxyServerGetClientsFromLobby,

    // Client component
    I_GalaxyClientGetServerFromLobby,
    I_GalaxyClientGetServerAddr,

    // User
    I_GalaxyGetLocalUserName,

    // Overlay
    I_GalaxyOverlayActive,
    I_GalaxyOverlayEventFilter,
    I_GalaxyOverlayEatsShiftTab
};

#endif

// EOF

