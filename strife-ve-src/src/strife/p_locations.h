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
//    Location database for automap objectives
//    Author: James Haley
//

#ifndef P_LOCATIONS_H__
#define P_LOCATIONS_H__

#include "doomtype.h"
#include "m_fixed.h"
#include "m_dllist.h"
#include "m_qstring.h"

//
// location_t
//
// Represents a location in the locations database for dynamic objectives
// display.
//
typedef struct location_s
{
    dllistitem_t links;    // list links
    qstring_t    name;     // name of location
    int          levelnum; // game map it is on
    fixed_t      x;        // x coordinate
    fixed_t      y;        // y coordinate
    boolean      active;   // if true, should display on the automap
} location_t;

location_t *P_LocationForName(const char *name);
void P_InitLocations(void);
void P_SetLocationActive(const char *name, boolean active);

typedef void (*locationcb_t)(location_t *);
void P_ActiveLocationIterator(locationcb_t callback);
void P_SetLocationsFromScript(char *locname);
void P_SetLocationsFromFile(const char *filepath);
void P_WriteActiveLocations(const char *filepath);

#endif

// EOF

