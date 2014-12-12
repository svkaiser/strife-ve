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
//    OpenGL Adpation of BSP Rendering
//

#include <math.h>

#include "rb_main.h"
#include "rb_view.h"
#include "rb_bsp.h"
#include "rb_data.h"
#include "rb_draw.h"
#include "rb_view.h"
#include "rb_texture.h"
#include "rb_clipper.h"
#include "rb_decal.h"
#include "rb_geom.h"
#include "rb_things.h"
#include "rb_wallshade.h"
#include "rb_dynlights.h"
#include "rb_config.h"
#include "r_main.h"
#include "r_defs.h"
#include "i_system.h"
#include "z_zone.h"
#include "m_bbox.h"
#include "p_local.h"
#include "doomstat.h"

static int currentssect = 0;

typedef enum
{
    BS_LOWER    = 0,
    BS_UPPER,
    BS_MIDDLE
} bspSide_t;

static boolean (*procsegs[][3])(struct vtxlist_s*, int*) =
{
    {
        RB_GenerateLowerSeg,
        RB_GenerateUpperSeg,
        RB_GenerateMiddleSeg
    },
    {
        RB_GenerateDynLightLowerSeg,
        RB_GenerateDynLightUpperSeg,
        RB_GenerateDynLightMiddleSeg
    },
    {
        RB_GenerateLightmapLowerSeg,
        RB_GenerateLightmapUpperSeg,
        RB_GenerateLightmapMiddleSeg
    }
};

//
// RB_SegDistanceToView
//

static float RB_SegDistanceToView(seg_t *seg)
{
    float pd;
    
    // get normal distance
    pd = seg->v1->fx * seg->linedef->fnx +
         seg->v1->fy * seg->linedef->fny;

    // dot product of player view and seg
    return fabsf((rbPlayerView.x * seg->linedef->fnx +
                  rbPlayerView.y * seg->linedef->fny) - pd);
}

//
// RB_AddSegToDrawlist
//

static void RB_AddSegToDrawlist(seg_t *seg, int texid, bspSide_t sidetype)
{
    vtxlist_t *list;
    drawlisttag_e dltag;
    sector_t *sector = seg->frontsector;
    
    if(sidetype == BS_MIDDLE && seg->backsector && seg->linedef->flags & (ML_TRANSPARENT1|ML_TRANSPARENT2))
    {
        // 2-sided lines with a non-masked middle texture is considered a transparent wall
        // for draw-sort reasons
        dltag = DLT_TRANSWALL;
    }
    else if((sidetype == BS_MIDDLE && seg->backsector) ||
        RB_GetTextureFlags(RDT_COLUMN, texid, 0) & TDF_MASKED)
    {
        dltag = DLT_MASKEDWALL;
    }
    else
    {
        dltag = DLT_WALL;
    }

    // add initial draw list
    list = DL_AddVertexList(&drawlist[dltag]);
    list->data = (seg_t*)seg;
    list->procfunc = procsegs[0][sidetype];
    list->preprocess = RB_PreProcessSeg;
    list->postprocess = 0;
    list->flags = 0;
    list->fparams = 0;
    list->params = (rbLightmaps && sector->altlightlevel != -1) ? sector->altlightlevel : sector->lightlevel;
    list->texid = texid;

    if(dltag == DLT_TRANSWALL)
    {
        // set distance for transparent walls
        list->fparams = RB_SegDistanceToView(seg);
    }

    //
    // handle additional draw lists
    //

    if(dltag == DLT_MASKEDWALL)
    {
        if(RB_GetTextureFlags(RDT_COLUMN, texid, 0) & TDF_BRIGHTMAP)
        {
            list = DL_AddVertexList(&drawlist[DLT_BRIGHTMASKED]);
            list->data = (seg_t*)seg;
            list->procfunc = procsegs[0][sidetype];
            list->preprocess = RB_PreProcessSeg;
            list->postprocess = 0;
            list->flags = 0;
            list->fparams = 0;
            list->params = 0xff;
            list->texid = texid;
        }
    }
    else if(dltag == DLT_WALL)
    {
        // add brightmap draw list
        // TODO: this may not get explicitly added on the very first frame
        if(RB_GetTextureFlags(RDT_COLUMN, texid, 0) & TDF_BRIGHTMAP)
        {
            list = DL_AddVertexList(&drawlist[DLT_BRIGHT]);
            list->data = (seg_t*)seg;
            list->procfunc = procsegs[0][sidetype];
            list->preprocess = RB_PreProcessSeg;
            list->postprocess = 0;
            list->flags = 0;
            list->fparams = 0;
            list->params = 0xff;
            list->texid = texid;
        }
        
        // dynamic light draw lists
        if(rbDynamicLights)
        {
            int marknum = RB_SubsectorMarked(currentssect);
            
            if(marknum)
            {
                int i;
                
                for(i = 0; i < MAX_DYNLIGHTS; ++i)
                {
                    if(!(marknum & (1 << i)))
                    {
                        continue;
                    }
                    
                    list = DL_AddVertexList(&drawlist[DLT_DYNLIGHT]);
                    list->data = (rbDynLight_t*)RB_GetDynLight(i);
                    list->procfunc = procsegs[1][sidetype];
                    list->preprocess = 0;
                    list->postprocess = rbDynamicLightFastBlend ? 0 : RB_DynLightPostProcess;
                    list->flags = 0;
                    list->texid = 0;
                    list->fparams = 0;
                    list->params = seg - segs;
                }
            }
        }

        // lightmap draw lists
        if(rbLightmaps && lightmapTextures)
        {
            int which;
            lightMapInfo_t *lmi;

            switch(sidetype)
            {
                case BS_LOWER:
                    which = 2;
                    break;
                    
                case BS_UPPER:
                    which = 1;
                    break;
                    
                case BS_MIDDLE:
                    which = 0;
                    break;
                    
                default:
                    return;
            }

            lmi = &seg->lightMapInfo[which];

            if(lmi->num == -1)
            {
                return;
            }

            list = DL_AddVertexList(&drawlist[DLT_LIGHTMAP]);
            list->data = (seg_t*)seg;
            list->procfunc = procsegs[2][sidetype];
            list->preprocess = RB_PreProcessLightmapSeg;
            list->postprocess = 0;
            list->flags = 0;
            list->fparams = 0;
            list->params = which;
            list->texid = lightmapTextures[lmi->num].texid;
        }
    }
}

//
// RB_AddLeafToDrawlist
//

static void RB_AddLeafToDrawlist(subsector_t *sub, int texid, boolean bCeiling)
{
    vtxlist_t *list;
    sector_t *sector = sub->sector;
    int marknum;
    
    // add initial draw list
    list = DL_AddVertexList(&drawlist[DLT_FLAT]);
    list->data = (subsector_t*)sub;
    list->procfunc = RB_GenerateSubSectors;
    list->preprocess = RB_PreProcessSubsector;
    list->postprocess = 0;
    list->params = (rbLightmaps && sector->altlightlevel != -1) ? sector->altlightlevel : sector->lightlevel;
    list->texid = texid;
    list->flags = 0;
    
    if(bCeiling)
    {
        list->flags |= DLF_CEILING;
    }
    
    // add brightmap draw list
    // TODO: this may not get explicitly added on the very first frame
    if(RB_GetTextureFlags(RDT_FLAT, texid, 0) & TDF_BRIGHTMAP)
    {
        list = DL_AddVertexList(&drawlist[DLT_BRIGHT]);
        list->data = (subsector_t*)sub;
        list->procfunc = RB_GenerateSubSectors;
        list->preprocess = RB_PreProcessSubsector;
        list->postprocess = 0;
        list->params = 0xff;
        list->texid = texid;
        list->flags = 0;
        
        if(bCeiling)
        {
            list->flags |= DLF_CEILING;
        }
    }
    
    // add dynamic light draw list
    if(rbDynamicLights)
    {
        marknum = RB_SubsectorMarked(currentssect);
        
        if(marknum)
        {
            int i;
            
            for(i = 0; i < MAX_DYNLIGHTS; ++i)
            {
                if(!(marknum & (1 << i)))
                {
                    continue;
                }
                
                list = DL_AddVertexList(&drawlist[DLT_DYNLIGHT]);
                list->data = (rbDynLight_t*)RB_GetDynLight(i);
                list->preprocess = 0;
                list->postprocess = rbDynamicLightFastBlend ? 0 : RB_DynLightPostProcess;
                list->procfunc = RB_GenerateDynLightFlat;
                list->texid = 0;
                list->fparams = 0;
                list->params = sub - subsectors;

                if(bCeiling)
                {
                    list->flags |= DLF_CEILING;
                }
            }
        }
    }

    // add lightmap drawlist
    if(rbLightmaps && lightmapTextures && sub->lightMapInfo[bCeiling].num != -1)
    {
        list = DL_AddVertexList(&drawlist[DLT_LIGHTMAP]);
        list->data = (subsector_t*)sub;
        list->procfunc = RB_GenerateLightMapFlat;
        list->preprocess = RB_PreProcessLightMapFlat;
        list->postprocess = 0;
        list->params = 0;
        list->texid = lightmapTextures[sub->lightMapInfo[bCeiling].num].texid;
        list->flags = 0;

        if(bCeiling)
        {
            list->flags |= DLF_CEILING;
        }
    }
}

//
// RB_AddClipLineToDrawlist
//

static void RB_AddClipLineToDrawlist(seg_t *seg)
{
    vtxlist_t *list;
    line_t *line;

    if(!rbFixSpriteClipping)
    {
        return;
    }

    line = seg->linedef;

    if(line->validcount == validcount)
    {
        return;
    }

    line->validcount = validcount;

    list = DL_AddVertexList(&drawlist[DLT_CLIPLINE]);
    list->data = (line_t*)line;
    list->preprocess = RB_PreProcessClipLine;
    list->postprocess = 0;
    list->procfunc = RB_GenerateClipLine;
    list->params = 0;
    list->texid = 0;
    list->flags = 0;
    
    if(line->backsector)
    {
        side_t *sidedef = seg->sidedef;
        
        // ignore texture-less sidedefs that borders between two sky sectors
        // these don't happen to clip sprites as observed in software mode
        if(sidedef->toptexture == 0 &&
           line->backsector->ceilingheight != line->frontsector->ceilingheight &&
           line->backsector->ceilingpic == skyflatnum &&
           line->frontsector->ceilingpic == skyflatnum)
        {
            list->params = 1;
        }
    }
}

//
// RB_AddSkyLineToDrawlist
//

static void RB_AddSkyLineToDrawlist(seg_t *seg, float height)
{
    vtxlist_t *list;

    list = DL_AddVertexList(&drawlist[DLT_SKY]);
    list->data = (seg_t*)seg;
    list->preprocess = RB_PreProcessClipLine;
    list->postprocess = 0;
    list->procfunc = RB_GenerateSkyLine;
    list->params = 0;
    list->fparams = height;
    list->texid = 0;
    list->flags = 0;
}

//
// RB_AddLine
//

void RB_AddLine(seg_t* seg)
{
    angle_t     angle1;
    angle_t     angle2;
    vtx_t       v[4];
    line_t      *linedef;
    side_t      *sidedef;
    float       top;
    float       bottom;
    float       btop;
    float       bbottom;
    boolean     infrustum;

    if(!seg->linedef)
    {
        return;
    }

    if(seg->backsector)
    {
        if(seg->backsector->ceilingheight == seg->frontsector->ceilingheight &&
            seg->backsector->floorheight == seg->frontsector->floorheight &&
            seg->sidedef->midtexture == 0)
        {
            seg->linedef->flags |= ML_MAPPED;
            return;
        }
    }

    if(seg->v1->validcount != validcount)
    {
        seg->v1->clipspan = RB_PointToBam(seg->v1->x, seg->v1->y);
        seg->v1->validcount = validcount;
    }

    if(seg->v2->validcount != validcount)
    {
        seg->v2->clipspan = RB_PointToBam(seg->v2->x, seg->v2->y);
        seg->v2->validcount = validcount;
    }

    angle1 = seg->v1->clipspan;
    angle2 = seg->v2->clipspan;

    if(!RB_Clipper_SafeCheckRange(angle2, angle1))
    {
        return;
    }
    
    linedef = seg->linedef;

    // did we already check this linedef?
    if(linedef->validclip[0] != validcount)
    {
        if(linedef->validclip[1] == validcount)
        {
            // this linedef was already clipped
            infrustum = false;
        }
        else
        {
            fixed_t z1, z2;
            
            if(linedef->backsector)
            {
                z1 = linedef->frontsector->floorheight < linedef->backsector->floorheight ?
                linedef->frontsector->floorheight :
                linedef->backsector->floorheight;

                z2 = linedef->frontsector->ceilingheight > linedef->backsector->ceilingheight ?
                linedef->frontsector->ceilingheight :
                linedef->backsector->ceilingheight;
            }
            else
            {
                z1 = linedef->frontsector->floorheight;
                z2 = linedef->frontsector->ceilingheight;
            }
            
            infrustum = RB_CheckBoxInView(&rbPlayerView, linedef->bbox, z1, z2);
            
            // mark it so we don't have to recheck this linedef again
            linedef->validclip[infrustum ? 0 : 1] = validcount;
        }
    }
    else
    {
        infrustum = true;
    }

    RB_AddClipLineToDrawlist(seg);

    // Back side, i.e. backface culling - read: endAngle >= startAngle!
    if(angle2 - angle1 < ANG180)
    {
        return;
    }

    if(seg->backsector)
    {
        if((seg->backsector->floorheight == seg->backsector->ceilingheight) ||
            seg->backsector->ceilingheight <= seg->frontsector->floorheight ||
            seg->backsector->floorheight >= seg->frontsector->ceilingheight)
        {
            RB_Clipper_SafeAddClipRange(angle2, angle1);
        }
    }
    else if(!seg->backsector)
    {
        // sanity check
        RB_Clipper_SafeAddClipRange(angle2, angle1);
    }

    if(!infrustum)
    {
        return;
    }

    linedef->flags |= ML_MAPPED;
    sidedef = seg->sidedef;

    v[0].x = v[2].x = seg->v1->fx;
    v[0].y = v[2].y = seg->v1->fy;
    v[1].x = v[3].x = seg->v2->fx;
    v[1].y = v[3].y = seg->v2->fy;

    RB_GetSideTopBottom(seg->frontsector, &top, &bottom);

    if(seg->backsector)
    {
        RB_GetSideTopBottom(seg->backsector, &btop, &bbottom);

        if((seg->frontsector->ceilingpic == skyflatnum) && (seg->backsector->ceilingpic == skyflatnum))
        {
            btop = top;
        }

        //
        // botom side line
        //
        if(bottom < bbottom && sidedef->bottomtexture != 0)
        {
            v[0].z = v[1].z = bbottom;
            v[2].z = v[3].z = bottom;
            
            if(RB_CheckPointsInView(&rbPlayerView, v, 4))
            {
                RB_AddSegToDrawlist(seg, sidedef->bottomtexture, BS_LOWER);
            }
            bottom = bbottom;
        }

        //
        // upper side line
        //
        if(top > btop && sidedef->toptexture != 0)
        {
            v[0].z = v[1].z = top;
            v[2].z = v[3].z = btop;

            if(RB_CheckPointsInView(&rbPlayerView, v, 4))
            {
                RB_AddSegToDrawlist(seg, sidedef->toptexture, BS_UPPER);
            }
            top = btop;
        }

        // check for fake seg lines that will be drawn to the depth buffer so
        // we can occlude fragments against the sky
        if(sidedef->toptexture != 0 && seg->backsector->ceilingpic != seg->frontsector->ceilingpic)
        {
            if(seg->backsector->ceilingpic == skyflatnum)
            {
                RB_AddSkyLineToDrawlist(seg, FIXED2FLOAT(seg->backsector->ceilingheight));
            }
            else if(seg->frontsector->ceilingpic == skyflatnum)
            {
                RB_AddSkyLineToDrawlist(seg, FIXED2FLOAT(seg->frontsector->ceilingheight));
            }
        }
    }
    else if(seg->frontsector->ceilingpic == skyflatnum)
    {
        // do nothing special here. just add it so we can mask it out in the stencil buffer
        RB_AddSkyLineToDrawlist(seg, FIXED2FLOAT(seg->frontsector->ceilingheight));
    }

    //
    // middle side line
    //
    if(sidedef->midtexture != 0)
    {
        v[0].z = v[1].z = top;
        v[2].z = v[3].z = bottom;

        if(RB_CheckPointsInView(&rbPlayerView, v, 4))
        {
            RB_AddSegToDrawlist(seg, sidedef->midtexture, BS_MIDDLE);
        }
    }
}

static int checkcoord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};

//
// RB_CheckBBox
//

static boolean RB_CheckBBox(fixed_t *bspcoord)
{
    angle_t     angle1;
    angle_t     angle2;
    int         boxpos;
    const int*  check;

    // Find the corners of the box
    // that define the edges from current viewpoint.
    boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT] ? 1 : 2) +
             (viewy >= bspcoord[BOXTOP] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8);

    if(boxpos == 5)
    {
        return true;
    }

    check = checkcoord[boxpos];
    angle1 = RB_PointToBam(bspcoord[check[0]], bspcoord[check[1]]) - viewangle;
    angle2 = RB_PointToBam(bspcoord[check[2]], bspcoord[check[3]]) - viewangle;

    return RB_Clipper_SafeCheckRange(angle2 + viewangle, angle1 + viewangle);
}

//
// RB_Subsector
//

void RB_Subsector(int num)
{
    sector_t *sector;
    subsector_t *sub;
    fixed_t bbox[4];
    fixed_t *blockbox;
    int i;
    leaf_t *leaf;
    
    currentssect = num;

    sub = &subsectors[num];
    sector = sub->sector;

    if(sub->numleafs < 3)
    {
        return;
    }
    
    if(sector->ceilingpic == skyflatnum)
    {
        skyvisible = true;
    }

    // haleyjd: set sector shade(s) now
    RB_SetSectorShades(sector);

    for(i = 0; i < sub->numleafs; i++)
    {
        leaf = &leafs[sub->leaf + i];
        if(leaf->seg != NULL)
        {
            RB_AddLine(leaf->seg);
        }
    }

    // did we already check this sector?
    if(sector->validclip[0] != validcount)
    {
        if(sector->validclip[1] == validcount)
        {
            // this sector was already clipped
            return;
        }

        blockbox = sector->blockbox;

        bbox[BOXTOP]    = ((blockbox[BOXTOP]    << MAPBLOCKSHIFT) + bmaporgy) + (80*FRACUNIT);
        bbox[BOXBOTTOM] = ((blockbox[BOXBOTTOM] << MAPBLOCKSHIFT) + bmaporgy) - (80*FRACUNIT);
        bbox[BOXRIGHT]  = ((blockbox[BOXRIGHT]  << MAPBLOCKSHIFT) + bmaporgx) + (80*FRACUNIT);
        bbox[BOXLEFT]   = ((blockbox[BOXLEFT]   << MAPBLOCKSHIFT) + bmaporgx) - (80*FRACUNIT);

        if(!RB_CheckBoxInView(&rbPlayerView, bbox, sector->floorheight, sector->ceilingheight))
        {
            // mark it so we don't have to recheck this sector again
            sector->validclip[1] = validcount;
            return;
        }
        else
        {
            // mark it so we don't have to recheck this sector again
            sector->validclip[0] = validcount;
        }
    }

    if(viewz > sector->floorheight && sector->floorpic != skyflatnum)
    {
        RB_AddLeafToDrawlist(sub, sector->floorpic, false);
    }

    if(viewz < sector->ceilingheight && sector->ceilingpic != skyflatnum)
    {
        RB_AddLeafToDrawlist(sub, sector->ceilingpic, true);
    }

    RB_AddSprites(sub);
    RB_AddDecals(sub);
}

//
// RB_RenderBSPNode
//

void RB_RenderBSPNode(int bspnum)
{
    node_t  *bsp;
    int     side;

    while(!(bspnum & NF_SUBSECTOR))
    {
        bsp = &nodes[bspnum];

        // Decide which side the view point is on.
        side = R_PointOnSide(viewx, viewy, bsp);

        // check the front space
        if(RB_CheckBBox(bsp->bbox[side]))
        {
            RB_RenderBSPNode(bsp->children[side]);
        }

        // continue down the back space
        if(!RB_CheckBBox(bsp->bbox[side^1]))
        {
            return;
        }

        bspnum = bsp->children[side^1];
    }

    // subsector with contents
    // add all the drawable elements in the subsector
    if(bspnum == -1)
    {
        bspnum = 0;
    }

    RB_Subsector(bspnum & ~NF_SUBSECTOR);
}
