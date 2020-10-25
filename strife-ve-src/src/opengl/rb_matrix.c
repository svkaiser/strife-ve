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
//    Matrix Operations
//
// Reference:
// _________________ 
// | 0   4   8   12 |
// | 1   5   9   13 |
// | 2   6  10   14 |
// | 3   7  11   15 |
// _________________
//
// translation
// _________________ 
// | 0   4   8   x  |
// | 1   5   9   y  |
// | 2   6  10   z  |
// | 3   7  11   15 |
// _________________
//
// rotation x
// _________________ 
// |(1)  4   8   x  |
// | 1   xc -xs  y  |
// | 2   xs  xs  z  |
// | 3   7  11  (1) |
// _________________
//
// rotation y
// _________________ 
// | yc  4  ys   12 |
// | 1  (1)  9   13 |
// |-ys  6  yc   14 |
// | 3   7  11  (1) |
// _________________
//
// rotation z
// _________________ 
// | zc -zs  8   12 |
// | zs  zc  9   13 |
// | 2   6  (1)  14 |
// | 3   7  11  (1) |
// _________________
//

#include <math.h>
#include "rb_common.h"
#include "rb_main.h"
#include "rb_matrix.h"

//
// MTX_Copy
//

void MTX_Copy(matrix dst, matrix src)
{
    memcpy(dst, src, sizeof(matrix));
}

//
// MTX_IsUninitialized
//

boolean MTX_IsUninitialized(matrix m)
{
    int i;
    float f = 0;

    for(i = 0; i < 16; i++)
    {
        f += m[i];
    }

    return (f == 0);
}

//
// MTX_Identity
//

void MTX_Identity(matrix m)
{
    m[ 0] = 1;
    m[ 1] = 0;
    m[ 2] = 0;
    m[ 3] = 0;
    m[ 4] = 0;
    m[ 5] = 1;
    m[ 6] = 0;
    m[ 7] = 0;
    m[ 8] = 0;
    m[ 9] = 0;
    m[10] = 1;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_SetTranslation
//

void MTX_SetTranslation(matrix m, float x, float y, float z)
{
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

//
// MTX_Scale
//

void MTX_Scale(matrix m, float x, float y, float z)
{
    m[ 0] = x * m[ 0];
    m[ 1] = x * m[ 1];
    m[ 2] = x * m[ 2];
    m[ 4] = y * m[ 4];
    m[ 5] = y * m[ 5];
    m[ 6] = y * m[ 6];
    m[ 8] = z * m[ 8];
    m[ 9] = z * m[ 9];
    m[10] = z * m[10];
}

//
// MTX_IdentityX
//

void MTX_IdentityX(matrix m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[ 0] = 1;
    m[ 1] = 0;
    m[ 2] = 0;
    m[ 3] = 0;
    m[ 4] = 0;
    m[ 5] = c;
    m[ 6] = s;
    m[ 7] = 0;
    m[ 8] = 0;
    m[ 9] = -s;
    m[10] = c;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_IdentityY
//

void MTX_IdentityY(matrix m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[ 0] = c;
    m[ 1] = 0;
    m[ 2] = -s;
    m[ 3] = 0;
    m[ 4] = 0;
    m[ 5] = 1;
    m[ 6] = 0;
    m[ 7] = 0;
    m[ 8] = s;
    m[ 9] = 0;
    m[10] = c;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_IdentityZ
//

void MTX_IdentityZ(matrix m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[ 0] = c;
    m[ 1] = s;
    m[ 2] = 0;
    m[ 3] = 0;
    m[ 4] = -s;
    m[ 5] = c;
    m[ 6] = 0;
    m[ 7] = 0;
    m[ 8] = 0;
    m[ 9] = 0;
    m[10] = 1;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_IdentityTranspose
//

void MTX_IdentityTranspose(matrix dst, matrix src)
{
    int i;

    for(i = 0; i < 4; ++i)
    {
        dst[0 * 4 + i] = src[0 * 4 + i];
    }

    for(i = 0; i < 4; ++i)
    {
        dst[1 * 4 + i] = src[2 * 4 + i];
    }

    for(i = 0; i < 4; ++i)
    {
        dst[2 * 4 + i] = src[1 * 4 + i];
    }

    for(i = 0; i < 4; ++i)
    {
        dst[3 * 4 + i] = src[3 * 4 + i];
    }
}

//
// MTX_RotateX
//

void MTX_RotateX(matrix m, float angle)
{
    float s;
    float c;
    float tm1;
    float tm5;
    float tm9;
    float tm13;

    s = sinf(angle);
    c = cosf(angle);

    tm1     = m[ 1];
    tm5     = m[ 5];
    tm9     = m[ 9];
    tm13    = m[13];

    m[ 1] = tm1  * c - s * m[ 2];
    m[ 2] = c * m[ 2] + tm1  * s;
    m[ 5] = tm5  * c - s * m[ 6];
    m[ 6] = c * m[ 6] + tm5  * s;
    m[ 9] = tm9  * c - s * m[10];
    m[10] = c * m[10] + tm9  * s;
    m[13] = tm13 * c - s * m[14];
    m[14] = c * m[14] + tm13 * s;
}

//
// MTX_RotateY
//

void MTX_RotateY(matrix m, float angle)
{
    float s;
    float c;
    float tm0;
    float tm1;
    float tm2;

    s = sinf(angle);
    c = cosf(angle);

    tm0     = m[ 0];
    tm1     = m[ 1];
    tm2     = m[ 2];

    m[ 0] = tm0 * c - s * m[ 8];
    m[ 8] = c * m[ 8] + tm0 * s;
    m[ 1] = tm1 * c - s * m[ 9];
    m[ 9] = c * m[ 9] + tm1 * s;
    m[ 2] = tm2 * c - s * m[10];
    m[10] = c * m[10] + tm2 * s;
}

//
// MTX_RotateZ
//

void MTX_RotateZ(matrix m, float angle)
{
    float s;
    float c;
    float tm0;
    float tm8;
    float tm4;
    float tm12;

    s = sinf(angle);
    c = cosf(angle);

    tm0     = m[ 0];
    tm4     = m[ 4];
    tm8     = m[ 8];
    tm12    = m[12];

    m[ 0] = s * m[ 2] + tm0  * c;
    m[ 2] = c * m[ 2] - tm0  * s;
    m[ 4] = s * m[ 6] + tm4  * c;
    m[ 6] = c * m[ 6] - tm4  * s;
    m[ 8] = s * m[10] + tm8  * c;
    m[10] = c * m[10] - tm8  * s;
    m[12] = s * m[14] + tm12 * c;
    m[14] = c * m[14] - tm12 * s;
}

//
// MTX_ViewFrustum
//

void MTX_ViewFrustum(matrix m, int width, int height, float fovy, float znear) {
    float left;
    float right;
    float bottom;
    float top;
    float aspect;

    aspect = (float)width / (float)height;
    top = znear * (float)tan((float)fovy * M_PI / 360.0f);
    bottom = -top;
    left = bottom * aspect;
    right = top * aspect;

    m[ 0] = (2 * znear) / (right - left);
    m[ 4] = 0;
    m[ 8] = (right + left) / (right - left);
    m[12] = 0;

    m[ 1] = 0;
    m[ 5] = (2 * znear) / (top - bottom);
    m[ 9] = (top + bottom) / (top - bottom);
    m[13] = 0;

    m[ 2] = 0;
    m[ 6] = 0;
    m[10] = -1;
    m[14] = -2 * znear;

    m[ 3] = 0;
    m[ 7] = 0;
    m[11] = -1;
    m[15] = 0;
}

//
// MTX_SetOrtho
//

void MTX_SetOrtho(matrix m, float left, float right, float bottom, float top, float zNear, float zFar)
{
    m[ 0] =  2 / (right - left);
    m[ 5] =  2 / (top - bottom);
    m[10] = -2 / (zFar - zNear);
    
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(zFar + zNear) / (zFar - zNear);
    m[15] = 1;
    
    m[ 1] = 0;
    m[ 2] = 0;
    m[ 3] = 0;
    m[ 4] = 0;
    m[ 6] = 0;
    m[ 7] = 0;
    m[ 8] = 0;
    m[ 9] = 0;
    m[11] = 0;
}

//
// MTX_Multiply
//

void MTX_Multiply(matrix m, matrix m1, matrix m2)
{
    m[ 0] = m1[ 1] * m2[ 4] + m2[ 8] * m1[ 2] + m1[ 3] * m2[12] + m1[ 0] * m2[ 0];
    m[ 1] = m1[ 0] * m2[ 1] + m2[ 5] * m1[ 1] + m2[ 9] * m1[ 2] + m2[13] * m1[ 3];
    m[ 2] = m1[ 0] * m2[ 2] + m2[10] * m1[ 2] + m2[14] * m1[ 3] + m2[ 6] * m1[ 1];
    m[ 3] = m1[ 0] * m2[ 3] + m2[15] * m1[ 3] + m2[ 7] * m1[ 1] + m2[11] * m1[ 2];
    m[ 4] = m2[ 0] * m1[ 4] + m1[ 7] * m2[12] + m1[ 5] * m2[ 4] + m1[ 6] * m2[ 8];
    m[ 5] = m1[ 4] * m2[ 1] + m1[ 5] * m2[ 5] + m1[ 7] * m2[13] + m1[ 6] * m2[ 9];
    m[ 6] = m1[ 5] * m2[ 6] + m1[ 7] * m2[14] + m1[ 4] * m2[ 2] + m1[ 6] * m2[10];
    m[ 7] = m1[ 6] * m2[11] + m1[ 7] * m2[15] + m1[ 5] * m2[ 7] + m1[ 4] * m2[ 3];
    m[ 8] = m2[ 0] * m1[ 8] + m1[10] * m2[ 8] + m1[11] * m2[12] + m1[ 9] * m2[ 4];
    m[ 9] = m1[ 8] * m2[ 1] + m1[10] * m2[ 9] + m1[11] * m2[13] + m1[ 9] * m2[ 5];
    m[10] = m1[ 9] * m2[ 6] + m1[10] * m2[10] + m1[11] * m2[14] + m1[ 8] * m2[ 2];
    m[11] = m1[ 9] * m2[ 7] + m1[11] * m2[15] + m1[10] * m2[11] + m1[ 8] * m2[ 3];
    m[12] = m2[ 0] * m1[12] + m2[12] * m1[15] + m2[ 4] * m1[13] + m2[ 8] * m1[14];
    m[13] = m2[13] * m1[15] + m2[ 1] * m1[12] + m2[ 9] * m1[14] + m2[ 5] * m1[13];
    m[14] = m2[ 6] * m1[13] + m2[14] * m1[15] + m2[10] * m1[14] + m2[ 2] * m1[12];
    m[15] = m2[ 3] * m1[12] + m2[ 7] * m1[13] + m2[11] * m1[14] + m2[15] * m1[15];
}

//
// MTX_MultiplyRotations
//

void MTX_MultiplyRotations(matrix m, matrix m1, matrix m2)
{
    m[ 0] = m2[ 4] * m1[ 1] + m1[ 2] * m2[ 8] + m2[ 0] * m1[ 0];
    m[ 1] = m1[ 0] * m2[ 1] + m2[ 9] * m1[ 2] + m2[ 5] * m1[ 1];
    m[ 2] = m1[ 0] * m2[ 2] + m1[ 1] * m2[ 6] + m1[ 2] * m2[10];
    m[ 3] = 0;
    m[ 4] = m2[ 0] * m1[ 4] + m2[ 4] * m1[ 5] + m1[ 6] * m2[ 8];
    m[ 5] = m2[ 5] * m1[ 5] + m1[ 6] * m2[ 9] + m1[ 4] * m2[ 1];
    m[ 6] = m1[ 5] * m2[ 6] + m1[ 6] * m2[10] + m1[ 4] * m2[ 2];
    m[ 7] = 0;
    m[ 8] = m2[ 0] * m1[ 8] + m1[10] * m2[ 8] + m1[ 9] * m2[ 4];
    m[ 9] = m1[ 8] * m2[ 1] + m1[ 9] * m2[ 5] + m1[10] * m2[ 9];
    m[10] = m1[ 8] * m2[ 2] + m1[ 9] * m2[ 6] + m1[10] * m2[10];
    m[11] = 0;
    m[12] = m2[ 0] * m1[12] + m1[14] * m2[ 8] + m1[13] * m2[ 4] + m2[12];
    m[13] = m1[13] * m2[ 5] + m1[14] * m2[ 9] + m1[12] * m2[ 1] + m2[13];
    m[14] = m1[12] * m2[ 2] + m1[14] * m2[10] + m1[13] * m2[ 6] + m2[14];
    m[15] = 1;
}

//
// MTX_MultiplyVector
//

void MTX_MultiplyVector(matrix m, float *xyz)
{
    float _x = xyz[0];
    float _y = xyz[1];
    float _z = xyz[2];
    
    xyz[0] = m[ 4] * _y + m[ 8] * _z + m[ 0] * _x + m[12];
    xyz[1] = m[ 5] * _y + m[ 9] * _z + m[ 1] * _x + m[13];
    xyz[2] = m[ 6] * _y + m[10] * _z + m[ 2] * _x + m[14];
}

//
// MTX_MultiplyVector4
//

void MTX_MultiplyVector4(matrix m, float *xyzw)
{
    float _x = xyzw[0];
    float _y = xyzw[1];
    float _z = xyzw[2];
    //float _w = xyzw[3];
    
    xyzw[0] = m[ 4] * _y + m[ 8] * _z + m[ 0] * _x + m[12];
    xyzw[1] = m[ 5] * _y + m[ 9] * _z + m[ 1] * _x + m[13];
    xyzw[2] = m[ 6] * _y + m[10] * _z + m[ 2] * _x + m[14];
    xyzw[3] = m[ 7] * _y + m[11] * _z + m[ 3] * _x + m[15];
}

//
// MTX_Invert
//

void MTX_Invert(matrix out, matrix in)
{
    float d;
    matrix m;
    
    MTX_Copy(m, in);
    
    d = m[ 0] * m[10] * m[ 5] -
        m[ 0] * m[ 9] * m[ 6] -
        m[ 1] * m[ 4] * m[10] +
        m[ 2] * m[ 4] * m[ 9] +
        m[ 1] * m[ 6] * m[ 8] -
        m[ 2] * m[ 5] * m[ 8];
    
    if(d != 0.0f)
    {
        matrix inv;
        
        d = (1.0f / d);
        
        inv[ 0] = (  m[10] * m[ 5] - m[ 9] * m[ 6]) * d;
        inv[ 1] = -((m[ 1] * m[10] - m[ 2] * m[ 9]) * d);
        inv[ 2] = (  m[ 1] * m[ 6] - m[ 2] * m[ 5]) * d;
        inv[ 3] = 0;
        inv[ 4] = (  m[ 6] * m[ 8] - m[ 4] * m[10]) * d;
        inv[ 5] = (  m[ 0] * m[10] - m[ 2] * m[ 8]) * d;
        inv[ 6] = -((m[ 0] * m[ 6] - m[ 2] * m[ 4]) * d);
        inv[ 7] = 0;
        inv[ 8] = -((m[ 5] * m[ 8] - m[ 4] * m[ 9]) * d);
        inv[ 9] = (  m[ 1] * m[ 8] - m[ 0] * m[ 9]) * d;
        inv[10] = -((m[ 1] * m[ 4] - m[ 0] * m[ 5]) * d);
        inv[11] = 0;
        inv[12] = (( m[13] * m[10] - m[14] * m[ 9]) * m[ 4]
                   + m[14] * m[ 5] * m[ 8]
                   - m[13] * m[ 6] * m[ 8]
                   - m[12] * m[10] * m[ 5]
                   + m[12] * m[ 9] * m[ 6]) * d;
        inv[13] = (  m[ 0] * m[14] * m[ 9]
                   - m[ 0] * m[13] * m[10]
                   - m[14] * m[ 1] * m[ 8]
                   + m[13] * m[ 2] * m[ 8]
                   + m[12] * m[ 1] * m[10]
                   - m[12] * m[ 2] * m[ 9]) * d;
        inv[14] = -((  m[ 0] * m[14] * m[ 5]
                     - m[ 0] * m[13] * m[ 6]
                     - m[14] * m[ 1] * m[ 4]
                     + m[13] * m[ 2] * m[ 4]
                     + m[12] * m[ 1] * m[ 6]
                     - m[12] * m[ 2] * m[ 5]) * d);
        inv[15] = 1.0f;
        
        MTX_Copy(out, inv);
        return;
    }
    
    MTX_Copy(out, m);
}

//
// MTX_ToQuaternion
//
// Assumes out is an array of 4 floats
//

void MTX_ToQuaternion(matrix m, float *out)
{
    float t, d;
    float mx, my, mz;
    float m21, m20, m10;
    
    mx = m[ 0];
    my = m[ 5];
    mz = m[10];
    
    m21 = (m[ 9] - m[ 6]);
    m20 = (m[ 8] - m[ 2]);
    m10 = (m[ 4] - m[ 1]);
    
    t = 1.0f + mx + my + mz;
    
    if(t > 0)
    {
        d = 0.5f / (t * InvSqrt(t));
        out[0] = m21 * d;
        out[1] = m20 * d;
        out[2] = m10 * d;
        out[3] = 0.25f / d;
    }
    else if(mx > my && mx > mz)
    {
        t = 1.0f + mx - my - mz;
        d = (t * InvSqrt(t)) * 2;
        out[0] = 0.5f / d;
        out[1] = m10 / d;
        out[2] = m20 / d;
        out[3] = m21 / d;
    }
    else if(my > mz)
    {
        t = 1.0f + my - mx - mz;
        d = (t * InvSqrt(t)) * 2;
        out[0] = m10 / d;
        out[1] = 0.5f / d;
        out[2] = m21 / d;
        out[3] = m20 / d;
    }
    else
    {
        t = 1.0f + mz - mx - my;
        d = (t * InvSqrt(t)) * 2;
        out[0] = m20 / d;
        out[1] = m21 / d;
        out[2] = 0.5f / d;
        out[3] = m10 / d;
    }
    
    //
    // normalize quaternion
    // TODO: figure out why InvSqrt produces inaccurate results
    // use sqrtf for now
    //
    d = sqrtf(out[0] * out[0] +
              out[1] * out[1] +
              out[2] * out[2] +
              out[3] * out[3]);
    
    if(d != 0.0f)
    {
        d = 1.0f / d;
        out[0] *= d;
        out[1] *= d;
        out[2] *= d;
        out[3] *= d;
    }
}
