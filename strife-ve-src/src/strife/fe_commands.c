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

#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "z_zone.h"

#include "doomtype.h"
#include "hu_stuff.h"
#include "i_sound.h"
#include "i_video.h"
#include "m_config.h"
#include "m_menu.h"
#include "m_misc.h"
#include "sounds.h"
#include "s_sound.h"

#include "fe_commands.h"
#include "fe_frontend.h"
#include "fe_gamepad.h"
#include "fe_menuengine.h"
#include "fe_menus.h"
#include "fe_multiplayer.h"

//=============================================================================
//
// Help Strings
//

// help string structure
typedef struct fehelpstr_s
{
    const char *var;
    const char *help;
} fehelpstr_t;

// help strings for variables
static fehelpstr_t helpStrs[] =
{
    {
        "autoaim",
        "Toggle automatic aim assistance on or off. If off, you'll need to be "
        "exactly on the mark to hit enemies above or below."
    },
    {
        "autorun",
        "You will run automatically, the run button will make you walk instead."
    },
    { 
        "classicmode", 
        "The game will behave as much as possible like the original "
        "\"Strife: Quest for the Sigil\", even including some bugs." 
    },
    {
        "d_fpslimit",
        "If enabled along with interpolation, the game will cap to 60 FPS. "
        "Uses less CPU, saves laptop battery power. Vsync overrides this."
    },
    {
        "damage_indicator",
        "A directional HUD arrow to help you figure out where those bullets "
        "came from, so you can get some payback."
    },
    {
        "deathmatch",
        "Select the type of game that will be played. Deathmatch is normal rules; "
        "AltDeath adds item respawning, and weapons don't stay."
    },
    {
        "fastparm",
        "For this game only, enemies attack with extreme aggression! Not for "
        "the assassin with a faint heart."
    },
    {
        "fe_musicnum",
#ifdef SVE_PLAT_SWITCH
        "Use left or right to choose a track, and select to play. "
#else
        "Use left or right to choose a track, and click or press confirm to play. "
#endif
        "In memory of composer Morey Goldstein."
    },
    {
        "fullscreen",
        "Select fullscreen or windowed mode. You must restart for this setting "
        "to take effect."
    },
    {
        "fullscreen_hud",
        "Toggle drawing of the fullscreen heads-up display."
    },
    {
        "gfx_more",
        "View and change more options for the high quality renderer."
    },
    {
        "gl_bloom_threshold",
        "Determines strength of the bloom effect. A lower threshold creates "
        "a more powerful effect."
    },
    {
        "gl_decals",
        "Enable decals on walls, floors, and ceilings in the high quality "
        "renderer."
    },
    {
        "gl_dynamic_lights",
        "Toggle dynamic lights in the high quality renderer."
    },
    {
        "gl_dynamic_light_fast_blend",
        "Choose a dynamic lighting model. \"Quality\" looks more like real "
        "light, but requires more horsepower."
    },
    {
        "gl_enable_bloom",
        "Toggle bloom in the high quality renderer. When enabled, bright areas "
        "will be emphasized through high dynamic range."
    },
    {
        "gl_enable_fxaa",
        "Toggle fullscreen anti-aliasing in the high quality renderer. Smoothes "
        "out the edges of walls."
    },
    {
        "gl_enable_motion_blur",
        "Toggle motion blur effect in the high quality renderer."
    },
    {
        "gl_enable_renderer",
        "Select the high quality true 3D renderer or the original software mode. "
        "You must restart for this setting to take effect."
    },
    {
        "gl_enable_vsync",
        "Avoids screen tearing in high quality renderer but limits FPS. "
        "You must restart for this setting to take effect.",
    },
    {
        "gl_fix_sprite_clipping",
        "Clip sprites in the high quality renderer in the same way the original "
        "game did in software."
    },
    {
        "gl_force_sync",
        "Forces all GL drawing to finish before the end of a frame. Recommended "
        "for AMD Radeon 4800 series cards and lower."
    },
    {
        "gl_lightmaps",
        "Toggle static lightmaps in the high quality renderer. Does not take "
        "effect until a new level or game is started."
    },
    {
        "gl_linear_filtering",
        "Toggle linear filtering in the high quality renderer. This trades fine "
        "pixel art detail for a smooth appearance."
    },
    {
        "gl_max_decals",
        "Set the maximum number of decals. Range is from 16 to 128."
    },
    {
        "gl_fov",
        "Set the view FOV. Range is from 74.0 to 110.0."
    },
    {
        "gl_motion_blur_samples",
        "Number of samples taken for motion blur. More is higher quality, but "
        "requires more processing power. Scale is logarithmic."
    },
    {
        "gl_motion_blur_ramp_speed",
        "Speed of motion blur effect."
    },
    {
        "gl_outline_sprites",
        "Highlight important items in the game world with a blinking outline. "
        "Requires high quality renderer."
    },
    {
        "gl_show_crosshair",
#ifdef SVE_PLAT_SWITCH
        "A persistent crosshair will be displayed for better aiming."
#else
        "A persistent crosshair will be displayed for better aiming. Requires "
        "the high quality renderer."
#endif
    },
    {
        "gl_textured_automap",
#ifdef SVE_PLAT_SWITCH
        "Toggle display of floor textures on the automap."
#else
        "Toggle display of floor textures on the automap in the high quality "
        "renderer."
#endif
    },
    {
        "gl_wall_shades",
#ifdef SVE_PLAT_SWITCH
        "Toggle shading effects on walls, floors, ceilings, and sprites."
#else
        "Toggle shading effects on walls, floors, ceilings, and sprites in the "
        "high quality renderer."
#endif
    },
    {
        "interpolate_frames",
        "When enabled, moving objects will change positions smoothly, "
        "even between animation frames."
    },
    {
        "invite",
        "Use the overlay to invite friends you want to play with into "
        "your lobby."
    },
    {
        "key_weapon1",
        "A katar you keep hidden for stealh attacks. Becomes "
        "more powerful with stamina upgrades."
    },
    {
        "key_weapon2",
        "The crossbow can fire electric and poison bolts. Poison bolts "
        "can kill humans in one hit, and are highly stealthy."
    },
    {
        "key_weapon3",
        "Fully automatic, this weapon is the Front's mainstay. Go light "
        "on the trigger or you'll chew through your ammo in no time."
    },
    {
        "key_weapon4",
        "The mini-missile launcher is fast and packs a punch. Use at a "
        "good range, or you'll take damage too."
    },
    {
        "key_weapon5",
        "Fires high explosive and white phosphorous rounds. Extremely "
        "powerful, and extremely dangerous. Don't blow yourself up!"
    },
    {
        "key_weapon6",
        "Made from parts torn off a downed Crusader. Immolates humans, "
        "but is less effective against machines."
    },
    { 
        "key_weapon7",
        "The Mauler is an energy shotgun, the favorite weapon of the "
        "Order's Templars. Its torpedo alt-mode destroys anything."
    },
    {
        "key_weapon8",
        "Mysterious sentient weapon worshipped by the Order. Find all "
        "five pieces and it's said you can rule the world - or destroy it."
    },
    {
        "key_prevweapon",
        "Select the previous weapon you currently own."
    },
    {
        "key_nextweapon",
        "Select the next weapon you currently own."
    },
    {
        "leavelobby",
        "Abandon the setup of this multiplayer game, and live to die another day."
    },
    {
        "lobbylist",
        "View a list of public lobbies you can join. Only lobbies created by your "
        "friends will show up in this list."
    },
    { 
        "max_gore", 
        "More blood, more gibs, more awesome. For those who like it \"brutal\"..." 
    },
    {
        "mbuttons",
        "Select this option to configure your mouse button bindings."
    },
    {
        "mouse_enable_acceleration",
        "Toggles mouse acceleration."
    },
    {
        "mouse_acceleration",
        "When the speed of mouse movement exceeds the threshold, it will be "
        "multiplied by this value. Adds more speed for fast movements."
    },
    {
        "mouse_sensitivity_X",
        "Basic scale factor for horizontal mouse motion."
        "A higher value means faster motion for less mouse movement."
    },
    {
        "mouse_sensitivity_Y",
        "Basic scale factor for vertical mouse motion."
        "A higher value means faster motion for less mouse movement."
    },
    {
        "mouse_threshold",
        "Determines when mouse acceleration kicks in. A lower value means "
        "higher speed, but less accuracy."
    },
    {
        "mouse_scale",
        "Overall scale multiplier for mouse motion. A higher value means faster "
        "motion for less mouse movement."
    },
    {
        "mouse_smooth",
        "Toggles smooth mouse movement."
    },
    {
        "mouse_invert",
        "Toggles inverted mouselook."
    },
    {
        "mpoptions",
        "Since you own this lobby, you can determine the starting level, "
        "skill, game mode, and other options here."
    },
    {
        "music_volume",
        "Volume of music."
    },
    {
        "newpublobby",
        "Create a new public game lobby. Any of your friends will "
        "be able to join this lobby if they feel like a good fragging."
    },
    {
        "newprivlobby",
        "Create a new private game lobby. You'll need to invite some of "
        "your friends into it in order to play a game."
    },
    {
        "nomonsters",
        "Monsters will not be spawned on the map, allowing for maximum "
        "concentration on the deathmatch."
    },
    {
        "novert",
        "If enabled, you can look up and down freely using the mouse. "
        "Using the high quality renderer allows a wider range."
    },
    {
        "randomparm",
        "The locations of respawning items will be randomized if AltDeath mode "
        "is selected."
    },
    {
        "respawnparm",
        "For this game only, robots and mechanical enemies will respawn "
        "after 16 seconds. Be quick, or die in shame and dishonor!"
    },
    {
        "screen_width",
        "Select a supported display resolution. You must restart for this setting "
        "to take effect."
    },
    {
        "setready",
        "Mark yourself as ready or not ready, so your friends will know if "
        "the game is good to go."
    },
    {
        "setteam",
        "Set the team you want to be on if this is a Capture the Chalice match. "
        "Auto allows the game to assign players to teams itself."
    },
    {
        "sfx_volume",
        "Volume of digital sound effects, excluding voice acting."
    },
    {
        "show_talk",
        "Full subcaption text will be displayed when talking to characters with "
        "voice acting."
    },
    {
        "snd_musicdevice",
#ifdef USE_YMFMOPL
        "Select high quality Roland Sound Canvas or emulated OPL music."
#else
        "Select high quality Roland Sound Canvas or emulated OPL music. You must "
        "restart for this setting to take effect."
#endif
    },
    {
        "startgame",
        "Exit setup and let the mayhem begin!"
    },
    {
        "startmap",
        "Start on the selected map. If a map between 36 and 38 is selected, "
        "a Capture the Chalice match will automatically begin."
    },
    {
        "startskill",
        "Select the skill level. Players take half damage in Training, and get "
        "double ammo in Training and Bloodbath."
    },
    {
        "timelimit",
        "Set a number of minutes after which play will automatically proceed "
        "to the next map in numeric order. If zero, there is no limit."
    },
    {
        "voice_volume",
        "Volume of character voices. Note that the volume of other sounds will be "
        "dynamically reduced during dialogue, to keep it audible."
    },
    {
        "weapon_recoil",
        "If using the high quality renderer, your view will kick back in response "
        "to heavy arms fire."
    },
    {
        "window_noborder",
        "Select whether or not to hide the window border. "
        "You must restart for this setting to take effect."
    },
};

//
// Find help for a variable.
// Returns NULL if there's no help string for that variable.
//
const char *FE_GetHelpForVar(const char *var)
{
    int i;

    for(i = 0; i < arrlen(helpStrs); i++)
    {
        if(!strcasecmp(var, helpStrs[i].var))
            return helpStrs[i].help;
    }

    return NULL;
}

//
// Returns a formatted copy of the help string for drawing at the given
// location. Free the string when you're done with it.
//
char *FE_GetFormattedHelpStr(const char *var, int x, int y)
{
    const char *help;

    if((help = FE_GetHelpForVar(var)))
    {
        char *mutableHelp = M_Strdup(help);
        M_DialogDimMsg(x, y, mutableHelp, true);
        return mutableHelp;
    }

    return NULL;
}

//=============================================================================
//
// Commands
//

static void FE_CmdPopMenu(void)
{
    FE_PopMenu(false);
}

// Menu command structure
typedef struct fecmd_s
{
    const char *name;
    void (*handler)(void);
} fecmd_t;

// commands lookup array
static fecmd_t cmds[] =
{
    { "go",           FE_CmdGo              },
    { "exit",         FE_CmdExit            },
    { "back",         FE_CmdPopMenu         },
    { "options",      FE_CmdOptions         },
    { "gameplay",     FE_CmdGameplay        },
#ifndef SVE_PLAT_SWITCH	
    { "keyboard",     FE_CmdKeyboard        },
    { "kb_functions", FE_CmdKeyboardFuncs   },
    { "kb_inventory", FE_CmdKeyboardInv     },
    { "kb_map",       FE_CmdKeyboardMap     },
    { "kb_menus",     FE_CmdKeyboardMenu    },
    { "kb_movement",  FE_CmdKeyboardMove    },
    { "kb_weapons",   FE_CmdKeyboardWeapons },
    { "mouse",        FE_CmdMouse           },
    { "mbuttons",     FE_CmdMouseButtons    },
#endif
    { "graphics",     FE_CmdGraphics        },
    { "gfxbasic",     FE_CmdGfxBasic        },
    { "gfxlights",    FE_CmdGfxLights       },
    { "gfxsprites",   FE_CmdGfxSprites      },
    { "gfxadvanced",  FE_CmdGfxAdvanced     },
    { "audio",        FE_CmdAudio           },
    { "about",        FE_CmdAbout           },
    { "about2",       FE_CmdAbout2          },
    { "multi",        FE_CmdMulti           },
    { "newpublobby",  FE_CmdNewPublicLobby  },
    { "newprivlobby", FE_CmdNewPrivateLobby },
    { "lobbysrv",     FE_CmdLobbySrv        },
    { "lobbyclient",  FE_CmdLobbyClient     },
    { "leavelobby",   FE_CmdLeaveLobby      },
    { "invite",       FE_CmdInvite          },
    { "mpoptions",    FE_CmdMPOptions       },
    { "joinlobby",    FE_CmdJoinLobby       },
    { "lobbyrefresh", FE_CmdLobbyRefresh    },
    { "lobbylist",    FE_CmdLobbyList       },
    { "startgame",    FE_CmdStartGame       },
    { "selgamepad",   FE_CmdSelGamepad      },
    { "gamepaddev",   FE_CmdGamepadDev      },
    { "gamepad",      FE_CmdGamepad         },
    { "gpaxes",       FE_CmdGPAxes          },
    { "gpautomap",    FE_CmdGPAutomap       },
    { "gpmenus",      FE_CmdGPMenus         },
    { "gpmovement",   FE_CmdGPMovement      },
    { "gpinv",        FE_CmdGPInv           },
    { "gpgyro",       FE_CmdGPGyro          },
    { "gpprofile",    FE_CmdGPProfile       },
    {"gpdefault",     FE_CmdJoyBindReset    },

    { NULL,           NULL                  }
};

//
// Execute a menu command
//
void FE_ExecCmd(const char *verb)
{
    fecmd_t *cmd = cmds;

    while(cmd->name && 
          strcasecmp(cmd->name, verb))
      ++cmd;

    if(cmd->handler)
        cmd->handler();
}

//=============================================================================
//
// Ranged Variables
//

static void FE_SetSfxVolume(int volume)
{
    S_SetSfxVolume(volume * 8);
}

static void FE_SetVoiceVolume(int volume)
{
    S_SetVoiceVolume(volume * 8);
}

static void FE_SetMusicVolume(int volume)
{
    S_SetMusicVolume(volume * 8);
}

static fevar_t feVariables[] =
{
    { "gl_bloom_threshold",        FE_VAR_FLOAT,   0,    0, 0, 0.5f, 1.0f, 0.05f   },
    { "gl_max_decals",             FE_VAR_INT,     16, 128, 8                      },
    { "gl_fov",                    FE_VAR_FLOAT,   0,    0, 0, 74.0f, 110.0f, 1.5f },
    { "gl_motion_blur_ramp_speed", FE_VAR_FLOAT,   0,    0, 0, 0.0f, 1.0f, 0.0625f },
    { "gl_motion_blur_samples",    FE_VAR_INT_PO2, 3,    6, 1                      },
    { "mouse_acceleration",        FE_VAR_FLOAT,   0,    0, 0, 1.0f, 5.0f, 0.25f   },
    { "mouse_sensitivity_X",       FE_VAR_INT,     0,    9, 1                      },
    { "mouse_sensitivity_Y",       FE_VAR_INT,     0,    9, 1                      },
    { "mouse_threshold",           FE_VAR_INT,     0,   32, 1                      },
    { "mouse_scale",               FE_VAR_INT,     0,   4,  1                      },
    { "music_volume",              FE_VAR_INT,     0,   15, 1, 0.0f, 0.0f, 0.0f,   FE_SetMusicVolume },
    { "sfx_volume",                FE_VAR_INT,     0,   15, 1, 0.0f, 0.0f, 0.0f,   FE_SetSfxVolume   },
    { "timelimit",                 FE_VAR_INT,     0,   15, 1                      },
    { "voice_volume",              FE_VAR_INT,     0,   15, 1, 0.0f, 0.0f, 0.0f,   FE_SetVoiceVolume },
    { "joy_gyrosensitivityh",      FE_VAR_FLOAT,   0,    0, 0, 0.0f, 4.0f, 0.1f    },
    { "joy_gyrosensitivityv",      FE_VAR_FLOAT,   0,    0, 0, 0.0f, 4.0f, 0.1f    },
    { "joystick_turnsensitivity",  FE_VAR_FLOAT,   0,    0, 0, 0.1f, 1.0f, 0.1f    },
    { "joystick_looksensitivity",  FE_VAR_FLOAT,   0,    0, 0, 0.1f, 1.0f, 0.1f    },
    { NULL,                        FE_VAR_INT,     0,    0, 0                      },
};

//
// Look up a fevar_t structure by variable name.
//
fevar_t *FE_VariableForName(const char *name)
{
    fevar_t *var = feVariables;

    while(var->name &&
          strcasecmp(var->name, name))
      ++var;

    return var->name ? var : NULL;
}

//
// Increment the value of a variable, not exceeding its allowed maximum.
//
boolean FE_IncrementVariable(const char *name)
{
    boolean  res = false;
    fevar_t *var = FE_VariableForName(name);
    if(!var)
        return false;

    switch(var->type)
    {
    case FE_VAR_INT:
        {
            char buf[33];
            int val = M_GetIntVariable(name);
            if(val < var->max)
            {
                val += var->istep;
                if(val > var->max)
                    val = var->max;
            }
            else
                val = var->min; // wrap around
                
            res = M_SetVariable(name, M_Itoa(val, buf, 10));

            // has callback?
            if(res && var->SetFunc)
                var->SetFunc(val);
        }
        break;
    case FE_VAR_INT_PO2:
        {
            char buf[33];
            unsigned int v = (unsigned int)M_GetIntVariable(name);
            unsigned int r = 0;

            // calculate rational log 2 of integer
            while(v >>= 1)
                ++r;

            if(r < var->max)
            {
                r += var->istep;
                if(r > var->max)
                    r = var->max;
            }
            else
                r = var->min;

            res = M_SetVariable(name, M_Itoa(1 << r, buf, 10));
        }
        break;
    case FE_VAR_FLOAT:
        {
            char buf[32];
            float val = M_GetFloatVariable(name);
            if(val < var->fmax)
            {
                val += var->fstep;
                if(val > var->fmax)
                    val = var->fmax;
            }
            else
                val = var->fmin; // wrap around

            M_snprintf(buf, sizeof(buf), "%.06f", val);
            res = M_SetVariable(name, buf);
        }
        break;
    default:
        break;
    }
    if(res)
        S_StartSound(NULL, sfx_stnmov);
    return res;
}

//
// Decrement the value of a variable, not exceeding its allowed minimum.
//
boolean FE_DecrementVariable(const char *name)
{
    boolean  res = false;
    fevar_t *var = FE_VariableForName(name);
    if(!var)
        return false;

    switch(var->type)
    {
    case FE_VAR_INT:
        {
            char buf[33];
            int val = M_GetIntVariable(name);
            if(val > var->min)
            {
                val -= var->istep;
                if(val < var->min)
                    val = var->min;
            }
            else
                val = var->max; // wrap around

            res = M_SetVariable(name, M_Itoa(val, buf, 10));

            // has callback?
            if(res && var->SetFunc)
                var->SetFunc(val);
        }
        break;
    case FE_VAR_INT_PO2:
        {
            char buf[33];
            unsigned int v = (unsigned int)M_GetIntVariable(name);
            unsigned int r = 0;

            // calculate rational log 2 of integer
            while(v >>= 1)
                ++r;

            if(r > var->min)
            {
                r -= var->istep;
                if(r < var->min)
                    r = var->min;
            }
            else
                r = var->max;

            res = M_SetVariable(name, M_Itoa(1 << r, buf, 10));
        }
        break;
    case FE_VAR_FLOAT:
        {
            char buf[32];
            float val = M_GetFloatVariable(name);
            if(val > var->fmin)
            {
                val -= var->fstep;
                if(val < var->fmin)
                    val = var->fmin;
            }
            else
                val = var->fmax; // wrap around
            
            M_snprintf(buf, sizeof(buf), "%.06f", val);
            res = M_SetVariable(name, buf);
        }
        break;
    default:
        break;
    }
    if(res)
        S_StartSound(NULL, sfx_stnmov);
    return res;
}

//=============================================================================
//
// Resolution pick list
//

typedef struct fevideomode_s
{
    int w;
    int h;
} fevideomode_t;

static const char    **modeStrings;
static fevideomode_t  *modeStructs;
static int             numModes;
static int             curModeNum;

//
// Build the video mode list if not already built
//
static void FE_BuildModeList(void)
{
    int i;
    int prevWidth, prevHeight;
    SDL_DisplayMode mode;
    boolean curModeInList = false;
    static boolean modesBuilt;
    int numModesAlloc;

    if(modesBuilt)
        return;
    
    const int numDisplays = SDL_GetNumDisplayModes(0);

    if(numDisplays <= 0)
    {
        char buf[32];
        
        numModes = 2;
        modeStrings = Z_Calloc(numModes, sizeof(*modeStrings), PU_STATIC, NULL);
        modeStructs = Z_Calloc(numModes, sizeof(*modeStructs), PU_STATIC, NULL);

        modeStructs[0].w = 640;
        modeStructs[0].h = 480;
        modeStrings[0] = "640x480";

        modeStructs[1].w = screen_width;
        modeStructs[1].h = screen_height;
        M_snprintf(buf, sizeof(buf), "%dx%d", screen_width, screen_height);
        modeStrings[1] = M_Strdup(buf);
        curModeNum = 1;


        modesBuilt = true;
        return;
    }

    // count modes, and check if current mode is in list
    prevWidth = prevHeight = -1;
    for(i = 0; i < numDisplays; i++)
    {
        if(SDL_GetDisplayMode(0, i, &mode) < 0)
        {
            continue;
        }

        if(prevWidth != mode.w || prevHeight != mode.h)
        {
            numModes++;
            prevWidth = mode.w;
            prevHeight = mode.h;
        }

        if(screen_width == mode.w && screen_height == mode.h)
        {
            curModeInList = true;
        }
    }

    numModesAlloc = numModes + !curModeInList;

    modeStrings = Z_Calloc(numModesAlloc, sizeof(*modeStrings), PU_STATIC, NULL);
    modeStructs = Z_Calloc(numModesAlloc, sizeof(*modeStructs), PU_STATIC, NULL);

    prevWidth = prevHeight = -1;
    // Keeping same ordering as old builds under SDL2 requires counting down
    int index = numModesAlloc - 1;
    for(i = 0; i < numDisplays; i++)
    {
        char buf[32];
        //int modeIdx = numModes - (i + 1);

        SDL_GetDisplayMode(0, i, &mode);

        // don't add modes for resolutions already present
        if(prevWidth == mode.w && prevHeight == mode.h)
        {
            continue;
        }

        modeStructs[index].w = prevWidth = mode.w;
        modeStructs[index].h = prevHeight = mode.h;

        M_snprintf(buf, sizeof(buf), "%dx%d", mode.w, mode.h);
        modeStrings[index] = M_Strdup(buf);

        if(screen_width == mode.w && screen_height == mode.h)
            curModeNum = index;

        index--;
    }

    if(!curModeInList)
    {
        char buf[32];

        ++numModes;
        modeStructs[0].w = screen_width;
        modeStructs[0].h = screen_height;
        
        M_snprintf(buf, sizeof(buf), "%dx%d", screen_width, screen_height);
        modeStrings[0] = M_Strdup(buf);
        curModeNum = 0;
    }

    modesBuilt = true;
}

//
// Get current mode string
//
const char *FE_CurrentVideoMode(void)
{
    // make sure modes are built
    FE_BuildModeList();

    return modeStrings[curModeNum];
}

//
// Increment mode num
//
void FE_NextVideoMode(void)
{
    // make sure modes are built
    FE_BuildModeList();

    if(curModeNum == numModes - 1)
        curModeNum = 0;
    else
        ++curModeNum;

    default_screen_width  = modeStructs[curModeNum].w;
    default_screen_height = modeStructs[curModeNum].h;

    S_StartSound(NULL, sfx_swtchn);
}

//
// Decrement mode num
//
void FE_PrevVideoMode(void)
{
    // make sure modes are built
    FE_BuildModeList();

    if(curModeNum == 0)
        curModeNum = numModes - 1;
    else
        --curModeNum;

    default_screen_width  = modeStructs[curModeNum].w;
    default_screen_height = modeStructs[curModeNum].h;

    S_StartSound(NULL, sfx_swtchn);
}

//=============================================================================
//
// Value Selector
//

typedef struct fevaluerange_s
{
    boolean toggle;      // if true, clatch
    const char *name;    // name of variable
    int min;             // minimum value 
    int max;             // maximum value
    const char *const *values; // array of values
    int (*callback)(struct fevaluerange_s *, int); // value callback
} fevaluerange_t;

static const char *skillNames[] =
{
    "Training",
    "Rookie",
    "Veteran",
    "Elite",
    "Bloodbath"
};

static const char *dmTypeNames[] =
{
    "Deathmatch",
    "AltDeath"
};

static const char *musDeviceNames[] =
{
    "Roland SC-55",
#ifdef USE_YMFMOPL
    "OPL3"
#else
    "OPL2"
#endif
};

static const char *dynLightTypes[] =
{
    "Quality", 
    "Fast"
};

static const char *gyroTypes[] =
{
    "Yaw",
    "Roll",
    "Inverse Roll"
};

enum
{
    FE_MUSIC_ACTION,
    FE_MUSIC_CASTLE,
    FE_MUSIC_DANGER,
    FE_MUSIC_DARK,
    FE_MUSIC_DARKER,
    FE_MUSIC_DRONE,
    FE_MUSIC_END,
    FE_MUSIC_FAST,
    FE_MUSIC_FIGHT,
    FE_MUSIC_HAPPY,
    FE_MUSIC_INDUSTRY,
    FE_MUSIC_INTRO,
    FE_MUSIC_LOGO,
    FE_MUSIC_MARCH,
    FE_MUSIC_MOOD,
    FE_MUSIC_PANTHER,
    FE_MUSIC_SAD,
    FE_MUSIC_SLIDE,
    FE_MUSIC_STRIKE,
    FE_MUSIC_SUSPENSE,
    FE_MUSIC_TAVERN,
    FE_MUSIC_TECH,
    FE_MUSIC_TRIBAL,

    FE_NUMMUSIC
};

static const char *feMusicNames[FE_NUMMUSIC] =
{
    "Action",
    "Castle",
    "Danger",
    "Dark",
    "Darker",
    "Drone",
    "End",
    "Fast",
    "Fight",
    "Happy",
    "Industry",
    "Intro",
    "Logo",
    "March",
    "Mood",
    "Panther",
    "Sad",
    "Slide",
    "Strike",
    "Suspense",
    "Tavern",
    "Tech",
    "Tribal"
};

static int FE_doChangeMusicEngine(fevaluerange_t *vr, int dir)
{
    int returnValue = 0;
    if(dir != 0) // toggling
    {
        if(default_snd_musicdevice == SNDDEVICE_GENMIDI)
        {
#ifdef USE_YMFMOPL
            default_snd_musicdevice = SNDDEVICE_OPL;
#else
            default_snd_musicdevice = SNDDEVICE_SB;
#endif
            returnValue = 1;
        }
        else
        {
            default_snd_musicdevice = SNDDEVICE_GENMIDI;
            returnValue = 0;
        }
    }
    else // just getting value
    {
        if(default_snd_musicdevice == SNDDEVICE_GENMIDI)
            return 0;
        else
            return 1;
    }

#ifdef USE_YMFMOPL
    // Restart the music track
    int saveMus, saveLoop;
    S_GetCurrentMusic(&saveMus, &saveLoop);
    S_StopMusic();
    snd_musicdevice = default_snd_musicdevice;
    S_ChangeMusic(saveMus, saveLoop);
#endif

    return returnValue;
}

static fevaluerange_t values[] =
{
    {
        false,
        "startmap",
        1,
        HU_NUMMAPNAMES,
        mapnames
    },
    {
        false,
        "startskill",
        0,
        4,
        skillNames
    },
    {
        false,
        "deathmatch",
        1,
        2,
        dmTypeNames
    },
    {
        false,
        "fe_musicnum",
        0,
        FE_NUMMUSIC-1,
        feMusicNames
    },
    {
        true,
        "gl_dynamic_light_fast_blend",
        0,
        1,
        dynLightTypes
    },
    {
        true,
        "snd_musicdevice",
        0,
        1,
        musDeviceNames,
        FE_doChangeMusicEngine
    },
    {
        true,
        "joy_gyrostyle",
        0,
        2,
        gyroTypes
    },
};

static fevaluerange_t *FE_FindValueRange(const char *valuevar)
{
    int i;
    fevaluerange_t *valrange = NULL;

    for(i = 0; i < arrlen(values); i++)
    {
        if(!strcasecmp(valuevar, values[i].name))
        {
            valrange = &values[i];
            break;
        }
    }

    return valrange;
}

const char *FE_NameForValue(const char *valuevar)
{
    fevaluerange_t *valrange = FE_FindValueRange(valuevar);
    int value;
    if(valrange->callback)
        value = valrange->callback(valrange, 0);
    else
        value = M_GetIntVariable(valuevar);

    if(!valrange || value < valrange->min || value > valrange->max)
        return "";
    
    value -= valrange->min;
    return valrange->values[value];
}

int FE_IncrementValue(const char *valuevar)
{
    fevaluerange_t *valrange = FE_FindValueRange(valuevar);
    int curValue; 
    char buf[33];
    if(valrange->callback)
        curValue = valrange->callback(valrange, 0);
    else
        curValue = M_GetIntVariable(valuevar);

    if(!valrange)
        return 0;

    ++curValue;
    if(curValue > valrange->max)
        curValue = valrange->min; // wrap around

    if(valrange->callback)
        valrange->callback(valrange, 1);
    else
        M_SetVariable(valuevar, M_Itoa(curValue, buf, 10));

    S_StartSound(NULL, valrange->toggle ? sfx_swtchn : sfx_stnmov);

    return curValue;
}

int FE_DecrementValue(const char *valuevar)
{
    fevaluerange_t *valrange = FE_FindValueRange(valuevar);
    int curValue;
    char buf[33];
    if(valrange->callback)
        curValue = valrange->callback(valrange, 0);
    else
        curValue = M_GetIntVariable(valuevar);

    if(!valrange)
        return 0;

    --curValue;
    if(curValue < valrange->min)
        curValue = valrange->max; // wrap around

    if(valrange->callback)
        valrange->callback(valrange, -1);
    else
        M_SetVariable(valuevar, M_Itoa(curValue, buf, 10));

    S_StartSound(NULL, valrange->toggle ? sfx_swtchn : sfx_stnmov);

    return curValue;
}

//=============================================================================
//
// Music Test
//

static int fe_musicnum;
static int fe_savedmus;
static int fe_savedloop;

static musicenum_t feMusNums[FE_NUMMUSIC] =
{
    mus_action,
    mus_castle,
    mus_danger,
    mus_dark,
    mus_darker,
    mus_drone,
    mus_end,
    mus_fast,
    mus_fight,
    mus_happy,
    mus_instry,
    mus_intro,
    mus_logo,
    mus_march,
    mus_mood,
    mus_panthr,
    mus_sad,
    mus_slide,
    mus_strike,
    mus_spense,
    mus_tavern,
    mus_tech,
    mus_tribal
};

static int feMusLoop[FE_NUMMUSIC] =
{
    1, // mus_action
    1, // mus_castle
    1, // mus_danger
    1, // mus_dark
    1, // mus_darker
    1, // mus_drone
    0, // mus_end
    1, // mus_fast
    1, // mus_fight
    0, // mus_happy
    1, // mus_instry
    1, // mus_intro
    0, // mus_logo
    1, // mus_march
    1, // mus_mood
    1, // mus_panthr
    1, // mus_sad
    1, // mus_slide
    1, // mus_strike
    1, // mus_spense
    1, // mus_tavern
    1, // mus_tech
    1  // mus_tribal
};

//
// Bind music var if not bound already
//
void FE_BindMusicTestVar(void)
{
    static boolean bound = false;

    if(!bound)
    {
        M_BindVariable("fe_musicnum", &fe_musicnum);
        bound = true;
    }
}

//
// Save the currently playing music track.
//
void FE_MusicTestSaveCurrent(void)
{
    S_GetCurrentMusic(&fe_savedmus, &fe_savedloop);
}

//
// Restart the music that was previously playing.
//
void FE_MusicTestRestoreCurrent(void)
{
    S_ChangeMusic(fe_savedmus, fe_savedloop);
}

void FE_MusicTestPlaySelection(void)
{
    int feTrackNum = M_GetIntVariable("fe_musicnum");

    S_ChangeMusic(feMusNums[feTrackNum], feMusLoop[feTrackNum]);
}

// EOF

