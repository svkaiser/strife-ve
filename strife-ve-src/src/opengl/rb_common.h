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

#ifndef __RB_COMMON_H__
#define __RB_COMMON_H__

//
// most of this stuff should probably go into a math module or something...
//

#define TRUEANGLES(x)   (((x) >> ANGLETOFINESHIFT) * 360.0f / FINEANGLES)

#define FLOAT2FIXED(x)  ((fixed_t)((x)*FRACUNIT))
#define FIXED2FLOAT(x)  (((float)(x))/FRACUNIT)
#define FLOATSIGNBIT(f) ((const unsigned int)(f) >> 31)

#define MAX_COORD 32767.0f

#ifndef M_PI
#define M_PI    3.1415926535897932384626433832795
#endif

#define M_RAD   (M_PI / 180.0f)
#define M_DEG   (180.0f / M_PI)

#define DEG2RAD(x) ((x) * M_RAD)
#define RAD2DEG(x) ((x) * M_DEG)

#ifndef BIT
#define BIT(num) (1<<(num))
#endif

#define D_RGBA(r,g,b,a) ((unsigned int)((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))

#if defined(__APPLE__)
typedef void* rhandle;
#else
typedef unsigned int rhandle;
#endif
typedef unsigned int dtexture;

// there's too many of these inline defines floating around......
#if defined(__GNUC__)
#define dinline __inline__
#elif defined(_MSC_VER)
#define dinline __inline
#else
#define dinline
#endif

static dinline float InvSqrt(float x)
{
    unsigned int i;
    float r;
    float y;
    
    y = x * 0.5f;
    i = *(unsigned int*)&x;
    i = 0x5f3759df - (i >> 1);
    r = *(float*)&i;
    r = r * (1.5f - r * r * y);
    
    return r;
}

#endif
