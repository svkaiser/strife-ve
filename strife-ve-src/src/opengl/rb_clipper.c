//
// Copyright(C) 2003 Tim Stump
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
//    World clipper: handles visibility checks
//

#include <stdlib.h>

#include "r_local.h"
#include "tables.h"
#include "m_fixed.h"
#include "z_zone.h"
#include <math.h>

typedef struct clipnode_s
{
    struct clipnode_s *prev, *next;
    angle_t start, end;
} clipnode_t;


clipnode_t *freelist    = NULL;
clipnode_t *clipnodes   = NULL;
clipnode_t *cliphead    = NULL;

static clipnode_t * RB_Clipnode_NewRange(angle_t start, angle_t end);
static boolean RB_Clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle);
static void RB_Clipper_AddClipRange(angle_t start, angle_t end);
static void RB_Clipper_RemoveRange(clipnode_t * range);
static void RB_Clipnode_Free(clipnode_t *node);

static clipnode_t *RB_Clipnode_GetNew(void)
{
    if(freelist)
    {
        clipnode_t *p = freelist;
        freelist = p->next;
        return p;
    }

    return malloc(sizeof(clipnode_t));
}

static clipnode_t * RB_Clipnode_NewRange(angle_t start, angle_t end)
{
    clipnode_t *c = RB_Clipnode_GetNew();
    c->start = start;
    c->end = end;
    c->next = c->prev=NULL;
    return c;
}

//
// RB_Clipper_SafeCheckRange
//

boolean RB_Clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle)
{
    if(startAngle > endAngle)
        return (RB_Clipper_IsRangeVisible(startAngle, ANG_MAX) ||
                RB_Clipper_IsRangeVisible(0, endAngle));

    return RB_Clipper_IsRangeVisible(startAngle, endAngle);
}

static boolean RB_Clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle)
{
    clipnode_t *ci;
    ci = cliphead;

    if(endAngle == 0 && ci && ci->start == 0)
    {
        return false;
    }

    while(ci != NULL && ci->start < endAngle)
    {
        if(startAngle >= ci->start && endAngle <= ci->end)
        {
            return false;
        }

        ci = ci->next;
    }

    return true;
}

static void RB_Clipnode_Free(clipnode_t *node)
{
    node->next = freelist;
    freelist = node;
}

static void RB_Clipper_RemoveRange(clipnode_t *range)
{
    if(range == cliphead)
    {
        cliphead = cliphead->next;
    }
    else {
        if(range->prev)
        {
            range->prev->next = range->next;
        }

        if(range->next)
        {
            range->next->prev = range->prev;
        }
    }

    RB_Clipnode_Free(range);
}

//
// RB_Clipper_SafeAddClipRange
//

void RB_Clipper_SafeAddClipRange(angle_t startangle, angle_t endangle)
{
    if(startangle > endangle)
    {
        // The range has to added in two parts.
        RB_Clipper_AddClipRange(startangle, ANG_MAX);
        RB_Clipper_AddClipRange(0, endangle);
    }
    else
    {
        // Add the range as usual.
        RB_Clipper_AddClipRange(startangle, endangle);
    }
}

static void RB_Clipper_AddClipRange(angle_t start, angle_t end)
{
    clipnode_t *node, *temp, *prevNode;
    if(cliphead)
    {
        //check to see if range contains any old ranges
        node = cliphead;
        while(node != NULL && node->start < end)
        {
            if(node->start >= start && node->end <= end)
            {
                temp = node;
                node = node->next;
                RB_Clipper_RemoveRange(temp);
            }
            else
            {
                if(node->start <= start && node->end >= end)
                {
                    return;
                }
                else
                {
                    node = node->next;
                }
            }
        }
        //check to see if range overlaps a range (or possibly 2)
        node = cliphead;
        while(node != NULL)
        {
            if(node->start >= start && node->start <= end)
            {
                node->start = start;
                return;
            }
            if(node->end >= start && node->end <= end)
            {
                // check for possible merger
                if(node->next && node->next->start <= end)
                {
                    node->end = node->next->end;
                    RB_Clipper_RemoveRange(node->next);
                }
                else
                {
                    node->end = end;
                }

                return;
            }

            node = node->next;
        }

        //just add range
        node = cliphead;
        prevNode = NULL;

        temp = RB_Clipnode_NewRange(start, end);

        while(node != NULL && node->start < end)
        {
            prevNode = node;
            node = node->next;
        }

        temp->next = node;

        if(node == NULL)
        {
            temp->prev = prevNode;

            if(prevNode)
            {
                prevNode->next = temp;
            }

            if(!cliphead) {
                cliphead = temp;
            }
        }
        else
        {
            if(node == cliphead)
            {
                cliphead->prev = temp;
                cliphead = temp;
            }
            else
            {
                temp->prev = prevNode;
                prevNode->next = temp;
                node->prev = temp;
            }
        }
    }
    else
    {
        temp = RB_Clipnode_NewRange(start, end);
        cliphead = temp;
        return;
    }
}

//
// RB_Clipper_Clear
//

void RB_Clipper_Clear(void)
{
    clipnode_t *node = cliphead;
    clipnode_t *temp;

    while(node != NULL)
    {
        temp = node;
        node = node->next;
        RB_Clipnode_Free(temp);
    }

    cliphead = NULL;
}
