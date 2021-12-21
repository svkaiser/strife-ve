#!/bin/sh

GAMEROOT=$(cd "${0%/*}" && echo $PWD)

export LD_LIBRARY_PATH="${GAMEROOT}/:${LD_LIBRARY_PATH}"

cd "$GAMEROOT"
"$GAMEROOT/strife-ve" "$@"
