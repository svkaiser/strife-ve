#!/bin/sh

GAMEROOT=$(cd "${0%/*}" && echo $PWD)

if [ -n "${XDG_DATA_HOME}" ]; then
  DATADIR="${XDG_DATA_HOME}/strife-ve"
else
  DATADIR="${HOME}/.local/share/strife-ve"
fi

# move ~/strife-ve to ~/.local/share/
if [ ! -d "$DATADIR" ] && [ -d "${HOME}/strife-ve" ]; then
  mkdir -p $DATADIR

  cd "${HOME}/strife-ve"
  [ -f "chocolate-strife.cfg" ] && mv chocolate-strife.cfg "${DATADIR}/"
  [ -f "strife.cfg" ]           && mv strife.cfg "${DATADIR}/"
  [ -d "savegames" ]            && mv savegames "${DATADIR}/"
  rmdir "${HOME}/strife-ve" # will fail if the directory is non-empty
fi

export LD_LIBRARY_PATH="${GAMEROOT}/:${LD_LIBRARY_PATH}"

cd "$GAMEROOT"
$GAMEROOT/strife-ve "$@"
