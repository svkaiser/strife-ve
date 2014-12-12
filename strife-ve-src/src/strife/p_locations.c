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

#include <ctype.h>
#include <string.h>

#include "z_zone.h"
#include "doomstat.h"
#include "m_misc.h"
#include "m_saves.h"
#include "p_locations.h"
#include "w_wad.h"

// locations hash table
#define NUM_LOC_CHAINS 127
static dllistitem_t *locations[NUM_LOC_CHAINS];

//
// SDBMHash
//
// haleyjd 20140823: [SVE] Need a good string hash function. This is one
// of the best (EE also uses this algorithm).
//
unsigned int SDBMHash(const char *str)
{
    const unsigned char *ustr = (const unsigned char *)str;
    int c;
    unsigned int h = 0;

    while((c = *ustr++))
        h = toupper(c) + (h << 6) + (h << 16) - h;

    return h;
}

//
// P_addLocationToHash
//
// Put a location into the hash table.
//
static void P_addLocationToHash(location_t *loc)
{
    unsigned int hash = SDBMHash(QStrConstPtr(&loc->name));
    M_DLListInsert(&loc->links, loc, &locations[hash % NUM_LOC_CHAINS]);
}

//
// P_newLocation
//
// Create a new location object
//
static location_t *P_newLocation(const qstring_t *name, int levelnum, int x, int y)
{
    location_t *newLoc = Z_Calloc(1, sizeof(location_t), PU_STATIC, NULL);
    
    QStrQCopy(QStrCreate(&newLoc->name), name);
    newLoc->levelnum = levelnum;
    newLoc->x = x * FRACUNIT;
    newLoc->y = y * FRACUNIT;
    newLoc->active = false;
    P_addLocationToHash(newLoc);

    return newLoc;
}

// tokenizer state enumeration
enum
{
    TSTATE_SCAN,    // scan for start of token
    TSTATE_TOKEN,   // inside token
    TSTATE_COMMENT, // scanning through comment
    TSTATE_DONE     // end of token
};

enum
{
    TOKEN_NONE,   // not yet determined
    TOKEN_STRING, // string token
    TOKEN_EOF     // at end of input
};

// tokenizer state data
typedef struct tokenstate_s
{
    qstring_t  *token;     // destination token
    int         state;     // current tokenizer state
    const char *input;     // base input string pointer
    int         idx;       // index into input
    int         tokentype; // type of token found
} tokenstate_t;

// Tokenizer state handlers

// Looking for start of token
static void Loc_doStateScan(tokenstate_t *ts)
{
    char c = ts->input[ts->idx];

    switch(c)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        // whitespace; remain in this state
        break;
    case '\0':
        // end of input
        ts->tokentype = TOKEN_EOF;
        ts->state     = TSTATE_DONE;
        break;
    case '#':
        // start of comment
        ts->state = TSTATE_COMMENT;
        break;
    default:
        // anything else is start of a token
        ts->tokentype = TOKEN_STRING;
        ts->state     = TSTATE_TOKEN;
        QStrPutc(ts->token, c);
        break;
    }
}

// Scanning inside token
static void Loc_doStateToken(tokenstate_t *ts)
{
    char c = ts->input[ts->idx];

    switch(c)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        // whitespace; end of token
        ts->state = TSTATE_DONE;
        break;
    case '\0':
    case '#':
        // end of input -or- start of a comment
        --ts->idx; // back up one
        ts->state = TSTATE_DONE;
        break;
    default:
        // anything else continues the current token
        QStrPutc(ts->token, c);
        break;
    }
}

// Reading out a single-line comment
static void Loc_doStateComment(tokenstate_t *ts)
{
    char c = ts->input[ts->idx];

    if(c == '\n')
        ts->state = TSTATE_SCAN; // go back to scanning
    else if(c == '\0')
    {
        ts->tokentype = TOKEN_EOF;
        ts->state     = TSTATE_DONE;
    }
}

typedef void (*tokenhandler_t)(tokenstate_t *);

// tokenizer state table
static tokenhandler_t tokenHandlers[] =
{
    Loc_doStateScan,   // TSTATE_SCAN
    Loc_doStateToken,  // TSTATE_TOKEN
    Loc_doStateComment // TSTATE_COMMENT
};

//
// P_nextLocationToken
//
// LOCATION lump tokenizer
//
static void P_nextLocationToken(tokenstate_t *ts)
{
    QStrClear(ts->token);
    ts->state     = TSTATE_SCAN;
    ts->tokentype = TOKEN_NONE;

    // already at end of input?
    if(ts->input[ts->idx] != '\0')
    {
        while(ts->state != TSTATE_DONE)
        {
            (tokenHandlers[ts->state])(ts);
            ++ts->idx;
        }
    }
    else
        ts->tokentype = TOKEN_EOF;
}

static void P_initLocationTokenizer(tokenstate_t *ts, 
                                    const char *input,
                                    qstring_t *token)
{
    ts->tokentype = TOKEN_NONE;
    ts->state     = TSTATE_SCAN;
    ts->input     = input;
    ts->idx       = 0;
    ts->token     = token;
}

// database parser state enumeration
enum
{
    STATE_EXPECTLOCATION,
    STATE_EXPECTLEVEL,
    STATE_EXPECTX,
    STATE_EXPECTY
};

typedef struct locparsestate_s
{
    tokenstate_t *ts;
    int       state;
    qstring_t name;
    int       levelnum;
    int       x;
    int       y;
} locparsestate_t;

// Location parser state handlers

// Expecting location name, which is the primary key
static void LocParse_doStateLocation(locparsestate_t *ps)
{
    if(ps->ts->tokentype == TOKEN_STRING)
    {
        QStrQCopy(&ps->name, ps->ts->token);
        ps->state = STATE_EXPECTLEVEL;
    }
}

// Expecting levelnum
static void LocParse_doStateLevelNum(locparsestate_t *ps)
{
    if(ps->ts->tokentype == TOKEN_STRING)
    {
        ps->levelnum = QStrAtoi(ps->ts->token);
        ps->state = STATE_EXPECTX;
    }
}

// Expecting x coordinate
static void LocParse_doStateX(locparsestate_t *ps)
{
    if(ps->ts->tokentype == TOKEN_STRING)
    {
        ps->x = QStrAtoi(ps->ts->token);
        ps->state = STATE_EXPECTY;
    }
}

// Expecting y coordinate
static void LocParse_doStateY(locparsestate_t *ps)
{
    if(ps->ts->tokentype == TOKEN_STRING)
    {
        ps->y = QStrAtoi(ps->ts->token);
        ps->state = STATE_EXPECTLOCATION;

        // create the new location
        P_newLocation(&ps->name, ps->levelnum, ps->x, ps->y);
        QStrClear(&ps->name);
        ps->levelnum = 0;
        ps->x = ps->y = 0;
    }
}

typedef void (*lpstatehandler_t)(locparsestate_t *);

// parser state table
static lpstatehandler_t locParseHandlers[] =
{
    LocParse_doStateLocation,
    LocParse_doStateLevelNum,
    LocParse_doStateX,
    LocParse_doStateY
};

//
// P_parseLocationDB
//
// Parse the LOCATION lump, which contains a set of records which have the
// following format:
//
//   <name> <level> <x> <y>
//
// Name is a unique hash key for the location. Level is the map that location
// exists on, and (x, y) is the actual location itself.
//
static void P_parseLocationDB(void)
{
    qstring_t    token;
    tokenstate_t tokenizer;
    int          lumpnum;
    int          lumplen;
    char        *buf;
    
    locparsestate_t ps;

    // load LOCATION lump into a string
    if((lumpnum = W_CheckNumForName("LOCATION")) < 0)
        return;

    lumplen = W_LumpLength(lumpnum);
    buf     = Z_Calloc(lumplen + 1, sizeof(char), PU_STATIC, NULL);
    W_ReadLump(lumpnum, buf);

    // initialize tokenizer
    QStrInitCreate(&token);
    P_initLocationTokenizer(&tokenizer, buf, &token);

    // init parser state
    QStrInitCreate(&ps.name);
    ps.levelnum = 0;
    ps.x = ps.y = 0;
    ps.ts = &tokenizer;
    ps.state = STATE_EXPECTLOCATION;

    while(tokenizer.tokentype != TOKEN_EOF)
    {
        P_nextLocationToken(&tokenizer);
        (locParseHandlers[ps.state])(&ps);
    }

    // dispose of input, token, and name buffers
    Z_Free(buf);
    QStrFree(&token);
    QStrFree(&ps.name);
}

//
// P_LocationForName
//
// Retrieve a location structure for its name. Returns NULL if the location
// does not exist.
//
location_t *P_LocationForName(const char *name)
{
    dllistitem_t *chain = locations[SDBMHash(name) % NUM_LOC_CHAINS];

    while(chain && 
          QStrCaseCmp(&((location_t *)(chain->object))->name, name))
    {
        chain = chain->next;
    }

    return chain ? (location_t *)(chain->object) : NULL;
}

//
// P_InitLocations
//
// haleyjd 20140823: [SVE] Initialize everything pertaining to the locations
// database used for dynamic display of objective locations.
//
void P_InitLocations(void)
{
    // Parse and load the locations database
    P_parseLocationDB();
}

//
// P_SetLocationActive
//
// Convenience routine to set the active state of a location by name.
//
void P_SetLocationActive(const char *name, boolean active)
{
    location_t *loc;

    if((loc = P_LocationForName(name)))
        loc->active = active;
}

//
// P_ActiveLocationIterator
//
// Run the provided callback function for each location.
//
void P_ActiveLocationIterator(locationcb_t callback)
{
    int i;

    // walk all hash chains
    for(i = 0; i < NUM_LOC_CHAINS; i++)
    {
        dllistitem_t *chain = locations[i];

        // walk down the chain
        while(chain)
        {
            location_t *loc = chain->object;
            // if location is active, dispatch the callback
            if(loc->active)
                callback(loc);
            chain = chain->next;
        }
    }
}

//
// P_SetAllLocations
//
// Change the state of all locations
//
void P_SetAllLocations(boolean active)
{
    int i;

    // walk all hash chains
    for(i = 0; i < NUM_LOC_CHAINS; i++)
    {
        dllistitem_t *chain = locations[i];

        // walk down the chain
        while(chain)
        {
            location_t *loc = chain->object;
            loc->active = active;
            chain = chain->next;
        }
    }
}

typedef struct locstate_s
{
    qstring_t *token;
    char      *input;
    int        idx;
} locstate_t;

typedef enum locret_e
{
    LOC_TOKEN,
    LOC_EOF
} locret_t;

//
// P_getLocScriptLine
//
// Extract one line of a LOC lump which tells which location(s) to turn off
// or on when a corresponding LOG is loaded as the objective.
//
static locret_t P_getLocScriptLine(locstate_t *ls)
{
    char c;

    while(1)
    {
        c = ls->input[ls->idx];
        switch(c)
        {
        case '\0': // end of input
            return LOC_EOF;
        case '\r':
            ls->idx++;
            break;
        case '\n': // end of line
            ls->idx++;
            return LOC_TOKEN;
        default:   // part of token
            ls->idx++;
            QStrPutc(ls->token, c);
            break;
        }
    }
}

//
// P_loadLocScriptLump
//
// haleyjd 20140829: [SVE] Load a LOC script from a lump.
//
static char *P_loadLocScriptLump(char *logname)
{
    char  locname[9];
    int   lumpnum;
    int   size;
    char *buf;

    strncpy(locname, logname, sizeof(locname));
    locname[2] = 'C';

    // load script
    if((lumpnum = W_CheckNumForName(locname)) < 0)
        return NULL;

    size = W_LumpLength(lumpnum);
    buf = Z_Calloc(size+1, sizeof(char), PU_STATIC, NULL);
    W_ReadLump(lumpnum, buf);

    return buf;
}

//
// P_loadLocScriptFile
//
// haleyjd 20140829: [SVE] Load a LOC script from a file.
//
static char *P_loadLocScriptFile(const char *filename)
{
    char *buf = NULL;

    return M_ReadFileAsString(filename, &buf) ? buf : NULL;
}

//
// P_parseLocScript
//
// Parse a given LOC script and turn on/off the listed locations.
//
static void P_parseLocScript(char *input)
{
    qstring_t  qs;
    locstate_t ls;
    
    QStrInitCreate(&qs);
    ls.input = input;
    ls.token = &qs;
    ls.idx   = 0;

    while(1)
    {
        locret_t lr;
        QStrClear(ls.token);
        lr = P_getLocScriptLine(&ls);
        if(QStrLen(ls.token) > 0)
        {
            boolean active;
            const char *name;

            if(QStrCharAt(ls.token, 0) == '+')
                active = true;
            else
                active = false;

            name = QStrBufferAt(ls.token, 1);
            if(!strcasecmp(name, "ALL"))
                P_SetAllLocations(active);
            else
                P_SetLocationActive(name, active);
        }
        if(lr == LOC_EOF)
            break;
    }

    QStrFree(&qs);
    Z_Free(ls.input);
}

void P_SetLocationsFromScript(char *logname)
{
    char *buf;
    
    if(deathmatch) // not in deathmatch.
        return;

    buf = P_loadLocScriptLump(logname);
    if(buf)
        P_parseLocScript(buf);
}

void P_SetLocationsFromFile(const char *filepath)
{
    char *srcfile = M_SafeFilePath(filepath, "mis_loc");
    char *buf     = P_loadLocScriptFile(srcfile);
    if(buf)
        P_parseLocScript(buf);

    Z_Free(srcfile);
}

// file handle for save output
static FILE *saveout;

//
// P_writeActiveLocation
//
// haleyjd: Output an active location to the save file.
//
static void P_writeActiveLocation(location_t *loc)
{
    fprintf(saveout, "+%s\n", QStrConstPtr(&loc->name));
}

//
// P_WriteActiveLocations
//
// haleyjd 20140829: For loading from save games.
//
void P_WriteActiveLocations(const char *filepath)
{
    char *dstfile = M_SafeFilePath(filepath, "mis_loc");

    saveout = fopen(dstfile, "w");
    if(saveout)
    {
       // first line always turns off all active locations
       fputs("-ALL\n", saveout);

       // write out all active locations
       P_ActiveLocationIterator(P_writeActiveLocation);

       fclose(saveout);
       saveout = NULL;
    }

    Z_Free(dstfile);
}

// EOF

