//
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
//     Kerning data for big font
//

#ifndef KERNING_H__
#define KERNING_H__

typedef struct kerndata_s
{
    char first;
    char second; 
    int  offset;
} kerndata_t;

extern kerndata_t kernings[];
extern size_t     numkernings;

#endif

// EOF

