#include "midi.h"
#include "io/read.h"
#include "io/endianness.h"
#include "io/vli.h"

void midi::read_chunk_header(std::istream& ss, CHUNK_HEADER* header)
{
	// io::read_to<CHUNK_HEADER>(ss, header); => linker error
	ss.read(reinterpret_cast<char*>(header), sizeof(CHUNK_HEADER));
	io::switch_endianness(&header->size);
}

std::string midi::header_id(CHUNK_HEADER header)
{
	return std::string(header.id, 4);
}

void midi::read_mthd(std::istream& ss, MTHD* mthd)
{
	ss.read(reinterpret_cast<char*>(mthd), sizeof(MTHD));
	io::switch_endianness(&mthd->ntracks);
	io::switch_endianness(&mthd->division);
	io::switch_endianness(&mthd->type);
	io::switch_endianness(&mthd->header.size);
}

bool midi::is_sysex_event(uint8_t byte)
{
	return byte == 0xF0 || byte == 0xF7;
}

bool midi::is_meta_event(uint8_t byte)
{
	return byte == 0xFF;
}

bool midi::is_midi_event(uint8_t byte)
{
	return (byte >> 4) == 0x08 || (byte >> 4) == 0x09 || (byte >> 4) == 0x0A || (byte >> 4) == 0x0B || (byte >> 4) == 0x0C || (byte >> 4) == 0x0D || (byte >> 4) == 0x0E;
}

bool midi::is_running_status(uint8_t byte)
{
	return byte >> 7 == 0x00;
}

uint8_t midi::extract_midi_event_type(uint8_t status)
{
	return status >> 4;
}

midi::Channel midi::extract_midi_event_channel(uint8_t status)
{
	return Channel(status & 0x0F);
}

bool midi::is_note_off(uint8_t status)
{
	return status == 0x08;
}

bool midi::is_note_on(uint8_t status)
{
	return status == 0x09;
}

bool midi::is_polyphonic_key_pressure(uint8_t status)
{
	return status == 0x0A;
}

bool midi::is_control_change(uint8_t status)
{
	return status == 0x0B;
}

bool midi::is_program_change(uint8_t status)
{
	return status == 0x0C;
}

bool midi::is_channel_pressure(uint8_t status)
{
	return status == 0x0D;
}

bool midi::is_pitch_wheel_change(uint8_t status)
{
	return status == 0x0E;
}

void midi::read_mtrk(std::istream& in, EventReceiver& receiver)
{
	CHUNK_HEADER header;
	read_chunk_header(in, &header);
	std::string vorige_event;
	Channel vorige_channel;
	if (header_id(header) == "MTrk")
	{
		bool end_of_track_reached = false;
		while (!end_of_track_reached)
		{
			Duration duration = Duration(io::read_variable_length_integer(in));
			uint8_t event = in.get();
			if (!is_running_status(event))
			{
				if (is_meta_event(event)) {
					vorige_event = "meta";
					uint8_t identifier = in.get();
					uint64_t length = io::read_variable_length_integer(in);
					auto data = std::make_unique<uint8_t[]>(length);
					in.read(reinterpret_cast<char*>(data.get()), length);
					receiver.meta(duration, identifier, std::move(data), length);
					if (identifier == 0x2F)
					{
						end_of_track_reached = true;
					}
				}
				else if (is_sysex_event(event)) {
					vorige_event = "sysex";
					uint64_t length = io::read_variable_length_integer(in);
					auto data = std::make_unique<uint8_t[]>(length);
					in.read(reinterpret_cast<char*>(data.get()), length);
					receiver.sysex(duration, std::move(data), length);
				}
				else if (is_midi_event(event))
				{
					if (is_note_off(extract_midi_event_type(event))) {
						vorige_event = "note_off";
						vorige_channel = extract_midi_event_channel(event);
						NoteNumber note = NoteNumber(in.get());
						uint8_t velocity = in.get();
						receiver.note_off(duration, extract_midi_event_channel(event), note, velocity);
					}
					else if (is_note_on(extract_midi_event_type(event))) {
						vorige_event = "note_on";
						vorige_channel = extract_midi_event_channel(event);
						NoteNumber note = NoteNumber(in.get());
						uint8_t velocity = in.get();
						receiver.note_on(duration, extract_midi_event_channel(event), note, velocity);
					}
					else if (is_polyphonic_key_pressure(extract_midi_event_type(event))) {
						vorige_event = "polyphonic";
						vorige_channel = extract_midi_event_channel(event);
						NoteNumber note = NoteNumber(in.get());
						uint8_t pressure = in.get();
						receiver.polyphonic_key_pressure(duration, extract_midi_event_channel(event), note, pressure);
					}
					else if (is_control_change(extract_midi_event_type(event))) {
						vorige_event = "control_change";
						vorige_channel = extract_midi_event_channel(event);
						uint8_t controller = in.get();
						uint8_t value = in.get();
						receiver.control_change(duration, extract_midi_event_channel(event), controller, value);
					}
					else if (is_program_change(extract_midi_event_type(event))) {
						vorige_event = "program_change";
						vorige_channel = extract_midi_event_channel(event);
						Instrument program = Instrument(in.get());
						receiver.program_change(duration, extract_midi_event_channel(event), program);
					}
					else if (is_channel_pressure(extract_midi_event_type(event))) {
						vorige_event = "channel_pressure";
						vorige_channel = extract_midi_event_channel(event);
						uint8_t pressure = in.get();
						receiver.channel_pressure(duration, extract_midi_event_channel(event), pressure);
					}
					else if (is_pitch_wheel_change((extract_midi_event_type(event))))
					{
						vorige_event = "pitch_wheel_change";
						vorige_channel = extract_midi_event_channel(event);
						uint8_t lower_bits = in.get() & 0b01111111;
						uint8_t upper_bits = in.get() & 0b01111111;
						uint16_t value = (upper_bits << 7) | lower_bits;
						receiver.pitch_wheel_change(duration, extract_midi_event_channel(event), value);
					}
				}
			}
			else {
				if (vorige_event == "meta") {
					uint8_t identifier = event;
					uint64_t length = io::read_variable_length_integer(in);
					auto data = std::make_unique<uint8_t[]>(length);
					in.read(reinterpret_cast<char*>(data.get()), length);
					receiver.meta(duration, identifier, std::move(data), length);
					if (identifier == 0x2F)
					{
						end_of_track_reached = true;
					}
				}
				else if (vorige_event == "syskey") {
					uint64_t length = io::read_variable_length_integer(in);
					auto data = std::make_unique<uint8_t[]>(length);
					in.read(reinterpret_cast<char*>(data.get()), length);
					receiver.sysex(duration, std::move(data), length);
				}
				else if (vorige_event == "note_off") {
					NoteNumber note = NoteNumber(event);
					uint8_t velocity = in.get();
					receiver.note_off(duration, vorige_channel, note, velocity);
				}
				else if (vorige_event == "note_on") {
					NoteNumber note = NoteNumber(event);
					uint8_t velocity = in.get();
					receiver.note_on(duration, vorige_channel, note, velocity);
				}
				else if (vorige_event == "polyphonic") {
					NoteNumber note = NoteNumber(event);
					uint8_t pressure = in.get();
					receiver.polyphonic_key_pressure(duration, vorige_channel, note, pressure);
				}
				else if (vorige_event == "control_change") {
					Instrument program = Instrument(event);
					receiver.program_change(duration, vorige_channel, program);
				}
				else if (vorige_event == "program_change") {
					Instrument program = Instrument(event);
					receiver.program_change(duration, vorige_channel, program);
				}
				else if (vorige_event == "channel_pressure") {
					uint8_t pressure = event;
					receiver.channel_pressure(duration, vorige_channel, pressure);
				}
				else if (vorige_event == "pitch_wheel_change") {
					uint8_t lower_bits = event & 0b01111111;
					uint8_t upper_bits = in.get() & 0b01111111;
					uint16_t value = (upper_bits << 7) | lower_bits;
					receiver.pitch_wheel_change(duration, vorige_channel, value);
				}
			}
		}
	}
}

std::ostream& midi::operator<<(std::ostream& out, const NOTE& note)
{
	return out << "Note(number=" << note.note_number << ",start=" << note.start << ",duration=" << note.duration << ",instrument=" << note.instrument << ")";
}

bool midi::operator==(const NOTE& x, const NOTE& y)
{
	return x.duration == y.duration && x.instrument == y.instrument && x.note_number == y.note_number && x.start == y.start && x.velocity == y.velocity;
}

bool midi::operator!=(const NOTE& x, const NOTE& y)
{
	return !(x == y);
}

void midi::ChannelNoteCollector::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
{
	if (channel == this->channel)
	{
		if (velocity == 0b00000000)
		{
			note_off(dt, channel, note, velocity);
		}
		else {
			this->start += dt;
			NOTE noot = NOTE();
			noot.note_number = note;
			noot.start = this->start;
			noot.velocity = velocity;
			noot.instrument = this->instrument;
			bool exists = false;
			for (int i = 0; i < this->notes.size(); i++)
			{
				if (this->notes[i].note_number == note)
				{
					note_off(dt, channel, note, velocity);
					exists = true;
					break;
				}
			}
			if (!exists)
			{
				for (int i = 0; i < this->notes.size(); i++)
				{
					this->notes[i].duration += dt;
				}
			}
			this->notes.push_back(noot);
		}
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
{
	if (channel == this->channel)
	{
		int index = -1;
		for (int i = 0; i < this->notes.size(); i++)
		{
			this->notes[i].duration += dt;
			if (this->notes[i].note_number == note) {
				NOTE noot = this->notes[i];
				this->note_receiver(noot);
				index = i;
			}
		}
		if (index != -1)
		{
			this->notes.erase(this->notes.begin() + index);
		}
		this->start += dt;
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure)
{
	if (channel == this->channel)
	{
		for (int i = 0; i < this->notes.size(); i++)
		{
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value)
{
	if (channel == this->channel)
	{
		for (int i = 0; i < this->notes.size(); i++)
		{
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::program_change(Duration dt, Channel channel, Instrument program)
{
	if (channel == this->channel)
	{
		this->instrument = program;
		for (int i = 0; i < this->notes.size(); i++)
		{
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::channel_pressure(Duration dt, Channel channel, uint8_t pressure)
{
	if (channel == this->channel)
	{
		for (int i = 0; i < this->notes.size(); i++)
		{
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::pitch_wheel_change(Duration dt, Channel channel, uint16_t value)
{
	if (channel == this->channel)
	{
		for (int i = 0; i < this->notes.size(); i++)
		{
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}
	else {
		action_other_channel(dt);
	}
}

void midi::ChannelNoteCollector::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
{
	for (int i = 0; i < this->notes.size(); i++)
	{
		this->notes[i].duration += dt;
	}
	this->start += dt;
}

void midi::ChannelNoteCollector::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
{
	for (int i = 0; i < this->notes.size(); i++)
	{
		this->notes[i].duration += dt;
	}
	this->start += dt;
}

void midi::ChannelNoteCollector::action_other_channel(Duration dt)
{
	for (int i = 0; i < this->notes.size(); i++)
	{
		this->notes[i].duration += dt;
	}
	this->start += dt;
}

void midi::EventMulticaster::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->note_on(dt, channel, note, velocity);
	}
}

void midi::EventMulticaster::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->note_off(dt, channel, note, velocity);
	}
}

void midi::EventMulticaster::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->polyphonic_key_pressure(dt, channel, note, pressure);
	}
}

void midi::EventMulticaster::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->control_change(dt, channel, controller, value);
	}
}

void midi::EventMulticaster::program_change(Duration dt, Channel channel, Instrument program)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->program_change(dt, channel, program);
	}
}

void midi::EventMulticaster::channel_pressure(Duration dt, Channel channel, uint8_t pressure)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->channel_pressure(dt, channel, pressure);
	}
}

void midi::EventMulticaster::pitch_wheel_change(Duration dt, Channel channel, uint16_t value)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->pitch_wheel_change(dt, channel, value);
	}
}

void midi::EventMulticaster::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->meta(dt, type, std::move(data), data_size);
	}
}

void midi::EventMulticaster::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
{
	for (int i = 0; i < this->event_receiver.size(); i++)
	{
		this->event_receiver[i]->sysex(dt, std::move(data), data_size);
	}
}

void midi::NoteCollector::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
{
	this->event_multicaster.note_on(dt, channel, note, velocity);
}

void midi::NoteCollector::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity)
{
	this->event_multicaster.note_off(dt, channel, note, velocity);
}

void midi::NoteCollector::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure)
{
	this->event_multicaster.polyphonic_key_pressure(dt, channel, note, pressure);
}

void midi::NoteCollector::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value)
{
	this->event_multicaster.control_change(dt, channel, controller, value);
}

void midi::NoteCollector::program_change(Duration dt, Channel channel, Instrument program)
{
	this->event_multicaster.program_change(dt, channel, program);
}

void midi::NoteCollector::channel_pressure(Duration dt, Channel channel, uint8_t pressure)
{
	this->event_multicaster.channel_pressure(dt, channel, pressure);
}

void midi::NoteCollector::pitch_wheel_change(Duration dt, Channel channel, uint16_t value)
{
	this->event_multicaster.pitch_wheel_change(dt, channel, value);
}

void midi::NoteCollector::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
{
	this->event_multicaster.meta(dt, type, std::move(data), data_size);
}

void midi::NoteCollector::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size)
{
	this->event_multicaster.sysex(dt, std::move(data), data_size);

}

std::vector<midi::NOTE> midi::read_notes(std::istream& in)
{
	std::vector<midi::NOTE> notes;
	MTHD mthd;
	read_mthd(in, &mthd);
	for (int i = 0; i < mthd.ntracks; i++)
	{
		NoteCollector note_collector = NoteCollector([&notes](const midi::NOTE& note) { notes.push_back(note); });
		read_mtrk(in, note_collector);	
	}
	return notes;
}