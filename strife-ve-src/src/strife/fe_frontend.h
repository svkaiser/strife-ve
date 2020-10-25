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

#ifndef FE_FRONTEND_H_
#define FE_FRONTEND_H_

//
// Frontend states
//

enum
{
    FE_STATE_MAINMENU,    // main menu, the starting state
    FE_STATE_KEYINPUT,    // rebinding a key
    FE_STATE_MBINPUT,     // rebinding a mouse button
    FE_STATE_JAINPUT,     // rebinding a gamepad axis
    FE_STATE_JBINPUT,     // rebinding a gamepad button
    FE_STATE_RESETCON,    // resetting binds
    FE_STATE_LOBBYCREATE, // trying to create a lobby
    FE_STATE_LOBBYJOIN,   // trying to join a lobby
    FE_STATE_REFRESH,     // refreshing lobby list
    FE_STATE_GAMESTART,   // network game started
    FE_STATE_EXITING,     // doing exit outro (with chance to cancel)

    FE_STATE_MAX          // must be last
};

extern int     frontend_fpslimit;
extern int     frontend_state;
extern int     frontend_sigil;
extern int     frontend_sgcount;
extern int     frontend_laser;
extern boolean frontend_wipe;
extern boolean frontend_ingame;
extern boolean frontend_waitframe;

extern const char *frontend_modalmsg;

void FE_CmdGo(void);
void FE_CmdExit(void);
void FE_StartFrontend(void);
boolean FE_InFrontend(void);

//
// In-Game Options
//
void    FE_StartInGameOptionsMenu(void);
void    FE_InGameOptionsTicker(void);
void    FE_InGameOptionsDrawer(void);
boolean FE_InGameOptionsResponder(void);
void    FE_EndInGameOptionsMenu(void);
boolean FE_InOptionsMenu(void);

#endif

// EOF

