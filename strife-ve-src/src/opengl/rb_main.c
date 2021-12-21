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
//    OpenGL Rendering Backend
//

#include <math.h>

#include "rb_main.h"
#include "rb_data.h"
#include "rb_matrix.h"
#include "rb_draw.h"
#include "rb_drawlist.h"
#include "rb_hudtext.h"
#include "rb_config.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "m_argv.h"
#include "r_state.h"

//static int          viewWidth;
//static int          viewHeight;
//static int          viewWindowX;
//static int          viewWindowY;
static int          maxTextureUnits;
static int          maxTextureSize;
static int          maxColorAttachments;
static float        maxAnisotropic;
//static boolean      bIsInit;
static SDL_Surface  *screen;

static boolean      bPrintStats;

static const char   *gl_vendor;
static const char   *gl_renderer;
static const char   *gl_version;

rbState_t rbState;

extern SDL_Window *windowscreen;

//
// RB_Init
//

void RB_Init(void)
{
#if 0
    screen = SDL_GetWindowSurface(windowscreen);
#endif
    gl_vendor = (const char*)dglGetString(GL_VENDOR);
    gl_renderer = (const char*)dglGetString(GL_RENDERER);
    gl_version = (const char*)dglGetString(GL_VERSION);
    
	dglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    dglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    dglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &maxColorAttachments);
    dglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropic);

    fprintf(stdout, "GL_VENDOR: %s\n", gl_vendor);
    fprintf(stdout, "GL_RENDERER: %s\n", gl_renderer);
    fprintf(stdout, "GL_VERSION: %s\n", gl_version);
    fprintf(stdout, "GL_MAX_TEXTURE_SIZE: %i\n", maxTextureSize);
    fprintf(stdout, "GL_MAX_TEXTURE_UNITS_ARB: %i\n", maxTextureUnits);
    fprintf(stdout, "GL_MAX_COLOR_ATTACHMENTS_EXT: %i\n", maxColorAttachments);

    RB_InitDefaultState();
    RB_InitDrawer();

    bPrintStats = M_CheckParm("-printglstats");
#ifndef SVE_PLAT_SWITCH
    I_AtExit(RB_Shutdown, true);
#endif
}

//
// RB_Shutdown
//

void RB_Shutdown(void)
{
    RB_DeleteData();
    RB_HudTextShutdown();
    RB_ShutdownDrawer();
}

//
// RB_InitDefaultState
//
// Resets the OpenGL state
//

void RB_InitDefaultState(void)
{
    rbState.glStateBits     = 0;
    rbState.alphaFunction   = -1;
    rbState.blendDest       = -1;
    rbState.blendSrc        = -1;
    rbState.cullType        = -1;
    rbState.depthMask       = -1;
    rbState.colormask       = -1;
    rbState.currentUnit     = -1;
    rbState.currentProgram  = 0;
    rbState.currentFBO      = 0;
    rbState.drawBuffer      = GL_NONE;
    rbState.readBuffer      = GL_NONE;

    dglClearDepth(1.0f);
    dglClearStencil(0);
    dglClearColor(0, 0, 0, 1);
    
    RB_ResetViewPort();
    RB_ClearBuffer(GLCB_ALL);
    RB_SetState(GLSTATE_TEXTURE0, true);
    RB_SetState(GLSTATE_CULL, true);
    RB_SetState(GLSTATE_SCISSOR, false);
    RB_SetCull(GLCULL_BACK);
    RB_SetDepth(GLFUNC_LEQUAL);
    RB_SetAlphaFunc(GLFUNC_GEQUAL, 0.01f);
    RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
    RB_SetDepthMask(1);
    RB_SetColorMask(1);

    dglDisable(GL_NORMALIZE);
    dglShadeModel(GL_SMOOTH);
    dglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    dglFogi(GL_FOG_MODE, GL_LINEAR);
    dglHint(GL_FOG_HINT, GL_NICEST);
    dglEnable(GL_DITHER);
    dglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    dglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    dglEnableClientState(GL_VERTEX_ARRAY);
    dglEnableClientState(GL_TEXTURE_COORD_ARRAY);
    dglEnableClientState(GL_COLOR_ARRAY);
}

//
// RB_ResetViewPort
//

void RB_ResetViewPort(void)
{
#if 1
	int w;
	int h;
	SDL_GetWindowSize(windowscreen, &w, &h);
	dglViewport(0, 0, w, h);
#else
    dglViewport(0, 0, screen->w, screen->h);
#endif
}

//
// RB_GetMaxAnisotropic
//

int RB_GetMaxAnisotropic(void)
{
    return maxAnisotropic;
}

//
// RB_GetMaxColorAttachments
//

int RB_GetMaxColorAttachments(void)
{
    return maxColorAttachments;
}

//
// RB_SetOrtho
//

void RB_SetOrtho(void)
{
    matrix mtx;

    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();

    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();

    MTX_SetOrtho(mtx, 0, (float)SCREENWIDTH, (float)SCREENHEIGHT, 0, -1, 1);
    dglLoadMatrixf(mtx);
}

//
// RB_SetMaxOrtho
//

void RB_SetMaxOrtho(int sw, int sh)
{
    matrix mtx;

    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();

    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();

    MTX_SetOrtho(mtx, 0, (float)sw, (float)sh, 0, -1, 1);
    dglLoadMatrixf(mtx);
}

//
// RB_PointToAngle
//

angle_t RB_PointToAngle(fixed_t x, fixed_t y)
{
    if((!x) && (!y))
    {
        return 0;
    }

    if(x >= 0)
    {
        // x >=0
        if(y>= 0)
        {
            // y>= 0

            if(x > y)
            {
                // octant 0
                return tantoangle[SlopeDiv(y, x)];
            }
            else
            {
                // octant 1
                return ANG90-1-tantoangle[SlopeDiv(x, y)];
            }
        }
        else
        {
            // y<0
            y = -y;

            if(x > y)
            {
                // octant 8
                return 0-tantoangle[SlopeDiv(y,x)];
            }
            else
            {
                // octant 7
                return ANG270+tantoangle[SlopeDiv(x,y)];
            }
        }
    }
    else
    {
        // x<0
        x = -x;

        if(y >= 0)
        {
            // y>= 0
            if(x > y)
            {
                // octant 3
                return ANG180-1-tantoangle[SlopeDiv(y,x)];
            }
            else
            {
                // octant 2
                return ANG90+ tantoangle[SlopeDiv(x,y)];
            }
        }
        else
        {
            // y<0
            y = -y;

            if(x > y)
            {
                // octant 4
                return ANG180+tantoangle[SlopeDiv(y,x)];
            }
            else
            {
                // octant 5
                return ANG270-1-tantoangle[SlopeDiv(x,y)];
            }
        }
    }
}

//
// RB_PointToBam
//
// Same as RB_PointToAngle but more precise
//

angle_t RB_PointToBam(fixed_t x, fixed_t y)
{
    fixed_t dx = viewx - x;
    fixed_t dy = viewy - y;
    fixed_t fx;
    fixed_t fy;
    fixed_t t;

    t = dx >> 31; fx = ((dx ^ t) - t);
    t = dy >> 31; fy = ((dy ^ t) - t);

    if(fx != 0 || fy != 0)
    {
        double an = (double)dy / (double)(fx + fy);

        if(dx < 0)
        {
            an = 2.0f - an;
        }

        return (angle_t)(int64_t)(an * ANG90);
    }

    return 0;
}

//
// RB_SwapBuffers
//

void RB_SwapBuffers(void)
{
    if(bPrintStats)
    {
        RB_Printf(0, 0, "State Changes: %i", rbState.numStateChanges);
        RB_Printf(0, 12, "Texture Binds: %i", rbState.numTextureBinds);

        RB_Printf(0, 36, "Wall list size: %i", DL_GetDrawListSize(DLT_WALL));
        RB_Printf(0, 48, "Flat list size: %i", DL_GetDrawListSize(DLT_FLAT));
        RB_Printf(0, 60, "Sprite list size: %i", DL_GetDrawListSize(DLT_SPRITE));
        
        RB_Printf(0, 84, "Drawn Vertices: %i", rbState.numDrawnVertices);
    }

#ifndef SVE_PLAT_SWITCH
    if(rbForceSync)
#endif
    {
        // force a gl sync
        dglFinish();
    }

    SDL_GL_SwapWindow(windowscreen);

    // reset debugging info
    rbState.numStateChanges = 0;
    rbState.numTextureBinds = 0;
    rbState.numDrawnVertices = 0;
}

//
// RB_ClearBuffer
//

void RB_ClearBuffer(const glClearBit_t bit)
{
    int clearBit = 0;
    
    if(bit == GLCB_ALL)
    {
        clearBit = (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    else
    {
        if(bit & GLCB_COLOR)
        {
            clearBit |= GL_COLOR_BUFFER_BIT;
        }
        if(bit & GLCB_DEPTH)
        {
            clearBit |= GL_DEPTH_BUFFER_BIT;
        }
        if(bit & GLCB_STENCIL)
        {
            clearBit |= GL_STENCIL_BUFFER_BIT;
        }
    }
    
    dglClear(clearBit);
}

//
// RB_SetState
//

void RB_SetState(const int bits, boolean bEnable)
{
    int stateFlag = 0;
    
    switch(bits)
    {
        case GLSTATE_BLEND:
            stateFlag = GL_BLEND;
            break;

        case GLSTATE_CULL:
            stateFlag = GL_CULL_FACE;
            break;

        case GLSTATE_TEXTURE0:
            RB_SetTextureUnit(0);
            stateFlag = GL_TEXTURE_2D;
            break;

        case GLSTATE_TEXTURE1:
            RB_SetTextureUnit(1);
            stateFlag = GL_TEXTURE_2D;
            break;

        case GLSTATE_TEXTURE2:
            RB_SetTextureUnit(2);
            stateFlag = GL_TEXTURE_2D;
            break;

        case GLSTATE_TEXTURE3:
            RB_SetTextureUnit(3);
            stateFlag = GL_TEXTURE_2D;
            break;

        case GLSTATE_ALPHATEST:
            stateFlag = GL_ALPHA_TEST;
            break;

        case GLSTATE_TEXGEN_S:
            stateFlag = GL_TEXTURE_GEN_S;
            break;

        case GLSTATE_TEXGEN_T:
            stateFlag = GL_TEXTURE_GEN_T;
            break;

        case GLSTATE_DEPTHTEST:
            stateFlag = GL_DEPTH_TEST;
            break;

        case GLSTATE_FOG:
            stateFlag = GL_FOG;
            break;

        case GLSTATE_STENCILTEST:
            stateFlag = GL_STENCIL_TEST;
            break;

        case GLSTATE_SCISSOR:
            stateFlag = GL_SCISSOR_TEST;
            break;

        default:
            return;
    }
    
    // if state was already set then don't set it again
    if(bEnable && !(rbState.glStateBits & (1 << bits)))
    {
        dglEnable(stateFlag);
        rbState.glStateBits |= (1 << bits);
        rbState.numStateChanges++;
    }
    // if state was already unset then don't unset it again
    else if(!bEnable && (rbState.glStateBits & (1 << bits)))
    {
        dglDisable(stateFlag);
        rbState.glStateBits &= ~(1 << bits);
        rbState.numStateChanges++;
    }
}

//
// RB_SetAlphaFunc
//

void RB_SetAlphaFunc(int func, float val)
{
    int pFunc = (rbState.alphaFunction ^ func) | (rbState.alphaFuncThreshold != val);
    int glFunc = 0;

    if(pFunc == 0)
        return; // already set

    switch(func)
    {
        case GLFUNC_EQUAL:
            glFunc = GL_EQUAL;
            break;

        case GLFUNC_ALWAYS:
            glFunc = GL_ALWAYS;
            break;

        case GLFUNC_LEQUAL:
            glFunc = GL_LEQUAL;
            break;

        case GLFUNC_GEQUAL:
            glFunc = GL_GEQUAL;
            break;

        case GLFUNC_NEVER:
            glFunc = GL_NEVER;
            break;
    }
    
    dglAlphaFunc(glFunc, val);

    rbState.alphaFunction = func;
    rbState.alphaFuncThreshold = val;
    rbState.numStateChanges++;
}

//
// RB_SetDepth
//

void RB_SetDepth(int func)
{
    int pFunc = rbState.depthFunction ^ func;
    int glFunc = 0;
    
    if(pFunc == 0)
        return; // already set
        
    switch(func)
    {
        case GLFUNC_EQUAL:
            glFunc = GL_EQUAL;
            break;

        case GLFUNC_ALWAYS:
            glFunc = GL_ALWAYS;
            break;

        case GLFUNC_LEQUAL:
            glFunc = GL_LEQUAL;
            break;

        case GLFUNC_GEQUAL:
            glFunc = GL_GEQUAL;
            break;

        case GLFUNC_NOTEQUAL:
            glFunc = GL_NOTEQUAL;
            break;

        case GLFUNC_GREATER:
            glFunc = GL_GREATER;
            break;

        case GLFUNC_LESS:
            glFunc = GL_LESS;
            break;

        case GLFUNC_NEVER:
            glFunc = GL_NEVER;
            break;
    }
    
    dglDepthFunc(glFunc);
    rbState.depthFunction = func;
    rbState.numStateChanges++;
}

//
// RB_SetBlend
//

void RB_SetBlend(int src, int dest)
{
    int pBlend = (rbState.blendSrc ^ src) | (rbState.blendDest ^ dest);
    int glSrc = GL_ONE;
    int glDst = GL_ONE;
    
    if(pBlend == 0)
        return; // already set
    
    switch(src)
    {
        case GLSRC_ZERO:
            glSrc = GL_ZERO;
            break;

        case GLSRC_ONE:
            glSrc = GL_ONE;
            break;

        case GLSRC_DST_COLOR:
            glSrc = GL_DST_COLOR;
            break;

        case GLSRC_ONE_MINUS_DST_COLOR:
            glSrc = GL_ONE_MINUS_DST_COLOR;
            break;

        case GLSRC_SRC_ALPHA:
            glSrc = GL_SRC_ALPHA;
            break;

        case GLSRC_ONE_MINUS_SRC_ALPHA:
            glSrc = GL_ONE_MINUS_SRC_ALPHA;
            break;

        case GLSRC_DST_ALPHA:
            glSrc = GL_DST_ALPHA;
            break;

        case GLSRC_ONE_MINUS_DST_ALPHA:
            glSrc = GL_ONE_MINUS_DST_ALPHA;
            break;

        case GLSRC_ALPHA_SATURATE:
            glSrc = GL_SRC_ALPHA_SATURATE;
            break;
    }
    
    switch(dest) {
        case GLDST_ZERO:
            glDst = GL_ZERO;
            break;

        case GLDST_ONE:
            glDst = GL_ONE;
            break;

        case GLDST_SRC_COLOR:
            glDst = GL_SRC_COLOR;
            break;

        case GLDST_ONE_MINUS_SRC_COLOR:
            glDst = GL_ONE_MINUS_SRC_COLOR;
            break;

        case GLDST_SRC_ALPHA:
            glDst = GL_SRC_ALPHA;
            break;

        case GLDST_ONE_MINUS_SRC_ALPHA:
            glDst = GL_ONE_MINUS_SRC_ALPHA;
            break;

        case GLDST_DST_ALPHA:
            glDst = GL_DST_ALPHA;
            break;

        case GLDST_ONE_MINUS_DST_ALPHA:
            glDst = GL_ONE_MINUS_DST_ALPHA;
            break;
    }
    
    dglBlendFunc(glSrc, glDst);

    rbState.blendSrc = src;
    rbState.blendDest = dest;
    rbState.numStateChanges++;
}

//
// RB_SetCull
//

void RB_SetCull(int type)
{
    int pCullType = rbState.cullType ^ type;
    int cullType = 0;
    
    if(pCullType == 0)
        return; // already set
    
    switch(type)
    {
        case GLCULL_FRONT:
            cullType = GL_FRONT;
            break;

        case GLCULL_BACK:
            cullType = GL_BACK;
            break;

        default:
            return;
    }
    
    dglCullFace(cullType);
    rbState.cullType = type;
    rbState.numStateChanges++;
}

//
// RB_SetDepthMask
//

void RB_SetDepthMask(int enable)
{
    int pEnable = rbState.depthMask ^ enable;
    int flag = 0;
    
    if(pEnable == 0)
        return; // already set
    
    switch(enable)
    {
        case 1:
            flag = GL_TRUE;
            break;

        case 0:
            flag = GL_FALSE;
            break;

        default:
            return;
    }
    
    dglDepthMask(flag);
    rbState.depthMask = enable;
}

//
// RB_SetColorMask
//

void RB_SetColorMask(int enable)
{
    int pEnable = rbState.colormask ^ enable;
    int flag = 0;
    
    if(pEnable == 0)
        return; // already set
    
    switch(enable)
    {
        case 1:
            flag = GL_TRUE;
            break;

        case 0:
            flag = GL_FALSE;
            break;

        default:
            return;
    }
    
    dglColorMask(flag, flag, flag, flag);
    rbState.colormask = enable;
}

//
// RB_SetTextureUnit
//

void RB_SetTextureUnit(int unit)
{
    if(unit > MAX_TEXTURE_UNITS || unit < 0)
    {
        return;
    }
    
    if(unit == rbState.currentUnit)
    {
        return; // already binded
    }
        
#ifndef SVE_PLAT_SWITCH
    dglActiveTextureARB(GL_TEXTURE0_ARB + unit);
    dglClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
#endif
    rbState.currentUnit = unit;
}

//
// RB_SetScissorRect
//

void RB_SetScissorRect(const int x, const int y, const int w, const int h)
{
    int wsw;
	int wsh;
	SDL_GetWindowSize(windowscreen, &wsw, &wsh);
    dglScissor(x, wsh - y, w, h);
}

//
// RB_DisableShaders
//

void RB_DisableShaders(void)
{
    if(rbState.currentProgram != 0)
    {
        dglUseProgramObjectARB(0);
        rbState.currentProgram = 0;
    }
}

//
// RB_RestoreFrameBuffer
// Resets back to primary framebuffer
//

void RB_RestoreFrameBuffer(void)
{
    dglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    RB_SetDrawBuffer(GL_BACK);
    RB_SetReadBuffer(GL_BACK);
    
    rbState.currentFBO = 0;
}

//
// RB_SetDrawBuffer
//

void RB_SetDrawBuffer(const GLenum state)
{
    if(rbState.drawBuffer == state)
    {
        return; // already set
    }
    
    dglDrawBuffer(state);
    rbState.drawBuffer = state;
}

//
// RB_SetReadBuffer
//

void RB_SetReadBuffer(const GLenum state)
{
    if(rbState.readBuffer == state)
    {
        return; // already set
    }
    
    dglReadBuffer(state);
    rbState.readBuffer = state;
}

//
// RB_Printf
//

void RB_Printf(const int x, const int y, const char *string, ...)
{
    static char buffer[1024];

    va_list	va;
    
    va_start(va, string);
    M_vsnprintf(buffer, sizeof(buffer), string, va);
    va_end(va);

    RB_HudTextDraw(x, y, 0.35f, 0xff, buffer);
}
