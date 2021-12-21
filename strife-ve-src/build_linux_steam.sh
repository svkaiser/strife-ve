#!/usr/bin/env bash
#
# Copyright 2020 Alex Mayfield
#
# If you update this script, please run shellcheck on it first.
#

# Steam runtime version to use.
RUNTIME_VERISON=0.20201007.1

# steam-runtime commit hash to use for setup_chroot.sh.
CHROOT_COMMIT=5fc1299e8aecb569315ae0a625c57942be1d0b78

# http://redsymbol.net/articles/unofficial-bash-strict-mode/s
set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR=$(dirname "$0")

function install_chroot {
    # See if we already have a steam chroot installed.
    if [[ -d /var/chroots/steamrt_scout_amd64/bin ]]; then
        echo "==> Steam chroot detected, skipping installation..."
        return 0
    fi

    echo "==> Steam chroot not detected, installing..."

    mkdir -p "$SCRIPT_DIR/build_schroot"
    cd "$SCRIPT_DIR/build_schroot"

    # Download the sysroot.
    if [[ ! -f com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.tar.gz ]]; then
        echo "==> Downloading Steam Runtime SDK sysroot..."
        curl -LO 'https://repo.steampowered.com/steamrt-images-scout/snapshots/'"${RUNTIME_VERISON}"'/com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.tar.gz'
    fi

    # Download the setup script.
    if [[ ! -f setup_chroot.sh ]]; then
        echo "==> Downloading chroot setup..."
        curl -LO 'https://raw.githubusercontent.com/ValveSoftware/steam-runtime/'"${CHROOT_COMMIT}"'/setup_chroot.sh'
        chmod +x setup_chroot.sh
    fi

    # Setup the chroot.
    ./setup_chroot.sh --amd64 --tarball com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.tar.gz
}

function build_game {
    echo "==> Configuring build..."
    schroot --chroot steamrt_scout_amd64 -- \
        mkdir -p build_steam && cd build_steam && \
        cmake .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_STEAM=On && \
        echo "==> Building..." && \
        cmake --build . --parallel
}

install_chroot
build_game
