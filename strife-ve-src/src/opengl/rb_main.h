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

#ifndef __RB_MAIN_H__
#define __RB_MAIN_H__

#include "doomtype.h"
#include "rb_gl.h"
#include "rb_common.h"
#include "tables.h"

typedef enum
{
    GLSTATE_BLEND   = 0,
    GLSTATE_CULL,
    GLSTATE_TEXTURE0,
    GLSTATE_TEXTURE1,
    GLSTATE_TEXTURE2,
    GLSTATE_TEXTURE3,
    GLSTATE_DEPTHTEST,
    GLSTATE_STENCILTEST,
    GLSTATE_SCISSOR,
    GLSTATE_ALPHATEST,
    GLSTATE_TEXGEN_S,
    GLSTATE_TEXGEN_T,
    GLSTATE_FOG,
    NUMGLSTATES
} glState_t;

typedef enum
{
    GLFUNC_LEQUAL   = 0,
    GLFUNC_GEQUAL,
    GLFUNC_EQUAL,
    GLFUNC_NOTEQUAL,
    GLFUNC_GREATER,
    GLFUNC_LESS,
    GLFUNC_ALWAYS,
    GLFUNC_NEVER,
} glFunctions_t;

typedef enum {
    GLCULL_FRONT    = 0,
    GLCULL_BACK
} glCullType_t;

typedef enum
{
    GLPOLY_FILL     = 0,
    GLPOLY_LINE
} glPolyMode_t;

typedef enum
{
    GLSRC_ZERO      = 0,
    GLSRC_ONE,
    GLSRC_DST_COLOR,
    GLSRC_ONE_MINUS_DST_COLOR,
    GLSRC_SRC_ALPHA,
    GLSRC_ONE_MINUS_SRC_ALPHA,
    GLSRC_DST_ALPHA,
    GLSRC_ONE_MINUS_DST_ALPHA,
    GLSRC_ALPHA_SATURATE,
} glSrcBlend_t;

typedef enum
{
    GLDST_ZERO      = 0,
    GLDST_ONE,
    GLDST_SRC_COLOR,
    GLDST_ONE_MINUS_SRC_COLOR,
    GLDST_SRC_ALPHA,
    GLDST_ONE_MINUS_SRC_ALPHA,
    GLDST_DST_ALPHA,
    GLDST_ONE_MINUS_DST_ALPHA,
} glDstBlend_t;

typedef enum
{
    GLCB_COLOR      = BIT(0),
    GLCB_DEPTH      = BIT(1),
    GLCB_STENCIL    = BIT(2),
    GLCB_ALL        = (GLCB_COLOR|GLCB_DEPTH|GLCB_STENCIL)
} glClearBit_t;

typedef struct
{
    dtexture        currentTexture;
    int             environment;
} texUnit_t;

typedef struct
{
    float   x;
    float   y;
    float   z;
    float   tu;
    float   tv;
    byte    r;
    byte    g;
    byte    b;
    byte    a;
} vtx_t;

#define MAX_TEXTURE_UNITS 4

typedef struct
{
    unsigned int    glStateBits;
    int             depthFunction;
    glSrcBlend_t    blendSrc;
    glDstBlend_t    blendDest;
    glCullType_t    cullType;
    glPolyMode_t    polyMode;
    int             depthMask;
    int             colormask;
    glFunctions_t   alphaFunction;
    float           alphaFuncThreshold;
    int             currentUnit;
    rhandle         currentProgram;
    dtexture        currentFBO;
    texUnit_t       textureUnits[MAX_TEXTURE_UNITS];
    int             numStateChanges;
    int             numTextureBinds;
    int             numDrawnVertices;
    GLenum          drawBuffer;
    GLenum          readBuffer;
} rbState_t;

extern rbState_t rbState;
#ifdef SVE_PLAT_SWITCH
extern GL_Context ctx;
#endif

void RB_Init(void);
void RB_Shutdown(void);
void RB_InitDefaultState(void);
void RB_ResetViewPort(void);
int RB_GetMaxAnisotropic(void);
int RB_GetMaxColorAttachments(void);
angle_t RB_PointToAngle(fixed_t x, fixed_t y);
angle_t RB_PointToBam(fixed_t x, fixed_t y);
void RB_SetOrtho(void);
void RB_SetMaxOrtho(int sw, int sh);
void RB_SwapBuffers(void);
void RB_ClearBuffer(const glClearBit_t bit);
void RB_SetState(const int bits, boolean bEnable);
void RB_SetAlphaFunc(int func, float val);
void RB_SetDepth(int func);
void RB_SetBlend(int src, int dest);
void RB_SetCull(int type);
void RB_SetDepthMask(int enable);
void RB_SetColorMask(int enable);
void RB_SetTextureUnit(int unit);
void RB_SetScissorRect(const int x, const int y, const int w, const int h);
void RB_DisableShaders(void);
void RB_RestoreFrameBuffer(void);
void RB_SetDrawBuffer(const GLenum state);
void RB_SetReadBuffer(const GLenum state);
void RB_Printf(const int x, const int y, const char *string, ...);

#endif
