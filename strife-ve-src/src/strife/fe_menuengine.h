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

#ifndef FE_MENUENGINE_H_
#define FE_MENUENGINE_H_

#include "doomtype.h"
#include "SDL.h"

// menu item types
enum
{
    FE_MITEM_CMD,     // run a command
    FE_MITEM_TOGGLE,  // boolean toggle option
    FE_MITEM_KEYBIND, // key binding option
    FE_MITEM_MBIND,   // mouse binding option
    FE_MITEM_ABIND,   // axis binding option
    FE_MITEM_JBIND,   // joy button binding option
    FE_MITEM_SLIDER,  // value slider
    FE_MITEM_VIDMODE, // video mode selector
    FE_MITEM_MPREADY, // multiplayer ready state
    FE_MITEM_MPTEAM,  // multiplayer team selector
    FE_MITEM_VALUES,  // select from a set of values
    FE_MITEM_MUSIC,   // pick a song
    FE_MITEM_LOBBY,   // a lobby to join
    FE_MITEM_GAP,     // just a gap
    FE_MITEM_END      // end of items
};

// flags for femenuitem_t::data when type == FE_MITEM_TOGGLE
enum
{
    FE_TOGGLE_INVERT  = 0x01, // value is inverted for display
    FE_TOGGLE_DEFAULT = 0x02, // manipulates the default location only
    FE_TOGGLE_NOTNET  = 0x04  // not allowed during netgames
};

// menu fonts
enum
{
    FE_FONT_SMALL, // use hu_font
    FE_FONT_BIG    // use big font
};

// menu cursors
enum
{
    FE_CURSOR_SIGIL, // use sigil cursor
    FE_CURSOR_LASER, // use laser cursor
    FE_CURSOR_NONE   // no cursor
};

// menu item structure
typedef struct femenuitem_s
{
    int         type;
    const char *description;
    const char *verb;
    int         font;
    int         data; // size, if a gap item; invert if toggle

    // runtime fields
    char       *help;    // cached help string, for vars
    int         x;       // drawn x position
    int         y;       // drawn y position
    int         w;       // drawn width
    int         h;       // drawn height
    int         valx;    // drawn value x pos
    int         valy;    // drawn value y pos
    int         valw;    // drawn value width
    int         valh;    // drawn value height
} femenuitem_t;

// menu structure
typedef struct femenu_s
{
    femenuitem_t *items;
    int           numitems;
    int           x;
    int           y;
    int           titley;
    const char   *title;
    int           background;
    void        (*Drawer)(void);
    int           cursortype;
    int           itemon;
    boolean       hasMerchant;
    boolean       exitForcedOnly;
	void        (*ExitCallback)(void);
    
    // runtime fields
    struct femenu_s *prevMenu; // menu we came to this one from
} femenu_t;

void FE_LoadSlider(void);
void FE_FreeSlider(void);
int FE_SliderPos(int pct);
int FE_CalcPct(femenuitem_t *item);

extern femenuitem_t *fe_kbitem;
extern femenu_t     *currentFEMenu;

void FE_MenuDraw(femenu_t *menu);
void FE_PushMenu(femenu_t *menu);
boolean FE_SetKeybinding(femenuitem_t *item, int key);
void FE_ClearMouseButton(femenuitem_t *item);
void FE_ToggleVar(femenuitem_t *item);
void FE_StartKB(femenuitem_t *item);
void FE_StartMB(femenuitem_t *item);
boolean FE_IsSelectable(femenuitem_t *item);
void FE_SetMouseButton(femenuitem_t *item, SDL_Event *ev);

void FE_PopMenu(boolean forced);

void FE_MenuChangeSfx(void);

const char *FE_GetKeyName(const char *varname);

void FE_NX_DrawToolTips(int type);

#endif

// EOF

