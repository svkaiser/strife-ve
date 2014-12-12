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

#ifndef FE_CHARACTERS_H_
#define FE_CHARACTERS_H_

// Characture structure (we're faking the in-game dialogue system here :P )
typedef struct fecharacter_s
{
    char *name;
    char *pic;
    char *voice;
    char *text;
} fecharacter_t;

extern fecharacter_t *curCharacter;

fecharacter_t *FE_GetCharacter(void);
void FE_DrawChar(void);

#define FE_MERCHANT_X 296
#define FE_MERCHANT_Y 193

void FE_MerchantSetState(int statenum);
void FE_InitMerchant(void);
void FE_MerchantTick(void);
void FE_DrawMerchant(int x, int y);

extern boolean merchantOn;

#endif

// EOF

