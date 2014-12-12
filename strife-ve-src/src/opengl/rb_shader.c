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
//    Shader Program (GLSL)
//

#include "rb_main.h"
#include "rb_gl.h"
#include "rb_shader.h"
#include "deh_str.h"
#include "w_wad.h"
#include "z_zone.h"

//
// SP_Enable
//

void SP_Enable(rbShader_t *shader)
{
    if(!has_GL_ARB_shader_objects)
    {
        return;
    }

    if(shader->programObj == rbState.currentProgram)
    {
        return;
    }
    
    dglUseProgramObjectARB(shader->programObj);
    rbState.currentProgram = shader->programObj;
}

//
// SP_Delete
//

void SP_Delete(rbShader_t *shader)
{
    if(!has_GL_ARB_shader_objects)
    {
        return;
    }

    if(shader->bLoaded == false)
    {
        return;
    }
    
    dglDeleteObjectARB(shader->fragmentProgram);
    dglDeleteObjectARB(shader->vertexProgram);
    dglDeleteObjectARB(shader->programObj);
    shader->bLoaded = false;
}

//
// SP_SetUniform1i
//

void SP_SetUniform1i(rbShader_t *shader, const char *name, const int val)
{
    int loc;
    
    if(!has_GL_ARB_shader_objects)
    {
        return;
    }

    loc = dglGetUniformLocationARB(shader->programObj, name);

    if(loc != -1)
    {
        dglUniform1iARB(loc, val);
    }
}

//
// SP_SetUniform1f
//

void SP_SetUniform1f(rbShader_t *shader, const char *name, const float val)
{
    int loc;
    
    if(!has_GL_ARB_shader_objects)
    {
        return;
    }

    loc = dglGetUniformLocationARB(shader->programObj, name);

    if(loc != -1)
    {
        dglUniform1fARB(loc, val);
    }
}

//
// SP_SetUniformMat4
//

void SP_SetUniformMat4(rbShader_t *shader, const char *name, matrix val, boolean bTranspose)
{
    int loc;

    if(!has_GL_ARB_shader_objects)
    {
        return;
    }
    
    loc = dglGetUniformLocationARB(shader->programObj, name);

    if(loc != -1)
    {
        dglUniformMatrix4fvARB(loc, 1, bTranspose, val);
    }
}

//
// SP_Compile
//

static void SP_Compile(rbShader_t *shader, const char *name, rShaderType_t type)
{
    byte *data;
    int length;
    int lump;
    rhandle *handle;

    lump = W_GetNumForName((char*)name);
    length = W_LumpLength(lump);

    data = (byte*)Z_Calloc(length+1, sizeof(char), PU_STATIC, NULL);
    W_ReadLump(lump, data);
    
    if(type == RST_VERTEX)
    {
        shader->vertexProgram = dglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
        handle = &shader->vertexProgram;
    }
    else if(type == RST_FRAGMENT)
    {
        shader->fragmentProgram = dglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
        handle = &shader->fragmentProgram;
    }
    else
    {
        Z_Free(data);
        return;
    }
    
    dglShaderSourceARB(*handle, 1, (const GLcharARB**)&data, NULL);
    dglCompileShaderARB(*handle);
    dglAttachObjectARB(shader->programObj, *handle);
    
    Z_Free(data);
}

//
// SP_DumpErrorLog
//

static void SP_DumpErrorLog(const rhandle handle)
{
    int logLength;

    dglGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
    
    if(logLength > 0)
    {
        int cw;
        char *log;
        
        log = (char*)malloc(logLength);
        
        dglGetInfoLogARB(handle, logLength, &cw, log);
        fprintf(stderr, "%s\n", log);

        free(log);
    }
}

//
// SP_Link
//

static boolean SP_Link(rbShader_t *shader)
{
    int linked;
    
    shader->bHasErrors = false;
    dglLinkProgramARB(shader->programObj);
    dglGetObjectParameterivARB(shader->programObj, GL_OBJECT_LINK_STATUS_ARB, &linked);
    
    if(!linked)
    {
        shader->bHasErrors = true;
        SP_DumpErrorLog(shader->programObj);
        SP_DumpErrorLog(shader->vertexProgram);
        SP_DumpErrorLog(shader->fragmentProgram);
    }
    
    dglUseProgramObjectARB(0);
    shader->bLoaded = true;
    return (linked > 0);
}

//
// SP_LoadProgram
//

void SP_LoadProgram(rbShader_t *shader, const char *program)
{
    char namebuf[9];

    shader->bHasErrors = false;
    shader->bLoaded = false;

    if(!has_GL_ARB_shader_objects)
    {
        return;
    }
    
    shader->programObj = dglCreateProgramObjectARB();

    DEH_snprintf(namebuf, 9, "%s_V", program);
    SP_Compile(shader, namebuf, RST_VERTEX);

    DEH_snprintf(namebuf, 9, "%s_F", program);
    SP_Compile(shader, namebuf, RST_FRAGMENT);

    SP_Link(shader);
}
