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

#ifdef _USE_STEAM_

#include "i_social.h"
#include "steamService.h"

static int I_SteamOverlayEventFilter(unsigned int event)
{
    // we do not need to filter any events for the Steam overlay;
    // it absorbs input properly in all cases we don't already deal with.
    return 0;
}

static int I_SteamOverlayEatsShiftTab(void)
{
    // Shift+Tab is handled properly in Steam, nothing special is needed.
    return 0;
}

IAppServices SteamAppServices =
{
    // Core
    I_SteamInit,
    I_SteamShutdown,
    I_SteamCheckForRestart,
    I_SteamUpdate,

    // Achievements
    I_SteamSetAchievement,
    I_SteamSetAchievementProgress,
    I_SteamHasAchievement,
   
    // P2P network driver
    I_SteamSendPacket,
    I_SteamRecvPacket,
    I_SteamResolveAddress,
    
    // Lobby join listener
    I_SteamBeginLobbyJoinListener,
    I_SteamDestroyLobbyJoinListener,
    I_SteamLobbyJoinRequested,

    // Lobby
    I_SteamLobbyPollState,
    I_SteamCreateNewLobby,
    I_SteamJoinLobbyFromStartup,
    I_SteamJoinLobbyFromInvite,
    I_SteamLobbyCleanUp,
    I_SteamLeaveLobby,
    I_SteamLeaveLobby,           // NB: there is no "lock" operation; users leave the lobby.
    I_SteamLobbyInviteFriends,
    I_SteamLobbyStartGame,
    I_SteamLobbyGetReady,
    I_SteamLobbyChangeReady,
    I_SteamLobbyGetUserReadyAt,
    I_SteamLobbyGetTeam,
    I_SteamLobbyChangeTeam,
    I_SteamLobbyGetTeamAt,
    I_SteamLobbyUserIsOwner,
    I_SteamLobbyGetNumMembers,
    I_SteamLobbyGetUserNameAt,
    I_SteamLobbyGetName,
    I_SteamLobbyUpdateName,
    I_SteamLobbyCheckStartGame,

    // Lobby manager
    I_SteamLobbyMgrRefresh,
    I_SteamLobbyMgrInRequest,
    I_SteamLobbyMgrGetCount,
    I_SteamLobbyMgrGetNameAt,
    I_SteamLobbyMgrJoinLobbyAt,

    // Server component
    I_SteamServerGetClientsFromLobby,

    // Client component
    I_SteamClientGetServerFromLobby,
    I_SteamClientGetServerAddr,

    // User
    I_SteamGetLocalUserName,

    // Overlay
    I_SteamOverlayActive,
    I_SteamOverlayEventFilter,
    I_SteamOverlayEatsShiftTab
};

#endif

// EOF

