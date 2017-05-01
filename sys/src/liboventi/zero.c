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
#include <oventi.h>

/* score of a zero length block */
uint8_t	vtZeroScore[VtScoreSize] = {
	0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
	0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
};


int
vtZeroExtend(int type, uint8_t *buf, int n, int nn)
{
	uint8_t *p, *ep;

	switch(type) {
	default:
		memset(buf+n, 0, nn-n);
		break;
	case VtPointerType0:
	case VtPointerType1:
	case VtPointerType2:
	case VtPointerType3:
	case VtPointerType4:
	case VtPointerType5:
	case VtPointerType6:
	case VtPointerType7:
	case VtPointerType8:
	case VtPointerType9:
		p = buf + (n/VtScoreSize)*VtScoreSize;
		ep = buf + (nn/VtScoreSize)*VtScoreSize;
		while(p < ep) {
			memmove(p, vtZeroScore, VtScoreSize);
			p += VtScoreSize;
		}
		memset(p, 0, buf+nn-p);
		break;
	}
	return 1;
}

int
vtZeroTruncate(int type, uint8_t *buf, int n)
{
	uint8_t *p;

	switch(type) {
	default:
		for(p = buf + n; p > buf; p--) {
			if(p[-1] != 0)
				break;
		}
		return p - buf;
	case VtRootType:
		if(n < VtRootSize)
			return n;
		return VtRootSize;
	case VtPointerType0:
	case VtPointerType1:
	case VtPointerType2:
	case VtPointerType3:
	case VtPointerType4:
	case VtPointerType5:
	case VtPointerType6:
	case VtPointerType7:
	case VtPointerType8:
	case VtPointerType9:
		/* ignore slop at end of block */
		p = buf + (n/VtScoreSize)*VtScoreSize;

		while(p > buf) {
			if(memcmp(p - VtScoreSize, vtZeroScore, VtScoreSize) != 0)
				break;
			p -= VtScoreSize;
		}
		return p - buf;
	}
}
