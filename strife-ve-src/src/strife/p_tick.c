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
//	Archiving: SaveGame I/O.
//	Thinker, Ticker.
//


#include "z_zone.h"
#include "p_local.h"

#include "doomstat.h"

// [SVE] svillarreal
#include "rb_decal.h"


int leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//



// Both the head and tail of the thinker list.
thinker_t   thinkercap;


//
// P_InitThinkers
//
// [STRIFE] Verified unmodified
//
void P_InitThinkers (void)
{
    thinkercap.prev = thinkercap.next  = &thinkercap;
}




//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
// [STRIFE] Verified unmodified
//
void P_AddThinker (thinker_t* thinker)
{
    thinkercap.prev->next = thinker;
    thinker->next = &thinkercap;
    thinker->prev = thinkercap.prev;
    thinkercap.prev = thinker;

    thinker->references = 0; // haleyjd: [SVE]
}

// haleyjd 20140926: currentthinker external pointer
static thinker_t *currentthinker;

//
// P_RemoveThinkerDelayed
//
// haleyjd 20140926: [SVE] Need deferred freeing of thinkers
//
void P_RemoveThinkerDelayed(thinker_t *thinker)
{
    if(!thinker->references)
    {
        thinker_t *next = thinker->next;
        (next->prev = currentthinker = thinker->prev)->next = next;
        Z_Free(thinker);
    }
}

//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// [STRIFE] Verified unmodified
//
void P_RemoveThinker(thinker_t *thinker)
{
    // [SVE] set to deferred removal state
    thinker->function.acp1 = (actionf_p1)P_RemoveThinkerDelayed;
}

//
// P_SetTarget
//
// haleyjd 20140926: [SVE] Needed to maintain referential integrity.
//
void P_SetTarget(mobj_t **mop, mobj_t *target)
{
    if(*mop)
        (*mop)->thinker.references--;
    if((*mop = target))
        target->thinker.references++;
}

//
// P_RunThinkers
//
// [STRIFE] Verified unmodified
// [SVE]: Modifications for maintaince of referential integrity.
//
void P_RunThinkers (void)
{
    for(currentthinker = thinkercap.next;
        currentthinker != &thinkercap;
        currentthinker = currentthinker->next)
    {
        if(currentthinker->function.acp1)
            currentthinker->function.acp1(currentthinker);
    }
}

//
// P_Ticker
//
// [STRIFE] Menu pause behavior modified
//
void P_Ticker (void)
{
    int     i;
    
    // run the tic
    if (paused)
        return;

    // pause if in menu and at least one tic has been run
    // haleyjd 09/08/10 [STRIFE]: menuactive -> menupause
    if (!netgame 
        && menupause 
        && !demoplayback 
        && players[consoleplayer].viewz != 1)
    {
        return;
    }
    
    // haleyjd 20140904: [SVE] interpolation: save current sector heights
    P_SaveSectorPositions();

    for (i=0 ; i<MAXPLAYERS ; i++)
        if (playeringame[i])
            P_PlayerThink (&players[i]);

    P_RunThinkers ();
    P_UpdateSpecials ();
    P_RespawnSpecials ();

    // [SVE] svillarreal
    if(use3drenderer)
        RB_UpdateDecals();

    // for par times
    leveltime++;
}
