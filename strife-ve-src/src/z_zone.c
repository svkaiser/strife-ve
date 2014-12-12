//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
//	Zone Memory Allocation. Neat.
//      haleyjd: Yeah, no. Replaced with native heap implementation.
//

#include <stdlib.h>
#include <string.h>

#include "z_zone.h"
#include "i_system.h"
#include "doomtype.h"

//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
// 
 
#define MEM_ALIGN sizeof(void *)
#define ZONEID	0x1d4a11

typedef struct memblock
{
    unsigned int id;
    struct memblock *next, **prev;
    size_t size;
    void **user;
    unsigned char tag;
} memblock_t;

static const size_t header_size = (sizeof(memblock_t) + 15) & ~15;

static memblock_t *blockbytag[PU_NUM_TAGS];

//
// Z_Init
//
void Z_Init(void)
{
}


//
// Z_Free
//
void Z_Free(void* ptr)
{
    memblock_t *block = (memblock_t *)((byte *)ptr - header_size);
    
    if(!ptr)
        I_Error("Z_Free: freed a NULL pointer");

    if(block->id != ZONEID)
        I_Error("Z_Free: freed a pointer without ZONEID");

    // nullify id so another free fails
    block->id = 0;

    if(block->tag == PU_FREE || block->tag >= PU_NUM_TAGS)
        I_Error("Z_Free: freed a pointer with invalid tag");

    // mark freed
    block->tag = PU_FREE;

    // nullify user if one exists
    if(block->user)
        *block->user = NULL;

    // unlink block
    if((*block->prev = block->next))
        block->next->prev = block->prev;

    free(block);
}

//
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//
void *Z_Malloc(int size, int tag, void **user)
{
    memblock_t *block;
    byte       *ret;

    if(tag >= PU_PURGELEVEL && !user)
        I_Error("Z_Malloc: an owner is required for purgable blocks");

    if(!size)
        size = 32; // vanilla compat

    if(!(block = (memblock_t *)(malloc(size + header_size))))
    {
        if(blockbytag[PU_CACHE])
        {
            Z_FreeTags(PU_CACHE, PU_CACHE);
            block = (memblock_t *)(malloc(size + header_size));
        }
    }

    if(!block)
        I_Error("Z_Malloc: failed on allocation of %u bytes", (unsigned int)size);

    block->size = size;

    if((block->next = blockbytag[tag]))
        block->next->prev = &block->next;
    blockbytag[tag] = block;
    block->prev = &blockbytag[tag];

    block->id   = ZONEID;
    block->tag  = tag;
    block->user = user;

    ret = ((byte *)block + header_size);
    if(user)
        *user = ret;

    return ret;
}

//
// Z_Calloc
//
// haleyjd 20140816: [SVE] Sorely needed.
//
void *Z_Calloc(int n1, int n2, int tag, void **user)
{
    int size = n1*n2;
    return memset(Z_Malloc(size, tag, user), 0, size);
}

//
// Z_Realloc
//
// haleyjd 20140816: [SVE] Necessary to remove static limits
//
void *Z_Realloc(void *ptr, int size, int tag, void **user)
{
    void *p;
    memblock_t *block, *newblock, *origblock;
    size_t origsize;

    // if not allocated at all, defer to Z_Malloc
    if(!ptr)
        return Z_Calloc(1, size, tag, user);

    // also defer for a 0 byte request
    if(size == 0)
    {
        Z_Free(ptr);
        return Z_Calloc(1, size, tag, user);
    }

    block = origblock = (memblock_t *)((byte *)ptr - header_size);

    if(block->id != ZONEID)
        I_Error("Z_Realloc: reallocated a block without ZONEID");

    origsize = block->size;

    // nullify current user, if any
    if(block->user)
        *(block->user) = NULL;

    // detach from list before reallocation
    if((*block->prev = block->next))
        block->next->prev = block->prev;

    block->next = NULL;
    block->prev = NULL;

    if(!(newblock = (memblock_t *)(realloc(block, size + header_size))))
    {
        if(blockbytag[PU_CACHE])
        {
            Z_FreeTags(PU_CACHE, PU_CACHE);
            newblock = (memblock_t *)(realloc(block, size + header_size));
        }
    }

    if(!(block = newblock))
    {
        if(origblock->size >= size)
        {
            // restore original alloc if shrinking realloc fails
            block = origblock;
            size = block->size;
        }
        else
            I_Error("Z_Realloc: failed on allocation of %u bytes", (unsigned int)size);
    }

    block->size = size;
    block->tag  = tag;

    if(size > origsize)
        memset((byte *)block + header_size + origsize, 0, size - origsize);

    p = (byte *)block + header_size;

    // set new user, if any
    block->user = user;
    if(user)
        *user = p;

    // reattach to list at possibly new address, new tag
    if((block->next = blockbytag[tag]))
        block->next->prev = &block->next;
    blockbytag[tag] = block;
    block->prev = &blockbytag[tag];

    return p;
}

//
// Z_FreeTags
//
void Z_FreeTags(int lowtag, int	hightag)
{
    memblock_t *block;

    if(lowtag <= PU_FREE)
        lowtag = PU_FREE + 1;

    if(hightag > PU_CACHE)
        hightag = PU_CACHE;

    for(; lowtag <= hightag; lowtag++)
    {
        for(block = blockbytag[lowtag], blockbytag[lowtag] = NULL; block; )
        {
            memblock_t *next = block->next;

            if(block->id != ZONEID)
                I_Error("Z_FreeTags: freed a block without ZONEID");

            Z_Free((byte *)block + header_size);
            block = next;
        }
    }
}

//
// Z_CheckHeap
//
void Z_CheckHeap(void)
{
    memblock_t *block;
    int lowtag;

    for(lowtag = PU_FREE + 1; lowtag < PU_NUM_TAGS; lowtag++)
    {
        for(block = blockbytag[lowtag]; block; block = block->next)
        {
            if(block->id != ZONEID)
                I_Error("Z_CheckHeap: block found without ZONEID");
        }
    }
}

//
// Z_ChangeTag
//
void Z_ChangeTag2(void *ptr, int tag, char *file, int line)
{
    memblock_t *block;

    if(!ptr)
        I_Error("Z_ChangeTag: can't change a NULL pointer at %s:%d", file, line);

    block = (memblock_t *)((byte *)ptr - header_size);

    if(block->id != ZONEID)
        I_Error("Z_ChangeTag: changed a tag without ZONEID");

    if(tag >= PU_PURGELEVEL && !block->user)
        I_Error("Z_ChangeTag: an owner is required for purgable blocks");

    if((*block->prev = block->next))
        block->next->prev = block->prev;
    if((block->next = blockbytag[tag]))
        block->next->prev = &block->next;
    block->prev = &blockbytag[tag];
    blockbytag[tag] = block;

    block->tag = tag;
}

// EOF


