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
// Steamworks Networking Interface
//

#include "SDL.h"

#include "z_zone.h"
#include "i_system.h"
#include "m_misc.h"

#include "net_defs.h"
#include "net_io.h"
#include "net_packet.h"
#include "net_sdl.h"
#include "net_steamworks.h"

#include "i_social.h"
#include <stdlib.h>

//
// Globals
//

boolean  net_SteamGame;     // a Steam-negotiated netgame is under way
int      net_SteamNodeType; // either NET_STEAM_CLIENT or NET_STEAM_SERVER
int      net_SteamNumNodes; // number of expected nodes
char    *net_SteamServerID; // ID of server if we are a client

//=============================================================================
//
// Steamworks Net Module
//

#ifdef I_APPSERVICES_NETWORKING

typedef struct
{
    net_addr_t net_addr;
    uint64_t   csid;
} addrpair_t;

static addrpair_t **addr_table;
static int addr_table_size = -1;

//
// Initializes the address table
//
static void NET_Steamworks_InitAddrTable(void)
{
    addr_table_size = 16;

    addr_table = Z_Malloc(sizeof(addrpair_t *) * addr_table_size,
                          PU_STATIC, 0);
    memset(addr_table, 0, sizeof(addrpair_t *) * addr_table_size);
}

//
// Finds an address by searching the table.  If the address is not found,
// it is added to the table.
//
static net_addr_t *NET_Steamworks_FindAddress(uint64_t csid)
{
    addrpair_t *new_entry;
    int empty_entry = -1;
    int i;

    if(addr_table_size < 0)
        NET_Steamworks_InitAddrTable();

    for(i = 0; i < addr_table_size; i++)
    {
        if(addr_table[i] != NULL &&
           csid == addr_table[i]->csid)
        {
            return &addr_table[i]->net_addr;
        }

        if(empty_entry < 0 && addr_table[i] == NULL)
            empty_entry = i;
    }

    // Was not found in list.  We need to add it.

    // Is there any space in the table? If not, increase the table size

    if (empty_entry < 0)
    {
        addrpair_t **new_addr_table;
        int new_addr_table_size;

        // after reallocing, we will add this in as the first entry
        // in the new block of memory

        empty_entry = addr_table_size;
        
        // allocate a new array twice the size, init to 0 and copy 
        // the existing table in.  replace the old table.

        new_addr_table_size = addr_table_size * 2;
        new_addr_table = Z_Malloc(sizeof(addrpair_t *) * new_addr_table_size,
                                  PU_STATIC, 0);
        memset(new_addr_table, 0, sizeof(addrpair_t *) * new_addr_table_size);
        memcpy(new_addr_table, addr_table, 
               sizeof(addrpair_t *) * addr_table_size);
        Z_Free(addr_table);
        addr_table = new_addr_table;
        addr_table_size = new_addr_table_size;
    }

    // Add a new entry
    
    new_entry = Z_Malloc(sizeof(addrpair_t), PU_STATIC, 0);

    new_entry->csid = csid;
    new_entry->net_addr.handle = &new_entry->csid;
    new_entry->net_addr.module = &net_steamworks_module;

    addr_table[empty_entry] = new_entry;

    return &new_entry->net_addr;
}

//
// Free an address
//
static void NET_Steamworks_FreeAddress(net_addr_t *addr)
{
    int i;
    
    for(i = 0; i < addr_table_size; i++)
    {
        if(addr == &addr_table[i]->net_addr)
        {
            Z_Free(addr_table[i]);
            addr_table[i] = NULL;
            return;
        }
    }

    I_Error("NET_Steamworks_FreeAddress: Attempted to remove an unused address!");
}

//
// Initialize client
//
static boolean NET_Steamworks_InitClient(void)
{
    // TODO: any sort of verification against the service provider?
    return true;
}

//
// Initialize server
//
static boolean NET_Steamworks_InitServer(void)
{
    // TODO: any sort of verification against the service provider?
    return true;
}

//
// Send a packet
//
static void NET_Steamworks_SendPacket(net_addr_t *addr, net_packet_t *packet)
{
    uint64_t csid = *((uint64_t *)addr->handle);
    if(!gAppServices->SendPacket(&csid, packet->data, (unsigned int)packet->len))
        I_Error("NET_Steamworks_SendPacket: Error transmitting packet");
}

//
// Receive a packet
//
static boolean NET_Steamworks_RecvPacket(net_addr_t **addr, net_packet_t **packet)
{
    uint64_t     csid = 0;
    void        *data = NULL;
    unsigned int size  = 0;
    
    gAppServices->RecvPacket(&csid, &data, &size);

    // no packets received
    if(!csid || !data || !size)
        return false;

    // Put the data into a new packet structure
    *packet = NET_NewPacket((int)size);
    memcpy((*packet)->data, data, size);
    (*packet)->len = size;

    // Address
    *addr = NET_Steamworks_FindAddress(csid);

    return true;
}

//
// Translate an address to string
//
void NET_Steamworks_AddrToString(net_addr_t *addr, char *buffer, int buffer_len)
{
    uint64_t csid = *((uint64_t *)addr->handle);
    M_snprintf(buffer, buffer_len, "%llu", csid);
}

//
// Resolve a string to an address
//
net_addr_t *NET_Steamworks_ResolveAddress(char *address)
{
    char* end;
    uint64_t addr = (uint64_t)(strtoull(address, &end, 10));

    if(!gAppServices->ResolveAddress(&addr))
    {
        // unable to resolve
        return NULL;
    }
    else
    {
        return NET_Steamworks_FindAddress(addr);
    }
}

// Complete module

net_module_t net_steamworks_module =
{
    NET_Steamworks_InitClient,
    NET_Steamworks_InitServer,
    NET_Steamworks_SendPacket,
    NET_Steamworks_RecvPacket,
    NET_Steamworks_AddrToString,
    NET_Steamworks_FreeAddress,
    NET_Steamworks_ResolveAddress,
};

#endif

// EOF

