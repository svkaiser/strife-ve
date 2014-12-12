//
// Steam Services Provider for Strife: Veteran Edition
//
// Copyright(C) 2014 Night Dive Studios, Inc.
// CONFIDENTIAL, ALL RIGHTS RESERVED
//
// Authors: Samuel Villarreal, James Haley
// Purpose: Public DLL exports for exposure to Chocolate Doom
//

#ifndef __STEAM_API_H__
#define __STEAM_API_H__

#if defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__)
#define KAPI __declspec(dllexport)
#else
#define KAPI
#endif

// Return values from I_SteamLobbyPollState
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

#ifdef __cplusplus
extern "C"
{
#endif
    // Core
    KAPI int  I_SteamInit(void);
    KAPI void I_SteamShutdown(void);
    KAPI int  I_SteamCheckForRestart(void);
    KAPI void I_SteamUpdate(void);

    // Achievements
    KAPI void I_SteamSetAchievement(const char *nameID);
    KAPI void I_SteamSetAchievementProgress(const char *nameID, const int current, const int max);
    KAPI int  I_SteamHasAchievement(const char *nameID);

    // Cloud storage
    KAPI int  I_SteamFileExists(const char *filename);
    KAPI int  I_SteamGetFileSize(const char *filename);
    KAPI int  I_SteamGetFileCount(void);
    KAPI void I_SteamGetFileQuota(int *totalBytes, int *remainingBytes);
    KAPI int  I_SteamFileWrite(const char *filename, const void *data, int length);
    KAPI int  I_SteamFileRead(const char *filename, void *data, int length);
    KAPI int  I_SteamFileDelete(const char *filename);
    KAPI int  I_SteamFileForget(const char *filename);
    
    // P2P network driver
    KAPI int  I_SteamSendPacket(void *addr, const void *data, unsigned int size);
    KAPI int  I_SteamRecvPacket(void *addr, void **data, unsigned int *size);
    KAPI int  I_SteamResolveAddress(void *addr);
    
    // Lobby join listener
    KAPI void I_SteamBeginLobbyJoinListener(void);
    KAPI void I_SteamDestroyLobbyJoinListener(void);
    KAPI int  I_SteamLobbyJoinRequested(void);

    // Lobby
    KAPI int  I_SteamLobbyPollState(void);
    KAPI void I_SteamCreateNewLobby(int publicLobby);
    KAPI void I_SteamJoinLobbyFromStartup(const char *ccSteamID);
    KAPI void I_SteamJoinLobbyFromInvite(void);
    KAPI void I_SteamLobbyCleanUp(void);
    KAPI void I_SteamLeaveLobby(void);
    KAPI void I_SteamLobbyInviteFriends(void);
    KAPI void I_SteamLobbyStartGame(void);
    KAPI int  I_SteamLobbyGetReady(void);
    KAPI void I_SteamLobbyChangeReady(void);
    KAPI int  I_SteamLobbyGetUserReadyAt(int idx);
    KAPI const char *I_SteamLobbyGetTeam(void);
    KAPI void I_SteamLobbyChangeTeam(const char *team);
    KAPI const char *I_SteamLobbyGetTeamAt(int idx);
    KAPI int  I_SteamLobbyUserIsOwner(void);
    KAPI int  I_SteamOverlayActive(void);
    KAPI int  I_SteamLobbyGetNumMembers(void);
    KAPI const char  *I_SteamLobbyGetUserNameAt(int idx);
    KAPI const char  *I_SteamLobbyGetName(void);
    KAPI void I_SteamLobbyUpdateName(void);
    KAPI int  I_SteamLobbyCheckStartGame(void);
    KAPI const char *I_SteamGetLocalUserName(void);

    // Lobby manager
    KAPI void I_SteamLobbyMgrRefresh(void);
    KAPI int  I_SteamLobbyMgrInRequest(void);
    KAPI unsigned int I_SteamLobbyMgrGetCount(void);
    KAPI const char  *I_SteamLobbyMgrGetNameAt(unsigned int idx);
    KAPI void I_SteamLobbyMgrJoinLobbyAt(unsigned int idx);

    // Server component
    KAPI int I_SteamServerGetClientsFromLobby(void);

    // Client component
    KAPI void I_SteamClientGetServerFromLobby(void);
    KAPI const char *I_SteamClientGetServerAddr(void);

#ifdef __cplusplus
}
#endif
#endif

// EOF


