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
//    Dynamic Lights
//

#include <math.h>

#include "doomstat.h"
#include "rb_main.h"
#include "rb_dynlights.h"
#include "rb_view.h"
#include "p_local.h"
#include "m_bbox.h"
#include "r_defs.h"
#include "r_state.h"
#include "z_zone.h"

static rbDynLight_t dynlightlist[MAX_DYNLIGHTS];
static rbDynLight_t *dynlight = NULL;
static unsigned int *markedssects;

typedef struct
{
    float radius;
    byte r;
    byte g;
    byte b;
} rbLightState_t;

static rbLightState_t lightStates[NUMSTATES];

typedef struct
{
    int state;
    float radius;
    byte r;
    byte g;
    byte b;
} rbLightStateDef_t;

static const rbLightStateDef_t lightStateDef[] =
{
    { S_ZAP1_00, 48, 215, 233, 255 },
    { S_ZAP1_01, 56, 215, 233, 255 },
    { S_ZAP1_02, 64, 215, 233, 255 },
    { S_ZAP1_03, 72, 160, 180, 240 },
    { S_ZAP1_04, 80, 156, 176, 224 },
    { S_ZAP1_05, 96, 128, 160, 208 },
    { S_ZAP1_06, 96, 112, 156, 208 },
    { S_ZAP1_07, 96, 112, 156, 208 },
    { S_ZAP1_08, 80, 156, 176, 224 },
    { S_ZAP1_09, 72, 160, 180, 240 },
    { S_ZAP1_10, 64, 215, 233, 255 },
    { S_ZAP1_11, 56, 215, 233, 255 },
    
    { S_PUFY_00, 32, 100, 50, 0 },
    { S_PUFY_01, 24, 100, 50, 0 },
    
    { S_SHT1_00, 96, 224, 0, 0 },
    { S_SHT1_01, 96, 160, 0, 0 },
    
    { S_BNG4_00, 96, 192, 176, 64 },
    { S_BNG4_01, 128, 192, 176, 64 },
    { S_BNG4_02, 192, 208, 180, 64 },
    { S_BNG4_03, 300, 224, 192, 80 },
    { S_BNG4_04, 280, 224, 192, 80 },
    { S_BNG4_05, 256, 224, 192, 80 },
    { S_BNG4_06, 224, 224, 192, 80 },
    { S_BNG4_07, 208, 224, 192, 80 },
    { S_BNG4_08, 192, 224, 192, 80 },
    { S_BNG4_09, 176, 224, 192, 80 },
    { S_BNG4_10, 160, 192, 176, 32 },
    { S_BNG4_11, 128, 144, 96, 16 },
    { S_BNG4_12, 112, 128, 64, 8 },
    { S_BNG4_13, 96, 96, 32, 0 },
    
    { S_FLBE_00, 64, 192, 128, 0 },
    { S_FLBE_01, 192, 224, 160, 0 },
    { S_FLBE_02, 320, 255, 192, 0 },
    { S_FLBE_03, 320, 255, 192, 0 },
    { S_FLBE_04, 320, 255, 192, 0 },
    { S_FLBE_05, 320, 255, 192, 0 },
    { S_FLBE_06, 320, 255, 192, 0 },
    { S_FLBE_07, 192, 224, 160, 0 },
    { S_FLBE_08, 128, 208, 144, 0 },
    { S_FLBE_09, 64, 192, 128, 0 },
    { S_FLBE_10, 32, 128, 96, 0 },
    
    { S_MICR_00, 128, 255, 192, 32 },
    
    { S_TORP_00, 320, 128, 255, 0 },
    { S_TORP_01, 320, 128, 255, 0 },
    { S_TORP_02, 320, 128, 255, 0 },
    { S_TORP_03, 320, 128, 255, 0 },
    
    { S_THIT_00, 256, 128, 255, 0 },
    { S_THIT_01, 280, 96, 224, 0 },
    { S_THIT_02, 296, 32, 208, 0 },
    { S_THIT_03, 308, 16, 192, 0 },
    { S_THIT_04, 320, 8, 128, 0 },
    
    { S_MISL_00, 128, 255, 192, 64 },
    { S_MISL_01, 128, 255, 192, 64 },
    { S_MISL_02, 136, 255, 192, 32 },
    { S_MISL_03, 144, 224, 160, 16 },
    { S_MISL_04, 156, 192, 108, 8 },
    { S_MISL_05, 160, 160, 96, 0 },
    { S_MISL_06, 168, 128, 64, 0 },
    { S_MISL_07, 176, 128, 64, 0 },
    
    { S_KLAX_02, 64, 128, 0, 0 },
    
    { S_BURN_00, 192, 255, 128, 0 },
    { S_BURN_01, 192, 255, 128, 0 },
    { S_BURN_02, 192, 255, 128, 0 },
    { S_BURN_03, 192, 255, 128, 0 },
    { S_BURN_04, 192, 255, 128, 0 },
    { S_BURN_05, 192, 255, 128, 0 },
    { S_BURN_06, 192, 255, 128, 0 },
    { S_BURN_07, 192, 255, 128, 0 },
    { S_BURN_08, 192, 255, 128, 0 },
    { S_BURN_09, 192, 255, 128, 0 },
    { S_BURN_10, 192, 255, 128, 0 },
    { S_BURN_11, 192, 255, 128, 0 },
    { S_BURN_12, 192, 255, 128, 0 },
    { S_BURN_13, 192, 255, 128, 0 },
    { S_BURN_14, 192, 255, 128, 0 },
    { S_BURN_15, 160, 224, 96, 0 },
    { S_BURN_16, 144, 208, 80, 0 },
    { S_BURN_17, 96, 176, 64, 0 },
    { S_BURN_18, 64, 128, 32, 0 },
    { S_BURN_19, 40, 96, 8, 0 },
    
    { S_ZOT3_00, 128, 215, 233, 255 },
    { S_ZOT3_01, 128, 215, 233, 255 },
    { S_ZOT3_02, 128, 215, 233, 255 },
    { S_ZOT3_03, 128, 215, 233, 255 },
    { S_ZOT3_04, 128, 215, 233, 255 },
    
    { S_ZAP6_03, 160, 215, 233, 255 },
    { S_ZAP6_04, 160, 215, 233, 255 },
    { S_ZAP6_05, 160, 215, 233, 255 },
    
    { S_ZAP7_00, 256, 215, 233, 255 },
    { S_ZAP7_01, 256, 215, 233, 255 },
    { S_ZAP7_02, 256, 215, 233, 255 },
    { S_ZAP7_03, 256, 215, 233, 255 },
    { S_ZAP7_04, 256, 215, 233, 255 },
    
    { S_ZOT1_00, 256, 215, 233, 255 },
    { S_ZOT1_01, 256, 215, 233, 255 },
    { S_ZOT1_02, 256, 215, 233, 255 },
    { S_ZOT1_03, 256, 215, 233, 255 },
    { S_ZOT1_04, 256, 215, 233, 255 },
    
    { S_ZAP5_00, 80, 215, 233, 255 },
    { S_ZAP5_01, 96, 215, 233, 255 },
    { S_ZAP5_02, 80, 215, 233, 255 },
    { S_ZAP5_03, 96, 215, 233, 255 },
    
    { S_ZOT2_00, 448, 215, 233, 255 },
    { S_ZOT2_01, 448, 215, 233, 255 },
    { S_ZOT2_02, 448, 215, 233, 255 },
    { S_ZOT2_03, 448, 215, 233, 255 },
    { S_ZOT2_04, 448, 215, 233, 255 },
    
    { S_FRBL_00, 32, 255, 128, 0 },
    { S_FRBL_01, 64, 255, 128, 0 },
    { S_FRBL_02, 64, 255, 128, 0 },
    { S_FRBL_03, 64, 224, 96, 0 },
    { S_FRBL_04, 56, 192, 64, 0 },
    { S_FRBL_05, 32, 144, 48, 0 },
    
    { S_BOOM_00, 512, 224, 192, 80 },
    { S_BOOM_01, 512, 224, 192, 80 },
    { S_BOOM_02, 512, 224, 192, 80 },
    { S_BOOM_03, 512, 224, 192, 80 },
    { S_BOOM_04, 512, 224, 192, 80 },
    { S_BOOM_05, 512, 224, 192, 80 },
    { S_BOOM_06, 512, 224, 192, 80 },
    { S_BOOM_07, 512, 224, 192, 80 },
    { S_BOOM_08, 512, 224, 192, 80 },
    { S_BOOM_09, 512, 224, 192, 80 },
    { S_BOOM_10, 512, 224, 192, 80 },
    { S_BOOM_11, 512, 224, 192, 80 },
    { S_BOOM_12, 512, 224, 192, 80 },
    { S_BOOM_13, 512, 224, 192, 80 },
    { S_BOOM_14, 512, 224, 192, 80 },
    { S_BOOM_15, 512, 224, 192, 80 },
    { S_BOOM_16, 448, 224, 192, 80 },
    { S_BOOM_17, 384, 208, 176, 80 },
    { S_BOOM_18, 256, 192, 160, 80 },
    { S_BOOM_19, 192, 176, 144, 72 },
    { S_BOOM_20, 160, 160, 128, 64 },
    { S_BOOM_21, 160, 144, 96, 32 },
    { S_BOOM_22, 160, 128, 80, 8 },
    { S_BOOM_23, 160, 96, 64, 0 },
    { S_BOOM_24, 128, 64, 32, 0 },
    
    { S_BART_01, 64, 255, 128, 32 },
    { S_BART_02, 96, 255, 128, 32 },
    { S_BART_03, 128, 255, 128, 32 },
    { S_BART_04, 160, 255, 128, 32 },
    { S_BART_05, 192, 255, 128, 32 },
    { S_BART_06, 208, 224, 96, 24 },
    { S_BART_07, 160, 208, 80, 16 },
    { S_BART_08, 128, 176, 64, 10 },
    { S_BART_09, 80, 160, 32, 4 },
    { S_BART_10, 32, 128, 8, 0 },

    { S_BNG2_00, 80, 255, 192, 64 },
    { S_BNG2_01, 128, 255, 192, 64 },
    { S_BNG2_02, 176, 255, 192, 64 },
    { S_BNG2_03, 256, 255, 192, 64 },
    { S_BNG2_04, 192, 224, 176, 48 },
    { S_BNG2_05, 176, 192, 128, 32 },
    { S_BNG2_06, 128, 160, 96, 16 },
    { S_BNG2_07, 96, 128, 64, 8 },
    { S_BNG2_08, 64, 96, 32, 0 },

    { S_BNG3_00, 64, 255, 192, 64 },
    { S_BNG3_01, 80, 255, 192, 64 },
    { S_BNG3_02, 96, 255, 192, 64 },
    { S_BNG3_03, 96, 255, 192, 64 },
    { S_BNG3_04, 104, 224, 176, 32 },
    { S_BNG3_05, 112, 192, 128, 8 },
    { S_BNG3_06, 112, 192, 128, 8 },
    { S_BNG3_07, 128, 128, 64, 0 },

    { S_ROB2_22, 128, 255, 192, 64 },
    { S_ROB2_23, 192, 255, 192, 64 },
    { S_ROB2_24, 256, 255, 192, 64 },
    { S_ROB2_25, 208, 224, 176, 32 },
    { S_ROB2_26, 176, 192, 128, 8 },
    { S_ROB2_27, 128, 128, 64, 0 },
    { S_ROB2_28, 64, 128, 32, 0 },

    { S_ROB1_18, 64, 176, 128, 32 },
    { S_ROB1_19, 96, 176, 128, 32 },
    { S_ROB1_20, 64, 176, 128, 32 },
    { S_ROB1_21, 32, 160, 96, 16 },
    { S_ROB1_22, 64, 255, 192, 64 },
    { S_ROB1_23, 128, 255, 192, 64 },
    { S_ROB1_24, 96, 128, 32, 0 },
    { S_ROB1_26, 64, 176, 128, 32 },
    { S_ROB1_27, 48, 176, 128, 32 },
    { S_ROB1_28, 32, 160, 96, 16 },
    { S_ROB1_29, 64, 255, 192, 64 },
    { S_ROB1_30, 128, 255, 192, 64 },
    { S_ROB1_31, 64, 128, 32, 0 }
};

//
// RB_InitDynLights
//

void RB_InitDynLights(void)
{
    int i;
    int j;
    
    memset(lightStates, 0, sizeof(rbLightState_t) * NUMSTATES);
    for(i = 0; i < NUMSTATES; ++i)
    {
        for(j = 0; j < arrlen(lightStateDef); ++j)
        {
            if(lightStateDef[j].state != i)
            {
                continue;
            }
            
            lightStates[i].radius = lightStateDef[j].radius;
            lightStates[i].r = lightStateDef[j].r;
            lightStates[i].g = lightStateDef[j].g;
            lightStates[i].b = lightStateDef[j].b;
            break;
        }
    }
}

//
// RB_ClearDynLights
//

void RB_ClearDynLights(void)
{
    dynlight = dynlightlist;
    memset(markedssects, 0, sizeof(unsigned int) * numsubsectors);
}

//
// RB_InitLightMarks
//

void RB_InitLightMarks(void)
{
    markedssects = (unsigned int*)Z_Calloc(1, sizeof(unsigned int) * numsubsectors, PU_LEVEL, 0);
}

//
// RB_SubsectorMarked
//

unsigned int RB_SubsectorMarked(const int num)
{
    return markedssects[num];
}

//
// RB_GetDynLight
//

rbDynLight_t *RB_GetDynLight(const int num)
{
    return &dynlightlist[num];
}

//
// RB_BoxOnNodeBounds
//

int RB_BoxOnNodeBounds(fixed_t *tmbox, node_t *node, int side)
{
    return !(tmbox[BOXRIGHT] < node->bbox[side][BOXLEFT] ||
             tmbox[BOXTOP] < node->bbox[side][BOXBOTTOM] ||
             tmbox[BOXLEFT] > node->bbox[side][BOXRIGHT] ||
             tmbox[BOXBOTTOM] > node->bbox[side][BOXTOP]);
}

//
// RB_ProcBSPDynLight
//

void RB_ProcBSPDynLight(rbDynLight_t *light, int bspnum)
{
    node_t *node;

    if(bspnum & NF_SUBSECTOR)
    {
        markedssects[bspnum & ~NF_SUBSECTOR] |= (1 << (light - dynlightlist));
        return;
    }

    node = &nodes[bspnum];
    
    if(RB_BoxOnNodeBounds(light->bbox, node, 0))
    {
        RB_ProcBSPDynLight(light, node->children[0]);
    }
    
    if(RB_BoxOnNodeBounds(light->bbox, node, 1))
    {
        RB_ProcBSPDynLight(light, node->children[1]);
    }
}

//
// RB_GetDynLightCount
//

const int RB_GetDynLightCount(void)
{
    return dynlight - dynlightlist;
}

//
// RB_AddDynLights
//

void RB_AddDynLights(void)
{
    thinker_t *th;
    byte *vis;
    int s1;
    int s2;
    
    RB_ClearDynLights();

    s1 = (players[displayplayer].mo->subsector - subsectors);
    vis = &pvsmatrix[(((numsubsectors + 7) / 8) * s1)];

    for(th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if(th->function.acp1 == (actionf_p1)P_MobjThinker)
        {
            mobj_t *mo = (mobj_t*)th;
            rbLightState_t *lightState;
            float x, y, z;

            lightState = &lightStates[mo->state - states];

            if(lightState->radius <= 0)
            {
                continue;
            }

            if(dynlight - dynlightlist >= MAX_DYNLIGHTS)
            {
                return;
            }

            s2 = (mo->subsector - subsectors);
            
            if(!(vis[s2 >> 3] & (1 << (s2 & 7))))
            {
                continue;
            }
            
            dynlight->x = FIXED2FLOAT(mo->x);
            dynlight->y = FIXED2FLOAT(mo->y);
            dynlight->z = FIXED2FLOAT(mo->z);

            dynlight->radius = lightState->radius;
            
            x = rbPlayerView.x - dynlight->x;
            y = rbPlayerView.y - dynlight->y;
            z = rbPlayerView.z - dynlight->z;

            if((x * x + y * y + z * z) > (dynlight->radius * dynlight->radius) * 128)
            {
                continue;
            }
            
            dynlight->thing = mo;
            dynlight->rgb[0] = lightState->r;
            dynlight->rgb[1] = lightState->g;
            dynlight->rgb[2] = lightState->b;
            
            dynlight->bbox[BOXTOP] = FLOAT2FIXED(dynlight->radius * 0.5f);
            dynlight->bbox[BOXBOTTOM] = FLOAT2FIXED(-dynlight->radius * 0.5f);
            dynlight->bbox[BOXRIGHT] = dynlight->bbox[BOXTOP];
            dynlight->bbox[BOXLEFT] = dynlight->bbox[BOXBOTTOM];
            
            dynlight->bbox[BOXTOP] += mo->y;
            dynlight->bbox[BOXBOTTOM] += mo->y;
            dynlight->bbox[BOXRIGHT] += mo->x;
            dynlight->bbox[BOXLEFT] += mo->x;
            
            if(mo->momx < 0)
            {
                dynlight->bbox[BOXLEFT] += (mo->momx-FRACUNIT);
            }
            else
            {
                dynlight->bbox[BOXRIGHT] += (mo->momx+FRACUNIT);
            }
            if(mo->momy < 0)
            {
                dynlight->bbox[BOXBOTTOM] += (mo->momy-FRACUNIT);
            }
            else
            {
                dynlight->bbox[BOXTOP] += (mo->momy+FRACUNIT);
            }

            RB_ProcBSPDynLight(dynlight, numnodes-1);
            dynlight++;
        }
    }
}
