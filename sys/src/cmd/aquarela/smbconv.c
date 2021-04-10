/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

u16
smbnhgets(u8 *p)
{
	return p[0] | (p[1] << 8);
}

u32
smbnhgetl(u8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void
smbhnputs(u8 *p, u16 v)
{
	p[0] = v;
	p[1] = v >> 8;
}

void
smbhnputl(u8 *p, u32 v)
{
	p[0] = v;
	p[1] = v >> 8;
	p[2] = v >> 16;
	p[3] = v >> 24;
}

void
smbhnputv(u8 *p, i64 v)
{
	smbhnputl(p, v);
	smbhnputl(p + 4, (v >> 32) & 0xffffffff);
}

i64
smbnhgetv(u8 *p)
{
	return (i64)smbnhgetl(p) | ((i64)smbnhgetl(p + 4) << 32);
}
