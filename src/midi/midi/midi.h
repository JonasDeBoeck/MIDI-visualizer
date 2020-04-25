#ifndef MIDI_H
#define MIDI_H

#include <cstdint>
#include <istream>
#include <functional>
#include "midi/primitives.h"
#include <vector>

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
		NOTE() : note_number(0), start(0), duration(0), velocity(0), instrument(0) {};
	};

	std::ostream& operator <<(std::ostream& out, const NOTE& note);
	bool operator ==(const NOTE& x, const NOTE& y);
	bool operator !=(const NOTE& x, const NOTE& y);

	class ChannelNoteCollector : public EventReceiver 
	{
		Channel channel;
		std::function<void(const NOTE&)> note_receiver;
		std::vector<NOTE> notes;
		Time start;
		Instrument instrument;

	public:
		ChannelNoteCollector(Channel channel, std::function<void(const NOTE&)> note_receiver) : channel(channel), note_receiver(note_receiver), start(0), notes(0), instrument(0) {};

		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;

		void action_other_channel(Duration dt);
	};

	class EventMulticaster : EventReceiver {
		std::vector<std::shared_ptr<EventReceiver>> event_receiver;

	public:
		EventMulticaster(std::vector<std::shared_ptr<EventReceiver>> event_receiver) : event_receiver(event_receiver) {};
		EventMulticaster() : event_receiver({}) {};

		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
	};

	class NoteCollector : public EventReceiver {
		std::function<void(const NOTE&)> note_receiver;
		EventMulticaster event_multicaster;

	public:
		NoteCollector(std::function<void(const NOTE&)> note_receiver) : note_receiver(note_receiver) {
			std::vector<std::shared_ptr<EventReceiver>> event_receiver;
			for (int i = 0; i < 16; i++)
			{
				ChannelNoteCollector channel_note_controller = ChannelNoteCollector(Channel(i), note_receiver);
				std::shared_ptr<ChannelNoteCollector> receiver = std::make_shared<ChannelNoteCollector>(channel_note_controller);
				event_receiver.push_back(receiver);
			}
			event_multicaster = EventMulticaster(event_receiver);
		};

		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
	};

	std::vector<NOTE> read_notes(std::istream& in);
}

#endif