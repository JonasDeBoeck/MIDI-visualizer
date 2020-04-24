#include "midi.h"
#include "io/read.h"
#include "io/endianness.h"

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
