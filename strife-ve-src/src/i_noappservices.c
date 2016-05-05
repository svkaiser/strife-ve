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
// DESCRIPTION: Social/software deployment platform dummy object
// AUTHORS: James Haley, Samuel Villarreal
//

#include "i_social.h"

static int I_NoAppServicesInit(void)
{
    // always reports successful initialization
    return 1;
}

static void I_NoAppServicesShutdown(void)
{
    // Nothing to do
}

static int I_NoAppServicesCheckForRestart(void)
{
    // restart is never needed
    return 0;
}

static void I_NoAppServicesUpdate(void)
{
    // Nothing to update
}

static void I_NoAppServicesSetAchievement(const char *nameID)
{
}

static void I_NoAppServicesSetAchievementProgress(const char *nameID, int current, int max)
{
}

static int I_NoAppServicesHasAchievement(const char *nameID)
{
    // always say yes
    return 1;
}

// P2P network driver
static int I_NoAppServicesSendPacket(void *addr, const void *data, unsigned int size)
{
    // never successful
    return 0;
}

static int I_NoAppServicesRecvPacket(void *addr, void **data, unsigned int *size)
{
    // never successful
    return 0;
}

static int I_NoAppServicesResolveAddress(void *addr)
{
    // never successful
    return 0;
}

// Lobby join listener
static void I_NoAppServicesBeginLobbyJoinListener(void)
{
}

static void I_NoAppServicesDestroyLobbyJoinListener(void)
{
}

static int I_NoAppServicesLobbyJoinRequested(void)
{
    // never successful
    return 0;
}

static int I_NoAppServicesLobbyPollState(void)
{
    // never successful
    return I_LOBBY_STATE_CREATEFAILED;
}

static void I_NoAppServicesCreateNewLobby(int publicLobby)
{
}

static void I_NoAppServicesJoinLobbyFromStartup(const char *ccUserID)
{
}

static void I_NoAppServicesJoinLobbyFromInvite(void)
{
}

static void I_NoAppServicesLobbyCleanUp(void)
{
}

static void I_NoAppServicesLeaveLobby(void)
{
}

static void I_NoAppServicesLockLobby(void)
{
}

static void I_NoAppServicesLobbyInviteFriends(void)
{
}

static void I_NoAppServicesLobbyStartGame(void)
{
}

static int I_NoAppServicesLobbyGetReady(void)
{
    // never ready
    return 0;
}

static void I_NoAppServicesLobbyChangeReady(void)
{
}

static int I_NoAppServicesLobbyGetUserReadyAt(int idx)
{
    // never ready
    return 0;
}

static const char *I_NoAppServicesLobbyGetTeam(void)
{
    return "Auto";
}

static void I_NoAppServicesLobbyChangeTeam(const char *team)
{
}

static const char *I_NoAppServicesLobbyGetTeamAt(int idx)
{
    return "Auto";
}

static int I_NoAppServicesLobbyUserIsOwner(void)
{
    // nothing to own
    return 0;
}

static int I_NoAppServicesLobbyGetNumMembers(void)
{
    // no members
    return 0;
}

static const char *I_NoAppServicesLobbyGetUserNameAt(int idx)
{
    return "";
}

static const char *I_NoAppServicesLobbyGetName(void)
{
    return "";
}

static void I_NoAppServicesLobbyUpdateName(void)
{
}

static int I_NoAppServicesLobbyCheckStartGame(void)
{
    // not ready to start
    return 0;
}

static void I_NoAppServicesLobbyMgrRefresh(void)
{
}

static int I_NoAppServicesLobbyMgrInRequest(void)
{
    // not in a request
    return 0;
}

static unsigned int I_NoAppServicesLobbyMgrGetCount(void)
{
    // no lobbies
    return 0;
}

static const char *I_NoAppServicesLobbyMgrGetNameAt(unsigned int idx)
{
    return "";
}

static void I_NoAppServicesLobbyMgrJoinLobbyAt(unsigned int idx)
{
}

static int I_NoAppServicesServerGetClientsFromLobby(void)
{
    // no clients in lobby
    return 0;
}

static void I_NoAppServicesClientGetServerFromLobby(void)
{
}

static const char *I_NoAppServicesClientGetServerAddr(void)
{
    return "";
}

static const char *I_NoAppServicesGetLocalUserName(void)
{
    return "";
}

static int I_NoAppServicesOverlayActive(void)
{
    // no overlay
    return 0;
}

static int I_NoAppServicesOverlayEventFilter(unsigned int event)
{
    // no events should be filtered here.
    return 0;
}

static int I_NoAppServicesOverlayEatsShiftTab(void)
{
    return 0;
}

// Global dummy app service provider
IAppServices noAppServices =
{
    I_NoAppServicesInit,
    I_NoAppServicesShutdown,
    I_NoAppServicesCheckForRestart,
    I_NoAppServicesUpdate,

    I_NoAppServicesSetAchievement,
    I_NoAppServicesSetAchievementProgress,
    I_NoAppServicesHasAchievement,

    I_NoAppServicesSendPacket,
    I_NoAppServicesRecvPacket,
    I_NoAppServicesResolveAddress,

    I_NoAppServicesBeginLobbyJoinListener,
    I_NoAppServicesDestroyLobbyJoinListener,
    I_NoAppServicesLobbyJoinRequested,
    I_NoAppServicesLobbyPollState,
    I_NoAppServicesCreateNewLobby,
    I_NoAppServicesJoinLobbyFromStartup,
    I_NoAppServicesJoinLobbyFromInvite,
    I_NoAppServicesLobbyCleanUp,
    I_NoAppServicesLeaveLobby,
    I_NoAppServicesLockLobby,
    I_NoAppServicesLobbyInviteFriends,
    I_NoAppServicesLobbyStartGame,
    I_NoAppServicesLobbyGetReady,
    I_NoAppServicesLobbyChangeReady,
    I_NoAppServicesLobbyGetUserReadyAt,
    I_NoAppServicesLobbyGetTeam,
    I_NoAppServicesLobbyChangeTeam,
    I_NoAppServicesLobbyGetTeamAt,
    I_NoAppServicesLobbyUserIsOwner,
    I_NoAppServicesLobbyGetNumMembers,
    I_NoAppServicesLobbyGetUserNameAt,
    I_NoAppServicesLobbyGetName,
    I_NoAppServicesLobbyUpdateName,
    I_NoAppServicesLobbyCheckStartGame,

    I_NoAppServicesLobbyMgrRefresh,
    I_NoAppServicesLobbyMgrInRequest,
    I_NoAppServicesLobbyMgrGetCount,
    I_NoAppServicesLobbyMgrGetNameAt,
    I_NoAppServicesLobbyMgrJoinLobbyAt,

    I_NoAppServicesServerGetClientsFromLobby,

    I_NoAppServicesClientGetServerFromLobby,

    I_NoAppServicesClientGetServerAddr,

    I_NoAppServicesGetLocalUserName,

    I_NoAppServicesOverlayActive,
    I_NoAppServicesOverlayEventFilter,
    I_NoAppServicesOverlayEatsShiftTab
};

// EOF

