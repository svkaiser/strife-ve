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
//    Config bindings for OpenGL renderer
//

#include "m_config.h"
#include "rb_config.h"

// general
boolean rbLinearFiltering = false;
boolean rbTexturedAutomap = true;
boolean rbFixSpriteClipping = true;
boolean rbOutlineSprites = true;
boolean rbWallShades = true;
boolean rbLightmaps = true;
boolean rbLightmapsDefault = true;
boolean rbDynamicLights = true;
boolean rbDynamicLightFastBlend = false;
boolean rbForceSync = false;
boolean rbCrosshair = false;
#if defined(SVE_PLAT_SWITCH)
boolean rbVsync = true;
#else
boolean rbVsync = false;
#endif
boolean rbDecals = true;
int     rbMaxDecals = 128;
float   rbFOV = 74.0f;

// motion blur
boolean rbEnableMotionBlur = false;
float   rbMotionBlurRampSpeed = 0.0f;
int     rbMotionBlurSamples = 32;

// fxaa
boolean rbEnableFXAA = true;

// bloom
boolean rbEnableBloom = true;
float   rbBloomThreshold = 0.5f;

//
// RB_BindVariables
//

void RB_BindVariables(void)
{
    M_BindVariableWithDefault("gl_enable_renderer", &use3drenderer, &default_use3drenderer);
    M_BindVariable("gl_linear_filtering", &rbLinearFiltering);
    M_BindVariable("gl_textured_automap", &rbTexturedAutomap);
    M_BindVariable("gl_fix_sprite_clipping", &rbFixSpriteClipping);
    M_BindVariable("gl_outline_sprites", &rbOutlineSprites);
    M_BindVariable("gl_wall_shades", &rbWallShades);
    M_BindVariableWithDefault("gl_lightmaps", &rbLightmaps, &rbLightmapsDefault);
    M_BindVariable("gl_dynamic_lights", &rbDynamicLights);
    M_BindVariable("gl_dynamic_light_fast_blend", &rbDynamicLightFastBlend);
    M_BindVariable("gl_force_sync", &rbForceSync);
    M_BindVariable("gl_show_crosshair", &rbCrosshair);
    M_BindVariable("gl_enable_vsync", &rbVsync);
    M_BindVariable("gl_decals", &rbDecals);
    M_BindVariable("gl_max_decals", &rbMaxDecals);
    M_BindVariable("gl_fov", &rbFOV);
    M_BindVariable("gl_enable_motion_blur", &rbEnableMotionBlur);
    M_BindVariable("gl_motion_blur_ramp_speed", &rbMotionBlurRampSpeed);
    M_BindVariable("gl_motion_blur_samples", &rbMotionBlurSamples);
    M_BindVariable("gl_enable_fxaa", &rbEnableFXAA);
    M_BindVariable("gl_enable_bloom", &rbEnableBloom);
    M_BindVariable("gl_bloom_threshold", &rbBloomThreshold);
}
