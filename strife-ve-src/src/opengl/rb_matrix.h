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

#ifndef __RB_MATRIX_H__
#define __RB_MATRIX_H__

typedef float matrix[16];

void MTX_Copy(matrix dst, matrix src);
boolean MTX_IsUninitialized(matrix m);
void MTX_Identity(matrix m);
void MTX_IdentityX(matrix m, float angle);
void MTX_IdentityY(matrix m, float angle);
void MTX_IdentityZ(matrix m, float angle);
void MTX_IdentityTranspose(matrix dst, matrix src);
void MTX_RotateX(matrix m, float angle);
void MTX_RotateY(matrix m, float angle);
void MTX_RotateZ(matrix m, float angle);
void MTX_SetTranslation(matrix m, float x, float y, float z);
void MTX_Scale(matrix m, float x, float y, float z);
void MTX_ViewFrustum(matrix m, int width, int height, float fovy, float znear);
void MTX_SetOrtho(matrix m, float left, float right, float bottom, float top, float zNear, float zFar);
void MTX_Multiply(matrix m, matrix m1, matrix m2);
void MTX_MultiplyRotations(matrix m, matrix m1, matrix m2);
void MTX_MultiplyVector(matrix m, float *xyz);
void MTX_MultiplyVector4(matrix m, float *xyzw);
void MTX_Invert(matrix out, matrix in);
void MTX_ToQuaternion(matrix m, float *out);

#endif
