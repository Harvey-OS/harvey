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
uint16_t
beswab(uint16_t s)
{
	uint8_t *p;

	p = (uint8_t*)&s;
	return (p[0]<<8) | p[1];
}

/*
 * big-endian long
 */
uint32_t
beswal(uint32_t l)
{
	uint8_t *p;

	p = (uint8_t*)&l;
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
}

/*
 * big-endian vlong
 */
uint64_t
beswav(uint64_t v)
{
	uint8_t *p;

	p = (uint8_t*)&v;
	return ((uint64_t)p[0]<<56) | ((uint64_t)p[1]<<48) | ((uint64_t)p[2]<<40)
				  | ((uint64_t)p[3]<<32) | ((uint64_t)p[4]<<24)
				  | ((uint64_t)p[5]<<16) | ((uint64_t)p[6]<<8)
				  | (uint64_t)p[7];
}

/*
 * little-endian short
 */
uint16_t
leswab(uint16_t s)
{
	uint8_t *p;

	p = (uint8_t*)&s;
	return (p[1]<<8) | p[0];
}

/*
 * little-endian long
 */
uint32_t
leswal(uint32_t l)
{
	uint8_t *p;

	p = (uint8_t*)&l;
	return (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
}

/*
 * little-endian vlong
 */
uint64_t
leswav(uint64_t v)
{
	uint8_t *p;

	p = (uint8_t*)&v;
	return ((uint64_t)p[7]<<56) | ((uint64_t)p[6]<<48) | ((uint64_t)p[5]<<40)
				  | ((uint64_t)p[4]<<32) | ((uint64_t)p[3]<<24)
				  | ((uint64_t)p[2]<<16) | ((uint64_t)p[1]<<8)
				  | (uint64_t)p[0];
}
