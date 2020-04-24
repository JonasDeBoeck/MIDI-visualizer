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
}

#endif