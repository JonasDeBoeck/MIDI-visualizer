#ifndef MIDI_H
#define MIDI_H

#include <cstdint>
#include <istream>
#include "midi/primitives.h"

namespace midi {
	struct CHUNK_HEADER
	{
		char id[4];
		uint32_t size;
	};

	void read_chunk_header(std::istream& ss, CHUNK_HEADER* header);

	std::string header_id(CHUNK_HEADER header);

#pragma pack(push, 1)

	struct MTHD 
	{
		CHUNK_HEADER header;
		uint16_t type;
		uint16_t ntracks;
		uint16_t division;
	};


#pragma pack(pop)

	void read_mthd(std::istream& ss, MTHD* mthd);

	bool is_sysex_event(uint8_t byte);
	bool is_meta_event(uint8_t byte);
	bool is_midi_event(uint8_t byte);
	bool is_running_status(uint8_t byte);
	uint8_t extract_midi_event_type(uint8_t status);
	Channel extract_midi_event_channel(uint8_t status);
	bool is_note_off(uint8_t status);
	bool is_note_on(uint8_t status);
	bool is_polyphonic_key_pressure(uint8_t status);
	bool is_control_change(uint8_t status);
	bool is_program_change(uint8_t status);
	bool is_channel_pressure(uint8_t status);
	bool is_pitch_wheel_change(uint8_t status);

	class EventReceiver 
	{
	public:
		virtual void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) = 0;
		virtual void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) = 0;
		virtual void program_change(Duration dt, Channel channel, Instrument program) = 0;
		virtual void channel_pressure(Duration dt, Channel channel, uint8_t pressure) = 0;
		virtual void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) = 0;
		virtual void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
		virtual void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
	};

	void read_mtrk(std::istream& in, EventReceiver& receiver);

	struct NOTE
	{
		NoteNumber note_number;
		Time start;
		Duration duration;
		uint8_t velocity;
		Instrument instrument;

		NOTE(NoteNumber note_number, Time start, Duration duration, uint8_t velocity, Instrument instrument) : note_number(note_number), start(start), duration(duration), velocity(velocity), instrument(instrument) {};
	};

	std::ostream& operator <<(std::ostream& out, const NOTE& note);
	bool operator ==(const NOTE& x, const NOTE& y);
	bool operator !=(const NOTE& x, const NOTE& y);
}

#endif