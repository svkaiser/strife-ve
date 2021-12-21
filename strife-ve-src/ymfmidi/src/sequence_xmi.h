#ifndef __SEQUENCE_XMI_H
#define __SEQUENCE_XMI_H

#include "sequence_mid.h"

class SequenceXMI : public SequenceMID
{
public:
	SequenceXMI();
	~SequenceXMI();
	
	void setTimePerBeat(uint32_t usec);
	
	static bool isValid(const uint8_t *data, size_t size);
	
private:
	void read(const uint8_t *data, size_t size);
	uint32_t readRootChunk(const uint8_t *data, size_t size);
};

#endif // __SEQUENCE_XMI_H
