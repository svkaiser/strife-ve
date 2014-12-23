//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
//	Put all global state variables here.
//

#include <stdio.h>

#include "doomstat.h"


// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t gamemode = indetermined;
GameMission_t	gamemission = doom;
GameVersion_t   gameversion = exe_strife_1_31;
char *gamedescription;

// Set if homebrew PWAD stuff has been added.
boolean	modifiedgame;

// haleyjd 20140816: [SVE] Classic mode toggle
// * true  == behave like vanilla Strife as much as is practical
// * false == fix non-critical bugs and enable new gameplay elements
boolean classicmode = false;

// [SVE] svillarreal - Use 3D renderer?
boolean use3drenderer = true;
boolean default_use3drenderer = true;

// [SVE] svillarreal - Skip intro movies?
boolean d_skipmovies = false;

// [SVE] interpolation
boolean d_interpolate = true;

// [SVE] for those Brutal Doom fans...
boolean d_maxgore = true;

// [SVE] svillarreal - player recoil bobbing
boolean d_recoil = true;

// [SVE] svillarreal - damage indicators
boolean d_dmgindictor = true;

// [SVE] haleyjd: track "cheating" state for achievements
boolean d_cheating = false;

// [SVE] haleyjd: if true, a netgame played on the current map will be a
// Capture the Chalice game. This is triggered by presence of the appropriate
// starts on the map.
boolean capturethechalice = false;
int     ctcpointlimit     = 5;
int     ctcbluescore;
int     ctcredscore;

// [SVE] haleyjd: a proper autorun setting
boolean autorun = false;

// [SVE] haleyjd: HUD toggle
boolean fullscreenhud = true;

// [SVE] own user's preferred ctc team
int ctcprefteam = PREF_TEAM_AUTO;

// [SVE] preferred ctc teams
int ctcprefteams[MAXPLAYERS];

// [SVE] autoaim toggle
int autoaim = true;

// EOF

