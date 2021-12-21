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

#include <time.h>
#include <stdlib.h>

#include "SDL.h"

#include "i_social.h"
#include "doomstat.h"
#include "rb_local.h"
#include "z_zone.h"

#include "doomkeys.h"
#include "doomtype.h"
#include "f_wipe.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_controls.h"
#include "sounds.h"
#include "s_sound.h"

#include "fe_characters.h"
#include "fe_commands.h"
#include "fe_frontend.h"
#include "fe_graphics.h"
#include "fe_menuengine.h"
#include "fe_menus.h"
#include "fe_multiplayer.h"
#include "fe_gamepad.h"

extern int TranslateKey(SDL_Keysym *sym);

//
// State Vars
//

static int      frontend_tic;       // current tic
static boolean  frontend_done;      // if becomes true, frontend will exit
static boolean  frontend_running;   // currently in frontend
static boolean  frontend_autolobby; // started with +connect_lobby
static char    *frontend_lobbyid;   // lobby ID when frontend_autolobby is true


int      frontend_fpslimit = 60;
int      frontend_state;        // current state, vis-a-vis above enum
int      frontend_sigil;        // which sigil cursor to show
int      frontend_sgcount;      // sigil cursor counter
int      frontend_laser;        // which laser cursor to show
boolean  frontend_wipe;         // wipe wanted
boolean  frontend_ingame;       // doing in-game options menu access
boolean  frontend_waitframe;    // svillarreal: let the frame finish after a wipe

const char *frontend_modalmsg; // message to display if in modal net state

extern SDL_Window *windowscreen;

//
// Mark the frontend as finished; the game proper will start after the end of
// the current main loop iteration.
//
void FE_CmdGo(void)
{
    frontend_done = true;
    frontend_wipe = true;
}

//
// Prompt to exit the game
//
void FE_CmdExit(void)
{
    frontend_state = FE_STATE_EXITING;
    curCharacter   = FE_GetCharacter();
    I_StartVoice(curCharacter->voice);
}

//=============================================================================
//
// Input State
//

static boolean fe_window_focused;

void FE_UpdateFocus(void)
{
    Uint8 state;
    static boolean currently_focused = false;

    SDL_PumpEvents();

    state = SDL_GetWindowFlags(windowscreen);

    //fe_window_focused = (state & SDL_APPINPUTFOCUS) && (state & SDL_APPACTIVE);
    fe_window_focused = true;

    // The above check can still be true even if Steam is eating input, but to
    // SDL it's the same as having lost focus.
    if(gAppServices->OverlayActive())
        fe_window_focused = false;

    if(currently_focused != fe_window_focused)
    {
        if(fe_window_focused)
        {
            SDL_Event ev;
            while(SDL_PollEvent(&ev));
            /*SDL_EnableKeyRepeat(
                SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL
            );*/
        }
        /*else
            SDL_EnableKeyRepeat(0, 0);*/

        currently_focused = fe_window_focused;
    }
}

//=============================================================================
//
// Ticking - Engine Logic
//

//
// Returns true if the game is in a modal network game waiting state.
//
static boolean FE_InModalNetState(void)
{
    boolean res = false;

    switch(frontend_state)
    {
    case FE_STATE_LOBBYCREATE:
    case FE_STATE_LOBBYJOIN:
    case FE_STATE_REFRESH:
    case FE_STATE_GAMESTART:
        res = true;
        break;
    default:
        break;
    }

    return res;
}

static boolean feInOverlay;
static boolean fePrevInOverlay;

//
// Do per-tic logic for the frontend.
//
static void FE_Ticker(void)
{
    ++frontend_tic;

    // update app services provider status
    gAppServices->Update();

    // check for overlay state change
    fePrevInOverlay = feInOverlay;
    feInOverlay     = !!gAppServices->OverlayActive();
    if(!feInOverlay && fePrevInOverlay)
        I_SetShowVisualCursor(true);

    // animate sigil cursor
    if(--frontend_sgcount <= 0)
    {
        frontend_sigil = (frontend_sigil + 1) % 8;
        frontend_sgcount = frontend_ingame ? 5 : 10;
    }

    // animate laser cursor
    if(!(frontend_tic & (frontend_ingame ? 7 : 15)))
        frontend_laser ^= 1;

    // advance character with time, approx. every 2 seconds
    if(frontend_state != FE_STATE_EXITING)
    {
        if(!(frontend_tic & 127))
            curCharacter = FE_GetCharacter();
    }

    // animate merchant
    FE_MerchantTick();

    // run gamepad button or axis ticker?
    if(frontend_state == FE_STATE_JBINPUT)
        FE_JoyBindTicker();
    else if(frontend_state == FE_STATE_JAINPUT)
        FE_JoyAxisTicker();

    // run modal net loop function if one is set
    if(feModalLoopFunc)
        feModalLoopFunc();

    // check for lobby invitations
    if(frontend_autolobby)
    {
        FE_JoinLobbyFromConnectStr(frontend_lobbyid);
        frontend_autolobby = false; // once only if at startup
    }
    else if(FE_CheckLobbyListener())
        FE_JoinLobbyFromInvite();

    // check for lobby owner status upgrade
    FE_CheckForLobbyUpgrade();

    // check for client game join
    FE_ClientCheckForGameStart();
}

//=============================================================================
//
// Drawing/Rendering
//

//
// Render the frontend.
//
static void FE_Drawer(void)
{
    boolean wipe = false;

    if (use3drenderer)
    {
        RB_CheckReInitDrawer();
    }

    if(frontend_wipe)
    {
        frontend_wipe = false;

        if(!netgame)
        {
            wipe = true;
            wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
        }
    }

    if(use3drenderer)
    {
        dglClearColor(0, 0, 0, 1);
        RB_ClearBuffer(GLCB_COLOR);
    }

    if(frontend_done) // exiting?
        FE_ClearScreen();
    else
    {
        // draw current menu
        FE_MenuDraw(currentFEMenu);
    }

    // special state drawing
    switch(frontend_state)
    {
    case FE_STATE_KEYINPUT:
        FE_DrawBox(64, 84, 192, 32);
        FE_WriteYellowTextCentered(90,  "Press a key...");
        FE_WriteYellowTextCentered(102, "(Press bound key to unbind)");
        break;
    case FE_STATE_MBINPUT:
        FE_DrawBox(64, 84, 192, 32);
        FE_WriteYellowTextCentered(90,  "Click a button...");
        FE_WriteYellowTextCentered(102, "(Backspace to unbind)");
        break;
    case FE_STATE_JBINPUT:
        FE_JoyBindDrawer();
        break;
    case FE_STATE_JAINPUT:
        FE_JoyAxisBindDrawer();
        break;
    case FE_STATE_RESETCON:
        FE_DrawBox(64, 84, 192, 32);
        FE_WriteYellowTextCentered(90, "Reset controls to defaults?");
        FE_WriteYellowTextCentered(102, "(Press confirm or cancel)");
        FE_NX_DrawToolTips(5);
        break;
    case FE_STATE_EXITING:
        // draw character to ask quit question
        FE_DrawChar();
        break;
    default:
        // draw modal net state message?
        if(FE_InModalNetState() && frontend_modalmsg)
        {
            FE_DrawBox(64, 84, 192, 32);
            FE_WriteYellowTextCentered(90, frontend_modalmsg);
            FE_WriteYellowTextCentered(102, "(Please wait)");
        }
        break;
    }

    // wipe, or just finish update if not wiping
    if(wipe)
        FE_DoWipe();
    else
    {
        I_FinishUpdate();
        frontend_waitframe = false;
    }
}

//=============================================================================
//
// Input Events
//

//
// Menu Forward action
//
static void FE_HandleMenuForward(femenuitem_t *mi)
{
    switch(mi->type)
    {
    case FE_MITEM_CMD:
    case FE_MITEM_LOBBY:
        FE_ExecCmd(mi->verb);
        break;
    case FE_MITEM_TOGGLE:
        FE_ToggleVar(mi);
        break;
    case FE_MITEM_KEYBIND:
        FE_StartKB(mi);
        break;
    case FE_MITEM_MBIND:
        FE_StartMB(mi);
        break;
    case FE_MITEM_ABIND:
        FE_JoyAxisBindStart(mi);
        break;
    case FE_MITEM_JBIND:
        FE_JoyBindStart(mi);
        break;
    case FE_MITEM_SLIDER:
        FE_IncrementVariable(mi->verb);
        break;
    case FE_MITEM_VIDMODE:
        FE_NextVideoMode();
        break;
    case FE_MITEM_MPREADY:
        FE_ToggleLobbyReady();
        break;
    case FE_MITEM_MPTEAM:
        FE_IncrementTeam();
        break;
    case FE_MITEM_VALUES:
        FE_IncrementValue(mi->verb);
        break;
    case FE_MITEM_MUSIC:
        FE_MusicTestPlaySelection();
        break;
    default:
        break;
    }
}

//
// Menu Left action
//
static void FE_HandleMenuLeft(femenuitem_t *mi)
{
    switch(mi->type)
    {
    case FE_MITEM_TOGGLE:
        FE_ToggleVar(mi);
        break;
    case FE_MITEM_SLIDER:
        FE_DecrementVariable(mi->verb);
        break;
    case FE_MITEM_VIDMODE:
        FE_PrevVideoMode();
        break;
    case FE_MITEM_MPREADY:
        FE_ToggleLobbyReady();
        break;
    case FE_MITEM_MPTEAM:
        FE_DecrementTeam();
        break;
    case FE_MITEM_VALUES:
    case FE_MITEM_MUSIC:
        FE_DecrementValue(mi->verb);
        break;
    default:
        break;
    }
}

//
// Menu Right action
//
static void FE_HandleMenuRight(femenuitem_t *mi)
{
    switch(mi->type)
    {
    case FE_MITEM_TOGGLE:
        FE_ToggleVar(mi);
        break;
    case FE_MITEM_SLIDER:
        FE_IncrementVariable(mi->verb);
        break;
    case FE_MITEM_VIDMODE:
        FE_NextVideoMode();
        break;
    case FE_MITEM_MPREADY:
        FE_ToggleLobbyReady();
        break;
    case FE_MITEM_MPTEAM:
        FE_IncrementTeam();
        break;
    case FE_MITEM_VALUES:
    case FE_MITEM_MUSIC:
        FE_IncrementValue(mi->verb);
        break;
    default:
        break;
    }
}

//
// Menu Up action
//
static void FE_HandleMenuUp(void)
{
    int itemon = currentFEMenu->itemon;

    do
    {             
        currentFEMenu->itemon--;
        if(currentFEMenu->itemon < 0)
            currentFEMenu->itemon = currentFEMenu->numitems - 1;
    }
    while(!FE_IsSelectable(&(currentFEMenu->items[currentFEMenu->itemon])) &&
          currentFEMenu->itemon != itemon);

    if(currentFEMenu->itemon != itemon)
    {
        S_StartSound(NULL, sfx_pstop);
        if(merchantOn)
            FE_MerchantSetState(S_MRGT_00);
    }
}

//
// Menu Down action
//
static void FE_HandleMenuDown(void)
{
    int itemon = currentFEMenu->itemon;

    do
    {
        currentFEMenu->itemon++;
        if(currentFEMenu->itemon >= currentFEMenu->numitems)
            currentFEMenu->itemon = 0;
    }
    while(!FE_IsSelectable(&(currentFEMenu->items[currentFEMenu->itemon])) &&
          currentFEMenu->itemon != itemon);

    if(currentFEMenu->itemon != itemon)
    {
        S_StartSound(NULL, sfx_pstop);
        if(merchantOn)
            FE_MerchantSetState(S_MRGT_00);
    }
}

//
// Handle a keydown event
//
static void FE_HandleKey(SDL_Event *ev)
{
    femenuitem_t *mi = &(currentFEMenu->items[currentFEMenu->itemon]);

    int key = TranslateKey(&(ev->key.keysym));

    // exiting state
    if(frontend_state == FE_STATE_EXITING)
    {
        if(key == key_menu_confirm)
            I_Quit();
        else
        {
            frontend_state = FE_STATE_MAINMENU;
            I_StartVoice(NULL);
        }
        return;
    }

    // key input state
    if(frontend_state == FE_STATE_KEYINPUT)
    {
        if(FE_SetKeybinding(fe_kbitem, key))
            frontend_state = FE_STATE_MAINMENU;
        return;
    }

    // mouse button input state
    if(frontend_state == FE_STATE_MBINPUT)
    {
        if(key == KEY_BACKSPACE)
            FE_ClearMouseButton(fe_kbitem);
        return;
    }

    if(key == key_menu_forward)
    {
        FE_HandleMenuForward(mi);
    }
    else if(key == key_menu_left)
    {
        FE_HandleMenuLeft(mi);
    }
    else if(key == key_menu_right)
    {
        FE_HandleMenuRight(mi);
    }
    else if(key == key_menu_up)
    {
        FE_HandleMenuUp();
    }
    else if(key == key_menu_down)
    {
        FE_HandleMenuDown();
    }
    else if(key == key_menu_activate)
    {
        FE_PopMenu(false);
    }
    else
    {
        char ch;

        // jump to item by first letter
        if(!ev->key.keysym.sym || ev->key.keysym.sym >= 0x7f)
            return;

        ch = (char)(ev->key.keysym.sym & 0x7f);
        ch = tolower(ch);

        if(isalnum(ch))
        {
            int n = currentFEMenu->itemon;
            do
            {
                ++n;
                if(currentFEMenu->items[n].type == FE_MITEM_END)
                    n = 0;

                if(FE_IsSelectable(&(currentFEMenu->items[n])))
                {
                    if(tolower(currentFEMenu->items[n].description[0]) == ch)
                    {
                        if(n != currentFEMenu->itemon)
                        {
                            S_StartSound(NULL, sfx_pstop);
                            if(merchantOn)
                                FE_MerchantSetState(S_MRGT_00);
                        }
                        currentFEMenu->itemon = n;
                        break;
                    }
                }
            }
            while(n != currentFEMenu->itemon);
        }
    }
}

//
// Handle gamepad/joystick buttons
//
static void FE_HandleJoyButtons(int joybuttons)
{
    femenuitem_t *mi = &(currentFEMenu->items[currentFEMenu->itemon]);
    int i;

    // exiting?
    if(frontend_state == FE_STATE_EXITING)
    {
        if(joybmenu_confirm >= 0 && (joybuttons & (1 << joybmenu_confirm)))
            I_Quit();
        else if(joybuttons)
        {
            frontend_state = FE_STATE_MAINMENU;
            I_StartVoice(NULL);
        }
        return;
    }
    else if(frontend_state == FE_STATE_RESETCON)
    {
        if (joybmenu_back >= 0 && (joybuttons & (1 << joybmenu_back)))
        {
            frontend_state = FE_STATE_MAINMENU;
        }
        else if (joybmenu_forward >= 0 && (joybuttons & (1 << joybmenu_forward)))
        {
            frontend_state = FE_STATE_MAINMENU;
            FE_AutoApplyPadProfile();
            joy_gyrosensitivityh = 0.8f;
            joy_gyrosensitivityv = 0.5f;
            joy_gyroscope = 1;
            joy_gyrostyle = 0;
        }
        return;
    }
    else if(frontend_state == FE_STATE_KEYINPUT || frontend_state == FE_STATE_MBINPUT)
    {
        // cancel keyboard or mouse key binding
        if(joybmenu_back >= 0 && (joybuttons & (1 << joybmenu_back)))
        {
            S_StartSound(NULL, sfx_swtchn);
            if(merchantOn)
                FE_MerchantSetState(S_MRNO_00);
            frontend_state = FE_STATE_MAINMENU;
        }
        return;
    }
    else if(frontend_state != FE_STATE_MAINMENU)
        return; // no joy input handled here except in main menu state

    for(i = 0; i < NUM_VIRTUAL_BUTTONS + 16; i++)
    {
        if(!(joybuttons & (1 << i)))
            continue;

        i_seejoysticks = true;
        i_seemouses = false;

        if(i == joybmenu_back)
        {
            FE_PopMenu(false);
        }
        else if(i == joybmenu_up)
        {
            FE_HandleMenuUp();
        }
        else if(i == joybmenu_down)
        {
            FE_HandleMenuDown();
        }
        else if(i == joybmenu_left)
        {
            FE_HandleMenuLeft(mi);
        }
        else if(i == joybmenu_right)
        {
            FE_HandleMenuRight(mi);
        }
        else if(i == joybmenu_forward)
        {
            FE_HandleMenuForward(mi);
        }
    }
}

//
// Test if mouse pointer is inside menu item rect
//
static boolean FE_MouseInRect(Uint32 mx, Uint32 my, femenuitem_t *item)
{
    if(!(item->w | item->h)) // zero size?
        return false;

    return (mx >= item->x && mx <= item->x + item->w &&
            my >= item->y && my <= item->y + item->h);
}

//
// Test if mouse pointer is inside value rect
//
static boolean FE_MouseInValueRect(Uint32 mx, Uint32 my, femenuitem_t *item)
{
    if(!(item->valw | item->valh))
        return false;

    return (mx >= item->valx && mx <= item->valx + item->valw &&
            my >= item->valy && my <= item->valy + item->valh);
}

//
// Transform mouse coordinates (window-relative) into HUD coordinates (bound to
// 320x200 vanilla screen space). This mapping varies depending on how the program
// has setup the framebuffer.
//
static void FE_TransformCoordinates(Uint16 mx, Uint16 my, Uint32 *sx, Uint32 *sy)
{
	int w;
	int h;
	SDL_GetWindowSize(windowscreen, &w, &h);	
    
    fixed_t aspectRatio = w * FRACUNIT / h;

    if(aspectRatio == 4 * FRACUNIT / 3) // nominal
    {
        *sx = mx * SCREENWIDTH  / w;
        *sy = my * SCREENHEIGHT / h;
    }
    else if(aspectRatio > 4 * FRACUNIT / 3) // widescreen
    {
        // calculate centered 4:3 subrect
        int sw = h * 4 / 3;
        int hw = (w - sw) / 2;

        *sx = (mx - hw) * SCREENWIDTH / sw;
        *sy = my * SCREENHEIGHT / h;
    }
    else // narrow
    {
        int sh = w * 3 / 4;
        int hh = (h - sh) / 2;

        *sx = mx * SCREENWIDTH / w;
        *sy = (my - hh) * SCREENHEIGHT / sh;
    }
}

//
// Find the menu item, if any, that the mouse is over. Returns NULL
// if the mouse is not over a menu item.
//
static femenuitem_t *FE_FindMouseMenuItem(Uint16 mx, Uint16 my)
{
    int i;
    femenuitem_t *item, *ret = NULL;
    Uint32 smx, smy;

    // rescale coordinates into 320x200 HUD space
    FE_TransformCoordinates(mx, my, &smx, &smy);

    for(i = 0; i < currentFEMenu->numitems; i++)
    {
        item = &(currentFEMenu->items[i]);

        if(!FE_IsSelectable(item))
            continue;

        // test if in main rect
        if(FE_MouseInRect(smx, smy, item))
        {
            ret = item;
            break;
        }

        // test if in value rect
        if(FE_MouseInValueRect(smx, smy, item))
        {
            ret = item;
            break;
        }
    }

    return ret;
}

#define FE_MERCHANT_XOFFS  11
#define FE_MERCHANT_YOFFS  55
#define FE_MERCHANT_WIDTH  25
#define FE_MERCHANT_HEIGHT 59

//
// Test if mouse is on merchant
//
static boolean FE_MouseOnMerchant(Uint16 mx, Uint16 my)
{
    Uint32 smx, smy;
    Uint32 merchRect[4];

    if(!merchantOn)
        return false;

    // rescale coordinates into 320x200 HUD space
    FE_TransformCoordinates(mx, my, &smx, &smy);

    merchRect[0] = FE_MERCHANT_X - FE_MERCHANT_XOFFS; // left
    merchRect[1] = merchRect[0] + FE_MERCHANT_WIDTH;  // right
    merchRect[2] = FE_MERCHANT_Y - FE_MERCHANT_YOFFS; // top
    merchRect[3] = merchRect[2] + FE_MERCHANT_HEIGHT; // bottom

    return (smx >= merchRect[0] && smx <= merchRect[1] &&
            smy >= merchRect[2] && smy <= merchRect[3]);
}

//
// Test mouse position relative to slider
//
static void FE_MouseOnSlider(femenuitem_t *item, Uint16 mx, Uint16 my)
{
    Uint32 smx, smy;
    Uint32 gem_x;
    
    // rescale coordinates into 320x200 HUD space
    FE_TransformCoordinates(mx, my, &smx, &smy);

    // get slider position
    gem_x = (Uint32)(item->valx + FE_SliderPos(FE_CalcPct(item)));

    // must be on slider portion for relative behavior
    if(FE_MouseInValueRect(smx, smy, item))
    {
        if(smx <= gem_x)
            FE_DecrementVariable(item->verb);
        else
            FE_IncrementVariable(item->verb);
    }
    else
        FE_IncrementVariable(item->verb);
}

//
// Left mouse button handling
//
static void FE_HandleLeftMouseButtonDown(SDL_Event *ev)
{
    femenuitem_t *theItem;

    switch(frontend_state)
    {
    case FE_STATE_MAINMENU:
        if((theItem = FE_FindMouseMenuItem(ev->button.x, ev->button.y)))
        {
            switch(theItem->type)
            {
            case FE_MITEM_CMD:
            case FE_MITEM_LOBBY:
                FE_ExecCmd(theItem->verb);
                break;
            case FE_MITEM_TOGGLE:
                FE_ToggleVar(theItem);
                break;
            case FE_MITEM_KEYBIND:
                FE_StartKB(theItem);
                break;
            case FE_MITEM_MBIND:
                FE_StartMB(theItem);
                break;
            case FE_MITEM_ABIND:
                FE_JoyAxisBindStart(theItem);
                break;
            case FE_MITEM_JBIND:
                FE_JoyBindStart(theItem);
                break;
            case FE_MITEM_SLIDER:
                FE_MouseOnSlider(theItem, ev->button.x, ev->button.y);
                break;
            case FE_MITEM_VIDMODE:
                FE_NextVideoMode();
                break;
            case FE_MITEM_MPREADY:
                FE_ToggleLobbyReady();
                break;
            case FE_MITEM_MPTEAM:
                FE_IncrementTeam();
                break;
            case FE_MITEM_VALUES:
                FE_IncrementValue(theItem->verb);
                break;
            case FE_MITEM_MUSIC:
                FE_MusicTestPlaySelection();
                break;
            default:
                break;
            }
        }    
        break;
    case FE_STATE_KEYINPUT:
        frontend_state = FE_STATE_MAINMENU;
        S_StartSound(NULL, sfx_mtalht);
        return;
    default:
        break;
    }

    // tickling the merchant?
    if(FE_MouseOnMerchant(ev->button.x, ev->button.y))
    {
        S_StartSound(NULL, sfx_pespna + abs(rand()) % 4);
        FE_MerchantSetState(S_MRPN_00); // 'ey, wassamatta wichoo?!
    }
}

//
// Handle right mouse button clicks
//
static void FE_HandleRightMouseButtonDown(SDL_Event *ev)
{
    switch(frontend_state)
    {
    case FE_STATE_MAINMENU:
        FE_PopMenu(false);
        break;
    case FE_STATE_KEYINPUT:
        frontend_state = FE_STATE_MAINMENU;
        S_StartSound(NULL, sfx_mtalht);
        break;
    default:
        break;
    }
}

//
// Mouse motion handling
//
static void FE_HandleMouseMotion(SDL_Event *ev)
{
    femenuitem_t *theItem;

    if(frontend_state != FE_STATE_MAINMENU)
        return;

    // main menu state
    if((theItem = FE_FindMouseMenuItem(ev->motion.x, ev->motion.y)))
    {
        int itemNum = theItem - currentFEMenu->items;

        if(FE_IsSelectable(theItem) && itemNum != currentFEMenu->itemon)
        {
            currentFEMenu->itemon = itemNum;
            S_StartSound(NULL, sfx_pstop);
            if(merchantOn)
                FE_MerchantSetState(S_MRGT_00);
        }
    }
}

static int fe_joywait;
static const int i_deadzone = (32767 / 2);

//
// Gamepad axis handling
//
static void FE_HandleJoyAxes(void)
{
    femenuitem_t *mi = &(currentFEMenu->items[currentFEMenu->itemon]);
    int x_axis, y_axis, s_axis, l_axis;

    if(frontend_state != FE_STATE_MAINMENU)
        return; // only during main menu state

    I_JoystickGetAxes(&x_axis, &y_axis, &s_axis, &l_axis);

    if(fe_joywait < frontend_tic)
    {
        if(y_axis < -i_deadzone || l_axis < -i_deadzone)
        {
            i_seejoysticks = true;
            i_seemouses = false;
            FE_HandleMenuUp();
            fe_joywait = frontend_tic + 10;
        }
        else if(y_axis > i_deadzone || l_axis > i_deadzone)
        {
            i_seejoysticks = true;
            i_seemouses = false;
            FE_HandleMenuDown();
            fe_joywait = frontend_tic + 10;
        }

        if(x_axis < -i_deadzone || s_axis < -i_deadzone)
        {
            i_seejoysticks = true;
            i_seemouses = false;
            FE_HandleMenuLeft(mi);
            fe_joywait = frontend_tic + 10;
        }
        else if(x_axis > i_deadzone || s_axis > i_deadzone)
        {
            i_seejoysticks = true;
            i_seemouses = false;
            FE_HandleMenuRight(mi);
            fe_joywait = frontend_tic + 10;
        }
    }
}

//
// Handle events for the frontend.
//
static void FE_Responder(void)
{
    SDL_Event ev;
    int joybuttons;

    FE_UpdateFocus();

    if(FE_InModalNetState())
        return;

    // [SVE] svillarreal
    if(!gAppServices->OverlayActive())
    {
        // run joybind responders?
        if(frontend_state == FE_STATE_JBINPUT)
        {
            FE_JoyBindResponder();
            return;
        }
        else if(frontend_state == FE_STATE_JAINPUT)
        {
            FE_JoyAxisResponder();
            return;
        }

        // poll gamepad
        joybuttons = I_JoystickGetButtons();
        if(joybuttons != 0 && joybuttons != -1)
        {
            FE_HandleJoyButtons(joybuttons);
        }

        FE_HandleJoyAxes();
    }

    while(SDL_PollEvent(&ev))
    {
        if(gAppServices->OverlayEventFilter(ev.type))
            continue;

        switch(ev.type)
        {
        case SDL_KEYDOWN:
            FE_HandleKey(&ev);
            break;
#ifndef SVE_PLAT_SWITCH
        case SDL_MOUSEBUTTONDOWN:
            i_seemouses = true;
            i_seejoysticks = false;
            if(frontend_state == FE_STATE_MBINPUT)
            {
                // rebind mouse button
                FE_SetMouseButton(fe_kbitem, &ev);
                break;
            }
            switch(ev.button.button)
            {
            case SDL_BUTTON_LEFT:
                FE_HandleLeftMouseButtonDown(&ev);
                break;
            case SDL_BUTTON_RIGHT:
                FE_HandleRightMouseButtonDown(&ev);
                break;
            default:
                break;
            }
            break;
        case SDL_MOUSEMOTION:
            i_seemouses = true;
            i_seejoysticks = false;
            FE_HandleMouseMotion(&ev);
            break;
        case SDL_MOUSEWHEEL:
            if(ev.wheel.y > 0 || ev.wheel.y < 0)
            {
                i_seemouses = true;
                i_seejoysticks = false;
                if(frontend_state == FE_STATE_MBINPUT)
                {
                    // rebind mouse button
                    FE_SetMouseButton(fe_kbitem, &ev);
                }
            }
            break;
#endif
        case SDL_APP_TERMINATING:
        case SDL_QUIT:
            I_Quit();
            break;
            /*
        case SDL_ACTIVEEVENT:
            FE_UpdateFocus();
            break;*/
        default:
            break;
        }
    }
}

//=============================================================================
//
// Main Subprogram
//

//
// Startup tasks for the frontend.
//
static void FE_Init(void)
{
    int p;

    srand(time(NULL));

    frontend_done       = false;
    frontend_state      = FE_STATE_MAINMENU;
    frontend_sgcount    = 20;
    frontend_wipe       = true;
    frontend_waitframe  = false;
    currentFEMenu       = &mainMenu;

    S_ChangeMusic(mus_panthr, 1);
    FE_InitMerchant();
    FE_InitBackgrounds();
    FE_LoadSlider();
    FE_BindMusicTestVar();

    // check for +connect_lobby from the Steam client
    if((p = M_CheckParmWithArgs("+connect_lobby", 1)) > 0)
    {
        frontend_autolobby = true;
        frontend_lobbyid   = myargv[p + 1];
    }
    else
        FE_EnableLobbyListener(); // can accept invites

    I_SetShowVisualCursor(true);
    frontend_running = true;
}

//
// End the frontend.
//
static void FE_Shutdown(void)
{
    frontend_running = false;

    FE_DisableLobbyListener(); // cannot join any games now

    S_StopMusic();     // stop music
    I_ResetBaseTime(); // don't screw with in-game timing

    I_SetShowVisualCursor(false);

    FE_DestroyBackgrounds();
    FE_FreeSlider();
}

static int frontend_ticcount;

//
// Update timing information
//
static void FE_UpdateTics(void)
{
    frontend_ticcount = I_GetTimeMS();
}

//
// haleyjd 20140904: [SVE]
// Choco's sound engine won't tolerate the main loop running at full blast 
// and turns into chop city. We must give up a non-trivial amount of CPU
// time on a consistent basis.
//
static boolean FE_CapFrameRate(void)
{
    float frameMillis = 1000.0f / frontend_fpslimit;
    int   curTics = I_GetTimeMS();
    float elapsed = (float)(curTics - frontend_ticcount);
    
    if(elapsed < frameMillis)
    {
        if(frameMillis - elapsed > 3.0f)
            I_Sleep(2);
        return true;
    }

    return false;
}

//
// Begin the frontend miniloop preceding main game execution.
//
void FE_StartFrontend(void)
{
    // initialize
    FE_Init();

    while(!frontend_done)
    {
        FE_UpdateTics();

        // get input
        FE_Responder();

        // run ticks
        FE_Ticker();

        // render
        FE_Drawer();

        // update sounds
        S_UpdateSounds(NULL);

        // framerate
        while(FE_CapFrameRate());
    }

    // shutdown
    FE_Shutdown();
}

//
// Test if in frontend
//
boolean FE_InFrontend(void)
{
    return frontend_running;
}

//=============================================================================
//
// In-Game Access
//

//
// Call to start display of the frontend options menu from an ongoing game.
//
void FE_StartInGameOptionsMenu(void)
{
    srand(time(NULL));

    // svillarreal - force-reset the mouse button state
    I_ClearMouseButtonState();

    frontend_done     = false;
    frontend_state    = FE_STATE_MAINMENU;
    frontend_sgcount  = 20;
    frontend_wipe     = true;
    frontend_modalmsg = NULL;
    feModalLoopFunc   = NULL;
    currentFEMenu     = &optionsMenuMain;

    // cannot pop out further than options menu
    optionsMenuMain.prevMenu = NULL;

    FE_InitMerchant();
    FE_InitBackgrounds();
    FE_LoadSlider();
    FE_BindMusicTestVar();

    // ensure lobby listener is NOT active
    frontend_autolobby = false;
    FE_DisableLobbyListener();

    frontend_running = true;
    frontend_ingame  = true;

}

//
// Call once per tic when running the in-game options menu
//
void FE_InGameOptionsTicker(void)
{
    if(frontend_ingame)
        FE_Ticker();
}

//
// Call once per frame when running the in-game options menu
//
void FE_InGameOptionsDrawer(void)
{
    if(frontend_ingame)
        FE_Drawer();
}

//
// Call once per frame when running the in-game options menu.
// Returns true if the frontend wants to exit.
//
boolean FE_InGameOptionsResponder(void)
{
    if(frontend_ingame)
        FE_Responder();

    return frontend_done;
}

//
// Call to end the in-game options menu
//
void FE_EndInGameOptionsMenu(void)
{
    if(frontend_ingame)
    {
        frontend_running = false;
        frontend_ingame  = false;
        FE_DestroyBackgrounds();
        FE_FreeSlider();
        wipegamestate = -1; // have the game wipe back in
    }
}

//
// Returns true if running the in-game options menu
//
boolean FE_InOptionsMenu(void)
{
    return (frontend_running && frontend_ingame);
}

// EOF

