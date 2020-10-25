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
//    Framebuffer Objects
//

#include "rb_main.h"
#include "rb_gl.h"
#include "rb_fbo.h"
#include "rb_draw.h"
#include "r_state.h"
#include "i_system.h"
#include "i_video.h"
 
//
// FBO_CheckStatus
//

static void FBO_CheckStatus(rbfbo_t *fbo)
{
    GLenum status = dglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
    
    if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        switch(status)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
                I_Error("FBO_CheckStatus: bad attachment\n");
                break;
                
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                I_Error("FBO_CheckStatus: attachment is missing\n");
                break;
                
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                I_Error("FBO_CheckStatus: bad dimentions\n");
                break;
                
            case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
                I_Error("FBO_CheckStatus: bad format\n");
                break;
                
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
                I_Error("FBO_CheckStatus: error with draw buffer\n");
                break;
                
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
                I_Error("FBO_CheckStatus: error with read buffer\n");
                break;
                
            default:
                I_Error("FBO_CheckStatus: frame buffer creation didn't complete\n");
                break;
        }
        
        fbo->bLoaded = false;
    }
    else
    {
        fbo->bLoaded = true;
    }
}

//
// FBO_InitColorAttachment
//

void FBO_InitColorAttachment(rbfbo_t *fbo, const int attachment,
                             const int width, const int height)
{
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    if(fbo->bLoaded)
    {
        return;
    }
    
    if(attachment < 0 || attachment >= RB_GetMaxColorAttachments())
    {
        return;
    }
    
    fbo->fboAttachment = GL_COLOR_ATTACHMENT0_EXT + attachment;
    
    fbo->fboWidth = width;
    fbo->fboHeight = height;
    
    // texture
    dglGenTextures(1, &fbo->fboTexId);
    dglBindTexture(GL_TEXTURE_2D, fbo->fboTexId);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    dglTexImage2D(GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  fbo->fboWidth,
                  fbo->fboHeight,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  0);
    
    // framebuffer
    dglGenFramebuffers(1, &fbo->fboId);
    dglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo->fboId);
    RB_SetDrawBuffer(GL_NONE);
    RB_SetReadBuffer(GL_NONE);
    dglFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                            fbo->fboAttachment,
                            GL_TEXTURE_2D,
                            fbo->fboTexId,
                            0);

    // renderbuffer
    dglGenRenderbuffers(1, &fbo->rboId);
    dglBindRenderbuffer(GL_RENDERBUFFER_EXT, fbo->rboId);
    dglRenderbufferStorage(GL_RENDERBUFFER_EXT,
                           GL_DEPTH_COMPONENT,
                           fbo->fboWidth,
                           fbo->fboHeight);
    
    dglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT,
                               GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT,
                               fbo->rboId);
    
    FBO_CheckStatus(fbo);
    
    dglBindTexture(GL_TEXTURE_2D, 0);
    RB_RestoreFrameBuffer();
}

//
// FBO_Delete
//

void FBO_Delete(rbfbo_t *fbo)
{
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    if(!fbo->bLoaded)
    {
        return;
    }
    
    if(fbo->fboTexId != 0)
    {
        dglDeleteTextures(1, &fbo->fboTexId);
        fbo->fboTexId = 0;
    }
    
    if(fbo->fboId != 0)
    {
        dglDeleteFramebuffers(1, &fbo->fboId);
        fbo->fboId = 0;
    }
    
    if(fbo->rboId != 0)
    {
        dglDeleteRenderbuffers(1, &fbo->rboId);
        fbo->rboId = 0;
    }
    
    fbo->bLoaded = false;
}

//
// FBO_CopyBackBuffer
// Copies over the main framebuffer
//

void FBO_CopyBackBuffer(rbfbo_t *fbo, const int x, const int y, const int width, const int height)
{
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    dglBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    RB_SetReadBuffer(GL_BACK);
    dglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo->fboId);
    RB_SetDrawBuffer(fbo->fboAttachment);
    dglBlitFramebuffer(x,
                       y,
                       width,
                       height,
                       0,
                       0,
                       fbo->fboWidth,
                       fbo->fboHeight,
                       GL_COLOR_BUFFER_BIT,
                       GL_LINEAR);

    RB_RestoreFrameBuffer();
}

//
// FBO_CopyFrameBuffer
//

void FBO_CopyFrameBuffer(rbfbo_t *src, rbfbo_t *dst,
                         const int width, const int height)
{
    int w, h;
    
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    w = dst->fboWidth - (dst->fboWidth - width);
    h = dst->fboHeight - (dst->fboHeight - height);
    
    if(w > dst->fboWidth)
    {
        w = dst->fboWidth;
    }
    if(h > dst->fboHeight)
    {
        h = dst->fboHeight;
    }

    dglBindFramebuffer(GL_READ_FRAMEBUFFER, dst->fboId);
    RB_SetReadBuffer(dst->fboAttachment);
    dglBindFramebuffer(GL_DRAW_FRAMEBUFFER, src->fboId);
    RB_SetDrawBuffer(src->fboAttachment);
    dglBlitFramebuffer(0,
                       0,
                       w,
                       h,
                       0,
                       0,
                       src->fboWidth,
                       src->fboHeight,
                       GL_COLOR_BUFFER_BIT,
                       GL_LINEAR);
    
    RB_RestoreFrameBuffer();
}

//
// FBO_Bind
//

void FBO_Bind(rbfbo_t *fbo)
{
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    if(fbo->fboId == rbState.currentFBO)
    {
        return;
    }
    
    dglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo->fboId);
    RB_SetReadBuffer(fbo->fboAttachment);
    RB_SetDrawBuffer(fbo->fboAttachment);
    
    rbState.currentFBO = fbo->fboId;
}

//
// FBO_UnBind
//

void FBO_UnBind(rbfbo_t *fbo)
{
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    if(rbState.currentFBO == 0)
    {
        return;
    }
    
    RB_RestoreFrameBuffer();
}

//
// FBO_BindImage
//

void FBO_BindImage(rbfbo_t *fbo)
{
    int unit = rbState.currentUnit;
    dtexture currentTexture = rbState.textureUnits[unit].currentTexture;
    
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    if(fbo->fboTexId == currentTexture)
    {
        return;
    }
    
    dglBindTexture(GL_TEXTURE_2D, fbo->fboTexId);
    rbState.textureUnits[unit].currentTexture = fbo->fboTexId;
}

//
// FBO_UnBindImage
//

void FBO_UnBindImage(rbfbo_t *fbo)
{
    int unit = rbState.currentUnit;
    
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }
    
    if(rbState.textureUnits[unit].currentTexture == 0)
    {
        return;
    }
    
    dglBindTexture(GL_TEXTURE_2D, 0);
    rbState.textureUnits[unit].currentTexture = 0;
}

//
// FBO_Draw
//

void FBO_Draw(rbfbo_t *fbo, boolean forceScreenSize)
{
    vtx_t v[4];
    float delta;
    
    if(!has_GL_ARB_framebuffer_object)
    {
        return;
    }

    delta = 0;

    // adjust texture to match the resizing screen
    if(!forceScreenSize && viewheight != SCREENHEIGHT)
    {
        delta = 16;
    }

    FBO_BindImage(fbo);

    RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
    v[0].z = v[1].z = v[2].z = v[3].z = 0;

    v[0].x = v[2].x = 0;
    v[0].y = v[1].y = delta;
    v[1].x = v[3].x = SCREENWIDTH;
    v[2].y = v[3].y = SCREENHEIGHT + delta;

    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 1;
    v[1].tu = v[3].tu = 1;
    v[2].tv = v[3].tv = 0;

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);
    RB_SetState(GLSTATE_BLEND, true);
    RB_SetState(GLSTATE_ALPHATEST, true);

    // render
    RB_DrawVtxQuadImmediate(v);
    FBO_UnBindImage(fbo);

    RB_SetState(GLSTATE_DEPTHTEST, true);
}
