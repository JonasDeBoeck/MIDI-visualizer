#ifndef MIDI_H
#define MIDI_H

#include <cstdint>
#include <istream>

namespace midi {
	struct CHUNK_HEADER
	{
		char id[4];
		uint32_t size;
	};

	void read_chunk_header(std::istream& ss, CHUNK_HEADER* header);

	std::string header_id(CHUNK_HEADER header);
}

#endif