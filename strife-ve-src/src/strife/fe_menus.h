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
//    Samuel Villarreal (Hi-Res BG code, mouse pointer)
//

#ifndef FE_MENUS_H_
#define FE_MENUS_H_

void FE_CmdMulti(void);
void FE_CmdOptions(void);
void FE_CmdGameplay(void);
void FE_CmdKeyboard(void);
void FE_CmdKeyboardFuncs(void);
void FE_CmdKeyboardInv(void);
void FE_CmdKeyboardMap(void);
void FE_CmdKeyboardMenu(void);
void FE_CmdKeyboardMove(void);
void FE_CmdKeyboardWeapons(void);
void FE_CmdMouse(void);
void FE_CmdMouseButtons(void);
void FE_CmdGraphics(void);
void FE_CmdGfxBasic(void);
void FE_CmdGfxLights(void);
void FE_CmdGfxSprites(void);
void FE_CmdGfxAdvanced(void);
void FE_CmdAudio(void);
void FE_CmdAbout(void);
void FE_CmdAbout2(void);
void FE_CmdLobbySrv(void);
void FE_CmdLobbyClient(void);
void FE_CmdMPOptions(void);

extern femenu_t mainMenu;
extern femenu_t optionsMenuMain;

boolean FE_InAudioMenu(void);
boolean FE_InClientLobby(void);

#endif

// EOF

