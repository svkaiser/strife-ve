#include "sequence_hmi.h"

#include <cstring>

#define READ_U16LE(data, pos) ((data[pos+1] << 8) | data[pos])
#define READ_U32LE(data, pos) ((data[pos+3] << 24) | (data[pos+2] << 16) | (data[pos+1] << 8) | data[pos])

class HMITrack : public MIDTrack
{
public:
	HMITrack(const uint8_t *data, size_t size, SequenceHMI* sequence);
	
protected:
	bool metaEvent(OPLPlayer& player);
};

// ----------------------------------------------------------------------------
HMITrack::HMITrack(const uint8_t *data, size_t size, SequenceHMI *sequence)
	: MIDTrack(data, size, sequence)
{
	m_useNoteDuration = true;
}

// ----------------------------------------------------------------------------
bool HMITrack::metaEvent(OPLPlayer& player)
{
	if (m_status == 0xFE)
	{
		uint8_t data = m_data[m_pos++];
		
		if (data == 0x10)
		{
			if (m_pos + 7 >= m_size)
				return false;
			
			m_pos += m_data[m_pos + 2] + 7;
		}
		else if (data == 0x12)
			m_pos += 2;
		else if (data == 0x13)
			m_pos += 10;
		else if (data == 0x14) // loop start
			m_pos += 2;
		else if (m_pos == 0x15) // loop end
			m_pos += 6;
		else
			return false;
		
		return m_pos < m_size;
	}
	else
	{
		return MIDTrack::metaEvent(player);
	}
}

// ----------------------------------------------------------------------------
SequenceHMI::SequenceHMI()
	: SequenceMID()
{
	m_type = 1;
}

// ----------------------------------------------------------------------------
SequenceHMI::~SequenceHMI() {}

// ----------------------------------------------------------------------------
void SequenceHMI::read(const uint8_t *data, size_t size)
{
	uint32_t numTracks  = READ_U32LE(data, 0xE4);
	uint32_t trackTable = READ_U32LE(data, 0xE8);
	
	m_ticksPerBeat = READ_U16LE(data, 0xD2);
	m_ticksPerSec  = READ_U16LE(data, 0xD4);
	
	for (int i = 0; i < numTracks; i++)
	{
		uint32_t trackPtr = trackTable + 4*i;
	
		if (trackPtr + 8 >= size)
			break;
		
		uint32_t offset = READ_U32LE(data, trackPtr);
		if (offset >= size)
			continue;
		
		uint32_t trackLen = READ_U32LE(data, trackPtr + 4) - offset;
		
		if (((offset + trackLen) > size) || (i == numTracks - 1))
			// stop at the end of the file if this is the last track
			// (or if the track is just malformed/truncated)
			trackLen = size - offset;
		if ((trackLen <= 0x5b) || memcmp(data + offset, "HMI-MIDITRACK", 13))
			continue;
		
		uint32_t trackStart = READ_U32LE(data, offset + 0x57);
		if (trackStart < 0x5b)
			continue;
		
		m_tracks.push_back(new HMITrack(data + offset + trackStart, trackLen - trackStart, this));
	}
}

// ----------------------------------------------------------------------------
bool SequenceHMI::isValid(const uint8_t *data, size_t size)
{
	if (size < 0xEC)
		return false;
	
	return !memcmp(data, "HMI-MIDISONG061595", 18);
}

// ----------------------------------------------------------------------------
void SequenceHMI::setTimePerBeat(uint32_t usec)
{
	// ?
}
