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
// DESCRIPTION:
//    Configuration and netgame negotiation frontend for
//    Strife: Veteran Edition
//
// AUTHORS:
//    James Haley

#ifndef FE_MULTIPLAYER_H__
#define FE_MULTIPLAYER_H__

extern void (*feModalLoopFunc)(void);

// Lobby listener
void FE_EnableLobbyListener(void);
void FE_DisableLobbyListener(void);
boolean FE_CheckLobbyListener(void);

// Lobby
void FE_CreateLobby(int publicLobby);
void FE_LeaveLobby(void);
void FE_JoinLobbyFromInvite(void);
void FE_JoinLobbyFromConnectStr(const char *lobbyID);
boolean FE_GetLobbyReady(void);
void FE_ToggleLobbyReady(void);
void FE_CheckForLobbyUpgrade(void);

// Lobby manager
void FE_DrawLobbyItem(femenuitem_t *item, int x, int y);

// Start game
void FE_ClientCheckForGameStart(void);

// Teams
void FE_IncrementTeam(void);
void FE_DecrementTeam(void);
const char *FE_GetLobbyTeam(void);

// Commands
void FE_CmdNewPublicLobby(void);
void FE_CmdNewPrivateLobby(void);
void FE_CmdLeaveLobby(void);
void FE_CmdInvite(void);
void FE_CmdJoinLobby(void);
void FE_CmdLobbyRefresh(void);
void FE_CmdLobbyList(void);
void FE_CmdStartGame(void);

// Drawing
void FE_DrawLobbyUserList(int x, int y);

#endif

// EOF

