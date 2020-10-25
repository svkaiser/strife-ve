// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//     OpenGL-based scaling.
//
// The idea here is to do screen scaling and aspect ratio correction similar
// to that done in software in i_scale.c. However, Chocolate Doom is pretty
// unique and unorthodox in the way it does scaling.
//
// OpenGL has two basic scaling modes:
//   GL_NEAREST: Blocky scale up which works fine for integer multiple
//       scaling, but for non-integer scale factors causes distortion
//       of shapes, as some pixels will be doubled up.
//   GL_LINEAR:  Scale based on linear interpolation between the pixels
//       of the original texture. This causes a blurry appearance which
//       is unpleasant and unauthentic.
//
// What we want instead is a "blocky" scale-up with linear interpolation
// at the edges of the pixels to give a consistent appearance. In OpenGL
// we achieve this through a two-stage process:
//
// 1. We draw the software screen buffer into an OpenGL texture (named
//    unscaled_texture).
// 2. We draw the unscaled texture into a larger texture, scaling up
//    using GL_NEAREST to an integer multiple of the original screen
//    size. For example, 320x200 -> 960x600. GL framebuffers are used
//    to draw into the second texture (scaled_texture).
// 3. We draw the scaled texture to the screen, using GL_LINEAR to scale
//    back down to fit the actual screen resolution.
//
// Notes:
//  * The scaled texture should be a maximum of 2x the actual screen
//    size. This is because GL_LINEAR uses a 2x2 matrix of pixels to
//    interpolate between. If we go over 2x the actual screen size,
//    when we scale down we end up losing quality.
//  * The larger the scaled texture, the more accurately "blocky" the
//    screen appears; however, it's diminishing returns. After about
//    4x there's no real perceptible difference.
//
//-----------------------------------------------------------------------------

// TODO: Entire file should be surrounded with a #ifdef ENABLE_OPENGL
// so we can build without OpenGL.

#include <SDL.h>
#include "SDL_opengl.h"

#include <stdio.h>
#include <stdlib.h>

#include "i_video.h"
#include "m_argv.h"

// [SVE] svillarreal
#include "rb_main.h"
#include "rb_texture.h"
#include "rb_fbo.h"

// Maximum scale factor for the intermediate scaled texture. A value
// of 4 is pretty much perfect; you can try larger values but it's
// a case of diminishing returns.
int gl_max_scale = 4;

// Simulate fake scanlines?
static boolean scanline_mode = false;

// Screen dimensions:
static int screen_w, screen_h;

// Size of the calculated "window" of the screen that shows content:
// at a 4:3 mode these are equal to screen_w, screen_h.
static int window_w, window_h;

// The texture that "receives" the original 320x200 screen contents:
static GLuint unscaled_texture;
static unsigned int *unscaled_data = NULL;

// The scaled framebuffer
static rbfbo_t scaled_framebuffer;
static int scaled_w, scaled_h;

#ifdef SVE_PLAT_SWITCH
GL_Context ctx;
#endif

enum
{
    GLSCALE_PIPELINE_FBO, // use FBO
    GLSCALE_PIPELINE_IMM  // use immediate mode
};

static int glscale_pipeline;

// Check we have all the required extensions, otherwise this
// isn't going to work.
static boolean CheckExtensions(void)
{
    return has_GL_ARB_texture_non_power_of_two
        && has_GL_ARB_framebuffer_object;
}

// Get the size of the intermediate buffer (scaled_texture) for a particular
// dimension.
//   base_size: SCREENWIDTH or SCREENHEIGHT
//   limit_size: Size below which we use scale factor 1
//   window_size: size (width or height) of the actual window.
static int GetScaledSize(int base_size, int limit_size, int window_size)
{
    GLint maxtexture = -1;
    int factor;

    // It must be an integer multiple of the original (unscaled) screen
    // size, but no more than 2x the screen size we are rendering -
    // GL_LINEAR uses a 2x2 matrix to calculate which pixel to use.
    // The scaled size must be an integer multiple of the original
    // (unscaled) screen size, as we are drawing into it with GL_NEAREST.

    // For a given screen size, we want to use the next largest scale
    // factor that encompasses the screen. For example:
    //   640x480 -> 640x600 (only needs smooth scaling vertically)
    //   800x600 -> 960x600 (smooth scaling horizontally)
    factor = (window_size + base_size - 1) / base_size;

    // We don't want the scale factor to be an insane size, so we
    // set a limit (gl_max_scale).
    if (window_size < limit_size)
    {
        factor = 1;
    }
    else if (factor > gl_max_scale)
    {
        factor = gl_max_scale;
    }

    // The system has a limit on the texture size, and we must not
    // exceed this either.
    dglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexture);

    if (base_size * factor > maxtexture)
    {
        factor = maxtexture / base_size;
    }

    return factor * base_size;
}

// Called on startup or on window resize so that we calculate the
// size of the actual "window" where we show the game screen.
static void CalculateWindowSize(void)
{
    int base_width, base_height;

    // Aspect ratio we want depends on whether we have correction
    // turned on.
    if (aspect_ratio_correct)
    {
        base_width = SCREENWIDTH;
        base_height = SCREENHEIGHT_4_3;
    }
    else
    {
        base_width = SCREENWIDTH;
        base_height = SCREENHEIGHT;
    }

    // Either we will have borders to the left and right sides,
    // or at the top and bottom. Which is it?
    if (screen_w * base_height > screen_h * base_width)
    {
        window_w = (screen_h * base_width) / base_height;
        window_h = screen_h;
    }
    else
    {
        window_w = screen_w;
        window_h = (screen_w * base_height) / base_width;
    }

    if (scanline_mode)
    {
        scaled_w = SCREENWIDTH * 5;   // 1600
        scaled_h = SCREENHEIGHT * 6;  // x1200
        return;
    }

    // Calculate the size of the intermediate scaled texture. It will
    // be an integer multiple of the original screen size.
    // Below ~480x360 the scaling doesn't look so great. Use this as
    // the limit, below which we (effectively) just use GL_LINEAR.
    scaled_w = GetScaledSize(SCREENWIDTH, base_width * 1.5, window_w);
    scaled_h = GetScaledSize(SCREENHEIGHT, base_height * 1.5, window_h);
}

// Create the OpenGL textures used for scaling.
static boolean CreateTextures(void)
{
    // Unscaled texture for input:
    if (unscaled_data == NULL)
    {
        if(glscale_pipeline == GLSCALE_PIPELINE_FBO)
            unscaled_data = malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(int));
        else
            unscaled_data = calloc(512 * 256, sizeof(int));
    }
    
    if (unscaled_texture == 0)
    {
        dglGenTextures(1, &unscaled_texture);
    }

    dglBindTexture(GL_TEXTURE_2D, unscaled_texture);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    if(glscale_pipeline == GLSCALE_PIPELINE_IMM)
    {
        dglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        dglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    return true;
}

static boolean SetupFramebuffer(void)
{
    // Translate scaled-up texture to the screen with linear filtering.
    // Don't wrap/repeat the texture; this stops the linear filtering
    // from blurring the edges of the screen with each other.
    FBO_InitColorAttachment(&scaled_framebuffer, 0, scaled_w, scaled_h);
    return scaled_framebuffer.bLoaded;
}

// Import screen data from the given pointer and palette and update
// the unscaled_texture texture.
static void SetInputData(byte *screen, SDL_Color *palette)
{
    SDL_Color *c;
    byte *s;

    // TODO: Maybe support GL_RGB as well as GL_RGBA?
    if(glscale_pipeline == GLSCALE_PIPELINE_FBO)
    {
        unsigned int i;
        s = (byte *) unscaled_data;
        for (i = 0; i < SCREENWIDTH * SCREENHEIGHT; ++i)
        {
            c = &palette[screen[i]];
            *s++ = c->r;
            *s++ = c->g;
            *s++ = c->b;
            *s++ = 0xff;
        }
    }
    else
    {
        int x, y;
        for(y = 0; y < SCREENHEIGHT; y++)
        {
            s = (byte *)(unscaled_data + y * 512);
            for(x = 0; x < SCREENWIDTH; x++)
            {
                c = &palette[screen[y * SCREENWIDTH + x]];
                *s++ = c->r;
                *s++ = c->g;
                *s++ = c->b;
                *s++ = 0xff;
            }
        }
    }
        
    dglBindTexture(GL_TEXTURE_2D, unscaled_texture);
    if(glscale_pipeline == GLSCALE_PIPELINE_FBO)
    {
        dglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREENWIDTH, SCREENHEIGHT, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, unscaled_data);
    }
    else
    {
        dglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, unscaled_data);
    }
}

// Draw fake scanlines.
static void DrawScanlines(void)
{
    GLfloat y1;
    int y;

    dglColor3f(0.0, 0.0, 0.0);

    // We draw two scanlines for each row of pixels; this matches the
    // behavior of the software code.
    for (y = 0; y < SCREENHEIGHT * 2; ++y)
    {
        y1 = (float) y / SCREENHEIGHT - 1.0;
        dglBegin(GL_LINES);
        dglVertex2f(-1, y1);
        dglVertex2f(1, y1);
        dglEnd();
    }

    dglColor3f(1.0, 1.0, 1.0);
}

// Draw unscaled_texture (containing the screen buffer) into the
// second scaled_texture texture.
static void DrawUnscaledToScaled(void)
{
    if(glscale_pipeline != GLSCALE_PIPELINE_FBO)
        return;

    // Render into scaled_texture through framebuffer.
    FBO_Bind(&scaled_framebuffer);

    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();

    dglViewport(0, 0, scaled_w, scaled_h);

    dglBindTexture(GL_TEXTURE_2D, unscaled_texture);

    dglBegin(GL_QUADS);
    dglTexCoord2f(0, 1); dglVertex2f(-1, 1);
    dglTexCoord2f(1, 1); dglVertex2f(1, 1);
    dglTexCoord2f(1, 0); dglVertex2f(1, -1);
    dglTexCoord2f(0, 0); dglVertex2f(-1, -1);
    dglEnd();

    // Scanline hack.
    if (scanline_mode)
    {
        DrawScanlines();
    }

    // Finished with framebuffer
    FBO_UnBind(&scaled_framebuffer);
}

// Render the scaled_texture to the screen.
static void DrawScreen(void)
{
    GLfloat w, h;

    dglClear(GL_COLOR_BUFFER_BIT);

    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();

    dglViewport(0, 0, screen_w, screen_h);

    w = (float) window_w / screen_w;
    h = (float) window_h / screen_h;

    if(glscale_pipeline == GLSCALE_PIPELINE_FBO)
    {
        FBO_BindImage(&scaled_framebuffer);

        dglBegin(GL_QUADS);
        dglTexCoord2f(0, 0); dglVertex2f(-w,  h);
        dglTexCoord2f(1, 0); dglVertex2f( w,  h);
        dglTexCoord2f(1, 1); dglVertex2f( w, -h);
        dglTexCoord2f(0, 1); dglVertex2f(-w, -h);
        dglEnd();
    }
    else
    {
        float smax = (320.f / 512.0f);
        float tmax = (200.f / 256.0f);
        
        dglBindTexture(GL_TEXTURE_2D, unscaled_texture);

        dglBegin(GL_QUADS);
        dglTexCoord2f(0,    0   ); dglVertex2f(-w,  h);
        dglTexCoord2f(smax, 0   ); dglVertex2f( w,  h);
        dglTexCoord2f(smax, tmax); dglVertex2f( w, -h);
        dglTexCoord2f(0,    tmax); dglVertex2f(-w, -h);
        dglEnd();

        RB_UnbindTexture();
    }
}

boolean I_GL_InitScale(int w, int h)
{
    if(CheckExtensions())
        glscale_pipeline = GLSCALE_PIPELINE_FBO;
    else
        glscale_pipeline = GLSCALE_PIPELINE_IMM;

    // Scanline hack. Don't enable at less than half the 1600x1200
    // intermediate buffer size or horrible aliasing effects will
    // occur.
    scanline_mode = 
        glscale_pipeline == GLSCALE_PIPELINE_FBO && M_ParmExists("-scanline") 
        && h > (SCREENHEIGHT * 3);

    screen_w = w;
    screen_h = h;
    CalculateWindowSize();
    if(!CreateTextures()) 
    {
        return false;
    }
    if(glscale_pipeline == GLSCALE_PIPELINE_FBO)
    {
        if(!SetupFramebuffer())
            return false;
    }

    return true;
}

void I_GL_UpdateScreen(byte *screendata, SDL_Color *palette)
{
    // disable culling
    RB_SetState(GLSTATE_CULL, false);
    RB_SetCull(GLCULL_BACK);

    SetInputData(screendata, palette);
    DrawUnscaledToScaled();
    DrawScreen();
}

