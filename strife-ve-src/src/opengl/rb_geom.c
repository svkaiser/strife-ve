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
//    Creation of world geometry
//

#include <math.h>

#include "doomstat.h"
#include "rb_main.h"
#include "rb_view.h"
#include "rb_things.h"
#include "rb_geom.h"
#include "rb_data.h"
#include "rb_texture.h"
#include "rb_draw.h"
#include "rb_config.h"
#include "rb_matrix.h"
#include "rb_wallshade.h"
#include "rb_lightgrid.h"
#include "rb_dynlights.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_state.h"

//=============================================================================
//
// Seg Geometry
//
//=============================================================================

//
// RB_SetSegColor
//

static void RB_SetSegColor(vtxlist_t *vl, seg_t *seg, vtx_t *vtx, boolean forceAlpha, rbWallSide_t side)
{
    int i, lightLevel;
    byte alpha;
    
    if(vl->drawTag == DLT_BRIGHT)
    {
        RB_SetVertexColor(vtx, 0xff, 0xff, 0xff, 0xff, 4);
        return;
    }

    lightLevel = vl->params + (rbPlayerView.extralight << 4);

    if(rbLightmaps && lightmapCount > 0)
    {
        // contrast against the sunlight direction
        float f = seg->linedef->fnx * sunlightdir2d[0] + seg->linedef->fny * sunlightdir2d[1];
        
        if(seg->backsector && (seg->sidedef - sides) == seg->linedef->sidenum[1])
        {
            // flip scalar if backside of line is facing the view
            f = -f;
        }
        
        lightLevel += MAX(MIN((int)floorf(32.0f * f), 32), -32);
    }
    else
    {
        // do fake contrast
        if(seg->v1->y == seg->v2->y)
        {
            lightLevel -= 32;
        }
        else if(seg->v1->x == seg->v2->x)
        {
            lightLevel += 32;
        }
    }

    // clamp
    if(lightLevel > 255) lightLevel = 255;
    if(lightLevel <   0) lightLevel = 0;

    // get real light color
    lightLevel = rbSectorLightTable[lightLevel];
    alpha = 255;

    if(forceAlpha && seg->backsector)
    {
        if(seg->linedef->flags & ML_TRANSPARENT1)
        {
            alpha = 64;
        }
        else if(seg->linedef->flags & ML_TRANSPARENT2)
        {
            alpha = 192;
        }
    }

    if(!rbWallShades || !RB_SetWallShade(vtx, lightLevel, seg, side))
    {
        // set the default color
        for(i = 0; i < 4; i++)
        {
            vtx[i].r = lightLevel;
            vtx[i].g = lightLevel;
            vtx[i].b = lightLevel;
        }
    }

    vtx[0].a = vtx[1].a = vtx[2].a = vtx[3].a = alpha;
}

//
// RB_GetSideTopBottom
//

void RB_GetSideTopBottom(sector_t *sector, float *top, float *bottom)
{
    *top    = FIXED2FLOAT(sector->ceilingheight);
    *bottom = FIXED2FLOAT(sector->floorheight);
}

//
// RB_PreProcessSeg
//

boolean RB_PreProcessSeg(vtxlist_t* vl)
{
    seg_t *seg = (seg_t*)vl->data;
    rbTexture_t *texture;
    
    if(vl->drawTag == DLT_BRIGHT || vl->drawTag == DLT_BRIGHTMASKED)
    {
        if((texture = RB_GetBrightmap(RDT_COLUMN, vl->texid, 0)))
        {
            RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
            RB_BindTexture(texture);
            return true;
        }
    }

    if(seg->linedef->flags & (ML_TRANSPARENT1|ML_TRANSPARENT2))
    {
        RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
    }

    if((texture = RB_GetTexture(RDT_COLUMN, vl->texid, 0)))
    {
        RB_BindTexture(texture);
        RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);
    }

    return true;
}

//
// RB_GenerateLowerSeg
//

boolean RB_GenerateLowerSeg(vtxlist_t *vl, int *drawcount)
{
    line_t      *linedef;
    side_t      *sidedef;
    vtx_t       *v;
    float       top;
    float       bottom;
    float       btop;
    float       bbottom;
    int         height;
    int         width;
    float       length;
    float       rowoffs;
    float       coloffs;
    seg_t       *seg;
    sector_t    *sec;
    
    seg = (seg_t*)vl->data;
    
    if(!seg)
    {
        return false;
    }
    
    sec = seg->frontsector;
    
    v = &drawVertex[*drawcount];
    
    linedef = seg->linedef;
    sidedef = seg->sidedef;
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    length = seg->length;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);
    
    if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
    {
        btop = top;
    }
    
    if(bottom < bbottom)
    {
        rbTexture_t *texture;

        v[0].z = v[1].z = bbottom;
        v[2].z = v[3].z = bottom;
        
        RB_SetSegColor(vl, seg, v, false, WS_LOWER);

        texture = RB_GetTexture(RDT_COLUMN, texturetranslation[sidedef->bottomtexture], 0);
        width = texture->width;
        height = texture->height;
        
        rowoffs = FIXED2FLOAT(sidedef->rowoffset) / height;
        coloffs = FIXED2FLOAT(sidedef->textureoffset + seg->offset) / width;
        
        v[0].tu = v[2].tu = coloffs;
        v[1].tu = v[3].tu = length / width + coloffs;
        
        if(linedef->flags & ML_DONTPEGBOTTOM)
        {
            v[0].tv = v[1].tv = rowoffs + (top - bbottom) / height;
            v[2].tv = v[3].tv = rowoffs + (top - bottom) / height;
        }
        else
        {
            v[0].tv = v[1].tv = rowoffs;
            v[2].tv = v[3].tv = rowoffs + (bbottom - bottom) / height;
        }
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

        *drawcount += 4;
        return true;
    }
    
    return false;
}

//
// RB_GenerateUpperSeg
//

boolean RB_GenerateUpperSeg(vtxlist_t *vl, int *drawcount)
{
    line_t      *linedef;
    side_t      *sidedef;
    vtx_t       *v;
    float       top;
    float       bottom;
    float       btop;
    float       bbottom;
    int         height;
    int         width;
    float       length;
    float       rowoffs;
    float       coloffs;
    seg_t       *seg;
    sector_t    *sec;
    
    seg = (seg_t*)vl->data;
    
    if(!seg)
    {
        return false;
    }
    
    sec = seg->frontsector;
    
    v = &drawVertex[*drawcount];
    
    linedef = seg->linedef;
    sidedef = seg->sidedef;
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    length = seg->length;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);
    
    if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
    {
        btop = top;
    }
    
    if(top > btop)
    {
        rbTexture_t *texture;

        v[0].z = v[1].z = top;
        v[2].z = v[3].z = btop;

        RB_SetSegColor(vl, seg, v, false, WS_UPPER);

        texture = RB_GetTexture(RDT_COLUMN, texturetranslation[sidedef->toptexture], 0);
        width = texture->width;
        height = texture->height;
        
        rowoffs = FIXED2FLOAT(sidedef->rowoffset) / height;
        coloffs = FIXED2FLOAT(sidedef->textureoffset + seg->offset) / width;
        
        v[0].tu = v[2].tu = coloffs;
        v[1].tu = v[3].tu = length / width + coloffs;
        
        if(linedef->flags & ML_DONTPEGTOP)
        {
            v[0].tv = v[1].tv = 1 + rowoffs;
            v[2].tv = v[3].tv = 1 + rowoffs + (top - btop) / height;
        }
        else
        {
            v[2].tv = v[3].tv = 1 + rowoffs;
            v[0].tv = v[1].tv = 1 + rowoffs - (top - btop) / height;
        }
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

        *drawcount += 4;
        return true;
    }
    
    return false;
}

//
// RB_GenerateMiddleSeg
//

boolean RB_GenerateMiddleSeg(vtxlist_t *vl, int *drawcount)
{
    line_t      *linedef;
    side_t      *sidedef;
    vtx_t       *v;
    float       top;
    float       bottom;
    float       btop;
    float       bbottom;
    int         height;
    int         width;
    float       length;
    float       rowoffs;
    float       coloffs;
    seg_t       *seg;
    sector_t    *sec;
    rbTexture_t *texture;
    
    seg = (seg_t*)vl->data;
    
    if(!seg)
    {
        return false;
    }
    
    sec = seg->frontsector;
    
    v = &drawVertex[*drawcount];
    
    btop = 0;
    bbottom = 0;
    
    linedef = seg->linedef;
    sidedef = seg->sidedef;
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    length = seg->length;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);

    texture = RB_GetTexture(RDT_COLUMN, texturetranslation[sidedef->midtexture], 0);
    width = texture->width;
    height = texture->height;
    
    if(seg->backsector)
    {
        float tmpb, tmpt;

        RB_SetSegColor(vl, seg, v, true, WS_MIDDLEBACK);
        RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);

        if(bottom > btop || top < bbottom)
        {
            return false;
        }

        tmpb = bottom;
        tmpt = top;

        if(linedef->flags & ML_DONTPEGBOTTOM)
        {
            if(bottom + height < top)
            {
                top = MAX(bottom, bbottom) + height;

                if(top > tmpt)
                {
                    top = tmpt;
                }
            }
        }
        else if(top - height > bottom)
        {
            bottom = MIN(top, btop) - height;

            if(bottom < tmpb)
            {
                bottom = tmpb;
            }
        }

        if(top > btop)
        {
            top = btop;
        }

        if(bottom < bbottom)
        {
            bottom = bbottom;
        }
    }
    else
    {
        RB_SetSegColor(vl, seg, v, true, WS_MIDDLE);
    }
    
    v[0].z = v[1].z = top;
    v[2].z = v[3].z = bottom;
    
    rowoffs = FIXED2FLOAT(sidedef->rowoffset) / height;
    coloffs = FIXED2FLOAT(sidedef->textureoffset + seg->offset) / width;
    
    v[0].tu = v[2].tu = coloffs;
    v[1].tu = v[3].tu = length / width + coloffs;
    
    if(linedef->flags & ML_DONTPEGBOTTOM)
    {
        v[0].tv = v[1].tv = 1 + rowoffs - (top - bottom) / height;
        v[2].tv = v[3].tv = 1 + rowoffs;
    }
    else
    {
        v[0].tv = v[1].tv = rowoffs;
        v[2].tv = v[3].tv = rowoffs + (top - bottom) / height;
    }
    
    RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

    *drawcount += 4;
    return true;
}

//=============================================================================
//
// Subsector Geometry
//
//=============================================================================

//
// RB_PreProcessSubsector
//

boolean RB_PreProcessSubsector(vtxlist_t* vl)
{
    rbTexture_t *texture;
    
    if(vl->drawTag == DLT_BRIGHT)
    {
        if((texture = RB_GetBrightmap(RDT_FLAT, vl->texid, 0)))
        {
            RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
            RB_BindTexture(texture);
            return true;
        }
    }
    
    if((texture = RB_GetTexture(RDT_FLAT, vl->texid, 0)))
    {
        RB_BindTexture(texture);
        RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);
    }
    
    return true;
}

//
// RB_GenerateSubSectors
//

boolean RB_GenerateSubSectors(vtxlist_t *vl, int *drawcount)
{
    int j;
    int count;
    fixed_t tx;
    fixed_t ty;
    leaf_t *leaf;
    subsector_t *ss;
    sector_t *sector;
    byte cr, cg, cb, fr, fg, fb;
    
    ss      = (subsector_t*)vl->data;
    leaf    = &leafs[ss->leaf];
    sector  = ss->sector;
    count   = *drawcount;

    if(vl->drawTag != DLT_BRIGHT)
    {
        cr = cg = cb = fr = fg = fb =
            rbSectorLightTable[vl->params + (rbPlayerView.extralight << 4)];

        if(rbWallShades)
        {
            RB_GetFloorShade(sector, &fr, &fg, &fb);
            RB_GetCeilingShade(sector, &cr, &cg, &cb);
        }
    }
    else
    {
        cr = cg = cb = 0xff;
        fr = fg = fb = 0xff;
    }
    
    
    for(j = 0; j < ss->numleafs - 2; ++j)
    {
        RB_AddTriangle(count, count + 1 + j, count + 2 + j);
    }
    
    // need to keep texture coords small to avoid
    // floor 'wobble' due to rounding errors on some cards
    // make relative to first vertex, not (0,0)
    // which is arbitary anyway
    
    tx = (leaf->vertex->x >> 6) & ~(FRACUNIT - 1);
    ty = (leaf->vertex->y >> 6) & ~(FRACUNIT - 1);
    
    for(j = 0; j < ss->numleafs; ++j)
    {
        vtx_t *v = &drawVertex[count];
        
        if(vl->flags & DLF_CEILING)
        {
            leaf = &leafs[(ss->leaf + (ss->numleafs - 1)) - j];
            v->r = cr;
            v->g = cg;
            v->b = cb;
        }
        else
        {
            leaf = &leafs[ss->leaf + j];
            v->r = fr;
            v->g = fg;
            v->b = fb;
        }
        
        v->x = leaf->vertex->fx;
        v->y = leaf->vertex->fy;
        
        if(vl->flags & DLF_CEILING)
        {
            v->z = FIXED2FLOAT(sector->ceilingheight);
        }
        else
        {
            v->z = FIXED2FLOAT(sector->floorheight);
        }
        
        v->tu = FIXED2FLOAT((leaf->vertex->x >> 6) - tx);
        v->tv = -FIXED2FLOAT((leaf->vertex->y >> 6) - ty);
        
        v->a = 0xff;
        
        count++;
    }
    
    *drawcount = count;
    return true;
}

//=============================================================================
//
// Sprite Geometry
//
//=============================================================================

//
// RB_PreProcessSprite
//

boolean RB_PreProcessSprite(vtxlist_t* vl)
{
    rbVisSprite_t   *vissprite;
    mobj_t          *thing;
    rbTexture_t     *texture;
    int             translation;

    vissprite = (rbVisSprite_t*)vl->data;
    thing = vissprite->spr;
    translation = vl->params;
    texture = NULL;

    switch(vl->drawTag)
    {
    case DLT_SPRITEBRIGHT:
        texture = RB_GetBrightmap(RDT_SPRITE, vl->texid, translation);
        break;
        
    case DLT_SPRITEOUTLINE:
        texture = RB_GetSpriteOutlineTexture(vl->texid);
        break;

    default:
        texture = RB_GetTexture(RDT_SPRITE, vl->texid, translation);
        break;
    }

    if(texture)
    {
        RB_BindTexture(texture);
        RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);
    }
    return true;
}

//
// RB_GenerateSpritePlane
//

boolean RB_GenerateSpritePlane(vtxlist_t* vl, int* drawcount)
{
    float           x, y, z;
    spritedef_t     *sprdef;
    spriteframe_t   *sprframe;
    angle_t         ang;
    byte            alpha;
    int             i;
    int             spritenum;
    int             lightlevel = 0xff;
    int             rot;
    float           dx1, dx2;
    float           tx;
    float           ty;
    float           yoffs;
    mobj_t          *thing;
    float           offs;
    float           topoffset;
    float           height;
    rbVisSprite_t   *vissprite;
    float           centerz;
    vtx_t           *vertex;
    rbTexture_t     *texture;
    matrix          rotation;
    boolean         drawOutline;

    vissprite = (rbVisSprite_t*)vl->data;
    thing = vissprite->spr;

    x = vissprite->x;
    y = vissprite->y;
    z = vissprite->z;

    vissprite->indiceStart = *drawcount;

    vertex = &drawVertex[*drawcount];

    sprdef = &sprites[thing->sprite];
    sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

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
    texture = RB_GetTexture(RDT_SPRITE, spritenum, 0);

    tx = (float)texture->origwidth / (float)texture->width;
    ty = (float)texture->origheight / (float)texture->height;

    // flip sprite if needed
    if(sprframe->flip[rot])
    {
        offs = tx;
    }
    else
    {
        offs = 0.0f;
    }

    drawOutline = (vl->drawTag == DLT_SPRITEOUTLINE && thing->info->flags2 & MF2_DRAWOUTLINE);

    if(vl->drawTag != DLT_SPRITEBRIGHT)
    {
        if(drawOutline)
        {
            RB_SetVertexColor(vertex, 0xff, 0, 0, 0xff, 4);
        }
        else
        {
            if(thing->frame & FF_FULLBRIGHT)
            {
                lightlevel = 0xff;
            }
            else
            {
                int sectorlight = (rbLightmaps && thing->subsector->sector->altlightlevel != -1) ?
                thing->subsector->sector->altlightlevel :
                thing->subsector->sector->lightlevel;
                
                lightlevel = sectorlight + (rbPlayerView.extralight << 4);
                lightlevel += 32;

                if(lightlevel > 255)
                {
                    lightlevel = 255;
                }
            }

            // haleyjd 20140826: [SVE] corrected logic:
            // * MF_SHADOW things are: ~25% translucent UNLESS...
            // * They are also MF_MVIS and NOT translated, in which case they are ~75%.
            alpha = 0xff;
            if(thing->flags & MF_SHADOW)
            {
                alpha = 64;
                if((thing->flags & MF_MVIS) && !(thing->flags & MF_TRANSLATION))
                {
                    alpha = 192;
                }
            }
            else if(thing->flags & MF_MVIS)
                alpha = 24; // ALMOST totally invisible.

            RB_SetVertexColor(vertex, rbSectorLightTable[lightlevel],
                                      rbSectorLightTable[lightlevel],
                                      rbSectorLightTable[lightlevel], alpha, 4);
        }
    }

    // set offset
    if(sprframe->flip[rot])
    {
        dx1 = FIXED2FLOAT(spriteoffset[spritenum]) - FIXED2FLOAT(spritewidth[spritenum]);
    }
    else
    {
        dx1 = -FIXED2FLOAT(spriteoffset[spritenum]);
    }

    dx2 = dx1 + FIXED2FLOAT(spritewidth[spritenum]);

    topoffset = FIXED2FLOAT(spritetopoffset[spritenum]);
    height = FIXED2FLOAT(spriteheight[spritenum]);

    yoffs = 0;

    if(vissprite->spr->flags & MF_FEETCLIPPED)
    {
        topoffset -= 10.0f;

        if(rbFixSpriteClipping)
        {
            yoffs = (ty / 0.1f) / height;
            height -= 10.0f;
        }
    }

    if(drawOutline)
    {
        dx1 -= 2.0f;
        dx2 += 2.0f;
    }

    // setup texture mapping
    vertex[0].tu = vertex[2].tu = offs;
    vertex[1].tu = vertex[3].tu = tx - offs;
    vertex[0].tv = vertex[1].tv = 0.0f;
    vertex[2].tv = vertex[3].tv = ty - yoffs;

    // rotate sprite's pitch from the center of the plane
    centerz = height * 0.5f;

    // setup vertex coordinates
    vertex[0].x = vertex[2].x = dx1;
    vertex[0].y = vertex[2].y = 0;
    vertex[1].x = vertex[3].x = dx2;
    vertex[1].y = vertex[3].y = 0;

    vertex[0].z = vertex[1].z = topoffset - centerz;
    vertex[2].z = vertex[3].z = vertex[0].z - height;

    if(drawOutline)
    {
        vertex[0].z = vertex[1].z = vertex[0].z + 2.0f;
        vertex[2].z = vertex[3].z = vertex[2].z - 2.0f;
    }

    if(thing->info->flags2 & MF2_DRAWBILLBOARD)
    {
        // sprite won't rotate along the x-axis
        MTX_Copy(rotation, rbSpriteViewBillboardMatrix);
    }
    else
    {
        // always face the player view
        MTX_Copy(rotation, rbSpriteViewMatrix);
    }

    MTX_SetTranslation(rotation, 0, 0, centerz);

    MTX_MultiplyVector(rotation, (float*)&vertex[0].x);
    MTX_MultiplyVector(rotation, (float*)&vertex[1].x);
    MTX_MultiplyVector(rotation, (float*)&vertex[2].x);
    MTX_MultiplyVector(rotation, (float*)&vertex[3].x);

    for(i = 0; i < 4; ++i)
    {
        vertex[i].x += x;
        vertex[i].y += y;
        vertex[i].z += z;
    }

    if(vl->drawTag != DLT_SPRITEBRIGHT)
    {
        if(rbWallShades)
        {
            if(!(drawOutline || (thing->frame & FF_FULLBRIGHT)))
            {
                RB_SetThingShade(thing, vertex, rbSectorLightTable[lightlevel]);
            }
        }

        if(!drawOutline && !(thing->flags & (MF_SHADOW|MF_MVIS)) && !(thing->frame & FF_FULLBRIGHT))
        {
            RB_SetSpriteCellColor(vertex, thing->x, thing->y, thing->z + (64*FRACUNIT),
                                  thing->subsector->sector);
        }
    }
    else
    {
        RB_SetVertexColor(vertex, 0xff, 0xff, 0xff, 0xff, 4);
    }

    RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);
    
    *drawcount += 4;

    return true;
}

//=============================================================================
//
// Clipline Geometry
//
//=============================================================================

//
// RB_PreProcessClipLine
//

boolean RB_PreProcessClipLine(vtxlist_t* vl)
{
    RB_BindTexture(&whiteTexture);
    return true;
}

//
// RB_GenerateClipLine
//

boolean RB_GenerateClipLine(vtxlist_t *vl, int *drawcount)
{
    vtx_t *v;
    float x1, y1;
    float x2, y2;
    line_t *line;
    int vtxCount;

    v = &drawVertex[*drawcount];
    vtxCount = 0;
    line = (line_t*)vl->data;

    x1 = line->v1->fx;
    y1 = line->v1->fy;
    x2 = line->v2->fx;
    y2 = line->v2->fy;

    if(!line->backsector)
    {
        v[vtxCount+0].x = v[vtxCount+2].x = x1;
        v[vtxCount+0].y = v[vtxCount+2].y = y1;
        v[vtxCount+1].x = v[vtxCount+3].x = x2;
        v[vtxCount+1].y = v[vtxCount+3].y = y2;
        v[vtxCount+0].z = v[vtxCount+1].z = MAX_COORD;
        v[vtxCount+2].z = v[vtxCount+3].z = -MAX_COORD;

        RB_SetVertexColor(&v[vtxCount], 0xff, 0xff, 0xff, 0xff, 4);
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

        *drawcount += 4;
        vtxCount += 4;
    }
    else
    {
        if(vl->params != 1 && line->frontsector->ceilingheight != line->backsector->ceilingheight)
        {
            float height;

            if(line->backsector->ceilingheight >= line->frontsector->ceilingheight)
            {
                height = FIXED2FLOAT(line->frontsector->ceilingheight);
            }
            else
            {
                height = FIXED2FLOAT(line->backsector->ceilingheight);
            }

            v[vtxCount+0].x = v[vtxCount+2].x = x1;
            v[vtxCount+0].y = v[vtxCount+2].y = y1;
            v[vtxCount+1].x = v[vtxCount+3].x = x2;
            v[vtxCount+1].y = v[vtxCount+3].y = y2;
            v[vtxCount+0].z = v[vtxCount+1].z = MAX_COORD;
            v[vtxCount+2].z = v[vtxCount+3].z = height;

            RB_SetVertexColor(&v[vtxCount], 0xff, 0xff, 0xff, 0xff, 4);

            RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
            RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

            *drawcount += 4;
            vtxCount += 4;
        }

        if(line->frontsector->floorheight != line->backsector->floorheight)
        {
            float height;

            if(line->backsector->floorheight < line->frontsector->floorheight)
            {
                if(line->frontsector->floorheight < viewz &&
                    line->frontsector->floorheight != line->frontsector->ceilingheight)
                {
                    if(P_PointOnLineSide(viewx, viewy, line) == 1)
                    {
                        height = FIXED2FLOAT(line->backsector->floorheight);
                    }
                    else
                    {
                        height = FIXED2FLOAT(line->frontsector->floorheight);
                    }
                }
                else
                {
                    height = FIXED2FLOAT(line->frontsector->floorheight);
                }
            }
            else
            {
                if(line->backsector->floorheight < viewz &&
                    line->backsector->floorheight != line->backsector->ceilingheight)
                {
                    if(P_PointOnLineSide(viewx, viewy, line) == 0)
                    {
                        height = FIXED2FLOAT(line->frontsector->floorheight);
                    }
                    else
                    {
                        height = FIXED2FLOAT(line->backsector->floorheight);
                    }
                }
                else
                {
                    height = FIXED2FLOAT(line->backsector->floorheight);
                }
            }

            v[vtxCount+0].x = v[vtxCount+2].x = x1;
            v[vtxCount+0].y = v[vtxCount+2].y = y1;
            v[vtxCount+1].x = v[vtxCount+3].x = x2;
            v[vtxCount+1].y = v[vtxCount+3].y = y2;
            v[vtxCount+0].z = v[vtxCount+1].z = height;
            v[vtxCount+2].z = v[vtxCount+3].z = -MAX_COORD;

            RB_SetVertexColor(&v[vtxCount], 0xff, 0xff, 0xff, 0xff, 4);

            RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
            RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

            *drawcount += 4;
            vtxCount += 4;
        }
    }

    return true;
}

//=============================================================================
//
// Sky Geometry
//
//=============================================================================

//
// RB_GenerateSkyLine
//

boolean RB_GenerateSkyLine(vtxlist_t *vl, int *drawcount)
{
    vtx_t *v;
    seg_t *seg;

    v = &drawVertex[*drawcount];
    seg = (seg_t*)vl->data;

    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    v[0].z = v[1].z = MAX_COORD;
    v[2].z = v[3].z = vl->fparams;

    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);

    RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

    *drawcount += 4;
    return true;
}

//=============================================================================
//
// Lightmap Geometry
//
//=============================================================================

//
// RB_PreProcessLightmapSeg
//

boolean RB_PreProcessLightmapSeg(vtxlist_t* vl)
{
    seg_t *seg = (seg_t*)vl->data;
    lightMapInfo_t *lmi;

    lmi = &seg->lightMapInfo[vl->params];
    RB_BindTexture(&lightmapTextures[lmi->num]);

    return true;
}

//
// RB_GenerateLightmapLowerSeg
//

boolean RB_GenerateLightmapLowerSeg(vtxlist_t *vl, int *drawcount)
{
    vtx_t           *v;
    float           top;
    float           bottom;
    float           btop;
    float           bbottom;
    seg_t           *seg;
    sector_t        *sec;
    lightMapInfo_t *lmi;
    
    seg = (seg_t*)vl->data;
    
    if(!seg)
    {
        return false;
    }
    
    sec = seg->frontsector;
    lmi = &seg->lightMapInfo[vl->params];
    
    v = &drawVertex[*drawcount];
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);

    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    
    if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
    {
        btop = top;
    }
    
    if(bottom < bbottom)
    {
        v[0].z = v[1].z = bbottom;
        v[2].z = v[3].z = bottom;
        
        v[0].tu = lmi->coords[0 * 2 + 0];
        v[2].tv = lmi->coords[0 * 2 + 1];
        v[1].tu = lmi->coords[1 * 2 + 0];
        v[3].tv = lmi->coords[1 * 2 + 1];
        v[2].tu = lmi->coords[2 * 2 + 0];
        v[0].tv = lmi->coords[2 * 2 + 1];
        v[3].tu = lmi->coords[3 * 2 + 0];
        v[1].tv = lmi->coords[3 * 2 + 1];
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

        *drawcount += 4;
        return true;
    }
    
    return false;
}

//
// RB_GenerateLightmapUpperSeg
//

boolean RB_GenerateLightmapUpperSeg(vtxlist_t *vl, int *drawcount)
{
    vtx_t           *v;
    float           top;
    float           bottom;
    float           btop;
    float           bbottom;
    seg_t           *seg;
    sector_t        *sec;
    lightMapInfo_t *lmi;
    
    seg = (seg_t*)vl->data;
    
    if(!seg)
    {
        return false;
    }
    
    sec = seg->frontsector;
    lmi = &seg->lightMapInfo[vl->params];
    
    v = &drawVertex[*drawcount];
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;

    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);
    
    if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
    {
        btop = top;
    }
    
    if(top > btop)
    {
        v[0].z = v[1].z = top;
        v[2].z = v[3].z = btop;
        
        v[0].tu = lmi->coords[0 * 2 + 0];
        v[2].tv = lmi->coords[0 * 2 + 1];
        v[1].tu = lmi->coords[1 * 2 + 0];
        v[3].tv = lmi->coords[1 * 2 + 1];
        v[2].tu = lmi->coords[2 * 2 + 0];
        v[0].tv = lmi->coords[2 * 2 + 1];
        v[3].tu = lmi->coords[3 * 2 + 0];
        v[1].tv = lmi->coords[3 * 2 + 1];
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

        *drawcount += 4;
        return true;
    }
    
    return false;
}

//
// RB_GenerateLightmapMiddleSeg
//

boolean RB_GenerateLightmapMiddleSeg(vtxlist_t *vl, int *drawcount)
{
    vtx_t           *v;
    float           top;
    float           bottom;
    float           btop;
    float           bbottom;
    seg_t           *seg;
    sector_t        *sec;
    lightMapInfo_t  *lmi;
    
    seg = (seg_t*)vl->data;
    
    if(!seg)
    {
        return false;
    }
    
    sec = seg->frontsector;
    lmi = &seg->lightMapInfo[vl->params];
    
    v = &drawVertex[*drawcount];
    
    btop = 0;
    bbottom = 0;
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;

    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    
    if(seg->backsector)
    {
        RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);

        if(bottom < bbottom)
        {
            bottom = bbottom;
        }

        top = bottom + RB_GetTexture(RDT_COLUMN, texturetranslation[seg->sidedef->midtexture], 0)->height;

        if(top > btop)
        {
            top = btop;
        }
    }
    
    v[0].z = v[1].z = top;
    v[2].z = v[3].z = bottom;
    
    v[0].tu = lmi->coords[0 * 2 + 0];
    v[2].tv = lmi->coords[0 * 2 + 1];
    v[1].tu = lmi->coords[1 * 2 + 0];
    v[3].tv = lmi->coords[1 * 2 + 1];
    v[2].tu = lmi->coords[2 * 2 + 0];
    v[0].tv = lmi->coords[2 * 2 + 1];
    v[3].tu = lmi->coords[3 * 2 + 0];
    v[1].tv = lmi->coords[3 * 2 + 1];
    
    RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

    *drawcount += 4;
    return true;
}

//
// RB_PreProcessLightMapFlat
//

boolean RB_PreProcessLightMapFlat(vtxlist_t* vl)
{
    subsector_t *ss;
    lightMapInfo_t *lmi;

    ss = (subsector_t*)vl->data;
    lmi = &ss->lightMapInfo[(vl->flags & DLF_CEILING)];

    RB_BindTexture(&lightmapTextures[lmi->num]);
    return true;
}

//
// RB_GenerateLightMapFlat
//

boolean RB_GenerateLightMapFlat(vtxlist_t *vl, int *drawcount)
{
    int j;
    int count;
    leaf_t *leaf;
    subsector_t *ss;
    lightMapInfo_t *lmi;
    sector_t *sector;
    
    ss      = (subsector_t*)vl->data;
    leaf    = &leafs[ss->leaf];
    sector  = ss->sector;
    count   = *drawcount;

    lmi = &ss->lightMapInfo[(vl->flags & DLF_CEILING)];
    
    for(j = 0; j < ss->numleafs - 2; ++j)
    {
        RB_AddTriangle(count, count + 1 + j, count + 2 + j);
    }
    
    for(j = 0; j < ss->numleafs; ++j)
    {
        vtx_t *v = &drawVertex[count];
        
        if(vl->flags & DLF_CEILING)
        {
            leaf = &leafs[(ss->leaf + (ss->numleafs - 1)) - j];
        }
        else
        {
            leaf = &leafs[ss->leaf + j];
        }
        
        v->x = leaf->vertex->fx;
        v->y = leaf->vertex->fy;
        
        if(vl->flags & DLF_CEILING)
        {
            v->z = FIXED2FLOAT(sector->ceilingheight);
        }
        else
        {
            v->z = FIXED2FLOAT(sector->floorheight);
        }
        
        v->tu = lmi->coords[((ss->numleafs - 1) - j) * 2 + 0];
        v->tv = lmi->coords[((ss->numleafs - 1) - j) * 2 + 1];
        
        v->r = v->g = v->b = v->a = 0xff;
        
        count++;
    }
    
    *drawcount = count;
    return true;
}

//=============================================================================
//
// Dynlight Geometry
//
//=============================================================================

//
// RB_SetupDynLightWall
//
// Maps texture coordinates and sets up RGB values for dynlight
//

static void RB_SetupDynLightWall(vtx_t *v, seg_t *seg, rbDynLight_t *light)
{
    float d1, d2, dz;
    float dist;
    
    dz = ((v[0].z - light->z) / light->radius) + 0.5f;
    
    // cross product of vertex-to-light distance and linedef normal.
    // 0.5 is added to make sure that the light texture is centered
    d1 = (((v[0].x - light->x) * seg->linedef->fny -
           (v[0].y - light->y) * seg->linedef->fnx) / light->radius) + 0.5f;
    
    // cross product of wall distance and linedef normal
    d2 = (((v[1].x - v[0].x) * seg->linedef->fny -
           (v[1].y - v[0].y) * seg->linedef->fnx) / light->radius);
    
    // set texture coordinates
    v[0].tu = v[2].tu = d1;
    v[0].tv = v[1].tv = dz;
    v[1].tu = v[3].tu = d1 + d2;
    v[2].tv = v[3].tv = dz + ((v[2].z - v[0].z) / light->radius);
    
    // dot product of light origin and linedef normal
    dist = (((light->x * seg->linedef->fnx + light->y * seg->linedef->fny) -
             (v[0].x * seg->linedef->fnx + v[0].y * seg->linedef->fny)) / (light->radius * 0.5f));
    
    // behind the linedef?
    if(dist < 0)
    {
        RB_SetVertexColor(v, 0, 0, 0, 0, 4);
    }
    else
    {
        byte a = 255 - (byte)MAX(MIN(dist * 255.0f, 255), 0);
        
        // alpha is affected by distance
        RB_SetVertexColor(v, light->rgb[0], light->rgb[1], light->rgb[2], a, 4);
    }
}

//
// RB_DynLightPostProcess
//

void RB_DynLightPostProcess(vtxlist_t *vl, int *drawcount)
{
    // nothing fancy. just redraw the geometry to intensify the blend
    RB_DrawElements();
}

//
// RB_GenerateDynLightLowerSeg
//

boolean RB_GenerateDynLightLowerSeg(vtxlist_t *vl, int *drawcount)
{
    vtx_t           *v;
    float           top;
    float           bottom;
    float           btop;
    float           bbottom;
    seg_t           *seg;
    rbDynLight_t    *light;
    
    light = (rbDynLight_t*)vl->data;
    
    if(!light)
    {
        return false;
    }
    
    seg = &segs[vl->params];
    
    v = &drawVertex[*drawcount];
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);
    
    if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
    {
        btop = top;
    }
    
    if(bottom < bbottom)
    {
        v[0].z = v[1].z = bbottom;
        v[2].z = v[3].z = bottom;
        
        RB_SetupDynLightWall(v, seg, light);
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);
        
        *drawcount += 4;
        return true;
    }
    
    return false;
}

//
// RB_GenerateDynLightUpperSeg
//

boolean RB_GenerateDynLightUpperSeg(vtxlist_t *vl, int *drawcount)
{
    vtx_t           *v;
    float           top;
    float           bottom;
    float           btop;
    float           bbottom;
    seg_t           *seg;
    rbDynLight_t    *light;
    
    light = (rbDynLight_t*)vl->data;
    
    if(!light)
    {
        return false;
    }
    
    seg = &segs[vl->params];
    
    v = &drawVertex[*drawcount];
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);
    
    if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
    {
        btop = top;
    }
    
    if(top > btop)
    {
        v[0].z = v[1].z = top;
        v[2].z = v[3].z = btop;
        
        RB_SetupDynLightWall(v, seg, light);
        
        RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
        RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);
        
        *drawcount += 4;
        return true;
    }
    
    return false;
}

//
// RB_GenerateDynLightMiddleSeg
//

boolean RB_GenerateDynLightMiddleSeg(vtxlist_t *vl, int *drawcount)
{
    vtx_t           *v;
    float           top;
    float           bottom;
    float           btop;
    float           bbottom;
    seg_t           *seg;
    rbDynLight_t    *light;
    
    light = (rbDynLight_t*)vl->data;
    
    if(!light)
    {
        return false;
    }
    
    seg = &segs[vl->params];
    
    v = &drawVertex[*drawcount];
    
    btop = 0;
    bbottom = 0;
    
    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;
    
    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);
    
    if(seg->backsector)
    {
        RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);
        
        if(bottom < bbottom)
        {
            bottom = bbottom;
        }
        
        top = bottom + RB_GetTexture(RDT_COLUMN, texturetranslation[seg->sidedef->midtexture], 0)->height;
        
        if(top > btop)
        {
            top = btop;
        }
    }
    
    v[0].z = v[1].z = top;
    v[2].z = v[3].z = bottom;
    
    RB_SetupDynLightWall(v, seg, light);
    
    RB_AddTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    RB_AddTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);
    
    *drawcount += 4;
    return true;
}

//
// RB_GenerateDynLightFlat
//

boolean RB_GenerateDynLightFlat(vtxlist_t *vl, int *drawcount)
{
    int             j;
    int             count;
    leaf_t          *leaf;
    subsector_t     *ss;
    sector_t        *sector;
    rbDynLight_t    *light;
    float           px;
    float           py;
    vtx_t           *startVtx;
    float           x;
    float           y;
    float           dist;
    float           height;
    
    light = (rbDynLight_t*)vl->data;
    
    if(!light)
    {
        return false;
    }
    
    ss          = &subsectors[vl->params];
    leaf        = &leafs[ss->leaf];
    sector      = ss->sector;
    count       = *drawcount;
    startVtx    = NULL;
    x           = light->x;
    y           = light->y;
    
    for(j = 0; j < ss->numleafs - 2; ++j)
    {
        RB_AddTriangle(count, count + 1 + j, count + 2 + j);
    }

    height = (vl->flags & DLF_CEILING) ? FIXED2FLOAT(sector->ceilingheight) :
                                         FIXED2FLOAT(sector->floorheight);

    dist = (light->z - height) / (light->radius * 0.5f);

    if(vl->flags & DLF_CEILING)
    {
        dist = -dist;
    }
    
    for(j = 0; j < ss->numleafs; ++j)
    {
        vtx_t *v = &drawVertex[count];
        
        if(vl->flags & DLF_CEILING)
        {
            leaf = &leafs[(ss->leaf + (ss->numleafs - 1)) - j];
        }
        else
        {
            leaf = &leafs[ss->leaf + j];
        }
        
        v->x = leaf->vertex->fx;
        v->y = leaf->vertex->fy;
        v->z = height;
        
        if(startVtx == NULL)
        {
            startVtx = v;
        }

        if(dist < 0)
        {
            // behind the flat
            v->r = v->g = v->b = v->a = 0;
        }
        else
        {
            // alpha is affected by distance
            v->r = light->rgb[0];
            v->g = light->rgb[1];
            v->b = light->rgb[2];
            v->a = 255 - (byte)MAX(MIN(dist * 255.0f, 255), 0);
        }
        
        px = ((startVtx->x - x) / light->radius) + 0.5f;
        py = ((startVtx->y - y) / light->radius) + 0.5f;
        
        v->tu = px + ((v->x - startVtx->x) / light->radius);
        v->tv = py + ((v->y - startVtx->y) / light->radius);
        
        count++;
    }
    
    *drawcount = count;
    return true;
}
