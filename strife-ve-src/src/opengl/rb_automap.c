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
//    Automap Renderer for OpenGL
//

#include <math.h>

#include "doomstat.h"
#include "rb_draw.h"
#include "rb_drawlist.h"
#include "rb_geom.h"
#include "rb_view.h"
#include "rb_data.h"
#include "rb_texture.h"
#include "rb_automap.h"
#include "deh_str.h"
#include "i_video.h"
#include "m_bbox.h"
#include "w_wad.h"
#include "r_defs.h"
#include "r_state.h"

static rbView_t rbAutomapView;

extern fixed_t m_x2, m_y2, m_w, m_h;
extern fixed_t scale_ftom;
extern int f_w, f_h;

static rbTexture_t *markTextures[10];
static rbTexture_t *objMarkIcon;

extern SDL_Window *windowscreen;
//
// RB_SetupAutomapView
//

static void RB_SetupAutomapView(rbView_t *view)
{
	float amx, amy;
	float px, py;
	float fscale;

#if 0
    SDL_Surface *screen = SDL_GetWindowSurface(windowscreen);
	float delta = (float)screen->h / ((float)SCREENHEIGHT / 16.0f);
	dglViewport(0, delta, screen->w, screen->h);
#else
	int w;
	int h;
	SDL_GetWindowSize(windowscreen, &w, &h);

	float delta = (float)h / ((float)SCREENHEIGHT / 16.0f);
	dglViewport(0, delta, w, h);
#endif
    
    fscale = FIXED2FLOAT(scale_ftom);

    px = FIXED2FLOAT(players[consoleplayer].mo->x);
    py = FIXED2FLOAT(players[consoleplayer].mo->y);

    amx = px - (FIXED2FLOAT(m_x2) - FIXED2FLOAT(m_w) * 0.5f);
    amy = py - (FIXED2FLOAT(m_y2) - FIXED2FLOAT(m_h) * 0.5f);

    //
    // m_x2 and m_y2 is sloppy as hell. using this instead of the player's coordinates
    // results in very jittery movement but we need it for panning. in this case we
    // only check if the values are larger than scale_ftom, which afterwards,
    // apply the panning offsets to px and py
    //
    if(fabs(amx) > fscale + 2)
    {
        px -= amx;
    }

    if(fabs(amy) > fscale + 2)
    {
        py -= amy;
    }

    view->x = px;
    view->y = py;
    view->z = fscale * (float)SCREENHEIGHT;

    view->viewyaw = DEG2RAD(90);
    view->viewpitch = DEG2RAD(-90);

    RB_SetupFrameForView(view, 45.0f);
}

//
// RB_DrawAutomapLine
//

void RB_DrawAutomapLine(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int color)
{
    byte rgb[3];
    fixed_t bbox[4];
    float fx1, fy1;
    float fx2, fy2;
    
    M_ClearBox(bbox);
    M_AddToBox(bbox, x1, y1);
    M_AddToBox(bbox, x2, y2);
    M_AddToBox(bbox, x2, y1);
    M_AddToBox(bbox, x1, y2);
    
    if(!RB_CheckBoxInView(&rbAutomapView, bbox, -1, 1))
    {
        return;
    }

    fx1 = FIXED2FLOAT(x1);
    fy1 = FIXED2FLOAT(y1);
    fx2 = FIXED2FLOAT(x2);
    fy2 = FIXED2FLOAT(y2);

    RB_BindTexture(&whiteTexture);
    I_GetPaletteColor(rgb, color);

    //
    // I despise opengl immediate mode but I just can't be arsed
    // to do anything fancy here
    //
    dglBegin(GL_LINES);
    dglColor4ub(rgb[0], rgb[1], rgb[2], 0xff);
    dglVertex3f(fx1, fy1, 0);
    dglVertex3f(fx2, fy2, 0);
    dglEnd();
}

//
// RB_DrawObjectiveMarker
//

void RB_DrawObjectiveMarker(fixed_t x, fixed_t y)
{
    vtx_t v[4];
    int i;
    float fx, fy;
    float w, h;
    float t;
    float scale;
    float fscale;
    int alpha;

    // if not loaded, then do it now
    if(!objMarkIcon)
    {
        objMarkIcon = RB_GetTexture(RDT_PATCH, W_GetNumForName(DEH_String("MARKER")), 0);
    }

    t = cosf((float)leveltime * 0.1f);
    alpha = (int)(fabsf(t) * 255.0f);

    for(i = 0; i < 4; ++i)
    {
        v[i].r = v[i].g = v[i].b = 0xff;
        v[i].a = alpha;
        v[i].z = 0;
    }

    fx = FIXED2FLOAT(x);
    fy = FIXED2FLOAT(y);

    fscale = FIXED2FLOAT(scale_ftom) * 0.125f;

    if(fscale < 1)
    {
        fscale = 1;
    }

    scale = (2.0f + (((float)alpha / 255.0f) * 2.0f)) * fscale;

    w = ((float)objMarkIcon->origwidth * 0.5f) * scale;
    h = ((float)objMarkIcon->origheight * 0.5f) * scale;

    v[0].x = v[2].x = fx + w;
    v[0].y = v[1].y = fy - h;
    v[1].x = v[3].x = fx - w;
    v[2].y = v[3].y = fy + h;

    v[0].tu = v[2].tu = 1;
    v[0].tv = v[1].tv = 1;
    v[1].tu = v[3].tu = 0;
    v[2].tv = v[3].tv = 0;

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);

    RB_BindTexture(objMarkIcon);

    // render
    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);
    RB_DrawElements();
    RB_ResetElements();
}

//
// RB_DrawMark
//

void RB_DrawMark(fixed_t x, fixed_t y, int marknum)
{
    vtx_t v[4];
    int i;
    float fx, fy;
    float w, h;
    float fscale;
    rbTexture_t *texture;

    if(marknum < 0 || marknum >= 10)
    {
        return;
    }

    for(i = 0; i < 4; ++i)
    {
        v[i].r = v[i].g = v[i].b = v[i].a = 0xff;
        v[i].z = 0;
    }

    fx = FIXED2FLOAT(x);
    fy = FIXED2FLOAT(y);

    fscale = FIXED2FLOAT(scale_ftom) * 0.75f;

    if(fscale < 1)
    {
        fscale = 1;
    }

    // if not loaded, then do it now
    if(!markTextures[marknum])
    {
        char namebuf[9];

        DEH_snprintf(namebuf, 9, DEH_String("PLMNUM%d"), marknum);
        markTextures[marknum] = RB_GetTexture(RDT_PATCH, W_GetNumForName(namebuf), 0);
    }

    texture = markTextures[marknum];

    w = ((float)texture->origwidth * 0.5f) * fscale;
    h = ((float)texture->origheight * 0.5f) * fscale;

    v[0].x = v[2].x = fx + w;
    v[0].y = v[1].y = fy - h;
    v[1].x = v[3].x = fx - w;
    v[2].y = v[3].y = fy + h;

    v[0].tu = v[2].tu = 1;
    v[0].tv = v[1].tv = 1;
    v[1].tu = v[3].tu = 0;
    v[2].tv = v[3].tv = 0;

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);
    RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);

    RB_BindTexture(texture);

    // render
    RB_BindDrawPointers(v);
    RB_AddTriangle(0, 1, 2);
    RB_AddTriangle(3, 2, 1);
    RB_DrawElements();
    RB_ResetElements();
}

//
// RB_GenerateAutomapSectors
//

boolean RB_GenerateAutomapSectors(vtxlist_t *vl, int *drawcount)
{
    int j;
    int count;
    fixed_t tx;
    fixed_t ty;
    leaf_t *leaf;
    subsector_t *ss;
    sector_t *sector;
    
    ss      = (subsector_t*)vl->data;
    leaf    = &leafs[ss->leaf];
    sector  = ss->sector;
    count   = *drawcount;
    
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
        }
        else
        {
            leaf = &leafs[ss->leaf + j];
        }
        
        v->x = leaf->vertex->fx;
        v->y = leaf->vertex->fy;
        v->z = 0;
        
        v->tu = FIXED2FLOAT((leaf->vertex->x >> 6) - tx);
        v->tv = -FIXED2FLOAT((leaf->vertex->y >> 6) - ty);
        
        v->r = v->g = v->b = rbSectorLightTable[sector->lightlevel];
        v->a = 0xff;
        
        count++;
    }
    
    *drawcount = count;
    return true;
}

//
// RB_AddLeafToDrawlist
//

static void RB_AddLeafToDrawlist(subsector_t *sub, int texid)
{
    vtxlist_t *list;
    
    list = DL_AddVertexList(&drawlist[DLT_AMAP]);
    list->data = (subsector_t*)sub;
    list->procfunc = RB_GenerateAutomapSectors;
    list->preprocess = NULL;
    list->postprocess = NULL;
    list->params = sub->sector->lightlevel;
    list->texid = texid;
    list->flags = 0;
}

//
// RB_DrawAutomapSectors
//

void RB_DrawAutomapSectors(boolean cheating)
{
    int i;
    int j;
    vtx_t *v = &drawVertex[0];
    subsector_t *sub;

    for(i = 0; i < numsubsectors; ++i)
    {
        sub = &subsectors[i];

        if(sub->sector->floorpic == skyflatnum)
        {
            // don't add sky flats
            continue;
        }

        for(j = 0; j < sub->numlines; ++j)
        {
            line_t *line;

            line = segs[sub->firstline + j].linedef;

            if(line == NULL)
            {
                // skip minisegs
                continue;
            }

            if(line->flags & ML_MAPPED)
            {
                break;
            }
        }

        if(j >= sub->numlines && !cheating && !players[consoleplayer].powers[pw_allmap])
        {
            // must be mapped
            continue;
        }

        for(j = 0; j < sub->numleafs; ++j)
        {
            vertex_t *vertex = leafs[sub->leaf + j].vertex;

            v[j].x = vertex->fx;
            v[j].y = vertex->fy;
            v[j].z = 0;
        }

        if(!RB_CheckPointsInView(&rbAutomapView, v, sub->numleafs))
        {
            continue;
        }

        RB_AddLeafToDrawlist(sub, sub->sector->floorpic);
    }

    RB_SetDepthMask(0);

    DL_ProcessDrawList(DLT_AMAP);
    DL_Reset(DLT_AMAP);

    RB_SetDepthMask(1);
}

//
// RB_BeginAutomapDraw
//

void RB_BeginAutomapDraw(void)
{
    dglClearColor(0, 0, 0, 1);

    RB_ClearBuffer(GLCB_ALL);
    RB_SetupAutomapView(&rbAutomapView);

    // load projection
    dglMatrixMode(GL_PROJECTION);
    dglLoadMatrixf(rbAutomapView.projection);

    // load modelview
    dglMatrixMode(GL_MODELVIEW);
    dglLoadMatrixf(rbAutomapView.modelview);

    DL_BeginDrawList();
}

//
// RB_EndAutomapDraw
//

void RB_EndAutomapDraw(void)
{
     RB_ResetViewPort();
     RB_DrawExtraHudPics();
}
