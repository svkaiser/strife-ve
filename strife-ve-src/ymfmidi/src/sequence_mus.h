#ifndef __SEQUENCE_MUS_H
#define __SEQUENCE_MUS_H

#include "sequence.h"

class SequenceMUS : public Sequence
{
public:
	SequenceMUS();
	
	void reset();
	uint32_t update(OPLPlayer& player);
	
	static bool isValid(const uint8_t *data, size_t size);
	
private:
	void read(const uint8_t *data, size_t size);
	void setDefaults();
	
	uint8_t m_data[1 << 16];
	uint16_t m_pos;
	uint8_t m_lastVol[16];
};

#endif // __SEQUENCE_MUS_H
