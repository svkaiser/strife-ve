##------------------------------------------------------------------------------
##
## SVE CMake configuration
##
## LICENSE:
##
## Copyright(C) 2014 Zohar Malamant
## Copyright(C) 2020 Alex Mayfield
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.
##

include(ExternalProject)

# [AM] Needed for target_link_directories
cmake_minimum_required(VERSION 3.13)

project(strife-ve)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(SOURCE_ROOT_DIR "${CMAKE_SOURCE_DIR}")

set(PACKAGE_NAME "strife-ve")
set(PACKAGE_STRING "Strife-VE 1.3")
set(PACKAGE_TARNAME "strife-ve")
set(PROGRAM_PREFIX "chocolate-")

##------------------------------------------------------------------------------
## Options
##

option(ENABLE_STEAM "Build with Steamworks support." OFF)
option(ENABLE_GOG "Build GOG version for Linux." OFF)
option(ENABLE_LUNA "Build with Luna support" OFF)

if(ENABLE_STEAM)
   set(ENABLE_STEAM_RUNTIME ON)
   set(ENABLE_STEAM_SERVICE ON)
elseif(ENABLE_GOG)
   set(ENABLE_STEAM_RUNTIME ON)
   set(ENABLE_STEAM_SERVICE OFF)
endif()

# If building for Steam, steam_api, FFMPEG, and SDL_net are all in the local dir.
# Same deal for GOG build
if(ENABLE_STEAM_RUNTIME)
   set(CMAKE_INSTALL_RPATH "\$ORIGIN")
   set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif()

##------------------------------------------------------------------------------
## CMake functions
##

function(add_sources PREFIX)
	unset(NEW_SOURCES)
	foreach(ARG ${ARGN})
		set(SOURCE "${SOURCE_ROOT_DIR}/${PREFIX}/${ARG}")
		list(APPEND NEW_SOURCES "${SOURCE}")
	endforeach()
	set(SOURCES ${SOURCES} ${NEW_SOURCES} PARENT_SCOPE)
	include_directories("${SOURCE_ROOT_DIR}/${PREFIX}")
endfunction()

function(add_link_libraries)
	unset(NEW_LIBRARIES)
	foreach(ARG ${ARGN})
		list(APPEND NEW_LIBRARIES ${ARG})
	endforeach()
	set(LIBRARIES ${LIBRARIES} ${NEW_LIBRARIES} PARENT_SCOPE)
endfunction()

##------------------------------------------------------------------------------
## Set up external dependencies
##

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Set up a standard include/lib directory where any dependencies built by
# this CMake buildfile can shove their libraries into.
set(STRIFE_LOCAL "${CMAKE_CURRENT_BINARY_DIR}/local")
make_directory("${STRIFE_LOCAL}/include")
include_directories("${STRIFE_LOCAL}/include")
link_directories("${STRIFE_LOCAL}/lib")

# Use prebuilt versions of libraries if none has been passed.
if(WIN32)
   if(NOT SDL2_INCLUDE_DIR AND NOT SDL2_LIBRARY)
      message(STATUS "Using internal SDL2")

      file(DOWNLOAD
         "https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip"
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
         EXPECTED_HASH SHA256=f83651227229e059a570aac26be24f5070352c0d23aaf3d2cfbd3eb2c9599334)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.16")
      set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include" CACHE PATH "")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(SDL2_LIBRARY "${SDL2_DIR}/lib/x64/SDL2.lib" CACHE FILEPATH "")
         set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x64/SDL2main.lib" CACHE FILEPATH "")
      else()
         set(SDL2_LIBRARY "${SDL2_DIR}/lib/x86/SDL2.lib" CACHE FILEPATH "")
         set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x86/SDL2main.lib" CACHE FILEPATH "")
      endif()
   endif()

   if(NOT SDL2_MIXER_INCLUDE_DIR AND NOT SDL2_MIXER_LIBRARY)
      message(STATUS "Using internal SDL2_mixer")

      file(DOWNLOAD
         "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip"
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
         EXPECTED_HASH SHA256=258788438b7e0c8abb386de01d1d77efe79287d9967ec92fbb3f89175120f0b0)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4")
      set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include" CACHE PATH "")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x64/SDL2_mixer.lib" CACHE FILEPATH "")
      else()
         set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x86/SDL2_mixer.lib" CACHE FILEPATH "")
      endif()
   endif()

   if(NOT SDL2_NET_INCLUDE_DIR AND NOT SDL2_NET_LIBRARY)
      message(STATUS "Using internal SDL2_net")

      file(DOWNLOAD
         "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-devel-2.0.1-VC.zip"
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2_net-VC.zip"
         EXPECTED_HASH SHA256=c1e423f2068adc6ff1070fa3d6a7886700200538b78fd5adc36903a5311a243e)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
         "${CMAKE_CURRENT_BINARY_DIR}/SDL2_net-VC.zip"
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      set(SDL2_NET_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_net-2.0.1")
      set(SDL2_NET_INCLUDE_DIR "${SDL2_NET_DIR}/include" CACHE PATH "")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(SDL2_NET_LIBRARY "${SDL2_NET_DIR}/lib/x64/SDL2_net.lib" CACHE FILEPATH "")
      else()
         set(SDL2_NET_LIBRARY "${SDL2_NET_DIR}/lib/x86/SDL2_net.lib" CACHE FILEPATH "")
      endif()
   endif()
endif()

# SDL2
find_package(SDL2 REQUIRED)

# SDL2_mixer
find_package(SDL2_mixer REQUIRED)

# SDL2_net
find_package(SDL2_net REQUIRED)

# TheoraPlay - Ogg
find_package(OGG REQUIRED)

# TheoraPlay - Theora
find_package(THEORA REQUIRED)

# TheoraPlay - Vorbis
find_package(VORBIS REQUIRED)

# Option to use in-tree libpng
if(PNG_STATIC)
   include_directories(${CMAKE_SOURCE_DIR}/lpng1612)
   link_directories(${CMAKE_SOURCE_DIR}/lpng1612/build)
else()
   # zlib
   find_package(ZLIB REQUIRED)

   # libpng
   find_package(PNG REQUIRED)

   include_directories(${PNG_INCLUDE_DIRS})
   add_link_libraries(${PNG_LIBRARIES})
endif()

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
add_link_libraries(${OPENGL_LIBRARIES})

##------------------------------------------------------------------------------
## Configure "config.h"
##

include_directories("${CMAKE_BINARY_DIR}")

configure_file("${CMAKE_MODULE_PATH}/config.h.in"
               "${CMAKE_BINARY_DIR}/config.h")

##------------------------------------------------------------------------------
## Platform-specific sources
##

# Libraries when compiling for Steam.
if(ENABLE_STEAM_SERVICE)
	set(STEAMWORKS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../steamworks_sdk_150/"
		CACHE PATH "Path to the root directory of the Steamworks SDK")

	if(NOT EXISTS "${STEAMWORKS_DIR}")
		message(FATAL_ERROR
			"SteamService is enabled, but \"${STEAMWORKS_DIR}\" does not exist\n"
			"Please ensure that the STEAMWORKS_DIR setting points to an extracted "
			"Steamworks SDK directory.")
	endif()

	set(STEAM_SERVICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../steam-service/api/SteamService"
		CACHE PATH "Path to the root directory of SteamService, containing a CMakeLists.txt")

	if(NOT EXISTS "${STEAM_SERVICE_DIR}/CMakeLists.txt")
		message(FATAL_ERROR "SteamService is enabled, but \"${STEAM_SERVICE_DIR}\" does not exist")
	endif()

	# Compile SteamService as an external project on build.
	ExternalProject_Add(
		steam-service-project
		# Point to an existing source checkout
		SOURCE_DIR "${STEAM_SERVICE_DIR}"
		# Skip the download step
		DOWNLOAD_COMMAND ""
		# Configure step points to the Steamworks SDK and installs it
		# someplace where we can find it later.
		CMAKE_ARGS
			"-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
			"-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
			"-DCMAKE_LINKER=${CMAKE_LINKER}"
			"-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
			"-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
			"-DSTEAMWORKS_DIR=${STEAMWORKS_DIR}"
			"-DCMAKE_INSTALL_PREFIX=${STRIFE_LOCAL}"
		BUILD_BYPRODUCTS
			"${STRIFE_LOCAL}/lib/libSteamService.so"
	)

	# Linux
	if(NOT APPLE AND NOT WIN32)
		add_library(SteamService INTERFACE IMPORTED)
		add_dependencies(SteamService steam-service-project)
		target_compile_definitions(SteamService INTERFACE _USE_STEAM_)
		target_link_directories(SteamService INTERFACE
			"${STRIFE_LOCAL}/lib/"
			"${STEAMWORKS_DIR}/sdk/redistributable_bin/linux64"
		)
		target_link_libraries(SteamService INTERFACE steam_api SteamService.so)
	else()
		message(FATAL_ERROR "This platform is not implemented")
	endif()
endif()

##------------------------------------------------------------------------------
## Sources
##

# opl
add_sources(opl
	opl.c
	opl_linux.c
	opl_obsd.c
	opl_queue.c
	opl_sdl.c
	opl_timer.c
	ioperm_sys.c
	dbopl.c
)

# textscreen
add_sources(textscreen
	txt_button.c
	txt_checkbox.c
	txt_desktop.c
	txt_dropdown.c
	txt_fileselect.c
	txt_gui.c
	txt_inputbox.c
	txt_io.c
	txt_label.c
	txt_radiobutton.c
	txt_scrollpane.c
	txt_sdl.c
	txt_separator.c
	txt_spinctrl.c
	txt_strut.c
	txt_table.c
	txt_utf8.c
	txt_widget.c
	txt_window.c
	txt_window_action.c
)

# pcsound
add_sources(pcsound
	pcsound.c
	pcsound_linux.c
	pcsound_sdl.c
)

# src
add_sources(src
	d_event.c
	d_event.h
	d_iwad.c
	d_iwad.h
	d_loop.c
	d_loop.h
	d_mode.c
	d_mode.h
	d_ticcmd.h

	deh_defs.h
	deh_io.c
	deh_io.h
	deh_main.c
	deh_main.h
	deh_mapping.c
	deh_mapping.h
	deh_str.c
	deh_str.h
	deh_text.c

	doomfeatures.h
	doomkeys.h
	doomtype.h

	gusconf.c
	gusconf.h

#	i_cdmus.c
#	i_cdmus.h
	i_cpumode.c
	i_cpumode.h
	i_endoom.c
	i_endoom.h
	i_galaxyservices.c
	i_galaxyservices.h
	i_glscale.c
	i_glscale.h
	i_joystick.c
	i_joystick.h
	i_main.c
	i_noappservices.c
	i_noappservices.h
	i_oplmusic.c
	i_pcsound.c
	i_platsystem.c
	i_platsystem.h
	i_scale.c
	i_scale.h
	i_sdlmusic.c
	i_sdlsound.c
	i_social.c
	i_social.h
	i_softkey.c
	i_softkey.h
	i_sound.c
	i_sound.h
	i_steamservices.c
	i_steamservices.h
	i_system.c
	i_system.h
	i_theoraplay.c
	i_theoraplay.h
	i_timer.c
	i_timer.h
	i_video.c
	i_video.h
#	i_videohr.c
#	i_videohr.h

	icon.c
	kerning.c
	kerning.h

	m_argv.c
	m_argv.h
	m_bbox.c
	m_bbox.h
	m_cheat.c
	m_cheat.h
	m_config.c
	m_config.h
	m_controls.c
	m_controls.h
	m_dllist.h
	m_fixed.c
	m_fixed.h
	m_misc.c
	m_misc.h
	m_parser.c
	m_parser.h
	m_qstring.c
	m_qstring.h

	memio.c
	memio.h

	midifile.c
	midifile.h
	mus2mid.c
	mus2mid.h

	net_client.c
	net_client.h
	net_common.c
	net_common.h
	net_dedicated.c
	net_defs.h
	net_gui.c
	net_gui.h
	net_io.c
	net_io.h
	net_loop.c
	net_loop.h
	net_packet.c
	net_packet.h
	net_query.c
	net_query.h
	net_sdl.c
	net_sdl.h
	net_server.c
	net_server.h
	net_steamworks.c
	net_steamworks.h
	net_structrw.c
	net_structrw.h

	sha1.c
	sha1.h
	tables.c
	tables.h

	v_patch.h
	v_video.c

	w_checksum.c
	w_file.c
	w_file.h
	w_file_posix.c
	w_file_stdc.c
	w_main.c
	w_main.h
	w_merge.c
	w_merge.h
	w_wad.c
	w_wad.h

	z_zone.c
	z_zone.h
)

add_sources(src/opengl
	rb_automap.c
	rb_automap.h
	rb_bsp.c
	rb_bsp.h
	rb_clipper.c
	rb_clipper.h
	rb_config.c
	rb_config.h
	rb_data.c
	rb_data.h
	rb_decal.c
	rb_decal.h
	rb_draw.c
	rb_draw.h
	rb_drawlist.c
	rb_drawlist.h
	rb_dynlights.c
	rb_dynlights.h
	rb_fbo.c
	rb_fbo.h
	rb_geom.c
	rb_geom.h
	rb_gl.c
	rb_gl.h
	rb_hudtext.c
	rb_hudtext.h
	rb_lightgrid.c
	rb_lightgrid.h
	rb_main.c
	rb_main.h
	rb_matrix.c
	rb_matrix.h
	rb_patch.c
	rb_patch.h
	rb_shader.c
	rb_shader.h
	rb_sky.c
	rb_sky.h
	rb_texture.c
	rb_texture.h
	rb_things.c
	rb_things.h
	rb_view.c
	rb_view.h
	rb_wallshade.c
	rb_wallshade.h
	rb_wipe.c
	rb_wipe.h
)

add_sources(src/strife
	am_map.c
	am_map.h

	d_englsh.h
	d_items.c
	d_items.h
	d_main.c
	d_main.h
	d_net.c
	d_player.h
	d_textur.h
	d_think.h

	deh_ammo.c
	deh_cheat.c
	deh_frame.c
	deh_misc.c
	deh_ptr.c
	deh_sound.c
	deh_strife.c
	deh_thing.c
	deh_weapon.c

	doomdata.h
	doomdef.c
	doomstat.c

	dstrings.c
	dstrings.h

	f_finale.c
	f_finale.h
	f_wipe.c
	f_wipe.h

	fe_characters.c
	fe_characters.h
	fe_commands.c
	fe_commands.h
	fe_frontend.c
	fe_frontend.h
	fe_gamepad.c
	fe_gamepad.h
	fe_graphics.c
	fe_graphics.h
	fe_menuengine.c
	fe_menuengine.h
	fe_menus.c
	fe_menus.h
	fe_mouse.c
	fe_mouse.h
	fe_multiplayer.c
	fe_multiplayer.h

	g_game.c
	g_game.h
	hu_lib.c
	hu_lib.h
	hu_stuff.c
	hu_stuff.h

	info.c
	info.h

	m_help.c
	m_help.h
	m_menu.c
	m_menu.h
	m_random.c
	m_random.h
	m_saves.c
	m_saves.h

	p_ceilng.c
	p_dialog.c
	p_dialog.h
	p_doors.c
	p_enemy.c
	p_floor.c
	p_inter.c
	p_inter.h
	p_lights.c
	p_local.h
	p_locations.c
	p_locations.h
	p_map.c
	p_maputl.c
	p_mobj.c
	p_mobj.h
	p_plats.c
	p_pspr.c
	p_pspr.h
	p_saveg.c
	p_saveg.h
	p_setup.c
	p_setup.h
	p_sight.c
	p_spec.c
	p_spec.h
	p_switch.c
	p_telept.c
	p_tick.c
	p_tick.h
	p_user.c

	r_bsp.c
	r_bsp.h
	r_data.c
	r_data.h
	r_defs.h
	r_draw.c
	r_draw.h
	r_local.h
	r_main.c
	r_main.h
	r_plane.c
	r_plane.h
	r_segs.c
	r_segs.h
	r_sky.c
	r_sky.h
	r_state.h
	r_things.c
	r_things.h

	s_sound.c
	s_sound.h

	sounds.c
	sounds.h

	st_lib.c
	st_lib.h
	st_stuff.c
	st_stuff.h

	wi_stuff.c
	wi_stuff.h
)

##------------------------------------------------------------------------------
## Build target
##

include_directories(${SOURCE_ROOT_DIR})
add_executable(strife-ve ${SOURCES})
set_property(TARGET strife-ve PROPERTY C_STANDARD 99)
target_compile_definitions(strife-ve PUBLIC SVE_USE_THEORAPLAY)
target_link_libraries(strife-ve m Threads::Threads ${LIBRARIES}
	SDL2::SDL2main SDL2::SDL2 SDL2::mixer SDL2::net
	OGG::OGG THEORA::DEC VORBIS::VORBIS)
if(ENABLE_STEAM_SERVICE)
	target_link_libraries(strife-ve SteamService)
endif()
if(ENABLE_LUNA)
	target_compile_definitions(strife-ve PUBLIC SVE_USE_LUNA)
endif()

##------------------------------------------------------------------------------
## Copy libraries
##

if(NOT WIN32 AND NOT APPLE)
	set(STRIFE_DLLS
		"${CMAKE_CURRENT_SOURCE_DIR}/strife-ve.sh"
	)

	if(ENABLE_STEAM_SERVICE)
		list(APPEND STRIFE_DLLS
			"${STRIFE_LOCAL}/lib/libSteamService.so")
		list(APPEND STRIFE_DLLS
			"${STEAMWORKS_DIR}/sdk/redistributable_bin/linux64/libsteam_api.so")
	endif()

	foreach(STRIFE_DLL ${STRIFE_DLLS})
		# Copy our libraries and the start script
		add_custom_command(TARGET strife-ve POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			${STRIFE_DLL} "${CMAKE_CURRENT_BINARY_DIR}"
		)
	endforeach()
endif()

##------------------------------------------------------------------------------
## Install target
##
