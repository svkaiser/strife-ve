#include "player.h"
#include "sequence.h"

#include <cmath>
#include <cstring>

static const unsigned voice_num[18] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
	0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108
};

static const unsigned oper_num[18] = {
	0x0, 0x1, 0x2, 0x8, 0x9, 0xA, 0x10, 0x11, 0x12,
	0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112
};

// ----------------------------------------------------------------------------
OPLPlayer::OPLPlayer(int numChips)
	: ymfm::ymfm_interface()
{
	m_numChips = numChips;
	m_opl3.resize(numChips);
	for (auto& opl : m_opl3)
		opl = new ymfm::ymf262(*this);
	m_sampleFIFO.resize(numChips);
		
	m_voices.resize(numChips * 18);
	m_sequence = nullptr;
	
	m_samplePos = 0.0;
	m_samplesLeft = 0;
	setSampleRate(44100);
	setGain(1.0);
	
	m_looping = false;
	
	reset();
}

// ----------------------------------------------------------------------------
OPLPlayer::~OPLPlayer()
{
	for (auto& opl : m_opl3)
		delete opl;
	delete m_sequence;
}

// ----------------------------------------------------------------------------
void OPLPlayer::setSampleRate(uint32_t rate)
{
	uint32_t rateOPL = m_opl3[0]->sample_rate(masterClock);
	m_sampleStep = (double)rate / rateOPL;
	m_sampleRate = rate;
	
//	printf("OPL sample rate = %u / output sample rate = %u / step %02f\n", rateOPL, rate, m_sampleStep);
}

// ----------------------------------------------------------------------------
void OPLPlayer::setGain(double gain)
{
	m_sampleGain = gain;
	m_sampleScale = 32768.0 / gain;
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadSequence(const char* path)
{
	delete m_sequence;
	m_sequence = Sequence::load(path);
	
	return m_sequence != nullptr;
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadSequence(FILE *file, int offset, size_t size)
{
	delete m_sequence;
	m_sequence = Sequence::load(file, offset, size);
	
	return m_sequence != nullptr;
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadSequence(const uint8_t *data, size_t size)
{
	delete m_sequence;
	m_sequence = Sequence::load(data, size);
	
	return m_sequence != nullptr;
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadPatches(const char* path)
{
	return OPLPatch::load(m_patches, path);
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadPatches(FILE *file, int offset, size_t size)
{
	return OPLPatch::load(m_patches, file, offset, size);
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadPatches(const uint8_t *data, size_t size)
{
	return OPLPatch::load(m_patches, data, size);
}

// ----------------------------------------------------------------------------
void OPLPlayer::generate(float *data, unsigned numSamples)
{
	ymfm::ymf262::output_data output[m_numChips];

	while (numSamples)
	{
		updateMIDI(output);

		while (m_samplePos >= 1.0 && numSamples > 0)
		{
			for (unsigned i = 0; i < m_numChips; i++)
			{
				data[0] += output[i].data[0] / m_sampleScale;
				data[1] += output[i].data[1] / m_sampleScale;
			}
			data += 2;
			numSamples--;
			m_samplePos -= 1.0;
			if (m_samplesLeft)
				m_samplesLeft--;
		}
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::generate(int16_t *data, unsigned numSamples)
{
	ymfm::ymf262::output_data output[m_numChips];

	while (numSamples)
	{
		updateMIDI(output);
		
		while (m_samplePos >= 1.0 && numSamples > 0)
		{
			int32_t samples[2] = {0};
		
			for (unsigned i = 0; i < m_numChips; i++)
			{
				samples[0] += (int32_t)output[i].data[0] * m_sampleGain;
				samples[1] += (int32_t)output[i].data[1] * m_sampleGain;
			}
			
			*data++ = ymfm::clamp(samples[0], -32768, 32767);
			*data++ = ymfm::clamp(samples[1], -32768, 32767);
			numSamples--;
			m_samplePos -= 1.0;
			if (m_samplesLeft)
				m_samplesLeft--;
		}
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateMIDI(ymfm::ymf262::output_data *output)
{
	while (!m_samplesLeft && m_sequence && !atEnd())
	{	
		// time to update midi playback
		m_samplesLeft = m_sequence->update(*this);
		for (auto& voice : m_voices)
		{
			if (voice.duration < UINT_MAX)
				voice.duration++;
			voice.justChanged = false;
		}
		
		if (m_samplesLeft)
			m_timePassed = true;
	}

	while (m_samplePos < 1.0)
	{
		for (unsigned i = 0; i < m_numChips; i++)
		{
			if (m_sampleFIFO[i].empty())
			{
				m_opl3[i]->generate(&output[i]);
			}
			else
			{
				output[i] = m_sampleFIFO[i].front();
				m_sampleFIFO[i].pop();
			}
		}
		m_samplePos += m_sampleStep;
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::displayClear()
{
	for (int i = 0; i < 18; i++)
		printf("%79s\n", "");
}

// ----------------------------------------------------------------------------
void OPLPlayer::displayChannels()
{
	unsigned numVoices[16] = {0};
	unsigned totalVoices = 0;
	for (auto& voice : m_voices)
	{
		if (voice.channel && (voice.on || voice.justChanged))
		{
			numVoices[voice.channel->num]++;
			totalVoices++;
		}
	}
	
	printf("Chn | Patch Name                       | Vol | Pan | Active Voices: %u/%-6llu\n", totalVoices, m_voices.size());
	printf("----+----------------------------------+-----+-----+---------------------------\n");
	for (int i = 0; i < 16; i++)
	{
		const auto& channel = m_channels[i];
		const OPLPatch *patch = findPatch(i, 0);
	
		printf("%3u | %-32.32s | %3u | %3u | ", i + 1, 
			channel.percussion ? "Percussion" : (patch ? patch->name.c_str() : ""),
			channel.volume, channel.pan);
		
		if (m_voices.size() < 100)
		{
			printf("%2u ", numVoices[i]);
			for (int j = 0; j < 23; j++)
				printf("%c", j < numVoices[i] ? '*' : ' ');
		}
		else
		{
			printf("%3u ", numVoices[i]);
			for (int j = 0; j < 22; j++)
				printf("%c", j < numVoices[i] ? '*' : ' ');
		}
		printf("\n");
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::displayVoices()
{
	for (int i = 0; i < 18; i++)
	{
		if (m_numChips == 1)
		{
			printf("voice %2u: ", i + 1);
			if (m_voices[i].channel)
			{
				printf("channel %2u, note %3u %c %-32.32s",
					m_voices[i].channel->num + 1, m_voices[i].note,
					m_voices[i].on ? '*' : ' ',
					m_voices[i].patch ? m_voices[i].patch->name.c_str() : "");
			}
			else
			{
				printf("%69s", "");
			}
		}
		else if (m_numChips == 2)
		{
			for (int j = i; j < m_voices.size(); j += 18)
			{
				printf("voice %2u: ", j + 1);
				if (m_voices[j].channel)
				{
					printf("channel %2u, note %3u %c",
						m_voices[j].channel->num + 1, m_voices[j].note,
						m_voices[j].on ? '*' : ' ');
				}
				else
				{
					printf("%22s", "");
				}
				
				if (j < 18)
					printf("        | ");
			}
		}
		else if (m_numChips <= 4)
		{
			for (int j = i; j < m_voices.size(); j += 18)
			{
				printf("%2u: ", j + 1);
				if (m_voices[j].channel)
				{
					printf("channel %2u %c",
						m_voices[j].channel->num + 1,
						m_voices[j].on ? '*' : ' ');
				}
				else
				{
					printf("%12s", "");
				}
				
				if (j < m_voices.size() - 18)
					printf(" | ");
			}
		}
		else if (m_numChips <= 8)
		{
			for (int j = i; j < m_voices.size(); j += 18)
			{
				printf("%3u: %c ", j + 1, m_voices[j].on ? '*' : ' ');
				
				if (j < m_voices.size() - 18)
					printf(" | ");
			}
		}
		
		printf("\n");
	}
}

// ----------------------------------------------------------------------------
bool OPLPlayer::atEnd() const
{
	// rewind song at end only if looping is enabled
	// AND if the song played for at least one sample,
	// otherwise just leave it at the end
	if (m_looping && m_timePassed)
		return false;
	if (m_sequence)
		return m_sequence->atEnd();
	return true;
}

// ----------------------------------------------------------------------------
void OPLPlayer::setSongNum(unsigned num)
{
	if (m_sequence)
		m_sequence->setSongNum(num);
	reset();
}

// ----------------------------------------------------------------------------
unsigned OPLPlayer::numSongs() const
{
	if (m_sequence)
		return m_sequence->numSongs();
	return 0;
}
	
// ----------------------------------------------------------------------------
unsigned OPLPlayer::songNum() const
{
	if (m_sequence)
		return m_sequence->songNum();
	return 0;
}

// ----------------------------------------------------------------------------
void OPLPlayer::reset()
{
	for (int i = 0; i < m_opl3.size(); i++)
	{
		m_opl3[i]->reset();
		// enable OPL3 stuff
		write(i, REG_TEST, 1 << 5);
		write(i, REG_NEW, 1);
	}
		
	// reset MIDI channel and OPL voice status
	m_midiType = GeneralMIDI;
	for (int i = 0; i < 16; i++)
	{
		m_channels[i] = MIDIChannel();
		m_channels[i].num = i;
	}
	m_channels[9].percussion = true;
	
	for (int i = 0; i < m_voices.size(); i++)
	{
		m_voices[i] = OPLVoice();
	//	m_voices[i].channel = m_channels;
		m_voices[i].chip = i / 18;
		m_voices[i].num = voice_num[i % 18];
		m_voices[i].op = oper_num[i % 18];
		
		switch (i % 9)
		{
		case 0: case 1: case 2:
			m_voices[i].fourOpPrimary = true;
			m_voices[i].fourOpOther = &m_voices[i+3];
			break;
		case 3: case 4: case 5:
			m_voices[i].fourOpPrimary = false;
			m_voices[i].fourOpOther = &m_voices[i-3];
			break;
		default:
			m_voices[i].fourOpPrimary = false;
			m_voices[i].fourOpOther = nullptr;
			break;
		}
	}
	
	if (m_sequence)
		m_sequence->reset();
	m_samplesLeft = 0;
	m_timePassed = 0;
}

// ----------------------------------------------------------------------------
void OPLPlayer::runOneSample(int chip)
{
	// clock one sample after changing the 4op state before writing other registers
	// so that ymfm can reassign operators to channels, etc
	ymfm::ymf262::output_data output;
	m_opl3[chip]->generate(&output);
	m_sampleFIFO[chip].push(output);
}

// ----------------------------------------------------------------------------
void OPLPlayer::write(int chip, uint16_t addr, uint8_t data)
{
//	if (addr != 0x104)
//		printf("write reg %03x val %02x\n", addr, data);
	if (addr < 0x100)
		m_opl3[chip]->write_address((uint8_t)addr);
	else
		m_opl3[chip]->write_address_hi((uint8_t)addr);
	m_opl3[chip]->write_data(data);
}

// ----------------------------------------------------------------------------
OPLVoice* OPLPlayer::findVoice(uint8_t channel, const OPLPatch *patch)
{
	OPLVoice *found = nullptr;
	uint32_t duration = 0;
	
	// try to find the "oldest" voice, prioritizing released notes
	// (or voices that haven't ever been used yet)
	for (auto& voice : m_voices)
	{
		if (patch->fourOp && !voice.fourOpPrimary)
			continue;
	
		if (!voice.channel)
			return &voice;
	
		if (!voice.on && !voice.justChanged
			&& voice.duration > duration)
		{
			found = &voice;
			duration = voice.duration;
		}
	}
	
	if (found) return found;
	// if we didn't find one yet, just try to find an old one
	// using the same channel and/or patch, even if it should still be playing.
	
	for (auto& voice : m_voices)
	{
		if (patch->fourOp && !voice.fourOpPrimary)
			continue;
		
		if ((voice.channel->num == channel || voice.patch == patch)
		    && voice.duration > duration)
		{
			found = &voice;
			duration = voice.duration;
		}
	}
	
	if (found) return found;
	// last resort - just find any old voice at all
	
	for (auto& voice : m_voices)
	{
		if (patch->fourOp && !voice.fourOpPrimary)
			continue;
		// don't let a 2op instrument steal an active voice from a 4op one
		if (!patch->fourOp && voice.on && voice.patch->fourOp)
			continue;
		
		if (voice.duration > duration)
		{
			found = &voice;
			duration = voice.duration;
		}
	}
	
	return found;
}

// ----------------------------------------------------------------------------
OPLVoice* OPLPlayer::findVoice(uint8_t channel, uint8_t note, bool justChanged)
{
	channel &= 15;
	for (auto& voice : m_voices)
	{
		if (voice.on 
		    && voice.justChanged == justChanged
		    && voice.channel == &m_channels[channel]
		    && voice.note == note)
		{
			return &voice;
		}
	}
	
	return nullptr;
}

// ----------------------------------------------------------------------------
const OPLPatch* OPLPlayer::findPatch(uint8_t channel, uint8_t note) const
{
	uint16_t key;
	const MIDIChannel &ch = m_channels[channel & 15];

	if (ch.percussion)
		key = 0x80 | note;
	else
		key = ch.patchNum | (ch.bank << 8);
	
	// if this patch+bank combo doesn't exist, default to bank 0
	if (!m_patches.count(key))
		key &= 0x00ff;
	// if patch still doesn't exist in bank 0, use patch 0 (or drum note 0)
	if (!m_patches.count(key))
		key &= 0x0080;
	// if that somehow still doesn't exist, forget it
	if (!m_patches.count(key))
		return nullptr;
	
	return &m_patches.at(key);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateChannelVoices(uint8_t channel, void(OPLPlayer::*func)(OPLVoice&))
{
	for (auto& voice : m_voices)
	{
		if (voice.channel == &m_channels[channel & 15])
			(this->*func)(voice);
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::updatePatch(OPLVoice& voice, const OPLPatch *newPatch, uint8_t numVoice)
{	
	// assign the MIDI channel's current patch (or the current drum patch) to this voice

	const PatchVoice& patchVoice = newPatch->voice[numVoice];
	
	if (voice.patchVoice != &patchVoice)
	{
		bool oldFourOp = voice.patch ? voice.patch->fourOp : false;
	
		voice.patch = newPatch;
		voice.patchVoice = &patchVoice;
		
		// update enable status for 4op channels on this chip
		if (newPatch->fourOp != oldFourOp)
		{
			// if going from part of a 4op patch to a 2op one, kill the other one
			OPLVoice *other = voice.fourOpOther;
			if (other && other->patch
				&& other->patch->fourOp && !newPatch->fourOp)
			{
				silenceVoice(*other);
			}
		
			uint8_t enable = 0x00;
			uint8_t bit = 0x01;
			for (unsigned i = voice.chip * 18; i < voice.chip * 18 + 18; i++)
			{
				if (m_voices[i].fourOpPrimary)
				{
					if (m_voices[i].patch && m_voices[i].patch->fourOp)
						enable |= bit;
					bit <<= 1;
				}
			}
			
			write(voice.chip, REG_4OP, enable);
			runOneSample(voice.chip);
		}
		
		// 0x20: vibrato, sustain, multiplier
		write(voice.chip, REG_OP_MODE + voice.op,     patchVoice.op_mode[0]);
		write(voice.chip, REG_OP_MODE + voice.op + 3, patchVoice.op_mode[1]);
		// 0x60: attack/decay
		write(voice.chip, REG_OP_AD + voice.op,     patchVoice.op_ad[0]);
		write(voice.chip, REG_OP_AD + voice.op + 3, patchVoice.op_ad[1]);
		// 0x80: sustain/release
		write(voice.chip, REG_OP_SR + voice.op,     patchVoice.op_sr[0]);
		write(voice.chip, REG_OP_SR + voice.op + 3, patchVoice.op_sr[1]);
		// 0xe0: waveform
		write(voice.chip, REG_OP_WAVEFORM + voice.op,     patchVoice.op_wave[0]);
		write(voice.chip, REG_OP_WAVEFORM + voice.op + 3, patchVoice.op_wave[1]);
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateVolume(OPLVoice& voice)
{
	// lookup table shamelessly stolen from Nuke.YKT
	static const uint8_t opl_volume_map[32] =
	{
		80, 63, 40, 36, 32, 28, 23, 21,
		19, 17, 15, 14, 13, 12, 11, 10,
		 9,  8,  7,  6,  5,  5,  4,  4,
		 3,  3,  2,  2,  1,  1,  0,  0
	};

	if (!voice.patch || !voice.channel) return;
	
	auto patchVoice = voice.patchVoice;

	uint8_t atten = opl_volume_map[(voice.velocity * voice.channel->volume) >> 9];
	uint8_t level;
	bool scale[2] = {0};
	
	// determine which operator(s) to scale based on the current operator settings
	if (!voice.patch->fourOp)
	{
		// 2op FM (0): scale op 2 only
		// 2op AM (1): scale op 1 and 2
		scale[0] = (patchVoice->conn & 1);
		scale[1] = true;
	}
	else if (voice.fourOpPrimary)
	{
		// 4op FM+FM (0, 0): don't scale op 1 or 2
		// 4op AM+FM (1, 0): scale op 1 only
		// 4op FM+AM (0, 1): scale op 2 only
		// 4op AM+AM (1, 1): scale op 1 only
		scale[0] = (voice.patch->voice[0].conn & 1);
		scale[1] = (voice.patch->voice[1].conn & 1) && !scale[0];
	}
	else
	{
		// 4op FM+FM (0, 0): scale op 4 only
		// 4op AM+FM (1, 0): scale op 4 only
		// 4op FM+AM (0, 1): scale op 4 only
		// 4op AM+AM (1, 1): scale op 3 and 4
		scale[0] = (voice.patch->voice[0].conn & 1)
		        && (voice.patch->voice[1].conn & 1);
		scale[1] = true;
	}
	
	// 0x40: key scale / volume
	if (scale[0])
		level = std::min(0x3f, patchVoice->op_level[0] + atten);
	else
		level = patchVoice->op_level[0];
	write(voice.chip, REG_OP_LEVEL + voice.op,     level | patchVoice->op_ksr[0]);
	
	if (scale[1])
		level = std::min(0x3f, patchVoice->op_level[1] + atten);
	else
		level = patchVoice->op_level[1];
	write(voice.chip, REG_OP_LEVEL + voice.op + 3, level | patchVoice->op_ksr[1]);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updatePanning(OPLVoice& voice)
{
	if (!voice.patch || !voice.channel) return;
	
	// 0xc0: output/feedback/mode
	uint8_t pan = 0x30;
	if (voice.channel->pan < 32)
		pan = 0x10;
	else if (voice.channel->pan >= 96)
		pan = 0x20;
	
	write(voice.chip, REG_VOICE_CNT + voice.num, voice.patchVoice->conn | pan);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateFrequency(OPLVoice& voice)
{
	static const uint16_t noteFreq[12] = {
		// calculated from A440
		345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651
	};

	if (!voice.patch || !voice.channel) return;
	if (voice.patch->fourOp && !voice.fourOpPrimary) return;
	
	int note = (!voice.channel->percussion ? voice.note : voice.patch->fixedNote)
	         + voice.patchVoice->tune;
	
	int octave = note / 12;
	note %= 12;
	
	// calculate base frequency (and apply pitch bend / patch detune)
	unsigned freq = (note >= 0) ? noteFreq[note] : (noteFreq[note + 12] >> 1);
	if (octave < 0)
		freq >>= -octave;
	else if (octave > 0)
		freq <<= octave;
	
	if (voice.channel->pitch > 0)
		freq += freq * voice.channel->noteBendUp * voice.channel->pitch;
	else if (voice.channel->pitch < 0)
		freq += freq * voice.channel->noteBendDown * voice.channel->pitch;
		
	if (voice.patchVoice->finetune > 0)
		freq += freq * MIDIChannel::defaultBendUp * voice.patchVoice->finetune;
	else if (voice.patchVoice->finetune < 0)
		freq += freq * MIDIChannel::defaultBendDown * voice.patchVoice->finetune;
	
	// convert the calculated frequency back to a block and F-number
	octave = 0;
	while (freq > 0x3ff)
	{
		freq >>= 1;
		octave++;
	}
	octave = std::min(7, octave);
	voice.freq = freq | (octave << 10);
	
	write(voice.chip, REG_VOICE_FREQL + voice.num, voice.freq & 0xff);
	write(voice.chip, REG_VOICE_FREQH + voice.num, (voice.freq >> 8) | (voice.on ? (1 << 5) : 0));
}

// ----------------------------------------------------------------------------
void OPLPlayer::silenceVoice(OPLVoice& voice)
{
	voice.channel    = nullptr;
	voice.patch      = nullptr;
	voice.patchVoice = nullptr;
	
	voice.on = false;
	voice.justChanged = true;
	voice.duration = UINT_MAX;
	
	write(voice.chip, REG_OP_LEVEL + voice.op,     0xff);
	write(voice.chip, REG_OP_LEVEL + voice.op + 3, 0xff);
	write(voice.chip, REG_VOICE_FREQL + voice.num, 0x00);
	write(voice.chip, REG_VOICE_FREQH + voice.num, 0x00);
	write(voice.chip, REG_VOICE_CNT + voice.num,   0x00);
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiEvent(uint8_t status, uint8_t data0, uint8_t data1)
{
	uint8_t channel = status & 15;
	int16_t pitch;

	switch (status >> 4)
	{
	case 8: // note off (ignore velocity)
		midiNoteOff(channel, data0);
		break;
	
	case 9: // note on
		midiNoteOn(channel, data0, data1);
		break;
	
	case 10: // polyphonic pressure (ignored)
		break;
		
	case 11: // controller change
		midiControlChange(channel, data0, data1);
		break;
	
	case 12: // program change
		midiProgramChange(channel, data0);
		break;
	
	case 13: // channel pressure (ignored)
		break;
	
	case 14: // pitch bend
		pitch = (int16_t)(data0 | (data1 << 7)) - 8192;
		midiPitchControl(channel, pitch / 8192.0);
		break;
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
	note &= 0x7f;
	velocity &= 0x7f;

	// if we just now turned this same note on, don't do it again
	if (findVoice(channel, note, true))
		return;
	
	midiNoteOff(channel, note);
	if (!velocity)
		return;
	
//	printf("midiNoteOn: chn %u, note %u\n", channel, note);
	const OPLPatch *newPatch = findPatch(channel, note);
	if (!newPatch) return;
	
	const int numVoices = ((newPatch->fourOp || newPatch->dualTwoOp) ? 2 : 1);

	OPLVoice *voice = nullptr;
	for (int i = 0; i < numVoices; i++)
	{
		if (voice && newPatch->fourOp && voice->fourOpOther)
			voice = voice->fourOpOther;
		else
			voice = findVoice(channel, newPatch);
		if (!voice) continue; // ??
		
		if (voice->on)
		{
			silenceVoice(*voice);
			runOneSample(voice->chip);
		}
		
		// update the note parameters for this voice
		voice->channel = &m_channels[channel & 15];
		voice->on = voice->justChanged = true;
		voice->note = note;
		voice->velocity = ymfm::clamp((int)velocity + newPatch->velocity, 0, 127);
		// for dual 2op, set the second voice's duration to 1 so it can get dropped if we need it to
		voice->duration = newPatch->dualTwoOp ? i : 0;
		
		updatePatch(*voice, newPatch, i);
		updateVolume(*voice);
		updatePanning(*voice);
		
		// for 4op instruments, don't key on until we've written both voices...
		if (!newPatch->fourOp)
		{
			updateFrequency(*voice);
			runOneSample(voice->chip);
		}
		else if (newPatch->fourOp && i > 0)
		{
			updateFrequency(*voice->fourOpOther);
			runOneSample(voice->chip);
		}		
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiNoteOff(uint8_t channel, uint8_t note)
{
	note &= 0x7f;
	
//	printf("midiNoteOff: chn %u, note %u\n", channel, note);
	OPLVoice *voice;
	while ((voice = findVoice(channel, note)) != nullptr)
	{
		voice->justChanged = voice->on;
		voice->on = false;

		write(voice->chip, REG_VOICE_FREQH + voice->num, voice->freq >> 8);
		runOneSample(voice->chip);
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiPitchControl(uint8_t channel, double pitch)
{
//	printf("midiPitchControl: chn %u, val %.02f\n", channel, pitch);
	m_channels[channel & 15].pitch = pitch;
	updateChannelVoices(channel, &OPLPlayer::updateFrequency);
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiProgramChange(uint8_t channel, uint8_t patchNum)
{
	m_channels[channel & 15].patchNum = patchNum & 0x7f;
	// patch change will take effect on the next note for this channel
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiControlChange(uint8_t channel, uint8_t control, uint8_t value)
{
	channel &= 15;
	control &= 0x7f;
	value   &= 0x7f;
	
	MIDIChannel& ch = m_channels[channel];
	
//	printf("midiControlChange: chn %u, ctrl %u, val %u\n", channel, control, value);
	switch (control)
	{
	case 0:
		if (m_midiType == RolandGS)
			ch.bank = value;
		else if (m_midiType == YamahaXG)
			ch.percussion = (value == 0x7f);
		break;
		
	case 6:
		if (ch.rpn == 0)
		{
			midiSetBendRange(channel, value);
			updateChannelVoices(channel, &OPLPlayer::updateFrequency);
		}
		break;
	
	case 7:
		ch.volume = value;
		updateChannelVoices(channel, &OPLPlayer::updateVolume);
		break;
	
	case 10:
		ch.pan = value;
		updateChannelVoices(channel, &OPLPlayer::updatePanning);
		break;
	
	case 32:
		if (m_midiType == YamahaXG || m_midiType == GeneralMIDI2)
			ch.bank = value;
		break;
	
	case 98:
	case 99:
		ch.rpn = 0x3fff;
		break;
	
	case 100:
		ch.rpn &= 0x3f80;
		ch.rpn |= value;
		break;
		
	case 101:
		ch.rpn &= 0x7f;
		ch.rpn |= (value << 7);
		break;
	
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiSysEx(const uint8_t *data, uint32_t length)
{
	if (length > 0 && data[0] == 0xF0)
	{
		data++;
		length--;
	}

	if (length == 0)
		return;

	if (data[0] == 0x7e) // universal non-realtime
	{
		if (length == 5 && data[1] == 0x7f && data[2] == 0x09)
		{
			if (data[3] == 0x01)
				m_midiType = GeneralMIDI;
			else if (data[3] == 0x03)
				m_midiType = GeneralMIDI2;
		}
	}
	else if (data[0] == 0x41 && length >= 10 // Roland
	         && data[2] == 0x42 && data[3] == 0x12)
	{
		// if we received one of these, assume GS mode
		// (some MIDIs seem to e.g. send drum map messages without a GS reset)
		m_midiType = RolandGS;
		
		uint32_t address = (data[4] << 16) | (data[5] << 8) | data[6];
		// for single part parameters, map "part number" to channel number
		// (using the default mapping)
		uint8_t channel = (address & 0xf00) >> 8;
		if (channel == 0)
			channel = 9;
		else if (channel <= 9)
			channel--;
			
		// Roland GS part parameters
		if ((address & 0xfff0ff) == 0x401015) // set drum map
			m_channels[channel].percussion = (data[7] != 0x00);
	}
	else if (length >= 8 && !memcmp(data, "\x43\x10\x4c\x00\x00\x7e\x00\xf7", 8)) // Yamaha
	{
		m_midiType = YamahaXG;
	}
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiSetBendRange(uint8_t channel, uint8_t semitones)
{
	MIDIChannel& ch = m_channels[channel & 15];
	
	ch.noteBendUp   = pow(2, semitones / 12.0) - 1;
	ch.noteBendDown = 1 - (1 / (ch.noteBendUp + 1));
}
