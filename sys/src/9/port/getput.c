#include "u.h"

void
put64be(uint8_t *p, uint64_t x)
{
	p[0] = x >> 56;
	p[1] = x >> 48;
	p[2] = x >> 40;
	p[3] = x >> 32;
	p[4] = x >> 24;
	p[5] = x >> 16;
	p[6] = x >> 8;
	p[7] = x;
}

void
put32be(uint8_t *p, uint32_t x)
{
	p[0] = x >> 24;
	p[1] = x >> 16;
	p[2] = x >> 8;
	p[3] = x;
}

void
put24be(uint8_t *p, int x)
{
	p[0] = x >> 16;
	p[1] = x >> 8;
	p[2] = x;
}

void
put16be(uint8_t *p, int x)
{
	p[0] = x >> 8;
	p[1] = x;
}

uint64_t
get64be(uint8_t *p)
{
	return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) | ((uint64_t)p[2] << 40) | (uint64_t)p[3] << 32 |
	       ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) | ((uint64_t)p[6] << 8) | (uint64_t)p[7];
}

uint32_t
get32be(uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

uint32_t
get24be(uint8_t *p)
{
	return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}

uint16_t
get16be(uint8_t *p)
{
	return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

void
put64le(uint8_t *p, uint64_t x)
{
	p[0] = x;
	p[1] = x >> 8;
	p[2] = x >> 16;
	p[3] = x >> 24;
	p[4] = x >> 32;
	p[5] = x >> 40;
	p[6] = x >> 48;
	p[7] = x >> 56;
}

void
put32le(uint8_t *p, uint32_t x)
{
	p[0] = x;
	p[1] = x >> 8;
	p[2] = x >> 16;
	p[3] = x >> 24;
}

void
put24le(uint8_t *p, int x)
{
	p[0] = x;
	p[1] = x >> 8;
	p[2] = x >> 16;
}

void
put16le(uint8_t *p, int x)
{
	p[0] = x;
	p[1] = x >> 8;
}

uint64_t
get64le(uint8_t *p)
{
	return ((uint64_t)p[0]) | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | (uint64_t)p[3] << 24 |
	       ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | (uint64_t)p[7] << 56;
}

uint32_t
get32le(uint8_t *p)
{
	return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | (uint32_t)p[3] << 24;
}

uint32_t
get24le(uint8_t *p)
{
	return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | (uint32_t)p[2] << 16;
}

uint16_t
get16le(uint8_t *p)
{
	return ((uint16_t)p[0]) | (uint16_t)p[1] << 8;
}
