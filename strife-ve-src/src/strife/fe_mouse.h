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

#ifndef FE_MOUSE_H_
#define FE_MOUSE_H_

enum
{
    FE_MVAR_FIRE,
    FE_MVAR_USE,
    FE_MVAR_JUMP,
    FE_MVAR_PREVWEAPON,
    FE_MVAR_NEXTWEAPON,
    FE_MVAR_STRAFEON,
    FE_MVAR_STRAFELEFT,
    FE_MVAR_STRAFERIGHT,
    FE_MVAR_FORWARD,
    FE_MVAR_BACKWARD,
    FE_MVAR_INVUSE,
    FE_MVAR_INVPREV,
    FE_MVAR_INVNEXT,
    FE_MVAR_NUMVARS
};

extern int *feMVars[FE_MVAR_NUMVARS];
extern const char *feMVarNames[FE_MVAR_NUMVARS];

int FE_CfgNumForSDLMouseButton(SDL_Event *ev);
const char *FE_NameForCfgMouseButton(int cfgMBNum);
void FE_ClearMVarsOfButton(int cfgMBNum, int mvarToBind);

#endif

// EOF

