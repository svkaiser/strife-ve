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
//      Timer functions.
//

#include "SDL.h"

#include "d_loop.h"
#include "i_timer.h"
#include "doomtype.h"
#include "m_fixed.h"

//
// I_GetTime
// returns time in 1/35th second tics
//

static Uint32 basetime = 0;

//
// I_ResetBaseTime
//
// haleyjd 20141002: [SVE] Needed for frontend.
//
void I_ResetBaseTime(void)
{
    basetime = 0;
}

int  I_GetTime (void)
{
    Uint32 ticks;

    ticks = SDL_GetTicks();

    if (basetime == 0)
        basetime = ticks;

    ticks -= basetime;

    return (ticks * TICRATE) / 1000;    
}

//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void)
{
    Uint32 ticks;

    ticks = SDL_GetTicks();

    if (basetime == 0)
        basetime = ticks;

    return ticks - basetime;
}

// Sleep for a specified number of ms

void I_Sleep(int ms)
{
    SDL_Delay(ms);
}

void I_WaitVBL(int count)
{
    I_Sleep((count * 1000) / 70);
}

//=============================================================================
//
// haleyjd 20140902: [SVE] Interpolation
//

static unsigned int start_displaytime;
static unsigned int displaytime;

static unsigned int rendertic_start;
unsigned int        rendertic_step;
static unsigned int rendertic_next;
float               rendertic_msec;

const float realtic_clock_rate = 100.0f;

//
// I_setMSec
//
// Module private; set the milliseconds per render frame.
//
static void I_setMSec(void)
{
    rendertic_msec = realtic_clock_rate * TICRATE / 100000.0f;
}

#define eclamp(a, min, max) (a < min ? min : (a > max ? max : a))

//
// I_TimerGetFrac
//
// Calculate the fractional multiplier for interpolating the current frame.
//
fixed_t I_TimerGetFrac(void)
{
    fixed_t frac = FRACUNIT;

    if(!singletics && rendertic_step != 0)
    {
        unsigned int now = SDL_GetTicks();
        frac = (fixed_t)((now - rendertic_start + displaytime) * FRACUNIT / rendertic_step);
        frac = eclamp(frac, 0, FRACUNIT);
    }

    return frac;
}

//
// I_TimerStartDisplay
//
// Calculate the starting display time.
//
void I_TimerStartDisplay(void)
{
    start_displaytime = SDL_GetTicks();
}

//
// I_TimerEndDisplay
//
// Calculate the ending display time.
//
void I_TimerEndDisplay(void)
{
    displaytime = SDL_GetTicks() - start_displaytime;
}

//
// I_TimerSaveMS
//
// Update interpolation state variables at the end of gamesim logic.
//
void I_TimerSaveMS(void)
{
    rendertic_start = SDL_GetTicks();
    rendertic_next  = (unsigned int)((rendertic_start * rendertic_msec + 1.0f) / rendertic_msec);
    rendertic_step  = rendertic_next - rendertic_start;
}

//
// haleyjd: end interpolation code
//
//=============================================================================

void I_InitTimer(void)
{
    // initialize timer

    SDL_Init(SDL_INIT_TIMER);

    // haleyjd: init interpolation
    I_setMSec();
}

