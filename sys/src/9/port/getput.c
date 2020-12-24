#include "u.h"

void
put64be(u8 *p, u64 x)
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
put32be(u8 *p, u32 x)
{
	p[0] = x >> 24;
	p[1] = x >> 16;
	p[2] = x >> 8;
	p[3] = x;
}

void
put24be(u8 *p, int x)
{
	p[0] = x >> 16;
	p[1] = x >> 8;
	p[2] = x;
}

void
put16be(u8 *p, int x)
{
	p[0] = x >> 8;
	p[1] = x;
}

u64
get64be(u8 *p)
{
	return ((u64)p[0] << 56) | ((u64)p[1] << 48) | ((u64)p[2] << 40) | (u64)p[3] << 32 |
	       ((u64)p[4] << 24) | ((u64)p[5] << 16) | ((u64)p[6] << 8) | (u64)p[7];
}

u32
get32be(u8 *p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

u32
get24be(u8 *p)
{
	return ((u32)p[0] << 16) | ((u32)p[1] << 8) | (u32)p[2];
}

u16
get16be(u8 *p)
{
	return ((u16)p[0] << 8) | (u16)p[1];
}

void
put64le(u8 *p, u64 x)
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
put32le(u8 *p, u32 x)
{
	p[0] = x;
	p[1] = x >> 8;
	p[2] = x >> 16;
	p[3] = x >> 24;
}

void
put24le(u8 *p, int x)
{
	p[0] = x;
	p[1] = x >> 8;
	p[2] = x >> 16;
}

void
put16le(u8 *p, int x)
{
	p[0] = x;
	p[1] = x >> 8;
}

u64
get64le(u8 *p)
{
	return ((u64)p[0]) | ((u64)p[1] << 8) | ((u64)p[2] << 16) | (u64)p[3] << 24 |
	       ((u64)p[4] << 32) | ((u64)p[5] << 40) | ((u64)p[6] << 48) | (u64)p[7] << 56;
}

u32
get32le(u8 *p)
{
	return ((u32)p[0]) | ((u32)p[1] << 8) | ((u32)p[2] << 16) | (u32)p[3] << 24;
}

u32
get24le(u8 *p)
{
	return ((u32)p[0]) | ((u32)p[1] << 8) | (u32)p[2] << 16;
}

u16
get16le(u8 *p)
{
	return ((u16)p[0]) | (u16)p[1] << 8;
}
