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

#ifndef __FBO_H__
#define __FBO_H__

typedef struct
{
    dtexture        fboId;
    dtexture        rboId;
    dtexture        fboTexId;
    boolean         bLoaded;
    unsigned int    fboAttachment;
    int             fboWidth;
    int             fboHeight;
} rbfbo_t;

void FBO_InitColorAttachment(rbfbo_t *fbo, const int attachment,
                             const int width, const int height);
void FBO_CopyBackBuffer(rbfbo_t *fbo, const int x, const int y, const int width, const int height);
void FBO_CopyFrameBuffer(rbfbo_t *src, rbfbo_t *dst, const int width, const int height);
void FBO_Delete(rbfbo_t *fbo);
void FBO_BindImage(rbfbo_t *fbo);
void FBO_Bind(rbfbo_t *fbo);
void FBO_UnBind(rbfbo_t *fbo);
void FBO_UnBindImage(rbfbo_t *fbo);
void FBO_Draw(rbfbo_t *fbo, boolean forceScreenSize);

#endif
