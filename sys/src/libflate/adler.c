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
#include <flate.h>

enum
{
	ADLERITERS	= 5552,	/* max iters before can overflow 32 bits */
	ADLERBASE	= 65521 /* largest prime smaller than 65536 */
};

u32
adler32(u32 adler, void *vbuf, int n)
{
	u32 s1, s2;
	u8 *buf, *ebuf;
	int m;

	buf = vbuf;
	s1 = adler & 0xffff;
	s2 = (adler >> 16) & 0xffff;
	for(; n >= 16; n -= m){
		m = n;
		if(m > ADLERITERS)
			m = ADLERITERS;
		m &= ~15;
		for(ebuf = buf + m; buf < ebuf; buf += 16){
			s1 += buf[0];
			s2 += s1;
			s1 += buf[1];
			s2 += s1;
			s1 += buf[2];
			s2 += s1;
			s1 += buf[3];
			s2 += s1;
			s1 += buf[4];
			s2 += s1;
			s1 += buf[5];
			s2 += s1;
			s1 += buf[6];
			s2 += s1;
			s1 += buf[7];
			s2 += s1;
			s1 += buf[8];
			s2 += s1;
			s1 += buf[9];
			s2 += s1;
			s1 += buf[10];
			s2 += s1;
			s1 += buf[11];
			s2 += s1;
			s1 += buf[12];
			s2 += s1;
			s1 += buf[13];
			s2 += s1;
			s1 += buf[14];
			s2 += s1;
			s1 += buf[15];
			s2 += s1;
		}
		s1 %= ADLERBASE;
		s2 %= ADLERBASE;
	}
	if(n){
		for(ebuf = buf + n; buf < ebuf; buf++){
			s1 += buf[0];
			s2 += s1;
		}
		s1 %= ADLERBASE;
		s2 %= ADLERBASE;
	}
	return (s2 << 16) + s1;
}
