//
// Copyright(C) 2007-2014 Samuel Villarreal
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
//    Vis Sprites
//

#include "doomstat.h"
#include "r_state.h"
#include "rb_things.h"
#include "rb_geom.h"
#include "rb_drawlist.h"
#include "rb_view.h"
#include "rb_draw.h"
#include "rb_data.h"
#include "rb_config.h"
#include "rb_texture.h"
#include "rb_lightgrid.h"
#include "m_bbox.h"

#define MAX_SPRITES    1024

static rbVisSprite_t visspritelist[MAX_SPRITES];
static rbVisSprite_t *vissprite = NULL;

matrix rbSpriteViewMatrix;
matrix rbSpriteViewBillboardMatrix;

//
// RB_AddSpriteDrawlist
//

static void RB_AddSpriteDrawlist(rbVisSprite_t *vis, int texid)
{
    vtxlist_t *list;
    int translation = MOBJTRANSLATION(vis->spr);

    if(vis->spr->flags & (MF_SHADOW|MF_MVIS))
    {
        list = DL_AddVertexList(&drawlist[DLT_SPRITEALPHA]);
        list->data = (rbVisSprite_t*)vis;
        list->procfunc = RB_GenerateSpritePlane;
        list->preprocess = RB_PreProcessSprite;
        list->postprocess = 0;
        list->texid = texid;
        list->params = translation;
    }
    else
    {
        list = DL_AddVertexList(&drawlist[DLT_SPRITE]);
        list->data = (rbVisSprite_t*)vis;
        list->procfunc = RB_GenerateSpritePlane;
        list->preprocess = RB_PreProcessSprite;
        list->postprocess = 0;
        list->texid = texid;
        list->params = translation;
    }

    if(!(vis->spr->frame & FF_FULLBRIGHT || vis->spr->flags & (MF_SHADOW|MF_MVIS)) &&
        RB_GetBrightmap(RDT_SPRITE, texid, translation))
    {
        list = DL_AddVertexList(&drawlist[DLT_SPRITEBRIGHT]);
        list->data = (rbVisSprite_t*)vis;
        list->procfunc = RB_GenerateSpritePlane;
        list->preprocess = RB_PreProcessSprite;
        list->postprocess = 0;
        list->texid = texid;
        list->params = translation;
    }

    if((!classicmode && rbOutlineSprites) &&
        vis->spr->info->flags2 & MF2_DRAWOUTLINE && (leveltime & 0x8))
    {
        list = DL_AddVertexList(&drawlist[DLT_SPRITEOUTLINE]);
        list->data = (rbVisSprite_t*)vis;
        list->procfunc = RB_GenerateSpritePlane;
        list->preprocess = RB_PreProcessSprite;
        list->postprocess = 0;
        list->texid = texid;
        list->params = translation;
    }
}

//
// RB_AddSprites
//

void RB_AddSprites(subsector_t *sub)
{
    mobj_t *thing;
    fixed_t height;
    spritedef_t *sprdef;
    spriteframe_t *sprframe;
    fixed_t bbox[4];

    // Handle all things in sector.
    for(thing = sub->sector->thinglist; thing; thing = thing->snext)
    {
        if(thing->subsector != sub)
        {
            // don't add sprite if it doesn't belong in this subsector
            continue;
        }

        if(thing->flags & MF_NOSECTOR)
        {
            // must be linked to a sector
            continue;
        }

        if(vissprite - visspritelist >= MAX_SPRITES)
        {
            fprintf(stderr, "RB_AddSprites: Sprite overflow");
            return;
        }
        
        bbox[BOXRIGHT]  = thing->x + thing->radius;
        bbox[BOXLEFT]   = thing->x - thing->radius;
        bbox[BOXTOP]    = thing->y + thing->radius;
        bbox[BOXBOTTOM] = thing->y - thing->radius;

        sprdef = &sprites[thing->sprite];
        sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

        if(sprframe)
        {
            height = spriteheight[sprframe->lump[0]] + (16*FRACUNIT);

            if(height < thing->height)
            {
                height = thing->height;
            }
        }
        else
        {
            height = thing->height;
        }
        
        if(!RB_CheckBoxInView(&rbPlayerView, bbox, thing->z, thing->z + height))
        {
            continue;
        }

        vissprite->spr = thing;
        vissprite++;
    }
}

//
// RB_ClearSprites
//

void RB_ClearSprites(void)
{
    vissprite = visspritelist;
}

//
// RB_AddVisSprite
//

static void RB_AddVisSprite(rbVisSprite_t* vissprite)
{
    spritedef_t     *sprdef;
    spriteframe_t   *sprframe;
    angle_t         ang;
    int             spritenum;
    int             rot;
    mobj_t          *thing;

    thing = vissprite->spr;

    sprdef = &sprites[thing->sprite];
    sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];
    
    if(!sprframe)
    {
        return;
    }
    
    if(sprframe->rotate)
    {
        // choose a different rotation based on player view
        ang = RB_PointToAngle(thing->x - viewx, thing->y - viewy);
        rot = (ang-thing->angle + (unsigned)(ANG45 / 2) * 9) >> 29;
    }
    else
    {
        // use single rotation for all views
        rot = 0;
    }
    
    spritenum = sprframe->lump[rot];
    RB_AddSpriteDrawlist(vissprite, spritenum);
}

//
// RB_SetupSprites
//

void RB_SetupSprites(void)
{
    rbVisSprite_t *vis;
    matrix pitchRotation;
    spritepos_t pos;

    // setup view matrix for sprites so they always face the player's view
    MTX_IdentityZ(rbSpriteViewBillboardMatrix, DEG2RAD(TRUEANGLES(viewangle) - 90.0f));
    MTX_IdentityX(pitchRotation, rbPlayerView.viewpitch);

    MTX_Multiply(rbSpriteViewMatrix, pitchRotation, rbSpriteViewBillboardMatrix);

    for(vis = vissprite - 1; vis >= visspritelist; vis--)
    {
        R_interpolateThingPosition(vis->spr, &pos);

        vis->x = FIXED2FLOAT(pos.x);
        vis->y = FIXED2FLOAT(pos.y);
        vis->z = FIXED2FLOAT(pos.z);

        vis->indiceStart = 0;

        // determine distance from player's view
        vis->dist = (int)((vis->x - rbPlayerView.x) * rbPlayerView.rotyaw.c +
                          (vis->y - rbPlayerView.y) * rbPlayerView.rotyaw.s) / 2;

        // don't draw the player that we're viewing
        if(vis->spr->type == MT_PLAYER && vis->spr->player == &players[displayplayer])
        {
            continue;
        }

        RB_AddVisSprite(vis);
    }
}

//
// RB_SetSpriteCellColor
//

void RB_SetSpriteCellColor(vtx_t *v, fixed_t x, fixed_t y, fixed_t z, sector_t *sector)
{
    if(rbLightmaps && lightgrid.count > 0)
    {
        int lightGridIndex;
        
        if((lightGridIndex = RB_GetLightGridIndex(x, y, z)) != -1)
        {
            boolean insky = sector->ceilingpic == skyflatnum;
            
            RB_ApplyLightGridRGB(&v[0], lightGridIndex, insky);
            RB_ApplyLightGridRGB(&v[1], lightGridIndex, insky);
            RB_ApplyLightGridRGB(&v[2], lightGridIndex, insky);
            RB_ApplyLightGridRGB(&v[3], lightGridIndex, insky);
        }
    }
}
