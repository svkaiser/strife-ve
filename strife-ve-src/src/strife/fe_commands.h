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
//    Configuration and netgame negotiation frontend for
//    Strife: Veteran Edition
//
// AUTHORS:
//    James Haley
//    Samuel Villarreal (Hi-Res BG code, mouse pointer)
//

#ifndef FE_COMMANDS_H_
#define FE_COMMANDS_H_

const char *FE_GetHelpForVar(const char *var);
char *FE_GetFormattedHelpStr(const char *var, int x, int y);

void FE_ExecCmd(const char *verb);

// variable types
enum
{
    FE_VAR_INT,
    FE_VAR_FLOAT,
    FE_VAR_INT_PO2
};

// variable structure
typedef struct fevar_s
{
    const char *name;
    int         type;
    int         min;
    int         max;
    int         istep;
    float       fmin;
    float       fmax;
    float       fstep;
    void        (*SetFunc)(int);
} fevar_t;

fevar_t *FE_VariableForName(const char *name);
boolean FE_IncrementVariable(const char *name);
boolean FE_DecrementVariable(const char *name);

const char *FE_CurrentVideoMode(void);
void FE_NextVideoMode(void);
void FE_PrevVideoMode(void);

const char *FE_NameForValue(const char *valuevar);
int FE_IncrementValue(const char *valuevar);
int FE_DecrementValue(const char *valuevar);

void FE_BindMusicTestVar(void);
void FE_MusicTestSaveCurrent(void);
void FE_MusicTestRestoreCurrent(void);
void FE_MusicTestPlaySelection(void);

#endif

// EOF

