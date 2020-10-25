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
//

#ifndef FE_GAMEPAD_H__
#define FE_GAMEPAD_H__

#include "fe_menuengine.h"

// Commands
void FE_CmdSelGamepad(void);
void FE_CmdGamepadDev(void);
void FE_CmdGamepad(void);
void FE_CmdGPAxes(void);
void FE_CmdGPAutomap(void);
void FE_CmdGPMenus(void);
void FE_CmdGPMovement(void);
void FE_CmdGPInv(void);
void FE_CmdGPGyro(void);
void FE_CmdGPProfile(void);
void FE_CmdJoyBindReset(void);

// Axis binding
void FE_JoyAxisResponder(void);
void FE_JoyAxisTicker(void);
void FE_JoyAxisBindStart(femenuitem_t *item);
void FE_JoyAxisBindDrawer(void);

// Button binding
void FE_JoyBindResponder(void);
void FE_JoyBindTicker(void);
void FE_JoyBindDrawer(void);
void FE_JoyBindStart(femenuitem_t *item);

const char *FE_ButtonNameForNum(int button);
const char *FE_AxisNameForNum(int axis);

// profile indices
enum
{
    FE_JOYPROF_Y_AXIS,
    FE_JOYPROF_X_AXIS,
    FE_JOYPROF_STRAFE_AXIS,
    FE_JOYPROF_LOOK_AXIS,
    FE_JOYPROF_Y_INVERT,
    FE_JOYPROF_X_INVERT,
    FE_JOYPROF_STRAFE_INVERT,
    FE_JOYPROF_LOOK_INVERT,
    FE_JOYPROF_MAP_TOGGLE,
    FE_JOYPROF_MAP_NORTH,
    FE_JOYPROF_MAP_SOUTH,
    FE_JOYPROF_MAP_EAST,
    FE_JOYPROF_MAP_WEST,
    FE_JOYPROF_MAP_ZOOMIN,
    FE_JOYPROF_MAP_ZOOMOUT,
    FE_JOYPROF_MAP_FOLLOW,
    FE_JOYPROF_MAP_MARK,
    FE_JOYPROF_MAP_CLEARMARKS,
    FE_JOYPROF_MENU_ACTIVATE,
    FE_JOYPROF_MENU_UP,
    FE_JOYPROF_MENU_DOWN,
    FE_JOYPROF_MENU_LEFT,
    FE_JOYPROF_MENU_RIGHT,
    FE_JOYPROF_MENU_BACK,
    FE_JOYPROF_MENU_FORWARD,
    FE_JOYPROF_MENU_CONFIRM,
    FE_JOYPROF_MENU_ABORT,
    FE_JOYPROF_STRAFELEFT,
    FE_JOYPROF_STRAFERIGHT,
    FE_JOYPROF_STRAFE,
    FE_JOYPROF_FIRE,
    FE_JOYPROF_USE,
    FE_JOYPROF_SPEED,
    FE_JOYPROF_JUMP,
    FE_JOYPROF_CENTERVIEW,
    FE_JOYPROF_PREVWEAPON,
    FE_JOYPROF_NEXTWEAPON,
    FE_JOYPROF_INVLEFT,
    FE_JOYPROF_INVRIGHT,
    FE_JOYPROF_INVUSE,
    FE_JOYPROF_INVDROP,
    FE_JOYPROF_INVPOP,
    FE_JOYPROF_MISSION,
    FE_JOYPROF_INVKEY,
    FE_JOYPROF_MAX
};

int  FE_PadHasProfile(void);
void FE_ApplyPadProfile(int index);
void FE_AutoApplyPadProfile(void);

#endif

// EOF

