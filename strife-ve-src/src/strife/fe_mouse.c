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

#include "SDL.h"

#include "i_video.h"
#include "m_controls.h"

#include "fe_mouse.h"


//=============================================================================
//
// Code to deal with mouse buttons (what a cluster...)
//

// Array of mouse config var locations by MVAR enum value
int *feMVars[FE_MVAR_NUMVARS] =
{
    &mousebfire, &mousebuse, &mousebjump, &mousebprevweapon, &mousebnextweapon,
    &mousebstrafe, &mousebstrafeleft, &mousebstraferight, &mousebforward,
    &mousebbackward, &mousebinvuse, &mousebinvprev, &mousebinvnext
};

//
// Variable name for FE_MVAR index
//
const char *feMVarNames[FE_MVAR_NUMVARS] =
{
    "mouseb_fire", "mouseb_use", "mouseb_jump", "mouseb_prevweapon", "mouseb_nextweapon",
    "mouseb_strafe", "mouseb_strafeleft", "mouseb_straferight", "mouseb_forward",
    "mouseb_backward", "mouseb_invuse", "mouseb_invprev", "mouseb_invnext"
};

//
// In:  SDL Event of type SDL_MOUSEBUTTONDOWN
// Out: Choco config mouse button #
//
int FE_CfgNumForSDLMouseButton(SDL_Event *ev)
{
    if(ev->button.button == 0 || ev->button.button > MAX_MOUSE_BUTTONS)
    {
        if(ev->wheel.y > 0)
        {
            return 3;
        }
        else if(ev->wheel.y < 0)
        {
            return 4;
        }
        else
        {
            return -1;
        }
    }

    switch(ev->button.button)
    {
    case SDL_BUTTON_LEFT:
        return 0;
    case SDL_BUTTON_RIGHT:
        return 1;
    case SDL_BUTTON_MIDDLE:
        return 2;
    default:
        return (ev->button.button - 1) + 2;
    }
}

//
// In:  Choco config mouse button #
// Out: Descriptive button name
//
const char *FE_NameForCfgMouseButton(int cfgMBNum)
{
    static const char *mbNames[MAX_MOUSE_BUTTONS] =
    {
        "Left",
        "Right",
        "Middle",
        "Wheel Up",
        "Wheel Down",
        "Button 6",
        "Button 7",
        "Button 8"
    };

    if(cfgMBNum < 0 || cfgMBNum >= MAX_MOUSE_BUTTONS)
        return "None";

    return mbNames[cfgMBNum];
}

//
// Clear the clicked mouse button off any other variables it may be set to
// other than the one that's being set.
//
void FE_ClearMVarsOfButton(int cfgMBNum, int mvarToBind)
{
    int i;

    for(i = FE_MVAR_FIRE; i < FE_MVAR_NUMVARS; i++)
    {
        if(i == mvarToBind)
            continue;
        if(*(feMVars[i]) == cfgMBNum)
            *(feMVars[i]) = -1;
    }
}

// EOF

