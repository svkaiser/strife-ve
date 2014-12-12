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

#ifndef __RB_SHADER_H__
#define __RB_SHADER_H__

#include "rb_matrix.h"

typedef enum
{
    RST_VERTEX          = 0,
    RST_FRAGMENT,
    RST_TOTAL
} rShaderType_t;

typedef struct
{
    rhandle     programObj;
    rhandle     vertexProgram;
    rhandle     fragmentProgram;
    boolean     bHasErrors;
    boolean     bLoaded;
} rbShader_t;

void SP_Enable(rbShader_t *shader);
void SP_Delete(rbShader_t *shader);
void SP_SetUniform1i(rbShader_t *shader, const char *name, const int val);
void SP_SetUniform1f(rbShader_t *shader, const char *name, const float val);
void SP_SetUniformMat4(rbShader_t *shader, const char *name, matrix val, boolean bTranspose);
void SP_LoadProgram(rbShader_t *shader, const char *program);

#endif
