#include "midi.h"
#include "io/read.h"
#include "io/endianness.h"
void midi::read_chunk_header(std::istream& ss, CHUNK_HEADER* header)
{
	// Gooide enge linker errors
	// io::read_to<CHUNK_HEADER>(ss, header);
	ss.read(reinterpret_cast<char*>(header), sizeof(header));
	io::switch_endianness(&header->size);
}

std::string midi::header_id(CHUNK_HEADER header)
{
	return std::string(header.id, 4);
}
