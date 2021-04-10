
/* $NetBSD: in_cksum.c,v 1.7 1997/09/02 13:18:15 thorpej Exp $ */

/*-
 * Copyright (c) 1988, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1996
 *	Matt Thomas <matt@3am-software.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)in_cksum.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Checksum routine for Internet Protocol family headers
 *    (Portable Alpha version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

#define ADDCARRY(x) (x > 65535 ? x -= 65535 : x)
#define REDUCE32                                                             \
	{                                                                    \
		q_util.q = sum;                                              \
		sum = q_util.s[0] + q_util.s[1] + q_util.s[2] + q_util.s[3]; \
	}
#define REDUCE16                                                                  \
	{                                                                         \
		q_util.q = sum;                                                   \
		l_util.l = q_util.s[0] + q_util.s[1] + q_util.s[2] + q_util.s[3]; \
		sum = l_util.s[0] + l_util.s[1];                                  \
		ADDCARRY(sum);                                                    \
	}

static const u32 in_masks[] = {
	/*0 bytes*/ /*1 byte*/ /*2 bytes*/		/*3 bytes*/
	0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF, /* offset 0 */
	0x00000000, 0x0000FF00, 0x00FFFF00, 0xFFFFFF00, /* offset 1 */
	0x00000000, 0x00FF0000, 0xFFFF0000, 0xFFFF0000, /* offset 2 */
	0x00000000, 0xFF000000, 0xFF000000, 0xFF000000, /* offset 3 */
};

union l_util {
	u16 s[2];
	u32 l;
};
union q_util {
	u16 s[4];
	u32 l[2];
	u64 q;
};

static u64
in_cksumdata(const void *buf, int len)
{
	const u32 *lw = (const u32 *)buf;
	u64 sum = 0;
	u64 prefilled;
	int offset;
	union q_util q_util;

	if((3 & (long)lw) == 0 && len == 20){
		sum = (u64)lw[0] + lw[1] + lw[2] + lw[3] + lw[4];
		REDUCE32;
		return sum;
	}

	if((offset = 3 & (long)lw) != 0){
		const u32 *masks = in_masks + (offset << 2);
		lw = (u32 *)(((long)lw) - offset);
		sum = *lw++ & masks[len >= 3 ? 3 : len];
		len -= 4 - offset;
		if(len <= 0){
			REDUCE32;
			return sum;
		}
	}
#if 0
	/*
	 * Force to cache line boundary.
	 */
	offset = 32 - (0x1f & (long) lw);
	if (offset < 32 && len > offset){
		len -= offset;
		if (4 & offset){
			sum += (u64) lw[0];
			lw += 1;
		}
		if (8 & offset){
			sum += (u64) lw[0] + lw[1];
			lw += 2;
		}
		if (16 & offset){
			sum += (u64) lw[0] + lw[1] + lw[2] + lw[3];
			lw += 4;
		}
	}
#endif
	/*
	 * access prefilling to start load of next cache line.
	 * then add current cache line
	 * save result of prefilling for loop iteration.
	 */
	prefilled = lw[0];
	while((len -= 32) >= 4){
		u64 prefilling = lw[8];
		sum += prefilled + lw[1] + lw[2] + lw[3] + lw[4] + lw[5] + lw[6] + lw[7];
		lw += 8;
		prefilled = prefilling;
	}
	if(len >= 0){
		sum += prefilled + lw[1] + lw[2] + lw[3] + lw[4] + lw[5] + lw[6] + lw[7];
		lw += 8;
	} else {
		len += 32;
	}
	while((len -= 16) >= 0){
		sum += (u64)lw[0] + lw[1] + lw[2] + lw[3];
		lw += 4;
	}
	len += 16;
	while((len -= 4) >= 0){
		sum += (u64)*lw++;
	}
	len += 4;
	if(len > 0)
		sum += (u64)(in_masks[len] & *lw);
	REDUCE32;
	return sum;
}
u16
ptclbsum(u8 *addr, int len)
{
	u64 sum = in_cksumdata(addr, len);
	union q_util q_util;
	union l_util l_util;
	REDUCE16;
	return ((sum & (u16)0x00ffU) << 8) |
	       ((sum & (u16)0xff00U) >> 8);
}
