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
//	Handling interactions (i.e., collisions).
//

// Data.
#include "doomdef.h"
#include "dstrings.h"
#include "sounds.h"
#include "deh_main.h"
#include "deh_misc.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_misc.h"
#include "m_random.h"
#include "i_system.h"
#include "am_map.h"
#include "p_local.h"
#include "p_dialog.h"   // villsa [STRIFE]
#include "s_sound.h"
#include "p_inter.h"

#include "st_stuff.h"   // [SVE] svillarreal
#include "hu_stuff.h"   // villsa [STRIFE]
#include "z_zone.h"     // villsa [STRIFE]

// haleyjd [STRIFE]
#include "w_wad.h"
#include "p_pspr.h"
#include "p_dialog.h"
#include "f_finale.h"

// [SVE] svillarreal
#include "i_social.h"
#include "net_client.h"

#include "hu_lib.h"

#define BONUSADD    6


// a weapon is found with two clip loads,
// a big item has five clip loads
// villsa [STRIFE] updated arrays
int maxammo[NUMAMMO]    = { 250, 50, 25, 400, 100, 30, 16 };
int clipammo[NUMAMMO]   = { 10, 4, 2, 20, 4, 6, 4 };

// [SVE] svillarreal
mobj_t *curignitemobj = NULL;
int numignitechains = 0;


//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//
// [STRIFE] Modified for Strife ammo types
//
boolean P_GiveAmmo(player_t* player, ammotype_t ammo, int num)
{
    int		oldammo;

    if(ammo == am_noammo)
        return false;

    if(ammo > NUMAMMO)
        return false; // haleyjd 20140816: [SVE] stability

    if(player->ammo[ammo] == player->maxammo[ammo])
        return false;

    if(num)
        num *= clipammo[ammo];
    else
        num = clipammo[ammo]/2;

    if(gameskill == sk_baby
        || gameskill == sk_nightmare)
    {
        // give double ammo in trainer mode,
        // you'll need in nightmare
        num <<= 1;
    }

    oldammo = player->ammo[ammo];
    player->ammo[ammo] += num;

    if(player->ammo[ammo] > player->maxammo[ammo])
        player->ammo[ammo] = player->maxammo[ammo];

    // If non zero ammo, 
    // don't change up weapons,
    // player was lower on purpose.
    if(oldammo)
        return true;

    // We were down to zero,
    // so select a new weapon.
    // Preferences are not user selectable.

    // villsa [STRIFE] ammo update
    // where's the check for grenades? - haleyjd: verified no switch to grenades
    //   haleyjd 10/03/10: don't change to electric bow when picking up poison
    //   arrows.
    if(!player->readyweapon)
    {
        switch(ammo)
        {
        case am_bullets:
            if(player->weaponowned[wp_rifle])
                player->pendingweapon = wp_rifle;
            break;

        case am_elecbolts:
            if(player->weaponowned[wp_elecbow])
                player->pendingweapon = wp_elecbow;
            break;

        case am_cell:
            if(player->weaponowned[wp_mauler])
                player->pendingweapon = wp_mauler;
            break;

        case am_missiles:
            if(player->weaponowned[wp_missile])
                player->pendingweapon = wp_missile;
            break;

        default:
            break;
        }
    }

    return true;
}


//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
// villsa [STRIFE] some stuff has been changed/moved around
//
boolean P_GiveWeapon(player_t* player, weapontype_t weapon, boolean dropped)
{
    boolean gaveammo;
    boolean gaveweapon;

    // villsa [STRIFE] new code for giving alternate version
    // of the weapon to player
    if(player->weaponowned[weapon])
        gaveweapon = false;
    else
    {
        gaveweapon = true;
        player->weaponowned[weapon] = true;

        // Alternate "sister" weapons that you also get as a bonus:
        switch(weapon)
        {
        case wp_elecbow:
            player->weaponowned[wp_poisonbow] = true;
            break;

        case wp_hegrenade:
            player->weaponowned[wp_wpgrenade] = true;
            break;

        case wp_mauler:
            player->weaponowned[wp_torpedo] = true;
            break;

        default:
            break;
        }

        // check for the standard weapons only
        if(weapon > player->readyweapon && weapon <= wp_sigil)
            player->pendingweapon = weapon;

    }

    if(netgame && (deathmatch != 2) && !dropped)
    {
        // leave placed weapons forever on net games
        if(!gaveweapon)
            return false;

        player->bonuscount += BONUSADD;
        player->weaponowned[weapon] = true;

        if(deathmatch)
            P_GiveAmmo(player, weaponinfo[weapon].ammo, 5);
        else
            P_GiveAmmo(player, weaponinfo[weapon].ammo, 2);

        if(player == &players[consoleplayer])
            S_StartSound (NULL, sfx_wpnup);
        return false;
    }

    if(weaponinfo[weapon].ammo != am_noammo)
    {
        // give one clip with a dropped weapon,
        // two clips with a found weapon
        if(dropped)
            gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 1);
        else
            gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
    }
    else
        gaveammo = false;

    return(gaveweapon || gaveammo);
}



//
// P_GiveBody
// Returns false if the body isn't needed at all
//
// villsa [STRIFE] a lot of changes have been added for stamina
//
boolean P_GiveBody(player_t* player, int num)
{
    int maxhealth;
    int healing;
    mobj_t *mo; // haleyjd 20110225: needed below...

    maxhealth = MAXHEALTH + player->stamina;

    if(num >= 0) // haleyjd 20100923: fixed to give proper amount of health
    {
        // any healing to do?
        if(player->health >= maxhealth)
            return false;

        // give, and cap to maxhealth
        player->health += num;
        if(player->health >= maxhealth)
            player->health = maxhealth;

        // Set mo->health for consistency.
        // haleyjd 20110225: Seems Strife can call this on a NULL player->mo
        // when giving items to players that are not in the game...
        mo = P_SubstNullMobj(player->mo);
        mo->health = player->health;
    }
    else
    {
        // [STRIFE] handle healing from the Front's medic
        // The amount the player's health will be set to scales up with stamina
        // increases.
        // Ex 1: On the wimpiest skill level, -100 is sent in. This restores 
        //       full health no matter what your stamina.
        //       (100*100)/100 = 100
        //       (200*100)/100 = 200
        // Ex 2: On the most stringent skill levels, -50 is sent in. This will
        //       restore at most half of your health.
        //       (100*50)/100 = 50
        //       (200*50)/100 = 100
        healing = (-num * maxhealth) / MAXHEALTH;

        // This is also the "threshold" of healing. You need less health than
        // the amount that will be restored in order to get any benefit.
        // So on the easiest skill you will always be fully healed.
        // On the hardest skill you must have less than 50 health, and will
        // only recover to 50 (assuming base stamina stat)
        if(player->health >= healing)
            return false;

        // Set health. BUG: Oddly, mo->health is NOT set here...
        player->health = healing;

        // haleyjd 20140817: [SVE] fix above problem.
        if(!classicmode)
        {
            mo = P_SubstNullMobj(player->mo);
            mo->health = player->health;
        }
    }

    return true;
}



//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
// [STRIFE] Modified for Strife armor items
//
boolean P_GiveArmor(player_t* player, int armortype)
{
    int hits;

    // villsa [STRIFE]
    if(armortype < 0)
    {
        if(player->armorpoints)
            return false;

        armortype = -armortype;
    }

    hits = armortype * 100;
    if(player->armorpoints >= hits)
        return false;   // don't pick up

    player->armortype = armortype;
    player->armorpoints = hits;

    return true;
}



//
// P_GiveCard
//
// [STRIFE] Modified to use larger bonuscount
//
boolean P_GiveCard(player_t* player, card_t card)
{
    if (player->cards[card])
        return false;
    
    // villsa [STRIFE] multiply by 2
    player->bonuscount = BONUSADD * 2;
    player->cards[card] = true;

    return true;
}


//
// P_GivePower
//
// [STRIFE] Modifications for new powerups
//
boolean P_GivePower(player_t* player, powertype_t power)
{
    // haleyjd 09/14/10: [STRIFE] moved to top, exception for Shadow Armor
    if(player->powers[power] && power != pw_invisibility)
        return false;	// already got it

    // if giving pw_invisibility and player already has MVIS, no can do.
    if(power == pw_invisibility && (player->mo->flags & MF_MVIS))
        return false;

    // villsa [STRIFE]
    if(power == pw_targeter)
    {
        player->powers[power] = TARGTICS;
        P_SetPsprite(player, ps_targcenter, S_TRGT_00); // 10
        P_SetPsprite(player, ps_targleft,   S_TRGT_01); // 11
        P_SetPsprite(player, ps_targright,  S_TRGT_02); // 12

        player->psprites[ps_targcenter].sx  = (160*FRACUNIT);
        player->psprites[ps_targleft  ].sy  = (100*FRACUNIT);
        player->psprites[ps_targcenter].sy  = (100*FRACUNIT);
        player->psprites[ps_targright ].sy  = (100*FRACUNIT);
        return true;
    }

    if(power == pw_invisibility)
    {
        // [SVE] 20160416: Chalice carriers cannot use Shadow Armor in CTC
        if(capturethechalice &&
           (P_PlayerHasItem(player, MT_INV_CHALICE) ||
            P_PlayerHasItem(player, MT_INV_BLUE_CHALICE)))
        {
            S_StartSound(player->mo, sfx_noway);
            return false;
        }

        // if player already had this power...
        if(player->powers[power])
        {
            // remove SHADOW, give MVIS.
            player->mo->flags &= ~MF_SHADOW;
            player->mo->flags |= MF_MVIS;
        }
        else // give SHADOW
            player->mo->flags |= MF_SHADOW;

        // set tics if giving shadow, or renew them if MVIS.
        player->powers[power] = INVISTICS;

        return true;
    }

    if(power == pw_ironfeet)
    {
        player->powers[power] = IRONTICS;
        return true;
    }

    if(power == pw_strength)
    {
        P_GiveBody(player, 100);
        player->powers[power] = 1;
        return true;
    }

    // villsa [STRIFE]
    if(power == pw_allmap)
    {
        // remember in mapstate
        if(gamemap < 40)
            player->mapstate[gamemap] = true;

        player->powers[power] = 1;
        return true;
    }

    // villsa [STRIFE]
    if(power == pw_communicator)
    {
        player->powers[power] = 1;
        return true;
    }

    // default behavior:
    player->powers[power] = 1;
    return true;
}

static char ctcbuffer[80];

//
// P_MessageAllPlayers
//
// haleyjd 20140920: [SVE] message all players
//
void P_MessageAllPlayers(char *message, int sfx_id)
{
    int i;
    for(i = 0; i < MAXPLAYERS; i++)
    {
        if(playeringame[i])
            players[i].message = message;
    }
    S_StartSound(NULL, sfx_id);
}

//
// P_returnChaliceToBase
//
// haleyjd 20140920: [SVE] Part of Capture the Chalice logic - if a player 
// touches their own team's chalice, return it to the respective base.
//
static void P_returnChaliceToBase(mobj_t *special, mobj_t *toucher)
{
    thinker_t *th;
    mobjtype_t type = NUMMOBJTYPES;

    if(special->type == MT_INV_CHALICE)
        type = MT_CTC_FLAGSPOT_RED;
    else if(special->type == MT_INV_BLUE_CHALICE)
        type = MT_CTC_FLAGSPOT_BLUE;
    else
        return; // eh??

    for(th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if(th->function.acp1 == (actionf_p1)P_MobjThinker)
        {
            mobj_t *mo = (mobj_t *)th;

            if(mo->type == type &&
               P_AproxDistance(mo->x - special->x, mo->y - special->y) > 64*FRACUNIT)
            {
                mobj_t *fog;
                fixed_t oldx = mo->x;
                fixed_t oldy = mo->y;
                fixed_t oldz = mo->z;
                fixed_t x = mo->x + 16 * finecosine[mo->angle >> ANGLETOFINESHIFT];
                fixed_t y = mo->y + 16 * finesine[mo->angle >> ANGLETOFINESHIFT];

                if(P_TeleportMove(special, x, y))
                {
                    fog = P_SpawnMobj(oldx, oldy, oldz, MT_TFOG);
                    S_StartSound(fog, sfx_telept);
                    fog = P_SpawnMobj(special->x, special->y, special->z, MT_TFOG);
                    S_StartSound(fog, sfx_telept);
                    M_snprintf(ctcbuffer, sizeof(ctcbuffer), "%s team chalice recovered!",
                        type == MT_CTC_FLAGSPOT_BLUE ? "Blue" : "Red");
                    P_MessageAllPlayers(ctcbuffer, sfx_yeah);
                }
                return;
            }
        }
    }
}

//
// P_stealChalice
//
// haleyjd 20140920: [SVE] Steal the chalice of the other team for CTC.
//
// "So, YOU'RE the fool who stole the chalice!"
//
static void P_stealChalice(mobj_t *special, mobj_t *toucher)
{
    if(P_GiveInventoryItem(toucher->player, special->sprite, special->type))
    {
        M_snprintf(ctcbuffer, sizeof(ctcbuffer), "%s team chalice stolen!",
            special->type == MT_INV_BLUE_CHALICE ? "Blue" : "Red");
        P_MessageAllPlayers(ctcbuffer, sfx_alarm);
        P_RemoveMobj(special);
        toucher->player->bonuscount += BONUSADD;

        // 20160416: cancel Shadow Armor from the chalice carrier
        if(toucher->player->powers[pw_invisibility])
        {
            toucher->player->powers[pw_invisibility] = 0;
            toucher->flags &= ~(MF_SHADOW|MF_MVIS);
        }
    }
}

//
// P_checkForCTCScore
//
// haleyjd 20140920: [SVE] Player is touching a Capture the Chalice flag base;
// see if a score should be given to the player's team.
//
static void P_checkForCTCScore(mobj_t *special, mobj_t *toucher)
{
    int i, score;
    thinker_t *th;
    mobj_t *myflag = NULL, *mybase = NULL;
    int myteam = toucher->player->allegiance;
    mobjtype_t mybasetype, myflagtype, otherflagtype;

    switch(myteam)
    {
    case CTC_TEAM_BLUE:
        mybasetype    = MT_CTC_FLAGSPOT_BLUE;
        myflagtype    = MT_INV_BLUE_CHALICE;
        otherflagtype = MT_INV_CHALICE;
        break;
    case CTC_TEAM_RED:
        mybasetype    = MT_CTC_FLAGSPOT_RED;
        myflagtype    = MT_INV_CHALICE;
        otherflagtype = MT_INV_BLUE_CHALICE;
        break;
    default:
        return; // borked.
    }

    // not touching your own base?
    if(special->type != mybasetype)
        return;

    // must possess other team's chalice in inventory
    for(i = 0; i < NUMINVENTORY; i++)
    {
        inventory_t *inv = &(toucher->player->inventory[i]);

        if(inv->type == otherflagtype && inv->amount >= 1)
            break;
    }
    if(i == NUMINVENTORY)
        return;

    // make sure own team's chalice is secure; it must be within 64 units 
    // of the team's flag base.
    for(th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if(th->function.acp1 == (actionf_p1)P_MobjThinker)
        {
            mobj_t *mo = (mobj_t *)th;
            if(mo->type == mybasetype)
                mybase = mo;
            else if(mo->type == myflagtype)
                myflag = mo;

            if(mybase && myflag)
                break;
        }
    }

    // If the base is missing, we are screwed. If the flag is missing, somebody
    // on the other team is probably toting it.
    if(!(mybase && myflag))
        return;

    // check if flag is too far from base
    if(P_AproxDistance(myflag->x - mybase->x, myflag->y - mybase->y) > 64*FRACUNIT)
        return;

    // award point
    switch(myteam)
    {
    case CTC_TEAM_BLUE:
        score = ++ctcbluescore;
        break;
    case CTC_TEAM_RED:
        score = ++ctcredscore;
        break;
    default:
        score = 0;
        break;
    }

    // own flag is at base, and touching own base with other team's flag; SCORE!
    M_snprintf(ctcbuffer, sizeof(ctcbuffer), "%s team scores! (%d/%d)",
               myteam == CTC_TEAM_BLUE ? "Blue" : "Red", score, ctcpointlimit);
    P_MessageAllPlayers(ctcbuffer, sfx_yeah);

    // take away other team's chalice from inventory; this will force the flag base
    // to respawn it.
    P_RemoveInventoryItem(toucher->player, i, 1);

    // check for score limit
    if(score >= ctcpointlimit)
        G_ExitLevel(0);
}

// villsa [STRIFE]
static char pickupmsg[80];

//
// P_CheckTalismans
//
// haleyjd [SVE]: For super secret level.
//
static void P_CheckTalismans(player_t *player, mobjtype_t type)
{
    const char *extra = "";

    // award talisman power
    if(!(player->cheats & CF_TALISMANPOWER))
    {
        if(P_PlayerHasItem(player, MT_AZTECARTI_RED) &&
           P_PlayerHasItem(player, MT_AZTECARTI_GREEN) &&
           P_PlayerHasItem(player, MT_AZTECARTI_BLUE))
        {
            player->cheats |= CF_TALISMANPOWER;
            S_StartSound(NULL, sfx_yeah);
            extra = " You have super strength!";

            // [SVE] svillarreal
            if(!P_CheckPlayersCheating(ACH_ALLOW_SP))
                gAppServices->SetAchievement("SVE_ACH_BESERK_REBORN");
        }
    }
    // Doom 64 tribute :)
    switch(type)
    {
    case MT_AZTECARTI_RED:
        M_snprintf(pickupmsg, sizeof(pickupmsg), 
                   "You have a feeling that it wasn't to be touched...%s", extra);
        break;
    case MT_AZTECARTI_GREEN:
        M_snprintf(pickupmsg, sizeof(pickupmsg), 
                   "Whatever it is, it doesn't belong in this world...%s", extra);
        break;
    case MT_AZTECARTI_BLUE:
        M_snprintf(pickupmsg, sizeof(pickupmsg),
                   "It must do something...%s", extra);
        break;
    default:
        break;
    }
}

//
// P_TouchSpecialThing
//
// [STRIFE] Rewritten for Strife collectables.
//
void P_TouchSpecialThing(mobj_t* special, mobj_t* toucher)
{
    player_t*   player;
    int         i;
    fixed_t     delta;
    int         sound;

    delta = special->z - toucher->z;

    if(delta > toucher->height || delta < -8*FRACUNIT)
        return; // out of reach

    sound = sfx_itemup;
    player = toucher->player;

    // Dead thing touching.
    // Can happen with a sliding player corpse.
    if(toucher->health <= 0)
        return;

    // villsa [STRIFE] damage toucher if special is spectral
    // haleyjd 09/21/10: corrected to test for SPECTRE thingtypes specifically
    switch(special->type)
    {
    case MT_SPECTRE_A:
    case MT_SPECTRE_B:
    case MT_SPECTRE_C:
    case MT_SPECTRE_D:
    case MT_SPECTRE_E:
    case MT_ENTITY:
    case MT_SUBENTITY:
        P_DamageMobj(toucher, NULL, NULL, 5);
        return;
    default:
        break;
    }

    // villsa [STRIFE]
    pickupmsg[0] = 0;

    // Identify by sprite.
    // villsa [STRIFE] new items
    switch(special->sprite)
    {
    // bullets
    case SPR_BLIT: // haleyjd: fixed missing MF_DROPPED check
        if(!P_GiveAmmo(player, am_bullets, !(special->flags & MF_DROPPED)))
            return;
        break;

    // box of bullets
    case SPR_BBOX:
        if(!P_GiveAmmo(player, am_bullets, 5))
            return;
        break;

    // missile
    case SPR_MSSL:
        if(!P_GiveAmmo(player, am_missiles, 1))
            return;
        break;

    // box of missiles
    case SPR_ROKT:
        if(!P_GiveAmmo(player, am_missiles, 5))
            return;
        break;

    // battery
    case SPR_BRY1:
        if(!P_GiveAmmo(player, am_cell, 1))
            return;
        break;

    // cell pack
    case SPR_CPAC:
        if(!P_GiveAmmo(player, am_cell, 5))
            return;
        break;

    // poison bolts
    case SPR_PQRL:
        if(!P_GiveAmmo(player, am_poisonbolts, 5))
            return;
        break;

    // electric bolts
    case SPR_XQRL:
        if(!P_GiveAmmo(player, am_elecbolts, 5))
            return;
        break;

    // he grenades
    case SPR_GRN1:
        if(!P_GiveAmmo(player, am_hegrenades, 1))
            return;
        break;

    // wp grenades
    case SPR_GRN2:
        if(!P_GiveAmmo(player, am_wpgrenades, 1))
            return;
        break;

    // rifle
    case SPR_RIFL:
        if(!P_GiveWeapon(player, wp_rifle, special->flags & MF_DROPPED))
            return;
        sound = sfx_wpnup; // haleyjd: SHK-CHK!
        break;

    // flame thrower
    case SPR_FLAM:
        if(!P_GiveWeapon(player, wp_flame, false))
            return;
        // haleyjd: gives extra ammo.
        P_GiveAmmo(player, am_cell, 3);
        sound = sfx_wpnup; // haleyjd: SHK-CHK!
        break;

    // missile launcher
    case SPR_MMSL:
        if(!P_GiveWeapon(player, wp_missile, false))
            return;
        sound = sfx_wpnup; // haleyjd: SHK-CHK!
        break;

    // grenade launcher
    case SPR_GRND:
        if(!P_GiveWeapon(player, wp_hegrenade, special->flags & MF_DROPPED))
            return;
        sound = sfx_wpnup; // haleyjd: SHK-CHK!
        break;

    // mauler
    case SPR_TRPD:
        if(!P_GiveWeapon(player, wp_mauler, false))
            return;
        sound = sfx_wpnup; // haleyjd: SHK-CHK!
        break;

    // electric bolt crossbow
    case SPR_CBOW:
        if(!P_GiveWeapon(player, wp_elecbow, special->flags & MF_DROPPED))
            return;
        sound = sfx_wpnup; // haleyjd: SHK-CHK!
        break;

    // haleyjd 09/21/10: missed case: THE SIGIL!
    case SPR_SIGL:
        if(!P_GiveWeapon(player, wp_sigil, special->flags & MF_DROPPED))
        {
            player->sigiltype = special->frame;
            return;
        }
        
        if(netgame)
            player->sigiltype = 4;

        player->pendingweapon = wp_sigil;
        player->st_update = true;
        if(deathmatch)
            return;
        sound = sfx_wpnup;
        break;

    // backpack
    case SPR_BKPK:
        if(!player->backpack)
        {
            for(i = 0; i < NUMAMMO; i++)
                player->maxammo[i] *= 2;

            player->backpack = true;
        }
        for(i = 0; i < NUMAMMO; i++)
            P_GiveAmmo(player, i, 1);
        break;

    // 1 Gold
    case SPR_COIN:
        P_GiveInventoryItem(player, SPR_COIN, MT_MONY_1);
        break;

    // 10 Gold
    case SPR_CRED:
        for(i = 0; i < 10; i++)
            P_GiveInventoryItem(player, SPR_COIN, MT_MONY_1);
        break;

    // 25 Gold
    case SPR_SACK:
        // haleyjd 09/21/10: missed code: if a SPR_SACK object has negative
        // health, it will give that much gold - STRIFE-TODO: verify
        if(special->health < 0)
        {
            for(i = special->health; i != 0; i++)
                P_GiveInventoryItem(player, SPR_COIN, MT_MONY_1);
        }
        else
        {
            for(i = 0; i < 25; i++)
                P_GiveInventoryItem(player, SPR_COIN, MT_MONY_1);
        }
        break;

    // 50 Gold
    case SPR_CHST:
        for(i = 0; i < 50; i++)
            P_GiveInventoryItem(player, SPR_COIN, MT_MONY_1);
        break;

    // Leather Armor
    case SPR_ARM1:
        if(!P_GiveArmor(player, -2))
            if(!P_GiveInventoryItem(player, special->sprite, special->type))
                pickupmsg[0] = '!';
        break;

    // Metal Armor
    case SPR_ARM2:
        if(!P_GiveArmor(player, -1))
            if(!P_GiveInventoryItem(player, special->sprite, special->type))
                pickupmsg[0] = '!';
        break;

    // All-map powerup
    case SPR_PMAP:
        if(!P_GivePower(player, pw_allmap))
            return;
        sound = sfx_yeah;
        break;

    // The Comm Unit - because you need Blackbird whining in your ear the
    // whole time and telling you how lost she is :P
    case SPR_COMM:
        if(!P_GivePower(player, pw_communicator))
            return;
        sound = sfx_yeah;
        break;

    // haleyjd 09/21/10: missed case - Shadow Armor; though, I do not know why
    // this has a case distinct from generic inventory items... Maybe it started
    // out as an auto-use-if-possible item much like regular armor...
    case SPR_SHD1:
        if(!P_GiveInventoryItem(player, SPR_SHD1, special->type))
            pickupmsg[0] = '!';
        break;

    case SPR_FLGR: // [SVE] red flag spot / red talisman
    case SPR_FLGB: // [SVE] blue flag spot / blue talisman
        if(capturethechalice)
        {
            P_checkForCTCScore(special, toucher);
            return;
        }
        else
        {
            if(P_GiveInventoryItem(player, special->sprite, special->type))
                P_CheckTalismans(player, special->type);
            else
                pickupmsg[0] = '!';
        }
        break;

    case SPR_FLGG: // [SVE] green talisman
        if(P_GiveInventoryItem(player, special->sprite, special->type))
            P_CheckTalismans(player, special->type);
        else
            pickupmsg[0] = '!';
        break;

    case SPR_RELC: // [SVE] red chalice
    case SPR_RELB: // [SVE] blue chalice
        if(capturethechalice)
        {
            if((special->sprite == SPR_RELC && player->allegiance == CTC_TEAM_RED) ||
               (special->sprite == SPR_RELB && player->allegiance == CTC_TEAM_BLUE))
            {
                // player is touching their own team's flag; if it is out of range, 
                // warp it back to base
                P_returnChaliceToBase(special, toucher);
                return;
            }
            else if((special->sprite == SPR_RELC && player->allegiance == CTC_TEAM_BLUE) ||
                    (special->sprite == SPR_RELB && player->allegiance == CTC_TEAM_RED))
            {
                // player is touching the OTHER team's chalice; steal it!
                P_stealChalice(special, toucher);
                return;
            }
            else
                return; // dunno, but you can't do anything to it.
        }
        // fall through to default case if not in Capture the Chalice mode

    // villsa [STRIFE] check default items
    case SPR_TOKN:
    default:
        if(special->type >= MT_KEY_BASE && special->type <= MT_NEWKEY5)
        {
            // haleyjd 09/21/10: Strife player still picks up keys that
            // he has already found. (break, not return)
            if(!P_GiveCard(player, special->type - MT_KEY_BASE))
                break; 
        }
        else
        {
            if(!P_GiveInventoryItem(player, special->sprite, special->type))
                pickupmsg[0] = '!';
        }
        break;
    }

    // villsa [STRIFE] set message
    if(!pickupmsg[0])
    {
        if(special->info->name)
        {
            DEH_snprintf(pickupmsg, sizeof(pickupmsg), 
                         "You picked up the %s.", DEH_String(special->info->name));
        }
        else
            DEH_snprintf(pickupmsg, sizeof(pickupmsg), "You picked up the item.");
    }
    // use the first character to indicate that the player is full on items
    else if(pickupmsg[0] == '!')
    {
        DEH_snprintf(pickupmsg, sizeof(pickupmsg), "You cannot hold any more.");
        player->message = pickupmsg;
        return;
    }

    if(special->flags & MF_GIVEQUEST)
    {
        // [STRIFE]: Award quest flag based on the thing's speed. Quest 8 was
        // apparently at some point given by the Broken Power Coupling, which is
        // why they don't want to award it if you have Quest 6 (which is 
        // acquired by destroying the Front's working power coupling). BUT, the 
        // broken coupling object's speed is NOT 8... it is 512*FRACUNIT. For
        // strict portability beyond the x86, we need to AND the operand by 31.
        if(special->info->speed != 8 || !(player->questflags & QF_QUEST6))
            player->questflags |= 1 << ((special->info->speed - 1) & 31);
    }


    // haleyjd 08/30/10: [STRIFE] No itemcount
    //if (special->flags & MF_COUNTITEM)
    //    player->itemcount++;

    P_RemoveMobj(special);
    player->message = pickupmsg;
    player->bonuscount += BONUSADD;

    if(player == &players[consoleplayer])
        S_StartSound(NULL, sound);
}

// villsa [STRIFE]
static char plrkilledmsg[80];

//
// KillMobj
//
// [STRIFE] Major modifications for drop types, no tic randomization, etc.
//
void P_KillMobj(mobj_t* source, mobj_t* target)
{
    mobjtype_t  item;
    mobj_t*     mo;
    line_t      junk;
    int         i;

    // villsa [STRIFE] corpse and dropoff are removed, but why when these two flags
    // are set a few lines later? watcom nonsense perhaps?
    target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_BOUNCE|MF_CORPSE|MF_DROPOFF);

    // villsa [STRIFE] unused
    /*
    if (target->type != MT_SKULL)
        target->flags &= ~MF_NOGRAVITY;
    */

    target->flags |= MF_CORPSE|MF_DROPOFF;
    target->height = FRACUNIT;  // villsa [STRIFE] set to fracunit instead of >>= 2

    if(source && source->player)
    {
        // count for intermission
        if(target->flags & MF_COUNTKILL)
            source->player->killcount++;

        if(target->player)
        {
            // [SVE]: use pretty player names
            char *srcName = HUlib_makePrettyPlayerName(source->player - players);
            char *dstName = HUlib_makePrettyPlayerName(target->player - players);
            source->player->frags[target->player-players]++;

            // villsa [STRIFE] new messages when fragging players
            DEH_snprintf(plrkilledmsg, sizeof(plrkilledmsg),
                         "%s killed %s",
                         srcName,  // fixed/changed for [SVE]
                         dstName);

            if(netgame)
                players[consoleplayer].message = plrkilledmsg;

            Z_Free(srcName);
            Z_Free(dstName);
        }
    }
    else if(!netgame && (target->flags & MF_COUNTKILL))
    {
        // count all monster deaths,
        // even those caused by other monsters
        players[0].killcount++;
    }
    
    if(target->player)
    {
        // count environment kills against you
        if(!source)
            target->player->frags[target->player-players]++;

        if(gamemap == 29 && !netgame)
        {
            // haleyjd 09/13/10: [STRIFE] Give player the bad ending.
            F_StartFinale();
            return;
        }

        // villsa [STRIFE] spit out inventory items when killed
        if(netgame)
        {
            int amount = 0;
            mobj_t* loot;
            int r = 0;

            while(1)
            {
                if(target->player->inventory[0].amount <= 0)
                    break;

                item = target->player->inventory[0].type;
                if(item == MT_MONY_1)
                {
                    loot = P_SpawnMobj(target->x, target->y, 
                                       target->z + (24*FRACUNIT), MT_MONY_25);

                    // [STRIFE] TODO - what the hell is it doing here?
                    loot->health =  target->player->inventory[0].amount;
                    loot->health = -target->player->inventory[0].amount;

                    amount = target->player->inventory[0].amount;
                }
                else
                {
                    loot = P_SpawnMobj(target->x, target->y, 
                                       target->z + (24*FRACUNIT), item);
                    amount = 1;
                }

                P_RemoveInventoryItem(target->player, 0, amount);
                r = P_Random();
                loot->momx += ((r & 7) - (P_Random() & 7)) << FRACBITS;
                loot->momy += ((P_Random() & 7) + 1) << FRACBITS;
                loot->flags |= MF_DROPPED;
            }
        }

        //target->flags &= ~MF_SOLID;
        target->player->playerstate = PST_DEAD;
        target->player->mo->momz += 5*FRACUNIT;  // [STRIFE]: small hop!
        P_DropWeapon(target->player);

        if(target->player == &players[consoleplayer]
           && automapactive)
        {
            // don't die in auto map,
            // switch view prior to dying
            AM_Stop ();
        }
    }

    // villsa [STRIFE] some modifications to setting states
    if(target->state != &states[S_BURN_23])
    {
        if(target->health == -6666)
            P_SetMobjState(target, S_DISR_00);  // 373
        else
        {
            int minhealth = target->info->spawnhealth;

            // [SVE] svillarreal - increase chance of gibbing
            if(d_maxgore)
                minhealth >>= 1;

            // haleyjd [STRIFE] 20160111: Rogue changed check from < to <=
            if(target->health <= -minhealth
                && target->info->xdeathstate)
                P_SetMobjState(target, target->info->xdeathstate);
            else
                P_SetMobjState(target, target->info->deathstate);
        }
    }

    // [SVE] svillarreal - achievements from killing bosses
    if(!P_CheckPlayersCheating(ACH_ALLOW_ANY))
    {
        if(gamemap == 9 && target->type == MT_PROGRAMMER)
            gAppServices->SetAchievement("SVE_ACH_CODE_MONKEY");

        if(gamemap == 16 && target->type == MT_SPECTRE_B)
            gAppServices->SetAchievement("SVE_ACH_BISHSLAPPED");

        if(gamemap == 12 && target->type == MT_SPECTRE_C)
            gAppServices->SetAchievement("SVE_ACH_ORACLE");

        if(gamemap == 10 && target->type == MT_SPECTRE_D)
            gAppServices->SetAchievement("SVE_ACH_MACIL");

        if(gamemap == 27 && target->type == MT_SPECTRE_E)
            gAppServices->SetAchievement("SVE_ACH_LOREMASTER");
    }

    // villsa [STRIFE] no death tics randomization

    // Drop stuff.
    // villsa [STRIFE] get item from dialog target
    item = P_DialogFind(target->type, target->miscdata)->dropitem;

    if(!item)
    {
        // villsa [STRIFE] drop default items
        switch(target->type)
        {
        case MT_ORACLE:
            item = MT_MEAT;
            break;

        case MT_PROGRAMMER:
            item = MT_SIGIL_A;
            break;

        case MT_PRIEST:
            item = MT_JUNK;
            break;

        case MT_BISHOP:
            item = MT_AMINIBOX;
            break;

        case MT_PGUARD:
        case MT_CRUSADER:
            item = MT_ACELL;
            break;

        case MT_RLEADER:
            item = MT_AAMMOBOX;
            break;

        case MT_GUARD1:
        case MT_REBEL1:
        case MT_SHADOWGUARD:
            item = MT_ACLIP;
            break;

        case MT_SPECTRE_B:
            item = MT_SIGIL_B;
            break;

        case MT_SPECTRE_C:
            item = MT_SIGIL_C;
            break;

        case MT_SPECTRE_D:
            item = MT_SIGIL_D;
            break;

        case MT_SPECTRE_E:
            item = MT_SIGIL_E;
            break;

        case MT_COUPLING:
            junk.tag = 225;
            EV_DoDoor(&junk, dr_close);

            junk.tag = 44;
            EV_DoFloor(&junk, lowerFloor);

            GiveVoiceObjective("VOC13", "LOG13", 0);

            item = MT_COUPLING_BROKEN;
            players[0].questflags |= (1 << (mobjinfo[MT_COUPLING].speed - 1));
            break;

        default:
            return;
        }
    }

    // handle special case for scripted target's dropped item
    switch(item)
    {
    case MT_TOKEN_SHOPCLOSE:
        junk.tag = 222;
        EV_DoDoor(&junk, dr_close);
        P_NoiseAlert(players[0].mo, players[0].mo);

        M_snprintf(plrkilledmsg, sizeof(plrkilledmsg),
                   "%s", DEH_String("You're dead!  You set off the alarm."));
        if(!deathmatch)
            players[consoleplayer].message = plrkilledmsg;

        return;

    case MT_TOKEN_PRISON_PASS:
        junk.tag = 223;
        EV_DoDoor(&junk, dr_open);
        return;

    case MT_TOKEN_DOOR3:
        junk.tag = 224;
        EV_DoDoor(&junk, dr_open);
        return;

    case MT_SIGIL_A:
    case MT_SIGIL_B:
    case MT_SIGIL_C:
    case MT_SIGIL_D:
    case MT_SIGIL_E:
        for(i = 0; i < MAXPLAYERS; i++)
        {
            if(!P_GiveWeapon(&players[i], wp_sigil, false))
            {
                if(players[i].sigiltype++ > 4)
                    players[i].sigiltype = 4;
            }

            // haleyjd 20110225: fixed these two assignments which Watcom munged
            // up in the assembly by pre-incrementing the pointer into players[]
            players[i].st_update = true;
            players[i].pendingweapon = wp_sigil;
        }
        return;

    case MT_TOKEN_ALARM:
        P_NoiseAlert(players[0].mo, players[0].mo);

        M_snprintf(plrkilledmsg, sizeof(plrkilledmsg),
                   "%s", DEH_String("You Fool!  You've set off the alarm"));
        if(!deathmatch)
            players[consoleplayer].message = plrkilledmsg;
        return;

    default:
        break;
    }

    // villsa [STRIFE] toss out item
    if(!deathmatch || !(mobjinfo[item].flags & MF_NOTDMATCH))
    {
        int r;

        mo = P_SpawnMobj(target->x, target->y, target->z + (24*FRACUNIT), item);
        r = P_Random();
        mo->momx += ((r & 7) - (P_Random() & 7)) << FRACBITS;
        r = P_Random();
        mo->momy += ((r & 7) - (P_Random() & 7)) << FRACBITS;
        mo->flags |= (MF_SPECIAL|MF_DROPPED);   // special versions of items
    }

    // [SVE] svillarreal
    if((target->type >= MT_GUARD1 && target->type <= MT_SHADOWGUARD) &&
        curignitemobj == target)
    {
        // reset the chain if 'fully' dead
        P_SetTarget(&curignitemobj, NULL);
        numignitechains = 0;
    }
}

//
// P_IsMobjBoss
//
// villsa [STRIFE] new function
//
static boolean P_IsMobjBoss(mobjtype_t type)
{
    switch(type)
    {
    case MT_PROGRAMMER:
    case MT_BISHOP:
    case MT_RLEADER:
    case MT_ORACLE:
    case MT_PRIEST:
        return true;

    default:
        return false;
    }
}


//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
// [STRIFE] Extensive changes for spectrals, fire damage, disintegration, and
//  a plethora of mobjtype-specific hacks.
//
void P_DamageMobj(mobj_t* target, mobj_t* inflictor, mobj_t* source, int damage)
{
    angle_t     ang;
    int         saved;
    player_t*   player;
    fixed_t     thrust;
    int         temp;

    if(!(target->flags & MF_SHOOTABLE) )
        return; // shouldn't happen...

    if(target->health <= 0)
        return;

    // haleyjd 20140827: [SVE] don't allow damaging or killing objects that hold
    // progress-blocking items that the player doesn't have yet and the things 
    // don't drop when killed.
    if(!classicmode && P_CheckForBlockingItems(target))
    {
        target->flags &= ~MF_NODIALOG;
        target->flags |= (target->info->flags & MF_NODIALOG);
        return;
    }

    player = target->player;

    // haleyjd 20140918: [SVE] no team damage in Capture the Chalice mode
    if(capturethechalice && player && damage < 10000)
    {
        if(source && source->player && player != source->player &&
           player->allegiance == source->player->allegiance)
            return;
        if(inflictor && inflictor->player && player != inflictor->player &&
           player->allegiance == inflictor->player->allegiance)
            return;
    }

    // villsa [STRIFE] unused - skullfly check (removed)

    // villsa [STRIFE] handle spectral stuff
    // notes on projectile health:
    // -2 == enemy spectral projectile
    // -1 == player spectral projectile

    // haleyjd 20110203: refactored completely
    if(inflictor && (inflictor->flags & MF_SPECTRAL))
    {
        // players aren't damaged by their own (or others???) sigils
        // STRIFE-TODO: verify in deathmatch
        if(target->type == MT_PLAYER && inflictor->health == -1)
            return;
        // enemies aren't damaged by enemy sigil attacks
        if((target->flags & MF_SPECTRAL) && inflictor->health == -2)
            return;
        // Macil2, Oracle, and Spectre C cannot be damaged by Sigil A
        switch(target->type)
        {
        case MT_RLEADER2:
        case MT_ORACLE:
        case MT_SPECTRE_C:
            // haleyjd: added source->player validity check for safety...
            if(source->player && source->player->sigiltype < 1)
                return;
        default:
            break;
        }
    }

    // villsa [STRIFE] new checks for various actors
    if(inflictor)
    {
        // Fire damage inflictors
        if(inflictor->type == MT_SFIREBALL || 
           inflictor->type == MT_C_FLAME   ||
           inflictor->type == MT_PFLAME)
        {
            temp = damage / 2;

            if(P_IsMobjBoss(target->type))
                damage /= 2;
            else if(inflictor->type == MT_PFLAME)
            {
                damage /= 2;
                // robots take very little damage
                if(target->flags & MF_NOBLOOD)
                    damage = temp / 2;
            }
        }
        else
        {
            switch(inflictor->type)
            {
            case MT_HOOKSHOT:
                // haleyjd 20110203: should use source, not inflictor
                ang = R_PointToAngle2(
                        target->x,
                        target->y,
                        source->x,
                        source->y) >> ANGLETOFINESHIFT;

                target->momx += FixedMul(finecosine[ang], (12750*FRACUNIT) / target->info->mass);
                target->momy += FixedMul(finesine[ang],   (12750*FRACUNIT) / target->info->mass);
                target->reactiontime += 10;

                temp = P_AproxDistance(target->x - source->x, target->y - source->y);
                temp /= target->info->mass;

                if(temp < 1)
                    temp = 1;

                target->momz = (source->z - target->z) / temp;
                break;

            case MT_POISARROW:
                // don't affect robots
                if(target->flags & MF_NOBLOOD)
                    return;

                // instant kill
                damage = target->health + 10;
                break;

            default:
                // Spectral retaliation, though this may in fact be unreachable
                // since non-spectral inflictors are mostly filtered out.
                if(target->flags & MF_SPECTRAL
                    && !(inflictor->flags & MF_SPECTRAL))
                {
                    P_SetMobjState(target, target->info->missilestate);
                    return; // take no damage
                }
                break;
            }
        }
    }

    // villsa [STRIFE] special cases for shopkeepers and macil
    if((target->type >= MT_SHOPKEEPER_W && target->type <= MT_SHOPKEEPER_M)
        || target->type == MT_RLEADER)
    {
        if(source)
            P_SetTarget(&target->target, source);

        P_SetMobjState(target, target->info->painstate);
        return;
    }

    // villsa [STRIFE] handle fieldguard damage
    if(target->type == MT_FIELDGUARD)
    {
        // degnin ores are only allowed to damage the fieldguard
        if(!inflictor || inflictor->type != MT_DEGNINORE)
            return;

        damage = target->health;
    }

    if(player && gameskill == sk_baby)
        damage >>= 1;   // take half damage in trainer mode


    // Some close combat weapons should not
    // inflict thrust and push the victim out of reach,
    // thus kick away unless using the chainsaw.
    if (inflictor
        && !(target->flags & MF_NOCLIP)
        && (!source
         || !source->player
         || source->player->readyweapon != wp_flame))
    {
        ang = R_PointToAngle2(inflictor->x,
                              inflictor->y,
                              target->x,
                              target->y);

        thrust = damage * (FRACUNIT>>3) * 100 / target->info->mass;

        // make fall forwards sometimes
        if(damage < 40
            && damage > target->health
            && target->z - inflictor->z > 64*FRACUNIT
            && (P_Random() & 1))
        {
            ang += ANG180;
            thrust *= 4;
        }

        ang >>= ANGLETOFINESHIFT;
        target->momx += FixedMul (thrust, finecosine[ang]);
        target->momy += FixedMul (thrust, finesine[ang]);
    }

    // player specific
    if(player)
    {
        // end of game hell hack
        if (target->subsector->sector->special == 11
            && damage >= target->health)
        {
            damage = target->health - 1;
        }

        // [SVE] svillarreal
        if(d_dmgindictor && player == &players[consoleplayer])
        {
            ST_AddDamageMarker(source);
        }

        // Below certain threshold,
        // ignore damage in GOD mode.
        // villsa [STRIFE] removed pw_invulnerability check
        if(damage < 1000 && (player->cheats & CF_GODMODE))
            return;

        // villsa [STRIFE] flame attacks don't damage player if wearing envirosuit
        if(player->powers[pw_ironfeet] && inflictor)
        {
            if(inflictor->type == MT_SFIREBALL || 
               inflictor->type == MT_C_FLAME   || 
               inflictor->type == MT_PFLAME)
            {
                damage = 0;
            }
        }

        if(player->armortype)
        {
            if (player->armortype == 1)
                saved = damage/3;
            else
                saved = damage/2;

            if(player->armorpoints <= saved)
            {
                // armor is used up
                saved = player->armorpoints;
                player->armortype = 0;

                // villsa [STRIFE]
                P_UseInventoryItem(player, SPR_ARM1);
                P_UseInventoryItem(player, SPR_ARM2);
            }
            player->armorpoints -= saved;
            damage -= saved;
        }
        player->health -= damage;   // mirror mobj health here for Dave
        
        // [STRIFE] haleyjd 20130302: bug fix - this is *not* capped here.
        //if(player->health < 0)
        //    player->health = 0;

        player->attacker = source;
        player->damagecount += damage;  // add damage after armor / invuln

        // haleyjd 20110203 [STRIFE]: target->target set here
        if(target != source)
            P_SetTarget(&target->target, source);

        if(player->damagecount > 100)
            player->damagecount = 100;  // teleport stomp does 10k points...

        temp = damage < 100 ? damage : 100;

        if(player == &players[consoleplayer])
            I_Tactile (40,10,40+temp*2);
    }

    // do the damage	
    target->health -= damage;

    // villsa [STRIFE] auto use medkits
    if(player && player->health < 50)
    {
        if(deathmatch || player->cheats & CF_AUTOHEALTH)
        {
            while(player->health < 50 && P_UseInventoryItem(player, SPR_MDKT));
            while(player->health < 50 && P_UseInventoryItem(player, SPR_STMP));
        }
    }


    if(target->health <= 0)
    {
        // villsa [STRIFE] grenades hurt... OUCH
        if(inflictor && inflictor->type == MT_HEGRENADE)
            target->health = -target->info->spawnhealth;
        else if(!(target->flags & MF_NOBLOOD))
        {
            // villsa [STRIFE] disintegration death
            if(inflictor &&
                (inflictor->type == MT_STRIFEPUFF3 || 
                 inflictor->type == MT_L_LASER     || 
                 inflictor->type == MT_TORPEDO     || 
                 inflictor->type == MT_TORPEDOSPREAD))
            {
                S_StartSound(target, sfx_dsrptr);
                target->health = -6666;
            }
        }

        // villsa [STRIFE] flame death stuff
        if(!(target->flags & MF_NOBLOOD)
            && inflictor
            && (inflictor->type == MT_SFIREBALL || 
                inflictor->type == MT_C_FLAME   || 
                inflictor->type == MT_PFLAME))
        {
            target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SHADOW|MF_MVIS);
            if(target->player)
            {
                target->player->cheats |= CF_ONFIRE;
                target->player->powers[pw_invisibility] = 0;
                // [SVE]: also clear the flags here.
                target->flags &= ~(MF_SHADOW|MF_MVIS);
                target->player->readyweapon = 0;
                P_SetPsprite(target->player, ps_weapon, S_WAVE_00); // 02
                P_SetPsprite(target->player, ps_flash, S_NULL);
            }

            // [SVE] svillarreal - Spontaneous Combustion achievement
            if((target->type >= MT_GUARD1 && target->type <= MT_SHADOWGUARD) &&
                inflictor->type == MT_SFIREBALL)
            {
                if(curignitemobj == NULL && inflictor->target &&
                    inflictor->target->player)
                {
                    P_SetTarget(&curignitemobj, target);
                    numignitechains = 0;
                }
                else if(inflictor == curignitemobj || !(inflictor->flags & MF_MISSILE))
                {
                    P_SetTarget(&curignitemobj, target);
                    numignitechains++;

                    if(numignitechains == 3)
                    {
                        if(!gAppServices->HasAchievement("SVE_ACH_FLAME_CHAIN") &&
                           !P_CheckPlayersCheating(ACH_ALLOW_SP))
                        {
                            if(players[consoleplayer].powers[pw_communicator] == true)
                                I_StartVoice(DEH_String("VOC219"));

                            gAppServices->SetAchievement("SVE_ACH_FLAME_CHAIN");
                        }
                        else if(players[consoleplayer].powers[pw_communicator] == true &&
                                !(M_Random() & 3) && !classicmode)
                        {
                            I_StartVoice(DEH_String("VOC219"));
                        }
                    }
                }
            }

            P_SetMobjState(target, S_BURN_00);  // 349
            S_StartSound(target, sfx_burnme);

            return;
        }
        
        P_KillMobj(source, target);
        return;
    }

    // villsa [STRIFE] set crash state
    if(target->health <= 6 && target->info->crashstate)
    {
        P_SetMobjState(target, target->info->crashstate);
        return;
    }

    if(damage)
    {
        // villsa [STRIFE] removed unused skullfly flag
        if(P_Random() < target->info->painchance) 
        {
            target->flags |= MF_JUSTHIT;    // fight back!
            P_SetMobjState (target, target->info->painstate);
        }
    }

    target->reactiontime = 0;       // we're awake now...

    // villsa [STRIFE] new checks for thing types
    if (target->type != MT_PROGRAMMER
        && (!target->threshold || target->type == MT_ENTITY)
        && source && source != target
        && source->type != MT_ENTITY
        && ((source->flags & MF_ALLY) != (target->flags & MF_ALLY)))
    {
        // if not intent on another player,
        // chase after this one
        P_SetTarget(&target->target, source);
        target->threshold = BASETHRESHOLD;

        if(target->state == &states[target->info->spawnstate]
           && target->info->seestate != S_NULL)
            P_SetMobjState (target, target->info->seestate);
    }
}

