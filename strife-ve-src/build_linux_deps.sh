#!/bin/bash
set -e

SDL_version=1.2.15
SDL_mixer_version=1.2.12
SDL_net_version=1.2.8
ffmpeg_version=2.5.4

if [ -d ./built_ext/ ]; then
	echo A directory named build_ext already exists.
	echo Please remove it if you want to recompile.
	exit
fi

mkdir ./build_ext/
cd ./build_ext/

install_dir=$(pwd)

function build_sdl {
	wget https://www.libsdl.org/release/SDL-${SDL_version}.tar.gz
	tar xvf SDL-${SDL_version}.tar.gz
	pushd SDL-${SDL_version}

	./configure --prefix=${install_dir} \
		--disable-video-directfb \
		--disable-input-tslib
	make
	make install

	popd
}

function build_sdl_mixer {
	wget https://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-${SDL_mixer_version}.tar.gz
	tar xvf SDL_mixer-${SDL_mixer_version}.tar.gz
	pushd SDL_mixer-${SDL_mixer_version}

	./configure --prefix=${install_dir} \
		--disable-music-ogg-shared
	make
	make install

	popd
}

function build_sdl_net {
	wget https://www.libsdl.org/projects/SDL_net/release/SDL_net-${SDL_net_version}.tar.gz
	tar xvf SDL_net-${SDL_net_version}.tar.gz
	pushd SDL_net-${SDL_net_version}

	./configure --prefix=${install_dir}
	make
	make install

	popd
}

function build_ffmpeg {
	wget http://ffmpeg.org/releases/ffmpeg-${ffmpeg_version}.tar.bz2
	tar xvf ffmpeg-${ffmpeg_version}.tar.bz2
	pushd ffmpeg-${ffmpeg_version}

	./configure --prefix=${install_dir} \
		--disable-all \
		--disable-yasm \
		--disable-debug \
		--enable-shared \
		--enable-avcodec \
		--enable-avdevice \
		--enable-avfilter \
		--enable-avformat \
		--enable-avutil \
		--enable-swresample \
		--enable-swscale \
		--enable-protocol=file \
		--enable-demuxer=ogg \
		--enable-decoder=theora \
		--enable-decoder=vorbis \
		--disable-hwaccels \
		--disable-vaapi        
	make
	make install

	popd
}

build_sdl
build_sdl_mixer
build_sdl_net
build_ffmpeg