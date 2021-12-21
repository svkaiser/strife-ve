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
//    OpenGL PlayerView Rendering
//

#include <math.h>

#include "rb_main.h"
#include "rb_view.h"
#include "rb_config.h"
#include "rb_clipper.h"
#include "rb_bsp.h"
#include "rb_drawlist.h"
#include "rb_things.h"
#include "rb_draw.h"
#include "rb_dynlights.h"
#include "p_local.h"
#include "i_video.h"
#include "r_main.h"
#include "r_state.h"
#include "m_bbox.h"
#include "doomstat.h"

rbView_t rbPlayerView;

extern SDL_Window *windowscreen;

#define Z_NEAR          0.1f
#define FIXED_ASPECT    1.2f

//
// RB_SetupMatrices
//

static void RB_SetupMatrices(rbView_t *view, const float fov)
{
    matrix transform;

    // setup projection matrix
    int w;
	int h;
	SDL_GetWindowSize(windowscreen, &w, &h);
    //screen = SDL_GetWindowSurface(windowscreen);
	MTX_ViewFrustum(view->projection, (float)w, (float)h, fov, Z_NEAR);

    // setup rotation matrix
    // start off with the matrix on it's z-axis and then rotate it along the x-axis
    MTX_IdentityZ(view->rotation, -view->viewyaw + DEG2RAD(90));
    MTX_RotateX(view->rotation, -view->viewpitch - DEG2RAD(90));

    // scale to aspect ratio
    MTX_Scale(view->rotation, 1, 1, FIXED_ASPECT);

    // setup modelview matrix
    MTX_Identity(transform);
    MTX_SetTranslation(transform, -view->x, -view->y, -view->z);
    MTX_Multiply(view->modelview, transform, view->rotation);
}

//
// RB_SetupClipFrustum
//

static void RB_SetupClipFrustum(rbView_t *view)
{
    matrix transform;
    int i;

    MTX_Multiply(transform, view->modelview, view->projection);
    
    for(i = 0; i < 4; ++i)
    {
        view->frustum[FP_RIGHT][i]  = transform[i * 4 + 3] - transform[i * 4 + 0];
        view->frustum[FP_LEFT][i]   = transform[i * 4 + 3] + transform[i * 4 + 0];
        view->frustum[FP_BOTTOM][i] = transform[i * 4 + 3] - transform[i * 4 + 1];
        view->frustum[FP_TOP][i]    = transform[i * 4 + 3] + transform[i * 4 + 1];
        view->frustum[FP_FAR][i]    = transform[i * 4 + 3] - transform[i * 4 + 2];
        view->frustum[FP_NEAR][i]   = transform[i * 4 + 3] + transform[i * 4 + 2];
    }
}

//
// RB_SetupClipper
//
// Roughly determine the angle span by looking at the
// fov and screen dimentions
//

static void RB_SetupClipper(rbView_t *view, const float fov)
{
    const float fovAngle = DEG2RAD(fov) + 0.2f;
    angle_t clipangle;
    float span;
    float tfov;
    float tilt;
    fixed_t aspect;
    float faspect;
    float expand;
    float ratio;
    
    // get screen aspect ratio
    aspect = (screen_width * FRACUNIT) / screen_height;
    
    // anything other than center pitch will add additional span for the clip range.
    // higher the fov, the more wider span is applied. this will always be
    // clamped between 0 and 1
    tfov = MAX(1.0f - (fabsf(view->rotpitch.s) * MAX(tanf(fovAngle / 2.0f), 1)), 0);
    
    faspect = FIXED2FLOAT(aspect);
    expand = 10.0f * faspect;
    ratio = FIXED_ASPECT;

    if(aspect > 4 * FRACUNIT / 3)
    {
        ratio *= FIXED2FLOAT(FixedDiv(4*FRACUNIT, 3 * aspect));
    }
    
    // compute the span angle. this shouldn't go below the fixed aspect ratio
    span = MIN((M_PI - fovAngle) * faspect, ratio * 2.0f);
    tilt = MIN((360.0f - (RAD2DEG(tfov * span))) + expand, 360.0f);
    
    // convert to angle_t and set the clip angle
    clipangle = ANG1 * (int)(tilt);
    
    RB_Clipper_Clear();
    RB_Clipper_SafeAddClipRange(viewangle + clipangle, viewangle - clipangle);
}

//
// RB_SetupFrameForView
//

void RB_SetupFrameForView(rbView_t *view, const float fov)
{
    // build matrices
    RB_SetupMatrices(view, fov);

    // setup clip frustum
    RB_SetupClipFrustum(view);

    // setup clipper
    RB_SetupClipper(view, fov);

    validcount++;
}

//
// RB_SetupView
//

static void RB_SetupView(player_t *player, rbView_t *view, const float fov)
{
    fixed_t pitch;
    
    viewlerp = R_GetLerp();

    R_interpolateViewPoint(player, viewlerp);

    if(viewlerp != FRACUNIT)
    {
        R_SetSectorInterpolationState(SEC_INTERPOLATE);
    }

    // adjust viewport to match the resizing screen
    if(viewheight != SCREENHEIGHT)
    {
#if 1
		int w;
		int h;
		SDL_GetWindowSize(windowscreen, &w, &h);
		 
		float delta = (float)h / ((float)SCREENHEIGHT / 16.0f);
		dglViewport(0, delta, w, h);
#else
		SDL_Surface *screen = SDL_GetWindowSurface(windowscreen);
		float delta = (float)screen->h / ((float)SCREENHEIGHT / 16.0f);
		dglViewport(0, delta, screen->w, screen->h);
#endif
        
    }

    view->x = FIXED2FLOAT(viewx);
    view->y = FIXED2FLOAT(viewy);
    view->z = FIXED2FLOAT(viewz);

    view->extralight = player->extralight;

    pitch = R_LerpCoord(viewlerp, player->prevpitch, player->pitch);
    
    view->viewyaw = DEG2RAD((TRUEANGLES(viewangle)));
    view->viewpitch = FIXED2FLOAT(P_PitchToFixedSlope(pitch + player->recoilpitch));

    view->rotyaw.s = sinf(view->viewyaw);
    view->rotyaw.c = cosf(view->viewyaw);
    view->rotpitch.s = sinf(view->viewpitch);
    view->rotpitch.c = cosf(view->viewpitch);

    RB_SetupFrameForView(view, fov);
}

//
// RB_CheckPointsInView
//

boolean RB_CheckPointsInView(rbView_t *view, vtx_t *vertex, int count)
{
    int p;
    int i;

    for(p = 0; p < NUMFRUSTUMPLANES; ++p)
    {
        for(i = 0; i < count; i++)
        {
            if( view->frustum[p][0] * vertex[i].x +
                view->frustum[p][1] * vertex[i].y +
                view->frustum[p][2] * vertex[i].z +
                view->frustum[p][3] > 0)
            {
                break;
            }
        }

        if(i != count)
        {
            continue;
        }

        return false;
    }

    return true;
}

//
// RB_CheckSphereInView
//

boolean RB_CheckSphereInView(rbView_t *view, const float x, const float y, const float z, const float radius)
{
    int i;

    for(i = 0; i < NUMFRUSTUMPLANES; ++i)
    {
        if( view->frustum[i][0] * x +
            view->frustum[i][1] * y +
            view->frustum[i][2] * z +
            view->frustum[i][3] <= -radius)
        {
            return false;
        }
        
    }
    return true;
}

//
// RB_CheckBoxInView
//

boolean RB_CheckBoxInView(rbView_t *view, fixed_t *box, const fixed_t z1, const fixed_t z2)
{
    float minz = FIXED2FLOAT(z1);
    float maxz = FIXED2FLOAT(z2);
    float minx = FIXED2FLOAT(box[BOXLEFT]);
    float miny = FIXED2FLOAT(box[BOXBOTTOM]);
    float maxx = FIXED2FLOAT(box[BOXRIGHT]);
    float maxy = FIXED2FLOAT(box[BOXTOP]);
    float d;
    float *p;
    int i;

    for(i = 0; i < NUMFRUSTUMPLANES; ++i)
    {
        p = view->frustum[i];

        d = p[0] * minx + p[1] * miny + p[2] * minz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * maxx + p[1] * miny + p[2] * minz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * minx + p[1] * maxy + p[2] * minz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * maxx + p[1] * maxy + p[2] * minz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * minx + p[1] * miny + p[2] * maxz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * maxx + p[1] * miny + p[2] * maxz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * minx + p[1] * maxy + p[2] * maxz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }
        d = p[0] * maxx + p[1] * maxy + p[2] * maxz + p[3];
        if(!FLOATSIGNBIT(d))
        {
            continue;
        }

        return false;
    }

    return true;
}

//
// RB_ProjectPointToView
//
// Project world fixed coordinates to screen cordinates
//

void RB_ProjectPointToView(rbView_t *view, fixed_t fx, fixed_t fy, fixed_t fz, float *out_x, float *out_y)
{
    float projVec[4];
    float modlVec[4];
    float delta;

    modlVec[0] = FIXED2FLOAT(fx);
    modlVec[1] = FIXED2FLOAT(fy);
    modlVec[2] = FIXED2FLOAT(fz);
    modlVec[3] = 0;

    MTX_MultiplyVector4(view->modelview, modlVec);

    projVec[0] = modlVec[0];
    projVec[1] = modlVec[1];
    projVec[2] = modlVec[2];
    projVec[3] = modlVec[3];

    MTX_MultiplyVector4(view->projection, projVec);

    projVec[0] *= modlVec[3];
    projVec[1] *= modlVec[3];
    projVec[2] *= modlVec[3];

    if(projVec[3] != 0)
    {
        projVec[3] = 1.0f / projVec[3];
        projVec[0] *= projVec[3];
        projVec[1] *= projVec[3];
        projVec[2] *= projVec[3];
    }

    if(viewheight != SCREENHEIGHT)
    {
        // fudge y offset if using non-fullscreen hud
        delta = (float)screen_height / ((float)SCREENHEIGHT / 16.0f);
    }
    else
    {
        delta = 0;
    }

    *out_x = ( projVec[0] * 0.5f + 0.5f) * screen_width;
    *out_y = (-projVec[1] * 0.5f + 0.5f) * screen_height - delta;
}

//
// RB_RenderPlayerView
//

void RB_RenderPlayerView(player_t *player)
{
    // setup view and sprite list
    RB_SetupView(player, &rbPlayerView, rbFOV);
    RB_ClearSprites();
    
    if(rbDynamicLights)
    {
		RB_AddDynLights();
    }

    // setup draw lists
    DL_BeginDrawList();

    // check for new console commands.
    NetUpdate ();

    // render nodes and determine sprite distances
    RB_RenderBSPNode(numnodes-1);
    RB_SetupSprites();

    // check for new console commands.
    NetUpdate ();

    // draw scene
    RB_DrawScene();

    // check for new console commands.
    NetUpdate ();

    // set interpolated sector heights
    if(viewlerp != FRACUNIT)
    {
        R_SetSectorInterpolationState(SEC_NORMAL);
    }

    RB_SetOrtho();

    // render framebuffer with sprites
    if(rbFixSpriteClipping && has_GL_ARB_framebuffer_object)
    {
        RB_SetBlend(GLSRC_ONE, GLDST_ONE_MINUS_SRC_ALPHA);
        FBO_Draw(&spriteFBO, false);
    }

    // render player weapons
    RB_RenderPlayerSprites(player);

    // reset the viewport back to full screen dimentions
    RB_ResetViewPort();

    // fancy post-process stuff
    // dimitrisg 20201806 : broken on NX 
#ifndef SVE_PLAT_SWITCH
    RB_RenderMotionBlur();
    RB_RenderBloom();
    RB_RenderFXAA();
#endif
    
    // render player flash
    RB_DrawPlayerFlash(player);

    // render additional hud set pieces independent from the patch buffer
    RB_DrawExtraHudPics();

    // draw player names in multiplayer
    if(netgame)
    {
        RB_DrawPlayerNames();
    }
    
    // render damage markers
    if(d_dmgindictor)
    {
        RB_DrawDamageMarkers(player);
    }

    // check for new console commands.
    NetUpdate ();
}
