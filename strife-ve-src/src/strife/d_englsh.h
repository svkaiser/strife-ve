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
//	Printed strings for translation.
//	English language support (default).
//

#ifndef __D_ENGLSH__
#define __D_ENGLSH__

//
//	Printed strings for translation
//

//
// D_Main.C
//
#define D_DEVSTR	"Development mode ON.\n"
#define D_CDROM	"CD-ROM Version: Accessing strife.cd\n"

//
//	M_Menu.C
//
#define PRESSKEY 	"press a key."
#define PRESSGPB        "press a button."
#define PRESSYN 	"press y or n."
#define PRESSGPCC       "press confirm or cancel."
#define QUITMSG	"are you sure you want to\nquit this great game?"
// [STRIFE] modified:
#define LOADNET 	"you can't load while in a net game!\n\n" PRESSKEY
#define LOADNETGP       "you can't load while in a net game!\n\n" PRESSGPB
#define QLOADNET	"you can't quickload during a netgame!\n\n" PRESSKEY
#define QLOADNETGP      "you can't quickload during a netgame!\n\n" PRESSGPB
// [STRIFE] modified:
#define QSAVESPOT	"you haven't picked a\nquicksave slot yet!\n\n" PRESSKEY
#define QSAVESPOTGP     "you haven't picked a\nquicksave slot yet!\n\n" PRESSGPB
// [STRIFE] modified:
#define SAVEDEAD 	"you're not playing a game\n\n" PRESSKEY
#define SAVEDEADGP      "you're not playing a game\n\n" PRESSGPB
#define QSPROMPT 	"quicksave over your game named\n\n'%s'?\n\n" PRESSYN
#define QSPROMPTGP      "quicksave over your game named\n\n'%s'?\n\n" PRESSGPCC

// [STRIFE] modified:
#define QLPROMPT	"do you want to quickload\n\n'%s'?\n\n" PRESSYN
#define QLPROMPTGP      "do you want to quickload\n\n'%s'?\n\n" PRESSGPCC

#define NEWGAME	\
"you can't start a new game\n"\
"while in a network game.\n\n" PRESSKEY

#define NEWGAMEGP \
"you can't start a new game\n" \
"while in a network game.\n\n" PRESSGPB

#define MSGOFF	"Messages OFF"
#define MSGON		"Messages ON"
#define NETEND	"you can't end a netgame!\n\n" PRESSKEY
#define NETENDGP "you can't end a netgame!\n\n" PRESSGPB
#define ENDGAME	"are you sure you want\nto end the game?\n\n" PRESSYN
#define ENDGAMEGP "are you sure you want\nto end the game?\n\n" PRESSGPCC

// [SVE]
#define NETNOSAVE   "You can't save a netgame.\n\n" PRESSKEY
#define NETNOSAVEGP "You can't save a netgame.\n\n" PRESSGPB

// [SVE]
#define CASTPROMPT   "Want to see the cast?\n\n(Spoiler alert!)\n\n" PRESSYN
#define CASTPROMPTGP "Want to see the cast?\n\n(Spoiler alert!)\n\n" PRESSGPCC

// haleyjd 09/11/10: [STRIFE] No "to dos." on this
#define DOSY		"(press y to quit)" 
#define DOSYGP          "(press confirm to quit)"

#define DETAILHI	"High detail"
#define DETAILLO	"Low detail"
#define GAMMALVL0	"Gamma correction OFF"
#define GAMMALVL1	"Gamma correction level 1"
#define GAMMALVL2	"Gamma correction level 2"
#define GAMMALVL3	"Gamma correction level 3"
#define GAMMALVL4	"Gamma correction level 4"
#define EMPTYSTRING	"empty slot"

//
//	G_game.C
//
#define GGSAVED	"game saved."

//
//	HU_stuff.C
//
#define HUSTR_MSGU	"[Message unsent]"

// haleyjd 08/31/10: [STRIFE] Strife map names

#define HUSTR_1         "AREA  1: sanctuary"
#define HUSTR_2         "AREA  2: town"
#define HUSTR_3         "AREA  3: front base"
#define HUSTR_4         "AREA  4: power station"
#define HUSTR_5         "AREA  5: prison"
#define HUSTR_6         "AREA  6: sewers"
#define HUSTR_7         "AREA  7: castle"
#define HUSTR_8         "AREA  8: Audience Chamber"
#define HUSTR_9         "AREA  9: Castle: Programmer's Keep"

#define HUSTR_10        "AREA 10: New Front Base"
#define HUSTR_11        "AREA 11: Borderlands"
#define HUSTR_12        "AREA 12: the temple of the oracle"
#define HUSTR_13        "AREA 13: Catacombs"
#define HUSTR_14        "AREA 14: mines"
#define HUSTR_15        "AREA 15: Fortress: Administration"
#define HUSTR_16        "AREA 16: Fortress: Bishop's Tower"
#define HUSTR_17        "AREA 17: Fortress: The Bailey"
#define HUSTR_18        "AREA 18: Fortress: Stores"
#define HUSTR_19        "AREA 19: Fortress: Security Complex"

#define HUSTR_20        "AREA 20: Factory: Receiving"
#define HUSTR_21        "AREA 21: Factory: Manufacturing"
#define HUSTR_22        "AREA 22: Factory: Forge"
#define HUSTR_23        "AREA 23: Order Commons"
#define HUSTR_24        "AREA 24: Factory: Conversion Chapel"
#define HUSTR_25        "AREA 25: Catacombs: Ruined Temple"
#define HUSTR_26        "AREA 26: proving grounds"
#define HUSTR_27        "AREA 27: The Lab"
#define HUSTR_28        "AREA 28: Alien Ship"
#define HUSTR_29        "AREA 29: Entity's Lair"

#define HUSTR_30        "AREA 30: Abandoned Front Base"
#define HUSTR_31        "AREA 31: Training Facility"

#define HUSTR_32        "DEMO  1: Sanctuary"
#define HUSTR_33        "DEMO  2: Town"
#define HUSTR_34        "DEMO  3: Movement Base"

// [SVE]: Super secret level
#define HUSTR_35        "AREA 35: Factory: Production"

// [SVE]: Capture the Chalice maps
#define HUSTR_36        "AREA 36: Castle Clash"
#define HUSTR_37        "AREA 37: Killing Grounds"
#define HUSTR_38        "AREA 38: Ordered Chaos"

// haleyjd 20110219: [STRIFE] replaced all with Strife chat macros:
#define HUSTR_CHATMACRO1        "Fucker!"
#define HUSTR_CHATMACRO2        "--SPLAT-- Instant wall art."
#define HUSTR_CHATMACRO3        "That had to hurt!"
#define HUSTR_CHATMACRO4        "Smackings!"
#define HUSTR_CHATMACRO5        "Gib-O-Matic baby."
#define HUSTR_CHATMACRO6        "Burn!  Yah! Yah!"
#define HUSTR_CHATMACRO7        "Buh-Bye!"
#define HUSTR_CHATMACRO8        "Sizzle chest!"
#define HUSTR_CHATMACRO9        "That sucked!"
#define HUSTR_CHATMACRO0        "Mommy?"

#define HUSTR_TALKTOSELF1	"You mumble to yourself"
#define HUSTR_TALKTOSELF2	"Who's there?"
#define HUSTR_TALKTOSELF3	"You scare yourself"
#define HUSTR_TALKTOSELF4	"You start to rave"
#define HUSTR_TALKTOSELF5	"You've lost it..."

#define HUSTR_MESSAGESENT	"[Message Sent]"

// The following should NOT be changed unless it seems
// just AWFULLY necessary

// [STRIFE]: Not used, as strings are local to hu_stuff.c
//#define HUSTR_PLRGREEN	"Green: "
//#define HUSTR_PLRINDIGO	"Indigo: "
//#define HUSTR_PLRBROWN	"Brown: "
//#define HUSTR_PLRRED		"Red: "

//#define HUSTR_KEYGREEN	'g'
//#define HUSTR_KEYINDIGO	'i'
//#define HUSTR_KEYBROWN	'b'
//#define HUSTR_KEYRED	'r'

//
//	AM_map.C
//

#define AMSTR_FOLLOWON	"Follow Mode ON"
#define AMSTR_FOLLOWOFF	"Follow Mode OFF"

#define AMSTR_GRIDON	"Grid ON"
#define AMSTR_GRIDOFF	"Grid OFF"

#define AMSTR_MARKEDSPOT        "Marked Spot"
#define AMSTR_MARKSCLEARED      "Last Mark Cleared"    // [STRIFE]

//
//	ST_stuff.C
//

#define STSTR_MUS               "Music Change"
#define STSTR_NOMUS             "IMPOSSIBLE SELECTION"
#define STSTR_DQDON             "You're Invincible!"   // [STRIFE]
#define STSTR_DQDOFF            "You're a looney!"     // [STRIFE]

#define STSTR_KFAADDED          "Very Happy Ammo Added"
#define STSTR_FAADDED           "Ammo Added"           // [STRIFE]

#define STSTR_NCON              "No Clipping Mode ON"
#define STSTR_NCOFF             "No Clipping Mode OFF"

#define STSTR_BEHOLD            "Bzrk, Inviso, Mask, Health, Pack, Stats"  // [STRIFE]
#define STSTR_BEHOLDX           "Power-up Toggled"

#define STSTR_CHOPPERS          "... doesn't suck - GM"
#define STSTR_CLEV              "Changing Level..."

#endif
