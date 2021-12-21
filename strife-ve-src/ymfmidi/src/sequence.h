#ifndef __SEQUENCE_H
#define __SEQUENCE_H

#include "player.h"

class Sequence
{
public:
	Sequence()
	{ 
		m_atEnd = false;
		m_songNum = 0;
	}
	virtual ~Sequence();
	
	// load a sequence from the given path/file
	static Sequence* load(const char *path);
	static Sequence* load(FILE *path, int offset = 0, size_t size = 0);
	static Sequence* load(const uint8_t *data, size_t size);
	
	// reset track to beginning
	virtual void reset() { m_atEnd = false; }
	
	// process and play any pending MIDI events
	// returns the number of output audio samples until the next event(s)
	virtual uint32_t update(OPLPlayer& player) = 0;
	
	virtual void setSongNum(unsigned num)
	{
		if (num < numSongs())
			m_songNum = num;
		reset();
	}
	virtual unsigned numSongs() const { return 1; }
	unsigned songNum() const { return m_songNum; }
	
	// has this track reached the end?
	// (this is true immediately after ending/looping, then becomes false after updating again)
	bool atEnd() const { return m_atEnd; }
	
protected:
	bool m_atEnd;
	unsigned m_songNum;
	
private:
	virtual void read(const uint8_t *data, size_t size) = 0;
};

#endif // __SEQUENCE_H

