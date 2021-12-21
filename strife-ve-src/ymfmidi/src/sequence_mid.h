#ifndef __SEQUENCE_MID_H
#define __SEQUENCE_MID_H

#include "sequence.h"

class SequenceMID;

class MIDTrack
{
public:
	MIDTrack(const uint8_t *data, size_t size, SequenceMID* sequence);
	virtual ~MIDTrack();
	
	void reset();
	void advance(uint32_t time);
	uint32_t update(OPLPlayer& player);
	
	bool atEnd() const { return m_atEnd; }
	
protected:
	uint32_t readVLQ();
	virtual uint32_t readDelay() { return readVLQ(); }
	int32_t minDelay();
	virtual bool metaEvent(OPLPlayer& player);

	SequenceMID *m_sequence;
	uint8_t *m_data;
	uint32_t m_pos, m_size;
	int32_t m_delay;
	bool m_atEnd;
	uint8_t m_status; // for MIDI running status
	
	// these are used for format-specific track data details
	bool m_initDelay; // true if there is an initial delay value at the start of the track
	bool m_useRunningStatus; // true if running status is supported by this format
	bool m_useNoteDuration; // true if note on events are followed by a length
	
	struct MIDNote
	{
		uint8_t channel, note;
		int32_t delay;
	};
	std::vector<MIDNote> m_notes;
};

class SequenceMID : public Sequence
{
public:
	SequenceMID();
	~SequenceMID();
	
	void reset();
	uint32_t update(OPLPlayer& player);
	
	virtual void setTimePerBeat(uint32_t usec);
	
	unsigned numSongs() const;
	
	static bool isValid(const uint8_t *data, size_t size);

protected:
	std::vector<MIDTrack*> m_tracks;
	
	uint16_t m_type;
	uint16_t m_ticksPerBeat;
	double m_ticksPerSec;

private:
	void read(const uint8_t *data, size_t size);
	virtual void setDefaults();
};

#endif // __SEQUENCE_MUS_H
