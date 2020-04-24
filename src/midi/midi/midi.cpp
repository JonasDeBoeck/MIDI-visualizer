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
