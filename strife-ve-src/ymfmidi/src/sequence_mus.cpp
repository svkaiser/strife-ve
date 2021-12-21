#include "sequence_mus.h"

#include <cmath>
#include <cstring>

// ----------------------------------------------------------------------------
SequenceMUS::SequenceMUS()
	: Sequence()
{
	// cheap safety measure - fill the whole song buffer w/ "end of track" commands
	// (m_pos is 16 bits, so a malformed track will either hit this or just wrap around)
	memset(m_data, 0x60, sizeof(m_data));
	setDefaults();
}

// ----------------------------------------------------------------------------
bool SequenceMUS::isValid(const uint8_t *data, size_t size)
{
	if (size < 8)
		return false;
	return !memcmp(data, "MUS\x1a", 4);
}

// ----------------------------------------------------------------------------
void SequenceMUS::read(const uint8_t *data, size_t size)
{
	if (size > 8)
	{
		uint16_t length = data[4] | (data[5] << 8);
		uint16_t pos    = data[6] | (data[7] << 8);
		
		if (pos < size)
		{
			if (pos + length > size)
				length = size - pos;
			memcpy(m_data, data + pos, length);
		}
	}
}

// ----------------------------------------------------------------------------
void SequenceMUS::reset()
{
	Sequence::reset();
	setDefaults();
}

// ----------------------------------------------------------------------------
void SequenceMUS::setDefaults()
{
	m_pos = 0;
	memset(m_lastVol, 0x7f, sizeof(m_lastVol));
}

// ----------------------------------------------------------------------------
uint32_t SequenceMUS::update(OPLPlayer& player)
{
	uint8_t event, channel, data, param;
	uint16_t lastPos;
	
	m_atEnd = false;
	
	do
	{
		lastPos = m_pos;
		event = m_data[m_pos++];
		channel = event & 0xf;
		
		// map MUS channels to MIDI channels
		// (don't bother with the primary/secondary channel thing unless we need to)
		if (channel == 15) // percussion
			channel = 9;
		else if (channel >= 9)
			channel++;
		
		switch ((event >> 4) & 0x7)
		{
		case 0: // note off
			player.midiNoteOff(channel, m_data[m_pos++]);
			break;
			
		case 1: // note on
			data = m_data[m_pos++];
			if (data & 0x80)
				m_lastVol[channel] = m_data[m_pos++];
			player.midiNoteOn(channel, data, m_lastVol[channel]);
			break;
		
		case 2: // pitch bend
			player.midiPitchControl(channel, (m_data[m_pos++] / 128.0) - 1.0);
			break;
			
		case 3: // system event (channel mode messages)
			data = m_data[m_pos++] & 0x7f;
			switch (data)
			{
			case 10: player.midiControlChange(channel, 120, 0); break; // all sounds off
			case 11: player.midiControlChange(channel, 123, 0); break; // all notes off
			case 12: player.midiControlChange(channel, 126, 0); break; // mono on
			case 13: player.midiControlChange(channel, 127, 0); break; // poly on
			case 14: player.midiControlChange(channel, 121, 0); break; // reset all controllers
			default: break;
			}
			break;
		
		case 4: // controller
			data  = m_data[m_pos++] & 0x7f;
			param = m_data[m_pos++];
			// clamp CC param value - some tracks from tnt.wad have bad volume CCs
			if (param > 0x7f)
				param = 0x7f;
			switch (data)
			{
			case 0: player.midiProgramChange(channel, param); break;
			case 1: player.midiControlChange(channel, 0,  param); break; // bank select
			case 2: player.midiControlChange(channel, 1,  param); break; // mod wheel
			case 3: player.midiControlChange(channel, 7,  param); break; // volume
			case 4: player.midiControlChange(channel, 10, param); break; // pan
			case 5: player.midiControlChange(channel, 11, param); break; // expression
			case 6: player.midiControlChange(channel, 91, param); break; // reverb
			case 7: player.midiControlChange(channel, 93, param); break; // chorus
			case 8: player.midiControlChange(channel, 64, param); break; // sustain pedal
			case 9: player.midiControlChange(channel, 67, param); break; // soft pedal
			default: break;
			}
			break;
		
		case 5: // end of measure
			break;
		
		case 6: // end of track
			reset();
			m_atEnd = true;
			return 0;
		
		case 7: // unused
			m_pos++;
			break;
		}
	} while (!(event & 0x80) && (m_pos > lastPos));
	
	// read delay in ticks, convert to # of samples
	uint32_t tickDelay = 0;
	do
	{
		event = m_data[m_pos++];
		tickDelay <<= 7;
		tickDelay |= (event & 0x7f);
	} while ((event & 0x80) && (m_pos > lastPos));
	
	if (m_pos < lastPos)
	{
		// premature end of track, 16 bit position overflowed
		reset();
		m_atEnd = true;
		return 0;
	}
	
	double samplesPerTick = player.sampleRate() / 140.0;
	return round(tickDelay * samplesPerTick);
}
