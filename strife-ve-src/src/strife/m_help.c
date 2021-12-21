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
//   [SVE]: Menu help screens
//
// AUTHOR:
//   James Haley
//    

#include <string.h>

#include "rb_local.h"
#include "rb_data.h"

#include "SDL.h"

#include "doomstat.h"
#include "z_zone.h"
#include "fe_gamepad.h"
#include "fe_menuengine.h"
#include "fe_mouse.h"
#include "i_swap.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_help.h"
#include "m_misc.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_patch.h"
#include "v_video.h"
#include "w_wad.h"

//=============================================================================
//
// Defines
//

// starting text coordinates, relative to a 320x200 screen
#define TEXT_START_X 226
#define TEXT_START_Y 49

// vertical step between lines
#define TEXT_LINE_STEP 9

// orange keyboard font
#define FONT_FMT_KB    "HFNT%.3d"

// blue mouse font
#define FONT_FMT_MOUSE "HFNM%.3d"

// white gamepad font
#define FONT_FMT_PAD   "HFNP%.3d"

// font array dimensions
#define M_FONTSTART '!'
#define M_FONTEND   '~'
#define M_FONTSIZE  (M_FONTEND - M_FONTSTART + 1)

// state countdown time
#define HELP_STATE_TIME (3*TICRATE)

//=============================================================================
//
// Data
//

enum
{
    HELP_PG_MOVEMENT,
    HELP_PG_INVENTORY,
    HELP_PG_WEAPONS,
    HELP_PG_MAP,
    HELP_PG_FUNCTIONS,
    HELP_PG_MENUS,

    HELP_NUMPAGES
};

// low-res backgrounds; used for 4:3 and 5:4
static char *bglow[HELP_NUMPAGES] =
{
    "HELP1",
    "HELP2",
    "HELP3",
    "HELP4",
    "HELP5",
    "HELP6"
};

// high-res backgrounds; used for widescreen
#if 0
static char *bghigh[HELP_NUMPAGES] =
{
    "HELP1W",
    "HELP2W",
    "HELP3W",
    "HELP4W",
    "HELP5W",
    "HELP6W"
};
#endif

//
// Variables for each page
//

typedef struct helpvar_s
{
    const char *keyboard; // keyboard binding variable name
    const char *mouse;    // mouse binding variable name if any   
    const char *gamepad;  // gamepad button binding variable name if any
} helpvar_t;

typedef struct helppage_s
{
    helpvar_t *helpvars;
    size_t     numhelpvars;
} helppage_t;

// Page 1 - Movement
static helpvar_t page1HelpVars[] =
{
    { "key_up",          "mouseb_forward",     NULL               }, // move forward
    { "key_down",        "mouseb_backward",    NULL               }, // move backward
    { "key_strafeleft",  "mouseb_strafeleft",  "joyb_strafeleft"  }, // strafe left
    { "key_straferight", "mouseb_straferight", "joyb_straferight" }, // strafe right
    { "key_left",        NULL,                 NULL               }, // turn left
    { "key_right",       NULL,                 NULL               }, // turn right
    { "key_fire",        "mouseb_fire",        "joyb_fire"        }, // attack
    { "key_use",         "mouseb_use",         "joyb_use"         }, // use/activate
    { "key_speed",       NULL,                 "joyb_speed"       }, // run
    { "key_jump",        "mouseb_jump",        "joyb_jump"        }, // jump
    { "key_lookUp",      NULL,                 NULL               }, // look up
    { "key_lookDown",    NULL,                 NULL               }, // look down
};

static helppage_t helppage1 =
{
    page1HelpVars,
    arrlen(page1HelpVars)
};

// Page 2 - Inventory
static helpvar_t page2HelpVars[] =
{
    { "key_invLeft",   NULL, "joyb_invleft"  }, // scroll left
    { "key_invRight",  NULL, "joyb_invright" }, // scroll right
    { "key_invHome",   NULL, NULL            }, // scroll to home
    { "key_invEnd",    NULL, NULL            }, // scroll to end
    { "key_invUse",    NULL, "joyb_invuse"   }, // use current item
    { "key_invDrop",   NULL, "joyb_invdrop"  }, // drop current item
    { "key_invquery",  NULL, NULL            }, // query current item
    { "key_useHealth", NULL, NULL            }, // use health item
    { "key_invPop",    NULL, "joyb_invpop"   }, // view status
    { "key_mission",   NULL, "joyb_mission"  }, // view mission
    { "key_invKey",    NULL, "joyb_invkey"   }, // view keys
};

static helppage_t helppage2 =
{
    page2HelpVars,
    arrlen(page2HelpVars)
};

// Page 3 - Weapons
static helpvar_t page3HelpVars[] =
{
    { "key_weapon1",    NULL,                NULL }, // punch dagger
    { "key_weapon2",    NULL,                NULL }, // crossbow
    { "key_weapon3",    NULL,                NULL }, // assault rifle
    { "key_weapon4",    NULL,                NULL }, // mini-missile launcher
    { "key_weapon5",    NULL,                NULL }, // grenade launcher
    { "key_weapon6",    NULL,                NULL }, // flamethrower
    { "key_weapon7",    NULL,                NULL }, // mauler
    { "key_weapon8",    NULL,                NULL }, // sigil of the one god
    { "key_nextweapon", "mouseb_nextweapon", "joyb_nextweapon" }, // next weapon
    { "key_prevweapon", "mouseb_prevweapon", "joyb_prevweapon" }, // previous weapon
};

static helppage_t helppage3 =
{
    page3HelpVars,
    arrlen(page3HelpVars)
};

// Page 4 - Map
static helpvar_t page4HelpVars[] =
{
    { "key_map_toggle",    NULL, "joybmap_toggle"    }, // toggle map view
    { "key_map_north",     NULL, "joybmap_north"     }, // scroll north
    { "key_map_south",     NULL, "joybmap_south"     }, // scroll south
    { "key_map_east",      NULL, "joybmap_east"      }, // scroll east
    { "key_map_west",      NULL, "joybmap_west"      }, // scroll west
    { "key_map_zoomin",    NULL, "joybmap_zoomin"    }, // zoom in
    { "key_map_zoomout",   NULL, "joybmap_zoomout"   }, // zoom out
    { "key_map_maxzoom",   NULL, NULL                }, // max zoom
    { "key_map_follow",    NULL, "joybmap_follow"    }, // follow mode
    { "key_map_mark",      NULL, "joybmap_mark"      }, // mark spot
    { "key_map_clearmark", NULL, "joybmap_clearmark" }, // clear last mark
};

static helppage_t helppage4 =
{
    page4HelpVars,
    arrlen(page4HelpVars)
};

// Page 5 - Functions
static helpvar_t page5HelpVars[] =
{
    { "key_menu_help",       NULL, NULL }, // help
    { "key_menu_save",       NULL, NULL }, // save game
    { "key_menu_load",       NULL, NULL }, // load game
    { "key_menu_volume",     NULL, NULL }, // sound volume
    { "key_menu_autohealth", NULL, NULL }, // toggle auto health
    { "key_menu_qsave",      NULL, NULL }, // quick save
    { "key_menu_endgame",    NULL, NULL }, // end game
    { "key_menu_messages",   NULL, NULL }, // toggle subtitles
    { "key_menu_qload",      NULL, NULL }, // quick load
    { "key_menu_quit",       NULL, NULL }, // quit
    { "key_menu_gamma",      NULL, NULL }, // adjust gamma
};

static helppage_t helppage5 =
{
    page5HelpVars,
    arrlen(page5HelpVars)
};

// Page 6 - Menus
static helpvar_t page6HelpVars[] =
{
    { "key_menu_activate",   NULL, "joyb_menu_activate" }, // toggle menus
    { "key_menu_up",         NULL, "joyb_menu_up"       }, // previous item
    { "key_menu_down",       NULL, "joyb_menu_down"     }, // next item
    { "key_menu_left",       NULL, "joyb_menu_left"     }, // decrease value
    { "key_menu_right",      NULL, "joyb_menu_right"    }, // increase value
    { "key_menu_back",       NULL, "joyb_menu_back"     }, // go back
    { "key_menu_forward",    NULL, "joyb_menu_forward"  }, // activate
    { "key_menu_confirm",    NULL, "joyb_menu_confirm"  }, // answer yes
    { "key_menu_abort",      NULL, "joyb_menu_abort"    }, // answer no
    { "key_menu_incscreen",  NULL, NULL                 }, // screen size up
    { "key_menu_decscreen",  NULL, NULL                 }, // screen size down
    { "key_menu_screenshot", NULL, NULL                 }, // screenshot
};

static helppage_t helppage6 =
{
    page6HelpVars,
    arrlen(page6HelpVars)
};

// Pages Lookup
static helppage_t *helpPages[HELP_NUMPAGES] =
{
    &helppage1,
    &helppage2,
    &helppage3,
    &helppage4,
    &helppage5,
    &helppage6
};

//=============================================================================
//
// Fonts
//

static patch_t *kbfont[M_FONTSIZE]; // orange keyboard font
static patch_t *msfont[M_FONTSIZE]; // blue mouse font
static patch_t *gpfont[M_FONTSIZE]; // white gamepad font

//
// Cache a single font character
//
static void M_CacheChar(int i, int j, char *fmt, patch_t **font)
{
    int  lumpnum;
    char buffer[9];

    M_snprintf(buffer, sizeof(buffer), fmt, j);

    if((lumpnum = W_CheckNumForName(buffer)) >= 0)
    {
        font[i] = W_CacheLumpNum(lumpnum, PU_STATIC);        
    }
}

//
// Load all help fonts
//
static void M_LoadHelpFonts(void)
{
    int i, j;

    for(i = 0, j = M_FONTSTART; i < M_FONTSIZE; i++, j++)
    {
        M_CacheChar(i, j, FONT_FMT_KB,    kbfont);
        M_CacheChar(i, j, FONT_FMT_MOUSE, msfont);
        M_CacheChar(i, j, FONT_FMT_PAD,   gpfont);
    }
}

//
// Free the help fonts
//
static void M_FreeHelpFonts(void)
{
    int i;

    for(i = 0; i < M_FONTSIZE; i++)
    {
        if(kbfont[i])
            Z_Free(kbfont[i]);
        if(msfont[i])
            Z_Free(msfont[i]);
        if(gpfont[i]) 
            Z_Free(gpfont[i]);
    }

    memset(kbfont, 0, sizeof(kbfont));
    memset(msfont, 0, sizeof(msfont));
    memset(gpfont, 0, sizeof(gpfont));
}

//
// Write text at location in font
//
static void M_WriteHelpText(const char *text, int x, int y, patch_t **font)
{
    int	        w;
    const char* ch;
    int         c;
    int         cx;
    int         cy;

    ch = text;
    cx = x;
    cy = y;

    while(1)
    {
        c = *ch++;
        if(!c)
            break;

        c = toupper(c) - M_FONTSTART;
        if(c < 0 || c >= M_FONTSIZE || !font[c])
        {
            cx += 4;
            continue;
        }

        w = SHORT(font[c]->width);

        V_DrawPatchDirect(cx, cy, font[c]);
        cx += w - 1;
    }
}

//=============================================================================
//
// State Machine
//
// Cycle through showing keyboard, mouse, and gamepad bindings.
//

enum
{
    HELP_STATE_KEYBOARD,
    HELP_STATE_MOUSE,
    HELP_STATE_GAMEPAD
};

static int helpstate;
static int helpstatetics; // # of tics til next state
static boolean helpWideScreenActive = false;

// font-for-state table
static patch_t **fontForState[] =
{
    kbfont,
    msfont,
    gpfont
};

//
// Increment the help state, with wrap around.
//
static void M_NextHelpState(void)
{
    ++helpstate;
    if(helpstate > HELP_STATE_GAMEPAD)
        helpstate = HELP_STATE_KEYBOARD;
}

//
// Get the proper description for a help variable depending on the current
// help state.
//
static const char *M_HelpGetItemText(helpvar_t *var)
{
    const char *desc = "--";

    switch(helpstate)
    {
    case HELP_STATE_KEYBOARD:
        if(var->keyboard)
            desc = FE_GetKeyName(var->keyboard);
        break;
    case HELP_STATE_MOUSE:
        if(var->mouse)
            desc = FE_NameForCfgMouseButton(M_GetIntVariable(var->mouse));
        break;
    case HELP_STATE_GAMEPAD:
        if(var->gamepad)
            desc = FE_ButtonNameForNum(M_GetIntVariable(var->gamepad));
        break;
    default:
        break;
    }

    return desc;
}

int helppage;

//=============================================================================
//
// Drawing Functions
//

//
// Draw low-resolution background
//
static void M_HelpDrawLowBG(void)
{
    V_DrawPatch(0, 0, W_CacheLumpName(bglow[helppage], PU_CACHE));
}

//
// Draw all the help variable description strings for the current page and
// current display state.
//
static void M_HelpDrawDescriptions(void)
{
    helppage_t *page = helpPages[helppage];
    size_t i, numitems = page->numhelpvars;

    int x = TEXT_START_X;
    int y = TEXT_START_Y;

    for(i = 0; i < numitems; i++)
    {
        const char *desc = M_HelpGetItemText(&page->helpvars[i]);
        M_WriteHelpText(desc, x, y, fontForState[helpstate]);

        y += TEXT_LINE_STEP;
    }
}

//=============================================================================
//
// External Interface
//

//
// Init the help screen system
//
void M_InitHelp(void)
{
    M_LoadHelpFonts();

#ifdef SVE_PLAT_SWITCH
    helpstate     = HELP_STATE_GAMEPAD;
#else
    helpstate     = HELP_STATE_KEYBOARD;
#endif
    helpstatetics = HELP_STATE_TIME;
    helppage      = HELP_PG_MOVEMENT;
    
    if(use3drenderer)
    {
        // check if widescreen is enabled or not
        if(((screen_width * FRACUNIT) / screen_height) > 4 * FRACUNIT / 3)
            helpWideScreenActive = true;
    }
}

//
// Shutdown the help screen system
//
void M_StopHelp(void)
{
    M_FreeHelpFonts();
    helpWideScreenActive = false;
}

//
// Respond to events for the help screens
// Returns true if the user is done with the help screens
//
boolean M_HelpResponder(event_t *ev)
{
    int startpage = helppage;

    // keyboard inputs
    if(ev->type == ev_keydown)
    {
        if(ev->data1 == key_menu_left || ev->data1 == key_menu_back)
        {
            if(helppage > HELP_PG_MOVEMENT)
                --helppage;
        }
        else if(ev->data1 == key_menu_right || ev->data1 == key_menu_forward)
        {
            if(helppage == HELP_NUMPAGES - 1)
                return true; // done
            else
                ++helppage;
        }
        else if(ev->data1 == key_menu_activate)
        {
            return true; // done
        }
    }
    else if(ev->type == ev_joybtndown) // gamepad inputs
    {
        if(ev->data1 == joybmenu_left || ev->data1 == joybmenu_back)
        {
            if(helppage > HELP_PG_MOVEMENT)
                --helppage;
        }
        else if(ev->data1 == joybmenu_right || ev->data1 == joybmenu_forward)
        {
            if(helppage == HELP_NUMPAGES - 1)
                return true; // done
            else
                ++helppage;
        }
        else if(ev->data1 == joybmenu)
        {
            return true; // done
        }
    }
    else if(ev->type == ev_mousebutton) // mouse click inputs
    {
        if(ev->data1 & 1)
        {
            if(helppage == HELP_NUMPAGES - 1)
                return true; // done
            else
                ++helppage;
        }
        if(ev->data1 & 2)
        {
            return true; // done
        }
    }

    if(helppage != startpage)
        S_StartSound(NULL, sfx_swtchn);

    return false;
}

//
// Per-tick logic for help screens
//
void M_HelpTicker(void)
{
    // count down state cycle
#ifndef SVE_PLAT_SWITCH
    if(--helpstatetics == 0)
    {
        helpstatetics = HELP_STATE_TIME;
        M_NextHelpState();
    }
#endif
}

//
// Rendering for help screens
//
void M_HelpDrawer(void)
{
    // don't draw here, because we're going to draw the
    // widescreen version later on
    if(helpWideScreenActive)
        return;

    // render background
    M_HelpDrawLowBG();

    // render descriptions
    M_HelpDrawDescriptions();
}

//
// GL rendering for hires screen modes
//
void M_HelpDrawerGL(void)
{
    if(!use3drenderer || !helpWideScreenActive)
        return;

    // pop back to normal HUD projection
    dglMatrixMode(GL_PROJECTION);
    dglPopMatrix();
    dglMatrixMode(GL_MODELVIEW);
    dglPopMatrix();

    patch_t* patch = W_CacheLumpName("HELP0", PU_CACHE);
    const int xoff = -((patch->width - 320) / 2);
    if (xoff < 0)
    {
        static rbTexture_t* rbPageTexture = NULL;

        if (rbPageTexture == NULL)
        {
            rbPageTexture = RB_GetTexture(RDT_PATCH, W_GetNumForName("HELP0"), 0);
        }

        RB_DrawTexture(rbPageTexture, xoff, 0, 0, 0, 0xff);
    }

    // render background
    M_HelpDrawLowBG();

    // draw text
    M_HelpDrawDescriptions();
}

// EOF

