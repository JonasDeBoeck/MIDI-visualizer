#ifndef ENDIANNESS_H
#define ENDIANNESS_H
#include "stdint.h"
namespace io {
	void switch_endianness(uint16_t* n);
	void switch_endianness(uint32_t* n);
	void switch_endianness(uint64_t* n);
}
#endif