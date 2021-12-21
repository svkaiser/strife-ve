#ifndef __SEQUENCE_HMI_H
#define __SEQUENCE_HMI_H

#include "sequence_mid.h"

class SequenceHMI : public SequenceMID
{
public:
	SequenceHMI();
	~SequenceHMI();
	
	void setTimePerBeat(uint32_t usec);
	
	static bool isValid(const uint8_t *data, size_t size);
	
private:
	void read(const uint8_t *data, size_t size);
};

#endif // __SEQUENCE_HMP_H
