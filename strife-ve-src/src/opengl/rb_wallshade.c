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
//    Wall light-shading effects
//

#include "rb_main.h"
#include "rb_config.h"
#include "rb_wallshade.h"
#include "rb_geom.h"
#include "r_state.h"
#include "deh_str.h"
#include "m_parser.h"
#include "z_zone.h"

static rbShadeDef_t *shadedefs;
static int numshadedefs;

//
// dfcmp
//
// Because RB_GetHSV and RB_GetRGB are fussy as hell....
//

static boolean dfcmp(float f1, float f2)
{
    float precision = 0.00001f;
    if(((f1 - precision) < f2) && ((f1 + precision) > f2))
    {
        return true;
    }

    return false;
}

//
// RB_SumRGB
//

static float RB_SumRGB(float c1, float c2, float min, float delta, float f)
{
    float dc1   = (delta - c1);
    float dc2   = (delta - c2);
    float dmin  = (delta - min);

    return (((dc2 / dmin) + f) - (dc1 / dmin));
}

//
// RB_GetHSV
// Set HSV values based on given RGB
//

static void RB_GetHSV(int r, int g, int b, int *h, int *s, int *v)
{
    int min = r;
    int max = r;
    float delta = 0.0f;
    float j = 0.0f;
    float x = 0.0f;
    float xr = 0.0f;
    float xg = 0.0f;
    float xb = 0.0f;
    float sum = 0.0f;
    float nmax;
    float nmin;

    if(g < min) min = g;
    if(b < min) min = b;
    if(g > max) max = g;
    if(b > max) max = b;

    nmax = (float)max / 255.0f;
    nmin = (float)min / 255.0f;

    delta = nmax;

    if(dfcmp(delta, 0.0f))
    {
        delta = 0;
    }
    else
    {
        j = ((delta - nmin) / delta);
    }

    xr = ((float)r / 255.0f);
    xg = ((float)g / 255.0f);
    xb = ((float)b / 255.0f);

    if(!dfcmp(j, 0.0f))
    {
        if(!dfcmp(xr, delta))
        {
            if(!dfcmp(xg, delta))
            {
                if(dfcmp(xb, delta))
                {
                    sum = RB_SumRGB(xr, xg, nmin, delta, 4.0f);
                }
            }
            else
            {
                sum = RB_SumRGB(xb, xr, nmin, delta, 2.0f);
            }
        }
        else
        {
            sum = RB_SumRGB(xg, xb, nmin, delta, 0);
        }

        x = (sum * 60.0f);

        if(x < 0)
        {
            x += 360.0f;
        }
    }
    else
    {
        j = 0.0f;
    }

    *h = (int)((x / 360.0f) * 255.0f);
    *s = (int)(j * 255.0f);
    *v = (int)(delta * 255.0f);
}

//
// RB_GetRGB
// Set RGB values based on given HSV
//

static void RB_GetRGB(int h, int s, int v, int *r, int *g, int *b)
{
    float x = 0.0f;
    float j = 0.0f;
    float i = 0.0f;
    int table = 0;
    float xr = 0.0f;
    float xg = 0.0f;
    float xb = 0.0f;

    j = (h / 255.0f) * 360.0f;

    if(360.0f <= j)
    {
        j -= 360.0f;
    }

    x = (s / 255.0f);
    i = (v / 255.0f);

    if(!dfcmp(x, 0.0f))
    {
        table = (int)(j / 60.0f);
        if(table < 6)
        {
            float t = (j / 60.0f);
            switch(table)
            {
            case 0:
                xr = i;
                xg = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                xb = ((1.0f - x) * i);
                break;
            case 1:
                xr = ((1.0f - (x * (t - (float)table))) * i);
                xg = i;
                xb = ((1.0f - x) * i);
                break;
            case 2:
                xr = ((1.0f - x) * i);
                xg = i;
                xb = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                break;
            case 3:
                xr = ((1.0f - x) * i);
                xg = ((1.0f - (x * (t - (float)table))) * i);
                xb = i;
                break;
            case 4:
                xr = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                xg = ((1.0f - x) * i);
                xb = i;
                break;
            case 5:
                xr = i;
                xg = ((1.0f - x) * i);
                xb = ((1.0f - (x * (t - (float)table))) * i);
                break;
            }
        }
    }
    else
    {
        xr = xg = xb = i;
    }

    *r = (int)(xr * 255.0f);
    *g = (int)(xg * 255.0f);
    *b = (int)(xb * 255.0f);
}

//
// RB_InitWallShades
//

void RB_InitWallShades(void)
{
    char *flatname;
    lexer_t *lexer;
    rbShadeDef_t *shade;
    int i = 0;

    numshadedefs = 0;
    if(!(lexer = M_ParserOpen("SHADEDEF")))
    {
        return;
    }

    // count the number of wallshade defs by scanning for each wallshade block
    while(M_ParserCheckState(lexer))
    {
        M_ParserFind(lexer);
        if(lexer->tokentype == TK_IDENIFIER && M_ParserMatches(lexer, "wallshade"))
        {
            numshadedefs++;
        }
    }

    if(numshadedefs)
    {
        const int size = sizeof(rbShadeDef_t) * numshadedefs;

        shadedefs = (rbShadeDef_t*)Z_Calloc(1, size, PU_STATIC, 0);
        M_ParserReset(lexer);

        shade = shadedefs;

        while(M_ParserCheckState(lexer))
        {
            M_ParserFind(lexer);

            if(M_ParserMatches(lexer, "wallshade"))
            {
                // check for left bracket and then jump to next token
                M_ParserExpectNextToken(lexer, TK_LBRACK);
                M_ParserFind(lexer);

                // set default values
                shade->r = shade->g = shade->b = 255;
                shade->flags = RBSF_THINGS;

                // keep searching until end of block
                while(lexer->tokentype != TK_RBRACK)
                {
                    if(M_ParserMatches(lexer, "flatname"))
                    {
                        M_ParserGetString(lexer);
                        strncpy(shade->flatname, M_ParserStringToken(lexer), 8);
                        shade->flatname[8] = 0;
                    }
                    else if(M_ParserMatches(lexer, "color"))
                    {
                        M_ParserGetString(lexer);
                        sscanf(M_ParserStringToken(lexer), "%hhi %hhi %hhi",
                            &shade->r, &shade->g, &shade->b);
                    }
                    else if(lexer->tokentype == TK_PLUS)
                    {
                        // parse flags
                        M_ParserFind(lexer);
                        if(M_ParserMatches(lexer, "things"))
                        {
                            shade->flags |= RBSF_THINGS;
                        }
                        else if(M_ParserMatches(lexer, "bright"))
                        {
                            shade->flags |= RBSF_BRIGHT;
                        }
                        else if(M_ParserMatches(lexer, "contrast"))
                        {
                            shade->flags |= RBSF_CONTRAST;
                        }
                        else if(M_ParserMatches(lexer, "floor"))
                        {
                            shade->flags |= RBSF_FLOOR;
                        }
                        else if(M_ParserMatches(lexer, "ceiling"))
                        {
                            shade->flags |= RBSF_CEILING;
                        }
                        else if(M_ParserMatches(lexer, "lowclamp"))
                        {
                            shade->flags |= RBSF_LOWCLAMP;
                        }
                        else if(M_ParserMatches(lexer, "sky"))
                        {
                            shade->flags |= RBSF_SKY;
                        }
                        else if(M_ParserMatches(lexer, "seglighting"))
                        {
                            shade->flags |= RBSF_SEGLIGHTING;
                        }
                        else if(M_ParserMatches(lexer, "hot"))
                        {
                            shade->flags |= RBSF_HOT;
                        }
                    }

                    // get next token
                    M_ParserFind(lexer);
                }

                shade++;
            }
        }

        shade = shadedefs;

        for(i = 0; i < numshadedefs; ++i)
        {
            flatname = shade->flatname;
            shade->flatnum = R_FlatNumForName(flatname);
            shade->self  = i;
            shade->first = -1;
            shade->next  = -1;

            RB_GetHSV(shade->r, shade->g, shade->b,
                      &shade->h, &shade->s, &shade->v);

            ++shade;
        }

        // initialize hash table
        shade = shadedefs;
        for(i = 0; i < numshadedefs; ++i)
        {
            unsigned int hc = (unsigned int)shade->flatnum % numshadedefs;
            shade->next = shadedefs[hc].first;
            shadedefs[hc].first = shade->self;
            ++shade;
        }
    }

    M_ParserClose();
}

//
// RB_FindShadeDef
//

static rbShadeDef_t *RB_FindShadeDef(int pic)
{
    unsigned int hc;
    int i;

    if(numshadedefs <= 0)
    {
        return NULL;
    }
    
    hc = (unsigned int)pic % numshadedefs;
    i = shadedefs[hc].first;

    while(i >= 0 && shadedefs[i].flatnum != pic)
    {
        i = shadedefs[i].next;
    }

    return i >= 0 ? &shadedefs[i] : NULL;
}

//
// RB_SetSkyShade
//

void RB_SetSkyShade(byte r, byte g, byte b)
{
    rbShadeDef_t *rsd;

    if(rbLightmaps)
    {
        r = 255;
        g = 255;
        b = 255;
    }

    if((rsd = RB_FindShadeDef(R_FlatNumForName("F_SKY001"))))
    {
        rsd->r = r;
        rsd->g = g;
        rsd->b = b;

        RB_GetHSV(r, g, b, &rsd->h, &rsd->s, &rsd->v);
    }
}


//
// RB_SplitWallShade
//
// Divides the color so it can distribute the gradiant evenly across all
// sidedefs of that line
//

static void RB_SplitWallShade(vtx_t *vtx, seg_t *line, rbWallSide_t side,
                              byte r1, byte g1, byte b1,
                              byte r2, byte g2, byte b2)
{
    int height = 0;
    int sideheight1 = 0;
    int sideheight2 = 0;
    float fr1, fg1, fb1;
    float fr2, fg2, fb2;

    height = (line->frontsector->ceilingheight - line->frontsector->floorheight)>>FRACBITS;

    if(side == WS_UPPER)
    {
        sideheight1 = (line->backsector->ceilingheight - line->frontsector->floorheight)>>FRACBITS;
        sideheight2 = (line->frontsector->ceilingheight - line->backsector->ceilingheight)>>FRACBITS;
    }
    else if(side == WS_LOWER)
    {
        sideheight1 = (line->backsector->floorheight - line->frontsector->floorheight)>>FRACBITS;
        sideheight2 = (line->frontsector->ceilingheight - line->backsector->floorheight)>>FRACBITS;
    }

    fr1 = (((float)r1 / height) * sideheight1);
    fg1 = (((float)g1 / height) * sideheight1);
    fb1 = (((float)b1 / height) * sideheight1);

    fr2 = (((float)r2 / height) * sideheight2);
    fg2 = (((float)g2 / height) * sideheight2);
    fb2 = (((float)b2 / height) * sideheight2);

    vtx->r = (byte)MIN((fr1+fr2), 0xff);
    vtx->g = (byte)MIN((fg1+fg2), 0xff);
    vtx->b = (byte)MIN((fb1+fb2), 0xff);
}

//
// RB_SetWallShadeColor
//

static void RB_SetWallShadeColor(seg_t *line, vtx_t* vtx, rbWallSide_t side,
                                 byte r1, byte g1, byte b1,
                                 byte r2, byte g2, byte b2,
                                 boolean lowClamp)
{
    // set the initial color
    vtx[0].r = vtx[1].r = r1;
    vtx[0].g = vtx[1].g = g1;
    vtx[0].b = vtx[1].b = b1;
    vtx[2].r = vtx[3].r = r2;
    vtx[2].g = vtx[3].g = g2;
    vtx[2].b = vtx[3].b = b2;

    if(line->backsector && side != WS_MIDDLE)
    {
        if(side == WS_UPPER)
        {
            if(line->frontsector->floorheight <= line->backsector->ceilingheight)
            {
                // split up the color for upper sidedef
                RB_SplitWallShade(&vtx[2], line, WS_UPPER, r1, g1, b1, r2, g2, b2);
                vtx[3].r = vtx[2].r;
                vtx[3].g = vtx[2].g;
                vtx[3].b = vtx[2].b;
            }
        }

        if(side == WS_LOWER && !lowClamp)
        {
            if(line->frontsector->ceilingheight >= line->backsector->floorheight)
            {
                // split up the color for the lower sidedef
                RB_SplitWallShade(&vtx[0], line, WS_LOWER, r1, g1, b1, r2, g2, b2);
                vtx[1].r = vtx[0].r;
                vtx[1].g = vtx[0].g;
                vtx[1].b = vtx[0].b;
            }
        }

        // handle special case with backsectors
        if(side == WS_MIDDLEBACK)
        {
            if(line->backsector->ceilingheight < line->frontsector->ceilingheight)
            {
                // split up the color for upper sidedef
                RB_SplitWallShade(&vtx[0], line, WS_UPPER, r1, g1, b1, r2, g2, b2);
                vtx[1].r = vtx[0].r;
                vtx[1].g = vtx[0].g;
                vtx[1].b = vtx[0].b;
            }
            else
            {
                vtx[0].r = vtx[1].r = r1;
                vtx[0].g = vtx[1].g = g1;
                vtx[0].b = vtx[1].b = b1;
            }

            if(line->backsector->floorheight > line->frontsector->floorheight)
            {
                // split up the color for the lower sidedef
                RB_SplitWallShade(&vtx[2], line, WS_LOWER, r1, g1, b1, r2, g2, b2);
                vtx[3].r = vtx[2].r;
                vtx[3].g = vtx[2].g;
                vtx[3].b = vtx[2].b;
            }
            else
            {
                vtx[2].r = vtx[3].r = r2;
                vtx[2].g = vtx[3].g = g2;
                vtx[2].b = vtx[3].b = b2;
            }
        }
    }
}

//
// RB_SetupWallShadeColor
//

static void RB_SetupWallShadeColor(seg_t *seg, rbShadeDef_t *shadedef, int *r, int *g, int *b, int *l)
{
    // make the source of the glow more dominant
    if(shadedef->flags & RBSF_HOT)
    {
        *r = shadedef->r;
        *g = shadedef->g;
        *b = shadedef->b;
        *l = ((shadedef->flags & RBSF_CONTRAST) ? (*l >> 2) : *l);
    }
    else
    {
        // need to adjust value for sector's brightness level
        int seglight, v;
        if(shadedef->flags & RBSF_SEGLIGHTING)
            seglight = *l; // preserve fake contrast already set on the seg
        else
            seglight = seg->frontsector->lightlevel; // go back to sector lighting

        v = MIN((int)((float)shadedef->v * ((float)(seglight << 2) / 1024)), 255);
        RB_GetRGB(shadedef->h, shadedef->s, v, r, g, b);
    }
}

//
// RB_SetSectorShades
//
// haleyjd: Set a sector's floor and ceiling shades if they've not been set
// already, or do not correspond to the sector's current flats.
//
void RB_SetSectorShades(sector_t *sec)
{
    if(!sec->floorshade || sec->floorshade->flatnum != sec->floorpic)
    {
        sec->floorshade = RB_FindShadeDef(sec->floorpic);
    }

    if(!sec->ceilingshade || sec->ceilingshade->flatnum != sec->ceilingpic)
    {
        sec->ceilingshade = RB_FindShadeDef(sec->ceilingpic);
    }

    if(sec->ceilingshade && sec->ceilingshade->flags & RBSF_SKY && !sec->floorshade)
    {
        sec->floorshade = sec->ceilingshade;
    }
}

//
// RB_SetWallShade
//

boolean RB_SetWallShade(vtx_t *vtx, int lightlevel, seg_t *seg, rbWallSide_t side)
{
    rbShadeDef_t *floorShadeDef;
    rbShadeDef_t *ceilingShadeDef;
    int r1, g1, b1, r2, g2, b2;
    int l1, l2;

    // try to not constantly look for shadedefs if this is the same sector
    // we've processed before
    floorShadeDef   = seg->frontsector->floorshade;
    ceilingShadeDef = seg->frontsector->ceilingshade;
    
    if(floorShadeDef == NULL && ceilingShadeDef == NULL)
    {
        return false;
    }

    if(((ceilingShadeDef && ceilingShadeDef->flags & RBSF_SKY) ||
        (floorShadeDef && floorShadeDef->flags & RBSF_SKY)) &&
        (rbLightmaps && seg->frontsector->altlightlevel != -1))
    {
        return false;
    }

    l1 = l2 = lightlevel;

    if(ceilingShadeDef && floorShadeDef)
    {
        RB_SetupWallShadeColor(seg, ceilingShadeDef, &r1, &g1, &b1, &l1);
        RB_SetupWallShadeColor(seg, floorShadeDef,   &r2, &g2, &b2, &l2);
        RB_SetWallShadeColor(seg, vtx, side, r1, g1, b1, r2, g2, b2,
                             (floorShadeDef->flags & RBSF_LOWCLAMP) |
                             (ceilingShadeDef->flags & RBSF_LOWCLAMP));
    }
    else if(ceilingShadeDef)
    {
        RB_SetupWallShadeColor(seg, ceilingShadeDef, &r1, &g1, &b1, &l1);
        RB_SetWallShadeColor(seg, vtx, side, r1, g1, b1, l1, l1, l1,
                             ceilingShadeDef->flags & RBSF_LOWCLAMP);
    }
    else if(floorShadeDef)
    {
        RB_SetupWallShadeColor(seg, floorShadeDef, &r1, &g1, &b1, &l1);
        RB_SetWallShadeColor(seg, vtx, side, l1, l1, l1, r1, g1, b1,
                             floorShadeDef->flags & RBSF_LOWCLAMP);
    }

    return true;
}

//
// RB_GetFloorShade
//

boolean RB_GetFloorShade(sector_t *sector, byte *r, byte *g, byte *b)
{
    int v;
    int tr, tg, tb;
    rbShadeDef_t *shadedef = sector->floorshade;

    if(!shadedef || !(shadedef->flags & RBSF_FLOOR))
    {
        return false;
    }
    
    if(shadedef->flags & RBSF_SKY && rbLightmaps && sector->altlightlevel != -1)
    {
        return false;
    }
    
    v = MIN((int)((float)shadedef->v * ((float)(sector->lightlevel << 2) / 1024)), 255);
    RB_GetRGB(shadedef->h, shadedef->s, v, &tr, &tg, &tb);

    *r = (byte)tr;
    *g = (byte)tg;
    *b = (byte)tb;

    return true;
}

//
// RB_GetCeilingShade
//

boolean RB_GetCeilingShade(sector_t *sector, byte *r, byte *g, byte *b)
{
    int v;
    int tr, tg, tb;
    rbShadeDef_t *shadedef = sector->ceilingshade;

    if(!shadedef || !(shadedef->flags & RBSF_CEILING))
    {
        return false;
    }
    
    v = MIN((int)((float)shadedef->v * ((float)(sector->lightlevel << 2) / 1024)), 255);
    RB_GetRGB(shadedef->h, shadedef->s, v, &tr, &tg, &tb);

    *r = (byte)tr;
    *g = (byte)tg;
    *b = (byte)tb;

    return true;
}

//
// RB_SetupThingShadeColor
//

static void RB_SetupThingShadeColor(sector_t *sec, rbShadeDef_t *shadedef, 
                                    int *r, int *g, int *b, int *l)
{
    // make the source of the glow more dominant
    if(shadedef->flags & (RBSF_SKY|RBSF_HOT))
    {
        *r = shadedef->r;
        *g = shadedef->g;
        *b = shadedef->b;
        // NB: no l set here for sprites, looks weird.
    }
    else
    {
        // need to adjust value for sector's brightness level
        int v = MIN((int)((float)shadedef->v * ((float)(sec->lightlevel << 2) / 1024)), 255);

        RB_GetRGB(shadedef->h, shadedef->s, v, r, g, b);
    }
}

//
// RB_SplitThingShade
//

static void RB_InterpThingShade(int tr, int tg, int tb,
                                int br, int bg, int bb,
                                float sectop, float secbottom,
                                float thingz, vtx_t *vtx, 
                                boolean top)
{
    float topdist = sectop - thingz;
    float botdist = thingz - secbottom;
    float height  = sectop - secbottom;

    if(top)
    {
        float t = topdist;
        topdist = botdist;
        botdist = t;
    }

    {
        float fr1 = (((float)tr / height) * topdist);
        float fg1 = (((float)tg / height) * topdist);
        float fb1 = (((float)tb / height) * topdist);

        float fr2 = (((float)br / height) * botdist);
        float fg2 = (((float)bg / height) * botdist);
        float fb2 = (((float)bb / height) * botdist);

        vtx->r = (byte)MIN((fr1+fr2), 0xff);
        vtx->g = (byte)MIN((fg1+fg2), 0xff);
        vtx->b = (byte)MIN((fb1+fb2), 0xff);
    }
}


//
// RB_SetThingShade
//

void RB_SetThingShade(mobj_t *thing, vtx_t *vtx, int lightlevel)
{
    int tr, tg, tb, tl;
    int br, bg, bb, bl;
    sector_t *sec = thing->subsector->sector;
    rbShadeDef_t *cs, *fs;

    cs = sec->ceilingshade;
    fs = sec->floorshade;
    
    if(!(cs || fs))
        return;

    // top color
    tr = vtx[0].r;
    tg = vtx[0].g;
    tb = vtx[0].b;
    tl = lightlevel;

    if(cs && cs->flags & RBSF_THINGS)
        RB_SetupThingShadeColor(sec, cs, &tr, &tg, &tb, &tl);

    // bottom color
    br = vtx[2].r;
    bg = vtx[2].g;
    bb = vtx[2].b;
    bl = lightlevel;

    if(fs && fs->flags & RBSF_THINGS)
        RB_SetupThingShadeColor(sec, fs, &br, &bg, &bb, &bl);

    {
        float   sectop      = FIXED2FLOAT(sec->ceilingheight);
        float   secbottom   = FIXED2FLOAT(sec->floorheight);
        float   thingtop    = vtx[0].z;
        float   thingbottom = vtx[2].z;

        if(thingtop > sectop)
        {
            thingtop = sectop;
        }
        if(thingbottom < secbottom)
        {
            thingbottom = secbottom;
        }

        if(cs && fs)
        {
            RB_InterpThingShade(tr, tg, tb, br, bg, bb, sectop, secbottom, thingtop,    &vtx[0], true);
            RB_InterpThingShade(br, bg, bb, tr, tg, tb, sectop, secbottom, thingbottom, &vtx[2], false);
        }
        else if(cs)
        {
            RB_InterpThingShade(tr, tg, tb, tl, tl, tl, sectop, secbottom, thingtop,    &vtx[0], true);
            RB_InterpThingShade(tr, tg, tb, tl, tl, tl, sectop, secbottom, thingbottom, &vtx[2], true);
        }
        else if(fs)
        {
            RB_InterpThingShade(br, bg, bb, bl, bl, bl, sectop, secbottom, thingtop,    &vtx[0], false);
            RB_InterpThingShade(br, bg, bb, bl, bl, bl, sectop, secbottom, thingbottom, &vtx[2], false);
        }
        vtx[1].r = vtx[0].r;
        vtx[1].g = vtx[0].g;
        vtx[1].b = vtx[0].b;
        vtx[3].r = vtx[2].r;
        vtx[3].g = vtx[2].g;
        vtx[3].b = vtx[2].b;
    }
}

//
// RB_SetPspriteShade
//

void RB_SetPspriteShade(player_t *player, vtx_t *vtx, int lightlevel)
{
    int tr, tg, tb, tl;
    int br, bg, bb, bl;
    sector_t *sec = player->mo->subsector->sector;
    rbShadeDef_t *cs, *fs;

    cs = sec->ceilingshade;
    fs = sec->floorshade;
    
    if(!(cs || fs))
        return;

    // top color
    tr = vtx[0].r;
    tg = vtx[0].g;
    tb = vtx[0].b;
    tl = lightlevel;

    if(cs && cs->flags & RBSF_THINGS)
        RB_SetupThingShadeColor(sec, cs, &tr, &tg, &tb, &tl);

    // bottom color
    br = vtx[2].r;
    bg = vtx[2].g;
    bb = vtx[2].b;
    bl = lightlevel;

    if(fs && fs->flags & RBSF_THINGS)
        RB_SetupThingShadeColor(sec, fs, &br, &bg, &bb, &bl);

    {
        float   sectop      = FIXED2FLOAT(sec->ceilingheight);
        float   secbottom   = FIXED2FLOAT(sec->floorheight);
        float   thingtop    = FIXED2FLOAT(player->mo->z + 32*FRACUNIT);
        float   thingbottom = FIXED2FLOAT(player->mo->z + 16*FRACUNIT);

        if(thingtop > sectop)
            thingtop = sectop;
        if(thingbottom < secbottom)
            thingbottom = secbottom;

        if(cs && fs)
        {
            RB_InterpThingShade(tr, tg, tb, br, bg, bb, sectop, secbottom, thingtop,    &vtx[0], true);
            RB_InterpThingShade(br, bg, bb, tr, tg, tb, sectop, secbottom, thingbottom, &vtx[2], false);
        }
        else if(cs)
        {
            RB_InterpThingShade(tr, tg, tb, tl, tl, tl, sectop, secbottom, thingtop,    &vtx[0], true);
            RB_InterpThingShade(tr, tg, tb, tl, tl, tl, sectop, secbottom, thingbottom, &vtx[2], true);
        }
        else if(fs)
        {
            RB_InterpThingShade(br, bg, bb, bl, bl, bl, sectop, secbottom, thingtop,    &vtx[0], false);
            RB_InterpThingShade(br, bg, bb, bl, bl, bl, sectop, secbottom, thingbottom, &vtx[2], false);
        }
        vtx[1].r = vtx[0].r;
        vtx[1].g = vtx[0].g;
        vtx[1].b = vtx[0].b;
        vtx[3].r = vtx[2].r;
        vtx[3].g = vtx[2].g;
        vtx[3].b = vtx[2].b;
    }
}

// EOF

