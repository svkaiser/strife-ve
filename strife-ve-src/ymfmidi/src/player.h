#ifndef __PLAYER_H
#define __PLAYER_H

#include <ymfm_opl.h>
#include <climits>
#include <queue>
#include <vector>

#include "patches.h"

class Sequence;

struct MIDIChannel
{
	uint8_t num = 0;

	bool percussion = false;
	uint8_t bank = 0;
	uint8_t patchNum = 0;
	uint8_t volume = 127;
	uint8_t pan = 64;
	double pitch = 0.0;
	
	uint16_t rpn = 0x3fff;

	static constexpr double defaultBendUp   = 0.1224620;  // ~2 semitones
	static constexpr double defaultBendDown = 0.1091013;

	double noteBendUp   = defaultBendUp;
	double noteBendDown = defaultBendDown;
};

struct OPLVoice
{
	int chip = 0;
	const MIDIChannel *channel = nullptr;
	const OPLPatch *patch = nullptr;
	const PatchVoice *patchVoice = nullptr;
	
	uint16_t num = 0;
	uint16_t op = 0; // base operator number, set based on voice num.
	bool fourOpPrimary = false;
	OPLVoice *fourOpOther = nullptr;
	
	bool on = false;
	bool justChanged = false; // true after note on/off, false after generating at least 1 sample
	uint8_t note = 0;
	uint8_t velocity = 0;
	
	// block and F number, calculated from note and channel pitch
	uint16_t freq = 0;
	
	// how long has this note been playing (incremented each midi update)
	uint32_t duration = UINT_MAX;
};

class OPLPlayer : public ymfm::ymfm_interface
{
public:
	enum MIDIType
	{
		GeneralMIDI,
		RolandGS,
		YamahaXG,
		GeneralMIDI2
	};

	OPLPlayer(int numChips = 1);
	virtual ~OPLPlayer();
	
	void setLoop(bool loop) { m_looping = loop; }
	void setSampleRate(uint32_t rate);
	void setGain(double gain);
	
	// load MIDI data from the specified path
	bool loadSequence(const char* path);
	// load MIDI data from an already opened file, optionally at a given offset
	// if 'size' is 0, the full file will be read (starting from 'offset')
	bool loadSequence(FILE *file, int offset = 0, size_t size = 0);
	// load MIDI data from a block of memory
	bool loadSequence(const uint8_t *data, size_t size);
	
	// load instrument patches from the specified path
	bool loadPatches(const char* path);
	// load instrument patches from an already opened file,
	// optionally at a given offset
	// if 'size' is 0, the full file will be read (starting from 'offset')
	bool loadPatches(FILE *file, int offset = 0, size_t size = 0);
	// load instrument patches from a block of memory
	bool loadPatches(const uint8_t *data, size_t size);
	
	void generate(float *data, unsigned numSamples);
	void generate(int16_t *data, unsigned numSamples);
	
	// reset OPL and midi file
	void reset();
	// reached end of song?
	bool atEnd() const;
	// song selection (for files with multiple songs)
	void     setSongNum(unsigned num);
	unsigned numSongs() const;
	unsigned songNum() const;
	
	// MIDI events, called by the file format handler
	void midiEvent(uint8_t status, uint8_t data0, uint8_t data1 = 0);
	// helpers for midiEvent
	void midiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
	void midiNoteOff(uint8_t channel, uint8_t note);
	void midiPitchControl(uint8_t channel, double pitch); // range is -1.0 to 1.0
	void midiProgramChange(uint8_t channel, uint8_t patchNum);
	void midiControlChange(uint8_t channel, uint8_t control, uint8_t value);
	// sysex data (data and length *don't* include the opening 0xF0)
	void midiSysEx(const uint8_t *data, uint32_t length);
	
	// helper for pitch bend range RPN
	void midiSetBendRange(uint8_t channel, uint8_t semitones);
	
	// debug
	void displayClear();
	void displayChannels();
	void displayVoices();
	
	// misc. informational stuff
	uint32_t sampleRate() const { return m_sampleRate; }
	const std::string& patchName(uint8_t num) { return m_patches[num].name; }
	
private:
	static const unsigned masterClock = 14318181;

	enum {
		REG_TEST        = 0x01,
	
		REG_OP_MODE     = 0x20,
		REG_OP_LEVEL    = 0x40,
		REG_OP_AD       = 0x60,
		REG_OP_SR       = 0x80,
		REG_VOICE_FREQL = 0xA0,
		REG_VOICE_FREQH = 0xB0,
		REG_VOICE_CNT   = 0xC0,
		REG_OP_WAVEFORM = 0xE0,
		
		REG_4OP         = 0x104,
		REG_NEW         = 0x105,
	};

	void updateMIDI(ymfm::ymf262::output_data *output);

	void runOneSample(int chip);

	void write(int chip, uint16_t addr, uint8_t data);
	
	// find a voice with the oldest note
	// if no "off" voices are found, steal one using the same patch or MIDI channel
	OPLVoice* findVoice(uint8_t channel, const OPLPatch *patch);
	// find a voice that's playing a specific note on a specific channel
	OPLVoice* findVoice(uint8_t channel, uint8_t note, bool justChanged = false);

	// find the patch to use for a specific MIDI channel and note
	const OPLPatch* findPatch(uint8_t channel, uint8_t note) const;

	// update a property of all currently playing voices on a MIDI channel
	void updateChannelVoices(uint8_t channel, void(OPLPlayer::*func)(OPLVoice&));

	// update the patch parameters for a voice
	void updatePatch(OPLVoice& voice, const OPLPatch *newPatch, uint8_t numVoice = 0);

	// update the volume level for a voice
	void updateVolume(OPLVoice& voice);

	// update the pan position for a voice
	void updatePanning(OPLVoice& voice);

	// update the block and F-number for a voice (also key on/off)
	void updateFrequency(OPLVoice& voice);

	// silence a voice immediately
	void silenceVoice(OPLVoice& voice);

	std::vector<ymfm::ymf262*> m_opl3;
	unsigned m_numChips;
	
	uint32_t m_sampleRate; // output sample rate (default 44.1k)
	double m_sampleGain;
	double m_sampleScale; // convert 16-bit samples to float (includes gain value)
	double m_sampleStep; // ratio of OPL sample rate to output sample rate (usually < 1.0)
	double m_samplePos; // number of pending output samples (when >= 1.0, output one)
	uint32_t m_samplesLeft; // remaining samples until next midi event
	// if we need to clock one of the OPLs between register writes, save the resulting sample
	std::vector<std::queue<ymfm::ymf262::output_data>> m_sampleFIFO;
	
	bool m_looping;
	bool m_timePassed;
	
	MIDIChannel m_channels[16];
	std::vector<OPLVoice> m_voices;
	MIDIType m_midiType;
	
	Sequence *m_sequence;
	OPLPatchSet m_patches;
};

#endif // __PLAYER_H
