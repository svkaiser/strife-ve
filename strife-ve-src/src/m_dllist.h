// Emacs style mode select   -*- C -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
// Copyright(C) 2014 Night Dive Studios, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  Generalized Double-linked List Routines
//    
//-----------------------------------------------------------------------------

#ifndef M_DLLIST_H__
#define M_DLLIST_H__

#if defined(__GNUC__)
#define dlinline __inline__
#elif defined(_MSC_VER)
#define dlinline __inline
#else
#define dlinline
#endif

typedef struct dllistitem_s
{
   struct dllistitem_s  *next;
   struct dllistitem_s **prev;
   void                 *object; // 08/02/09: pointer back to object
   unsigned int          data;   // 02/07/10: arbitrary data cached at node
} dllistitem_t;

dlinline static void M_DLListInsert(dllistitem_t *item, void *object, dllistitem_t **head)
{
   dllistitem_t *next = *head;

   if((item->next = next))
      next->prev = &item->next;
   item->prev = head;
   *head = item;

   item->object = object;
}

dlinline static void M_DLListRemove(dllistitem_t *item)
{
   dllistitem_t **prev = item->prev;
   dllistitem_t *next  = item->next;
   
   if((*prev = next))
      next->prev = prev;
}

#endif

// EOF

