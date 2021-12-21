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
//	Intermission screens.
//

#include "z_zone.h"

#include "doomstat.h"
#include "d_main.h"
#include "d_player.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "m_menu.h"
#include "sounds.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"

// [SVE] svillarreal
#include "p_local.h"
#include "i_social.h"

extern patch_t *hu_font[HU_FONTSIZE];

static int bluescore;
static int redscore;
static int statstate;
static int wi_time;
static int wi_pausetime;
static int acceleratestage;
static int wi_exiting;
static wbstartstruct_t *wbs;

enum
{
    WI_STATE_BEGIN,    // beginning
    WI_STATE_BLUETEAM, // tick up blue team's score
    WI_STATE_PAUSE1,   // wait
    WI_STATE_REDTEAM,  // tick up red team's score
    WI_STATE_PAUSE2,   // wait
    WI_STATE_WINNER,   // show winner
    WI_STATE_END       // end
};

static void WI_updateStats(void)
{
    // check for acceleration
    if(acceleratestage)
    {
        acceleratestage = 0;

        if(statstate == WI_STATE_BLUETEAM || statstate == WI_STATE_REDTEAM)
        {
            if(statstate == WI_STATE_BLUETEAM)
                bluescore = ctcbluescore;
            else if(statstate == WI_STATE_REDTEAM)
                redscore = ctcredscore;

            ++statstate;
            S_StartSound(NULL, sfx_explod);
            wi_pausetime = TICRATE;
        }
        else if(wi_pausetime)
            wi_pausetime = 1;
    }

    // count up stats or wait for next state
    switch(statstate)
    {
    case WI_STATE_BLUETEAM:
        if(!(wi_time % 5))
        {
            S_StartSound(NULL, sfx_rifle);
            ++bluescore;
        }
        if(bluescore >= ctcbluescore)
        {
            bluescore = ctcbluescore;
            S_StartSound(NULL, sfx_explod);
            ++statstate;
            wi_pausetime = 2*TICRATE;
        }
        break;
    case WI_STATE_REDTEAM:
        if(!(wi_time % 5))
        {
            S_StartSound(NULL, sfx_rifle);
            ++redscore;
        }
        if(redscore >= ctcredscore)
        {
            redscore = ctcredscore;
            S_StartSound(NULL, sfx_explod);
            ++statstate;
            wi_pausetime = 2*TICRATE;
        }
        break;
    case WI_STATE_BEGIN:
    case WI_STATE_PAUSE1:
    case WI_STATE_PAUSE2:
        if(wi_pausetime > 0)
            --wi_pausetime;
        if(wi_pausetime == 0)
            ++statstate;
        if(statstate == WI_STATE_WINNER)
            S_StartSound(NULL, sfx_inqdth);
        break;
    default:
        break;
    }
}

static void WI_checkForAccelerate(void)
{
    int i;
    player_t *player;

    // check for button presses to skip delays
    for(i = 0, player = players; i < MAXPLAYERS; i++, player++)
    {
        if(playeringame[i])
        {
            if(player->cmd.buttons & BT_ATTACK)
            {
                if(!player->attackdown)
                    acceleratestage = 1;
                player->attackdown = true;
            }
            else
                player->attackdown = false;

            if(player->cmd.buttons & BT_USE)
            {
                if(!player->usedown)
                    acceleratestage = 1;
                player->usedown = true;
            }
            else
                player->usedown = false;
        }
    }
}

// Updates stuff each tick
void WI_Ticker(void)
{
    // counter for animation
    ++wi_time;

    // intermission music
    if(wi_time == 1)
        S_ChangeMusic(mus_tech, 1);

    WI_checkForAccelerate();

    if(statstate < WI_STATE_WINNER)
        WI_updateStats();
    else if(statstate == WI_STATE_WINNER)
    {
        if(acceleratestage && !wi_exiting)
        {
            wi_exiting = 1;
            wi_pausetime = TICRATE;
            S_StartSound(NULL, sfx_wpnup);
        }
        else if(--wi_pausetime == 0)
        {
            G_WorldDone();
            HU_Stop();
        }
    }
}

static void WI_drawNum(int x, int y, int n)
{
    short zeroWidth = SHORT(hu_font['0' - HU_FONTSTART]->width);

    // draw the new number
    do
    {
        x -= zeroWidth;
        V_DrawPatch(x, y, hu_font[('0' + n%10) - HU_FONTSTART]);
        n /= 10;
    }
    while(n > 0);
}

static void WI_writeBigTextCentered(int y, char *string)
{
    V_WriteBigText(string, 160 - V_BigFontStringWidth(string)/2, y);
}

static void WI_writeTextCentered(int y, char *string)
{
    M_WriteText(160 - M_StringWidth(string)/2, y, string);
}

int WI_NumPlayers(void)
{
    int i, count = 0;

    for(i = 0; i < MAXPLAYERS; i++)
    {
        if(playeringame[i])
            ++count;
    }

    return count;
}

int WI_PlayersOnTeam(int team)
{
    int i, count = 0;

    for(i = 0; i < MAXPLAYERS; i++)
    {
        if(playeringame[i] && players[i].allegiance == team)
            ++count;
    }

    return count;
}

void RB_PageDrawer(const char* szPagename, const int xoff);
void WI_Drawer(void)
{
    // draw background
    patch_t *background = W_CacheLumpName("HELP0", PU_CACHE);
    const int xoff = -((background->width - SCREENWIDTH) / 2);

    if(use3drenderer && (xoff < 0))
    {
        RB_PageDrawer("HELP0", xoff);
    }
    else
    {
        V_DrawPatch(xoff, 0, background);
    }

    // draw title
    WI_writeBigTextCentered(8, "Capture the Chalice");
    WI_writeTextCentered(28, "Results");

    // draw blue team stats
    if(statstate >= WI_STATE_BLUETEAM)
    {
        V_DrawPatch(50, 68, W_CacheLumpName("I_RELB", PU_CACHE));
        M_WriteText(72, 74, "Blue Team Score");
        WI_drawNum(268, 74, bluescore);
    }

    // draw red team stats
    if(statstate >= WI_STATE_REDTEAM)
    {
        V_DrawPatch(50, 100, W_CacheLumpName("I_RELC", PU_CACHE));
        M_WriteText(72, 106, "Red Team Score");
        WI_drawNum(268, 106, redscore);
    }

    // draw victor
    if(statstate >= WI_STATE_WINNER)
    {
        if(ctcbluescore > ctcredscore)
        {
            WI_writeTextCentered(150, "Blue Team Wins the Match!");

            // [SVE] svillarreal - fancy cup achievement
            if(wbs->numplayers > 1 &&
               !P_CheckPlayersCheating(ACH_ALLOW_DM) && 
               players[consoleplayer].allegiance == CTC_TEAM_BLUE)
            {
                gAppServices->SetAchievement("SVE_ACH_CTC");
            }
        }
        else if(ctcredscore > ctcbluescore)
        {
            WI_writeTextCentered(150, "Red Team Wins the Match!");

            // [SVE] svillarreal - fancy cup achievement
            if(wbs->numplayers > 1 &&
               !P_CheckPlayersCheating(ACH_ALLOW_DM) && 
               players[consoleplayer].allegiance == CTC_TEAM_RED)
            {
                gAppServices->SetAchievement("SVE_ACH_CTC");
            }
        }
        else
        {
            WI_writeTextCentered(150, "The Match is a Draw!"); // How did THIS happen?
        }
    }
}

static void WI_initVariables(void)
{
    bluescore    = 0;
    redscore     = 0;
    statstate    = WI_STATE_BEGIN;
    wi_time      = 0;
    wi_pausetime = 2*TICRATE;
    wi_exiting   = 0;

    // not skipping forward
    acceleratestage = 0;
}

void WI_Start(wbstartstruct_t *wbstartstruct)
{
    gamestate  = GS_INTERMISSION;
    gameaction = ga_nothing;
    wbs = wbstartstruct;
    WI_initVariables();
}

// EOF

