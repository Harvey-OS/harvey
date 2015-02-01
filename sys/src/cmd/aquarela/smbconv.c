/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

ushort
smbnhgets(uchar *p)
{
	return p[0] | (p[1] << 8);
}

ulong
smbnhgetl(uchar *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void
smbhnputs(uchar *p, ushort v)
{
	p[0] = v;
	p[1] = v >> 8;
}

void
smbhnputl(uchar *p, ulong v)
{
	p[0] = v;
	p[1] = v >> 8;
	p[2] = v >> 16;
	p[3] = v >> 24;
}

void
smbhnputv(uchar *p, vlong v)
{
	smbhnputl(p, v);
	smbhnputl(p + 4, (v >> 32) & 0xffffffff);
}

vlong
smbnhgetv(uchar *p)
{
	return (vlong)smbnhgetl(p) | ((vlong)smbnhgetl(p + 4) << 32);
}
