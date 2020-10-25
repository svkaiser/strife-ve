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

#ifndef I_SOCIAL_H__
#define I_SOCIAL_H__

// Return values from LobbyPollState
// Must match ssLobby enumeration
enum
{
    I_LOBBY_STATE_UNINITED,      // uninitialized
    I_LOBBY_STATE_CREATING,      // trying to create
    I_LOBBY_STATE_JOINING,       // trying to join existing
    I_LOBBY_STATE_INLOBBY,       // currently in lobby
    I_LOBBY_STATE_CREATEFAILED,  // creation of a lobby failed
    I_LOBBY_STATE_JOINFAILED,    // join of a lobby failed
    I_LOBBY_STATE_GAMESTARTING   // game wants to start
};

typedef struct IAppServices
{
    // Core
    int  (*Init)(void);
    void (*Shutdown)(void);
    int  (*CheckForRestart)(void);
    void (*Update)(void);

    // Achievements
    void (*SetAchievement)(const char *nameID);
    void (*SetAchievementProgress)(const char *nameID, int current, int max);
    int  (*HasAchievement)(const char *nameID);

    // P2P network driver
    int  (*SendPacket)(void *addr, const void *data, unsigned int size);
    int  (*RecvPacket)(void *addr, void **data, unsigned int *size);
    int  (*ResolveAddress)(void *addr);

    // Lobby join listener
    void (*BeginLobbyJoinListener)(void);
    void (*DestroyLobbyJoinListener)(void);
    int  (*LobbyJoinRequested)(void);

    // Lobby
    int  (*LobbyPollState)(void);
    void (*CreateNewLobby)(int publicLobby);
    void (*JoinLobbyFromStartup)(const char *ccUserID);
    void (*JoinLobbyFromInvite)(void);
    void (*LobbyCleanUp)(void);
    void (*LeaveLobby)(void);
    void (*LockLobby)(void);
    void (*LobbyInviteFriends)(void);
    void (*LobbyStartGame)(void);
    int  (*LobbyGetReady)(void);
    void (*LobbyChangeReady)(void);
    int  (*LobbyGetUserReadyAt)(int idx);
    const char *(*LobbyGetTeam)(void);
    void (*LobbyChangeTeam)(const char *team);
    const char *(*LobbyGetTeamAt)(int idx);
    int  (*LobbyUserIsOwner)(void);
    int  (*LobbyGetNumMembers)(void);
    const char *(*LobbyGetUserNameAt)(int idx);
    const char *(*LobbyGetName)(void);
    void (*LobbyUpdateName)(void);
    int  (*LobbyCheckStartGame)(void);

    // Lobby manager
    void (*LobbyMgrRefresh)(void);
    int  (*LobbyMgrInRequest)(void);
    unsigned int (*LobbyMgrGetCount)(void);
    const char  *(*LobbyMgrGetNameAt)(unsigned int idx);
    void (*LobbyMgrJoinLobbyAt)(unsigned int idx);

    // Server component
    int  (*ServerGetClientsFromLobby)(void);

    // Client component
    void (*ClientGetServerFromLobby)(void);
    const char *(*ClientGetServerAddr)(void);
    
    // User
    const char *(*GetLocalUserName)(void);
    
    // Overlay
    int (*OverlayActive)(void);
    int (*OverlayEventFilter)(unsigned int event);
    int (*OverlayEatsShiftTab)(void);
} IAppServices;

extern IAppServices *gAppServices;

void I_InitAppServices(void);

// Include app provider headers for support defines
#include "i_steamservices.h"
#include "i_galaxyservices.h"
#include "i_switchservices.h"

#ifndef I_APPSERVICES_PLATFORMNAME
#define I_APPSERVICES_PLATFORMNAME "Strife"
#endif

#endif

// EOF

