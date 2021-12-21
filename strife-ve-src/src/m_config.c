//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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
//    Configuration file interface.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include <SDL.h>

#include "config.h"

#include "doomtype.h"
#include "doomkeys.h"
#include "doomfeatures.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"

#include "z_zone.h"

// [SVE] svillarreal
#include "rb_config.h"
#include "i_joystick.h"
#include "i_social.h"
#include "i_platsystem.h"

//
// DEFAULTS
//

// Location where all configuration data is stored - 
// default.cfg, savegames, etc.

char *configdir;

// [SVE] svillarreal
boolean config_fresh = false;
boolean extra_config_fresh = false;

// Default filenames for configuration files.

static char *default_main_config;
static char *default_extra_config;

typedef enum 
{
    DEFAULT_INT,
    DEFAULT_INT_HEX,
    DEFAULT_STRING,
    DEFAULT_FLOAT,
    DEFAULT_KEY,
} default_type_t;

typedef struct
{
    // Name of the variable
    char *name;

    // Pointer to the location in memory of the variable
    void *location;

    // haleyjd [SVE]: Pointer to default location, if any (allows runtime mod
    // of variables which must be runtime-invariant)
    void *default_location;

    // Type of the variable
    default_type_t type;

    // If this is a key value, the original integer scancode we read from
    // the config file before translating it to the internal key value.
    // If zero, we didn't read this value from a config file.
    int untranslated;

    // The value we translated the scancode into when we read the 
    // config file on startup.  If the variable value is different from
    // this, it has been changed and needs to be converted; otherwise,
    // use the 'untranslated' value.
    int original_translated;

    // If true, this config variable has been bound to a variable
    // and is being used.
    boolean bound;
} default_t;

typedef struct
{
    default_t *defaults;
    int numdefaults;
    char *filename;
} default_collection_t;

#define CONFIG_VARIABLE_GENERIC(name, type) \
    { #name, NULL, NULL, type, 0, 0, false }

#define CONFIG_VARIABLE_KEY(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_KEY)
#define CONFIG_VARIABLE_INT(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT)
#define CONFIG_VARIABLE_INT_HEX(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT_HEX)
#define CONFIG_VARIABLE_FLOAT(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_FLOAT)
#define CONFIG_VARIABLE_STRING(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_STRING)

//! @begin_config_file default

static default_t	doom_defaults_list[] =
{
    //!
    // Mouse sensitivity.  This value is used to multiply input mouse
    // movement to control the effect of moving the mouse.
    //
    // The "normal" maximum value available for this through the
    // in-game options menu is 9. A value of 31 or greater will cause
    // the game to crash when entering the options menu.
    //

    CONFIG_VARIABLE_INT(mouse_sensitivity_X),
    CONFIG_VARIABLE_INT(mouse_sensitivity_Y),

    //!
    // Volume of sound effects, range 0-15.
    //

    CONFIG_VARIABLE_INT(sfx_volume),

    //!
    // Volume of in-game music, range 0-15.
    //

    CONFIG_VARIABLE_INT(music_volume),

    //!
    // @game strife
    //
    // If non-zero, dialogue text is displayed over characters' pictures
    // when engaging actors who have voices.
    //

    CONFIG_VARIABLE_INT(show_talk),

    //!
    // @game strife
    //
    // Volume of voice sound effects, range 0-15.
    //

    CONFIG_VARIABLE_INT(voice_volume),

    //!
    // @game doom
    //
    // If non-zero, messages are displayed on the heads-up display
    // in the game ("picked up a clip", etc).  If zero, these messages
    // are not displayed.
    //

    CONFIG_VARIABLE_INT(show_messages),

    //!
    // Keyboard key to turn right.
    //

    CONFIG_VARIABLE_KEY(key_right),

    //!
    // Keyboard key to turn left.
    //

    CONFIG_VARIABLE_KEY(key_left),

    //!
    // Keyboard key to move forward.
    //

    CONFIG_VARIABLE_KEY(key_up),

    //!
    // Keyboard key to move backward.
    //

    CONFIG_VARIABLE_KEY(key_down),

    //!
    // Keyboard key to strafe left.
    //

    CONFIG_VARIABLE_KEY(key_strafeleft),

    //!
    // Keyboard key to strafe right.
    //

    CONFIG_VARIABLE_KEY(key_straferight),

    //!
    // @game strife
    //
    // Keyboard key to use health.
    //

    CONFIG_VARIABLE_KEY(key_useHealth),

    //!
    // @game hexen
    //
    // Keyboard key to jump.
    //

    CONFIG_VARIABLE_KEY(key_jump),

    //!
    // @game heretic hexen
    //
    // Keyboard key to fly upward.
    //

    CONFIG_VARIABLE_KEY(key_flyup),

    //!
    // @game heretic hexen
    //
    // Keyboard key to fly downwards.
    //

    CONFIG_VARIABLE_KEY(key_flydown),

    //!
    // @game heretic hexen
    //
    // Keyboard key to center flying.
    //

    CONFIG_VARIABLE_KEY(key_flycenter),

    //!
    // @game heretic hexen
    //
    // Keyboard key to look up.
    //

    CONFIG_VARIABLE_KEY(key_lookup),

    //!
    // @game heretic hexen
    //
    // Keyboard key to look down.
    //

    CONFIG_VARIABLE_KEY(key_lookdown),

    //!
    // @game heretic hexen
    //
    // Keyboard key to center the view.
    //

    CONFIG_VARIABLE_KEY(key_lookcenter),

    //!
    // @game strife
    //
    // Keyboard key to query inventory.
    //

    CONFIG_VARIABLE_KEY(key_invquery),

    //!
    // @game strife
    //
    // Keyboard key to display mission objective.
    //

    CONFIG_VARIABLE_KEY(key_mission),

    //!
    // @game strife
    //
    // Keyboard key to display inventory popup.
    //

    CONFIG_VARIABLE_KEY(key_invPop),

    //!
    // @game strife
    //
    // Keyboard key to display keys popup.
    //

    CONFIG_VARIABLE_KEY(key_invKey),

    //!
    // @game strife
    //
    // Keyboard key to jump to start of inventory.
    //

    CONFIG_VARIABLE_KEY(key_invHome),

    //!
    // @game strife
    //
    // Keyboard key to jump to end of inventory.
    //

    CONFIG_VARIABLE_KEY(key_invEnd),

    //!
    // @game heretic hexen
    //
    // Keyboard key to scroll left in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invleft),

    //!
    // @game heretic hexen
    //
    // Keyboard key to scroll right in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invright),

    //!
    // @game strife
    //
    // Keyboard key to scroll left in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invLeft),

    //!
    // @game strife
    //
    // Keyboard key to scroll right in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invRight),

    //!
    // @game heretic hexen
    //
    // Keyboard key to use the current item in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_useartifact),

    //!
    // @game strife
    //
    // Keyboard key to use inventory item.
    //

    CONFIG_VARIABLE_KEY(key_invUse),

    //!
    // @game strife
    //
    // Keyboard key to drop an inventory item.
    //

    CONFIG_VARIABLE_KEY(key_invDrop),
    
    //!
    // @game strife
    //
    // Keyboard key to center view
    //
    
    CONFIG_VARIABLE_KEY(key_centerview),

    //!
    // @game strife
    //
    // Keyboard key to look up.
    //

    CONFIG_VARIABLE_KEY(key_lookUp),

    //!
    // @game strife
    //
    // Keyboard key to look down.
    //

    CONFIG_VARIABLE_KEY(key_lookDown),

    //!
    // Keyboard key to fire the currently selected weapon.
    //

    CONFIG_VARIABLE_KEY(key_fire),

    //!
    // Keyboard key to "use" an object, eg. a door or switch.
    //

    CONFIG_VARIABLE_KEY(key_use),

    //!
    // Keyboard key to turn on strafing.  When held down, pressing the
    // key to turn left or right causes the player to strafe left or
    // right instead.
    //

    CONFIG_VARIABLE_KEY(key_strafe),

    //!
    // Keyboard key to make the player run.
    //

    CONFIG_VARIABLE_KEY(key_speed),

    //!
    // If non-zero, mouse input is enabled.  If zero, mouse input is
    // disabled.
    //

    CONFIG_VARIABLE_INT(use_mouse),

    //!
    // Mouse button to fire the currently selected weapon.
    //

    CONFIG_VARIABLE_INT(mouseb_fire),

    //!
    // Mouse button to turn on strafing.  When held down, the player
    // will strafe left and right instead of turning left and right.
    //

    CONFIG_VARIABLE_INT(mouseb_strafe),

    //!
    // Mouse button to move forward.
    //

    CONFIG_VARIABLE_INT(mouseb_forward),

    //!
    // If non-zero, joystick input is enabled.
    //

    CONFIG_VARIABLE_INT(use_joystick),

    //!
    // Joystick virtual button that fires the current weapon.
    //

    CONFIG_VARIABLE_INT(joyb_fire),

    //!
    // Joystick virtual button that makes the player strafe while
    // held down.
    //

    CONFIG_VARIABLE_INT(joyb_strafe),

    //!
    // Joystick virtual button to "use" an object, eg. a door or switch.
    //

    CONFIG_VARIABLE_INT(joyb_use),

    //!
    // Joystick virtual button that makes the player run while held
    // down.
    //
    // If this has a value of 20 or greater, the player will always run,
    // even if use_joystick is 0.
    //

    CONFIG_VARIABLE_INT(joyb_speed),

    //!
    // @game hexen strife
    //
    // Joystick virtual button that makes the player jump.
    //

    CONFIG_VARIABLE_INT(joyb_jump),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_invleft),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_invright),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_invuse),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_invdrop),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_centerview),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_mission),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_invpop),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joyb_invkey),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joy_gyroscope),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_INT(joy_gyrostyle),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_FLOAT(joy_gyrosensitivityh),

    //!
    // @game strife [SVE]
    //

    CONFIG_VARIABLE_FLOAT(joy_gyrosensitivityv),

    //!
    // @game doom heretic hexen
    //
    // Screen size, range 3-11.
    //
    // A value of 11 gives a full-screen view with the status bar not
    // displayed.  A value of 10 gives a full-screen view with the
    // status bar displayed.
    //

    CONFIG_VARIABLE_INT(screenblocks),

    //!
    // @game strife
    //
    // Screen size, range 3-11.
    //
    // A value of 11 gives a full-screen view with the status bar not
    // displayed.  A value of 10 gives a full-screen view with the
    // status bar displayed.
    //

    CONFIG_VARIABLE_INT(screensize),

    //!
    // @game doom
    //
    // Screen detail.  Zero gives normal "high detail" mode, while
    // a non-zero value gives "low detail" mode.
    //

    CONFIG_VARIABLE_INT(detaillevel),

    //!
    // Number of sounds that will be played simultaneously.
    //

    CONFIG_VARIABLE_INT(snd_channels),

    //!
    // Music output device.  A non-zero value gives MIDI sound output,
    // while a value of zero disables music.
    //

    CONFIG_VARIABLE_INT(snd_musicdevice),

    //!
    // Sound effects device.  A value of zero disables in-game sound
    // effects, a value of 1 enables PC speaker sound effects, while
    // a value in the range 2-9 enables the "normal" digital sound
    // effects.
    //

    CONFIG_VARIABLE_INT(snd_sfxdevice),

    //!
    // SoundBlaster I/O port. Unused.
    //

    CONFIG_VARIABLE_INT(snd_sbport),

    //!
    // SoundBlaster IRQ.  Unused.
    //

    CONFIG_VARIABLE_INT(snd_sbirq),

    //!
    // SoundBlaster DMA channel.  Unused.
    //

    CONFIG_VARIABLE_INT(snd_sbdma),

    //!
    // Output port to use for OPL MIDI playback.  Unused.
    //

    CONFIG_VARIABLE_INT(snd_mport),

    //!
    // Gamma correction level.  A value of zero disables gamma
    // correction, while a value in the range 1-4 gives increasing
    // levels of gamma correction.
    //

    CONFIG_VARIABLE_INT(usegamma),

    //!
    // @game hexen
    //
    // Directory in which to store savegames.
    //

    CONFIG_VARIABLE_STRING(savedir),

    //!
    // @game hexen
    //
    // Controls whether messages are displayed in the heads-up display.
    // If this has a non-zero value, messages are displayed.
    //

    CONFIG_VARIABLE_INT(messageson),

    //!
    // @game strife
    //
    // Name of background flat used by view border.
    //

    CONFIG_VARIABLE_STRING(back_flat),

    //!
    // @game strife
    //
    // Multiplayer nickname (?).
    //

    CONFIG_VARIABLE_STRING(nickname),

    //!
    // Multiplayer chat macro: message to send when alt+0 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro0),

    //!
    // Multiplayer chat macro: message to send when alt+1 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro1),

    //!
    // Multiplayer chat macro: message to send when alt+2 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro2),

    //!
    // Multiplayer chat macro: message to send when alt+3 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro3),

    //!
    // Multiplayer chat macro: message to send when alt+4 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro4),

    //!
    // Multiplayer chat macro: message to send when alt+5 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro5),

    //!
    // Multiplayer chat macro: message to send when alt+6 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro6),

    //!
    // Multiplayer chat macro: message to send when alt+7 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro7),

    //!
    // Multiplayer chat macro: message to send when alt+8 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro8),

    //!
    // Multiplayer chat macro: message to send when alt+9 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro9),

    //!
    // @game strife
    //
    // Serial port number to use for SERSETUP.EXE (unused).
    //

    CONFIG_VARIABLE_INT(comport),
};

static default_collection_t doom_defaults =
{
    doom_defaults_list,
    arrlen(doom_defaults_list),
    NULL,
};

//! @begin_config_file extended

static default_t extra_defaults_list[] =
{
    //!
    // @game heretic hexen strife
    //
    // If non-zero, display the graphical startup screen.
    //

    CONFIG_VARIABLE_INT(graphical_startup),

    //!
    // @game strife [SVE]
    //
    // If non-zero, frame rates will be interpolated to 60fps
    //

    CONFIG_VARIABLE_INT(interpolate_frames),

    //!
    // @game strife [SVE]
    //
    // If non-zero, limit game to 60 FPS
    //

    CONFIG_VARIABLE_INT(d_fpslimit),

    //!
    // @game strife [SVE]
    //
    // If non-zero, intro movies will not play and instead skip
    // directly to the frontend
    //

    CONFIG_VARIABLE_INT(skip_movies),

    //!
    // @game strife [SVE]
    //
    // If non-zero, player's view will bob after firing certain weapons
    //

    CONFIG_VARIABLE_INT(weapon_recoil),

    //!
    // @game strife [SVE]
    //
    // if non-zero, indicator arrows will display showing the source of the damage
    //

    CONFIG_VARIABLE_INT(damage_indicator),

    //!
    // @game strife [SVE]
    //
    // If non-zero, player autoaims in single player mode
    //

    CONFIG_VARIABLE_INT(autoaim),

    //!
    // @game strife [SVE]
    //
    // If non-zero, fullscreen HUD will be rendered
    //

    CONFIG_VARIABLE_INT(fullscreen_hud),

    //!
    // @game strife [SVE]
    //
    // If non-zero, enemy deaths will be twice of violent
    //

    CONFIG_VARIABLE_INT(max_gore),

    //!
    // @game strife [SVE]
    //
    // If non-zero, be as much like vanilla as is practical
    //

    CONFIG_VARIABLE_INT(classicmode),

    //!
    // @game strife [SVE]
    //
    // If non-zero, player always runs.
    //

    CONFIG_VARIABLE_INT(autorun),

    //!
    // If non-zero, video settings will be autoadjusted to a valid
    // configuration when the screen_width and screen_height variables
    // do not match any valid configuration.
    //

    CONFIG_VARIABLE_INT(autoadjust_video_settings),

    //!
    // If non-zero, the game will run in full screen mode.  If zero,
    // the game will run in a window.
    //

    CONFIG_VARIABLE_INT(fullscreen),


    //!
    // If non-zero, the game will not display window borders.
    //

    CONFIG_VARIABLE_INT(window_noborder),

    //!
    // If non-zero, the screen will be stretched vertically to display
    // correctly on a square pixel video mode.
    //

    CONFIG_VARIABLE_INT(aspect_ratio_correct),

    //!
    // Number of milliseconds to wait on startup after the video mode
    // has been set, before the game will start.  This allows the
    // screen to settle on some monitors that do not display an image
    // for a brief interval after changing video modes.
    //

    CONFIG_VARIABLE_INT(startup_delay),

    //!
    // Screen width in pixels.  If running in full screen mode, this is
    // the X dimension of the video mode to use.  If running in
    // windowed mode, this is the width of the window in which the game
    // will run.
    //

    CONFIG_VARIABLE_INT(screen_width),

    //!
    // Screen height in pixels.  If running in full screen mode, this is
    // the Y dimension of the video mode to use.  If running in
    // windowed mode, this is the height of the window in which the game
    // will run.
    //

    CONFIG_VARIABLE_INT(screen_height),

    //!
    // If 0, the game is going to try to set the best resolution available
    // on the first run regardless of other settings. [SVE]
    //

    CONFIG_VARIABLE_INT(screen_init),

    //!
    // Color depth of the screen, in bits.
    // If this is set to zero, the color depth will be automatically set
    // on startup to the machine's default/native color depth.
    //

    CONFIG_VARIABLE_INT(screen_bpp),

    //!
    // [SVE] svillarreal - from gl scale branch
    //
    // Maximum scale factor for the intermediate buffer used for doing
    // hardware-based scaling. A scale factor of 1 will be very blurry
    // but not use a lot of texture memory; a scale factor of 4 gives
    // pretty much best results.

    CONFIG_VARIABLE_INT(gl_max_scale),

    //!
    // If this is non-zero, the mouse will be "grabbed" when running
    // in windowed mode so that it can be used as an input device.
    // When running full screen, this has no effect.
    //

    CONFIG_VARIABLE_INT(grabmouse),

    //!
    // If non-zero, all vertical mouse movement is ignored.  This
    // emulates the behavior of the "novert" tool available under DOS
    // that performs the same function.
    //

    CONFIG_VARIABLE_INT(novert),

    //!
    // Mouse acceleration factor.  When the speed of mouse movement
    // exceeds the threshold value (mouse_threshold), the speed is
    // multiplied by this value.
    //

    CONFIG_VARIABLE_FLOAT(mouse_acceleration),

    //!
    // Mouse acceleration threshold.  When the speed of mouse movement
    // exceeds this threshold value, the speed is multiplied by an
    // acceleration factor (mouse_acceleration).
    //

    CONFIG_VARIABLE_INT(mouse_threshold),

    // [SVE] svillarreal
    CONFIG_VARIABLE_INT(mouse_invert),
    CONFIG_VARIABLE_INT(mouse_scale),
    CONFIG_VARIABLE_INT(mouse_enable_acceleration),
    CONFIG_VARIABLE_INT(mouse_smooth),

    //!
    // Sound output sample rate, in Hz.  Typical values to use are
    // 11025, 22050, 44100 and 48000.
    //

    CONFIG_VARIABLE_INT(snd_samplerate),

    //!
    // Maximum number of bytes to allocate for caching converted sound
    // effects in memory. If set to zero, there is no limit applied.
    //

    CONFIG_VARIABLE_INT(snd_cachesize),

    //!
    // Maximum size of the output sound buffer size in milliseconds.
    // Sound output is generated periodically in slices. Higher values
    // might be more efficient but will introduce latency to the
    // sound output. The default is 28ms (one slice per tic with the
    // 35fps timer).

    CONFIG_VARIABLE_INT(snd_maxslicetime_ms),

    //!
    // External command to invoke to perform MIDI playback. If set to
    // the empty string, SDL_mixer's internal MIDI playback is used.
    // This only has any effect when snd_musicdevice is set to General
    // MIDI output.

    CONFIG_VARIABLE_STRING(snd_musiccmd),

    //!
    // The I/O port to use to access the OPL chip.  Only relevant when
    // using native OPL music playback.
    //

    CONFIG_VARIABLE_INT_HEX(opl_io_port),

    //!
    // @game doom heretic strife
    //
    // If non-zero, the ENDOOM text screen is displayed when exiting the
    // game. If zero, the ENDOOM screen is not displayed.
    //

    CONFIG_VARIABLE_INT(show_endoom),

    //!
    // If non-zero, save screenshots in PNG format.
    //

    CONFIG_VARIABLE_INT(png_screenshots),

    //!
    // @game doom strife
    //
    // If non-zero, the Vanilla savegame limit is enforced; if the
    // savegame exceeds 180224 bytes in size, the game will exit with
    // an error.  If this has a value of zero, there is no limit to
    // the size of savegames.
    //

    CONFIG_VARIABLE_INT(vanilla_savegame_limit),

    //!
    // @game doom strife
    //
    // If non-zero, the Vanilla demo size limit is enforced; the game
    // exits with an error when a demo exceeds the demo size limit
    // (128KiB by default).  If this has a value of zero, there is no
    // limit to the size of demos.
    //

    CONFIG_VARIABLE_INT(vanilla_demo_limit),

    //!
    // If non-zero, the game behaves like Vanilla Doom, always assuming
    // an American keyboard mapping.  If this has a value of zero, the
    // native keyboard mapping of the keyboard is used.
    //

    CONFIG_VARIABLE_INT(vanilla_keyboard_mapping),

    //!
    // Name of the SDL video driver to use.  If this is an empty string,
    // the default video driver is used.
    //

    CONFIG_VARIABLE_STRING(video_driver),

    //!
    // Position of the window on the screen when running in windowed
    // mode. Accepted values are: "" (empty string) - don't care,
    // "center" - place window at center of screen, "x,y" - place
    // window at the specified coordinates.

    CONFIG_VARIABLE_STRING(window_position),

#ifdef FEATURE_MULTIPLAYER

    //!
    // Name to use in network games for identification.  This is only
    // used on the "waiting" screen while waiting for the game to start.
    //

    CONFIG_VARIABLE_STRING(player_name),

#endif

    //!
    // Joystick number to use; '0' is the first joystick.  A negative
    // value ('-1') indicates that no joystick is configured.
    //

    CONFIG_VARIABLE_INT(joystick_index),

    //!
    // Joystick axis to use to for horizontal (X) movement.
    //

    CONFIG_VARIABLE_INT(joystick_x_axis),

    //!
    // If non-zero, movement on the horizontal joystick axis is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_x_invert),

    //!
    // Joystick axis to use to for vertical (Y) movement.
    //

    CONFIG_VARIABLE_INT(joystick_y_axis),

    //!
    // If non-zero, movement on the vertical joystick axis is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_y_invert),

    //!
    // Joystick axis to use to for strafing movement.
    //

    CONFIG_VARIABLE_INT(joystick_strafe_axis),

    //!
    // If non-zero, movement on the joystick axis used for strafing
    // is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_strafe_invert),

    //!
    // [SVE] svillarreal - Joystick axis to use to for looking
    //

    CONFIG_VARIABLE_INT(joystick_look_axis),

    //!
    // [SVE] svillarreal - If non-zero, movement on the joystick axis used for looking
    // is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_look_invert),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #0.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button0),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #1.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button1),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #2.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button2),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #3.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button3),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #4.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button4),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #5.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button5),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #6.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button6),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #7.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button7),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #8.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button8),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #9.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button9),

    // [SVE]
    // If you ask me this is totally braindead. EE supports practically
    // unlimited joybuttons and never does anything remotely like this.

    CONFIG_VARIABLE_INT(joystick_physical_button10),
    CONFIG_VARIABLE_INT(joystick_physical_button11),
    CONFIG_VARIABLE_INT(joystick_physical_button12),
    CONFIG_VARIABLE_INT(joystick_physical_button13),
    CONFIG_VARIABLE_INT(joystick_physical_button14),
    CONFIG_VARIABLE_INT(joystick_physical_button15),

    //!
    // Joystick virtual button to make the player strafe left.
    //

    CONFIG_VARIABLE_INT(joyb_strafeleft),

    //!
    // Joystick virtual button to make the player strafe right.
    //

    CONFIG_VARIABLE_INT(joyb_straferight),

    // [SVE] svillarreal
    //!
    // XInput right stick sensitivity
    //

    CONFIG_VARIABLE_FLOAT(joystick_turnsensitivity),
    CONFIG_VARIABLE_FLOAT(joystick_looksensitivity),

    //!
    // XInput right stick threshold
    //

    CONFIG_VARIABLE_FLOAT(joystick_threshold),

    //!
    // Joystick virtual button to activate the menu.
    //

    CONFIG_VARIABLE_INT(joyb_menu_activate),
    CONFIG_VARIABLE_INT(joyb_menu_up),
    CONFIG_VARIABLE_INT(joyb_menu_down),
    CONFIG_VARIABLE_INT(joyb_menu_left),
    CONFIG_VARIABLE_INT(joyb_menu_right),
    CONFIG_VARIABLE_INT(joyb_menu_back),
    CONFIG_VARIABLE_INT(joyb_menu_forward),
    CONFIG_VARIABLE_INT(joyb_menu_confirm),
    CONFIG_VARIABLE_INT(joyb_menu_abort),

    //!
    // Joystick virtual button that cycles to the previous weapon.
    //

    CONFIG_VARIABLE_INT(joyb_prevweapon),

    //!
    // Joystick virtual button that cycles to the next weapon.
    //

    CONFIG_VARIABLE_INT(joyb_nextweapon),

    //!
    // Mouse button to strafe left.
    //

    CONFIG_VARIABLE_INT(mouseb_strafeleft),

    //!
    // Mouse button to strafe right.
    //

    CONFIG_VARIABLE_INT(mouseb_straferight),

    //!
    // Mouse button to "use" an object, eg. a door or switch.
    //

    CONFIG_VARIABLE_INT(mouseb_use),

    //!
    // Mouse button to move backwards.
    //

    CONFIG_VARIABLE_INT(mouseb_backward),

    //!
    // Mouse button to cycle to the previous weapon.
    //

    CONFIG_VARIABLE_INT(mouseb_prevweapon),

    //!
    // Mouse button to cycle to the next weapon.
    //

    CONFIG_VARIABLE_INT(mouseb_nextweapon),

    //!
    // [SVE] svillarreal - moved here
    // Mouse button to jump.
    //

    CONFIG_VARIABLE_INT(mouseb_jump),
    
    //!
    // [SVE] haleyjd
    // Mouse button to use inventory
    //

    CONFIG_VARIABLE_INT(mouseb_invuse),
    
    //!
    // [SVE] haleyjd
    // Mouse button to scroll to prev inventory
    //

    CONFIG_VARIABLE_INT(mouseb_invprev),
    
    //!
    // [SVE] haleyjd
    // Mouse button to scroll to next inventory
    //

    CONFIG_VARIABLE_INT(mouseb_invnext),

    //!
    // If non-zero, double-clicking a mouse button acts like pressing
    // the "use" key to use an object in-game, eg. a door or switch.
    //

    CONFIG_VARIABLE_INT(dclick_use),

#ifdef FEATURE_SOUND

    //!
    // Controls whether libsamplerate support is used for performing
    // sample rate conversions of sound effects.  Support for this
    // must be compiled into the program.
    //
    // If zero, libsamplerate support is disabled.  If non-zero,
    // libsamplerate is enabled. Increasing values roughly correspond
    // to higher quality conversion; the higher the quality, the
    // slower the conversion process.  Linear conversion = 1;
    // Zero order hold = 2; Fast Sinc filter = 3; Medium quality
    // Sinc filter = 4; High quality Sinc filter = 5.
    //

    CONFIG_VARIABLE_INT(use_libsamplerate),

    //!
    // Scaling factor used by libsamplerate. This is used when converting
    // sounds internally back into integer form; normally it should not
    // be necessary to change it from the default value. The only time
    // it might be needed is if a PWAD file is loaded that contains very
    // loud sounds, in which case the conversion may cause sound clipping
    // and the scale factor should be reduced. The lower the value, the
    // quieter the sound effects become, so it should be set as high as is
    // possible without clipping occurring.

    CONFIG_VARIABLE_FLOAT(libsamplerate_scale),

    //!
    // Full path to a Timidity configuration file to use for MIDI
    // playback. The file will be evaluated from the directory where
    // it is evaluated, so there is no need to add "dir" commands
    // into it.
    //

    CONFIG_VARIABLE_STRING(timidity_cfg_path),

    //!
    // Path to GUS patch files to use when operating in GUS emulation
    // mode.
    //

    CONFIG_VARIABLE_STRING(gus_patch_path),

    //!
    // Number of kilobytes of RAM to use in GUS emulation mode. Valid
    // values are 256, 512, 768 or 1024.
    //

    CONFIG_VARIABLE_INT(gus_ram_kb),

#endif

    //!
    // Key to pause or unpause the game.
    //

    CONFIG_VARIABLE_KEY(key_pause),

    //!
    // Key that activates the menu when pressed.
    //

    CONFIG_VARIABLE_KEY(key_menu_activate),

    //!
    // Key that moves the cursor up on the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_up),

    //!
    // Key that moves the cursor down on the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_down),

    //!
    // Key that moves the currently selected slider on the menu left.
    //

    CONFIG_VARIABLE_KEY(key_menu_left),

    //!
    // Key that moves the currently selected slider on the menu right.
    //

    CONFIG_VARIABLE_KEY(key_menu_right),

    //!
    // Key to go back to the previous menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_back),

    //!
    // Key to activate the currently selected menu item.
    //

    CONFIG_VARIABLE_KEY(key_menu_forward),

    //!
    // Key to answer 'yes' to a question in the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_confirm),

    //!
    // Key to answer 'no' to a question in the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_abort),

    //!
    // Keyboard shortcut to bring up the help screen.
    //

    CONFIG_VARIABLE_KEY(key_menu_help),

    //!
    // Keyboard shortcut to bring up the save game menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_save),

    //!
    // Keyboard shortcut to bring up the load game menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_load),

    //!
    // Keyboard shortcut to bring up the sound volume menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_volume),

    //!
    // Keyboard shortcut to toggle the detail level.
    //

    // [SVE] svillarreal - renamed
    CONFIG_VARIABLE_KEY(key_menu_autohealth),

    //!
    // Keyboard shortcut to quicksave the current game.
    //

    CONFIG_VARIABLE_KEY(key_menu_qsave),

    //!
    // Keyboard shortcut to end the game.
    //

    CONFIG_VARIABLE_KEY(key_menu_endgame),

    //!
    // Keyboard shortcut to toggle heads-up messages.
    //

    CONFIG_VARIABLE_KEY(key_menu_messages),

    //!
    // Keyboard shortcut to load the last quicksave.
    //

    CONFIG_VARIABLE_KEY(key_menu_qload),

    //!
    // Keyboard shortcut to quit the game.
    //

    CONFIG_VARIABLE_KEY(key_menu_quit),

    //!
    // Keyboard shortcut to toggle the gamma correction level.
    //

    CONFIG_VARIABLE_KEY(key_menu_gamma),

    //!
    // Keyboard shortcut to switch view in multiplayer.
    //

    CONFIG_VARIABLE_KEY(key_spy),

    //!
    // Keyboard shortcut to increase the screen size.
    //

    CONFIG_VARIABLE_KEY(key_menu_incscreen),

    //!
    // Keyboard shortcut to decrease the screen size.
    //

    CONFIG_VARIABLE_KEY(key_menu_decscreen),

    //!
    // Keyboard shortcut to save a screenshot.
    //

    CONFIG_VARIABLE_KEY(key_menu_screenshot),

    //!
    // Key to toggle the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_toggle),

    //!
    // Key to pan north when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_north),

    //!
    // Key to pan south when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_south),

    //!
    // Key to pan east when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_east),

    //!
    // Key to pan west when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_west),

    //!
    // Key to zoom in when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_zoomin),

    //!
    // Key to zoom out when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_zoomout),

    //!
    // Key to zoom out the maximum amount when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_maxzoom),

    //!
    // Key to toggle follow mode when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_follow),

    //!
    // Key to toggle the grid display when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_grid),

    //!
    // Key to set a mark when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_mark),

    //!
    // Key to clear all marks when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_clearmark),

    //!
    // Key to toggle the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_toggle),

    //!
    // Key to pan north when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_north),

    //!
    // Key to pan south when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_south),

    //!
    // Key to pan east when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_east),

    //!
    // Key to pan west when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_west),

    //!
    // Key to zoom in when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_zoomin),

    //!
    // Key to zoom out when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_zoomout),

    //!
    // Key to zoom out the maximum amount when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_maxzoom),

    //!
    // Key to toggle follow mode when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_follow),

    //!
    // Key to set a mark when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_mark),

    //!
    // Key to clear all marks when in the map view.
    //

    CONFIG_VARIABLE_INT(joybmap_clearmark),

    //!
    // Key to select weapon 1.
    //

    CONFIG_VARIABLE_KEY(key_weapon1),

    //!
    // Key to select weapon 2.
    //

    CONFIG_VARIABLE_KEY(key_weapon2),

    //!
    // Key to select weapon 3.
    //

    CONFIG_VARIABLE_KEY(key_weapon3),

    //!
    // Key to select weapon 4.
    //

    CONFIG_VARIABLE_KEY(key_weapon4),

    //!
    // Key to select weapon 5.
    //

    CONFIG_VARIABLE_KEY(key_weapon5),

    //!
    // Key to select weapon 6.
    //

    CONFIG_VARIABLE_KEY(key_weapon6),

    //!
    // Key to select weapon 7.
    //

    CONFIG_VARIABLE_KEY(key_weapon7),

    //!
    // Key to select weapon 8.
    //

    CONFIG_VARIABLE_KEY(key_weapon8),

    //!
    // Key to cycle to the previous weapon.
    //

    CONFIG_VARIABLE_KEY(key_prevweapon),

    //!
    // Key to cycle to the next weapon.
    //

    CONFIG_VARIABLE_KEY(key_nextweapon),

    //!
    // @game hexen
    //
    // Key to use one of each artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_all),

    //!
    // @game hexen
    //
    // Key to use "quartz flask" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_health),

    //!
    // @game hexen
    //
    // Key to use "flechette" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_poisonbag),

    //!
    // @game hexen
    //
    // Key to use "disc of repulsion" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_blastradius),

    //!
    // @game hexen
    //
    // Key to use "chaos device" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_teleport),

    //!
    // @game hexen
    //
    // Key to use "banishment device" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_teleportother),

    //!
    // @game hexen
    //
    // Key to use "porkalator" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_egg),

    //!
    // @game hexen
    //
    // Key to use "icon of the defender" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_invulnerability),

    //!
    // Key to re-display last message.
    //

    CONFIG_VARIABLE_KEY(key_message_refresh),

    //!
    // Key to quit the game when recording a demo.
    //

    CONFIG_VARIABLE_KEY(key_demo_quit),

    //!
    // Key to send a message during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msg),

    //!
    // Key to send a message to player 1 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer1),

    //!
    // Key to send a message to player 2 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer2),

    //!
    // Key to send a message to player 3 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer3),

    //!
    // Key to send a message to player 4 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer4),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 5 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer5),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 6 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer6),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 7 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer7),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 8 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer8),

    //
    // [SVE] svillarreal - add rb variables here
    //

    ADD_RB_VARIABLES()
};

static default_collection_t extra_defaults =
{
    extra_defaults_list,
    arrlen(extra_defaults_list),
    NULL,
};

// haleyjd 20141008: [SVE] variables we allow to be manipulated, but are not saved in
// the configuration file.
static default_t unsaved_defaults_list[] =
{
    CONFIG_VARIABLE_INT(deathmatch),
    CONFIG_VARIABLE_INT(fastparm),
    CONFIG_VARIABLE_INT(nomonsters),
    CONFIG_VARIABLE_INT(randomparm),
    CONFIG_VARIABLE_INT(respawnparm),
    CONFIG_VARIABLE_INT(startmap),
    CONFIG_VARIABLE_INT(startskill),
    CONFIG_VARIABLE_INT(timelimit),
    CONFIG_VARIABLE_INT(fe_musicnum),
};

static default_collection_t unsaved_defaults =
{
    unsaved_defaults_list,
    arrlen(unsaved_defaults_list),
    NULL
};

// Search a collection for a variable

static default_t *SearchCollection(default_collection_t *collection, const char *name)
{
    int i;

    for (i=0; i<collection->numdefaults; ++i) 
    {
        if (!strcmp(name, collection->defaults[i].name))
        {
            return &collection->defaults[i];
        }
    }

    return NULL;
}

//
// [SVE] svillarreal
//

typedef struct keyname_s
{
    const char *name;
    int keycode;
} keyname_t;

static const keyname_t keynames[] =
{
    { "rightarrow",     KEY_RIGHTARROW },
    { "leftarrow",      KEY_LEFTARROW },
    { "uparrow",        KEY_UPARROW },
    { "downarrow",      KEY_DOWNARROW },
    { "escape",         KEY_ESCAPE },
    { "enter",          KEY_ENTER },
    { "tab",            KEY_TAB },
    { "space",          0x20 },
    { "f1",             KEY_F1 },
    { "f2",             KEY_F2 },
    { "f3",             KEY_F3 },
    { "f4",             KEY_F4 },
    { "f5",             KEY_F5 },
    { "f6",             KEY_F6 },
    { "f7",             KEY_F7 },
    { "f8",             KEY_F8 },
    { "f9",             KEY_F9 },
    { "f10",            KEY_F10 },
    { "f11",            KEY_F11 },
    { "f12",            KEY_F12 },
    { "backspace",      KEY_BACKSPACE },
    { "pause",          KEY_PAUSE },
    { "equals",         KEY_EQUALS },
    { "minus",          KEY_MINUS },
    { "rshift",         KEY_RSHIFT },
    { "rctrl",          KEY_RCTRL },
    { "ralt",           KEY_RALT },
    { "lalt",           KEY_LALT },
    { "capslock",       KEY_CAPSLOCK },
    { "numlock",        KEY_NUMLOCK },
    { "scrlck",         KEY_SCRLCK },
    { "prtscr",         KEY_PRTSCR },
    { "home",           KEY_HOME },
    { "end",            KEY_END },
    { "pgup",           KEY_PGUP },
    { "pgdn",           KEY_PGDN },
    { "ins",            KEY_INS },
    { "del",            KEY_DEL },
    { "keypad0",        KEYP_0 },
    { "keypad1",        KEYP_1 },
    { "keypad2",        KEYP_2 },
    { "keypad3",        KEYP_3 },
    { "keypad4",        KEYP_4 },
    { "keypad5",        KEYP_5 },
    { "keypad6",        KEYP_6 },
    { "keypad7",        KEYP_7 },
    { "keypad8",        KEYP_8 },
    { "keypad9",        KEYP_9 },
    { "keypad_divide",  KEYP_DIVIDE },
    { "keypad_plus",    KEYP_PLUS },
    { "keypad_minus",   KEYP_MINUS },
    { "keypad_mult",    KEYP_MULTIPLY },
    { "keypad_period",  KEYP_PERIOD },
    { "keypad_equals",  KEYP_EQUALS },
    { "keypad_enter",   KEYP_ENTER }
};

//
// GetNameForKey
//

const char *GetNameForKey(const int key)
{
    int i;
    static char keychar[2];
    
    for(i = 0; i < arrlen(keynames); ++i)
    {
        if(keynames[i].keycode == key)
        {
            return keynames[i].name;
        }
    }
    
    if(isprint(key & 0x7f))
    {
        keychar[0] = key;
        return keychar;
    }
    
    return 0;
}

//
// GetKeyForName
//

const int GetKeyForName(const char *name)
{
    int i;
    
    for(i = 0; i < arrlen(keynames); ++i)
    {
        if(!strcmp(name, keynames[i].name))
        {
            return keynames[i].keycode;
        }
    }
    
    if(isprint(name[0]&0x7f))
    {
        return name[0];
    }
    
    return 0;
}


static void SaveDefaultCollection(default_collection_t *collection)
{
    default_t *defaults;
    char *keyname;
    int i, v;
    FILE *f;
    void *location; // haleyjd: [SVE]
	
    f = fopen (collection->filename, "w");
    if (!f)
	return; // can't write the file, but don't complain

    defaults = collection->defaults;
		
    for (i=0 ; i<collection->numdefaults ; i++)
    {
        int chars_written;

        // Ignore unbound variables

        if (!defaults[i].bound)
        {
            continue;
        }

        // Print the name and line up all values at 30 characters

        chars_written = fprintf(f, "%s ", defaults[i].name);

        for (; chars_written < 30; ++chars_written)
            fprintf(f, " ");

        // [SVE] save the value from default, if it is set.
        location = defaults[i].default_location ? 
            defaults[i].default_location : defaults[i].location;

        // Print the value

        switch (defaults[i].type) 
        {
            case DEFAULT_KEY:
                v = * (int *) location;
                keyname = (char*)GetNameForKey(v);
                if(keyname)
                {
                    fprintf(f, "\"%s\"", keyname);
                    break;
                }
                else
                {
                    fprintf(f, "\"\"");
                    break;
                }
                break;

            case DEFAULT_INT:
	        fprintf(f, "%i", * (int *) location);
                break;

            case DEFAULT_INT_HEX:
	        fprintf(f, "0x%x", * (int *) location);
                break;

            case DEFAULT_FLOAT:
                fprintf(f, "%f", * (float *) location);
                break;

            case DEFAULT_STRING:
	        fprintf(f,"\"%s\"", * (char **) location);
                break;
        }

        fprintf(f, "\n");
    }

    fclose (f);
}

// Parses integer values in the configuration file

static int ParseIntParameter(const char *strparm)
{
    int parm;

    if (strparm[0] == '0' && strparm[1] == 'x')
        sscanf(strparm+2, "%x", &parm);
    else
        sscanf(strparm, "%i", &parm);

    return parm;
}

static void SetVariable(default_t *def, const char *value, int flags)
{
    // parameter found
    void *location     = (flags & M_CFG_SETCURRENT) ? def->location : NULL;
    void *def_location = (flags & M_CFG_SETDEFAULT) ? def->default_location : NULL;


    switch (def->type)
    {
        case DEFAULT_STRING:
            if(location)
                * (char **) location = M_Strdup(value);
            if(def_location)
                * (char **) def_location = M_Strdup(value);
            break;

        case DEFAULT_INT:
        case DEFAULT_INT_HEX:
            {
                int val = ParseIntParameter(value);
                if(location)
                    * (int *) location = val;
                if(def_location)
                    * (int *) def_location = val;
            }
            break;

        case DEFAULT_KEY:
            {
                int key = GetKeyForName(value);
                if(location)
                    * (int *) location = key;
                if(def_location)
                    * (int *) def_location = key;
            }
            break;

        case DEFAULT_FLOAT:
            {
                float val = (float) atof(value);
                if(location)
                    * (float *) location = val;
                if(def_location)
                    * (float *) def_location = val;
            }
            break;
    }
}

static boolean LoadDefaultCollection(default_collection_t *collection)
{
    FILE *f;
    default_t *def;
    char defname[80];
    char strparm[100];

    // read the file in, overriding any set defaults
    f = fopen(collection->filename, "r");

    if (f == NULL)
    {
        // File not opened, but don't complain. 
        // It's probably just the first time they ran the game.

        return false;
    }

    while (!feof(f))
    {
        if (fscanf(f, "%79s %99[^\n]\n", defname, strparm) != 2)
        {
            // This line doesn't match

            continue;
        }

        // Find the setting in the list

        def = SearchCollection(collection, defname);

        if (def == NULL || !def->bound)
        {
            // Unknown variable?  Unbound variables are also treated
            // as unknown.

            continue;
        }

        // Strip off trailing non-printable characters (\r characters
        // from DOS text files)

        while (strlen(strparm) > 0 && !isprint(strparm[strlen(strparm)-1]))
        {
            strparm[strlen(strparm)-1] = '\0';
        }

        // Surrounded by quotes? If so, remove them.
        if (strlen(strparm) >= 2
         && strparm[0] == '"' && strparm[strlen(strparm) - 1] == '"')
        {
            strparm[strlen(strparm) - 1] = '\0';
            memmove(strparm, strparm + 1, sizeof(strparm) - 1);
        }

        SetVariable(def, strparm, M_CFG_SETALL);
    }

    fclose (f);
    return true;
}

// Set the default filenames to use for configuration files.

void M_SetConfigFilenames(char *main_config, char *extra_config)
{
    default_main_config = main_config;
    default_extra_config = extra_config;
}

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
    SaveDefaultCollection(&doom_defaults);
    SaveDefaultCollection(&extra_defaults);

    // dimitrisg 20200629 : commit save data to NX
    I_FlushSaves();
}

//
// Save defaults to alternate filenames
//

void M_SaveDefaultsAlternate(char *main, char *extra)
{
    char *orig_main;
    char *orig_extra;

    // Temporarily change the filenames

    orig_main = doom_defaults.filename;
    orig_extra = extra_defaults.filename;

    doom_defaults.filename = main;
    extra_defaults.filename = extra;

    M_SaveDefaults();

    // Restore normal filenames

    doom_defaults.filename = orig_main;
    extra_defaults.filename = orig_extra;
}

//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
    int i;
 
    // check for a custom default file

    //!
    // @arg <file>
    // @vanilla
    //
    // Load main configuration from the specified file, instead of the
    // default.
    //

    i = M_CheckParmWithArgs("-config", 1);

    if (i)
    {
	doom_defaults.filename = myargv[i+1];
	printf ("	default file: %s\n",doom_defaults.filename);
    }
    else
    {
        doom_defaults.filename
            = M_StringJoin(configdir, default_main_config, NULL);
    }

    printf("saving config in %s\n", doom_defaults.filename);

    //!
    // @arg <file>
    //
    // Load additional configuration from the specified file, instead of
    // the default.
    //

    i = M_CheckParmWithArgs("-extraconfig", 1);

    if (i)
    {
        extra_defaults.filename = myargv[i+1];
        printf("        extra configuration file: %s\n", 
               extra_defaults.filename);
    }
    else
    {
        extra_defaults.filename
            = M_StringJoin(configdir, default_extra_config, NULL);
    }

    if(!LoadDefaultCollection(&doom_defaults))
        config_fresh = true;

    if(!LoadDefaultCollection(&extra_defaults))
        extra_config_fresh = true;
}

// Get a configuration file variable by its name

static default_t *GetDefaultForName(const char *name)
{
    default_t *result;

    // Try the main list and the extras

    result = SearchCollection(&doom_defaults, name);

    if (result == NULL)
    {
        result = SearchCollection(&extra_defaults, name);
    }

    // haleyjd 20141008: try internal list
    if (result == NULL)
    {
        result = SearchCollection(&unsaved_defaults, name);
    }

    // Not found? Internal error.

    if (result == NULL)
    {
        I_Error("Unknown configuration variable: '%s'", name);
    }

    return result;
}

//
// Bind a variable to a given configuration file variable, by name.
//

void M_BindVariable(char *name, void *location)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    variable->location = location;
    variable->bound = true;
}

// Set the value of a particular variable; an API function for other
// parts of the program to assign values to config variables by name.

boolean M_SetVariable(const char *name, const char *value)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound)
    {
        return false;
    }

    SetVariable(variable, value, M_CFG_SETALL);

    return true;
}

// Get the value of a variable.

int M_GetIntVariable(const char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || (variable->type != DEFAULT_INT && 
         variable->type != DEFAULT_INT_HEX &&
         variable->type != DEFAULT_KEY)) // haleyjd: [SVE]
    {
        return 0;
    }

    return *((int *) variable->location);
}

const char *M_GetStrVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_STRING)
    {
        return NULL;
    }

    return *((const char **) variable->location);
}

float M_GetFloatVariable(const char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_FLOAT)
    {
        return 0;
    }

    return *((float *) variable->location);
}

#if defined(__APPLE__)
#define SVE_APPLE_PATH "~/Library/Application Support/com.nightdivestudios.Strife"
#define SVE_APPLE_PATH_S SVE_APPLE_PATH DIR_SEPARATOR_S
#endif

#if !defined(_WIN32) && defined(I_APPSERVICES_OLDCFGDIR)
//
// Tries to fix SVE's old incorrect save location on Linux and Apple 
// (~/strife-ve/) when using Steam.
//
static void MoveOldCfgDir(void)
{
    char *dest = NULL;
    char *homedir;

    // Determine the path we *want* to use
#if defined(__APPLE__)
    // Mac OS X
    dest = M_Strdup(SVE_APPLE_PATH);
#else
    // Linux
    dest = getenv("XDG_DATA_HOME");
    if(dest != NULL && dest[0] != '\0')
    {
        dest = M_StringJoin(dest, "/" PACKAGE_TARNAME, NULL);
    }
    else
    {
        // Default to $HOME/.local/share if $XDG_DATA_HOME isn't set or is empty.
        dest = getenv("HOME");
        if(dest != NULL)
        {
            dest = M_StringJoin(dest, "/.local/share/" PACKAGE_TARNAME, NULL);
        }
    }
    if(!dest)
        return;
#endif

    // Get the path we *were* using, and try to move it.
    homedir = getenv("HOME");
    if(homedir != NULL)
    {
        char *src = M_StringJoin(homedir, "/" PACKAGE_TARNAME, NULL);
        struct stat st;

        // if this folder exists, try to rename it
        if(!stat(src, &st) && S_ISDIR(st.st_mode))
        {
            rename(src, dest);
        }

        free(src);
    }

    free(dest);
}
#endif

// Get the path to the default configuration dir to use, if NULL
// is passed to M_SetConfigDir.

static char *GetDefaultConfigDir(void)
{
    // [SVE] dotfloat 20141216
#if !defined(_WIN32)
    char *datadir = NULL;
    struct stat st;

#if defined(I_APPSERVICES_OLDCFGDIR)
    // For compatibility's sake, SVE needs to keep preferentially looking in an
    // incorrect location on Linux and Apple (~/strife-ve/) when using Steam - we
    // will only use that directory if it already exists

    // Try to rename it first.
    MoveOldCfgDir();

    datadir = getenv("HOME");
    if(datadir != NULL)
    {
        // put all configuration in a config directory off the
        // homedir
        char *res1 = M_StringJoin(datadir, "/" PACKAGE_TARNAME, NULL);

        // if this folder exists, use it; otherwise, go on below.
        if(!stat(res1, &st) && S_ISDIR(st.st_mode))
        {
            return M_StringJoin(res1, "/", NULL);
        }
        else
            free(res1);
    }
#endif

#ifdef SVE_PLAT_SWITCH
    // dimitrisg : 20201506 use the default mount path for NX
    return "save://";
#endif

#if defined(__APPLE__)
    // On Apple, try to create a folder under "~/Library/Application Support".
    // This is not guaranteed to work, and there is no way to get the proper path when
    // it won't without use of Objective-C code.

    if((!stat(SVE_APPLE_PATH, &st) && S_ISDIR(st.st_mode)) || // already exists?
       M_MakeDirectory(SVE_APPLE_PATH))                       // created successfully?
    {
        return M_Strdup(SVE_APPLE_PATH_S);
    }
    else
#elif defined(__linux__) || defined(__OpenBSD__)
    // Linux defaults
    // Configuration settings are stored in $XDG_DATA_HOME instead of $HOME on Linux.

    datadir = getenv("XDG_DATA_HOME");
    if(datadir != NULL && datadir[0] != '\0')
    {
        return M_StringJoin(datadir, "/" PACKAGE_TARNAME "/", NULL);
    }

    // Default to $HOME/.local/share if $XDG_DATA_HOME isn't set or is empty.
    datadir = getenv("HOME");
    if(datadir != NULL)
    {
        return M_StringJoin(datadir, "/.local/share/" PACKAGE_TARNAME "/", NULL);
    }
    else
#endif // #elif defined(__linux__)
#else // #if !defined(_WIN32)
#if defined(LUNA_RELEASE)
	// Luna needs a standard savegame directory.
	{
		char *pref = SDL_GetPrefPath("Nightdive Studios", "Strife");
		if (pref == NULL)
		{
			I_Error("Could not create user directory.");
		}

		// Switch to the proper allocator.
		char *dup = M_Strdup(pref);
		SDL_free(pref);

		return dup;
	}
#else
	{
		return M_Strdup("");
	}
#endif // #if defined(LUNA_RELEASE)
#endif // #if !defined(_WIN32)
}

// 
// SetConfigDir:
//
// Sets the location of the configuration directory, where configuration
// files are stored - default.cfg, chocolate-doom.cfg, savegames, etc.
//

void M_SetConfigDir(char *dir)
{
    // Use the directory that was passed, or find the default.

    if (dir != NULL)
    {
        configdir = dir;
    }
    else
    {
        configdir = GetDefaultConfigDir();
    }


    if (strcmp(configdir, "") != 0)
    {
        printf("Using %s for configuration and saves\n", configdir);
    }

#ifndef SVE_PLAT_SWITCH
    // Make the directory if it doesn't already exist:
    M_MakeDirectory(configdir);
#endif
}

//
// Calculate the path to the directory to use to store save games.
// Creates the directory as necessary.
//

char *M_GetSaveGameDir(char *iwadname)
{
    char *savegamedir;
    char *topdir;

    // If not "doing" a configuration directory (Windows), don't "do"
    // a savegame directory, either.

    if (!strcmp(configdir, ""))
    {
	savegamedir = M_Strdup("");
    }
    else
    {
        // ~/.chocolate-doom/savegames
#ifdef SVE_PLAT_SWITCH
		// dimitrisg 20201506 
		topdir = M_StringJoin("save://", "savegames", NULL);
#else
        topdir = M_StringJoin(configdir, "savegames", NULL);
#endif
        M_MakeDirectory(topdir);

        // eg. ~/.chocolate-doom/savegames/doom2.wad/

        savegamedir = M_StringJoin(topdir, DIR_SEPARATOR_S, iwadname,
                                   DIR_SEPARATOR_S, NULL);

        M_MakeDirectory(savegamedir);

        free(topdir);
    }

    return savegamedir;
}

// edward [SVE]: new function for dealing with the temp save directory

char *M_GetTmpSaveGameDir(char *iwadname)
{
    char *savegamedir;
    char *topdir;

    // If not "doing" a configuration directory (Windows), don't "do"
    // a savegame directory, either.

    if (!strcmp(configdir, ""))
    {
        savegamedir = M_Strdup("");
    }
    else
    {
        // ~/.chocolate-doom/savegames
#ifdef SVE_PLAT_SWITCH
        // dimitrisg 20201506 
        topdir = M_StringJoin(I_TempSaveDir(), "savegames", NULL);
#else
        topdir = M_StringJoin(configdir, "savegames", NULL);
#endif
        M_MakeDirectory(topdir);

        // eg. ~/.chocolate-doom/savegames/doom2.wad/

        savegamedir = M_StringJoin(topdir, DIR_SEPARATOR_S, iwadname,
            DIR_SEPARATOR_S, NULL);

        M_MakeDirectory(savegamedir);

        free(topdir);
}

    return savegamedir;
}

// haleyjd [SVE]: new functions for dealing with defaults

//
// Bind a variable providing a default backup location along with the primary address
//
void M_BindVariableWithDefault(char *name, void *location, void *default_location)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    variable->location = location;
    variable->default_location = default_location;
    variable->bound = true;
}

//
// Set any of the current location, default location, or both locations depending
// on the flag values (any combination of M_CFG_SETCURRENT or M_CFG_SETDEFAULT)
//
boolean M_SetVariableByFlags(const char *name, const char *value, int flags)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound)
    {
        return false;
    }

    SetVariable(variable, value, flags); // pass on flags

    return true;
}

//
// Get the value in the variable's default location, if it has one; otherwise the
// current value is returned.
//
int M_GetIntVariableDefault(const char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || (variable->type != DEFAULT_INT && 
         variable->type != DEFAULT_INT_HEX &&
         variable->type != DEFAULT_KEY)) // haleyjd: [SVE]
    {
        return 0;
    }

    if(variable->default_location)
        return *((int *) variable->default_location);
    else
        return *((int *) variable->location);
}

//
// Get the value in the variable's default location, if it has one; otherwise the
// current value is returned.
//
const char *M_GetStrVariableDefault(const char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_STRING)
    {
        return NULL;
    }

    if(variable->default_location)
        return *((const char **) variable->default_location);
    else
        return *((const char **) variable->location);
}

//
// Get the value in the variable's default location, if it has one; otherwise the
// current value is returned.
//
float M_GetFloatVariableDefault(const char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_FLOAT)
    {
        return 0;
    }

    if(variable->default_location)
        return *((float *) variable->default_location);
    else
        return *((float *) variable->location);
}

// EOF


