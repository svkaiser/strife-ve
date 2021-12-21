#!/usr/bin/env bash
#
# Copyright 2020 Alex Mayfield
#
# If you update this script, please run shellcheck on it first.
#

function build_game {
    echo "==> Configuring build..."
    mkdir -p build && cd build && \
        cmake .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_STEAM=Off

    echo "==> Building..."
    cmake --build . --parallel
}

build_game
