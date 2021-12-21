# ymfmidi

ymfmidi is a MIDI player based on the OPL3 emulation core from [ymfm](https://github.com/aaronsgiles/ymfm).

# Support

### Features

* Can emulate multiple chips at once to increase polyphony
* Can output to WAV files
* Supported sequence formats:
    * `.mid` Standard MIDI files (format 0 or 1)
    * `.mus` DMX sound system / Doom engine
* Supported instrument patch formats:
    * `.wopl` Wohlstand OPL3 editor
    * `.op2` DMX sound system / Doom engine

May be supported in the future:

* More sequence and instrument file formats
* Some Roland GS and Yamaha XG features (e.g. additional instrument banks, multiple percussion channels)
* True 4-operator instruments (currently only supported in "pair of 2-op voices" mode)

# Usage

ymfmidi can be used as a standalone program (with SDL2), or incorporated into other software to provide easy OPL3-based MIDI playback.

For standalone usage instructions, run the player with no arguments. The player uses patches from [DMXOPL](https://github.com/sneakernets/DMXOPL) by default, but an alternate patch set can be specified as a command-line argument.

To incorporate ymfmidi into your own programs, include everything in the `src` and `ymfm` directories (except for `src/main.cpp` and `src/console.*`), preferably in its own subdirectory somewhere. After that, somewhere in your code:
* `#include "player.h"`
* Create an instance of `OPLPlayer`, optionally specifying a number of chips to emulate (the default is 1)
* Call the `loadSequence` and `loadPatches` methods to load music and instrument data from a path or an existing `FILE*`
* (Optional) Call the `setLoop`, `setSampleRate`, and `setGain` methods to set up playback parameters
* Periodically call one of the `generate` methods to output audio in either signed 16-bit or floating-point format
* (Optional) Call the `reset` method to restart playback at the beginning

A proper static lib build method will be available sooner or later.

# License

ymfmidi and the underlying ymfm library are both released under the 3-clause BSD license.

