/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

void
hnputv(void *p, u64 v)
{
	u8 *a;

	a = p;
	a[0] = v>>56;
	a[1] = v>>48;
	a[2] = v>>40;
	a[3] = v>>32;
	a[4] = v>>24;
	a[5] = v>>16;
	a[6] = v>>8;
	a[7] = v;
}

void
hnputl(void *p, uint v)
{
	u8 *a;

	a = p;
	a[0] = v>>24;
	a[1] = v>>16;
	a[2] = v>>8;
	a[3] = v;
}

void
hnputs(void *p, u16 v)
{
	u8 *a;

	a = p;
	a[0] = v>>8;
	a[1] = v;
}

u64
nhgetv(void *p)
{
	u8 *a;
	u64 v;

	a = p;
	v = (u64)a[0]<<56;
	v |= (u64)a[1]<<48;
	v |= (u64)a[2]<<40;
	v |= (u64)a[3]<<32;
	v |= a[4]<<24;
	v |= a[5]<<16;
	v |= a[6]<<8;
	v |= a[7]<<0;
	return v;
}

uint
nhgetl(void *p)
{
	u8 *a;

	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

u16
nhgets(void *p)
{
	u8 *a;

	a = p;
	return (a[0]<<8)|(a[1]<<0);
}
