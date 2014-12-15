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
//    Basic decal system
//

#include <math.h>

#include "rb_main.h"
#include "rb_config.h"
#include "rb_drawlist.h"
#include "rb_geom.h"
#include "rb_data.h"
#include "rb_draw.h"
#include "rb_decal.h"
#include "rb_matrix.h"
#include "m_random.h"
#include "p_local.h"
#include "r_state.h"
#include "r_main.h"
#include "z_zone.h"
#include "w_wad.h"
#include "deh_str.h"
#include "m_parser.h"

static rbDecal_t decalhead;
static int activedecals;
static line_t *decalwall = NULL;

static fixed_t decal_x;
static fixed_t decal_y;
static fixed_t decal_z;

static rbDecalDef_t *decalDefs;
static int numDecalDefs;

//
// RB_InitDecals
//

void RB_InitDecals(void)
{
    lexer_t *lexer;

    numDecalDefs = 0;
    if(!(lexer = M_ParserOpen("DECLDEF")))
    {
        return;
    }

    // count the number of decal defs by scanning for each decal block
    while(M_ParserCheckState(lexer))
    {
        M_ParserFind(lexer);
        if(lexer->tokentype == TK_IDENIFIER && M_ParserMatches(lexer, "decal"))
        {
            numDecalDefs++;
        }
    }

    if(numDecalDefs)
    {
        const int size = sizeof(rbDecalDef_t) * numDecalDefs;
        rbDecalDef_t *decalDef;

        decalDefs = (rbDecalDef_t*)Z_Calloc(1, size, PU_STATIC, 0);
        M_ParserReset(lexer);

        decalDef = decalDefs;

        while(M_ParserCheckState(lexer))
        {
            M_ParserFind(lexer);

            if(M_ParserMatches(lexer, "decal"))
            {
                // check for left bracket and then jump to next token
                M_ParserExpectNextToken(lexer, TK_LBRACK);
                M_ParserFind(lexer);

                // set default values
                decalDef->lumpnum = -1;
                decalDef->fadetime = 100;
                decalDef->isgore = false;
                decalDef->lifetime = 1000;
                decalDef->scale = 1.0f;
                decalDef->startingAlpha = 255;

                // keep searching until end of block
                while(lexer->tokentype != TK_RBRACK)
                {
                    if(M_ParserMatches(lexer, "name"))
                    {
                        M_ParserGetString(lexer);
                        decalDef->lumpnum = W_GetNumForName((char*)M_ParserStringToken(lexer));
                    }
                    else if(M_ParserMatches(lexer, "thingtype"))
                    {
                        decalDef->mobjtype = M_ParserGetNumber(lexer);
                        if(decalDef->mobjtype > 0 && decalDef->mobjtype < NUMMOBJTYPES)
                        {
                            mobjinfo[decalDef->mobjtype].flags2 |= MF2_MARKDECAL;
                        }
                    }
                    else if(M_ParserMatches(lexer, "lifetime"))
                    {
                        decalDef->lifetime = M_ParserGetNumber(lexer);
                    }
                    else if(M_ParserMatches(lexer, "fadetime"))
                    {
                        decalDef->fadetime = M_ParserGetNumber(lexer);
                    }
                    else if(M_ParserMatches(lexer, "initialalpha"))
                    {
                        decalDef->startingAlpha = M_ParserGetNumber(lexer);
                    }
                    else if(M_ParserMatches(lexer, "randomRotations"))
                    {
                        decalDef->randRotate = M_ParserGetNumber(lexer);
                    }
                    else if(M_ParserMatches(lexer, "scale"))
                    {
                        decalDef->scale = (float)M_ParserGetFloat(lexer);
                    }
                    else if(M_ParserMatches(lexer, "randomScaleFactor"))
                    {
                        decalDef->randScaleFactor = (float)M_ParserGetFloat(lexer);
                    }
                    else if(M_ParserMatches(lexer, "noterrain"))
                    {
                        decalDef->noterrain = M_ParserGetNumber(lexer);
                    }
                    else if(M_ParserMatches(lexer, "isgore"))
                    {
                        decalDef->isgore = M_ParserGetNumber(lexer);
                    }
                    else if(M_ParserMatches(lexer, "variations"))
                    {
                        decalDef->count = M_ParserGetNumber(lexer);
                    }

                    // get next token
                    M_ParserFind(lexer);
                }

                decalDef++;
            }
        }
    }

    M_ParserClose();
}

//
// RB_GetDecalDef
//

static rbDecalDef_t *RB_GetDecalDef(mobjtype_t type)
{
    int i;
    
    for(i = 0; numDecalDefs; ++i)
    {
        if(decalDefs[i].mobjtype == type)
        {
            if(decalDefs[i].isgore && !d_maxgore)
            {
                return NULL;
            }

            return &decalDefs[i];
        }
    }
    
    return NULL;
}

//
// RB_LinkDecal
//

static void RB_LinkDecal(rbDecal_t *decal)
{
    sector_t *sec;

    decal->ssect = R_PointInSubsector(decal->x, decal->y);
    sec = decal->ssect->sector;

    decal->sprev = NULL;
    decal->snext = sec->decallist;

    if(sec->decallist)
    {
        sec->decallist->sprev = decal;
    }

    sec->decallist = decal;
}

//
// RB_UnlinkDecal
//

static void RB_UnlinkDecal(rbDecal_t *decal)
{
    if(decal->snext)
    {
        decal->snext->sprev = decal->sprev;
    }

    if(decal->sprev)
    {
        decal->sprev->snext = decal->snext;
    }
    else
    {
        decal->ssect->sector->decallist = decal->snext;
    }
}

//
// RB_UpdateDecals
//

void RB_UpdateDecals(void)
{
    rbDecal_t *decal;

    for(decal = decalhead.next; decal != &decalhead; decal = decal->next)
    {
        if(decal->tics <= decal->fadetime)
        {
            decal->alpha -= (((float)decal->def->startingAlpha / decal->fadetime) / 255.0f);

            if(decal->alpha < 0)
            {
                decal->alpha = 0;
            }
        }

        if(activedecals >= rbMaxDecals || !decal->tics--)
        {
            rbDecal_t *decalTmp = decal;
            rbDecal_t *next = decalTmp->next;

            (next->prev = decal = decalTmp->prev)->next = next;

            RB_UnlinkDecal(decalTmp);
            Z_Free(decalTmp);

            activedecals--;
        }
    }
}

//
// RB_ClearDecalLinks
//

void RB_ClearDecalLinks(void)
{
    decalhead.next = decalhead.prev = &decalhead;
    activedecals = 0;
}

//
// RB_IntersectDecalSegment
//

static boolean RB_IntersectDecalSegment(float x1, float y1, float x2, float y2, line_t *line,
                                        float *x, float *y)
{
    float ax, ay;
    float bx, by;
    float cx, cy;
    float dx, dy;
    float d, c, s, u;
    float newX;
    float ab;

    ax = x1;
    ay = y1;
    bx = x2;
    by = y2;
    cx = line->v1->fx;
    cy = line->v1->fy;
    dx = line->v2->fx;
    dy = line->v2->fy;
    
    if((ax == bx && ay == by) || (cx == dx && cy == dy))
    {
        // zero length
        return false;
    }
    
    if((ax == cx && ay == cy) ||
       (bx == cx && by == cy) ||
       (ax == dx && ay == dy) ||
       (bx == dx && by == dy))
    {
        // shares end point
        return false;
    }
    
    // translate to origin
    bx -= ax; by -= ay;
    cx -= ax; cy -= ay;
    dx -= ax; dy -= ay;

    // normalize
    u = bx * bx + by * by;
    d = InvSqrt(u);
    c = bx * d;
    s = by * d;

    // rotate points c and d so they're on the positive x axis
    newX = cx * c + cy * s;
    cy = cy * c - cx * s;
    cx = newX;
    
    newX = dx * c + dy * s;
    dy = dy * c - dx * s;
    dx = newX;
    
    if((cy < 0 && dy < 0) || (cy >= 0 && dy >= 0))
    {
        // c and d didn't cross
        return false;
    }
    
    ab = dx + (cx - dx) * dy / (dy - cy);
    
    if(ab < 0 || ab > (u * d))
    {
        // c and d crosses but outside of points a and b
        return false;
    }

    // lerp
    *x = ax + ab * c;
    *y = ay + ab * s;

    return true;
}

//
// RB_CarveDecal
//

static void RB_CarveDecal(rbDecal_t *decal)
{
    line_t *line;
    sector_t *sector;
    int side;
    int leftcount;
    int rightcount;
    byte pointsides[NUM_DECAL_POINTS];
    int i, j;
    
    sector = decal->ssect->sector;
    
    for(i = 0; i < sector->linecount; ++i)
    {
        boolean ok = false;

        line = sector->lines[i];
        
        if(line->backsector)
        {
            if(line->frontsector == line->backsector)
            {
                // ignore 2-sided lines
                continue;
            }
            
            if(decal->type == DCT_FLOOR)
            {
                if(line->backsector->floorheight == line->frontsector->floorheight)
                {
                    // ignore flat floor heights
                    continue;
                }
            }
            else if(decal->type == DCT_CEILING)
            {
                if(line->backsector->ceilingheight == line->frontsector->ceilingheight)
                {
                    // ignore flat ceiling heights
                    continue;
                }
            }
        }
        
        leftcount = 0;
        rightcount = 0;

        side = P_PointOnLineSide(decal->x, decal->y, line);
        memset(pointsides, 0, sizeof(NUM_DECAL_POINTS));

        for(j = 0; j < decal->numpoints; ++j)
        {
            pointsides[j] = P_PointOnLineSide(FLOAT2FIXED(decal->points[j].x),
                                              FLOAT2FIXED(decal->points[j].y),
                                              line);

            leftcount += pointsides[j];
            rightcount += pointsides[j] ^ 1;
        }

        if(leftcount == decal->numpoints || rightcount == decal->numpoints)
        {
            // all points are on one side
            continue;
        }
        
        //
        // some points are on the other side. begin cutting
        //
        for(j = 0; j < decal->numpoints; ++j)
        {
            int idx1 = j;
            int idx2 = j + 1;
            
            if(idx2 == decal->numpoints)
            {
                idx2 -= decal->numpoints;
            }
            
            // check if segments crosses sides
            if(pointsides[idx1] != pointsides[idx2])
            {
                float x, y;
                float mx, my;
                
                if(!RB_IntersectDecalSegment(decal->points[idx1].x,
                                             decal->points[idx1].y,
                                             decal->points[idx2].x,
                                             decal->points[idx2].y,
                                             line, &x, &y))
                {
                    // didn't actually intersect
                    continue;
                }

                ok = true;

                if(decal->numpoints+1 >= NUM_DECAL_POINTS)
                {
                    fprintf(stderr, "RB_CarveDecal: Vertex overflow\n");
                    return;
                }

                decal->numpoints++;

                mx = decal->points[idx2].x - decal->points[idx1].x;
                my = decal->points[idx2].y - decal->points[idx1].y;

                // make room for new vertex
                memmove(&decal->points[idx2 + 1], &decal->points[idx2],
                        (decal->numpoints - idx2 - 1) * sizeof(rbDecalVertex_t));
                
                decal->points[idx2].x = x;
                decal->points[idx2].y = y;
                decal->points[idx2].z = decal->points[0].z;

                if(mx == 0)
                {
                    // segment is perfectly straight, vertically.
                    // tu should be already known
                    decal->points[idx2].tu = decal->points[idx2+1].tu;
                }
                else
                {
                    float mu = (x - decal->points[idx1].x) / mx;
                    float tu2 = decal->points[idx2+1].tu;
                    float tu1 = decal->points[idx1].tu;

                    decal->points[idx2].tu = (tu2 - tu1) * mu + tu1;

                    // goddamnit this is not going to work...
                    if(decal->points[idx2].tu > 1) decal->points[idx2].tu = 1;
                    if(decal->points[idx2].tu < -1) decal->points[idx2].tu = -1;
                }

                if(my == 0)
                {
                    // segment is perfectly straight, horizontally.
                    // tv should be already known
                    decal->points[idx2].tv = decal->points[idx2+1].tv;
                }
                else
                {
                    float mu = (y - decal->points[idx1].y) / my;
                    float tv2 = decal->points[idx2+1].tv;
                    float tv1 = decal->points[idx1].tv;

                    decal->points[idx2].tv = (tv2 - tv1) * mu + tv1;

                    // goddamnit this is not going to work...
                    if(decal->points[idx2].tv > 1) decal->points[idx2].tv = 1;
                    if(decal->points[idx2].tv < -1) decal->points[idx2].tv = -1;
                }
                
                memmove(&pointsides[idx2 + 1], &pointsides[idx2], decal->numpoints - idx2 - 1);
                pointsides[idx2] = side;

                // skip new vertex
                j++;
            }
        }

        if(ok)
        {
            // discard verts that's on the other side
            for(j = 0; j < decal->numpoints; ++j)
            {
                if(pointsides[j] != side)
                {
                    // shift next item in array down
                    memmove(&decal->points[j], &decal->points[j + 1],
                            (decal->numpoints - j - 1) * sizeof(rbDecalVertex_t));
                    memmove(&pointsides[j], &pointsides[j + 1], decal->numpoints - j - 1);
                    
                    decal->numpoints--;
                    j--;
                }
            }
        }
    }
}

//
// RB_CreateDecal
//

static rbDecal_t *RB_CreateDecal(rbDecalDef_t *decalDef)
{
    rbDecal_t *decal;
    
    decal = Z_Calloc(1, sizeof(*decal), PU_LEVEL, 0);
    decal->def = decalDef;
    decal->tics = decalDef->lifetime;
    decal->lump = decalDef->lumpnum + (M_Random() % decalDef->count);
    decal->offset = M_Random() & 7;
    decal->alpha = (float)decalDef->startingAlpha / 255.0f;
    decal->fadetime = decalDef->fadetime;
    decal->scale = decalDef->scale;
    decal->stickSector = NULL;
    decal->initialStickZ = 0;
    
    if(decalDef->randScaleFactor != 0)
    {
        decal->scale += (decalDef->randScaleFactor * ((float)M_Random() / 255.0f));
    }
    
    if(decalDef->randRotate)
    {
        decal->rotation = DEG2RAD(((M_Random() - M_Random()) + ((M_Random() & 3) * 35)) - M_PI);
    }
    else
    {
        decal->rotation = 0;
    }

    decalhead.prev->next = decal;
    decal->next = &decalhead;
    decal->prev = decalhead.prev;
    decalhead.prev = decal;

    return decal;
}

//
// RB_RotateDecalTextureCoords
//

static void RB_RotateDecalTextureCoords(rbDecal_t *decal)
{
    matrix mtx1, mtx2, mtx3;
    int i;

    MTX_IdentityZ(mtx1, decal->rotation);
    MTX_Identity(mtx2);
    MTX_SetTranslation(mtx2, -0.5f, -0.5f, 0);
    MTX_MultiplyRotations(mtx3, mtx2, mtx1);
    mtx3[12] += 0.5f;
    mtx3[13] += 0.5f;

    for(i = 0; i < decal->numpoints; ++i)
    {
        float _x = decal->points[i].tu;
        float _y = decal->points[i].tv;
        
        decal->points[i].tu = mtx3[ 4] * _y + mtx3[ 0] * _x + mtx3[12];
        decal->points[i].tv = mtx3[ 5] * _y + mtx3[ 1] * _x + mtx3[13];
    }
}

//
// RB_ClampWallDecalToLine
//

static void RB_ClampWallDecalToLine(rbDecalVertex_t *point, boolean backsector,
                                    float cHeight, float fHeight, float z,
                                    float lx1, float ly1, float lx2, float ly2)
{
    // clamp x
    if(lx2 > lx1)
    {
        if(point->x < lx1) point->x = lx1;
        if(point->x > lx2) point->x = lx2;
    }
    else
    {
        if(point->x < lx2) point->x = lx2;
        if(point->x > lx1) point->x = lx1;
    }
    
    // clamp y
    if(ly2 > ly1)
    {
        if(point->y < ly1) point->y = ly1;
        if(point->y > ly2) point->y = ly2;
    }
    else
    {
        if(point->y < ly2) point->y = ly2;
        if(point->y > ly1) point->y = ly1;
    }
    
    // clamp z
    if(backsector)
    {
        if(z > cHeight && point->z < cHeight)
        {
            point->z = cHeight;
        }
        
        if(z < fHeight && point->z > fHeight)
        {
            point->z = fHeight;
        }
    }
}

//
// PIT_DecalCheckLine
//


static boolean PIT_DecalCheckLine(intercept_t *in)
{
    line_t *ld = in->d.line;

    if(!ld->backsector)
    {
        decalwall = ld;
        return false;
    }
    else
    {
        if(decal_z < ld->backsector->floorheight || decal_z > ld->backsector->ceilingheight)
        {
            decalwall = ld;
            return false;
        }
    }

    return true;
}

//
// RB_SpawnWallDecal
//

void RB_SpawnWallDecal(mobj_t *mobj)
{
    int i;
    line_t *line;
    rbDecal_t *decal;
    rbDecalDef_t *decalDef;
    float dx, dy;
    float cx, cy;
    float fx, fy, fz;
    float nx, ny;
    float s, c;
    float lx1, lx2;
    float ly1, ly2;
    float d;
    float an;
    float offs;
    float cHeight = 0;
    float fHeight = 0;
    float size;

    if(!rbDecals)
    {
        return;
    }

    decalwall = NULL;
    decal_x = mobj->x;
    decal_y = mobj->y;
    decal_z = mobj->z;

    if(P_PathTraverse(decal_x, decal_y,
                      mobj->x + FixedMul(mobj->momx, 10*FRACUNIT),
                      mobj->y + FixedMul(mobj->momy, 10*FRACUNIT),
                      PT_ADDLINES, PIT_DecalCheckLine))
    {
        return;
    }

    line = decalwall;

    if(!line || !(decalDef = RB_GetDecalDef(mobj->type)))
    {
        return;
    }
    
    decal = RB_CreateDecal(decalDef);
    
    decal->x = mobj->x;
    decal->y = mobj->y;
    decal->z = mobj->z;
    decal->type = DCT_WALL;

    // look for a sector to stick to
    if(line->backsector)
    {
        decal->stickSector = line->backsector;
        if(decal->z > line->backsector->ceilingheight)
        {
            decal->type = DCT_UPPERWALL;
            decal->initialStickZ = line->backsector->ceilingheight;
        }
        else if(decal->z < line->backsector->floorheight)
        {
            decal->type = DCT_LOWERWALL;
            decal->initialStickZ = line->backsector->floorheight;
        }
    }

    RB_LinkDecal(decal);
    
    dx = line->fdx;
    dy = line->fdy;
    
    lx1 = line->v1->fx;
    ly1 = line->v1->fy;
    lx2 = line->v2->fx;
    ly2 = line->v2->fy;
    
    // get line angle (direction)
    an = atan2f(dx, dy);
    c = cosf(an);
    s = sinf(an);
    
    // get line distance
    cx = lx1 - FIXED2FLOAT(decal->x);
    cy = ly1 - FIXED2FLOAT(decal->y);
    
    d = (dx * cy - dy * cx) * InvSqrt(dx * dx + dy * dy);
    
    // get nudge direction
    an -= DEG2RAD(90.0f);
    offs = (float)decal->offset / 256.0f;

    nx = (d - 0.8f) * sinf(an);
    ny = (d - 0.8f) * cosf(an);

    /*
  2 ----------- 3
    |         |
    |         |
    |         |
    |         |
    |         |
  1 ----------- 0
    */

    decal->numpoints = 4;
    size = 16 * decal->scale;

    decal->points[0].x = decal->points[3].x =  size * s;
    decal->points[0].y = decal->points[3].y =  size * c;
    decal->points[1].x = decal->points[2].x = -size * s;
    decal->points[1].y = decal->points[2].y = -size * c;
    decal->points[2].z = decal->points[3].z =  size;
    decal->points[0].z = decal->points[1].z = -size;

    fx = FIXED2FLOAT(decal->x);
    fy = FIXED2FLOAT(decal->y);
    fz = FIXED2FLOAT(decal->z);

    decal->points[0].tu = decal->points[3].tu = 0;
    decal->points[0].tv = decal->points[1].tv = 0;
    decal->points[1].tu = decal->points[2].tu = 1;
    decal->points[3].tv = decal->points[2].tv = 1;
    
    if(line->backsector)
    {
        cHeight = FIXED2FLOAT(line->backsector->ceilingheight);
        fHeight = FIXED2FLOAT(line->backsector->floorheight);
    }
    
    for(i = 0; i < decal->numpoints; ++i)
    {
        decal->points[i].x += fx;
        decal->points[i].y += fy;
        decal->points[i].z += fz;
        
        // nudge decal to be closer to the wall
        decal->points[i].x += nx;
        decal->points[i].y += ny;
        
        RB_ClampWallDecalToLine(&decal->points[i], line->backsector != NULL,
                                cHeight, fHeight, fz,
                                lx1, ly1, lx2, ly2);

        // jitter offset a bit
        decal->points[i].x -= (nx * offs);
        decal->points[i].y -= (ny * offs);
    }

    RB_RotateDecalTextureCoords(decal);
    activedecals++;
}

//
// RB_SpawnFloorDecal
//

void RB_SpawnFloorDecal(mobj_t *mobj, boolean floor)
{
    rbDecal_t *decal;
    rbDecalDef_t *decalDef;
    sector_t *sector;
    int i;
    float fx, fy, fz;
    float size;
    
    if(!rbDecals || !(decalDef = RB_GetDecalDef(mobj->type)))
    {
        return;
    }

    if((!floor && mobj->subsector->sector->ceilingpic == skyflatnum) ||
        (floor && mobj->subsector->sector->floorpic == skyflatnum))
    {
        return;
    }

    if(floor && decalDef->noterrain && P_GetTerrainType(mobj) != FLOOR_SOLID)
    {
        return;
    }
    
    decal = RB_CreateDecal(decalDef);
    
    decal->x = mobj->x;
    decal->y = mobj->y;

    RB_LinkDecal(decal);

    sector = decal->ssect->sector;

    decal->z = floor ? sector->floorheight : sector->ceilingheight;
    decal->type = floor ? DCT_FLOOR : DCT_CEILING;

    // stick to the floor
    decal->stickSector = decal->ssect->sector;
    decal->initialStickZ = decal->z;
    
    /*
  2 ----------- 3
    |         |
    |         |
    |         |
    |         |
    |         |
  1 ----------- 0
    */

    decal->numpoints = 4;
    size = 16 * decal->scale;
    
    if(floor)
    {
        decal->points[0].x = decal->points[3].x =  size;
        decal->points[0].y = decal->points[1].y = -size;
        decal->points[1].x = decal->points[2].x = -size;
        decal->points[3].y = decal->points[2].y =  size;
    }
    else
    {
        decal->points[3].x = decal->points[0].x =  size;
        decal->points[3].y = decal->points[2].y = -size;
        decal->points[2].x = decal->points[1].x = -size;
        decal->points[0].y = decal->points[1].y =  size;
    }

    decal->points[0].tu = decal->points[3].tu = 0;
    decal->points[0].tv = decal->points[1].tv = 0;
    decal->points[1].tu = decal->points[2].tu = 1;
    decal->points[3].tv = decal->points[2].tv = 1;
    
    fx = FIXED2FLOAT(decal->x);
    fy = FIXED2FLOAT(decal->y);
    fz = FIXED2FLOAT(decal->z) + ((float)decal->offset / 256.0f);
    
    for(i = 0; i < decal->numpoints; ++i)
    {
        decal->points[i].x += fx;
        decal->points[i].y += fy;
        decal->points[i].z += fz;
    }
    
    RB_CarveDecal(decal);
    RB_RotateDecalTextureCoords(decal);
    activedecals++;
}

//
// RB_GenerateDecal
//

static boolean RB_GenerateDecal(vtxlist_t *vl, int *drawcount)
{
    vtx_t *v;
    rbDecal_t *decal;
    float offset;
    int count;
    int i;

    v = &drawVertex[*drawcount];
    decal = (rbDecal_t*)vl->data;
    count = *drawcount;

    for(i = 0; i < decal->numpoints - 2; ++i)
    {
        RB_AddTriangle(count, count + 1 + i, count + 2 + i);
    }

    offset = 0;

    if(decal->type == DCT_UPPERWALL || decal->type == DCT_CEILING)
    {
        offset = FIXED2FLOAT(decal->initialStickZ - decal->stickSector->ceilingheight);
    }
    else if(decal->type == DCT_LOWERWALL || decal->type == DCT_FLOOR)
    {
        offset = FIXED2FLOAT(decal->initialStickZ - decal->stickSector->floorheight);
    }

    for(i = 0; i < decal->numpoints; ++i)
    {
        v[i].x = decal->points[i].x;
        v[i].y = decal->points[i].y;
        v[i].z = decal->points[i].z - offset;

        v[i].tu = decal->points[i].tu;
        v[i].tv = decal->points[i].tv;

        v[i].r =
        v[i].g =
        v[i].b =
        v[i].a = (byte)(decal->alpha * 255.0f);
    }

    *drawcount += decal->numpoints;
    return true;
}

//
// RB_AddDecalDrawlist
//

static void RB_AddDecalDrawlist(rbDecal_t *decal)
{
    vtxlist_t *list;
    
    list = DL_AddVertexList(&drawlist[DLT_DECAL]);
    list->data = (rbDecal_t*)decal;
    list->procfunc = RB_GenerateDecal;
    list->preprocess = 0;
    list->postprocess = 0;
    list->texid = decal->lump;
}

//
// RB_AddDecals
//

void RB_AddDecals(subsector_t *sub)
{
    rbDecal_t *decal;

    for(decal = sub->sector->decallist; decal; decal = decal->snext)
    {
        if(decal->ssect != sub)
        {
            continue;
        }

        RB_AddDecalDrawlist(decal);
    }
}

