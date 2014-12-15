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
//  Do all the WAD I/O, get map description,
//  set up initial state and misc. LUTs.
//



#include <math.h>

// [SVE] svillarreal
#include "rb_config.h"
#include "rb_common.h"
#include "rb_drawlist.h"
#include "rb_wallshade.h"
#include "rb_decal.h"
#include "rb_level.h"
#include "rb_data.h"
#include "rb_dynlights.h"

#include "z_zone.h"
#include "deh_main.h"
#include "i_swap.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "g_game.h"
#include "i_system.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "doomstat.h"
#include "p_locations.h"


void    P_SpawnMapThing (mapthing_t*    mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int                 numvertexes;
vertex_t*           vertexes;
static int          numglverts;

int                 numsegs;
seg_t*              segs;

int                 numsectors;
sector_t*           sectors;

// haleyjd 20140904: [SVE] sector interpolation data
sectorinterp_t      *sectorinterps;

int                 numsubsectors;
subsector_t*        subsectors;

int                 numnodes;
node_t*             nodes;

int                 numlines;
line_t*             lines;

int                 numsides;
side_t*             sides;

// [SVE] svillarreal - leafs
int                 numleafs;
leaf_t*             leafs;

// [SVE] svillarreal - lightmap data
lightGridInfo_t     lightgrid;
float               sunlightdir[3];
float               sunlightdir2d[2];

static float        *lmtexcoords;

static int          totallines;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int         bmapwidth;
int         bmapheight; // size in mapblocks
short*      blockmap;   // int for larger maps
// offsets in blockmap are from here
short*      blockmaplump;       
// origin of block map
fixed_t     bmaporgx;
fixed_t     bmaporgy;
// for thing chains
mobj_t**    blocklinks;     


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*       rejectmatrix;

// [SVE] svillarreal
byte*       pvsmatrix;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS   10

// haleyjd 20140819: [SVE] remove deathmatch starts limit
mapthing_t *deathmatchstarts;
int         numdeathmatchstarts;
int         numdmstartsalloc;

// haleyjd 20140917: [SVE] Capture the Chalice starts
mapthing_t *ctcbluestarts;
mapthing_t *ctcredstarts;
int         numctcbluestarts;
int         numctcbluestartsalloc;
int         numctcredstarts;
int         numctcredstartsalloc;

mapthing_t  playerstarts[MAXPLAYERS];

// haleyjd 08/24/10: [STRIFE] rift spots for player spawning
mapthing_t  riftSpots[MAXRIFTSPOTS];

// [SVE]: track which are valid, for scoot cheat.
boolean     riftSpotInit[MAXRIFTSPOTS];

// [SVE] svillarreal
boolean      mapwithspecialtags;
extern int numignitechains;
extern mobj_t *curignitemobj;

//
// P_LoadVertexes
//

void P_LoadVertexes(int lump)
{
    byte*           data;
    int             i;
    mapvertex_t     *ml;
    vertex_t        *li;

    // Determine number of lumps:
    //  total lump length / vertex record length.

    numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);  

    // Load data into cache.
    data = W_CacheLumpNum(lump, PU_STATIC);
    li = vertexes;
    
    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    ml = (mapvertex_t*)data;

    for(i = 0; i < numvertexes; i++, li++, ml++)
    {
        li->x = SHORT(ml->x)<<FRACBITS;
        li->y = SHORT(ml->y)<<FRACBITS;

        li->fx = FIXED2FLOAT(li->x);
        li->fy = FIXED2FLOAT(li->y);

        // [SVE] svillarreal
        li->validcount = -1;
        li->clipspan = ANG_MAX;
    }

    // Free buffer memory.
    W_ReleaseLumpNum(lump);
}

//
// P_LoadGLVertexes
//

void P_LoadGLVertexes(int lump)
{
    byte            *data;
    int             i;
    glVert_t        *ml;
    vertex_t        *li;

    // Load data into cache.
    data = W_CacheLumpNum(lump, PU_STATIC);

    if(*((int*)data) != gNd2)
    {
        I_Error("P_LoadGLVertexes: GL_VERTS must be version 2 only");
        return;
    }

    // Determine number of lumps:
    // total lump length / vertex record length.
    numglverts = (W_LumpLength(lump) - GL_VERT_OFFSET) / sizeof(glVert_t);
    numvertexes += numglverts;

    // Allocate zone memory for buffer.
    vertexes = Z_Realloc(vertexes, numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    li = &vertexes[numvertexes - numglverts];
    
    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    ml = (glVert_t*)(data + GL_VERT_OFFSET);

    for(i = 0; i < numglverts; i++, li++, ml++)
    {
        li->x = LONG(ml->x);
        li->y = LONG(ml->y);

        li->fx = FIXED2FLOAT(li->x);
        li->fy = FIXED2FLOAT(li->y);

        // [SVE] svillarreal
        li->validcount = -1;
        li->clipspan = ANG_MAX;
    }

    // Free buffer memory.
    W_ReleaseLumpNum(lump);
}



//
// P_LoadSegs
//

void P_LoadSegs (int lump)
{
    byte*       data;
    int         i;
    mapseg_t*   ml;
    seg_t*      li;
    line_t*     ldef;
    int         linedef;
    int         side;
    int         sidenum;
    float       x;
    float       y;
    
    numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
    segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0); 
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
    
    ml = (mapseg_t *)data;
    li = segs;
    for(i = 0; i < numsegs; i++, li++, ml++)
    {
        li->v1 = &vertexes[SHORT(ml->v1)];
        li->v2 = &vertexes[SHORT(ml->v2)];

        li->angle = (SHORT(ml->angle))<<16;
        li->offset = (SHORT(ml->offset))<<16;
        linedef = SHORT(ml->linedef);
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = SHORT(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        if(ldef-> flags & ML_TWOSIDED)
        {
            sidenum = ldef->sidenum[side ^ 1];

            // If the sidenum is out of range, this may be a "glass hack"
            // impassible window.  Point at side #0 (this may not be
            // the correct Vanilla behavior; however, it seems to work for
            // OTTAWAU.WAD, which is the one place I've seen this trick
            // used).

            if (sidenum < 0 || sidenum >= numsides)
            {
                sidenum = 0;
            }

            li->backsector = sides[sidenum].sector;
        }
        else
        {
            li->backsector = 0;
        }

        // [SVE] svillarreal - calculate length of seg
        x = FIXED2FLOAT(li->v1->x - li->v2->x);
        y = FIXED2FLOAT(li->v1->y - li->v2->y);

        li->length = sqrtf(x * x + y * y);

        li->lightMapInfo[0].num = -1;
        li->lightMapInfo[1].num = -1;
        li->lightMapInfo[2].num = -1;
    }
    
    W_ReleaseLumpNum(lump);
}

//
// P_CheckGLVertex
//

static int P_CheckGLVertex(int num)
{
    if(num & 0x8000)
    {
        num = (num & 0x7FFF) + (numvertexes - numglverts);
    }

    return num;
}

//
// P_LoadGLSegs
//

void P_LoadGLSegs(int lump)
{
    byte*       data;
    int         i;
    glSeg_t*    ml;
    seg_t*      li;
    line_t*     ldef;
    int         linedef;
    int         side;
    int         sidenum;
    float       x;
    float       y;
    
    numsegs = W_LumpLength (lump) / sizeof(glSeg_t);

    segs = Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL,0); 
    memset(segs, 0, numsegs * sizeof(seg_t));

    data = W_CacheLumpNum(lump, PU_STATIC);
    
    ml = (glSeg_t*)data;
    li = segs;

    for(i = 0; i < numsegs; i++, li++, ml++)
    {
        float a, b;
        vertex_t *v;

        li->v1 = &vertexes[P_CheckGLVertex(SHORT(ml->v1))];
        li->v2 = &vertexes[P_CheckGLVertex(SHORT(ml->v2))];

        if(ml->linedef == 0xFFFF)
        {
            continue;
        }

        li->angle = RB_PointToAngle(li->v2->x - li->v1->x, li->v2->y - li->v1->y);

        linedef = SHORT(ml->linedef);
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = SHORT(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        v = ml->side ? ldef->v2 : ldef->v1;

        a = FIXED2FLOAT(li->v1->x - v->x);
        b = FIXED2FLOAT(li->v1->y - v->y);

        li->offset = FLOAT2FIXED(sqrtf(a * a + b * b));

        if(ldef-> flags & ML_TWOSIDED)
        {
            sidenum = ldef->sidenum[side ^ 1];

            // If the sidenum is out of range, this may be a "glass hack"
            // impassible window.  Point at side #0 (this may not be
            // the correct Vanilla behavior; however, it seems to work for
            // OTTAWAU.WAD, which is the one place I've seen this trick
            // used).

            if (sidenum < 0 || sidenum >= numsides)
            {
                sidenum = 0;
            }

            li->backsector = sides[sidenum].sector;
        }
        else
        {
            li->backsector = 0;
        }

        // [SVE] svillarreal - calculate length of seg
        x = FIXED2FLOAT(li->v1->x - li->v2->x);
        y = FIXED2FLOAT(li->v1->y - li->v2->y);

        li->length = sqrtf(x * x + y * y);

        li->lightMapInfo[0].num = -1;
        li->lightMapInfo[1].num = -1;
        li->lightMapInfo[2].num = -1;
    }
    
    W_ReleaseLumpNum(lump);
}


//
// P_LoadSubsectors
//

void P_LoadSubsectors (int lump)
{
    byte*       data;
    int         i;
    mapsubsector_t* ms;
    subsector_t*    ss;
    
    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);   
    data = W_CacheLumpNum (lump,PU_STATIC);
    
    ms = (mapsubsector_t *)data;
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;
    
    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
        ss->numlines = SHORT(ms->numsegs);
        ss->firstline = SHORT(ms->firstseg);
        ss->leaf = 0;
        ss->numleafs = 0;
        ss->lightMapInfo[0].num = -1;
        ss->lightMapInfo[1].num = -1;
    }
    
    W_ReleaseLumpNum(lump);
}



//
// P_LoadSectors
//

void P_LoadSectors (int lump)
{
    byte*       data;
    int         i;
    mapsector_t*    ms;
    sector_t*       ss;

    numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
    sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);    
    memset (sectors, 0, numsectors*sizeof(sector_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    ms = (mapsector_t *)data;
    ss = sectors;
    for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
        ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
        ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = SHORT(ms->lightlevel);
        ss->special = SHORT(ms->special);
        ss->tag = SHORT(ms->tag);
        ss->thinglist = NULL;
        ss->floorshade   = NULL; // haleyjd [SVE]
        ss->ceilingshade = NULL;
        ss->decallist = NULL; // [SVE] svillarreal
        ss->altlightlevel = -1; // [SVE] svillarreal
        ss->validclip[0] = -1;
        ss->validclip[1] = -1;
        ss->bloomthreshold = -1;

        // [SVE] svillarreal - track if this map has special tags
        if(ss->tag == 667)
            mapwithspecialtags = true;
    }

    W_ReleaseLumpNum(lump);
}

//
// P_CreateSectorInterps
//
// haleyjd 20140904: [SVE] Create sector interpolation structures.
//
static void P_CreateSectorInterps(void)
{
    int i;
    sectorinterps = Z_Calloc(numsectors, sizeof(sectorinterp_t), PU_LEVEL, NULL);

    for(i = 0; i < numsectors; i++)
    {
        sectorinterps[i].prevfloorheight   = sectors[i].floorheight;
        sectorinterps[i].prevceilingheight = sectors[i].ceilingheight;
    }
}

//
// P_LoadNodes
//

void P_LoadNodes (int lump)
{
    byte*   data;
    int     i;
    int     j;
    int     k;
    mapnode_t*  mn;
    node_t* no;
    
    numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);  
    data = W_CacheLumpNum (lump,PU_STATIC);
    
    mn = (mapnode_t *)data;
    no = nodes;
    
    for (i=0 ; i<numnodes ; i++, no++, mn++)
    {
        no->x = SHORT(mn->x)<<FRACBITS;
        no->y = SHORT(mn->y)<<FRACBITS;
        no->dx = SHORT(mn->dx)<<FRACBITS;
        no->dy = SHORT(mn->dy)<<FRACBITS;
        for (j=0 ; j<2 ; j++)
        {
            no->children[j] = SHORT(mn->children[j]);
            for (k=0 ; k<4 ; k++)
            no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
        }
    }
    
    W_ReleaseLumpNum(lump);
}

//
// P_BuildLeafs
//

void P_BuildLeafs(void)
{
    subsector_t *ss;
    leaf_t      *lf;
    int         i;
    int         j;

    leafs = Z_Malloc(numsegs * sizeof(leaf_t), PU_LEVEL, 0);
    numleafs = numsubsectors;

    ss = subsectors;
    lf = leafs;

    for(i = 0; i < numsubsectors; ++i, ++ss)
    {
        ss->numleafs = ss->numlines;
        ss->leaf = (lf - leafs);

        if(ss->numlines)
        {
            for(j = 0; j < ss->numlines; ++j, ++lf)
            {
                seg_t *seg = &segs[ss->firstline + j];
                lf->vertex = seg->v1;
                lf->seg = seg;
            }
        }
    }
}

//
// P_LoadThings
//
// haleyjd 08/24/10: [STRIFE]:
// * Added code to record rift spots
//

void P_LoadThings (int lump)
{
    byte               *data;
    int         i;
    mapthing_t         *mt;
    mapthing_t          spawnthing;
    int         numthings;
    boolean     spawn;

    data = W_CacheLumpNum (lump,PU_STATIC);
    numthings = W_LumpLength (lump) / sizeof(mapthing_t);

    // [SVE]: clear out riftSpotInit array
    for(i = 0; i < MAXRIFTSPOTS; i++)
        riftSpotInit[i] = false;

    mt = (mapthing_t *)data;
    for (i=0 ; i<numthings ; i++, mt++)
    {
        spawn = true;

        // Do not spawn cool, new monsters if !commercial
        // STRIFE-TODO: replace with isregistered stuff
        /*
        if (gamemode != commercial)
        {
            switch (SHORT(mt->type))
            {
            case 68:    // Arachnotron
            case 64:    // Archvile
            case 88:    // Boss Brain
            case 89:    // Boss Shooter
            case 69:    // Hell Knight
            case 67:    // Mancubus
            case 71:    // Pain Elemental
            case 65:    // Former Human Commando
            case 66:    // Revenant
            case 84:    // Wolf SS
                spawn = false;
                break;
            }
        }
        if (spawn == false)
            break;
        */

        // Do spawn all other stuff. 
        spawnthing.x = SHORT(mt->x);
        spawnthing.y = SHORT(mt->y);
        spawnthing.angle = SHORT(mt->angle);
        spawnthing.type = SHORT(mt->type);
        spawnthing.options = SHORT(mt->options);

        // haleyjd 08/24/2010: Special Strife checks
        if(spawnthing.type >= 118 && spawnthing.type < 128)
        {
            // initialize riftSpots
            int riftSpotNum = spawnthing.type - 118;
            riftSpots[riftSpotNum] = spawnthing;
            riftSpots[riftSpotNum].type = 1;
            riftSpotInit[riftSpotNum] = true; // [SVE]
        }
        else if(spawnthing.type >= 9001 && spawnthing.type < 9011)
        {
            // STRIFE-TODO: mystery array of 90xx objects
        }
        // [SVE] svillarreal - set alternate light levels
        else if(spawnthing.type == 7957)
        {
            sector_t *sector = R_PointInSubsector(spawnthing.x<<FRACBITS, spawnthing.y<<FRACBITS)->sector;
            sector->altlightlevel = spawnthing.angle;
        }
        // [SVE] svillarreal - set bloom threshold
        else if(spawnthing.type == 7969)
        {
            sector_t *sector = R_PointInSubsector(spawnthing.x<<FRACBITS, spawnthing.y<<FRACBITS)->sector;
            sector->bloomthreshold = spawnthing.angle;
        }
        else
        {
            P_SpawnMapThing(&spawnthing);
        }
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//

void P_LoadLineDefs (int lump)
{
    byte*           data;
    int             i;
    maplinedef_t*   mld;
    line_t*         ld;
    vertex_t*       v1;
    vertex_t*       v2;
    angle_t         an;
    
    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
    lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);  
    memset (lines, 0, numlines*sizeof(line_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
    
    mld = (maplinedef_t *)data;
    ld = lines;
    for(i = 0; i < numlines; i++, mld++, ld++)
    {
        ld->flags = SHORT(mld->flags);
        ld->special = SHORT(mld->special);
        ld->tag = SHORT(mld->tag);
        v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
        v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;

        ld->fdx = FIXED2FLOAT(ld->dx);
        ld->fdy = FIXED2FLOAT(ld->dy);
        
        if(!ld->dx)
        {
            ld->slopetype = ST_VERTICAL;
        }
        else if(!ld->dy)
        {
            ld->slopetype = ST_HORIZONTAL;
        }
        else
        {
            if(FixedDiv(ld->dy , ld->dx) > 0)
            {
                ld->slopetype = ST_POSITIVE;
            }
            else
            {
                ld->slopetype = ST_NEGATIVE;
            }
        }
            
        if(v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }

        ld->sidenum[0] = SHORT(mld->sidenum[0]);
        ld->sidenum[1] = SHORT(mld->sidenum[1]);

        if(ld->sidenum[0] != -1)
        {
            ld->frontsector = sides[ld->sidenum[0]].sector;
        }
        else
        {
            ld->frontsector = 0;
        }

        if(ld->sidenum[1] != -1)
        {
            ld->backsector = sides[ld->sidenum[1]].sector;
        }
        else
        {
            ld->backsector = 0;
        }

        // [SVE] svillarreal - compute normals
        an = RB_PointToAngle(ld->dx, ld->dy) - ANG90;

        ld->nx = finecosine[an >> ANGLETOFINESHIFT];
        ld->ny = finesine[an >> ANGLETOFINESHIFT];

        ld->fnx = FIXED2FLOAT(ld->nx);
        ld->fny = FIXED2FLOAT(ld->ny);
        
        ld->validclip[0] = -1;
        ld->validclip[1] = -1;
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadSideDefs
//

void P_LoadSideDefs (int lump)
{
    byte*       data;
    int         i;
    mapsidedef_t*   msd;
    side_t*     sd;
    
    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
    sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);  
    memset (sides, 0, numsides*sizeof(side_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
    
    msd = (mapsidedef_t *)data;
    sd = sides;
    for (i=0 ; i<numsides ; i++, msd++, sd++)
    {
    sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
    sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
    sd->toptexture = R_TextureNumForName(msd->toptexture);
    sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
    sd->midtexture = R_TextureNumForName(msd->midtexture);
    sd->sector = &sectors[SHORT(msd->sector)];
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadBlockMap
//

void P_LoadBlockMap (int lump)
{
    int i;
    int count;
    int lumplen;

    lumplen = W_LumpLength(lump);
    count = lumplen / 2;
    
    blockmaplump = Z_Malloc(lumplen, PU_LEVEL, NULL);
    W_ReadLump(lump, blockmaplump);
    blockmap = blockmaplump + 4;

    // Swap all short integers to native byte ordering.
  
    for (i=0; i<count; i++)
    {
    blockmaplump[i] = SHORT(blockmaplump[i]);
    }
        
    // Read the header

    bmaporgx = blockmaplump[0]<<FRACBITS;
    bmaporgy = blockmaplump[1]<<FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];
    
    // Clear out mobj chains

    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
}

//
// P_AllocLightmapSurfaceInfo
//

static void P_AllocLightmapSurfaceInfo(lightMapInfo_t *lmi, mapSurface_t *surf)
{
    int i;

    // point to which lightmap texture we're using
    lmi->num = surf->lightmapNum;

    // allocate array to hold texture coordinates
    lmi->coords = (float*)Z_Malloc(sizeof(float) * surf->numCoords, PU_LEVEL, 0);

    for(i = 0; i < surf->numCoords; i++)
    {
        // copy texture coord floats
        lmi->coords[i] = lmtexcoords[surf->coordOffset + i];
    }
}

//
// P_LoadSurfaces
//

static void P_LoadSurfaces(const int lump)
{
    int numSurfaces;
    int i;
    seg_t *seg;
    subsector_t *sub;
    mapSurface_t *surfaces, *surf;

    numSurfaces = W_LumpLength(lump) / sizeof(mapSurface_t);
    surfaces = (mapSurface_t*)W_CacheLumpNum(lump, PU_STATIC);

    for(i = 0; i < numSurfaces; i++)
    {
        short index;

        surf = &surfaces[i];
        index = SHORT(surf->typeIndex);

        if(index <= -1)
            continue;

        switch(SHORT(surf->type))
        {
        case SFT_MIDDLESEG:
            if(index < numsegs)
            {
                seg = &segs[index];
                P_AllocLightmapSurfaceInfo(&seg->lightMapInfo[0], surf);
            }
            break;

        case SFT_UPPERSEG:
            if(index < numsegs)
            {
                seg = &segs[index];
                P_AllocLightmapSurfaceInfo(&seg->lightMapInfo[1], surf);
            }
            break;

        case SFT_LOWERSEG:
            if(index < numsegs)
            {
                seg = &segs[index];
                P_AllocLightmapSurfaceInfo(&seg->lightMapInfo[2], surf);
            }
            break;

        case SFT_FLOOR:
            if(index < numsubsectors)
            {
                sub = &subsectors[index];
                P_AllocLightmapSurfaceInfo(&sub->lightMapInfo[0], surf);
            }
            break;

        case SFT_CEILING:
            if(index < numsubsectors)
            {
                sub = &subsectors[index];
                P_AllocLightmapSurfaceInfo(&sub->lightMapInfo[1], surf);
            }
            break;

        default:
            break;
        }
    }

    W_ReleaseLumpNum(lump);
}

//
// P_LoadTextureCoordinates
//

static void P_LoadTextureCoordinates(const int lump)
{
    lmtexcoords = (float*)W_CacheLumpNum(lump, PU_LEVEL);
    W_ReleaseLumpNum(lump);
}

//
// P_LoadSunLight
//

static void P_LoadSunLight(const int lump)
{
    float *data = (float*)W_CacheLumpNum(lump, PU_STATIC);
    float d;

    sunlightdir[0] = data[0];
    sunlightdir[1] = data[1];
    sunlightdir[2] = data[2];

    d = InvSqrt(data[0]*data[0]+data[1]*data[1]);

    sunlightdir2d[0] = sunlightdir[0] * d;
    sunlightdir2d[1] = sunlightdir[1] * d;

    W_ReleaseLumpNum(lump);
}

//
// P_LoadLightMapTextures
//

static void P_LoadLightmapTextures(const int lump)
{
    byte *data;
    byte *textures;
    int *infos;
    int count;
    int width;
    int height;

    data = (byte*)W_CacheLumpNum(lump, PU_STATIC);
    infos = (int*)data;

    count = LONG(infos[0]);
    width = LONG(infos[1]);
    height = LONG(infos[2]);

    textures = data + 12;

    RB_InitLightmapTextures(textures, count, width, height);

    W_ReleaseLumpNum(lump);
}

//
// P_LoadLightGrid
//
// [SVE] svillarreal - loads a lightgrid lump
//

static void P_LoadLightGrid(const int lump)
{
    byte *data;
    byte *bits;
    byte *types;
    byte *rgb;
    int i;
    int numrgb;
    mapLightGrid_t *lg;

    lightgrid.count = 0;

    data = (byte*)W_CacheLumpNum(lump, PU_STATIC);
    lg = (mapLightGrid_t*)data;

    if(lg->count == 0)
    {
        W_ReleaseLumpNum(lump);
        return;
    }

    lightgrid.count = lg->count;
    lightgrid.bits = (byte*)Z_Calloc(1, lg->count, PU_LEVEL, 0);
    lightgrid.types = (byte*)Z_Calloc(1, lg->count, PU_LEVEL, 0);
    lightgrid.rgb = (byte*)Z_Calloc(1, lg->count * 3, PU_LEVEL, 0);

    for(i = 0; i < 3; ++i)
    {
        lightgrid.min[i] = lg->min[i];
        lightgrid.max[i] = lg->max[i];
        lightgrid.gridSize[i] = lg->gridSize[i];
        lightgrid.blockSize[i] = lg->blockSize[i];
        lightgrid.gridUnit[i] = 1.0f / (float)lightgrid.gridSize[i];
    }

    bits = data + sizeof(mapLightGrid_t);
    rgb = data + sizeof(mapLightGrid_t) + lg->count;
    numrgb = 0;

    for(i = 0; i < lg->count; ++i)
    {
        lightgrid.bits[i] = *bits++;
        if(lightgrid.bits[i])
        {
            lightgrid.rgb[i * 3 + 0] = *rgb++;
            lightgrid.rgb[i * 3 + 1] = *rgb++;
            lightgrid.rgb[i * 3 + 2] = *rgb++;
            numrgb++;
        }
    }

    types = (data + sizeof(mapLightGrid_t) + lg->count) + (numrgb * 3);

    for(i = 0; i < lg->count; ++i)
    {
        if(lightgrid.bits[i])
        {
            lightgrid.types[i] = *types++;
        }
    }

    W_ReleaseLumpNum(lump);
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//

void P_GroupLines (void)
{
    line_t**        linebuffer;
    int         i;
    int         j;
    line_t*     li;
    sector_t*       sector;
    subsector_t*    ss;
    seg_t*      seg;
    fixed_t     bbox[4];
    int         block;
    
    // look up sector number for each subsector
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
    seg = &segs[ss->firstline];
    ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    totallines = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
    totallines++;
    li->frontsector->linecount++;

    if (li->backsector && li->backsector != li->frontsector)
    {
        li->backsector->linecount++;
        totallines++;
    }
    }

    // build line tables for each sector    
    linebuffer = Z_Malloc (totallines*sizeof(line_t *), PU_LEVEL, 0);

    for (i=0; i<numsectors; ++i)
    {
        // Assign the line buffer for this sector

        sectors[i].lines = linebuffer;
        linebuffer += sectors[i].linecount;

        // Reset linecount to zero so in the next stage we can count
        // lines into the list.

        sectors[i].linecount = 0;
    }

    // Assign lines to sectors

    for (i=0; i<numlines; ++i)
    { 
        li = &lines[i];

        if (li->frontsector != NULL)
        {
            sector = li->frontsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }

        if (li->backsector != NULL && li->frontsector != li->backsector)
        {
            sector = li->backsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }
    }
    
    // Generate bounding boxes for sectors
    
    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
    M_ClearBox (bbox);

    for (j=0 ; j<sector->linecount; j++)
    {
            li = sector->lines[j];

            M_AddToBox (bbox, li->v1->x, li->v1->y);
            M_AddToBox (bbox, li->v2->x, li->v2->y);
    }

    // set the degenmobj_t to the middle of the bounding box
    sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
    sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
        
    // adjust bounding box to map blocks
    block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block >= bmapheight ? bmapheight-1 : block;
    sector->blockbox[BOXTOP]=block;

    block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block < 0 ? 0 : block;
    sector->blockbox[BOXBOTTOM]=block;

    block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block >= bmapwidth ? bmapwidth-1 : block;
    sector->blockbox[BOXRIGHT]=block;

    block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
    block = block < 0 ? 0 : block;
    sector->blockbox[BOXLEFT]=block;
    }
    
}

// Pad the REJECT lump with extra data when the lump is too small,
// to simulate a REJECT buffer overflow in Vanilla Doom.

static void PadRejectArray(byte *array, unsigned int len)
{
    unsigned int i;
    unsigned int byte_num;
    byte *dest;
    unsigned int padvalue;

    // Values to pad the REJECT array with:

    unsigned int rejectpad[4] =
    {
        ((totallines * 4 + 3) & ~3) + 24,     // Size
        0,                                    // Part of z_zone block header
        50,                                   // PU_LEVEL
        0x1d4a11                              // DOOM_CONST_ZONEID
    };

    // Copy values from rejectpad into the destination array.

    dest = array;

    for (i=0; i<len && i<sizeof(rejectpad); ++i)
    {
        byte_num = i % 4;
        *dest = (rejectpad[i / 4] >> (byte_num * 8)) & 0xff;
        ++dest;
    }

    // We only have a limited pad size.  Print a warning if the
    // REJECT lump is too small.

    if (len > sizeof(rejectpad))
    {
        fprintf(stderr,
                "PadRejectArray: REJECT lump too short to pad! (%i > %i)\n",
                len, (int) sizeof(rejectpad));

        // Pad remaining space with 0 (or 0xff, if specified on command line).

        if (M_CheckParm("-reject_pad_with_ff"))
        {
            padvalue = 0xff;
        }
        else
        {
            padvalue = 0xf00;
        }

        memset(array + sizeof(rejectpad), padvalue, len - sizeof(rejectpad));
    }
}

//
// P_LoadReject
//

static void P_LoadReject(int lumpnum)
{
    int minlength;
    int lumplen;

    // Calculate the size that the REJECT lump *should* be.

    minlength = (numsectors * numsectors + 7) / 8;

    // If the lump meets the minimum length, it can be loaded directly.
    // Otherwise, we need to allocate a buffer of the correct size
    // and pad it with appropriate data.

    lumplen = W_LumpLength(lumpnum);

    if (lumplen >= minlength)
    {
        rejectmatrix = W_CacheLumpNum(lumpnum, PU_LEVEL);
    }
    else
    {
        rejectmatrix = Z_Malloc(minlength, PU_LEVEL, (void**)&rejectmatrix);
        W_ReadLump(lumpnum, rejectmatrix);

        PadRejectArray(rejectmatrix + lumplen, minlength - lumplen);
    }
}

//
// P_LoadPVS
//

static void P_LoadPVS(int lumpnum)
{
    int minlength;
    int lumplen;

    minlength = ((numsubsectors + 7) / 8) * numsubsectors;
    lumplen = W_LumpLength(lumpnum);

    if(lumplen == 0 || lumplen != minlength)
    {
        pvsmatrix = Z_Malloc(minlength, PU_LEVEL, (void**)&pvsmatrix);
        memset(pvsmatrix, 0xff, minlength);
        return;
    }

    pvsmatrix = W_CacheLumpNum(lumpnum, PU_LEVEL);
}

//
// P_SetupLevel
//

void P_SetupLevel(int map, int playermask, skill_t skill)
{
    int     i;
    char    lumpname[9];
    int     lumpnum;
    int     gllumpnum;
    wad_file_t *mapwadfile; // [SVE] svillarreal

    // haleyjd 20110205 [STRIFE]: removed totalitems and wminfo
    totalkills =  totalsecret = 0;

    // [SVE] svillarreal - be sure to set this to false on every level load
    mapwithspecialtags = false;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        // haleyjd 20100830: [STRIFE] Removed secretcount, itemcount
        //         20110205: [STRIFE] Initialize players.allegiance
        players[i].allegiance = i;
        players[i].killcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1; 

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start();

    if(use3drenderer)
    {
        // [SVE] svillarreal - remove old lightmaps from memory
        RB_FreeLightmapTextures();

        // haleyjd 20141016: propagate default lightmaps to lightmaps here
        rbLightmaps = rbLightmapsDefault;
    }

    // [SVE] svillarreal
    ST_ClearDamageMarkers();
    
#if 0 // UNUSED
    if (debugfile)
    {
        Z_FreeTags (PU_LEVEL, INT_MAX);
        Z_FileDumpHeap (debugfile);
    }
    else
#endif
    Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);


    // UNUSED W_Profile ();
    P_InitThinkers ();

    // [SVE] svillarreal
    RB_ClearDecalLinks();

    // [STRIFE] Removed ExMy map support
    if(map < 10)
        DEH_snprintf(lumpname, 9, "map0%i", map);
    else
        DEH_snprintf(lumpname, 9, "map%i", map);

    lumpnum = W_GetNumForName(lumpname);
    mapwadfile = W_WadFileForLumpNum(lumpnum);
    gllumpnum = -1;

    // [SVE] svillarreal
    if(use3drenderer)
    {
        DEH_snprintf(lumpname, 9, "GL_MAP%02d", map);
        if((gllumpnum = W_CheckNumForName(lumpname)) == -1 ||
            W_WadFileForLumpNum(gllumpnum) != mapwadfile)
        {
            I_Error("P_SetupLevel: No GL nodes found (required for high quality renderer)\n");
            return;
        }
    }

    leveltime = 0;

    // note: most of this ordering is important 
    P_LoadBlockMap(lumpnum+ML_BLOCKMAP);
    P_LoadVertexes(lumpnum+ML_VERTEXES);

    // [SVE] svillarreal
    if(use3drenderer)
        P_LoadGLVertexes(gllumpnum+ML_GL_VERTS);

    P_LoadSectors(lumpnum+ML_SECTORS);
    P_LoadSideDefs(lumpnum+ML_SIDEDEFS);
    P_LoadLineDefs(lumpnum+ML_LINEDEFS);

    // [SVE] svillarreal
    if(use3drenderer)
    {
        int lmlumpnum;

        P_LoadSubsectors(gllumpnum+ML_GL_SSECT);
        P_LoadNodes(gllumpnum+ML_GL_NODES);
        P_LoadGLSegs(gllumpnum+ML_GL_SEGS);
        P_LoadPVS(gllumpnum+ML_GL_PVS);

        P_BuildLeafs();

        DEH_snprintf(lumpname, 9, "LM_MAP%02d", map);
        lmlumpnum = W_CheckNumForName(lumpname);

        lightmapCount = 0;
        memset(&lightgrid, 0, sizeof(lightGridInfo_t));

        if(lmlumpnum != -1 && W_WadFileForLumpNum(lmlumpnum) == mapwadfile)
        {
            P_LoadSunLight(lmlumpnum + ML_LM_SUN);
            P_LoadTextureCoordinates(lmlumpnum + ML_LM_TXCRD);
            P_LoadSurfaces(lmlumpnum + ML_LM_SURFS);
            P_LoadLightGrid(lmlumpnum + ML_LM_CELLS);
            P_LoadLightmapTextures(lmlumpnum + ML_LM_LMAPS);

            Z_Free(lmtexcoords);
        }
    }
    else
    {
        P_LoadSubsectors(lumpnum+ML_SSECTORS);
        P_LoadNodes(lumpnum+ML_NODES);
        P_LoadSegs(lumpnum+ML_SEGS);
    }

    // haleyjd 20140904: [SVE] create sector interpolation data
    P_CreateSectorInterps();

    P_GroupLines();
    P_LoadReject(lumpnum+ML_REJECT);

    //bodyqueslot = 0; [STRIFE] unused
    numdeathmatchstarts = 0; // haleyjd 20140819: [SVE] rem dmspots limit
    numctcbluestarts = numctcredstarts = 0; // [SVE]: CTC
    capturethechalice = false;
    ctcbluescore = ctcredscore = 0;
    P_LoadThings(lumpnum+ML_THINGS);
    
    // if deathmatch, randomly spawn the active players
    if(deathmatch)
    {
        for(i = 0; i < MAXPLAYERS; i++)
            if (playeringame[i])
            {
                players[i].mo = NULL;
                G_DeathMatchSpawnPlayer (i);
            }

    }

    // clear special respawning que
    iquehead = iquetail = 0;

    // [SVE] svillarreal - Spontaneous Combustion achievement vars
    curignitemobj = NULL;
    numignitechains = 0;

    // set up world state
    P_SpawnSpecials ();

    // build subsector connect matrix
    // UNUSED P_ConnectSubsectors ();

    // preload graphics
    if(precache && !use3drenderer)
        R_PrecacheLevel ();

    // [SVE] svillarreal
    if(use3drenderer)
    {
        RB_PrecacheLevel();
        RB_InitLightMarks();
        DL_Init();
    }

    //printf ("free memory: 0x%x\n", Z_FreeMemory());
}



//
// P_Init
//

void P_Init (void)
{
    P_InitSwitchList();
    P_InitPicAnims();
    P_InitTerrainTypes();   // villsa [STRIFE]
    R_InitSprites(sprnames);

    // [SVE] svillarreal
    RB_InitDynLights();
    RB_InitWallShades();

    // [SVE] haleyjd
    P_InitLocations();
}
