#include "endianness.h"

void io::switch_endianness(uint16_t* n)
{
	uint16_t right = (*n & 0x00FF) >> 0;
	uint16_t left = (*n & 0xFF00) >> 8;
	*n = ((right << 8) | left);
}

void io::switch_endianness(uint32_t* n)
{
	uint32_t right = (*n & 0x000000FF) >> 0;
	uint32_t rightmid = (*n & 0x0000FF00) >> 8;
	uint32_t leftmid = (*n & 0x00FF0000) >> 16;
	uint32_t left = (*n & 0xFF000000) >> 24;
	*n = ((right << 24) | (rightmid << 16) | (leftmid << 8) | left);
}

void io::switch_endianness(uint64_t* n)
{
	uint64_t right1 = (*n & 0x00000000000000FF) >> 0;
	uint64_t right2 = (*n & 0x000000000000FF00) >> 8;
	uint64_t rightmid = (*n & 0x0000000000FF0000) >> 16;
	uint64_t mid1 = (*n & 0x00000000FF000000) >> 24;
	uint64_t mid2 = (*n & 0x000000FF00000000) >> 32;
	uint64_t leftmid = (*n & 0x0000FF0000000000) >> 40;
	uint64_t left2 = (*n & 0x00FF000000000000) >> 48;
	uint64_t left1 = (*n & 0xFF00000000000000) >> 56;
	*n = ((right1 <<= 56) | (right2 <<= 48) | (rightmid <<= 40) | (mid1 <<= 32) | (mid2 <<= 24) | (leftmid <<= 16) | (left2 <<= 8) | left1);
}
