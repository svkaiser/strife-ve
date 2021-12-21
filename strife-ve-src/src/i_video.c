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
//	DOOM graphics stuff for SDL.
//
 
#include "SDL.h"
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "icon.c"

#include "config.h"
#include "deh_str.h"
#include "doomtype.h"
#include "doomkeys.h"

// [SVE] svillarreal - from gl scale branch
#include "i_glscale.h"

#include "i_joystick.h"
#include "i_system.h"
#include "i_swap.h"
#include "i_timer.h"
#include "i_video.h"
#include "i_scale.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

// [SVE] svillarreal
#include "doomstat.h"
#include "rb_main.h"
#include "rb_config.h"
#include "rb_draw.h"
#include "rb_wipe.h"
#include "fe_frontend.h"
#include "m_help.h"

#include "i_social.h"

// [SVE]: Track whether or not we see mouse events
boolean i_seemouses;

#define LOADING_DISK_W 16
#define LOADING_DISK_H 16
 
// Non aspect ratio-corrected modes (direct multiples of 320x200)

static screen_mode_t *screen_modes[] = {
    &mode_scale_1x,
    &mode_scale_2x,
    &mode_scale_3x,
    &mode_scale_4x,
    &mode_scale_5x,
};

// Aspect ratio corrected modes (4:3 ratio)

static screen_mode_t *screen_modes_corrected[] = {

    // Vertically stretched modes (320x200 -> 320x240 and multiples)

    &mode_stretch_1x,
    &mode_stretch_2x,
    &mode_stretch_3x,
    &mode_stretch_4x,
    &mode_stretch_5x,

    // Horizontally squashed modes (320x200 -> 256x200 and multiples)

    &mode_squash_1x,
    &mode_squash_2x,
    &mode_squash_3x,
    &mode_squash_4x,
    &mode_squash_5x,
};

// SDL video driver name

char *video_driver = "";

// Window position:

static char *window_position = "center";

// SDL screen.
SDL_Window *windowscreen;
static SDL_Renderer *renderer;
static SDL_GLContext *GlContext;
static SDL_Surface *argbbuffer = NULL;
static SDL_Texture *texture = NULL;
static SDL_Texture *texture_upscaled = NULL;

static SDL_Rect blit_rect = { 0, 0, SCREENWIDTH, SCREENHEIGHT };

// Window title

static char *window_title = "";

// Intermediate 8-bit buffer that we draw to instead of 'screen'.
// This is used when we are rendering in 32-bit screen mode.

static SDL_Surface *screenbuffer = NULL;

// palette

static SDL_Color palette[256];
static boolean palette_to_set;

// display has been set up?

static boolean initialized = false;

// disable mouse?

static boolean nomouse = false;
int usemouse = 1;

// Bit mask of mouse button state.

static unsigned int mouse_button_state = 0;

// Disallow mouse and joystick movement to cause forward/backward
// motion.  Specified with the '-novert' command line parameter.
// This is an int to allow saving to config file

int novert = 0;

// Save screenshots in PNG format.

int png_screenshots = 0;

// Screen width and height, from configuration file.

int screen_width = SCREENWIDTH;
int screen_height = SCREENHEIGHT;
int default_screen_width = SCREENWIDTH;
int default_screen_height = SCREENHEIGHT;

// [SVE] haleyjd
boolean screen_init;

// [SVE] svillarreal
static boolean show_visual_cursor = false;

// Automatically adjust video settings if the selected mode is 
// not a valid video mode.

static int autoadjust_video_settings = 1;

// Run in full screen mode?  (int type for config code)

int fullscreen = true;
int default_fullscreen = true; // [SVE]

int window_noborder = false;
int default_window_noborder = false;

// Aspect ratio correction mode

int aspect_ratio_correct = true;

// Time to wait for the screen to settle on startup before starting the
// game (ms)

static int startup_delay = 1000;

// Grab the mouse? (int type for config code)

static int grabmouse = true;

// The screen buffer; this is modified to draw things to the screen

byte *I_VideoBuffer = NULL;

// If true, game is running as a screensaver

boolean screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

// [SVE] svillarreal - from gl scale branch
//
// If true, we are rendering the screen using OpenGL hardware scaling
// rather than software mode.

static boolean using_opengl = true;

// If true, we display dots at the bottom of the screen to 
// indicate FPS.

static boolean display_fps_dots;

// If this is true, the screen is rendered but not blitted to the
// video buffer.

static boolean noblit;

// Callback function to invoke to determine whether to grab the 
// mouse pointer.

static grabmouse_callback_t grabmouse_callback = NULL;
static warpmouse_callback_t warpmouse_callback = NULL; // haleyjd [SVE]

// disk image data and background overwritten by the disk to be
// restored by EndRead

static byte *disk_image = NULL;
static byte *saved_background;
static boolean window_focused;

// Empty mouse cursor

static SDL_Cursor *cursors[2];

// The screen mode and scale functions being used

static screen_mode_t *screen_mode;

// Window resize state.

static boolean need_resize = false;
static unsigned int resize_w, resize_h;
static unsigned int last_resize_time;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = false; // haleyjd [SVE]: false by default

// Is the shift key currently down?

static int shiftdown = 0;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;
boolean mouse_invert = false;   // [SVE] svillarreal
boolean mouse_enable_acceleration = false; // [SVE] svillarreal
boolean mouse_smooth = false; // [SVE] svillarreal
int mouse_scale = 2;

// Gamma correction level to use

int usegamma = 0;

static void ApplyWindowResize(unsigned int w, unsigned int h);
 
static boolean MouseShouldBeGrabbed()
{
    // never grab the mouse when in screensaver mode
   
    if (screensaver_mode)
        return false;

    // if the window doesn't have focus, never grab it

    if (!window_focused)
        return false;

    // always grab the mouse when full screen (dont want to 
    // see the mouse pointer)

    if (fullscreen && !show_visual_cursor) // [SVE]: allow visual cursor
        return true;

    // Don't grab the mouse if mouse input is disabled

    if (!usemouse || nomouse)
        return false;

    // if we specify not to grab the mouse, never grab

    if (!grabmouse)
        return false;

    // Invoke the grabmouse callback function to determine whether
    // the mouse should be grabbed

    if (grabmouse_callback != NULL)
    {
        return grabmouse_callback();
    }
    else
    {
        return true;
    }
}

//
// haleyjd 20141007: [SVE]
// Our improved mouse support in the menus for SVE requires not warping the
// mouse based on the menu being active, even though it's been released. This
// callback is used similar to the above to let the game code tell us when 
// we're ok to warp the mouse and when we should leave it alone.
//
#if 0
static boolean MouseShouldBeWarped(void)
{
    return warpmouse_callback ? warpmouse_callback() : true;
}
#endif

void I_SetGrabMouseCallback(grabmouse_callback_t func)
{
    grabmouse_callback = func;
}

void I_SetWarpMouseCallback(warpmouse_callback_t func)
{
    warpmouse_callback = func;
}

// Set the variable controlling FPS dots.

void I_DisplayFPSDots(boolean dots_on)
{
    display_fps_dots = dots_on;
}

// Update the value of window_focused when we get a focus event
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.

static void UpdateFocus(void)
{
    Uint32 wflags;

    SDL_PumpEvents();

    wflags = windowscreen ? SDL_GetWindowFlags(windowscreen) : 0;
    screenvisible = ((wflags & SDL_WINDOW_SHOWN)
                  && !(wflags & SDL_WINDOW_MINIMIZED));

    window_focused = (screenvisible
                   && ((wflags & (SDL_WINDOW_INPUT_GRABBED
                                | SDL_WINDOW_INPUT_FOCUS
                                | SDL_WINDOW_MOUSE_FOCUS)) != 0));
}

// Show or hide the mouse cursor. We have to use different techniques
// depending on the OS.

void I_SetShowCursor(boolean show)
{
    // On Windows, using SDL_ShowCursor() adds lag to the mouse input,
    // so work around this by setting an invisible cursor instead. On
    // other systems, it isn't possible to change the cursor, so this
    // hack has to be Windows-only. (Thanks to entryway for this)

#ifdef _WIN32
    boolean shouldshow = show;

#if !defined(LUNA_RELEASE) // always show hardware cursor on Luna
    shouldshow = shouldshow && !show_visual_cursor; // [SVE]
#endif

    if (shouldshow) // [SVE]
    {
        SDL_SetCursor(cursors[1]);
    }
    else
    {
        SDL_SetCursor(cursors[0]);
    }
#else
    SDL_ShowCursor(show && !show_visual_cursor);
#endif

    // When the cursor is hidden, grab the input.

    if (!screensaver_mode)
    {
        SDL_SetRelativeMouseMode(!show);
    }
}

//
// I_SetShowVisualCursor
//

void I_SetShowVisualCursor(boolean show)
{
    show_visual_cursor = show;
#ifdef _WIN32
    SDL_SetCursor(cursors[0]);
#else
    SDL_ShowCursor(0);
#endif
}

void I_EnableLoadingDisk(void)
{
    patch_t *disk;
    byte *tmpbuf;
    const char *disk_name;
    int y;
    char buf[20];

    //SDL_VideoDriverName(buf, 15);

    if (!strcmp(buf, "Quartz"))
    {
        // MacOS Quartz gives us pageflipped graphics that screw up the 
        // display when we use the loading disk.  Disable it.
        // This is a gross hack.

        return;
    }

    if (M_CheckParm("-cdrom") > 0)
        disk_name = DEH_String("STCDROM");
    else
        disk_name = DEH_String("STDISK");

    disk = W_CacheLumpName(disk_name, PU_STATIC);

    // Draw the patch into a temporary buffer

    tmpbuf = Z_Malloc(SCREENWIDTH * (disk->height + 1), PU_STATIC, NULL);
    V_UseBuffer(tmpbuf);

    // Draw the disk to the screen:

    V_DrawPatch(0, 0, disk);

    disk_image = Z_Malloc(LOADING_DISK_W * LOADING_DISK_H, PU_STATIC, NULL);
    saved_background = Z_Malloc(LOADING_DISK_W * LOADING_DISK_H, PU_STATIC, NULL);

    for (y=0; y<LOADING_DISK_H; ++y) 
    {
        memcpy(disk_image + LOADING_DISK_W * y,
               tmpbuf + SCREENWIDTH * y,
               LOADING_DISK_W);
    }

    // All done - free the screen buffer and restore the normal 
    // video buffer.

    W_ReleaseLumpName(disk_name);
    V_RestoreBuffer();
    Z_Free(tmpbuf);
}

static const int scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

//
// Translates the SDL key
//
// [SVE]: Externalized (needed in frontend)
//
int TranslateKey(SDL_Keysym *sym)
{
    int scancode = sym->scancode;

    switch (scancode)
    {
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return KEY_RCTRL;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return KEY_RSHIFT;
        case SDL_SCANCODE_LALT:
            return KEY_LALT;
        case SDL_SCANCODE_RALT:
            return KEY_RALT;

        default:
            if (scancode >= 0 && scancode < arrlen(scancode_translate_table))
            {
                return scancode_translate_table[scancode];
            }
            else
            {
                return 0;
            }
    }
}

void I_ShutdownGraphics(void)
{
    if (initialized)
    {
        I_SetShowCursor(true);

        SDL_QuitSubSystem(SDL_INIT_VIDEO);

        initialized = false;
    }
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // [SVE] svillarreal
    gAppServices->Update();
}

//
// I_ClearMouseButtonState
//
// [SVE] svillarreal
//

void I_ClearMouseButtonState(void)
{
    mouse_button_state = 0;
}

static void UpdateMouseButtonState(unsigned int button, boolean on)
{
    event_t event;

    if (button < SDL_BUTTON_LEFT || button > MAX_MOUSE_BUTTONS)
    {
        return;
    }

    // Note: button "0" is left, button "1" is right,
    // button "2" is middle for Doom.  This is different
    // to how SDL sees things.

    switch (button)
    {
        case SDL_BUTTON_LEFT:
            button = 0;
            break;

        case SDL_BUTTON_RIGHT:
            button = 1;
            break;

        case SDL_BUTTON_MIDDLE:
            button = 2;
            break;

        default:
            // SDL buttons are indexed from 1.
            --button;
            break;
    }

    // Turn bit representing this button on or off.

    if (on)
    {
        mouse_button_state |= (1 << button);
    }
    else
    {
        mouse_button_state &= ~(1 << button);
    }

    // Post an event with the new button state.

    event.type = ev_mousebutton;
    event.data1 = mouse_button_state;
    event.data2 = event.data3 = 0;
    D_PostEvent(&event);
    i_seemouses = true;
    i_seejoysticks = false;
}

static int AccelerateMouse(int val)
{
    if(mouse_acceleration <= 0 || !mouse_enable_acceleration)
        return val;

    if (val < 0)
        return -AccelerateMouse(-val);

    if (val > mouse_threshold)
    {
        return (int)((val - mouse_threshold) * mouse_acceleration + mouse_threshold);
    }
    else
    {
        return val;
    }
}

static void UpdateShiftStatus(SDL_Event *event)
{
    int change;

    if (event->type == SDL_KEYDOWN)
    {
        change = 1;
    }
    else if (event->type == SDL_KEYUP)
    {
        change = -1;
    }
    else
    {
        return;
    }

    if (event->key.keysym.sym == SDLK_LSHIFT 
     || event->key.keysym.sym == SDLK_RSHIFT)
    {
        shiftdown += change;
    }
}

//
// haleyjd 20141004: [SVE] Get true mouse position
//
void I_GetAbsoluteMousePosition(int *x, int *y)
{
#if 1     
	int w;
	int h;
	SDL_GetWindowSize(windowscreen, &w, &h);
#else
	SDL_Surface *display = SDL_GetWindowSurface(windowscreen);
	int w = display->w;
	int h = display->h;

	if (!display)
		return;
#endif

    fixed_t aspectRatio = w * FRACUNIT / h;

    SDL_PumpEvents();
    SDL_GetMouseState(x, y);

    if(aspectRatio == 4 * FRACUNIT / 3) // nominal
    {
        *x = *x * SCREENWIDTH  / w;
        *y = *y * SCREENHEIGHT / h;
    }
    else if(aspectRatio > 4 * FRACUNIT / 3) // widescreen
    {
        // calculate centered 4:3 subrect
        int sw = h * 4 / 3;
        int hw = (w - sw) / 2;

        *x = (*x - hw) * SCREENWIDTH / sw;
        *y = *y * SCREENHEIGHT / h;
    }
    else // narrow
    {
        int sh = w * 3 / 4;
        int hh = (h - sh) / 2;

        *x = *x * SCREENWIDTH / w;
        *y = (*y - hh) * SCREENHEIGHT / sh;
    }
}

void I_GetEvent(void)
{
    static boolean ate_shift_tab;
    SDL_Event sdlevent;
    event_t event;

    // possibly not needed
    
    SDL_PumpEvents();

    // haleyjd: [SVE] overlays with shift+tab issues need to purge the event 
    // queue and update focus status when exiting the overlay.
    if(ate_shift_tab && !gAppServices->OverlayActive())
    {
        SDL_Event ev;
        while(SDL_PollEvent(&ev));
        //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
        UpdateFocus();
        ate_shift_tab = false;
    }

    // put event-grabbing stuff in here
    
    while (SDL_PollEvent(&sdlevent))
    {
        if(gAppServices->OverlayEventFilter(sdlevent.type))
            continue;

        // ignore mouse events when the window is not focused

        if (!window_focused 
         && (sdlevent.type == SDL_MOUSEMOTION
          || sdlevent.type == SDL_MOUSEBUTTONDOWN
          || sdlevent.type == SDL_MOUSEBUTTONUP
          || sdlevent.type == SDL_MOUSEWHEEL))
        {
            continue;
        }

        if (screensaver_mode && (sdlevent.type == SDL_QUIT || sdlevent.type == SDL_APP_TERMINATING))
        {
            I_Quit();
        }

        UpdateShiftStatus(&sdlevent);

        // process event
        
        switch (sdlevent.type)
        {
            case SDL_TEXTINPUT:
                // Discard any UTF-8 text
                if ((sdlevent.text.text[0] & 0x80) != 0)
                {
                    break;
                }

                for(unsigned int i = 0; i < SDL_strlen(sdlevent.text.text); i++)
                {
                    const unsigned char currchar = sdlevent.text.text[i];
                    if (isprint(currchar))
                    {
                        event_t textevent = { ev_text, currchar,
                                              sdlevent.key.repeat, 0, 0 };
                        D_PostEvent(&textevent);
                    }
                }
                break;

            case SDL_KEYDOWN:
                // haleyjd: [SVE] some overlays have an issue with shift+tab
                if(gAppServices->OverlayEatsShiftTab())
                {
                    // Going into the overlay?
                    if(sdlevent.key.keysym.sym == SDLK_TAB && shiftdown)
                    {
                        // ignore the event and remember it
                        ate_shift_tab = true;
                        //SDL_EnableKeyRepeat(0, 0);
                        break;
                    }
                }

                // data1 has the key pressed, data2 has the character
                // (shift-translated, etc)
                event.type = ev_keydown;
                event.data1 = TranslateKey(&sdlevent.key.keysym);

                if (event.data1 != 0)
                {
                    D_PostEvent(&event);
                }
                break;

            case SDL_KEYUP:
                event.type = ev_keyup;
                event.data1 = TranslateKey(&sdlevent.key.keysym);

                if (event.data1 != 0)
                {
                    D_PostEvent(&event);
                }
                break;

                /*
            case SDL_MOUSEMOTION:
                event.type = ev_mouse;
                event.data1 = mouse_button_state;
                event.data2 = AccelerateMouse(sdlevent.motion.xrel);
                event.data3 = -AccelerateMouse(sdlevent.motion.yrel);
                D_PostEvent(&event);
                break;
                */

            case SDL_MOUSEBUTTONDOWN:
                if (usemouse && !nomouse)
                {
                    if(sdlevent.button.button > 3)
                    {
                        UpdateMouseButtonState(sdlevent.button.button + 2, true);
                    }
                    else
                    {
                        UpdateMouseButtonState(sdlevent.button.button, true);
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (usemouse && !nomouse)
                {
                    if(sdlevent.button.button > 3)
                    {
                        UpdateMouseButtonState(sdlevent.button.button + 2, false);
                    }
                    else
                    {
                        UpdateMouseButtonState(sdlevent.button.button, false);
                    }
                }
                break;

            case SDL_MOUSEWHEEL:
                if(usemouse && !nomouse)
                {
                    if(sdlevent.wheel.y > 0)
                    {
                        UpdateMouseButtonState(4, true);
                        UpdateMouseButtonState(4, false);
                    }
                    else if(sdlevent.wheel.y < 0)
                    {
                        UpdateMouseButtonState(5, true);
                        UpdateMouseButtonState(5, false);
                    }
                }
                break;

            case SDL_APP_TERMINATING:
                I_Quit();
                break;

            case SDL_QUIT:
                event.type = ev_quit;
                D_PostEvent(&event);
                break;

            case SDL_WINDOWEVENT:
                if (sdlevent.window.event == SDL_WINDOWEVENT_EXPOSED)
                    palette_to_set = true;
                else if (sdlevent.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                    window_focused = false;
                // need to update our focus state
                UpdateFocus();
                break;

                // [SVE] svillarreal - this is a huge hassle to support with
                // the OpenGL context
#if 0
            case SDL_WINDOWEVENT_RESIZED:
                need_resize = true;
                resize_w = sdlevent.resize.w;
                resize_h = sdlevent.resize.h;
                last_resize_time = SDL_GetTicks();
                break;
#endif

            default:
                break;
        }
    }
}

// Warp the mouse back to the middle of the screen

static void CenterMouse(void)
{
    // Warp the the screen center

    int screenW, screenH;
    SDL_GetWindowSize(windowscreen, &screenW, &screenH);
    SDL_WarpMouseInWindow(windowscreen, screenW / 2, screenH / 2);

    // Clear any relative movement caused by warping

    SDL_PumpEvents();
    SDL_GetRelativeMouseState(NULL, NULL);
}

//
// Read the change in mouse state to generate mouse motion events
//
// This is to combine all mouse movement for a tic into one mouse
// motion event.

static void I_ReadMouse(void)
{
    static int last_x = 0, last_y = 0;
    static boolean did_filter = false;
    int x, y;
    event_t ev;

    // if the app services overlay would filter out mouse motion events,
    // then do not read the raw mouse position here either.
    if(gAppServices->OverlayEventFilter(SDL_MOUSEMOTION))
    {
        did_filter = true;
        return;
    }
    else if(did_filter)
    {
        // we need to warp the mouse if the condition above was true and
        // then stops being so.
        CenterMouse();
        did_filter = false;
    }

    SDL_GetRelativeMouseState(&x, &y);

    // [SVE] svillarreal
    if(mouse_scale > 4) mouse_scale = 4;
    if(mouse_scale < 0) mouse_scale = 0;

    if(x != 0 || y != 0) 
    {
        // [SVE] svillarreal
        if(mouse_scale > 0)
        {
            x *= (1 + mouse_scale);
            y *= (0 + mouse_scale);
        }

        ev.type = ev_mouse;
        ev.data1 = 0;
        ev.data2 = AccelerateMouse(mouse_smooth ? ((x + last_x) / 2) : x);

        // [SVE]: "novert" not supported with this meaning; instead we disable
        // mouse look, since we don't support moving forward with the mouse, and
        // we require it to be active in the menus.
        //if (!novert)
        {
            ev.data3 = -AccelerateMouse(mouse_smooth ? ((y + last_y) / 2) : y);
        }
        /*
        else
        {
            ev.data3 = 0;
        }
        */
        
        D_PostEvent(&ev);
        i_seemouses = true;
        i_seejoysticks = false;

        last_x = x;
        last_y = y;
    }

    if (MouseShouldBeGrabbed())
    {
        //CenterMouse();
    }
}

//
// I_StartTic
//
void I_StartTic (void)
{
    if (!initialized)
    {
        return;
    }

    // [SVE]:
    // if doing in-game options, call the frontend responder for all input
    if(FE_InOptionsMenu())
    {
        if(FE_InGameOptionsResponder())
            FE_EndInGameOptionsMenu();
    }
    else
    {
        I_GetEvent();

        if (usemouse && !nomouse)
        {
            I_ReadMouse();
        }

        I_UpdateJoystick();
    }
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

static void UpdateGrab(void)
{
    static boolean currently_grabbed = false;
    boolean grab;

    grab = MouseShouldBeGrabbed();

    if (screensaver_mode)
    {
        // Hide the cursor in screensaver mode

        I_SetShowCursor(false);
    }
    else if (grab && !currently_grabbed)
    {
        I_SetShowCursor(false);
        CenterMouse();
    }
    else if (!grab && currently_grabbed)
    {
        I_SetShowCursor(true);

        // When releasing the mouse from grab, warp the mouse cursor to
        // the bottom-right of the screen. This is a minimally distracting
        // place for it to appear - we may only have released the grab
        // because we're at an end of level intermission screen, for
        // example.

        int screenW, screenH;
        SDL_GetWindowSize(windowscreen, &screenW, &screenH);
        SDL_WarpMouseInWindow(windowscreen, screenW, screenH);
        SDL_GetRelativeMouseState(NULL, NULL);
    }

    currently_grabbed = grab;

}

// Update a small portion of the screen
//
// Does stretching and buffer blitting if neccessary
//
// Return true if blit was successful.

static boolean BlitArea(int x1, int y1, int x2, int y2)
{
    int x_offset, y_offset;
    boolean result;

    x_offset = (screenbuffer->w - screen_mode->width) / 2;
    y_offset = (screenbuffer->h - screen_mode->height) / 2;

    if (SDL_LockSurface(screenbuffer) >= 0)
    {
        I_InitScale(I_VideoBuffer,
                    (byte *) screenbuffer->pixels
                                + (y_offset * screenbuffer->pitch)
                                + x_offset,
                    screenbuffer->pitch);
        result = screen_mode->DrawScreen(x1, y1, x2, y2);
      	SDL_UnlockSurface(screenbuffer);
    }
    else
    {
        result = false;
    }

    return result;
}

static void UpdateRect(int x1, int y1, int x2, int y2)
{
    int x1_scaled, x2_scaled, y1_scaled, y2_scaled;

    // Do stretching and blitting

    if (BlitArea(x1, y1, x2, y2))
    {
        // Update the area

        x1_scaled = (x1 * screen_mode->width) / SCREENWIDTH;
        y1_scaled = (y1 * screen_mode->height) / SCREENHEIGHT;
        x2_scaled = (x2 * screen_mode->width) / SCREENWIDTH;
        y2_scaled = (y2 * screen_mode->height) / SCREENHEIGHT;

        SDL_RenderPresent(renderer);
    }
}

void I_BeginRead(void)
{
    byte *screenloc = I_VideoBuffer
                    + (SCREENHEIGHT - LOADING_DISK_H) * SCREENWIDTH
                    + (SCREENWIDTH - LOADING_DISK_W);
    int y;

    // [SVE] svillarreal - from gl scale branch
    if (!initialized || disk_image == NULL || using_opengl)
        return;

    // save background and copy the disk image in

    for (y=0; y<LOADING_DISK_H; ++y)
    {
        memcpy(saved_background + y * LOADING_DISK_W,
               screenloc,
               LOADING_DISK_W);
        memcpy(screenloc,
               disk_image + y * LOADING_DISK_W,
               LOADING_DISK_W);

        screenloc += SCREENWIDTH;
    }

    UpdateRect(SCREENWIDTH - LOADING_DISK_W, SCREENHEIGHT - LOADING_DISK_H,
               SCREENWIDTH, SCREENHEIGHT);
}

void I_EndRead(void)
{
    byte *screenloc = I_VideoBuffer
                    + (SCREENHEIGHT - LOADING_DISK_H) * SCREENWIDTH
                    + (SCREENWIDTH - LOADING_DISK_W);
    int y;

    // [SVE] svillarreal - from gl scale branch
    if (!initialized || disk_image == NULL || using_opengl)
        return;

    // save background and copy the disk image in

    for (y=0; y<LOADING_DISK_H; ++y)
    {
        memcpy(screenloc,
               saved_background + y * LOADING_DISK_W,
               LOADING_DISK_W);

        screenloc += SCREENWIDTH;
    }

    UpdateRect(SCREENWIDTH - LOADING_DISK_W, SCREENHEIGHT - LOADING_DISK_H,
               SCREENWIDTH, SCREENHEIGHT);
}

// [SVE] svillarreal - from gl scale branch
// Ending of I_FinishUpdate() when in software scaling mode.

static void FinishUpdateSoftware(void)
{
    // draw to screen

    //BlitArea(0, 0, SCREENWIDTH, SCREENHEIGHT);

    if (palette_to_set)
    {
        SDL_SetPaletteColors(screenbuffer->format->palette, palette, 0, 256);
        palette_to_set = false;

        if(1)
        {
            // "flash" the pillars/letterboxes with palette changes, emulating
            // VGA "porch" behaviour (GitHub issue #832)
            SDL_SetRenderDrawColor(renderer, palette[0].r, palette[0].g,
                palette[0].b, SDL_ALPHA_OPAQUE);
        }

        // In native 8-bit mode, if we have a palette to set, the act
        // of setting the palette updates the screen
        // Edward: Still relevant?
        /*if (screenbuffer == screen)
        {
            return;
        }*/
    }

    // Blit from the paletted 8-bit screen buffer to the intermediate
    // 32-bit RGBA buffer that we can load into the texture.

    SDL_LowerBlit(screenbuffer, &blit_rect, argbbuffer, &blit_rect);

    // Update the intermediate texture with the contents of the RGBA buffer.

    SDL_UpdateTexture(texture, NULL, argbbuffer->pixels, argbbuffer->pitch);

    // Make sure the pillarboxes are kept clear each frame.

    SDL_RenderClear(renderer);

    // Render this intermediate texture into the upscaled texture
    // using "nearest" integer scaling.

    SDL_SetRenderTarget(renderer, texture_upscaled);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Finally, render this upscaled texture to screen using linear scaling.

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, texture_upscaled, NULL, NULL);

    // Draw!

    SDL_RenderPresent(renderer);
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    static int	lasttic;
    int		tics;
    int		i;
    Uint32  wflags;

    if (!initialized)
        return;

    if (noblit)
        return;

    if (need_resize && SDL_GetTicks() > last_resize_time + 500)
    {
        ApplyWindowResize(resize_w, resize_h);
        need_resize = false;
        palette_to_set = true;
    }

    UpdateGrab();

    // Don't update the screen if the window isn't visible.

    wflags = windowscreen ? SDL_GetWindowFlags(windowscreen) : 0;
    if (!(wflags & SDL_WINDOW_SHOWN) || (wflags & SDL_WINDOW_MINIMIZED))
        return;

    // draws little dots on the bottom of the screen

    if(display_fps_dots)
    {
	    i = I_GetTime();
	    tics = i - lasttic;
	    lasttic = i;
	    if (tics > 20) tics = 20;

	    for (i=0 ; i<tics*4 ; i+=4)
	        I_VideoBuffer[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	    for ( ; i<20*4 ; i+=4)
	        I_VideoBuffer[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

    // draw to screen
    // [SVE] svillarreal - from gl scale branch
    if (using_opengl)
    {
        int mouse_x, mouse_y;

        if(!use3drenderer)
        {
            I_GL_UpdateScreen(I_VideoBuffer, palette);
        }
        else
        {
            M_HelpDrawerGL();
            RB_DrawPatchBuffer();
        }

        if(show_visual_cursor && !gAppServices->OverlayActive())
        {
            if(i_seemouses || !i_seejoysticks) // haleyjd 20141202: this is overtime work.
            {
                SDL_GetMouseState(&mouse_x, &mouse_y);
#if !defined(LUNA_RELEASE) // only use hardware cursor on Luna
                RB_DrawMouseCursor(mouse_x, mouse_y);
#endif
            }
        }

        RB_SwapBuffers();
    }
    else
    {
        FinishUpdateSoftware();
    }
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy(scr, I_VideoBuffer, SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte *doompalette)
{
    int i;

    for (i=0; i<256; ++i)
    {
        // Zero out the bottom two bits of each channel - the PC VGA
        // controller only supports 6 bits of accuracy.

        palette[i].r = gammatable[usegamma][*doompalette++] & ~3;
        palette[i].g = gammatable[usegamma][*doompalette++] & ~3;
        palette[i].b = gammatable[usegamma][*doompalette++] & ~3;
    }

    palette_to_set = true;
}

// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex(int r, int g, int b)
{
    int best, best_diff, diff;
    int i;

    best = 0; best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        diff = (r - palette[i].r) * (r - palette[i].r)
             + (g - palette[i].g) * (g - palette[i].g)
             + (b - palette[i].b) * (b - palette[i].b);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }

    return best;
}

//
// I_GetPaletteColor
//

void I_GetPaletteColor(byte *rgb, int index)
{
    rgb[0] = palette[index].r;
    rgb[1] = palette[index].g;
    rgb[2] = palette[index].b;
}

// 
// Set the window title
//

void I_SetWindowTitle(char *title)
{
    window_title = title;
}

//
// Call the SDL function to set the window title, based on 
// the title set with I_SetWindowTitle.
//

void I_InitWindowTitle(void)
{
    // haleyjd 20140827: [SVE] hard coded title
    char *buf = "Strife: Veteran Edition";

    buf = M_StringJoin(window_title, " - ", PACKAGE_STRING, NULL);
    SDL_SetWindowTitle(windowscreen, buf);
    free(buf);
}

// Set the application icon

void I_InitWindowIcon(void)
{
    SDL_Surface *surface;

    surface = SDL_CreateRGBSurfaceFrom(icon_data, icon_w, icon_h,
                                       24, icon_w * 3,
                                       0xff << 0, 0xff << 8, 0xff << 16, 0);

    SDL_SetWindowIcon(windowscreen, surface);
    SDL_FreeSurface(surface);
}

// Pick the modes list to use:

static void GetScreenModes(screen_mode_t ***modes_list, int *num_modes)
{
    if (aspect_ratio_correct)
    {
        *modes_list = screen_modes_corrected;
        *num_modes = arrlen(screen_modes_corrected);
    }
    else
    {
        *modes_list = screen_modes;
        *num_modes = arrlen(screen_modes);
    }
}

// Find which screen_mode_t to use for the given width and height.

static screen_mode_t *I_FindScreenMode(int w, int h)
{
    screen_mode_t **modes_list;
    screen_mode_t *best_mode;
    int modes_list_length;
    int num_pixels;
    int best_num_pixels;
    int i;

    // [SVE] svillarreal - from gl scale branch
    // In OpenGL mode the rules are different. We can have any
    // resolution, though it needs to match the aspect ratio we
    // expect.

    if (using_opengl)
    {
        static screen_mode_t gl_mode;
        int screenheight;
        float screenwidth;

        if (aspect_ratio_correct)
        {
            screenheight = SCREENHEIGHT_4_3;
        }
        else
        {
            screenheight = SCREENHEIGHT;
        }
        
        // [SVE] svillarreal - if using 3d renderer then account for widescreen resolutions
        screenwidth = use3drenderer ? (float)screenheight * (float)w / (float)h : SCREENWIDTH;
        screenwidth = ((float)h * screenwidth / (float)screenheight);

        gl_mode.width = (int)screenwidth;
        gl_mode.height = h;
        gl_mode.InitMode = NULL;
        gl_mode.DrawScreen = NULL;
        gl_mode.poor_quality = false;

        return &gl_mode;
    }

    // Special case: 320x200 and 640x400 are available even if aspect 
    // ratio correction is turned on.  These modes have non-square
    // pixels.

    if (fullscreen)
    {
        if (w == SCREENWIDTH && h == SCREENHEIGHT)
        {
            return &mode_scale_1x;
        }
        else if (w == SCREENWIDTH*2 && h == SCREENHEIGHT*2)
        {
            return &mode_scale_2x;
        }
    }

    GetScreenModes(&modes_list, &modes_list_length);

    // Find the biggest screen_mode_t in the list that fits within these 
    // dimensions

    best_mode = NULL;
    best_num_pixels = 0;

    for (i=0; i<modes_list_length; ++i) 
    {
        // Will this fit within the dimensions? If not, ignore.

        if (modes_list[i]->width > w || modes_list[i]->height > h)
        {
            continue;
        }

        num_pixels = modes_list[i]->width * modes_list[i]->height;

        if (num_pixels > best_num_pixels)
        {
            // This is a better mode than the current one

            best_mode = modes_list[i];
            best_num_pixels = num_pixels;
        }
    }

    return best_mode;
}

// Adjust to an appropriate fullscreen mode.
// Returns true if successful.

static boolean AutoAdjustFullscreen(void)
{
    SDL_DisplayMode *modes;
    SDL_DisplayMode *best_mode;
    screen_mode_t *screen_mode;
    int diff, best_diff;
    int i;
    int displayindex, numdisplaymodes;

    //SDL_Window *temp;

    // Based on SDL_GetWindowDisplayIndex and SDL_CreateWindowAndRenderer
    // we can be certain that displayindex == 0.
    displayindex = 0;
    if((numdisplaymodes = SDL_GetNumDisplayModes(displayindex)) >= 1)
    {
       modes = Z_Calloc(numdisplaymodes, sizeof(SDL_DisplayMode), PU_STATIC, NULL);
       for(i = 0; i < numdisplaymodes; ++i)
       {
          SDL_GetDisplayMode(displayindex, i, &modes[i]);
       }
    }
    else // No fullscreen modes available at all?
    {
        return false;
    }
    
    // [SVE]: On first time run, set desired res to best available res
    if(!screen_init && use3drenderer)
    {
        screen_width  = default_screen_width  = modes[0].w;
        screen_height = default_screen_height = modes[0].h;
        screen_init   = true;        // do not do this again
    }

    // Find the best mode that matches the mode specified in the
    // configuration file

    best_mode = NULL;
    best_diff = INT_MAX;

    for (i = 0; i < numdisplaymodes; ++i)
    {
        //printf("%ix%i?\n", modes[i]->w, modes[i]->h);

        // What screen_mode_t would be used for this video mode?

        screen_mode = I_FindScreenMode(modes[i].w, modes[i].h);

        // Never choose a screen mode that we cannot run in, or
        // is poor quality for fullscreen

        if (screen_mode == NULL || screen_mode->poor_quality)
        {
        //    printf("\tUnsupported / poor quality\n");
            continue;
        }

        // Do we have the exact mode?
        // If so, no autoadjust needed

        if (screen_width == modes[i].w && screen_height == modes[i].h)
        {
        //    printf("\tExact mode!\n");
            Z_Free(modes);

            return true;
        }

        // Is this mode better than the current mode?

        diff = (screen_width - modes[i].w) * (screen_width - modes[i].w)
             + (screen_height - modes[i].h) * (screen_height - modes[i].h);

        if (diff < best_diff)
        {
        //    printf("\tA valid mode\n");
            best_mode = modes + i;
            best_diff = diff;
        }
    }

    if (best_mode == NULL)
    {
        // Unable to find a valid mode!
        Z_Free(modes);

        return false;
    }

    printf("I_InitGraphics: %ix%i mode not supported on this machine.\n",
           screen_width, screen_height);

    screen_width  = default_screen_width  = best_mode->w;
    screen_height = default_screen_height = best_mode->h;

    Z_Free(modes);

    return true;
}

// Auto-adjust to a valid windowed mode.

static void AutoAdjustWindowed(void)
{
    screen_mode_t *best_mode;

    // Find a screen_mode_t to fit within the current settings

    best_mode = I_FindScreenMode(screen_width, screen_height);

    if (best_mode == NULL)
    {
        // Nothing fits within the current settings.
        // Pick the closest to 320x200 possible.

        best_mode = I_FindScreenMode(SCREENWIDTH, SCREENHEIGHT_4_3);
    }

    // Switch to the best mode if necessary.

    if (best_mode->width != screen_width || best_mode->height != screen_height)
    {
        printf("I_InitGraphics: Cannot run at specified mode: %ix%i\n",
               screen_width, screen_height);

        screen_width  = default_screen_width  = best_mode->width;
        screen_height = default_screen_height = best_mode->height;
    }
}

// If the video mode set in the configuration file is not available,
// try to choose a different mode.

static void I_AutoAdjustSettings(void)
{
    int old_screen_w, old_screen_h;

    old_screen_w = screen_width;
    old_screen_h = screen_height;

    // If we are running fullscreen, try to autoadjust to a valid fullscreen
    // mode.  If this is impossible, switch to windowed.

    if (fullscreen && !AutoAdjustFullscreen())
    {
        fullscreen = default_fullscreen = 0;
    }

    // If we are running windowed, pick a valid window size.

    if (!fullscreen)
    {
        AutoAdjustWindowed();
    }

    // Have the settings changed?  Show a message.

    if (screen_width != old_screen_w || screen_height != old_screen_h)
    {
        printf("I_InitGraphics: Auto-adjusted to %ix%ix.\n",
               screen_width, screen_height);

        printf("NOTE: Your video settings have been adjusted.  "
               "To disable this behavior,\n"
               "set autoadjust_video_settings to 0 in your "
               "configuration file.\n");
    }
}

// Set video size to a particular scale factor (1x, 2x, 3x, etc.)

static void SetScaleFactor(int factor)
{
    int w, h;

    // Pick 320x200 or 320x240, depending on aspect ratio correct

    if (aspect_ratio_correct)
    {
        w = SCREENWIDTH;
        h = SCREENHEIGHT_4_3;
    }
    else
    {
        w = SCREENWIDTH;
        h = SCREENHEIGHT;
    }

    screen_width  = default_screen_width  = w * factor;
    screen_height = default_screen_height = h * factor;
}

void I_GraphicsCheckCommandLine(void)
{
    int i;

    //!
    // @vanilla
    //
    // Disable blitting the screen.
    //

    noblit = M_CheckParm ("-noblit"); 

    //!
    // @category video 
    //
    // Grab the mouse when running in windowed mode.
    //

    if (M_CheckParm("-grabmouse"))
    {
        grabmouse = true;
    }

    //!
    // @category video 
    //
    // Don't grab the mouse when running in windowed mode.
    //

    if (M_CheckParm("-nograbmouse"))
    {
        grabmouse = false;
    }

    // default to fullscreen mode, allow override with command line
    // nofullscreen because we love prboom

    //!
    // @category video 
    //
    // Run in a window.
    //

    if (M_CheckParm("-window") || M_CheckParm("-nofullscreen"))
    {
        fullscreen = false;
    }

    //!
    // @category video 
    //
    // Run in fullscreen mode.
    //

    if (M_CheckParm("-fullscreen"))
    {
        fullscreen = true;
    }

    //!
    // @category video 
    //
    // Disable the mouse.
    //

    nomouse = M_CheckParm("-nomouse") > 0;

#ifdef SVE_PLAT_SWITCH
    nomouse = true;
#endif

    //!
    // @category video
    // @arg <x>
    //
    // Specify the screen width, in pixels.
    //

    i = M_CheckParmWithArgs("-width", 1);

    if (i > 0)
    {
        screen_width = atoi(myargv[i + 1]);
    }

    //!
    // @category video
    // @arg <y>
    //
    // Specify the screen height, in pixels.
    //

    i = M_CheckParmWithArgs("-height", 1);

    if (i > 0)
    {
        screen_height = atoi(myargv[i + 1]);
    }

    // Because we love Eternity:

    //!
    // @category video
    // @arg <WxY>
    //
    // Specify the screen mode (when running fullscreen) or the window
    // dimensions (when running in windowed mode).

    i = M_CheckParmWithArgs("-geometry", 1);

    if (i > 0)
    {
        int w, h;

        if (sscanf(myargv[i + 1], "%ix%i", &w, &h) == 2)
        {
            screen_width = w;
            screen_height = h;
        }
    }

    //!
    // @category video
    //
    // Don't scale up the screen.
    //

    if (M_CheckParm("-1")) 
    {
        SetScaleFactor(1);
    }

    //!
    // @category video
    //
    // Double up the screen to 2x its normal size.
    //

    if (M_CheckParm("-2")) 
    {
        SetScaleFactor(2);
    }

    //!
    // @category video
    //
    // Double up the screen to 3x its normal size.
    //

    if (M_CheckParm("-3")) 
    {
        SetScaleFactor(3);
    }

    //!
    // @category video
    //
    // Disable vertical mouse movement.
    //

    if (M_CheckParm("-novert"))
    {
        novert = true;
    }

    //!
    // @category video
    //
    // Enable vertical mouse movement.
    //

    if (M_CheckParm("-nonovert"))
    {
        novert = false;
    }
}

// Check if we have been invoked as a screensaver by xscreensaver.

void I_CheckIsScreensaver(void)
{
    char *env;

    env = getenv("XSCREENSAVER_WINDOW");

    if (env != NULL)
    {
        screensaver_mode = true;
    }
}

static void CreateCursors(void)
{
    static Uint8 empty_cursor_data = 0;

    // Save the default cursor so it can be recalled later

    cursors[1] = SDL_GetCursor();

    // Create an empty cursor

    cursors[0] = SDL_CreateCursor(&empty_cursor_data,
                                  &empty_cursor_data,
                                  1, 1, 0, 0);
}

static void SetSDLVideoDriver(void)
{
    // Allow a default value for the SDL video driver to be specified
    // in the configuration file.

    if (strcmp(video_driver, "") != 0)
    {
        char *env_string;

        env_string = M_StringJoin("SDL_VIDEODRIVER=", video_driver, NULL);
        putenv(env_string);
        free(env_string);
    }
}

static void SetWindowPositionVars(void)
{
    char buf[64];
    int x, y;

    if (window_position == NULL || !strcmp(window_position, ""))
    {
        return;
    }

    if (!strcmp(window_position, "center"))
    {
        putenv("SDL_VIDEO_CENTERED=1");
    }
    else if (sscanf(window_position, "%i,%i", &x, &y) == 2)
    {
        M_snprintf(buf, sizeof(buf), "SDL_VIDEO_WINDOW_POS=%i,%i", x, y);
        putenv(buf);
    }
}

static char *WindowBoxType(screen_mode_t *mode, int w, int h)
{
    if (mode->width != w && mode->height != h) 
    {
        return "Windowboxed";
    }
    else if (mode->width == w) 
    {
        return "Letterboxed";
    }
    else if (mode->height == h)
    {
        return "Pillarboxed";
    }
    else
    {
        return "...";
    }
}

static void SetVideoMode(screen_mode_t *mode, int w, int h)
{
    byte *doompal;
    int flags = 0;

    doompal = W_CacheLumpName(DEH_String("PLAYPAL"), PU_CACHE);

    // If we are already running and in a true color mode, we need
    // to free the screenbuffer surface before setting the new mode.

    // [SVE] svillarreal - from gl scale branch
    if (!using_opengl && screenbuffer != NULL)
    {
        SDL_FreeSurface(screenbuffer);
    }

    // Generate lookup tables before setting the video mode.

    if (mode != NULL && mode->InitMode != NULL)
    {
        mode->InitMode(doompal);
    }

    if (fullscreen)
    { 
#if 0//defined(SVE_PLAT_SWITCH)
        flags |= SDL_WINDOW_FULLSCREEN;
#else
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;	
#endif
    }
    else if (window_noborder)
    {
        flags |= SDL_WINDOW_BORDERLESS;
    }

    // [SVE] svillarreal - this is a huge hassle to support with
    // the OpenGL context
#if 0
    else
    {
        // In windowed mode, the window can be resized while the game is
        // running. Mac OS X has a quirk where an ugly resize handle is
        // shown in software mode when resizing is enabled, so avoid that.
#ifdef __MACOSX__
        if (using_opengl)
#endif
        {
            flags |= SDL_RESIZABLE;
        }
    }
#endif

    // [SVE] svillarreal - from gl scale branch
    if (using_opengl)
    {
        flags |= SDL_WINDOW_OPENGL;
    }

    windowscreen = SDL_CreateWindow("",  SDL_WINDOWPOS_CENTERED,  SDL_WINDOWPOS_CENTERED, w, h, flags);
    if (windowscreen == NULL)
    {
        I_Error("Could not construct window: %s\n", SDL_GetError());
    }

    if (using_opengl)
    {
        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
        }
        renderer = NULL;

#ifdef SVE_PLAT_SWITCH
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif

        GlContext = SDL_GL_CreateContext(windowscreen);
        if (GlContext == NULL)
        {
            I_Error("Error getting GL context: %s\n", SDL_GetError());
        }
#ifdef SVE_PLAT_SWITCH
        LoadContext(&ctx);
#endif
    }
    else
    {
        if (GlContext)
        {
            SDL_GL_DeleteContext(GlContext);
        }
        GlContext = NULL;

        renderer = SDL_CreateRenderer(windowscreen, -1, SDL_RENDERER_PRESENTVSYNC);
        if (!renderer)
        {
            I_Error("Error getting renderer: %s\n", SDL_GetError());
        }
    }
 
    if (windowscreen == NULL)
    {
        I_Error("Error setting video mode %ix%ix: %s\n",
                w, h, SDL_GetError());
    }

    // Set up window title and icon.
    // This must be done AFTER the window is created.
    I_InitWindowTitle();

    // TODO: SDL2 might being fine setting window icon on OSX/macOS now.
#ifndef __MACOSX__
    I_InitWindowIcon();
#endif

    // [SVE] svillarreal
    DEH_printf("GL_Init: Initializing OpenGL extensions\n");
    GL_Init();

    DEH_printf("RB_Init: Initializing OpenGL render backend\n");
    RB_Init();

    // [SVE] svillarreal - from gl scale branch
    if (using_opengl)
    {
        // Try to initialize OpenGL scaling backend. This can fail,
        // because we need an OpenGL context before we can find out
        // if we have all the extensions that we need to make it work.
        // If it does, then fall back to software mode instead.

#if 0
        SDL_Surface* surface = SDL_GetWindowSurface(windowscreen);

        if (!I_GL_InitScale(surface->w, surface->h))
#else
		int w;
		int h;
		SDL_GetWindowSize(windowscreen, &w, &h);
		if (!I_GL_InitScale(w, h))
#endif
        {
            fprintf(stderr,
                    "Failed to initialize in OpenGL mode. "
                    "Falling back to software mode instead.\n");
            using_opengl = false;

            // TODO: This leaves us in window with borders around it.
            // We shouldn't call with NULL here; this needs to be refactored
            // so that 'mode' isn't even an argument to this function.
            SetVideoMode(NULL, w, h);
            return;
        }
    }
    else
    {
        SDL_Surface* surface = SDL_GetWindowSurface(windowscreen);

        // Blank out the full screen area in case there is any junk in
        // the borders that won't otherwise be overwritten.

        SDL_FillRect(surface, NULL, 0);

        // If mode was not set, it must be set now that we know the
        // screen size.

        if (mode == NULL)
        {
            mode = I_FindScreenMode(surface->w, surface->h);

            if (mode == NULL)
            {
                I_Error("I_InitGraphics: Unable to find a screen mode small "
                        "enough for %ix%i", surface->w, surface->h);
            }

            // Generate lookup tables before setting the video mode.

            if (mode->InitMode != NULL)
            {
                mode->InitMode(doompal);
            }
        }

        // Save screen mode.

        screen_mode = mode;

        // Create the screenbuffer surface
        screenbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            mode->width, mode->height, 8,
                                            0, 0, 0, 0);

        SDL_FillRect(screenbuffer, NULL, 0);
    }
}

static void ApplyWindowResize(unsigned int w, unsigned int h)
{
    screen_mode_t *mode;

    // Find the biggest screen mode that will fall within these
    // dimensions, falling back to the smallest mode possible if
    // none is found.

    mode = I_FindScreenMode(w, h);

    if (mode == NULL)
    {
        mode = I_FindScreenMode(SCREENWIDTH, SCREENHEIGHT);
    }

    // Reset mode to resize window.

    printf("Resize to %ix%i\n", mode->width, mode->height);
    SetVideoMode(mode, mode->width, mode->height);

    // Save settings.

    screen_width  = default_screen_width  = mode->width;
    screen_height = default_screen_height = mode->height;
}

void I_InitGraphics(void)
{
    SDL_Event dummy;
    byte *doompal;
    char *env;
	const char *luna_width, *luna_height;

#if defined(SVE_PLAT_SWITCH)
    screen_width = 1280;
    screen_height = 720;
#elif LUNA_RELEASE
	// Ensure that no matter what is in the config file, we always use the
	// proper settings.
	fullscreen = 1;
	rbVsync = true;

	// On Luna, our fullscreen resolution is always our environment vars.
	luna_width = getenv("SOLSTICE_DISPLAY_RESOLUTION_WIDTH");
	luna_height = getenv("SOLSTICE_DISPLAY_RESOLUTION_HEIGHT");
	if (!luna_width || !luna_height)
	{
        luna_width  = "1920";
        luna_height = "1080";
	}

	screen_width = atoi(luna_width);
	screen_height = atoi(luna_height);
	if (!screen_width || !screen_height)
	{
		I_Error("Unknown display resolution");
	}
#endif

    // Pass through the XSCREENSAVER_WINDOW environment variable to 
    // SDL_WINDOWID, to embed the SDL window into the Xscreensaver
    // window.

    env = getenv("XSCREENSAVER_WINDOW");

    if (env != NULL)
    {
        char winenv[30];
        int winid;

        sscanf(env, "0x%x", &winid);
        M_snprintf(winenv, sizeof(winenv), "SDL_WINDOWID=%i", winid);

        putenv(winenv);
    }

    SetSDLVideoDriver();
    SetWindowPositionVars();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        I_Error("Failed to initialize video: %s", SDL_GetError());
    }
 
 

    // Warning to OS X users... though they might never see it :(
#ifdef __MACOSX__
    if (fullscreen)
    {
        printf("Some old versions of OS X might crash in fullscreen mode.\n"
               "If this happens to you, switch back to windowed mode.\n");
    }
#endif

    // [SVE] svillarreal - from gl scale branch
    // If we're using OpenGL, call the preinit function now; if it fails
    // then we have to fall back to software mode.

    if (using_opengl && !GL_PreInit())
    {
        using_opengl = false;
        // [SVE] svillarreal - OpenGL must be supported
        I_Error("Failed to initialize OpenGL");
        return;
    }

    //
    // Enter into graphics mode.
    //
    // When in screensaver mode, run full screen and auto detect
    // screen dimensions (don't change video mode)
    //

      if (screensaver_mode)
    {
        SetVideoMode(NULL, 0, 0);
    }
    else
    {
        int w, h;

        if (autoadjust_video_settings)
        {
            I_AutoAdjustSettings();
        }

#if defined(SVE_PLAT_SWITCH)
        w = screen_width = 1280;
        h = screen_height = 720;
#else
        w = screen_width;
        h = screen_height;
#endif

        screen_mode = I_FindScreenMode(w, h);

        if (screen_mode == NULL)
        {
            I_Error("I_InitGraphics: Unable to find a screen mode small "
                    "enough for %ix%i", w, h);
        }

        if (w != screen_mode->width || h != screen_mode->height)
        {
            printf("I_InitGraphics: %s (%ix%i within %ix%i)\n",
                   WindowBoxType(screen_mode, w, h),
                   screen_mode->width, screen_mode->height, w, h);
        }

        SetVideoMode(screen_mode, w, h);
    }

    // Set the palette

    doompal = W_CacheLumpName(DEH_String("PLAYPAL"), PU_CACHE);
    I_SetPalette(doompal);

    if (!using_opengl)
    {
        SDL_SetPaletteColors(screenbuffer->format->palette, palette, 0, 256);

        // Start with a clear black screen
        // (screen will be flipped after we set the palette)

        SDL_FillRect(screenbuffer, NULL, 0);
    }

    CreateCursors();

    UpdateFocus();
    UpdateGrab();

    // On some systems, it takes a second or so for the screen to settle
    // after changing modes.  We include the option to add a delay when
    // setting the screen mode, so that the game doesn't start immediately
    // with the player unable to see anything.

    if (fullscreen && !screensaver_mode)
    {
        SDL_Delay(startup_delay);
    }

	I_VideoBuffer = (unsigned char *) Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);

    V_RestoreBuffer();

    // Clear the screen to black.

    memset(I_VideoBuffer, 0, SCREENWIDTH * SCREENHEIGHT);

    // We need SDL to give us translated versions of keys as well

    //SDL_EnableUNICODE(1);

    // Repeat key presses - this is what Vanilla Doom does
    // Not sure about repeat rate - probably dependent on which DOS
    // driver is used.  This is good enough though.

    //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    // Clear out any events waiting at the start:

    while (SDL_PollEvent(&dummy));

    initialized = true;

    // Call I_ShutdownGraphics on quit
#ifndef SVE_PLAT_SWITCH
    I_AtExit(I_ShutdownGraphics, true);
#endif
}

// Bind all variables controlling video options into the configuration
// file system.

void I_BindVideoVariables(void)
{
    M_BindVariable("use_mouse",                 &usemouse);
    M_BindVariable("autoadjust_video_settings", &autoadjust_video_settings);
    M_BindVariable("aspect_ratio_correct",      &aspect_ratio_correct);
    M_BindVariable("startup_delay",             &startup_delay);
    M_BindVariable("screen_init",               &screen_init);
    M_BindVariable("grabmouse",                 &grabmouse);
    M_BindVariable("mouse_acceleration",        &mouse_acceleration);
    M_BindVariable("mouse_threshold",           &mouse_threshold);
    M_BindVariable("mouse_scale",               &mouse_scale);
    M_BindVariable("mouse_invert",              &mouse_invert);
    M_BindVariable("mouse_enable_acceleration", &mouse_enable_acceleration);
    M_BindVariable("mouse_smooth",              &mouse_smooth);
    M_BindVariable("video_driver",              &video_driver);
    M_BindVariable("window_position",           &window_position);
    M_BindVariable("usegamma",                  &usegamma);
    M_BindVariable("vanilla_keyboard_mapping",  &vanilla_keyboard_mapping);
    M_BindVariable("novert",                    &novert);
    M_BindVariable("gl_max_scale",              &gl_max_scale);
    M_BindVariable("png_screenshots",           &png_screenshots);

    // [SVE]
    M_BindVariableWithDefault("fullscreen",      &fullscreen,      &default_fullscreen);
    M_BindVariableWithDefault("window_noborder", &window_noborder, &default_window_noborder);
    M_BindVariableWithDefault("screen_width",    &screen_width,    &default_screen_width);
    M_BindVariableWithDefault("screen_height",   &screen_height,   &default_screen_height);

    // Disable fullscreen by default on OS X, as there is an SDL bug
    // where some old versions of OS X (<= Snow Leopard) crash.

#ifdef __MACOSX__
    fullscreen    = default_fullscreen    = 0;
    screen_width  = default_screen_width  = 800;
    screen_height = default_screen_height = 600;
    screen_init = true;
#endif
}
