/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>

/*
 * big-endian short
 */
u16
beswab(u16 s)
{
	u8 *p;

	p = (u8*)&s;
	return (p[0]<<8) | p[1];
}

/*
 * big-endian long
 */
u32
beswal(u32 l)
{
	u8 *p;

	p = (u8*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

/*
 * big-endian vlong
 */
u64
beswav(u64 v)
{
	u8 *p;

	p = (u8*)&v;
	return ((u64)p[0]<<56) | ((u64)p[1]<<48) | ((u64)p[2]<<40)
				  | ((u64)p[3]<<32) | ((u64)p[4]<<24)
				  | ((u64)p[5]<<16) | ((u64)p[6]<<8)
				  | (u64)p[7];
}

/*
 * little-endian short
 */
u16
leswab(u16 s)
{
	u8 *p;

	p = (u8*)&s;
	return (p[1]<<8) | p[0];
}

/*
 * little-endian long
 */
u32
leswal(u32 l)
{
	u8 *p;

	p = (u8*)&l;
	return (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
}

/*
 * little-endian vlong
 */
u64
leswav(u64 v)
{
	u8 *p;

	p = (u8*)&v;
	return ((u64)p[7]<<56) | ((u64)p[6]<<48) | ((u64)p[5]<<40)
				  | ((u64)p[4]<<32) | ((u64)p[3]<<24)
				  | ((u64)p[2]<<16) | ((u64)p[1]<<8)
				  | (u64)p[0];
}
