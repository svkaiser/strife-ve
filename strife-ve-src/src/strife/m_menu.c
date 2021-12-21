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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//


#include <stdlib.h>
#include <ctype.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "dstrings.h"

#include "d_main.h"
#include "deh_main.h"

#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "i_joystick.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"

#include "r_local.h"


#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_help.h"
#include "m_saves.h"    // [STRIFE]
#include "p_saveg.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"
#include "p_dialog.h"

// [SVE]
#include "fe_menuengine.h"
#include "fe_frontend.h"
#include "fe_graphics.h"
#include "i_social.h"
#include "i_softkey.h"

// declared here because the rb_** headers causes msvc to throw a big fuss
void RB_InitExtraHudTextures(void);
void RB_DeleteDoomData(void);
void RB_SetPatchBufferPalette(void);

extern void M_QuitStrife(int);

extern patch_t*         hu_font[HU_FONTSIZE];

extern boolean          chat_on;        // in heads-up code
extern boolean          sendsave;       // [STRIFE]

//
// defaulted values
//
int			mouseSensitivityX = 5;
int         mouseSensitivityY = 5;

// Edward [SVE]:
float		joy_gyrosensitivityh = 0.8f;
float		joy_gyrosensitivityv = 0.5f;
int		    joy_gyroscope = 1;
int         joy_gyrostyle = 0;

// [STRIFE]: removed this entirely
// Show messages has default, 0 = off, 1 = on
//int			showMessages = 1;
	

// Blocky mode, has default, 0 = high, 1 = normal
int			detailLevel = 0;
int			screenblocks = 10; // [SVE]: default 10

// temp for screenblocks (0-9)
int			screenSize;

// -1 = no quicksave slot picked!
int			quickSaveSlot;

 // 1 = message to be printed
int			messageToPrint;
// ...and here is the message string!
const char*			messageString;

// Edward [SVE]: print help text
int         messageHelp;

// message x & y
int			messx;
int			messy;
int			messageLastMenuActive;
boolean                 messageLastMenuPause; // haleyjd [SVE]: forgotten by Rogue

// timed message = no input from user
boolean			messageNeedsInput;

void    (*messageRoutine)(int response);

char gammamsg[5][26] =
{
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4
};

// we are going to be entering a savegame string
int			saveStringEnter;              
int             	saveSlot;	// which slot to save in
int			saveCharIndex;	// which char we're editing
// old save description before edit
char			saveOldString[SAVESTRINGSIZE];  

boolean                 inhelpscreens;
boolean                 menuactive;
boolean                 menupause;      // haleyjd 08/29/10: [STRIFE] New global
int                     menupausetime;  // haleyjd 09/04/10: [STRIFE] New global
boolean                 menuindialog;   // haleyjd 09/04/10: ditto

// haleyjd 08/27/10: [STRIFE] SKULLXOFF == -28, LINEHEIGHT == 19
#define CURSORXOFF		-28
#define LINEHEIGHT		19

extern boolean		sendpause;
char			savegamestrings[10][SAVESTRINGSIZE];

char	endstring[160];

// haleyjd 09/04/10: [STRIFE] Moved menuitem / menu structures into header
// because they are needed externally by the dialog engine.

// haleyjd 08/27/10: [STRIFE] skull* stuff changed to cursor* stuff
short		itemOn;			// menu item skull is on
short		cursorAnimCounter;	// skull animation counter
short		whichCursor;		// which skull to draw

// graphic name of cursors
// haleyjd 08/27/10: [STRIFE] M_SKULL* -> M_CURS*
char    *cursorName[8] = {"M_CURS1", "M_CURS2", "M_CURS3", "M_CURS4", 
                          "M_CURS5", "M_CURS6", "M_CURS7", "M_CURS8" };

// haleyjd 20110210 [STRIFE]: skill level for menus
int menuskill;
int menuepisode; // [SVE]: allow starting demo maps from menus

// current menudef
menu_t*	currentMenu;                          

// haleyjd 03/01/13: [STRIFE] v1.31-only:
// Keeps track of whether the save game menu is being used to name a new
// character slot, or to just save the current game. In the v1.31 disassembly
// this was the new dword_8632C variable.
boolean namingCharacter; 

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_StartCast(int choice); // [SVE]
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_ReadThis3(int choice); // [STRIFE]

//void M_ChangeMessages(int choice); [STRIFE]
void M_ChangeSensitivity(int choice);
void M_SfxVol(int choice);
void M_VoiceVol(int choice); // [STRIFE]
void M_MusicVol(int choice);
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

//void M_FinishReadThis(int choice); - [STRIFE] unused
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
/*
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawReadThis3(void); // [STRIFE]
*/
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x,int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
int  M_StringWidth(const char *string);
int  M_StringHeight(const char *string);
void M_StartMessage(const char *string,void *routine,boolean input);
void M_StopMessage(void);

//
// M_AdvancedOptions
//
// haleyjd 20141102: [SVE] Access to frontend options menu from in-game.
//
void M_AdvancedOptions(int choice)
{
    FE_StartInGameOptionsMenu();
}

//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
#ifndef SVE_PLAT_SWITCH
    readthis,
    quitdoom,
#endif
    main_end
} main_e;

menuitem_t MainMenu[]=
{
    {1,"M_NGAME",M_NewGame,'n'},
    {1,"M_OPTION",M_Options,'o'},
    {1,"M_LOADG",M_LoadGame,'l'},
    {1,"M_SAVEG",M_SaveGame,'s'},
    // Another hickup with Special edition.
#ifndef SVE_PLAT_SWITCH
    {1,"M_RDTHIS",M_ReadThis,'h'}, // haleyjd 08/28/10: 'r' -> 'h'
    {1,"M_QUITG",M_QuitStrife,'q'}
#endif
};

menu_t  MainDef =
{
    main_end,
    NULL,
    MainMenu,
    M_DrawMainMenu,
    97,45, // haleyjd 08/28/10: [STRIFE] changed y coord
    0
};

//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[]=
{
    // haleyjd 08/28/10: [STRIFE] changed all shortcut letters
    {1,"M_JKILL",   M_ChooseSkill, 't'},
    {1,"M_ROUGH",   M_ChooseSkill, 'r'},
    {1,"M_HURT",    M_ChooseSkill, 'v'},
    {1,"M_ULTRA",   M_ChooseSkill, 'e'},
    {1,"M_NMARE",   M_ChooseSkill, 'b'}
};

menu_t  NewDef =
{
    newg_end,           // # of menu items
    &MainDef,           // previous menu - haleyjd [STRIFE] changed to MainDef
    NewGameMenu,        // menuitem_t ->
    M_DrawNewGame,      // drawing routine ->
    48,63,              // x,y
    toorough            // lastOn - haleyjd [STRIFE]: default to skill 1
};

//
// EPISODE SELECT
//
// haleyjd [SVE] 20141014: Restored to allow selecting the demo episode
// from the menus
//

enum
{
    ep1,
    ep2,
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[]=
{
    {1, "Quest for the Sigil", M_Episode, 'q'},
    {1, "Trust No One (Demo)", M_Episode, 't'},
};

menu_t  EpiDef =
{
    ep_end,		// # of menu items
    &NewDef,		// previous menu
    EpisodeMenu,	// menuitem_t ->
    M_DrawEpisode,	// drawing routine ->
    44,63,              // x,y
    ep1			// lastOn
};

//
// OPTIONS MENU
//
enum
{
    // haleyjd 08/28/10: [STRIFE] Removed messages, mouse sens., detail.
    endgame,
    scrnsize,
    option_empty1,
    soundvol,
    advancedopts,
    startcast,
    opt_end
} options_e;

menuitem_t OptionsMenu[]=
{
    // haleyjd 08/28/10: [STRIFE] Removed messages, mouse sens., detail.
    { 1, "M_ENDGAM",  M_EndGame,         'e'  },
    { 2, "M_SCRNSZ",  M_SizeDisplay,     's'  },
    {-1, "",          NULL,              '\0' },
    { 1, "M_SVOL",    M_Sound,           's'  },
    { 1, "Advanced",  M_AdvancedOptions, 'a'  }, // [SVE]
    { 1, "Show Cast", M_StartCast,       's'  }, // [SVE]
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,37,
    0
};

/*
//
// Read This! MENU 1 & 2 & [STRIFE] 3
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
{
    {1,"",M_ReadThis2,0}
};

menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[]=
{
    {1,"",M_ReadThis3,0} // haleyjd 08/28/10: [STRIFE] Go to ReadThis3
};

menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    250,185, // haleyjd 08/28/10: [STRIFE] changed coords
    0
};

// haleyjd 08/28/10: Added Read This! menu 3
enum
{
    rdthsempty3,
    read3_end
} read_e3;

menuitem_t ReadMenu3[]=
{
    {1,"",M_ClearMenus,0}
};

menu_t  ReadDef3 =
{
    read3_end,
    &ReadDef2,
    ReadMenu3,
    M_DrawReadThis3,
    250, 185,
    0
};
*/

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    voice_vol,
    sfx_empty3,
    sound_end
} sound_e;

// haleyjd 08/29/10:
// [STRIFE] 
// * Added voice volume
// * Moved mouse sensitivity here (who knows why...)
menuitem_t SoundMenu[]=
{
    {2,"M_SFXVOL",M_SfxVol,'s'},
    {-1,"",0,'\0'},
    {2,"M_MUSVOL",M_MusicVol,'m'},
    {-1,"",0,'\0'},
    {2,"M_VOIVOL",M_VoiceVol,'v'}, 
    {-1,"",0,'\0'},
};

menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    80,35,       // [STRIFE] changed y coord 64 -> 35
    0
};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
} load_e;

menuitem_t LoadMenu[]=
{
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'}
};

menu_t  LoadDef =
{
    load_end,
    &MainDef,
    LoadMenu,
    M_DrawLoad,
    80,54,
    0
};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[]=
{
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'}
};

menu_t  SaveDef =
{
    load_end,
    &MainDef,
    SaveMenu,
    M_DrawSave,
    80,54,
    0
};

void M_DrawNameChar(void);

//
// NAME CHARACTER MENU
//
// [STRIFE]
// haleyjd 20110210: New "Name Your Character" Menu
//
menu_t NameCharDef =
{
    load_end,
    &NewDef,
    SaveMenu,
    M_DrawNameChar,
    80,54,
    0
};


//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
// [STRIFE]
// haleyjd 20110210: Rewritten to read "name" file in each slot directory
//
void M_ReadSaveStrings(void)
{
    FILE *handle;
    int   i;
    char *fname = NULL;

    for(i = 0; i < load_end; i++)
    {
        if(fname)
            Z_Free(fname);
        fname = M_SafeFilePath(savegamedir, M_MakeStrifeSaveDir(i, "\\name"));

        handle = fopen(fname, "rb");
        if(handle == NULL)
        {
            M_StringCopy(savegamestrings[i], EMPTYSTRING,
                         sizeof(savegamestrings[i]));
            LoadMenu[i].status = 0;
            continue;
        }
        fread(savegamestrings[i], 1, SAVESTRINGSIZE, handle);
        fclose(handle);
        LoadMenu[i].status = 1;
    }

    if(fname)
        Z_Free(fname);
}

//
// M_DrawNameChar
//
// haleyjd 09/22/10: [STRIFE] New function
// Handler for drawing the "Name Your Character" menu.
//
void M_DrawNameChar(void)
{
    int i;

    M_WriteText(72, 28, DEH_String("Name Your Character"));

    for (i = 0;i < load_end; i++)
    {
        // [SVE]
        NameCharDef.menuitems[i].x = LoadDef.x - 10;
        NameCharDef.menuitems[i].y = (LoadDef.y + LINEHEIGHT*i) - 4;
        NameCharDef.menuitems[i].w = 209;
        NameCharDef.menuitems[i].h = LINEHEIGHT - 4;

        M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
        M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }

    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[quickSaveSlot]);
        M_WriteText(LoadDef.x + i,LoadDef.y+LINEHEIGHT*quickSaveSlot,"_");
    }
}

//
// M_DoNameChar
//
// haleyjd 09/22/10: [STRIFE] New function
// Handler for items in the "Name Your Character" menu.
//
void M_DoNameChar(int choice)
{
    int map;

    // 20130301: clear naming character flag for 1.31 save logic
    if(gameversion == exe_strife_1_31)
        namingCharacter = false;
    sendsave = 1;
    ClearTmp();
    G_WriteSaveName(choice, savegamestrings[choice]);
    quickSaveSlot = choice;  
    SaveDef.lastOn = choice;
    ClearSlot();
    FromCurr();
    
    if(menuepisode || isdemoversion) // [SVE]: allow demo episode select
        map = 33;
    else
        map = 2;

    G_DeferedInitNew(menuskill, map);
    M_ClearMenus(0);
}

//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
    int             i;

    V_DrawPatchDirect(72, 28, 
                      W_CacheLumpName(DEH_String("M_LOADG"), PU_CACHE));

    for (i = 0;i < load_end; i++)
    {
        // [SVE]
        LoadDef.menuitems[i].x = LoadDef.x - 10;
        LoadDef.menuitems[i].y = (LoadDef.y + LINEHEIGHT*i) - 4;
        LoadDef.menuitems[i].w = 209;
        LoadDef.menuitems[i].h = LINEHEIGHT - 4;

        M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
        M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }

	FE_NX_DrawToolTips(4);
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
    int             i;

    V_DrawPatchDirect(x - 8, y + 7,
                      W_CacheLumpName(DEH_String("M_LSLEFT"), PU_CACHE));

    for (i = 0;i < 24;i++)
    {
        V_DrawPatchDirect(x, y + 7,
                          W_CacheLumpName(DEH_String("M_LSCNTR"), PU_CACHE));
        x += 8;
    }

    V_DrawPatchDirect(x, y + 7, 
                      W_CacheLumpName(DEH_String("M_LSRGHT"), PU_CACHE));
}



//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    // [STRIFE]: completely rewritten
    char *name = NULL;

    G_WriteSaveName(choice, savegamestrings[choice]);
    ToCurr();

    // use safe & portable filepath concatenation for Choco
    name = M_SafeFilePath(savegamedir, M_MakeStrifeSaveDir(choice, ""));

    G_ReadCurrent(name);
    quickSaveSlot = choice;
    M_ClearMenus(0);

    Z_Free(name);
}

//
// Selected from DOOM menu
//
// [STRIFE] Verified unmodified
//
void M_LoadGame (int choice)
{
    if (netgame)
    {
        M_StartMessage(DEH_String(i_seejoysticks ? LOADNETGP : LOADNET), NULL, false);
        return;
    }

    // Edward 20200710: Make absolutly sure load game clears the namingCharacter flag.
    if (gameversion == exe_strife_1_31)
        namingCharacter = false;

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}


//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
    int             i;

    V_DrawPatchDirect(72, 28, W_CacheLumpName(DEH_String("M_SAVEG"), PU_CACHE));
    for (i = 0;i < load_end; i++)
    {
        // [SVE]
        SaveDef.menuitems[i].x = SaveDef.x - 10;
        SaveDef.menuitems[i].y = (SaveDef.y + LINEHEIGHT*i) - 4;
        SaveDef.menuitems[i].w = 209;
        SaveDef.menuitems[i].h = LINEHEIGHT - 4;

        M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
        M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }

    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[quickSaveSlot]);
        M_WriteText(LoadDef.x + i,LoadDef.y+LINEHEIGHT*quickSaveSlot,"_");
    }
	 
	FE_NX_DrawToolTips(4);
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    // [STRIFE]: completely rewritten
    if(slot >= 0)
    {
        sendsave = 1;
        G_WriteSaveName(slot, savegamestrings[slot]);
        M_ClearMenus(0);
        quickSaveSlot = slot;        
        // haleyjd 20130922: slight divergence. We clear the destination slot 
        // of files here, which vanilla did not do. As a result, 1.31 had 
        // broken save behavior to the point of unusability. fraggle agrees 
        // this is detrimental enough to be fixed - unconditionally, for now.
        ClearSlot();        
        FromCurr();
    }
    else
        M_StartMessage(DEH_String(i_seejoysticks ? QSAVESPOTGP : QSAVESPOT), NULL, false);
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;

    // [STRIFE]
    quickSaveSlot = choice;
    //saveSlot = choice;

    M_StringCopy(saveOldString, savegamestrings[choice], sizeof(saveOldString));
    if (!strcmp(savegamestrings[choice],EMPTYSTRING))
    {
        savegamestrings[choice][0] = '\0';

#if defined(LUNA_RELEASE)
        // [SVE] Allow creating an automatic save name for Luna
        if (i_seejoysticks)
        {
            snprintf(savegamestrings[choice], SAVESTRINGSIZE, "MERCENARY %d", choice + 1);
        }
#endif
    }
    saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
{
    // [STRIFE]
    if (netgame)
    {
        // haleyjd 20110211: Hooray for Rogue's awesome multiplayer support...
        // [SVE]: added PRESSKEY here
        M_StartMessage(DEH_String(i_seejoysticks? NETNOSAVEGP : NETNOSAVE), NULL, false);
        return;
    }
    if (!usergame)
    {
        M_StartMessage(DEH_String(i_seejoysticks ? SAVEDEADGP : SAVEDEAD),NULL,false);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    // [SVE]: no saving when dead
    if(players[consoleplayer].health <= 0 || players[consoleplayer].cheats & CF_ONFIRE)
    {
        M_StartMessage(DEH_String("You can't save when you're dead!\n\n"
                                  "(Press \"Use\" to respawn)"), NULL, false);
        return;
    }

    // [STRIFE]
    if(gameversion == exe_strife_1_31)
    {
        // Edward 20200710: Make absolutly sure save game clears the namingCharacter flag.
        namingCharacter = false;

        // haleyjd 20130301: in 1.31, we can choose a slot again.
        M_SetupNextMenu(&SaveDef);
        M_ReadSaveStrings();
    }
    else
    {
        // In 1.2 and lower, you save over your character slot exclusively
        M_ReadSaveStrings();
        M_DoSave(quickSaveSlot);
    }
}



//
//      M_QuickSave
//
char    tempstring[80];

void M_QuickSaveResponse(int key)
{
    if (key == key_menu_confirm)
    {
        M_DoSave(quickSaveSlot);
        S_StartSound(NULL, sfx_mtalht); // [STRIFE] sound
    }
}

void M_QuickSave(void)
{
    if (netgame)
    {
        // haleyjd 20110211 [STRIFE]: More fun...
        // [SVE]: added PRESSKEY here
        M_StartMessage(DEH_String(i_seejoysticks ? NETNOSAVEGP : NETNOSAVE), NULL, false);
        return;
    }

    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    // [SVE]: no saving when dead
    if(players[consoleplayer].health <= 0 || players[consoleplayer].cheats & CF_ONFIRE)
    {
        M_StartMessage(DEH_String("You can't save when you're dead!\n\n"
                                  "(Press \"Use\" to respawn)"), NULL, false);
        return;
    }

    if (quickSaveSlot < 0)
    {
        M_StartControlPanel();
        M_ReadSaveStrings();
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2;	// means to pick a slot now
        return;
    }
    DEH_snprintf(tempstring, 80, i_seejoysticks ? QSPROMPTGP : QSPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring,M_QuickSaveResponse,true);
}



//
// M_QuickLoadResponse
//
void M_QuickLoadResponse(int key)
{
    if (key == key_menu_confirm)
    {
        M_LoadSelect(quickSaveSlot);
        S_StartSound(NULL, sfx_mtalht); // [STRIFE] sound
    }
}

//
// M_QuickLoad
//
// [STRIFE] Verified unmodified
//
void M_QuickLoad(void)
{
    if (netgame)
    {
        M_StartMessage(DEH_String(i_seejoysticks ? QLOADNETGP : QLOADNET),NULL,false);
        return;
    }

    if (quickSaveSlot < 0)
    {
        M_StartMessage(DEH_String(i_seejoysticks ? QSAVESPOTGP : QSAVESPOT),NULL,false);
        return;
    }
    DEH_snprintf(tempstring, 80, i_seejoysticks ? QLPROMPTGP : QLPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring,M_QuickLoadResponse,true);
}



/*
//
// Read This Menus
// Had a "quick hack to fix romero bug"
// haleyjd 08/28/10: [STRIFE] Draw HELP1, unconditionally.
//
void M_DrawReadThis1(void)
{
    inhelpscreens = true;

    V_DrawPatchDirect (0, 0, W_CacheLumpName(DEH_String("HELP1"), PU_CACHE));
}



//
// Read This Menus
// haleyjd 08/28/10: [STRIFE] Not optional, draws HELP2
//
void M_DrawReadThis2(void)
{
    inhelpscreens = true;

    V_DrawPatchDirect(0, 0, W_CacheLumpName(DEH_String("HELP2"), PU_CACHE));
}


//
// Read This Menus
// haleyjd 08/28/10: [STRIFE] New function to draw HELP3.
//
void M_DrawReadThis3(void)
{
    inhelpscreens = true;
    
    V_DrawPatchDirect(0, 0, W_CacheLumpName(DEH_String("HELP3"), PU_CACHE));
}
*/

//
// Change Sfx & Music volumes
//
// haleyjd 08/29/10: [STRIFE]
// * Changed title graphic coordinates
// * Added voice volume and sensitivity sliders
//
void M_DrawSound(void)
{
    V_DrawPatchDirect (100, 10, W_CacheLumpName(DEH_String("M_SVOL"), PU_CACHE));

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1),
                 16,sfxVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1),
                 16,musicVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(voice_vol+1),
                 16,voiceVolume);
 
	FE_NX_DrawToolTips(4);
}

void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(int choice)
{
    switch(choice)
    {
    case 0:
        sfxVolume--;
        if(sfxVolume < 0)
            sfxVolume = 15;
        break;
    case 1:
        sfxVolume++;
        if(sfxVolume > 15)
            sfxVolume = 0;
        break;
    }

    S_SetSfxVolume(sfxVolume * 8);
}

//
// M_VoiceVol
//
// haleyjd 08/29/10: [STRIFE] New function
// Sets voice volume level.
//
void M_VoiceVol(int choice)
{
    switch(choice)
    {
    case 0:
        voiceVolume--;
        if(voiceVolume < 0)
            voiceVolume = 15;
        break;
    case 1:
        voiceVolume++;
        if(voiceVolume > 15)
            voiceVolume = 0;
        break;
    }

    S_SetVoiceVolume(voiceVolume * 8);
}

void M_MusicVol(int choice)
{
    switch(choice)
    {
    case 0:
        musicVolume--;
        if(musicVolume < 0)
            musicVolume = 15;
        break;
    case 1:
        musicVolume++;
        if(musicVolume > 15)
            musicVolume = 0;
        break;
    }

    S_SetMusicVolume(musicVolume * 8);
}

//
// M_DrawMainMenu
//
// haleyjd 08/27/10: [STRIFE] Changed x coordinate; M_DOOM -> M_STRIFE
//
void M_DrawMainMenu(void)
{
    V_DrawPatchDirect(84, 2,
                      W_CacheLumpName(DEH_String("M_STRIFE"), PU_CACHE));

    if (currentMenu->prevMenu == NULL)
    {
        FE_NX_DrawToolTips(2);
    }
    else
    {
        FE_NX_DrawToolTips(4);
    }
}




//
// M_NewGame
//
// haleyjd 08/31/10: [STRIFE] Changed M_NEWG -> M_NGAME
//
void M_DrawNewGame(void)
{
    V_DrawPatchDirect(96, 14, W_CacheLumpName(DEH_String("M_NGAME"), PU_CACHE));
    V_DrawPatchDirect(54, 38, W_CacheLumpName(DEH_String("M_SKILL"), PU_CACHE));
 
	FE_NX_DrawToolTips(4);
 
}

void M_NewGame(int choice)
{
    if (netgame && !demoplayback)
    {
        M_StartMessage(DEH_String(i_seejoysticks ? NEWGAMEGP : NEWGAME),NULL,false);
        return;
    }
    // haleyjd 09/07/10: [STRIFE] Removed Chex Quest and DOOM gamemodes
    if(gameversion == exe_strife_1_31)
       namingCharacter = true; // for 1.31 save logic
    M_SetupNextMenu(&NewDef);
 
}


//
//      M_Episode
//

// haleyjd: [STRIFE] Unused
// haleyjd 20141014: [SVE] restored for allowing selection of demo maps

void M_DrawEpisode(void)
{
    V_WriteBigText("Choose Campaign", 54, 38);

	FE_NX_DrawToolTips(4);
}

void M_ChooseSkill(int choice)
{
    // haleyjd 09/07/10: Removed nightmare confirmation
    menuskill = choice;
    M_SetupNextMenu(&EpiDef); // [SVE]: episode menu
 
}

// haleyjd [STRIFE] Unused
// haleyjd 20141014: [SVE] restored for allowing selection of demo maps
void M_Episode(int choice)
{
    // [STRIFE]: start "Name Your Character" menu
    // [SVE]: moved to after episode selection
    menuepisode = choice;
    currentMenu = &NameCharDef;
    itemOn = NameCharDef.lastOn;
    M_ReadSaveStrings();
}

//
// M_Options
//
char    detailNames[2][9]	= {"M_GDHIGH","M_GDLOW"};
char	msgNames[2][9]		= {"M_MSGOFF","M_MSGON"};


void M_DrawOptions(void)
{
    // haleyjd 08/27/10: [STRIFE] M_OPTTTL -> M_OPTION
    V_DrawPatchDirect(108, 15, 
                      W_CacheLumpName(DEH_String("M_OPTION"), PU_CACHE));

    // haleyjd 08/26/10: [STRIFE] Removed messages, sensitivity, detail.

    // [SVE]: only range of 7 -> 8 is allowed
    M_DrawThermo(OptionsDef.x,OptionsDef.y+LINEHEIGHT*(scrnsize+1),
                 9, screenSize == 7 ? 0 : 8);
 
	FE_NX_DrawToolTips(4);
}

void M_Options(int choice)
{
#ifndef SVE_PLAT_SWITCH
    // haleyjd 20141031: [SVE] make cast call context sensitive
    if(usergame)
        OptionsDef.numitems = opt_end - 1;
    else
        OptionsDef.numitems = opt_end;
    if(OptionsDef.lastOn >= OptionsDef.numitems)
        OptionsDef.lastOn = 0;
#endif

    M_SetupNextMenu(&OptionsDef);
}

//
// M_AutoUseHealth
//
// [STRIFE] New function
// haleyjd 20110211: toggle autouse health state
//
void M_AutoUseHealth(void)
{
    if(!netgame && usergame)
    {
        players[consoleplayer].cheats ^= CF_AUTOHEALTH;

        if(players[consoleplayer].cheats & CF_AUTOHEALTH)
            players[consoleplayer].message = DEH_String("Auto use health ON");
        else
            players[consoleplayer].message = DEH_String("Auto use health OFF");
    }
}

//
// M_ChangeShowText
//
// [STRIFE] New function
//
void M_ChangeShowText(void)
{
    dialogshowtext ^= true;

    if(dialogshowtext)
        players[consoleplayer].message = DEH_String("Conversation Text On");
    else
        players[consoleplayer].message = DEH_String("Conversation Text Off");
}

//
//      Toggle messages on/off
//
// [STRIFE] Messages cannot be disabled in Strife
/*
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;

    if (!showMessages)
        players[consoleplayer].message = DEH_String(MSGOFF);
    else
        players[consoleplayer].message = DEH_String(MSGON);

    message_dontfuckwithme = true;
}
*/


//
// M_EndGame
//
void M_EndGameResponse(int key)
{
#ifdef SVE_PLAT_SWITCH
    if (key != key_menu_forward)
#else
    if (key != key_menu_confirm)
#endif
        return;

    currentMenu->lastOn = itemOn;
    M_ClearMenus (0);
    D_StartTitle ();
}

void M_CheckStartCast(void);

static void M_CastCallResponse(int key)
{
#ifdef SVE_PLAT_SWITCH
    if (key != key_menu_forward)
#else
    if (key != key_menu_confirm)
#endif
        return;
    M_CheckStartCast();
}

void M_StartCast(int choice)
{
    if(usergame)
    {
        M_StartMessage(DEH_String("You have to end your game first."), NULL, false);
        return;
    }
    M_StartMessage(i_seejoysticks ? CASTPROMPTGP : CASTPROMPT, M_CastCallResponse, true);
    messageHelp = 1;
}

void M_EndGame(int choice)
{
    choice = 0;
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (netgame)
    {
        M_StartMessage(DEH_String(i_seejoysticks ? NETENDGP : NETEND),NULL,false);
        return;
    }

    M_StartMessage(DEH_String(i_seejoysticks ? ENDGAMEGP : ENDGAME),M_EndGameResponse,true);
    messageHelp = 1;
}

//
// M_StartHelpScreens
//
// [SVE]: activate custom dynamic help screens
//
static void M_StartHelpScreens(void)
{
    M_InitHelp();
    inhelpscreens = true;
    S_StartSound(NULL, sfx_swtchn);
}

//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    // [SVE]
    M_StartHelpScreens();
}

/*
//
// M_ReadThis2
//
// haleyjd 08/28/10: [STRIFE] Eliminated DOOM stuff.
//
void M_ReadThis2(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef2);
}

//
// M_ReadThis3
//
// haleyjd 08/28/10: [STRIFE] New function.
//
void M_ReadThis3(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef3);
}
*/

/*
// haleyjd 08/28/10: [STRIFE] Not used.
void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}
*/

// haleyjd [SVE]: Implemented cast call!
#if 1
extern void F_StartCast(void);

//
// M_CheckStartCast
//
// [STRIFE] New but unused function. Was going to start a cast
//   call from within the menu system... not functional even in
//   the earliest demo version.
//
void M_CheckStartCast(void)
{
    F_StartCast();
    M_ClearMenus(0);
}
#endif

//
// M_QuitResponse
//
// haleyjd 09/11/10: [STRIFE] Modifications to start up endgame
// demosequence.
//
void M_QuitResponse(int key)
{
    char buffer[20];

    if (key != key_menu_confirm)
        return;

    if(netgame)
        I_Quit();
    else
    {
        int i = gametic % 8 + 1;
        DEH_snprintf(buffer, sizeof(buffer), "qfmrm%i", i);
        I_StartVoice(buffer);
        D_QuitGame(3*TICRATE + ((i == 7) * 51)); // [SVE]: fix for QFMRM7
    }
}

/*
// haleyjd 09/11/10: [STRIFE] Unused
static char *M_SelectEndMessage(void)
{
}
*/

//
// M_QuitStrife
//
// [STRIFE] Renamed from M_QuitDOOM
// haleyjd 09/11/10: No randomized text message; that's taken care of
// by the randomized voice message after confirmation.
//
void M_QuitStrife(int choice)
{
    char *str = 
        i_seejoysticks ? "Do you really want to leave?\n\n" DOSYGP :
                         "Do you really want to leave?\n\n" DOSY;
    DEH_snprintf(endstring, sizeof(endstring), str);
  
    M_StartMessage(endstring, M_QuitResponse, true);
}




void M_ChangeSensitivity(int choice)
{
    switch(choice)
    {
    case 0:
        mouseSensitivityX--;
        if(mouseSensitivityX < 0)
            mouseSensitivityX = 9;
        break;
    case 1:
        mouseSensitivityX++;
        if(mouseSensitivityX > 9)
            mouseSensitivityX = 0;
        break;
    }
}

/*
// haleyjd [STRIFE] Unused
void M_ChangeDetail(int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    R_SetViewSize (screenblocks, detailLevel);

    if (!detailLevel)
	players[consoleplayer].message = DEH_String(DETAILHI);
    else
	players[consoleplayer].message = DEH_String(DETAILLO);
}
*/

// [STRIFE] Verified unmodified
void M_SizeDisplay(int choice)
{
#if 0
    switch(choice)
    {
    case 0:
        if (screenSize > 0)
        {
            screenblocks--;
            screenSize--;
        }
        break;
    case 1:
        if (screenSize < 8)
        {
            screenblocks++;
            screenSize++;
        }
        break;
    }
#else
    // [SVE]: we're not supporting screensizes with a border, due to HUD issues.

    switch(choice)
    {
    case 0:
        if(screenSize > 7)
        {
            screenblocks--;
            screenSize--;
        }
        else
        {
            screenblocks = 11;
            screenSize = screenblocks - 3;
        }
        break;
    case 1:
        if(screenSize < 8)
        {
            screenblocks++;
            screenSize++;
        }
        else
        {
            screenblocks = 10;
            screenSize = screenblocks - 3;
        }
        break;
    }

    // [SVE] svillarreal
    if(use3drenderer)
    {
        if(screenblocks < 10)
        {
            screenblocks = 10;
            screenSize = 7;
        }
    }
#endif

    R_SetViewSize (screenblocks, detailLevel);
}




//
//      Menu Functions
//

//
// M_DrawThermo
//
// haleyjd 08/28/10: [STRIFE] Changes to some patch coordinates.
//
void
M_DrawThermo
( int	x,
  int	y,
  int	thermWidth,
  int	thermDot )
{
    int         xx;
    int         yy; // [STRIFE] Needs a temp y coordinate variable
    int         i;

    xx = x;
    yy = y + 6; // [STRIFE] +6 to y coordinate
    V_DrawPatchDirect(xx, yy, W_CacheLumpName(DEH_String("M_THERML"), PU_CACHE));
    xx += 8;
    for (i=0;i<thermWidth;i++)
    {
        V_DrawPatchDirect(xx, yy, W_CacheLumpName(DEH_String("M_THERMM"), PU_CACHE));
        xx += 8;
    }
    V_DrawPatchDirect(xx, yy, W_CacheLumpName(DEH_String("M_THERMR"), PU_CACHE));

    // [STRIFE] +2 to initial y coordinate
    V_DrawPatchDirect((x + 8) + thermDot * 8, y + 2,
                      W_CacheLumpName(DEH_String("M_THERMO"), PU_CACHE));
}


// haleyjd: These are from DOOM v0.5 and the prebeta! They drew those ugly red &
// blue checkboxes... preserved for historical interest, as not in Strife.
void
M_DrawEmptyCell
( menu_t*	menu,
  int		item )
{
    V_DrawPatchDirect(menu->x - 10, menu->y + item * LINEHEIGHT - 1, 
                      W_CacheLumpName(DEH_String("M_CELL1"), PU_CACHE));
}

void
M_DrawSelCell
( menu_t*	menu,
  int		item )
{
    V_DrawPatchDirect(menu->x - 10, menu->y + item * LINEHEIGHT - 1,
                      W_CacheLumpName(DEH_String("M_CELL2"), PU_CACHE));
}


void
M_StartMessage
( const char*		string,
  void*		routine,
  boolean	input )
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageHelp = 0;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    messageLastMenuPause = menupause; // haleyjd: [SVE] forgotten by Rogue
    menuactive = true;
    menupause  = true;                // haleyjd: [SVE] ditto
    return;
}



void M_StopMessage(void)
{
    menuactive = messageLastMenuActive;
    menupause  = messageLastMenuPause;  // [SVE]
    messageToPrint = 0;
}



//
// Find string width from hu_font chars
//
int M_StringWidth(const char* string)
{
    size_t             i;
    int             w = 0;
    int             c;

    for (i = 0;i < strlen(string);i++)
    {
        c = toupper(string[i]) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
            w += 4;
        else
            w += SHORT (hu_font[c]->width);
    }

    return w;
}



//
//      Find string height from hu_font chars
//
int M_StringHeight(const char* string)
{
    size_t             i;
    int             h;
    int             height = SHORT(hu_font[0]->height);

    h = height;
    for (i = 0;i < strlen(string);i++)
        if (string[i] == '\n')
            h += height;

    return h;
}


//
// M_WriteTextEx
//
// Write a string using the hu_font
// haleyjd 09/04/10: [STRIFE]
// * Rogue made a lot of changes to this for the dialog system.
// MaxW: [SVE] We need to go to the end of the line sometimes, so there's now a bool for that
//
int
M_WriteTextEx
( int           x,
  int           y,
  const char*   string, // haleyjd: made const for safety w/dialog engine
  boolean       breakearly)
{
    int	        w;
    const char* ch;
    int         c;
    int         cx;
    int         cy;

    ch = string;
    cx = x;
    cy = y;

    while(1)
    {
        c = *ch++;
        if (!c)
            break;

        // haleyjd 09/04/10: [STRIFE] Don't draw spaces at the start of lines.
        if(c == ' ' && cx == x)
            continue;

        if (c == '\n')
        {
            cx = x;
            cy += 11; // haleyjd 09/04/10: [STRIFE]: Changed 12 -> 11
            continue;
        }

        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c>= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = SHORT (hu_font[c]->width);

        // haleyjd 09/04/10: [STRIFE] Different linebreak handling
        if (cx + w > (breakearly ? SCREENWIDTH - 20 : SCREENWIDTH))
        {
            cx = x;
            cy += 11;
            --ch;
        }
        else
        {
            V_DrawPatchDirect(cx, cy, hu_font[c]);
            cx += w;
        }
    }

    // [STRIFE] Return final y coordinate.
    return cy + 12;
}

//
// M_WriteText
//
// MaxW: [SVE] This is now a wrapper because I can't be
// bothered to redo the calls for 99% of M_WriteText calls
//
int M_WriteText(int x, int y, const char *string)
{
    return M_WriteTextEx(x, y, string, true);
}

//
// M_DialogDimMsg
//
// [STRIFE] New function
// haleyjd 09/04/10: Painstakingly transformed from the assembly code, as the
// decompiler could not touch it. Redimensions a string to fit on screen, leaving
// at least a 20 pixel margin on the right side. The string passed in must be
// writable.
//
void M_DialogDimMsg(int x, int y, char *str, boolean useyfont)
{
    int rightbound = (SCREENWIDTH - 20) - x;
    patch_t **fontarray;  // ebp
    int linewidth = 0;    // esi
    int i = 0;            // edx
    char *message = str;  // edi
    char  bl;             // bl

    if(useyfont)
       fontarray = yfont;
    else
       fontarray = hu_font;

    bl = toupper(*message);

    if(!bl)
        return;

    // outer loop - run to end of string
    do
    {
        if(bl != '\n')
        {
            int charwidth; // eax
            int tempwidth; // ecx

            if(bl < HU_FONTSTART || bl > HU_FONTEND)
                charwidth = 4;
            else
                charwidth = SHORT(fontarray[bl - HU_FONTSTART]->width);

            tempwidth = linewidth + charwidth;

            // Test if the line still fits within the boundary...
            if(tempwidth >= rightbound)
            {
                // Doesn't fit...
                char *tempptr = &message[i]; // ebx
                char  al;                    // al

                // inner loop - run backward til a space (or the start of the
                // string) is found, subtracting width off the current line.
                // BUG: shouldn't we stop at a previous '\n' too?
                while(*tempptr != ' ' && i > 0)
                {
                    tempptr--;
                    // BUG: they didn't add the first char to linewidth yet...
                    linewidth -= charwidth; 
                    i--;
                    al = toupper(*tempptr);
                    if(al < HU_FONTSTART || al > HU_FONTEND)
                        charwidth = 4;
                    else
                        charwidth = SHORT(fontarray[al - HU_FONTSTART]->width);
                }
                // Replace the space with a linebreak.
                // BUG: what if i is zero? ... infinite loop time!
                message[i] = '\n';
                linewidth = 0;
            }
            else
            {
                // The line does fit.
                // Spaces at the start of a line don't count though.
                if(!(bl == ' ' && linewidth == 0))
                    linewidth += charwidth;
            }
        }
        else
            linewidth = 0; // '\n' seen, so reset the line width
    }
    while((bl = toupper(message[++i])) != 0); // step to the next character
}

// These keys evaluate to a "null" key in Vanilla Doom that allows weird
// jumping in the menus. Preserve this behavior for accuracy.

static boolean IsNullKey(int key)
{
    return key == KEY_PAUSE || key == KEY_CAPSLOCK
        || key == KEY_SCRLCK || key == KEY_NUMLOCK;
}

//
// Test if mouse pointer is inside menu item rect
//
static boolean M_MouseInRect(int x, int y, menuitem_t *item)
{
    if(!(item->w | item->h)) // zero size?
        return false;

    return (x >= item->x && x <= item->x + item->w &&
            y >= item->y && y <= item->y + item->h);
}

//
// haleyjd [SVE] 20141007: Better mouse support (similar to that in frontend 
// component)
//
static menuitem_t *M_FindMouseMenuItem(int x, int y)
{
    int i;
    menuitem_t *item, *ret = NULL;

    for(i = 0; i < currentMenu->numitems; i++)
    {
        item = &(currentMenu->menuitems[i]);

        if(item->status < 1)
            continue;

        // test if in main rect
        if(M_MouseInRect(x, y, item))
        {
            ret = item;
            break;
        }

        /*
        // TODO: values, sliders
        // test if in value rect
        if(FE_MouseInValueRect(smx, smy, item))
        {
            ret = item;
            break;
        }
        */
    }

    return ret;
}



//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
    int             ch;
    int             key;
    int             i;
    static  int     joywait = 0;

    // In testcontrols mode, none of the function keys should do anything
    // - the only key is escape to quit.

    if (testcontrols)
    {
        if (ev->type == ev_quit
         || (ev->type == ev_keydown
          && (ev->data1 == key_menu_activate || ev->data1 == key_menu_quit)))
        {
            I_Quit();
            return true;
        }

        return false;
    }

    // [SVE]: in help screens?
    if(inhelpscreens)
    {
        boolean res = M_HelpResponder(ev);
        if(res)
        {
            M_StopHelp();
            S_StartSound(NULL, sfx_mtalht);
            inhelpscreens = false;
        }
        return true;
    }

    // "close" button pressed on window?
    if (ev->type == ev_quit)
    {
        // First click on close button = bring up quit confirm message.
        // Second click on close button = confirm quit

        if (menuactive && messageToPrint && messageRoutine == M_QuitResponse)
        {
            M_QuitResponse(key_menu_confirm);
        }
        else
        {
            S_StartSound(NULL, sfx_swtchn);
            M_QuitStrife(0);
        }

        return true;
    }

    // key is the key pressed, ch is the actual character typed
  
    ch = 0;
    key = -1;

    static const int i_deadzone = (32767 / 2);

    if(ev->type == ev_joystick && joywait < I_GetTime())
    {
        if(ev->data3 < -i_deadzone)
        {
            key = key_menu_up;
            joywait = I_GetTime() + 5;
        }
        else if (ev->data3 > i_deadzone)
        {
            key = key_menu_down;
            joywait = I_GetTime() + 5;
        }

        if (ev->data2 < -i_deadzone)
        {
            key = key_menu_left;
            joywait = I_GetTime() + 2;
        }
        else if (ev->data2 > i_deadzone)
        {
            key = key_menu_right;
            joywait = I_GetTime() + 2;
        }
    }
    // [SVE] svillarreal
    else if(ev->type == ev_joybtndown)
    {
        if(menuactive && messageToPrint && messageRoutine == M_QuitResponse)
        {
            static const int *joyconfirmkeys[][2] =
            {
                { &joybmenu, &key_menu_activate },
                { &joybmenu_confirm, &key_menu_confirm },
                { &joybmenu_abort, &key_menu_abort }
            };

            for(i = 0; i < arrlen(joyconfirmkeys); ++i)
            {
                if(ev->data1 == *joyconfirmkeys[i][0])
                {
                    key = *joyconfirmkeys[i][1];
                    break;
                }
            }
        }
        else
        {
            static const int *joymenukeys[][2] =
            {
                { &joybmenu, &key_menu_activate },
                { &joybmenu_forward, &key_menu_forward },
                { &joybmenu_up, &key_menu_up },
                { &joybmenu_down, &key_menu_down },
                { &joybmenu_left, &key_menu_left },
                { &joybmenu_right, &key_menu_right },
                { &joybmenu_back, &key_menu_back }
            };

            for(i = 0; i < arrlen(joymenukeys); ++i)
            {
                if(ev->data1 == *joymenukeys[i][0])
                {
                    key = *joymenukeys[i][1];
                    break;
                }
            }
        }
    }

    else if(ev->type == ev_mouse || ev->type == ev_mousebutton)
    {
        // haleyjd 20141006: [SVE] improved mouse behavior
        int mx = 0, my = 0;
        boolean mouseState = (menuactive && currentMenu && !(saveStringEnter || messageToPrint));
        menuitem_t *item;

        if((ev->data1 & 1) || (ev->data2 | ev->data3))
            I_GetAbsoluteMousePosition(&mx, &my);

        // left mouse button down
        if(ev->data1 & 1)
        {
            if(!mouseState)
                key = key_menu_forward;
            else
            {
                if((item = M_FindMouseMenuItem(mx, my)))
                {
                    if(item != &(currentMenu->menuitems[itemOn]))
                    {
                        // pop selection
                        itemOn = item - currentMenu->menuitems;
                        S_StartSound(NULL, sfx_pstop);
                    }
                    else if(item->routine && item->status)
                    {
                        // activate item
                        itemOn = item - currentMenu->menuitems;
                        currentMenu->lastOn = itemOn;
                        if(item->status == 2)
                        {
                            item->routine(1);      // right arrow
                            S_StartSound(NULL, sfx_stnmov);
                        }
                        else
                        {
                            item->routine(itemOn);
                        }
                    }
                    return true;
                }
            }
        }
        if(ev->data1 & 2)
        {
            if(menuactive && (menuindialog || currentMenu == &MainDef))
                key = key_menu_activate;
            else
                key = key_menu_back;
        }

        // track selection with mouse motion
        if(mouseState && (ev->data2 | ev->data3))
        {
            if((item = M_FindMouseMenuItem(mx, my)) && 
               item != &(currentMenu->menuitems[itemOn]))
            {
                itemOn = item - currentMenu->menuitems;
                S_StartSound(NULL, sfx_pstop);
            }
            return true;
        }
    }
    else if (ev->type == ev_keydown)
    {
        key = ev->data1;
        i_seejoysticks = false; // getting keyboard input, change state
    }
    else if (ev->type == ev_text)
    {
        ch = ev->data1;
        // TODO: Uncomment below?
        //i_seejoysticks = false;
    }

    if(key == -1 && (ch == 0 && !saveStringEnter))
        return false;

    // Save Game string input
    if(saveStringEnter)
    {
        if (I_HaveSoftwareKeyboard() > 0)
        {
            char *t = NULL;
            char* vkname = NULL;
            vkname = I_RunSoftwareKeyboard("Name", savegamestrings[quickSaveSlot], SAVESTRINGSIZE - 1);

            if (vkname != NULL && strlen(vkname) > 0)
            {
                saveCharIndex = 0;
                for (t = vkname; *t != '\0'; t++)
                {
                    savegamestrings[quickSaveSlot][saveCharIndex++] = *t;
                    savegamestrings[quickSaveSlot][saveCharIndex] = '\0';
                }

                saveStringEnter = 0;

                if (gameversion == exe_strife_1_31 && !namingCharacter)
                {
                    M_DoSave(quickSaveSlot);
                    return true;
                }
                if (savegamestrings[quickSaveSlot][0])
                    M_DoNameChar(quickSaveSlot);

            }
            else
            {
                saveStringEnter = 0;
                M_StringCopy(savegamestrings[quickSaveSlot], saveOldString,
                    sizeof(savegamestrings[quickSaveSlot]));

                return false;
            }
        }

        switch(key)
        {
        case KEY_BACKSPACE:
            if (saveCharIndex > 0)
            {
                saveCharIndex--;
                savegamestrings[quickSaveSlot][saveCharIndex] = 0;
            }
            break;

        case KEY_ESCAPE:
            saveStringEnter = 0;
            M_StringCopy(savegamestrings[quickSaveSlot], saveOldString,
                         sizeof(savegamestrings[quickSaveSlot]));
            break;

        case KEY_ENTER:
            // [STRIFE]
            saveStringEnter = 0;
            if(gameversion == exe_strife_1_31 && !namingCharacter)
            {
               // In 1.31, we can be here as a result of normal saving again,
               // whereas in 1.2 this only ever happens when naming your
               // character to begin a new game.
               M_DoSave(quickSaveSlot);
               return true;
            }
            if (savegamestrings[quickSaveSlot][0])
                M_DoNameChar(quickSaveSlot);
            break;

        default:
            // This is complicated.
            // Vanilla has a bug where the shift key is ignored when entering
            // a savegame name. If vanilla_keyboard_mapping is on, we want
            // to emulate this bug by using 'data1'. But if it's turned off,
            // it implies the user doesn't care about Vanilla emulation: just
            // use the correct 'data2'.

            if (vanilla_keyboard_mapping)
            {
                ch = key;
            }
            break;
        }

        ch = toupper(ch);

        if (ch != ' '
         && (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
        {
            return true;
        }

        if (ch >= 32 && ch <= 127 &&
            saveCharIndex < SAVESTRINGSIZE - 1 &&
            M_StringWidth(savegamestrings[quickSaveSlot]) <
            (SAVESTRINGSIZE - 2) * 8)
        {
            savegamestrings[quickSaveSlot][saveCharIndex++] = ch;
            savegamestrings[quickSaveSlot][saveCharIndex] = 0;
        }
        return true;
    }

    // Take care of any messages that need input
    if(messageToPrint)
    {
        if (messageNeedsInput)
        {
            if ((key != ' ' && key != KEY_ESCAPE
                && key != key_menu_confirm && key != key_menu_abort)
#ifdef SVE_PLAT_SWITCH
                && (key != key_menu_forward && key != key_menu_back)
#endif
                )
            {
                return false;
            }
        }

        menuactive = messageLastMenuActive;
        messageToPrint = 0;
        if (messageRoutine)
            messageRoutine(key);

        menupause = false;                // [STRIFE] unpause
        menuactive = false;
        S_StartSound(NULL, sfx_mtalht);   // [STRIFE] sound
        return true;
    }

    // [STRIFE]:
    // * In v1.2 this is moved to F9 (quickload)
    // * In v1.31 it is moved to F12 with DM spy, and quicksave
    //   functionality is restored separate from normal saving
    /*
    if (devparm && key == key_menu_help)
    {
        G_ScreenShot ();
        return true;
    }
    */

    // F-Keys
    if (!(menuactive || menuindialog)) // [SVE]: no F-keys during dialogs
    {
        if (key == key_menu_decscreen)      // Screen size down
        {
            if (automapactive || chat_on)
                return false;
            M_SizeDisplay(0);
            S_StartSound(NULL, sfx_stnmov);
            return true;
        }
        else if (key == key_menu_incscreen) // Screen size up
        {
            if (automapactive || chat_on)
                return false;
            M_SizeDisplay(1);
            S_StartSound(NULL, sfx_stnmov);
            return true;
        }
        else if (key == key_menu_help)     // Help key
        {
            M_StartControlPanel();
            M_StartHelpScreens();
            return true;
        }
        else if (key == key_menu_save)     // Save
        {
            // [STRIFE]: Hub saves
            if(gameversion == exe_strife_1_31)
                namingCharacter = false; // just saving normally, in 1.31

            if(netgame)
            {
                return false; // [SVE]: Don't eat F2 in multiplayer
            }                
            else if(players[consoleplayer].health <= 0 ||
                    players[consoleplayer].cheats & CF_ONFIRE)
            {
                players[consoleplayer].message = DEH_String("You can't save when you're dead!"); // [SVE]
                S_StartSound(NULL, sfx_oof);
            }
            else
            {
                M_StartControlPanel();
                S_StartSound(NULL, sfx_swtchn);
                M_SaveGame(0);
            }
            return true;
        }
        else if (key == key_menu_load)     // Load
        {
            // [STRIFE]: Hub saves
            if(gameversion == exe_strife_1_31)
            {
                // 1.31: normal save loading
                namingCharacter = false;
                M_StartControlPanel();
                M_LoadGame(0);
                S_StartSound(NULL, sfx_swtchn);
            }
            else
            {
                // Pre 1.31: quickload only
                S_StartSound(NULL, sfx_swtchn);
                M_QuickLoad();
            }
            return true;
        }
        else if (key == key_menu_volume)   // Sound Volume
        {
            M_StartControlPanel ();
            currentMenu = &SoundDef;
            itemOn = sfx_vol;
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_detail)   // Detail toggle
        {
            //M_ChangeDetail(0);
            M_AutoUseHealth(); // [STRIFE]
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_qsave)    // Quicksave
        {
            // [STRIFE]: Hub saves
            if(gameversion == exe_strife_1_31)
                namingCharacter = false; // for 1.31 save changes

            if(netgame)
            {
                players[consoleplayer].message = DEH_String("You can't save a netgame."); // [SVE]
                S_StartSound(NULL, sfx_oof);
            }                
            else if(players[consoleplayer].health <= 0 ||
                    players[consoleplayer].cheats & CF_ONFIRE)
            {
                players[consoleplayer].message = DEH_String("You can't save when you're dead!"); // [SVE]
                S_StartSound(NULL, sfx_oof);
            }
            else
            {
                S_StartSound(NULL, sfx_swtchn);
                M_QuickSave();
            }
            return true;
        }
        else if (key == key_menu_endgame)  // End game
        {
            S_StartSound(NULL, sfx_swtchn);
            M_EndGame(0);
            return true;
        }
        else if (key == key_menu_messages) // Toggle messages
        {
            //M_ChangeMessages(0);
            M_ChangeShowText(); // [STRIFE]
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_qload)    // Quickload
        {
            // [STRIFE]
            // * v1.2: takes a screenshot
            // * v1.31: does quickload again
            if(gameversion == exe_strife_1_31)
            {
                namingCharacter = false;
                S_StartSound(NULL, sfx_swtchn);
                M_QuickLoad();
            }
            else
                G_ScreenShot();
            return true;
        }
        else if (key == key_menu_quit)     // Quit DOOM
        {
            S_StartSound(NULL, sfx_swtchn);
            M_QuitStrife(0);
            return true;
        }
        else if (key == key_menu_gamma)    // gamma toggle
        {
            usegamma++;
            if (usegamma > 4)
                usegamma = 0;

            // [SVE] svillarreal
            if(use3drenderer)
            {
                RB_DeleteDoomData();
                RB_InitExtraHudTextures(); // reload them
                FE_RefreshBackgrounds();
                RB_SetPatchBufferPalette();
            }

            players[consoleplayer].message = DEH_String(gammamsg[usegamma]);
            I_SetPalette (W_CacheLumpName (DEH_String("PLAYPAL"),PU_CACHE));
            return true;
        }
        else if(gameversion == exe_strife_1_31 && key == key_spy)
        {
            // [SVE]: F12 is the default hotkey for Steam screenshots so, if
            // building with Steam support, don't enable local ones here, as
            // this causes two copies of the shot to be written. Do eat the
            // key though, because the Steam popup is going to come up.
#ifndef I_APPSERVICES_SCREENSHOTS
            // haleyjd 20130301: 1.31 moved screenshots to F12.
            G_ScreenShot();
#endif
            return true;
        }
        else if (key != 0 && key == key_menu_screenshot)
        {
            G_ScreenShot();
            return true;
        }
    }

    // Pop-up menu?
    if (!menuactive)
    {
        if (key == key_menu_activate && !chat_on)
        {
            M_StartControlPanel ();
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        return false;
    }

    
    // Keys usable within menu

    if (key == key_menu_down)
    {
        // Move down to next item
        int oldItemOn = itemOn;

        do
        {
            if (itemOn+1 > currentMenu->numitems-1)
                itemOn = 0;
            else itemOn++;
        } while(currentMenu->menuitems[itemOn].status==-1);

        // [SVE]: play sound only once, and only if it actually moved
        if(itemOn != oldItemOn)
            S_StartSound(NULL, sfx_pstop);

        return true;
    }
    else if (key == key_menu_up)
    {
        // Move back up to previous item
        int oldItemOn = itemOn;

        do
        {
            if (!itemOn)
                itemOn = currentMenu->numitems-1;
            else itemOn--;
        } while(currentMenu->menuitems[itemOn].status==-1);

        // [SVE]: play sound only once, and only if it actually moved
        if(itemOn != oldItemOn)
            S_StartSound(NULL, sfx_pstop);

        return true;
    }
    else if (key == key_menu_left)
    {
        // Slide slider left

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(0);
        }
        return true;
    }
    else if (key == key_menu_right)
    {
        // Slide slider right

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(1);
        }
        return true;
    }
    else if (key == key_menu_forward)
    {
        // Activate menu item

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                currentMenu->menuitems[itemOn].routine(1);      // right arrow
                S_StartSound(NULL, sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(itemOn);
                //S_StartSound(NULL, sfx_swish); [STRIFE] No sound is played here.
            }
        }
        return true;
    }
    else if (key == key_menu_activate)
    {
        // Deactivate menu
        if(gameversion == exe_strife_1_31) // [STRIFE]: 1.31 saving
            namingCharacter = false;

        if(menuindialog) // [STRIFE] - Get out of dialog engine semi-gracefully
            P_DialogDoChoice(-1);

        currentMenu->lastOn = itemOn;
        M_ClearMenus (0);
        S_StartSound(NULL, sfx_mtalht); // villsa [STRIFE]: sounds
        return true;
    }
    else if (key == key_menu_back)
    {
        // Go back to previous menu

        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(NULL, sfx_swtchn);
        }
        return true;
    }

    // Keyboard shortcut?
    // Vanilla Strife has a weird behavior where it jumps to the scroll bars
    // when certain keys are pressed, so emulate this.

    else if (ch != 0 || IsNullKey(key))
    {
        // Keyboard shortcut?

        for (i = itemOn+1;i < currentMenu->numitems;i++)
        {
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return true;
            }
        }

        for (i = 0;i <= itemOn;i++)
        {
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                // [SVE]: sound only if actually moves
                if(i != itemOn)
                    S_StartSound(NULL, sfx_pstop);
                itemOn = i;
                return true;
            }
        }
    }

    return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;
    
    menuactive = 1;
    menupause = true;
    currentMenu = &MainDef;         // JDC
    itemOn = currentMenu->lastOn;   // JDC
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
    static short x;
    static short y;
    unsigned int i;
    unsigned int max;
    char         string[80];
    const char  *name;
    int          start;

    // [SVE]: in help screens?
    if(inhelpscreens)
    {
        M_HelpDrawer();
        return;
    }

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        start = 0;
        y = 100 - M_StringHeight(messageString) / 2;
        while (messageString[start] != '\0')
        {
            int foundnewline = 0;

            for (i = 0; i < strlen(messageString + start); i++)
            {
                if (messageString[start + i] == '\n')
                {
                    M_StringCopy(string, messageString + start, sizeof(string));
                    if (i < sizeof(string))
                    {
                        string[i] = '\0';
                    }

                    foundnewline = 1;
                    start += i + 1;
                    break;
                }
            }

            if (!foundnewline)
            {
                M_StringCopy(string, messageString + start, sizeof(string));
                start += strlen(string);
            }

            x = 160 - M_StringWidth(string) / 2;
            M_WriteText(x, y, string);
            y += SHORT(hu_font[0]->height);
        }

        if (messageHelp)
        {
            FE_NX_DrawToolTips(5);
        }

        return;
    }

	if (!menuactive)
	{
		// dimitrisg : if theres no menu on NX. let the user know what they need to do to bring it up
        if (gamestate != GS_LEVEL)
        {
            // Edward: If we are on the credits page, don't draw this to prevent covering any of the text.
            extern int demosequence;
            if (gamestate == GS_DEMOSCREEN && demosequence == 11)
            {
                return;
            }
            FE_NX_DrawToolTips(3);
        }

		return;
	}

    if (currentMenu->routine)
        currentMenu->routine();         // call Draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    max = currentMenu->numitems;

    for(i = 0; i < max; i++)
    {
        // haleyjd 20141004: [SVE] mouse in menus support
        // haleyjd 20141014: [SVE] allow item->name to be a big font string
        menuitem_t *item = &(currentMenu->menuitems[i]);
        name = DEH_String(item->name);

        if(*name)
        {
            int lumpnum = W_CheckNumForName(name);

            if(lumpnum >= 0)
            {
                patch_t *p = W_CacheLumpName(name, PU_CACHE);
                item->x = x - SHORT(p->leftoffset);
                item->y = y - SHORT(p->topoffset);
                item->w = SHORT(p->width);
                item->h = SHORT(p->height);
                V_DrawPatchDirect(x, y, p);
            }
            else
            {
                item->x = x;
                item->y = y+4;
                item->w = V_BigFontStringWidth(item->name);
                item->h = V_BigFontStringHeight(item->name);
                V_WriteBigText(item->name, x, y+4);

            }
        }
        y += LINEHEIGHT;
    }


    // haleyjd 08/27/10: [STRIFE] Adjust to draw spinning Sigil
    // DRAW SIGIL
    V_DrawPatchDirect(x + CURSORXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT,
                      W_CacheLumpName(DEH_String(cursorName[whichCursor]),
                      PU_CACHE));
}


//
// M_ClearMenus
//
// haleyjd 08/28/10: [STRIFE] Added an int param so this can be called by menus.
//         09/08/10: Added menupause.
//
void M_ClearMenus (int choice)
{
    choice = 0;     // haleyjd: for no warning; not from decompilation.
    menuactive = 0;
    menupause = 0;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}


//
// M_Ticker
//
// haleyjd 08/27/10: [STRIFE] Rewritten for Sigil cursor
//
void M_Ticker (void)
{
    // [SVE]: visual mouse cursor state
    static boolean prevmenuactive;

    // [SVE]: running frontend options menu?
    if(FE_InOptionsMenu())
    {
        FE_InGameOptionsTicker();
        return;
    }

    if(menuactive != prevmenuactive)
    {
        I_SetShowVisualCursor(menuactive);
        prevmenuactive = menuactive;
    }

    if (--cursorAnimCounter <= 0)
    {
        whichCursor = (whichCursor + 1) % 8;
        cursorAnimCounter = 5;
    }

    // [SVE]: in help screens?
    if(inhelpscreens)
        M_HelpTicker();
}


//
// M_Init
//
// haleyjd 08/27/10: [STRIFE] Removed DOOM gamemode stuff
//
void M_Init (void)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichCursor = 0;
    cursorAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive; // STRIFE-FIXME: assigns 0 here...
    quickSaveSlot = -1;

    // [STRIFE]: Initialize savegame paths and clear temporary directory
    G_WriteSaveName(5, "ME");
    ClearTmp();

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.
}

