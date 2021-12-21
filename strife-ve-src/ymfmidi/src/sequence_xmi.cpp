#include "sequence_xmi.h"

#include <cstring>

#define READ_U16BE(data, pos) ((data[pos] << 8) | data[pos+1])
#define READ_U24BE(data, pos) ((data[pos] << 16) | (data[pos+1] << 8) | data[pos+2])
#define READ_U32BE(data, pos) ((data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3])

class XMITrack : public MIDTrack
{
public:
	XMITrack(const uint8_t *data, size_t size, SequenceXMI* sequence);
	
protected:
	uint32_t readDelay();
};

// ----------------------------------------------------------------------------
XMITrack::XMITrack(const uint8_t *data, size_t size, SequenceXMI *sequence)
	: MIDTrack(data, size, sequence)
{
	m_initDelay = false;
	m_useRunningStatus = false;
	m_useNoteDuration = true;
}

// ----------------------------------------------------------------------------
uint32_t XMITrack::readDelay()
{
	uint32_t delay = 0;
	uint8_t data = 0;

	if (m_pos >= m_size || (m_data[m_pos] & 0x80))
		return 0;

	do
	{
		data = m_data[m_pos];
		if (!(data & 0x80))
		{
			delay += data;
			m_pos++;
		}
	} while ((data == 0x7f) && (m_pos < m_size));
	
	return delay;
}

// ----------------------------------------------------------------------------
SequenceXMI::SequenceXMI()
	: SequenceMID()
{
	m_type = 2;
	m_ticksPerBeat = 0; // unused
	m_ticksPerSec = 120;
}

// ----------------------------------------------------------------------------
SequenceXMI::~SequenceXMI() {}

// ----------------------------------------------------------------------------
void SequenceXMI::read(const uint8_t *data, size_t size)
{
	uint32_t chunkSize;
	while ((chunkSize = readRootChunk(data, size)) != 0)
	{
		data += chunkSize;
		size -= chunkSize;
	}
}

// ----------------------------------------------------------------------------
uint32_t SequenceXMI::readRootChunk(const uint8_t *data, size_t size)
{
	// need at least a root chunk and one subchunk (and its contents)
	if (size > 12 + 8)
	{
		// length of the root chunk
		uint32_t rootLen = READ_U32BE(data, 4);
		rootLen = (rootLen + 1) & ~1;
		// offset to the current sub-chunk
		uint32_t offset = 12;
		// offset to the data after the root chunk
		uint32_t rootEnd = std::min(rootLen + 8, (uint32_t)size);
		
		if (!memcmp(data, "FORM", 4))
		{
			uint32_t chunkLen;
		
			while (offset < rootEnd)
			{
				const uint8_t *bytes = data + offset;
				
				chunkLen = READ_U32BE(bytes, 4);
				chunkLen = (chunkLen + 1) & ~1;
				
				// move to next subchunk
				offset += chunkLen + 8;
				if (offset > rootEnd)
				{
					// try to handle a malformed/truncated chunk
					chunkLen -= (offset - rootEnd);
					offset = rootEnd;
				}
				
				if (!memcmp(bytes, "EVNT", 4))
					m_tracks.push_back(new XMITrack(bytes + 8, chunkLen, this));
			}
		}
		else if (!memcmp(data, "CAT ", 4))
		{
			while (offset < rootEnd)
			{
				offset += readRootChunk(data + offset, size - offset);
			}
		}
		
		return rootEnd;
	}
	
	return 0;
}

// ----------------------------------------------------------------------------
bool SequenceXMI::isValid(const uint8_t *data, size_t size)
{
	// need at least 2 root chunks and one EVNT chunk header
	if (size < 12)
		return false;
	
	if (memcmp(data,     "FORM", 4))
		return false;
	if (memcmp(data + 8, "XDIR", 4))
		return false;
	
	return true;
}

// ----------------------------------------------------------------------------
void SequenceXMI::setTimePerBeat(uint32_t usec)
{
	double usecPerTick = (double)usec / ((usec * 3) / 25000);
	m_ticksPerSec = 1000000 / usecPerTick;
}
