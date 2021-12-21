#include "sequence_mid.h"

#include <cmath>
#include <cstring>

#define READ_U16BE(data, pos) ((data[pos] << 8) | data[pos+1])
#define READ_U24BE(data, pos) ((data[pos] << 16) | (data[pos+1] << 8) | data[pos+2])
#define READ_U32BE(data, pos) ((data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3])

// ----------------------------------------------------------------------------
MIDTrack::MIDTrack(const uint8_t *data, size_t size, SequenceMID *sequence)
{
	m_data = new uint8_t[size];
	m_size = size;
	memcpy(m_data, data, size);
	m_sequence = sequence;
	
	m_initDelay = true;
	m_useRunningStatus = true;
	m_useNoteDuration = false;
	
	reset();
}

// ----------------------------------------------------------------------------
MIDTrack::~MIDTrack()
{
	delete[] m_data;
}

// ----------------------------------------------------------------------------
void MIDTrack::reset()
{
	m_pos = m_delay = 0;
	m_atEnd = false;
	m_status = 0x00;
}

// ----------------------------------------------------------------------------
void MIDTrack::advance(uint32_t time)
{
	if (m_atEnd)
		return;
	
	m_delay -= time;
	if (m_useNoteDuration)
		for (auto& note : m_notes)
			note.delay -= time;
}

// ----------------------------------------------------------------------------
uint32_t MIDTrack::readVLQ()
{
	uint32_t vlq = 0;
	uint8_t data = 0;

	do
	{
		data = m_data[m_pos++];
		vlq <<= 7;
		vlq |= (data & 0x7f);
	} while ((data & 0x80) && (m_pos < m_size));
	
	return vlq;
}

// ----------------------------------------------------------------------------
int32_t MIDTrack::minDelay()
{
	int32_t delay = m_delay;
	if (m_useNoteDuration)
		for (auto& note : m_notes)
			delay = std::min(delay, note.delay);
	return delay;
}

// ----------------------------------------------------------------------------
uint32_t MIDTrack::update(OPLPlayer& player)
{
	if (m_initDelay && !m_pos)
	{
		m_delay = readDelay();
	}
	
	if (m_useNoteDuration)
	{
		for (int i = 0; i < m_notes.size();)
		{
			if (m_notes[i].delay <= 0)
			{
				player.midiNoteOff(m_notes[i].channel, m_notes[i].note);
				m_notes[i] = m_notes.back();
				m_notes.pop_back();
			}	
			else
				i++;
		}
	}
	
	while (m_delay <= 0)
	{
		uint8_t data[2];
		MIDNote note;
		
		// make sure we have enough data left for one full event
		if (m_size - m_pos < 3)
		{
			m_atEnd = true;
			return UINT_MAX;
		}
		
		if (!m_useRunningStatus || (m_data[m_pos] & 0x80))
			m_status = m_data[m_pos++];
		
		switch (m_status >> 4)
		{
		case 9: // note on
			data[0] = m_data[m_pos++];
			data[1] = m_data[m_pos++];
			player.midiEvent(m_status, data[0], data[1]);
			
			if (m_useNoteDuration)
			{
				note.channel = m_status & 15;
				note.note    = data[0];
				note.delay   = readVLQ();
				m_notes.push_back(note);
			}
			break;
		
		case 8:  // note off
		case 10: // polyphonic pressure
		case 11: // controller change
		case 14: // pitch bend
			data[0] = m_data[m_pos++];
			data[1] = m_data[m_pos++];
			player.midiEvent(m_status, data[0], data[1]);
			break;
			
		case 12: // program change
		case 13: // channel pressure (ignored)
			data[0] = m_data[m_pos++];
			player.midiEvent(m_status, data[0]);
			break;
		
		case 15: // sysex / meta event
			if (!metaEvent(player))
			{			
				m_atEnd = true;
				return UINT_MAX;
			}
			break;
		}
		
		m_delay += readDelay();
	}

	return minDelay();
}

// ----------------------------------------------------------------------------
bool MIDTrack::metaEvent(OPLPlayer& player)
{
	uint32_t len;
	
	if (m_status != 0xFF)
	{
		len = readVLQ();
		if (m_pos + len < m_size)
		{
			if (m_status == 0xf0)
				player.midiSysEx(m_data + m_pos, len);
		}
		else
		{
			return false;
		}
	}
	else
	{
		uint8_t data = m_data[m_pos++];
		len = readVLQ();
		
		// end-of-track marker (or data just ran out)
		if (data == 0x2F || (m_pos + len >= m_size))
		{
			return false;
		}
		// tempo change
		if (data == 0x51)
		{
			m_sequence->setTimePerBeat(READ_U24BE(m_data, m_pos));
		}
	}
	
	m_pos += len;
	return true;
}

// ----------------------------------------------------------------------------
SequenceMID::SequenceMID()
	: Sequence()
{
	m_type = 0;
	m_ticksPerBeat = 24;
	m_ticksPerSec = 48;
}

// ----------------------------------------------------------------------------
SequenceMID::~SequenceMID()
{
	for (auto track : m_tracks)
		delete track;
}

// ----------------------------------------------------------------------------
bool SequenceMID::isValid(const uint8_t *data, size_t size)
{
	if (size < 12)
		return false;
	
	if (!memcmp(data, "MThd", 4))
	{
		uint32_t len = READ_U32BE(data, 4);
		if (len < 6) return false;
		
		uint16_t type = READ_U16BE(data, 8);
		if (type > 2) return false;
		
		return true;
	}
	else if (!memcmp(data, "RIFF", 4)
	      && !memcmp(data + 8, "RMID", 4))
	{
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
void SequenceMID::read(const uint8_t *data, size_t size)
{
	// need at least the MIDI header + one track header
	if (size < 23)
		return;
	
	if (!memcmp(data, "RIFF", 4))
	{
		uint32_t offset = 12;
		while (offset + 8 < size)
		{
			const uint8_t *bytes = data + offset;
			uint32_t chunkLen = bytes[4] | (bytes[5] << 8) | (bytes[6] << 16) | (bytes[7] << 24);
			chunkLen = (chunkLen + 1) & ~1;
			
			// move to next subchunk
			offset += chunkLen + 8;
			if (offset > size)
			{
				// try to handle a malformed/truncated chunk
				chunkLen -= (offset - size);
				offset = size;
			}
			
			if (!memcmp(bytes, "data", 4))
			{
				if (isValid(bytes + 8, chunkLen))
					read(bytes + 8, chunkLen);
				break;
			}
		}
	}
	else
	{
		uint32_t len = READ_U32BE(data, 4);
		
		m_type = READ_U16BE(data, 8);
		uint16_t numTracks = READ_U16BE(data, 10);
		m_ticksPerBeat = READ_U16BE(data, 12);
		
		uint32_t offset = len + 8;
		for (unsigned i = 0; i < numTracks; i++)
		{
			if (offset + 8 >= size)
				break;
			
			const uint8_t *bytes = data + offset;
			if (memcmp(bytes, "MTrk", 4))
				break;
			
			len = READ_U32BE(bytes, 4);
			offset += len + 8;
			if (offset > size)
			{
				// try to handle a malformed/truncated chunk
				len -= (offset - size);
				offset = size;
			}
			
			m_tracks.push_back(new MIDTrack(bytes + 8, len, this));
		}
	}
}

// ----------------------------------------------------------------------------
void SequenceMID::reset()
{
	Sequence::reset();
	setDefaults();
	
	for (auto& track : m_tracks)
		track->reset();
}

// ----------------------------------------------------------------------------
void SequenceMID::setDefaults()
{
	setTimePerBeat(500000);
}

// ----------------------------------------------------------------------------
void SequenceMID::setTimePerBeat(uint32_t usec)
{
	double usecPerTick = (double)usec / m_ticksPerBeat;
	m_ticksPerSec = 1000000 / usecPerTick;
}

// ----------------------------------------------------------------------------
unsigned SequenceMID::numSongs() const
{
	if (m_type != 2)
		return 1;
	else
		return m_tracks.size();
}

// ----------------------------------------------------------------------------
uint32_t SequenceMID::update(OPLPlayer& player)
{
	uint32_t tickDelay = UINT_MAX;
	
	bool tracksAtEnd = true;

	if (m_type != 2)
	{
		for (auto track : m_tracks)
		{
			if (!track->atEnd())
				tickDelay = std::min(tickDelay, track->update(player));
			tracksAtEnd &= track->atEnd();
		}
	}
	else if (m_songNum < m_tracks.size())
	{
		tickDelay   = m_tracks[m_songNum]->update(player);
		tracksAtEnd = m_tracks[m_songNum]->atEnd();
	}
	
	if (tracksAtEnd)
	{
		reset();
		m_atEnd = true;
		return 0;
	}
	
	m_atEnd = false;
	
	for (auto track : m_tracks)
		track->advance(tickDelay);
	
	double samplesPerTick = player.sampleRate() / m_ticksPerSec;	
	return round(tickDelay * samplesPerTick);
}
