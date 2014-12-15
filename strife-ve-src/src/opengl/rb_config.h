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

#ifndef __RB_CONFIG_H__
#define __RB_CONFIG_H__

#include "doomstat.h"

extern boolean  rbLinearFiltering;
extern boolean  rbTexturedAutomap;
extern boolean  rbFixSpriteClipping;
extern boolean  rbOutlineSprites;
extern boolean  rbWallShades;
extern boolean  rbLightmaps;
extern boolean  rbLightmapsDefault;
extern boolean  rbDynamicLights;
extern boolean  rbDynamicLightFastBlend;
extern boolean  rbForceSync;
extern boolean  rbCrosshair;
extern boolean  rbVsync;
extern boolean  rbDecals;
extern int      rbMaxDecals;
extern float    rbFOV;
extern boolean  rbEnableMotionBlur;
extern float    rbMotionBlurRampSpeed;
extern int      rbMotionBlurSamples;
extern boolean  rbEnableFXAA;
extern boolean  rbEnableBloom;
extern float    rbBloomThreshold;

void RB_BindVariables(void);

#define ADD_RB_VARIABLES()                              \
    CONFIG_VARIABLE_INT(gl_enable_renderer),            \
    CONFIG_VARIABLE_INT(gl_linear_filtering),           \
    CONFIG_VARIABLE_INT(gl_textured_automap),           \
    CONFIG_VARIABLE_INT(gl_fix_sprite_clipping),        \
    CONFIG_VARIABLE_INT(gl_outline_sprites),            \
    CONFIG_VARIABLE_INT(gl_wall_shades),                \
    CONFIG_VARIABLE_INT(gl_lightmaps),                  \
    CONFIG_VARIABLE_INT(gl_dynamic_lights),             \
    CONFIG_VARIABLE_INT(gl_dynamic_light_fast_blend),   \
    CONFIG_VARIABLE_INT(gl_force_sync),                 \
    CONFIG_VARIABLE_INT(gl_show_crosshair),             \
    CONFIG_VARIABLE_INT(gl_enable_vsync),               \
    CONFIG_VARIABLE_INT(gl_decals),                     \
    CONFIG_VARIABLE_INT(gl_max_decals),                 \
    CONFIG_VARIABLE_FLOAT(gl_fov),                      \
    CONFIG_VARIABLE_INT(gl_enable_motion_blur),         \
    CONFIG_VARIABLE_FLOAT(gl_motion_blur_ramp_speed),   \
    CONFIG_VARIABLE_INT(gl_motion_blur_samples),        \
    CONFIG_VARIABLE_INT(gl_enable_fxaa),                \
    CONFIG_VARIABLE_INT(gl_enable_bloom),               \
    CONFIG_VARIABLE_FLOAT(gl_bloom_threshold),

#endif
