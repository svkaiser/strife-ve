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

#include "doomtype.h"
#include "i_video.h"
#include "hu_lib.h"
#include "m_menu.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"

#include "fe_commands.h"
#include "fe_frontend.h"
#include "fe_graphics.h"
#include "fe_menuengine.h"
#include "fe_mouse.h"
#include "fe_multiplayer.h"

//
// Menus
//

// Main Menu --------------------------------------------------

static femenuitem_t mainMenuItems[] =
{
    { FE_MITEM_CMD, "Start Game",  "go",      FE_FONT_BIG },
    { FE_MITEM_CMD, "Multiplayer", "multi",   FE_FONT_BIG },
    { FE_MITEM_CMD, "Options",     "options", FE_FONT_BIG },
    { FE_MITEM_CMD, "Quit",        "exit",    FE_FONT_BIG },
    { FE_MITEM_END, "",            ""                     }
};

static void FE_DrawMainMenu(void)
{
    V_DrawPatch(80, 2, W_CacheLumpName("M_STRIFE", PU_CACHE));
}

femenu_t mainMenu =
{
    mainMenuItems,
    arrlen(mainMenuItems),
    88,
    110,
    52,
    "Veteran Edition",
    FE_BG_SIGIL,
    FE_DrawMainMenu,
    FE_CURSOR_SIGIL,
    0
};

// Options Menu - Main ----------------------------------------

static femenuitem_t optionsMenuMainItems[] =
{
    { FE_MITEM_CMD, "Gameplay", "gameplay", FE_FONT_BIG },
    { FE_MITEM_CMD, "Keyboard", "keyboard", FE_FONT_BIG },
    { FE_MITEM_CMD, "Mouse",    "mouse",    FE_FONT_BIG },
    { FE_MITEM_CMD, "Gamepad",  "gamepad",  FE_FONT_BIG },
    { FE_MITEM_CMD, "Graphics", "graphics", FE_FONT_BIG },
    { FE_MITEM_CMD, "Audio",    "audio",    FE_FONT_BIG },
    { FE_MITEM_CMD, "About",    "about",    FE_FONT_BIG },
    { FE_MITEM_END, "",     "" }
};

femenu_t optionsMenuMain =
{
    optionsMenuMainItems,
    arrlen(optionsMenuMainItems),
    98,
    40,
    2,
    "Options",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_SIGIL,
    0
};

// "options" command
void FE_CmdOptions(void)
{
    FE_PushMenu(&optionsMenuMain);
}

// Options - Gameplay --------------------------------------

static femenuitem_t optionsGameplayItems[] =
{
    { FE_MITEM_TOGGLE, "Always Run",       "autorun",           FE_FONT_SMALL },
    { FE_MITEM_TOGGLE, "Classic Mode",     "classicmode",       FE_FONT_SMALL, FE_TOGGLE_NOTNET },
    { FE_MITEM_TOGGLE, "Crosshair",        "gl_show_crosshair", FE_FONT_SMALL },
    { FE_MITEM_TOGGLE, "Damage Indicator", "damage_indicator",  FE_FONT_SMALL },
    { FE_MITEM_TOGGLE, "Fast Enemies",     "fastparm",          FE_FONT_SMALL, FE_TOGGLE_NOTNET },
    { FE_MITEM_TOGGLE, "Fullscreen HUD",   "fullscreen_hud",    FE_FONT_SMALL },
    { FE_MITEM_TOGGLE, "Machines Respawn", "respawnparm",       FE_FONT_SMALL, FE_TOGGLE_NOTNET },
    { FE_MITEM_TOGGLE, "Maximum Gore",     "max_gore",          FE_FONT_SMALL, FE_TOGGLE_NOTNET },
    { FE_MITEM_TOGGLE, "Show Dialogue",    "show_talk",         FE_FONT_SMALL },
    { FE_MITEM_TOGGLE, "Weapon Recoil",    "weapon_recoil",     FE_FONT_SMALL },
    { FE_MITEM_END,    "",                 "" }
};

static femenu_t optionsGameplayMenu =
{
    optionsGameplayItems,
    arrlen(optionsGameplayItems),
    74,
    32,
    2,
    "Gameplay Options",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "gameplay" command
void FE_CmdGameplay(void)
{
    FE_PushMenu(&optionsGameplayMenu);
}

// Options - Keyboard (Main) -------------------------------

static femenuitem_t optionsKeyboardItems[] =
{
    { FE_MITEM_CMD, "Functions", "kb_functions", FE_FONT_BIG },
    { FE_MITEM_CMD, "Inventory", "kb_inventory", FE_FONT_BIG },
    { FE_MITEM_CMD, "Map",       "kb_map",       FE_FONT_BIG },
    { FE_MITEM_CMD, "Menus",     "kb_menus",     FE_FONT_BIG },
    { FE_MITEM_CMD, "Movement",  "kb_movement",  FE_FONT_BIG },
    { FE_MITEM_CMD, "Weapons",   "kb_weapons",   FE_FONT_BIG },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardMain =
{
    optionsKeyboardItems,
    arrlen(optionsKeyboardItems),
    98,
    40,
    4,
    "Keyboard",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_SIGIL,
    0,
    false
};

// "keyboard" command
void FE_CmdKeyboard(void)
{
    FE_PushMenu(&optionsKeyboardMain);
}

// Options - Keyboard - Functions --------------------------

static femenuitem_t optionsKeyboardFuncItems[] =
{
    { FE_MITEM_KEYBIND, "Help",               "key_menu_help"       },
    { FE_MITEM_KEYBIND, "Save Game",          "key_menu_save"       },
    { FE_MITEM_KEYBIND, "Load Game",          "key_menu_load"       }, 
    { FE_MITEM_KEYBIND, "Sound Volume",       "key_menu_volume"     },
    { FE_MITEM_KEYBIND, "Toggle Auto Health", "key_menu_autohealth" },
    { FE_MITEM_KEYBIND, "Quick Save",         "key_menu_qsave"      },
    { FE_MITEM_KEYBIND, "End Game",           "key_menu_endgame"    },
    { FE_MITEM_KEYBIND, "Toggle Subtitles",   "key_menu_messages"   },
    { FE_MITEM_KEYBIND, "Quick Load",         "key_menu_qload"      },
    { FE_MITEM_KEYBIND, "Quit",               "key_menu_quit"       },
    { FE_MITEM_KEYBIND, "Adjust Gamma",       "key_menu_gamma"      },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardFuncs =
{
    optionsKeyboardFuncItems,
    arrlen(optionsKeyboardFuncItems),
    70,
    40,
    2,
    "Functions",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    false
};

// "kb_functions" command
void FE_CmdKeyboardFuncs(void)
{
    FE_PushMenu(&optionsKeyboardFuncs);
}

// Keyboard - Inventory ------------------------------------

static femenuitem_t optionsKeyboardInvItems[] =
{
    { FE_MITEM_KEYBIND, "Scroll Left",        "key_invLeft"   },
    { FE_MITEM_KEYBIND, "Scroll Right",       "key_invRight"  },
    { FE_MITEM_KEYBIND, "Scroll to Home",     "key_invHome"   },
    { FE_MITEM_KEYBIND, "Scroll to End",      "key_invEnd"    },
    { FE_MITEM_KEYBIND, "Use Current Item",   "key_invUse"    },
    { FE_MITEM_KEYBIND, "Drop Current Item",  "key_invDrop"   },
    { FE_MITEM_KEYBIND, "Query Current Item", "key_invquery"  },
    { FE_MITEM_KEYBIND, "Use Health Item",    "key_useHealth" },
    { FE_MITEM_KEYBIND, "View Status",        "key_invPop"    },
    { FE_MITEM_KEYBIND, "View Mission",       "key_mission"   },
    { FE_MITEM_KEYBIND, "View Keys",          "key_invKey"    },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardInv =
{
    optionsKeyboardInvItems,
    arrlen(optionsKeyboardInvItems),
    70,
    40,
    2,
    "Inventory",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    false
};

// "kb_inventory" command
void FE_CmdKeyboardInv(void)
{
    FE_PushMenu(&optionsKeyboardInv);
}

// Keyboard - Map ------------------------------------------

static femenuitem_t optionsKeyboardMapItems[] =
{
    { FE_MITEM_KEYBIND, "Toggle Map View", "key_map_toggle"    },
    { FE_MITEM_KEYBIND, "Scroll North",    "key_map_north"     },
    { FE_MITEM_KEYBIND, "Scroll South",    "key_map_south"     },
    { FE_MITEM_KEYBIND, "Scroll East",     "key_map_east"      },
    { FE_MITEM_KEYBIND, "Scroll West",     "key_map_west"      },
    { FE_MITEM_KEYBIND, "Zoom In",         "key_map_zoomin"    },
    { FE_MITEM_KEYBIND, "Zoom Out",        "key_map_zoomout"   },
    { FE_MITEM_KEYBIND, "Max Zoom",        "key_map_maxzoom"   },
    { FE_MITEM_KEYBIND, "Follow Mode",     "key_map_follow"    },
    { FE_MITEM_KEYBIND, "Mark Spot",       "key_map_mark"      },
    { FE_MITEM_KEYBIND, "Clear Last Mark", "key_map_clearmark" },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardMap =
{
    optionsKeyboardMapItems,
    arrlen(optionsKeyboardMapItems),
    70,
    40,
    2,
    "Automap",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    false
};

// "kb_map" command
void FE_CmdKeyboardMap(void)
{
    FE_PushMenu(&optionsKeyboardMap);
}

// Keyboard - Menus ----------------------------------------

static femenuitem_t optionsKeyboardMenuItems[] =
{
    { FE_MITEM_KEYBIND, "Toggle Menus",     "key_menu_activate"   },
    { FE_MITEM_KEYBIND, "Previous Item",    "key_menu_up"         },
    { FE_MITEM_KEYBIND, "Next Item",        "key_menu_down"       },
    { FE_MITEM_KEYBIND, "Decrease Value",   "key_menu_left"       },
    { FE_MITEM_KEYBIND, "Increase Value",   "key_menu_right"      },
    { FE_MITEM_KEYBIND, "Go Back",          "key_menu_back"       },
    { FE_MITEM_KEYBIND, "Activate Item",    "key_menu_forward"    },
    { FE_MITEM_KEYBIND, "Answer Yes",       "key_menu_confirm"    },
    { FE_MITEM_KEYBIND, "Answer No",        "key_menu_abort"      },
    { FE_MITEM_KEYBIND, "Screen Size Up",   "key_menu_incscreen"  },
    { FE_MITEM_KEYBIND, "Screen Size Down", "key_menu_decscreen"  },
    { FE_MITEM_KEYBIND, "Screenshot",       "key_menu_screenshot" },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardMenu =
{
    optionsKeyboardMenuItems,
    arrlen(optionsKeyboardMenuItems),
    70,
    40,
    2,
    "Menus",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    false
};

// "kb_menus" command
void FE_CmdKeyboardMenu(void)
{
    FE_PushMenu(&optionsKeyboardMenu);
}

// Keyboard - Movement -------------------------------------

static femenuitem_t optionsKeyboardMoveItems[] =
{
    { FE_MITEM_KEYBIND, "Move Forward",   "key_up"          },
    { FE_MITEM_KEYBIND, "Move Backward",  "key_down"        },
    { FE_MITEM_KEYBIND, "Strafe Left",    "key_strafeleft"  },
    { FE_MITEM_KEYBIND, "Strafe Right",   "key_straferight" },
    { FE_MITEM_KEYBIND, "Turn Left",      "key_left"        },
    { FE_MITEM_KEYBIND, "Turn Right",     "key_right"       },
    { FE_MITEM_KEYBIND, "Attack",         "key_fire"        },
    { FE_MITEM_KEYBIND, "Use / Activate", "key_use"         },
    { FE_MITEM_KEYBIND, "Strafe On",      "key_strafe"      },
    { FE_MITEM_KEYBIND, "Run",            "key_speed"       },
    { FE_MITEM_KEYBIND, "Jump",           "key_jump"        },
    { FE_MITEM_KEYBIND, "Look Up",        "key_lookUp"      },
    { FE_MITEM_KEYBIND, "Look Down",      "key_lookDown"    },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardMove =
{
    optionsKeyboardMoveItems,
    arrlen(optionsKeyboardMoveItems),
    70,
    32,
    2,
    "Movement",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    false
};

// "kb_movement" command
void FE_CmdKeyboardMove(void)
{
    FE_PushMenu(&optionsKeyboardMove);
}

// Keyboard - Weapons --------------------------------------
// Mmm, sounds like weapons! We can always use more firepower. -BlackBird

static femenuitem_t optionsKeyboardWeaponItems[] =
{
    { FE_MITEM_KEYBIND, "Punch Dagger",          "key_weapon1"    },
    { FE_MITEM_KEYBIND, "Crossbow",              "key_weapon2"    },
    { FE_MITEM_KEYBIND, "Assault Rifle",         "key_weapon3"    },
    { FE_MITEM_KEYBIND, "Mini-Missile Launcher", "key_weapon4"    },
    { FE_MITEM_KEYBIND, "Grenade Launcher",      "key_weapon5"    },
    { FE_MITEM_KEYBIND, "Flamethrower",          "key_weapon6"    },
    { FE_MITEM_KEYBIND, "Mauler",                "key_weapon7"    },
    { FE_MITEM_KEYBIND, "Sigil of the One God",  "key_weapon8"    },
    { FE_MITEM_KEYBIND, "Previous Weapon",       "key_prevweapon" },
    { FE_MITEM_KEYBIND, "Next Weapon",           "key_nextweapon" },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsKeyboardWeapons =
{
    optionsKeyboardWeaponItems,
    arrlen(optionsKeyboardWeaponItems),
    40,
    32,
    2,
    "Weapons",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "kb_weapons" command
void FE_CmdKeyboardWeapons(void)
{
    FE_PushMenu(&optionsKeyboardWeapons);
}

// Options - Mouse ----------------------------------------

static femenuitem_t optionsMouseItems[] =
{
    { FE_MITEM_SLIDER, "Sensitivity",       "mouse_sensitivity"  },
    { FE_MITEM_SLIDER, "Acceleration",      "mouse_acceleration" },
    { FE_MITEM_SLIDER, "Threshold",         "mouse_threshold"    },
    { FE_MITEM_TOGGLE, "Mouselook",         "novert", FE_FONT_SMALL, FE_TOGGLE_INVERT },
    { FE_MITEM_TOGGLE, "Invert Mouselook",  "mouse_invert", FE_FONT_SMALL },
    { FE_MITEM_CMD,    "Bind Buttons...",   "mbuttons" },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsMouse =
{
    optionsMouseItems,
    arrlen(optionsMouseItems),
    64,
    42,
    2,
    "Mouse Options",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "mouse" command
void FE_CmdMouse(void)
{
    FE_PushMenu(&optionsMouse);
}

// Options - Mouse - Buttons -------------------------------

static femenuitem_t optionsMouseButtonItems[] =
{
    { FE_MITEM_MBIND, "Attack",       "", FE_FONT_SMALL, FE_MVAR_FIRE        },
    { FE_MITEM_MBIND, "Use",          "", FE_FONT_SMALL, FE_MVAR_USE         },
    { FE_MITEM_MBIND, "Jump",         "", FE_FONT_SMALL, FE_MVAR_JUMP        },
    { FE_MITEM_MBIND, "Prev Weapon",  "", FE_FONT_SMALL, FE_MVAR_PREVWEAPON  },
    { FE_MITEM_MBIND, "Next Weapon",  "", FE_FONT_SMALL, FE_MVAR_NEXTWEAPON  },
    { FE_MITEM_MBIND, "Strafe On",    "", FE_FONT_SMALL, FE_MVAR_STRAFEON    },
    { FE_MITEM_MBIND, "Strafe Left",  "", FE_FONT_SMALL, FE_MVAR_STRAFELEFT  },
    { FE_MITEM_MBIND, "Strafe Right", "", FE_FONT_SMALL, FE_MVAR_STRAFERIGHT },
    { FE_MITEM_MBIND, "Forward",      "", FE_FONT_SMALL, FE_MVAR_FORWARD     },
    { FE_MITEM_MBIND, "Backward",     "", FE_FONT_SMALL, FE_MVAR_BACKWARD    },
    { FE_MITEM_END,   "", "" }
};

static femenu_t optionsMouseButtons =
{
    optionsMouseButtonItems,
    arrlen(optionsMouseButtonItems),
    64,
    42,
    2,
    "Mouse Buttons",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    false
};

// "mbuttons" command
void FE_CmdMouseButtons(void)
{
    FE_PushMenu(&optionsMouseButtons);
}

// Options - Graphics --------------------------------------

static femenuitem_t optionsGraphicsItems[] =
{
    { FE_MITEM_CMD, "Basic Settings", "gfxbasic",    FE_FONT_BIG },
    { FE_MITEM_CMD, "Lights",         "gfxlights",   FE_FONT_BIG },
    { FE_MITEM_CMD, "Sprites",        "gfxsprites",  FE_FONT_BIG },
    { FE_MITEM_CMD, "Advanced",       "gfxadvanced", FE_FONT_BIG },
    { FE_MITEM_END }
};

static femenu_t optionsGraphics =
{
    optionsGraphicsItems,
    arrlen(optionsGraphicsItems),
    70,
    55,
    2,
    "Graphics Options",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_SIGIL,
    0,
    false
};

// "graphics" command
void FE_CmdGraphics(void)
{
    FE_PushMenu(&optionsGraphics);
}

static femenuitem_t optionsGraphicsBasicItems[] = 
{
    { FE_MITEM_VIDMODE, "Resolution",       "screen_width"        },
    { FE_MITEM_TOGGLE,  "Fullscreen",       "fullscreen",         FE_FONT_SMALL, FE_TOGGLE_DEFAULT },
    { FE_MITEM_TOGGLE,  "High Quality",     "gl_enable_renderer", FE_FONT_SMALL, FE_TOGGLE_DEFAULT },
    { FE_MITEM_TOGGLE,  "Linear Filtering", "gl_linear_filtering" },
    { FE_MITEM_TOGGLE,  "Interpolation",    "interpolate_frames"  },
    { FE_MITEM_TOGGLE,  "Textured Automap", "gl_textured_automap" },  
    { FE_MITEM_SLIDER,  "Field of View",    "gl_fov"              },
    { FE_MITEM_END,     "", "" }
};

static femenu_t optionsGraphicsBasic =
{
    optionsGraphicsBasicItems,
    arrlen(optionsGraphicsBasicItems),
    60,
    40,
    2,
    "Graphics - Basic",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "gfxbasic" command
void FE_CmdGfxBasic(void)
{
    FE_PushMenu(&optionsGraphicsBasic);
}

static femenuitem_t optionsGraphicsLightItems[] =
{
    { FE_MITEM_TOGGLE, "Bloom",              "gl_enable_bloom"    },
    { FE_MITEM_SLIDER, "Bloom Threshold",    "gl_bloom_threshold" },
    { FE_MITEM_TOGGLE, "Dynamic Lights",     "gl_dynamic_lights"  },
    { FE_MITEM_VALUES, "Dynamic Light Type", "gl_dynamic_light_fast_blend" },
    { FE_MITEM_TOGGLE, "Lightmaps",          "gl_lightmaps",      FE_FONT_SMALL, FE_TOGGLE_DEFAULT },
    { FE_MITEM_TOGGLE, "Shading Effects",    "gl_wall_shades"     },
    { FE_MITEM_END,     "", "" }
};

static femenu_t optionsGraphicsLights =
{
    optionsGraphicsLightItems,
    arrlen(optionsGraphicsLightItems),
    60,
    40,
    2,
    "Graphics - Lights",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "gfxlights" command
void FE_CmdGfxLights(void)
{
    FE_PushMenu(&optionsGraphicsLights);
}

static femenuitem_t optionsGraphicsSpriteItems[] =
{
    { FE_MITEM_TOGGLE, "Decals",          "gl_decals"              },
    { FE_MITEM_SLIDER, "Max Decals",      "gl_max_decals"          },
    { FE_MITEM_TOGGLE, "Outline Items",   "gl_outline_sprites"     },
    { FE_MITEM_TOGGLE, "Sprite Clipping", "gl_fix_sprite_clipping" },
    { FE_MITEM_END,    "", "" }
};

static femenu_t optionsGraphicsSprites =
{
    optionsGraphicsSpriteItems,
    arrlen(optionsGraphicsSpriteItems),
    60,
    40,
    2,
    "Graphics - Sprites",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "gfxsprites" command
void FE_CmdGfxSprites(void)
{
    FE_PushMenu(&optionsGraphicsSprites);
}

static femenuitem_t optionsGraphicsAdvancedItems[] =
{
    { FE_MITEM_TOGGLE, "Force Frame Finish",  "gl_force_sync"             },
    { FE_MITEM_TOGGLE, "Fullscreen AA",       "gl_enable_fxaa"            },
    { FE_MITEM_TOGGLE, "Motion Blur",         "gl_enable_motion_blur"     },
    { FE_MITEM_SLIDER, "Motion Blur Samples", "gl_motion_blur_samples"    },
    { FE_MITEM_SLIDER, "Motion Blur Speed",   "gl_motion_blur_ramp_speed" },
    { FE_MITEM_END,     "", "" }
};

static femenu_t optionsGraphicsAdvanced =
{
    optionsGraphicsAdvancedItems,
    arrlen(optionsGraphicsAdvancedItems),
    40,
    40,
    2,
    "Graphics - Advanced",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "gfxadvanced" command
void FE_CmdGfxAdvanced(void)
{
    FE_PushMenu(&optionsGraphicsAdvanced);
}

// Options - Audio -----------------------------------------

static femenuitem_t optionsAudioItems[] =
{
    { FE_MITEM_SLIDER, "Sfx Volume",   "sfx_volume"      },
    { FE_MITEM_SLIDER, "Voice Volume", "voice_volume"    },
    { FE_MITEM_SLIDER, "Music Volume", "music_volume"    },
    { FE_MITEM_VALUES, "Music Type",   "snd_musicdevice" },
    { FE_MITEM_MUSIC,  "Music Test",   "fe_musicnum"     },
    { FE_MITEM_END, "", "" }
};

static femenu_t optionsAudio =
{
    optionsAudioItems,
    arrlen(optionsAudioItems),
    70,
    40,
    4,
    "Audio Options",
    FE_BG_RSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "audio" command
void FE_CmdAudio(void)
{
    FE_MusicTestSaveCurrent();
    FE_PushMenu(&optionsAudio);
}

boolean FE_InAudioMenu(void)
{
    return currentFEMenu == &optionsAudio;
}

// Options - About (The MOST IMPORTANT Menu!) --------------

static femenuitem_t optionsAboutItems[] =
{
    { FE_MITEM_CMD, "", "about2", FE_FONT_SMALL },
    { FE_MITEM_END, "",     "" }
};

enum
{
   CAT_PRODUCER,
   CAT_BY,
   CAT_PROGRAMMING1,
   CAT_PROGRAMMING2,
   CAT_GRAPHICS,
   CAT_QALEAD,
   CAT_QATEST,
   CAT_SPECIALTHANKS,
   NUMCATS // meow!
};

static const char *cat_strs[NUMCATS] =
{
    "Producer:",
    "Strife By:",
    "Code & Design:",
    "",
    "Artwork:",
    "QA Lead:",
    "QA Tester:",
    "Special Thanks:",
};

static const char *val_strs[NUMCATS] =
{
    "Stephen Kick",
    "Rogue Entertainment",
    "Samuel Villarreal",
    "James Haley",
    "Sven Ruthner",
    "Daniel Grayshon",
    "Casey Day",
    "Simon Howard",
};

static void FE_DrawAboutMenu(void)
{
    static int cat_width = -1, val_width = -1, line_x;
    int i, y;
    
    FE_DrawMainMenu();

    if(cat_width == -1)
    {
        // determine widest category string
        int w;

        for(i = 0; i < NUMCATS; i++)
        {
            w = M_StringWidth(cat_strs[i]);
            if(w > cat_width)
                cat_width = w;
        }

        // determine widest value string
        for(i = 0; i < NUMCATS; i++)
        {
            w = M_StringWidth(val_strs[i]);
            if(w > val_width)
                val_width = w;
        }

        // determine line position
        line_x = ((SCREENWIDTH - (cat_width + val_width + 8)) >> 1) - 8;
    }

    y = 82;

    // draw info categories
    for(i = 0; i < NUMCATS; i++)
    {
        int catStrWidth = M_StringWidth(cat_strs[i]);
        M_WriteText(line_x + (cat_width - catStrWidth), y, cat_strs[i]);
        M_WriteText(line_x + cat_width + 10, y, val_strs[i]);

        y += 11;
    }

    y += 11;

    FE_WriteSmallTextCentered(y, "Copyright 2014 Night Dive Studios");
}

static femenu_t optionsMenuAbout =
{
    optionsAboutItems,
    arrlen(optionsAboutItems),
    144,
    188,
    52,
    "Veteran Edition",
    FE_BG_SIGIL,
    FE_DrawAboutMenu,
    FE_CURSOR_NONE,
    0
};

// "about" command
void FE_CmdAbout(void)
{
    FE_PushMenu(&optionsMenuAbout);
}

//
// Page 2
//

static femenuitem_t optionsAboutItems2[] =
{
    { FE_MITEM_CMD, "", "back", FE_FONT_SMALL },
    { FE_MITEM_END, "",     "" }
};

// NB: These are legally required notices in the case of FFmpeg and SDL.
// Others are courtesy to end user and to the Chocolate Doom team.
static const char *licText =
"The \"Strife: Veteran Edition\" program code is Free Software available under the "
"GNU GPL v2.0. See the SRCLICENSE.txt file in your Steam directory for details.\n\n"
"Based on \"Chocolate Strife\", (c) 2014 Simon Howard et al.\n\n"
"FFmpeg is used under terms of the LGPL 2.1\n"
"See https://www.ffmpeg.org\n\n"
"SDL is used under terms of the LGPL 2.1\n"
"See https://www.libsdl.org"; 
static char *licCopy;

static void FE_DrawAboutMenu2(void)
{
    if(!licCopy)
    {
        licCopy = M_Strdup(licText);
        M_DialogDimMsg(30, 30, licCopy, true);
    }

    HUlib_drawYellowText(30, 30, licCopy, true);
}

static femenu_t optionsMenuAbout2 =
{
    optionsAboutItems2,
    arrlen(optionsAboutItems2),
    144,
    188,
    2,
    "Legal",
    FE_BG_SIGIL,
    FE_DrawAboutMenu2,
    FE_CURSOR_NONE,
    0
};

// "about2" command
void FE_CmdAbout2(void)
{
    currentFEMenu           = &optionsMenuAbout2;
    currentFEMenu->prevMenu = optionsMenuAbout.prevMenu;
    frontend_wipe  = true;
    frontend_sgcount = 20;
    FE_MenuChangeSfx();
}

// Multiplayer Menu ----------------------------------------

static femenuitem_t multiMenuItems[] =
{
    { FE_MITEM_CMD, "Create Public Lobby",  "newpublobby",  FE_FONT_BIG },
    { FE_MITEM_CMD, "Create Private Lobby", "newprivlobby", FE_FONT_BIG },
    { FE_MITEM_CMD, "Lobby Search",         "lobbylist",    FE_FONT_BIG },
    { FE_MITEM_END, "", "" }
};

static femenu_t multiMenu =
{
    multiMenuItems,
    arrlen(multiMenuItems),
    40,
    60,
    2,
    "Multiplayer",
    FE_BG_TSKULL,
    NULL,
    FE_CURSOR_SIGIL,
    0,
    true
};

// "multi" command
void FE_CmdMulti(void)
{
    FE_PushMenu(&multiMenu);
}

// Lobby Server Menu ---------------------------------------
//
// This menu is used when the player creates a lobby and will 
// become the game server.
//

static void FE_DrawLobbyUsers(void)
{
    FE_DrawLobbyUserList(60, 24);
}

static femenuitem_t lobbyServerMenuItems[] =
{
    { FE_MITEM_CMD,     "Invite Friends", "invite"     },
    { FE_MITEM_CMD,     "Game Settings",  "mpoptions"  },
    { FE_MITEM_MPTEAM,  "Set CTC Team",   "setteam"    },
    { FE_MITEM_MPREADY, "Change State",   "setready"   },
    { FE_MITEM_CMD,     "Start Game",     "startgame"  },
    { FE_MITEM_CMD,     "Leave Lobby",    "leavelobby" },
    { FE_MITEM_END,     "", "" }
};

static femenu_t lobbyServerMenu =
{
    lobbyServerMenuItems,
    arrlen(lobbyServerMenuItems),
    64,
    120,
    2,
    "Multiplayer Lobby",
    FE_BG_TSKULL,
    FE_DrawLobbyUsers,
    FE_CURSOR_LASER,
    0,
    false,
    true  // requires forced exit
};

// "lobbysrv" command
void FE_CmdLobbySrv(void)
{
    FE_PushMenu(&lobbyServerMenu);
}

// Lobby Server Menu - Game Settings -----------------------

static femenuitem_t lobbyServerGameSettingItems[] =
{
    { FE_MITEM_TOGGLE,  "Fast Monsters",    "fastparm"    },
    { FE_MITEM_VALUES,  "Game Type",        "deathmatch"  },
    { FE_MITEM_TOGGLE,  "Machines Respawn", "respawnparm" },
    { FE_MITEM_TOGGLE,  "No Monsters",      "nomonsters"  },
    { FE_MITEM_TOGGLE,  "Randomize Items",  "randomparm"  },
    { FE_MITEM_VALUES,  "Skill Level",      "startskill"  },
    { FE_MITEM_VALUES,  "Start on Map",     "startmap"    },
    { FE_MITEM_GAP,     "",                 "",           FE_FONT_SMALL, 12 },
    { FE_MITEM_SLIDER,  "Time Limit",       "timelimit"   },
    { FE_MITEM_END, "", "" }
};

static femenu_t lobbyServerGameSettingsMenu =
{
    lobbyServerGameSettingItems,
    arrlen(lobbyServerGameSettingItems),
    15,
    40,
    2,
    "Game Settings",
    FE_BG_TSKULL,
    NULL,
    FE_CURSOR_LASER,
    0,
    true
};

// "mpoptions" command
void FE_CmdMPOptions(void)
{
    FE_PushMenu(&lobbyServerGameSettingsMenu);
}

// Lobby Client Menu ---------------------------------------
//
// This menu is used when the player joins a lobby and will 
// become a game client.
//

static femenuitem_t lobbyClientMenuItems[] =
{
    { FE_MITEM_MPTEAM,  "Set CTC Team", "setteam"    },
    { FE_MITEM_MPREADY, "Change State", "setready"   },
    { FE_MITEM_CMD,     "Leave Lobby",  "leavelobby" },
    { FE_MITEM_END, "", "" }
};

static femenu_t lobbyClientMenu =
{
    lobbyClientMenuItems,
    arrlen(lobbyClientMenuItems),
    64,
    120,
    2,
    "Multiplayer Lobby",
    FE_BG_TSKULL,
    FE_DrawLobbyUsers,
    FE_CURSOR_LASER,
    0,
    false,
    true  // requires forced exit
};

// "lobbyclient" command
void FE_CmdLobbyClient(void)
{
    FE_PushMenu(&lobbyClientMenu);
}

//
// Test if user is in client lobby
//
boolean FE_InClientLobby(void)
{
    return currentFEMenu == &lobbyClientMenu;
}

// EOF

