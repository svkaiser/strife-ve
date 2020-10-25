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
// DESCRIPTION: Switch platform services implementation
// AUTHORS: Edward Richardson
//

#ifdef SVE_PLAT_SWITCH

#include "i_social.h"
//#include "switchService.h"

static int I_SwitchInit(void)
{
    // always reports successful initialization
    return 1;
}

static void I_SwitchShutdown(void)
{
    // Nothing to do
}

static int I_SwitchCheckForRestart(void)
{
    // restart is never needed
    return 0;
}

static void I_SwitchUpdate(void)
{
    // Nothing to update
}

static void I_SwitchSetAchievement(const char *nameID)
{
}

static void I_SwitchSetAchievementProgress(const char *nameID, int current, int max)
{
}

static int I_SwitchHasAchievement(const char *nameID)
{
    // always say yes
    return 1;
}

// P2P network driver
static int I_SwitchSendPacket(void *addr, const void *data, unsigned int size)
{
    // never successful
    return 0;
}

static int I_SwitchRecvPacket(void *addr, void **data, unsigned int *size)
{
    // never successful
    return 0;
}

static int I_SwitchResolveAddress(void *addr)
{
    // never successful
    return 0;
}

// Lobby join listener
static void I_SwitchBeginLobbyJoinListener(void)
{
}

static void I_SwitchDestroyLobbyJoinListener(void)
{
}

static int I_SwitchLobbyJoinRequested(void)
{
    // never successful
    return 0;
}

static int I_SwitchLobbyPollState(void)
{
    // never successful
    return I_LOBBY_STATE_CREATEFAILED;
}

static void I_SwitchCreateNewLobby(int publicLobby)
{
}

static void I_SwitchJoinLobbyFromStartup(const char *ccUserID)
{
}

static void I_SwitchJoinLobbyFromInvite(void)
{
}

static void I_SwitchLobbyCleanUp(void)
{
}

static void I_SwitchLeaveLobby(void)
{
}

static void I_SwitchLockLobby(void)
{
}

static void I_SwitchLobbyInviteFriends(void)
{
}

static void I_SwitchLobbyStartGame(void)
{
}

static int I_SwitchLobbyGetReady(void)
{
    // never ready
    return 0;
}

static void I_SwitchLobbyChangeReady(void)
{
}

static int I_SwitchLobbyGetUserReadyAt(int idx)
{
    // never ready
    return 0;
}

static const char *I_SwitchLobbyGetTeam(void)
{
    return "Auto";
}

static void I_SwitchLobbyChangeTeam(const char *team)
{
}

static const char *I_SwitchLobbyGetTeamAt(int idx)
{
    return "Auto";
}

static int I_SwitchLobbyUserIsOwner(void)
{
    // nothing to own
    return 0;
}

static int I_SwitchLobbyGetNumMembers(void)
{
    // no members
    return 0;
}

static const char *I_SwitchLobbyGetUserNameAt(int idx)
{
    return "";
}

static const char *I_SwitchLobbyGetName(void)
{
    return "";
}

static void I_SwitchLobbyUpdateName(void)
{
}

static int I_SwitchLobbyCheckStartGame(void)
{
    // not ready to start
    return 0;
}

static void I_SwitchLobbyMgrRefresh(void)
{
}

static int I_SwitchLobbyMgrInRequest(void)
{
    // not in a request
    return 0;
}

static unsigned int I_SwitchLobbyMgrGetCount(void)
{
    // no lobbies
    return 0;
}

static const char *I_SwitchLobbyMgrGetNameAt(unsigned int idx)
{
    return "";
}

static void I_SwitchLobbyMgrJoinLobbyAt(unsigned int idx)
{
}

static int I_SwitchServerGetClientsFromLobby(void)
{
    // no clients in lobby
    return 0;
}

static void I_SwitchClientGetServerFromLobby(void)
{
}

static const char *I_SwitchClientGetServerAddr(void)
{
    return "";
}

static const char *I_SwitchGetLocalUserName(void)
{
    return "";
}

static int I_SwitchOverlayActive(void)
{
    // no overlay
    return 0;
}

static int I_SwitchOverlayEventFilter(unsigned int event)
{
    // we do not need to filter any events for the Switch overlay;
    // it absorbs input properly in all cases we don't already deal with.
    return 0;
}

static int I_SwitchOverlayEatsShiftTab(void)
{
    // Shift+Tab on a Switch? What do you think this is?
    return 0;
}

IAppServices SwitchAppServices =
{
    // Core
    I_SwitchInit,
    I_SwitchShutdown,
    I_SwitchCheckForRestart,
    I_SwitchUpdate,

    // Achievements
    I_SwitchSetAchievement,
    I_SwitchSetAchievementProgress,
    I_SwitchHasAchievement,
   
    // P2P network driver
    I_SwitchSendPacket,
    I_SwitchRecvPacket,
    I_SwitchResolveAddress,
    
    // Lobby join listener
    I_SwitchBeginLobbyJoinListener,
    I_SwitchDestroyLobbyJoinListener,
    I_SwitchLobbyJoinRequested,

    // Lobby
    I_SwitchLobbyPollState,
    I_SwitchCreateNewLobby,
    I_SwitchJoinLobbyFromStartup,
    I_SwitchJoinLobbyFromInvite,
    I_SwitchLobbyCleanUp,
    I_SwitchLeaveLobby,
    I_SwitchLeaveLobby,           // NB: there is no "lock" operation; users leave the lobby.
    I_SwitchLobbyInviteFriends,
    I_SwitchLobbyStartGame,
    I_SwitchLobbyGetReady,
    I_SwitchLobbyChangeReady,
    I_SwitchLobbyGetUserReadyAt,
    I_SwitchLobbyGetTeam,
    I_SwitchLobbyChangeTeam,
    I_SwitchLobbyGetTeamAt,
    I_SwitchLobbyUserIsOwner,
    I_SwitchLobbyGetNumMembers,
    I_SwitchLobbyGetUserNameAt,
    I_SwitchLobbyGetName,
    I_SwitchLobbyUpdateName,
    I_SwitchLobbyCheckStartGame,

    // Lobby manager
    I_SwitchLobbyMgrRefresh,
    I_SwitchLobbyMgrInRequest,
    I_SwitchLobbyMgrGetCount,
    I_SwitchLobbyMgrGetNameAt,
    I_SwitchLobbyMgrJoinLobbyAt,

    // Server component
    I_SwitchServerGetClientsFromLobby,

    // Client component
    I_SwitchClientGetServerFromLobby,
    I_SwitchClientGetServerAddr,

    // User
    I_SwitchGetLocalUserName,

    // Overlay
    I_SwitchOverlayActive,
    I_SwitchOverlayEventFilter,
    I_SwitchOverlayEatsShiftTab
};

#endif

// EOF

