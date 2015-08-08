/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

uint16_t
smbnhgets(uint8_t* p)
{
	return p[0] | (p[1] << 8);
}

uint32_t
smbnhgetl(uint8_t* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void
smbhnputs(uint8_t* p, uint16_t v)
{
	p[0] = v;
	p[1] = v >> 8;
}

void
smbhnputl(uint8_t* p, uint32_t v)
{
	p[0] = v;
	p[1] = v >> 8;
	p[2] = v >> 16;
	p[3] = v >> 24;
}

void
smbhnputv(uint8_t* p, int64_t v)
{
	smbhnputl(p, v);
	smbhnputl(p + 4, (v >> 32) & 0xffffffff);
}

int64_t
smbnhgetv(uint8_t* p)
{
	return (int64_t)smbnhgetl(p) | ((int64_t)smbnhgetl(p + 4) << 32);
}
