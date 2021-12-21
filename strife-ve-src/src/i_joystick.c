//
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
//       SDL Joystick code.
//


#include "SDL.h"
#include "SDL_joystick.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "doomtype.h"
#include "d_event.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"

#include "m_config.h"
#include "m_misc.h"
#include "m_saves.h"
#include "fe_gamepad.h"

// [SVE] svillarreal
#include "i_social.h"

#include "m_controls.h"

// [SVE]: Track whether or not we're seeing joystick events.
boolean i_seejoysticks;

// When an axis is within the dead zone, it is set to zero.
// This is 5% of the full range:

#define INVOKE_DEAD_ZONE (32768 / 4)
#define DIRECT_DEAD_ZONE (32768 / 8)

float joystick_turnsensitivity = 0.5f;
float joystick_looksensitivity = 0.5f;
float joystick_threshold = 10.0f;

static SDL_GameController *controller = NULL;

// Configuration variables:

// Standard default.cfg Joystick enable/disable

static int usejoystick = 1;
static boolean joystickInit; // true if SDL subsystem init'd

// Joystick to use, as an SDL joystick index:

static int joystick_index = -1;

// Which joystick axis to use for horizontal movement, and whether to
// invert the direction:

static int joystick_x_axis = -1;
static int joystick_x_invert = 0;

// Which joystick axis to use for vertical movement, and whether to
// invert the direction:

static int joystick_y_axis = -1;
static int joystick_y_invert = 0;

// Which joystick axis to use for strafing?

static int joystick_strafe_axis = -1;
static int joystick_strafe_invert = 0;

// [SVE] svillarreal - joystick axis for looking

static int joystick_look_axis = -1;
static int joystick_look_invert = 0;

// [SVE] svillarreal

static int joystick_oldbuttons = 0;

// Virtual to physical button joystick button mapping. By default this
// is a straight mapping.
static int joystick_physical_buttons[NUM_VIRTUAL_BUTTONS] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
};

// [SVE] svillarreal
static int joystick_axisoffset[6] = { 0, 0, 0, 0, 0, 0 };

//
// I_CloseJoystickDevice
//
// haleyjd 20141020: [SVE] separated from I_ShutdownJoystick
//
static void I_CloseJoystickDevice(void)
{
    if(controller != NULL)
    {
        SDL_GameControllerClose(controller);
        controller = NULL;
    }
}

//
// I_ShutdownJoystick
//

void I_ShutdownJoystick(void)
{
    I_CloseJoystickDevice();
    if(joystickInit)
    {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

#if 0
static boolean IsValidAxis(int axis)
{
    int num_axes;

    if(axis < 0)
    {
        return true;
    }

    if(IS_BUTTON_AXIS(axis))
    {
        return true;
    }

    if(IS_HAT_AXIS(axis))
    {
        return HAT_AXIS_HAT(axis) < SDL_JoystickNumHats(joystick);
    }

    num_axes = SDL_JoystickNumAxes(joystick);

    return axis < num_axes;
}
#endif

//
// Load the SDL controller database file if it is available
//
static void I_LoadSDLControllerDB(void)
{
#if !defined(SVE_PLAT_SWITCH)
    const char *hint = SDL_GetHint(SDL_HINT_GAMECONTROLLERCONFIG);
    if(!(hint && *hint))
    {
        char *filename = M_SafeFilePath(SDL_GetBasePath(), "gamecontrollerdb.txt");
        char *buffer   = NULL;
        int   length   = M_ReadFileAsString(filename, &buffer);
        if(buffer != NULL)
        {
            if(length > 0)
            {
                SDL_RWops *rw;
                if((rw = SDL_RWFromConstMem(buffer, length)))
                {
                    SDL_GameControllerAddMappingsFromRW(rw, 1);
                }
            }
            Z_Free(buffer);
        }
        Z_Free(filename);
    }
#endif
}

//
// I_InitJoystick
//

void I_InitJoystick(void)
{
    if(!usejoystick)
    {
        return;
    }

    // init subsystem
    if(SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
    {
        return;
    }

    // load up controller database
    I_LoadSDLControllerDB();

    joystickInit = true; // subsystem initialized
    I_AtExit(I_ShutdownJoystick, true);

    // auto open first valid device if one is available and nothing is configured
    if(joystick_index == -1 && SDL_NumJoysticks() >= 1)
    {
        int max = SDL_NumJoysticks();
        int i;
        for(i = 0; i < max; i++)
        {
            if(SDL_IsGameController(i))
            {
                joystick_index = i;
                break;
            }
        }
    }

    // open initial device if valid
    I_ActivateJoystickDevice(joystick_index);

    // [SVE] svillarreal - just pick whatever profile is available if
    // no configs are present
    if(extra_config_fresh || M_CheckGamepadButtonVars())
    {
        FE_AutoApplyPadProfile();
    }
}

//
// I_JoystickAllowed
//

boolean I_JoystickAllowed(void)
{
    return usejoystick;
}

//
// I_QueryNumJoysticks
//
// haleyjd 20141020: [SVE] Get number of joysticks
//

int I_QueryNumJoysticks(void)
{
    return SDL_NumJoysticks();
}

//
// I_QueryJoystickName
//
// haleyjd 20141020: [SVE] Get device name at index
//

const char *I_QueryJoystickName(int index)
{
    if(index >= 0 && index < SDL_NumJoysticks())
    {
        return SDL_JoystickNameForIndex(index);
    }

    return NULL;
}

//
// I_QueryActiveJoystickNum
//

int I_QueryActiveJoystickNum(void)
{
    return joystick_index;
}

//
// I_QueryActiveJoystickName
//

const char *I_QueryActiveJoystickName(void)
{
    return I_QueryJoystickName(joystick_index);
}

//
// I_GetJoystickAxis
//

static int I_GetJoystickAxis(const int index)
{
    if(index < 0 || index >= 6)
    {
        return 0;
    }

    return MAX(MIN(SDL_GameControllerGetAxis(controller, index) -
        joystick_axisoffset[index], 32767), -32768);
}

//
// I_ActivateJoystickDevice
//
// haleyjd 20141020: [SVE] Activate a selected joystick device.
//

void I_ActivateJoystickDevice(int index)
{
    // close any currently open device
    I_CloseJoystickDevice();

    // set new index
    joystick_index = index;

    // validate
    if(joystick_index < 0 || joystick_index >= SDL_NumJoysticks())
    {
        return;
    }

    // open the device
    if(!(controller = SDL_GameControllerOpen(joystick_index)))
    {
        return;
    }

    // allow event polling
    SDL_JoystickEventState(SDL_ENABLE);
}

static boolean IsAxisButton(int physbutton)
{
    if(IS_BUTTON_AXIS(joystick_x_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_x_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_x_axis))
        {
            return true;
        }
    }
    if(IS_BUTTON_AXIS(joystick_y_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_y_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_y_axis))
        {
            return true;
        }
    }
    if(IS_BUTTON_AXIS(joystick_strafe_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_strafe_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_strafe_axis))
        {
            return true;
        }
    }
    if(IS_BUTTON_AXIS(joystick_look_axis))
    {
        if (physbutton == BUTTON_AXIS_NEG(joystick_look_axis)
         || physbutton == BUTTON_AXIS_POS(joystick_look_axis))
        {
            return true;
        }
    }

    return false;
}

// Get the state of the given virtual button.

static int ReadButtonState(int vbutton)
{
    int physbutton;

    if(!controller)
    {
        return 0;
    }

    // Map from virtual button to physical (SDL) button.
    physbutton = joystick_physical_buttons[vbutton];

    // Never read axis buttons as buttons.
    if(IsAxisButton(physbutton))
    {
        return 0;
    }

    if(physbutton < 0 || physbutton >= SDL_CONTROLLER_BUTTON_MAX)
    {
        return 0;
    }

    return SDL_GameControllerGetButton(controller, physbutton);
}

//
// I_GetJoystickEventID
//
// [SVE] svillarreal - Scans for a joystick event and
// remaps it as a button ID
//

int I_GetJoystickEventID(void)
{
    int i;
    int axis;
    int greatest_absaxisvalue =  0;
    int greatest_axis         = -1;
    int greatest_axissign     =  0;

    if(!controller)
    {
        return -1;
    }

    // check for button presses
    for(i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
        if(ReadButtonState(i))
        {
            return i;
        }
    }

    // check for axis movement
    for(i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
    {
        axis = I_GetJoystickAxis(i);

        if(axis > INVOKE_DEAD_ZONE *2 || axis < -INVOKE_DEAD_ZONE *2)
        {
            int absaxis = abs(axis);
            if(absaxis > greatest_absaxisvalue)
            {
                greatest_absaxisvalue = absaxis;
                greatest_axis         = i;
                greatest_axissign     = (axis > 0) ? 1 : -1;
            }
        }
    }

    if(greatest_axis == -1)
        return -1;
    else if(greatest_axis == 4)
    {
        return NUM_VIRTUAL_BUTTONS + 14;
    }
    else if(greatest_axis == 5)
    {
        return NUM_VIRTUAL_BUTTONS + 15;
    }
    else
    {
        if(greatest_axissign > 0)
            return NUM_VIRTUAL_BUTTONS + greatest_axis;
        else
            return NUM_VIRTUAL_BUTTONS + 5 + greatest_axis;
    }
}

//
// I_GetJoystickAxisID
//
// [SVE] svillarreal - Returns the invoked joystick axis
//

int I_GetJoystickAxisID(int *axisvalue)
{
    int i;
    int axis;
    int greatest_absaxisvalue =  0;
    int greatest_axis         = -1;

    if(!controller)
    {
        return -1;
    }

    // check for axis movement
    for(i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
    {
        axis = I_GetJoystickAxis(i);

        if(axis > INVOKE_DEAD_ZONE || axis < -INVOKE_DEAD_ZONE)
        {
            int absaxis = abs(axis);
            if(absaxis > greatest_absaxisvalue)
            {
                greatest_absaxisvalue = absaxis;
                greatest_axis         = i;
            }
        }
    }

    if(axisvalue)
        *axisvalue = greatest_absaxisvalue;
    return greatest_axis;
}

// Get a bitmask of all currently-pressed buttons

static int GetButtonsState(void)
{
    int i;
    int axis;
    int result;

    result = 0;

    for(i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
        if(ReadButtonState(i))
        {
            result |= 1 << i;
        }
    }

    for(i = 0; i < 4; ++i)
    {
        axis = I_GetJoystickAxis(i);

        if(axis > INVOKE_DEAD_ZONE || axis < -INVOKE_DEAD_ZONE)
        {
            if(axis > 0)
            {
                result |= 1 << (NUM_VIRTUAL_BUTTONS + i);
            }
            else
            {
                result |= 1 << (NUM_VIRTUAL_BUTTONS + 5 + i);
            }
        }
    }

    // Treat the final two axis (triggers) as buttons
    for (; i < 6; ++i)
    {
        axis = I_GetJoystickAxis(i);

        if (axis > INVOKE_DEAD_ZONE || axis < -INVOKE_DEAD_ZONE)
        {
            result |= 1 << (NUM_VIRTUAL_BUTTONS + i + 10);
        }
    }

    return result;
}

// Read the state of an axis, inverting if necessary.

static int GetAxisState(int axis, int invert)
{
    int result;

    // Axis -1 means disabled.

    if(axis < 0)
    {
        return 0;
    }

    // Is this a button axis, or a hat axis?
    // If so, we need to handle it specially.

    result = 0;

    if(IS_BUTTON_AXIS(axis))
    {
        int button = BUTTON_AXIS_NEG(axis);

        if(button >= 0 && button < SDL_CONTROLLER_BUTTON_MAX &&
            SDL_GameControllerGetButton(controller, button))
        {
            result -= 32767;
        }

        button = BUTTON_AXIS_POS(axis);

        if(button >= 0 && button < SDL_CONTROLLER_BUTTON_MAX &&
            SDL_GameControllerGetButton(controller, button))
        {
            result += 32767;
        }
    }
    else if(axis >= 0 && axis < SDL_CONTROLLER_AXIS_MAX)
    {
        result = I_GetJoystickAxis(axis);

        if (result < DIRECT_DEAD_ZONE && result > -DIRECT_DEAD_ZONE)
        {
            result = 0;
        }
    }

    if (invert)
    {
        result = -result;
    }

    return result;
}

//
// I_JoystickGetButtons
//
// haleyjd 20141104: [SVE] return button state directly
//
int I_JoystickGetButtons(void)
{
    int bits;
    int data;
    int ret = 0;

    if(!controller)
        return -1;
    
    data = GetButtonsState();        

    if(data)
    {
        bits = data;

        bits &= ~joystick_oldbuttons;
        ret = bits;
    }

    joystick_oldbuttons = data;
    return ret;
}

//
// I_JoystickGetButtonsEvent
//

int I_JoystickGetButtonsEvent(void)
{
    int bits;
    int data;
    int ret = 0;

    if(!controller)
        return -1;
    
    data = I_GetJoystickEventID();        

    if(data >= 0)
    {
        bits = data;

        bits &= ~joystick_oldbuttons;
        ret = bits;
    }
    else
    {
        return -1;
    }

    joystick_oldbuttons = data;
    return ret;
}

//
// I_JoystickResetOldButtons
//

void I_JoystickResetOldButtons(void)
{
    joystick_oldbuttons = 0;
}

//
// I_JoystickButtonEvent
//

static void I_JoystickButtonEvent(void)
{
    int bits;
    int data;
    int i;
    event_t ev;

    data = GetButtonsState();

    if(data)
    {
        bits = data;

        // check for button press
        bits &= ~joystick_oldbuttons;
        for(i = 0; i < NUM_VIRTUAL_BUTTONS + 16; ++i)
        {
            if(bits & (1 << i))
            {
                ev.type = ev_joybtndown;
                ev.data1 = i;
                D_PostEvent(&ev);
            }
        }
    }

    bits = joystick_oldbuttons;
    joystick_oldbuttons = data;

    // check for button release
    bits &= ~joystick_oldbuttons;
    for(i = 0; i < NUM_VIRTUAL_BUTTONS + 16; ++i)
    {
        if(bits & (1 << i))
        {
            ev.type = ev_joybtnup;
            ev.data1 = i;
            D_PostEvent(&ev);
            i_seejoysticks = true;
            i_seemouses    = false;
        }
    }
}

//
// I_JoystickGetAxes
//
// haleyjd 20141104: [SVE] return axis state directly
//
void I_JoystickGetAxes(int *x_axis, int *y_axis, int *s_axis, int *l_axis)
{
    *x_axis = 0;
    *y_axis = 0;
    *s_axis = 0;
    *l_axis = 0;

    if(controller)
    {
        if(joystick_turnsensitivity < 0.1f)
        {
            joystick_turnsensitivity = 0.1f;
        }

        if(joystick_looksensitivity < 0.1f)
        {
            joystick_looksensitivity = 0.1f;
        }

        if(joystick_threshold < 1.0f)
        {
            joystick_threshold = 1.0f;
        }

        *x_axis = GetAxisState(joystick_x_axis, joystick_x_invert);
        *y_axis = GetAxisState(joystick_y_axis, joystick_y_invert);
        *s_axis = GetAxisState(joystick_strafe_axis, joystick_strafe_invert);
        *l_axis = GetAxisState(joystick_look_axis, joystick_look_invert);
    }
}

//
// I_UpdateJoystick
//

void I_UpdateJoystick(void)
{
    //
    // [SVE] svillarreal
    // clamp cvar values
    //
    if(joystick_turnsensitivity < 0.1f)
    {
        joystick_turnsensitivity = 0.1f;
    }

    if(joystick_looksensitivity < 0.1f)
    {
        joystick_looksensitivity = 0.1f;
    }

    if(joystick_threshold < 1.0f)
    {
        joystick_threshold = 1.0f;
    }

    // [SVE] svillarreal
    if(gAppServices->OverlayActive())
        return;

    if(controller != NULL)
    {
        int x = GetAxisState(joystick_x_axis, joystick_x_invert);
        int y = GetAxisState(joystick_y_axis, joystick_y_invert);
        int s = GetAxisState(joystick_strafe_axis, joystick_strafe_invert);
        int l = GetAxisState(joystick_look_axis, joystick_look_invert);

        if(x || y || s || l)
        {
            event_t ev;

            ev.type = ev_joystick;
            ev.data1 = l;
            ev.data2 = x;
            ev.data3 = y;
            ev.data4 = s;

            D_PostEvent(&ev);
            i_seejoysticks = true; // [SVE]
            i_seemouses    = false;
        }

        I_JoystickButtonEvent();
    }
}

void I_BindJoystickVariables(void)
{
    int i;

    M_BindVariable("use_joystick",          &usejoystick);
    M_BindVariable("joystick_index",        &joystick_index);
    M_BindVariable("joystick_x_axis",       &joystick_x_axis);
    M_BindVariable("joystick_y_axis",       &joystick_y_axis);
    M_BindVariable("joystick_strafe_axis",  &joystick_strafe_axis);
    M_BindVariable("joystick_look_axis",    &joystick_look_axis);
    M_BindVariable("joystick_x_invert",     &joystick_x_invert);
    M_BindVariable("joystick_y_invert",     &joystick_y_invert);
    M_BindVariable("joystick_strafe_invert",&joystick_strafe_invert);
    M_BindVariable("joystick_look_invert",  &joystick_look_invert);

    M_BindVariable("joystick_turnsensitivity", &joystick_turnsensitivity);
    M_BindVariable("joystick_looksensitivity", &joystick_looksensitivity);
    M_BindVariable("joystick_threshold",     &joystick_threshold);

    for (i = 0; i < NUM_VIRTUAL_BUTTONS; ++i)
    {
        char name[64];
        M_snprintf(name, sizeof(name), "joystick_physical_button%i", i);
        M_BindVariable(name, &joystick_physical_buttons[i]);
    }
}
