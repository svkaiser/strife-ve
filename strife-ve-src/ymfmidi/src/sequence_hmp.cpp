#include "sequence_hmp.h"

#include <cstring>

#define READ_U32LE(data, pos) ((data[pos+3] << 24) | (data[pos+2] << 16) | (data[pos+1] << 8) | data[pos])

class HMPTrack : public MIDTrack
{
public:
	HMPTrack(const uint8_t *data, size_t size, SequenceHMP* sequence);
	
protected:
	uint32_t readDelay();
};

// ----------------------------------------------------------------------------
HMPTrack::HMPTrack(const uint8_t *data, size_t size, SequenceHMP *sequence)
	: MIDTrack(data, size, sequence)
{
	m_useRunningStatus = false;
}

// ----------------------------------------------------------------------------
uint32_t HMPTrack::readDelay()
{
	uint32_t delay = 0;
	uint8_t data = 0;
	unsigned shift = 0;

	do
	{
		data = m_data[m_pos++];
		delay |= ((data & 0x7f) << shift);
		shift += 7;
	} while (!(data & 0x80) && (m_pos < m_size));
	
	return delay;
}

// ----------------------------------------------------------------------------
SequenceHMP::SequenceHMP()
	: SequenceMID()
{
	m_type = 1;
	m_ticksPerBeat = 120;
	m_ticksPerSec = 120;
}

// ----------------------------------------------------------------------------
SequenceHMP::~SequenceHMP() {}

// ----------------------------------------------------------------------------
void SequenceHMP::read(const uint8_t *data, size_t size)
{
	uint32_t numTracks = READ_U32LE(data, 0x30);
	
	m_ticksPerBeat = READ_U32LE(data, 0x34);
	m_ticksPerSec  = READ_U32LE(data, 0x38);
	
	// longer signature = extended format
	uint32_t offset = (data[8] == 0) ? 0x308 : 0x388;
	
	for (int i = 0; i < numTracks; i++)
	{
		if (offset + 12 >= size)
			break;
		
		uint32_t trackLen = READ_U32LE(data, offset + 4);
		if (offset + trackLen >= size)
			// try to handle a malformed/truncated chunk
			trackLen = (size - offset);
		
		if (trackLen <= 12)
			break;
		
		m_tracks.push_back(new HMPTrack(data + offset + 12, trackLen - 12, this));
		
		offset += trackLen;
	}
}

// ----------------------------------------------------------------------------
bool SequenceHMP::isValid(const uint8_t *data, size_t size)
{
	if (size < 0x40)
		return false;
	
	return !memcmp(data, "HMIMIDIP", 8);
}

// ----------------------------------------------------------------------------
void SequenceHMP::setTimePerBeat(uint32_t usec)
{
	// ?
}
