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
#include "z_zone.h"

#include "doomkeys.h"
#include "doomtype.h"
#include "doomstat.h"
#include "hu_lib.h"
#include "i_swap.h"
#include "i_video.h"
#include "m_config.h"
#include "m_menu.h"
#include "m_misc.h"
#include "sounds.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"

#include "fe_characters.h"
#include "fe_commands.h"
#include "fe_frontend.h"
#include "fe_gamepad.h"
#include "fe_graphics.h"
#include "fe_menuengine.h"
#include "fe_menus.h"
#include "fe_mouse.h"
#include "fe_multiplayer.h"

//=============================================================================
//
// Frontend menus
//

//
// Draw menu item description
//
static int FE_DrawDescription(femenuitem_t *item, int x, int y)
{
    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->w = M_StringWidth(item->description);
        item->h = 12;
        M_WriteText(x, y, item->description);
        break;
    case FE_FONT_BIG:
        item->w = V_BigFontStringWidth(item->description);
        item->h = 20;
        V_WriteBigText(item->description, x, y);
        break;
    default:
        item->w = item->h = 0;
        break;
    }

    return item->h;
}

//
// Draw toggle value
//
static void FE_DrawToggleValue(femenuitem_t *item, int x, int y)
{
    const char *text;
    boolean val    = M_GetIntVariableDefault(item->verb);
    boolean invert = ((item->data & FE_TOGGLE_INVERT) == FE_TOGGLE_INVERT);

    text = (val ^ invert) ? "On" : "Off";
    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(text);
        item->valh = 12;
        M_WriteText(x, y, text);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(text);
        item->valh = 20;
        V_WriteBigText(text, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

typedef struct keynamerepl_s
{
    int key;
    const char *altName;
} keynamerepl_t;

// some config keynames don't work in the menus...
static keynamerepl_t keyNameRepls[] =
{
    { KEY_RIGHTARROW, "right"   },
    { KEY_LEFTARROW,  "left"    },
    { KEY_UPARROW,    "up"      },
    { KEY_DOWNARROW,  "down"    },
    { KEY_BACKSPACE,  "backsp"  },
    { KEY_CAPSLOCK,   "caplock" },
    { KEYP_0,         "kp 0"    },
    { KEYP_1,         "kp 1"    },
    { KEYP_2,         "kp 2"    },
    { KEYP_3,         "kp 3"    },
    { KEYP_4,         "kp 4"    },
    { KEYP_5,         "kp 5"    },
    { KEYP_6,         "kp 6"    },
    { KEYP_7,         "kp 7"    },
    { KEYP_8,         "kp 8"    },
    { KEYP_9,         "kp 9"    },
    { KEYP_DIVIDE,    "kp /"    },
    { KEYP_PLUS,      "kp +"    },
    { KEYP_MINUS,     "kp -"    },
    { KEYP_MULTIPLY,  "kp *"    },
    { KEYP_PERIOD,    "kp ."    },
    { KEYP_EQUALS,    "kp ="    },
    { KEYP_ENTER,     "kpenter" },
};

const char *FE_GetKeyName(const char *varname)
{
    int i;
    int val = M_GetIntVariable(varname);
    const char *text = GetNameForKey(val);

    if(!val || !text)
        text = "None";

    for(i = 0; i < arrlen(keyNameRepls); i++)
    {
        if(val == keyNameRepls[i].key)
        {
            text = keyNameRepls[i].altName;
            break;
        }
    }

    return text;
}

//
// Draw keybinding value
//
static void FE_DrawKBValue(femenuitem_t *item, int x, int y)
{
    const char *text = FE_GetKeyName(item->verb);

    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(text);
        item->valh = 12;
        M_WriteText(x, y, text);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(text);
        item->valh = 20;
        V_WriteBigText(text, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

//
// Draw mouse binding value
//
static void FE_DrawMBValue(femenuitem_t *item, int x, int y)
{
    int feMVarNum    = item->data;
    int mbNum        = *(feMVars[feMVarNum]);
    const char *name = FE_NameForCfgMouseButton(mbNum);
    
    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(name);
        item->valh = 12;
        M_WriteText(x, y, name);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(name);
        item->valh = 20;
        V_WriteBigText(name, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

//
// Draw gamepad button binding value
//
static void FE_DrawJBValue(femenuitem_t *item, int x, int y)
{
    int button = M_GetIntVariable(item->verb);
    const char *name = FE_ButtonNameForNum(button);

    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(name);
        item->valh = 12;
        M_WriteText(x, y, name);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(name);
        item->valh = 20;
        V_WriteBigText(name, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

//
// Draw gamepad axis binding value
//
static void FE_DrawJAValue(femenuitem_t *item, int x, int y)
{
    int axis = M_GetIntVariable(item->verb);
    const char *name = FE_AxisNameForNum(axis);

    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(name);
        item->valh = 12;
        M_WriteText(x, y, name);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(name);
        item->valh = 20;
        V_WriteBigText(name, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

static patch_t *feslidergfx[FE_SLIDER_NUMGFX];
static short    fesliderwidths[FE_SLIDER_NUMGFX];
static short    fesliderheights[FE_SLIDER_NUMGFX];

//
// Get horizontal offset of gem patch.
//
int FE_SliderPos(int pct)
{
    int slider_width = (fesliderwidths[FE_SLIDER_MIDDLE] - 1) * 9;
    int wl = fesliderwidths[FE_SLIDER_LEFT];
    int ws = fesliderwidths[FE_SLIDER_GEM];
    return wl + (pct * (slider_width - ws)) / 100;
}

//
// Calculate percentage along the slider of a slider's value
//
int FE_CalcPct(femenuitem_t *item)
{
    fevar_t *var = FE_VariableForName(item->verb);
    if(!var)
        return 0;

    switch(var->type)
    {
    case FE_VAR_INT:
        {
            int val = M_GetIntVariable(var->name);
            return (val - var->min) * 100 / (var->max - var->min);
        }
    case FE_VAR_INT_PO2:
        {
            unsigned int v = (unsigned int)(M_GetIntVariable(var->name));
            unsigned int r = 0;

            // calculate rational log 2 of integer
            while(v >>= 1)
                ++r;

            return (r - var->min) * 100 / (var->max - var->min);
        }
    case FE_VAR_FLOAT:
        {
            float val = M_GetFloatVariable(var->name);
            return (int)((val - var->fmin) * 100 / (var->fmax - var->fmin));
        }
    default:
        return 0;
    }
}

//
// Load the slider graphics.
//
void FE_LoadSlider(void)
{
    int i;

    feslidergfx[FE_SLIDER_LEFT  ] = W_CacheLumpName("M_SLIDEL", PU_STATIC);
    feslidergfx[FE_SLIDER_RIGHT ] = W_CacheLumpName("M_SLIDER", PU_STATIC);
    feslidergfx[FE_SLIDER_MIDDLE] = W_CacheLumpName("M_SLIDEM", PU_STATIC);
    feslidergfx[FE_SLIDER_GEM   ] = W_CacheLumpName("M_SLIDEO", PU_STATIC);

    for(i = 0; i < FE_SLIDER_NUMGFX; i++)
    {
        fesliderwidths[i]  = SHORT(feslidergfx[i]->width);
        fesliderheights[i] = SHORT(feslidergfx[i]->height);
    }
}

//
// Free the slider graphics.
//
void FE_FreeSlider(void)
{
    int i;

    for(i = 0; i < FE_SLIDER_NUMGFX; i++)
        Z_ChangeTag(feslidergfx[i], PU_CACHE);
}

//
// Draw small slider.
//
static void FE_DrawSlider(femenuitem_t *item, int x, int y, int pct)
{
    int i;
    int draw_x = x;
    int slider_width = 0;
    short wl, wm, wr, ws, hs;

    wl = fesliderwidths[FE_SLIDER_LEFT];
    wm = fesliderwidths[FE_SLIDER_MIDDLE];
    wr = fesliderwidths[FE_SLIDER_RIGHT];
    ws = fesliderwidths[FE_SLIDER_GEM];
    hs = fesliderheights[FE_SLIDER_GEM];

    item->valx = draw_x;
    item->valy = y;
    item->valh = hs;

    // left end
    V_DrawPatch(draw_x, y, feslidergfx[FE_SLIDER_LEFT]);
    draw_x += wl;

    // middle
    for(i = 0; i < 9; i++)
    {
        V_DrawPatch(draw_x, y, feslidergfx[FE_SLIDER_MIDDLE]);
        draw_x += wm - 1;
    }

    // right end
    V_DrawPatch(draw_x, y, feslidergfx[FE_SLIDER_RIGHT]);

    item->valw = (draw_x + wr) - item->valx;

    // gem position
    draw_x = FE_SliderPos(pct);

    // gem
    V_DrawPatch(x + draw_x, y, feslidergfx[FE_SLIDER_GEM]);
}

//
// Draw video mode string
//
static void FE_DrawVideoMode(femenuitem_t *item, int x, int y)
{
    const char *text = FE_CurrentVideoMode();

    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(text);
        item->valh = 12;
        M_WriteText(x, y, text);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(text);
        item->valh = 20;
        V_WriteBigText(text, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

//
// Draw a value string
//
static void FE_DrawValue(femenuitem_t *item, int x, int y)
{
    char *text = M_Strdup(FE_NameForValue(item->verb));

    M_DialogDimMsg(x, y, text, true);

    item->valx = x;
    item->valy = y;

    item->valw = HUlib_yellowTextWidth(text);
    item->valh = 8;
    HUlib_drawYellowText(x, y, text, true);

    free(text);
}

//
// Draw lobby ready value
//
static void FE_DrawLobbyReadyValue(femenuitem_t *item, int x, int y)
{
    const char *text;
    boolean val = FE_GetLobbyReady();

    text = val ? "Ready" : "Not Ready";
    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(text);
        item->valh = 12;
        M_WriteText(x, y, text);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(text);
        item->valh = 20;
        V_WriteBigText(text, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}

//
// Draw lobby team value
//
static void FE_DrawLobbyTeamValue(femenuitem_t *item, int x, int y)
{
    const char *text = FE_GetLobbyTeam();
    item->valx = x;
    item->valy = y;

    switch(item->font)
    {
    case FE_FONT_SMALL:
        item->valw = M_StringWidth(text);
        item->valh = 12;
        M_WriteText(x, y, text);
        break;
    case FE_FONT_BIG:
        item->valw = V_BigFontStringWidth(text);
        item->valh = 20;
        V_WriteBigText(text, x, y);
        break;
    default:
        item->valw = item->valh = 0;
        break;
    }
}


//
// Draw a menu
//
void FE_MenuDraw(femenu_t *menu)
{
    int i;
    int x = menu->x;
    int y = menu->y;
    int width = 0;

    // draw background
    FE_DrawBackground(menu->background);

    // specific menu drawer?
    if(menu->Drawer)
        menu->Drawer();

    // draw title
    FE_WriteBigTextCentered(menu->titley, menu->title);

    for(i = 0; i < menu->numitems; i++)
    {
        femenuitem_t *item = &menu->items[i];

        if(i == menu->itemon)
        {
            // draw specified cursor if currently selected item
            switch(menu->cursortype)
            {
            case FE_CURSOR_SIGIL:
                FE_DrawSigilCursor(x, y);
                break;
            case FE_CURSOR_LASER:
                FE_DrawLaserCursor(x, y);
                break;
            default:
                break;
            }

            if(merchantOn)
            {
                FE_DrawBox(0, 160, SCREENWIDTH, 40);
                FE_DrawMerchant(FE_MERCHANT_X, FE_MERCHANT_Y);

                // draw help string for selected item
                if(item->verb)
                {
                    if(!item->help)
                        item->help = FE_GetFormattedHelpStr(item->verb, 20, 0);
                    if(item->help)
                        HUlib_drawYellowText(4, 164, item->help, true);
                }
            }
        }

        switch(item->type)
        {
        case FE_MITEM_CMD:
        case FE_MITEM_TOGGLE:
        case FE_MITEM_KEYBIND:
        case FE_MITEM_MBIND:
        case FE_MITEM_ABIND:
        case FE_MITEM_JBIND:
        case FE_MITEM_SLIDER:
        case FE_MITEM_VIDMODE:
        case FE_MITEM_MPREADY:
        case FE_MITEM_MPTEAM:
        case FE_MITEM_VALUES:
        case FE_MITEM_MUSIC:
            item->x = x;
            item->y = y;
            // draw description
            y += FE_DrawDescription(item, x, y);
            // remember widest width
            if(item->w > width)
                width = item->w;
            break;
        case FE_MITEM_LOBBY:
            FE_DrawLobbyItem(item, x, y);
            y += 12;
            break;
        case FE_MITEM_GAP:
            // Gap - no drawing; just step down by the item gap size
            y += item->data;
            break;
        default:
            break;
        }
    }

    // draw values for valued items
    for(i = 0; i < menu->numitems; i++)
    {
        femenuitem_t *item = &menu->items[i];

        switch(item->type)
        {
        case FE_MITEM_TOGGLE:
            FE_DrawToggleValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_KEYBIND:
            FE_DrawKBValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_MBIND:
            FE_DrawMBValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_ABIND:
            FE_DrawJAValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_JBIND:
            FE_DrawJBValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_SLIDER:
            FE_DrawSlider(item, item->x + width + 12, item->y, FE_CalcPct(item));
            break;
        case FE_MITEM_VIDMODE:
            FE_DrawVideoMode(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_MPREADY:
            FE_DrawLobbyReadyValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_MPTEAM:
            FE_DrawLobbyTeamValue(item, item->x + width + 12, item->y);
            break;
        case FE_MITEM_VALUES:
        case FE_MITEM_MUSIC:
            FE_DrawValue(item, item->x + width + 12, item->y);
            break;
        default:
            break;
        }
    }
}

// Current menu pointer
femenu_t *currentFEMenu;

//
// Menu change sound effects
//
void FE_MenuChangeSfx(void)
{
    S_StartSound(NULL, sfx_swston);
}

// 
// Push a menu onto the menu stack; it will remember the menu it came from
// by saving the value of currentFEMenu
//
void FE_PushMenu(femenu_t *menu)
{
    menu->prevMenu = currentFEMenu;
    currentFEMenu  = menu;
    frontend_wipe  = true; // do a transition so it doesn't just flop
    frontend_sgcount = 20;
    FE_MenuChangeSfx();

    // advance character, so it's not always the same one; depends on user input pattern
    curCharacter = FE_GetCharacter();

    // toggle merchant
    merchantOn = currentFEMenu->hasMerchant;
}

//
// Pop back to the previous menu
//
void FE_PopMenu(boolean forced)
{
    // svillarreal - need to wait a frame before going into wipe again
    if(!forced && frontend_waitframe)
        return;

    if(!currentFEMenu->prevMenu)
    {
        if(frontend_ingame)
            FE_ExecCmd("go"); // done with in-game options menu

        return; // eh?
    }

    // check for forced exit requirement
    if(currentFEMenu->exitForcedOnly && !forced)
        return;

    if(FE_InAudioMenu()) // audio menu hack
        FE_MusicTestRestoreCurrent();
    
    currentFEMenu = currentFEMenu->prevMenu;
    frontend_wipe = true;
    frontend_sgcount = 20;
    FE_MenuChangeSfx();

    // toggle merchant
    merchantOn = currentFEMenu->hasMerchant;
}

//
// Toggle a boolean variable
//
void FE_ToggleVar(femenuitem_t *item)
{
    int     val      = M_GetIntVariableDefault(item->verb);
    boolean invert   = ((item->data & FE_TOGGLE_INVERT) == FE_TOGGLE_INVERT);
    int     varflags = M_CFG_SETALL;

    if(netgame && item->data & FE_TOGGLE_NOTNET)
    {
        S_StartSound(NULL, sfx_noway); // nope!
        return;
    }

    if(item->data & FE_TOGGLE_DEFAULT) // sets default only?
        varflags &= ~M_CFG_SETCURRENT;

    val ^= 1;
    M_SetVariableByFlags(item->verb, val ? "1" : "0", varflags);

    S_StartSound(NULL, sfx_swtchn);
    if(merchantOn)
        FE_MerchantSetState((val ^ invert) ? S_MRYS_00 : S_MRNO_00);
}

femenuitem_t *fe_kbitem;

//
// Start keybinding change
//
void FE_StartKB(femenuitem_t *item)
{
    // remember item, and start key input state
    fe_kbitem = item;
    frontend_state = FE_STATE_KEYINPUT;
    S_StartSound(NULL, sfx_mtalht);
}

//
// Start mouse button binding change
//
void FE_StartMB(femenuitem_t *item)
{
    // remember item, and start mb input state
    fe_kbitem = item;
    frontend_state = FE_STATE_MBINPUT;
    S_StartSound(NULL, sfx_mtalht);
}

//
// Change a keybinding option
//
boolean FE_SetKeybinding(femenuitem_t *item, int key)
{
    const char *keyname;
    boolean res = false;
    boolean cleared = false;

    if(!key)
        return false;

    if(key == KEY_BACKSPACE) // clear binding
    {
        res = M_SetVariable(item->verb, "");
        cleared = true;
    }
    else
    {
        // try to find name for key
        keyname = GetNameForKey(key);
        if(!keyname)
            return false;

        res = M_SetVariable(item->verb, keyname);
    }

    if(res)
    {
        S_StartSound(NULL, sfx_swtchn);
        if(merchantOn)
            FE_MerchantSetState(!cleared ? S_MRYS_00 : S_MRNO_00);
    }

    return res;
}

//
// Change a mouse button binding option
//
void FE_SetMouseButton(femenuitem_t *item, SDL_Event *ev)
{
    char buf[33];
    int feMVarNum = item->data;
    int cfgMBNum  = FE_CfgNumForSDLMouseButton(ev);

    // set this variable
    M_SetVariable(feMVarNames[feMVarNum], M_Itoa(cfgMBNum, buf, 10));

    // clear any other variables of the same mouse button
    FE_ClearMVarsOfButton(cfgMBNum, feMVarNum);

    S_StartSound(NULL, sfx_swtchn);
    if(merchantOn)
        FE_MerchantSetState(cfgMBNum > 0 ? S_MRYS_00: S_MRNO_00);
    frontend_state = FE_STATE_MAINMENU;
}

//
// Clear mouse binding
//
void FE_ClearMouseButton(femenuitem_t *item)
{
    int feMVarNum = item->data;

    M_SetVariable(feMVarNames[feMVarNum], "-1");

    S_StartSound(NULL, sfx_mtalht);
    if(merchantOn)
        FE_MerchantSetState(S_MRNO_00);
    frontend_state = FE_STATE_MAINMENU;
}

//
// Some menu items aren't selectable
//
boolean FE_IsSelectable(femenuitem_t *item)
{
    return !(item->type == FE_MITEM_GAP || 
             item->type == FE_MITEM_END);
}

// EOF

