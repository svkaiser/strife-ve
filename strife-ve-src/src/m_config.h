//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
//      Configuration file interface.
//    


#ifndef __M_CONFIG__
#define __M_CONFIG__

#include "doomtype.h"

// [SVE]
enum
{
    M_CFG_SETCURRENT = 0x01, // set current value
    M_CFG_SETDEFAULT = 0x02, // set default value

    M_CFG_SETALL     = M_CFG_SETCURRENT | M_CFG_SETDEFAULT
};

void M_LoadDefaults(void);
void M_SaveDefaults(void);
void M_SaveDefaultsAlternate(char *main, char *extra);
void M_SetConfigDir(char *dir);
void M_BindVariable(char *name, void *variable);
boolean M_SetVariable(const char *name, const char *value);
int M_GetIntVariable(const char *name);
const char *M_GetStrVariable(char *name);
float M_GetFloatVariable(const char *name);
void M_SetConfigFilenames(char *main_config, char *extra_config);
char *M_GetSaveGameDir(char *iwadname);

extern char *configdir;

//
// [SVE] svillarreal
//
extern boolean config_fresh;
extern boolean extra_config_fresh;

const char *GetNameForKey(const int key);
const int GetKeyForName(const char *name);

// [SVE] haleyjd: new config functions
void M_BindVariableWithDefault(char *name, void *location, void *default_location);
boolean M_SetVariableByFlags(const char *name, const char *value, int flags);
int M_GetIntVariableDefault(const char *name);
const char *M_GetStrVariableDefault(const char *name);
float M_GetFloatVariableDefault(const char *name);
char *M_GetTmpSaveGameDir(char *iwadname);

#endif
