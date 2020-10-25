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
//    Draw Lists
//

#include <stdlib.h>

#include "rb_main.h"
#include "rb_drawlist.h"
#include "rb_data.h"
#include "rb_draw.h"
#include "rb_things.h"
#include "rb_config.h"
#include "i_system.h"
#include "z_zone.h"

drawlist_t drawlist[NUMDRAWLISTS];

//
// DL_AddVertexList
//

vtxlist_t *DL_AddVertexList(drawlist_t *dl)
{
    vtxlist_t *list;

    // exceeded max capacity?
    if(dl->index == dl->max)
    {
        vtxlist_t *old = dl->list;
        vtxlist_t *newlist;

        // expand stack
        dl->max += 128;

        // allocate new array
        newlist = (vtxlist_t*)Z_Calloc(dl->max, sizeof(vtxlist_t), PU_LEVEL, NULL);
        memcpy(newlist, old, dl->index * sizeof(vtxlist_t));

        dl->list = newlist;
        Z_Free(old);
    }

    list = &dl->list[dl->index];

    list->flags = 0;
    list->texid = 0;
    list->params = 0;
    list->drawTag = dl->drawTag;

    return &dl->list[dl->index++];
}

//
// SortTranWalls
//

static int SortTranWalls(const void *a, const void *b)
{
    vtxlist_t *xa = (vtxlist_t*)a;
    vtxlist_t *xb = (vtxlist_t*)b;

    return xb->fparams - xa->fparams;
}

//=============================================================================
//
// Sorting
//
// haleyjd 20140919: [SVE] mergesort, even a relatively naive implementation,
// is an order of magnitude faster than libc qsort for sprites.
//
//=============================================================================

//
// merge_sprite
//

void merge_sprite(vtxlist_t *list, vtxlist_t *aux, int left, int right, int rightEnd)
{
    int i, num, temp, leftEnd = right - 1;
    temp = left;
    num = rightEnd - left + 1;

    while(left <= leftEnd && right <= rightEnd)
    {
        rbVisSprite_t *al = list[left].data;
        rbVisSprite_t *ar = list[right].data;

        if(al->dist > ar->dist)
        {
            aux[temp++] = list[left++];
        }
        else
        {
            aux[temp++] = list[right++];
        }
    }
    while(left <= leftEnd)
    {
        aux[temp++] = list[left++];
    }
    while(right <= rightEnd)
    {
        aux[temp++] = list[right++];
    }
    for(i = 1; i <= num; i++, rightEnd--)
    {
        list[rightEnd] = aux[rightEnd];
    }
}

//
// msort_sprite
//

void msort_sprite(vtxlist_t *list, vtxlist_t *temp, int left, int right)
{
    int center;

    if(left < right)
    {
        center = (left+right) / 2;
        msort_sprite(list, temp, left, center);
        msort_sprite(list, temp, center+1, right);
        merge_sprite(list, temp, left, center+1, right);
    }
}

//
// merge_wall
//
// Also for regular walls, it's worth it.
//

void merge_wall(vtxlist_t *list, vtxlist_t *aux, int left, int right, int rightEnd)
{
    int i, num, temp, leftEnd = right - 1;
    temp = left;
    num = rightEnd - left + 1;

    while(left <= leftEnd && right <= rightEnd)
    {
        if(list[left].texid > list[right].texid)
        {
            aux[temp++] = list[left++];
        }
        else
        {
            aux[temp++] = list[right++];
        }
    }
    while(left <= leftEnd)
    {
        aux[temp++] = list[left++];
    }
    while(right <= rightEnd)
    {
        aux[temp++] = list[right++];
    }
    for(i = 1; i <= num; i++, rightEnd--)
    {
        list[rightEnd] = aux[rightEnd];
    }
}

//
// msort_wall
//

void msort_wall(vtxlist_t *list, vtxlist_t *temp, int left, int right)
{
    if(left < right)
    {
        int center = (left+right) / 2;
        msort_wall(list, temp, left, center);
        msort_wall(list, temp, center+1, right);
        merge_wall(list, temp, left, center+1, right);
    }
}

//
// End sorting
//
//=============================================================================

//
// DL_ProcessDrawList
//

void DL_ProcessDrawList(int tag)
{
    drawlist_t* dl;
    int i;
    int drawcount;
    vtxlist_t* head;
    vtxlist_t* tail;

    if(tag < 0 && tag >= NUMDRAWLISTS)
    {
        return;
    }

    dl = &drawlist[tag];
    drawcount = 0;

    if(dl->max > 0)
    {
        if(tag != DLT_DYNLIGHT)
        {
            if(tag != DLT_SPRITE && tag != DLT_SPRITEALPHA && tag != DLT_SPRITEOUTLINE)
            {
                if(tag == DLT_TRANSWALL)
                {
                    qsort(dl->list, dl->index, sizeof(vtxlist_t), SortTranWalls);
                }
                else
                {
                    vtxlist_t *temp = malloc(dl->index * sizeof(vtxlist_t));
                    msort_wall(dl->list, temp, 0, dl->index - 1);
                    free(temp);
                }
            }
            else
            {
                vtxlist_t *temp = malloc(dl->index * sizeof(vtxlist_t));
                msort_sprite(dl->list, temp, 0, dl->index - 1);
                free(temp);
            }
        }
        
        tail = &dl->list[dl->index];

        for(i = 0; i < dl->index; ++i)
        {
            vtxlist_t *rover;

            head = &dl->list[i];

            // break if no data found in list
            if(!head->data)
            {
                break;
            }

            if(head->procfunc)
            {
                if(!head->procfunc(head, &drawcount))
                {
                    rover = &dl->list[i+1];
                    continue;
                }
            }

            rover = &dl->list[i+1];

            if(tag != DLT_SPRITE)
            {
                if(rover != tail)
                {
                    if(head->texid == rover->texid)
                    {
                        continue;
                    }
                }
            }

            if(head->preprocess)
            {
                head->preprocess(head);
            }
            else if(tag != DLT_DYNLIGHT)
            {
                rbDataType_t dataType = RDT_INVALID;
                rbTexture_t *texture;

                // setup texture ID
                switch(tag)
                {
                case DLT_WALL:
                case DLT_MASKEDWALL:
                case DLT_TRANSWALL:
                    dataType = RDT_COLUMN;
                    break;

                case DLT_FLAT:
                case DLT_AMAP:
                    dataType = RDT_FLAT;
                    break;

                case DLT_SPRITE:
                case DLT_SPRITEALPHA:
                case DLT_SPRITEBRIGHT:
                case DLT_SPRITEOUTLINE:
                    dataType = RDT_SPRITE;
                    break;

                case DLT_DECAL:
                    dataType = RDT_PATCH;
                    break;

                default:
                    break;
                }

                texture = RB_GetTexture(dataType, head->texid, 0);

                if(texture)
                {
                    RB_BindTexture(texture);
                    RB_ChangeTexParameters(texture, TC_REPEAT, TEXFILTER);
                }
            }

            RB_DrawElements();

            if(head->postprocess)
            {
                head->postprocess(head, &drawcount);
            }

            RB_ResetElements();
            
            rbState.numDrawnVertices += drawcount;
            drawcount = 0;
        }
    }
}

//
// DL_GetDrawListSize
//

int DL_GetDrawListSize(int tag)
{
    int i;

    for(i = 0; i < NUMDRAWLISTS; ++i)
    {
        drawlist_t *dl;

        if(i != tag)
        {
            continue;
        }

        dl = &drawlist[i];
        return dl->max;
    }

    return 0;
}

//
// DL_BeginDrawList
//

void DL_BeginDrawList(void)
{
    int i;

    for(i = 0; i < NUMDRAWLISTS; ++i)
    {
        drawlist[i].index = 0;
    }

    RB_BindDrawPointers(drawVertex);
}

//
// DL_Reset
//

void DL_Reset(int tag)
{
    vtxlist_t* head;
    drawlist_t* dl;
    
    if(tag < 0 && tag >= NUMDRAWLISTS)
    {
        return;
    }
    
    dl = &drawlist[tag];
    
    if(dl->max > 0)
    {
        int i;
        
        for(i = 0; i < dl->index; ++i)
        {
            head = &dl->list[i];
            head->data = NULL;
        }
    }
}

//
// DL_Init
// Intialize draw lists
//

void DL_Init(void)
{
    drawlist_t *dl;
    int i;

    for(i = 0; i < NUMDRAWLISTS; ++i)
    {
        dl = &drawlist[i];

        dl->index   = 0;
        dl->max     = 128;
        dl->list    = Z_Calloc(1, sizeof(vtxlist_t) * dl->max, PU_LEVEL, 0);
        dl->drawTag = i;
    }
}

