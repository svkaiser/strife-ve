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
//      Refresh/rendering module, shared data struct definitions.
//


#ifndef __R_DEFS__
#define __R_DEFS__


// Screenwidth.
#include "doomdef.h"

// Some more or less basic data types
// we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "i_video.h"

#include "v_patch.h"

// [SVE] svillarreal
#include "rb_decal.h"


// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
#define SIL_NONE        0
#define SIL_BOTTOM      1
#define SIL_TOP         2
#define SIL_BOTH        3

#define MAXDRAWSEGS     256





//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//  like some DOOM-alikes ("wt", "WebView") did.
//
typedef struct
{
    fixed_t x;
    fixed_t y;

    // [SVE] svillarreal - new properties below
    
    float   fx;
    float   fy;
    
    // info for occlusion
    int     validcount;
    angle_t clipspan;

} vertex_t;


// Forward of LineDefs, for Sectors.
struct line_s;

// Each sector has a degenmobj_t in its center
//  for sound origin purposes.
// I suppose this does not handle sound from
//  moving objects (doppler), because
//  position is prolly just buffered, not
//  updated.
typedef struct
{
    thinker_t       thinker;    // not used for anything
    fixed_t     x;
    fixed_t     y;
    fixed_t     z;

} degenmobj_t;

// sector interpolation values
// haleyjd 20140904: [SVE]
typedef struct sectorinterp_s
{
    boolean interpolated;      // if true, interpolated

    fixed_t prevfloorheight;   // previous values, stored for interpolation
    fixed_t prevceilingheight;

    fixed_t backfloorheight;   // backup values, used as cache during rendering
    fixed_t backceilingheight;
} sectorinterp_t;

enum rbshadeflags_e
{
    RBSF_BRIGHT      = 0x00000001, // bright
    RBSF_CONTRAST    = 0x00000002, // increases contrast by fading to darker light level
    RBSF_FLOOR       = 0x00000004, // applies to floors
    RBSF_CEILING     = 0x00000008, // applies to ceilings
    RBSF_THINGS      = 0x00000010, // applies to things in sector
    RBSF_LOWCLAMP    = 0x00000020, // don't stretch RGB for lower sidedef
    RBSF_SKY         = 0x00000040, // sky effect
    RBSF_SEGLIGHTING = 0x00000080, // preserves per-seg lighting (ie, fake contrast)

    // combos
    RBSF_HOT      = (RBSF_BRIGHT|RBSF_CONTRAST)
};

// svillarreal: [SVE] Hardware renderer color shading information
typedef struct rbShadeDef_s
{
    char flatname[9];
    byte r;
    byte g;
    byte b;
    unsigned int flags;
    int flatnum;

    int self;  // self-index in table
    int first; // first on hash chain
    int next;  // next on hash chain

    int h, s, v; // cached HSV color
} rbShadeDef_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
typedef struct sector_s
{
    fixed_t floorheight;
    fixed_t ceilingheight;
    short   floorpic;
    short   ceilingpic;
    short   lightlevel;
    short   special;
    short   tag;

    // 0 = untraversed, 1,2 = sndlines -1
    int     soundtraversed;

    // thing that made a sound (or null)
    mobj_t* soundtarget;

    // mapblock bounding box for height changes
    int     blockbox[4];

    // origin for any sounds played by the sector
    degenmobj_t soundorg;

    // if == validcount, already checked
    int     validcount;
    
    // [SVE] svillarreal
    int     validclip[2];

    // list of mobjs in sector
    mobj_t* thinglist;

    // thinker_t for reversable actions
    void*   specialdata;

    int         linecount;
    struct line_s** lines;  // [linecount] size
    
    // haleyjd 20140907: [SVE] hardware shading info
    rbShadeDef_t *floorshade;
    rbShadeDef_t *ceilingshade;

    // [SVE] svillarreal - special light levels
    int altlightlevel;

    // [SVE] svillarreal - minimum bloom threshold
    short bloomthreshold;

    // [SVE] svillarreal - decal links
    struct rbDecal_s *decallist;

} sector_t;




//
// The SideDef.
//

typedef struct
{
    // add this to the calculated texture column
    fixed_t textureoffset;
    
    // add this to the calculated texture top
    fixed_t rowoffset;

    // Texture indices.
    // We do not maintain names here. 
    short   toptexture;
    short   bottomtexture;
    short   midtexture;

    // Sector the SideDef is facing.
    sector_t*   sector;
    
} side_t;



//
// Move clipping aid for LineDefs.
//
typedef enum
{
    ST_HORIZONTAL,
    ST_VERTICAL,
    ST_POSITIVE,
    ST_NEGATIVE

} slopetype_t;



typedef struct line_s
{
    // Vertices, from v1 to v2.
    vertex_t*   v1;
    vertex_t*   v2;

    // Precalculated v2 - v1 for side checking.
    fixed_t dx;
    fixed_t dy;
    
    // [SVE] svillarreal - float verion of dx/dy
    float   fdx;
    float   fdy;

    // Animation related.
    short   flags;
    short   special;
    short   tag;

    // Visual appearance: SideDefs.
    //  sidenum[1] will be -1 if one sided
    short   sidenum[2];         

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
    fixed_t bbox[4];

    // To aid move clipping.
    slopetype_t slopetype;

    // Front and back sector.
    // Note: redundant? Can be retrieved from SideDefs.
    sector_t*   frontsector;
    sector_t*   backsector;

    // if == validcount, already checked
    int     validcount;

    // thinker_t for reversable actions
    void*   specialdata;        

    // [SVE] svillarreal - normals
    fixed_t nx, ny;
    float fnx, fny;
    
    // [SVE] svillarreal
    int     validclip[2];

} line_t;


// [SVE] svillarreal - lightmap info struct
typedef struct
{
    int         num;
    float       *coords;
} lightMapInfo_t;

typedef enum
{
    LGT_NONE        = 0,    // does nothing
    LGT_SUNSHADE,           // halves the rgb color
    LGT_SUN,                // use sector light/color instead
    NUMLIGHTGRIDTYPES
} rbLightGridType_t;

// [SVE] svillarreal - lightgrid info struct
typedef struct
{
    int         count;
    short       min[3];
    short       max[3];
    short       gridSize[3];
    short       blockSize[3];
    float       gridUnit[3];
    byte        *bits;
    byte        *types;
    byte        *rgb;
} lightGridInfo_t;

//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs,
//  indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
    sector_t*       sector;
    short           numlines;
    short           firstline;

    // [SVE] svillarreal
    word            numleafs;
    word            leaf;
    lightMapInfo_t  lightMapInfo[2];
} subsector_t;



//
// The LineSeg.
//
typedef struct
{
    vertex_t*       v1;
    vertex_t*       v2;
    
    fixed_t         offset;

    angle_t         angle;

    side_t*         sidedef;
    line_t*         linedef;

    // Sector references.
    // Could be retrieved from linedef, too.
    // backsector is NULL for one sided lines
    sector_t*       frontsector;
    sector_t*       backsector;

    // [SVE] svillarreal
    float           length;
    lightMapInfo_t  lightMapInfo[3];
} seg_t;

//
// [SVE] svillarreal - LEAFS structure
//
typedef struct {
    vertex_t    *vertex;
    seg_t       *seg;
} leaf_t;

//
// BSP node.
//
typedef struct
{
    // Partition line.
    fixed_t x;
    fixed_t y;
    fixed_t dx;
    fixed_t dy;

    // Bounding box for each child.
    fixed_t bbox[2][4];

    // If NF_SUBSECTOR its a subsector.
    unsigned short children[2];
    
} node_t;




// PC direct to screen pointers
//B UNUSED - keep till detailshift in r_draw.c resolved
//extern byte*  destview;
//extern byte*  destscreen;





//
// OTHER TYPES
//

// This could be wider for >8 bit display.
// Indeed, true color support is posibble
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even us emore than 32 levels.
typedef byte    lighttable_t;   




//
// ?
//
typedef struct drawseg_s
{
    seg_t*      curline;
    int         x1;
    int         x2;

    fixed_t     scale1;
    fixed_t     scale2;
    fixed_t     scalestep;

    // 0=none, 1=bottom, 2=top, 3=both
    int         silhouette;

    // do not clip sprites above this
    fixed_t     bsilheight;

    // do not clip sprites below this
    fixed_t     tsilheight;
    
    // Pointers to lists for sprite clipping,
    //  all three adjusted so [x1] is first value.
    short*      sprtopclip;     
    short*      sprbottomclip;  
    short*      maskedtexturecol;
    
} drawseg_t;

// A vissprite_t is a thing
//  that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
typedef struct vissprite_s
{
    // Doubly linked list.
    struct vissprite_s* prev;
    struct vissprite_s* next;
    
    int         x1;
    int         x2;

    // for line side calculation
    fixed_t     gx;
    fixed_t     gy;     

    // global bottom / top for silhouette clipping
    fixed_t     gz;
    fixed_t     gzt;

    // horizontal position of x1
    fixed_t     startfrac;
    
    fixed_t     scale;
    
    // negative if flipped
    fixed_t     xiscale;    

    fixed_t     texturemid;
    int         patch;

    // for color translation and shadow draw,
    //  maxbright frames as well
    lighttable_t*   colormap;
   
    int         mobjflags;
    
} vissprite_t;


//  
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//
typedef struct
{
    // If false use 0 for any position.
    // Note: as eight entries are available,
    //  we might as well insert the same name eight times.
    boolean rotate;

    // Lump to use for view angles 0-7.
    short   lump[8];

    // Flip bit (1 = flip) to use for view angles 0-7.
    byte    flip[8];
    
} spriteframe_t;



//
// A sprite definition:
//  a number of animation frames.
//
typedef struct
{
    int         numframes;
    spriteframe_t*  spriteframes;

} spritedef_t;



//
// Now what is a visplane, anyway?
// 
typedef struct visplane_s
{
  struct visplane_s *next; // haleyjd [SVE]
  fixed_t       height;
  int           picnum;
  int           lightlevel;
  int           minx;
  int           maxx;
  
  // leave pads for [minx-1]/[maxx+1]
  
  byte      pad1;
  // Here lies the rub for all
  //  dynamic resize/change of resolution.
  byte      top[SCREENWIDTH];
  byte      pad2;
  byte      pad3;
  // See above.
  byte      bottom[SCREENWIDTH];
  byte      pad4;

} visplane_t;




#endif
