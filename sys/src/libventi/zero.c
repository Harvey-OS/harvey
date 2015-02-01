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
#include <venti.h>

void
vtzeroextend(int type, uchar *buf, uint n, uint nn)
{
	uchar *p, *ep;

	switch(type&7) {
	case 0:
		memset(buf+n, 0, nn-n);
		break;
	default:
		p = buf + (n/VtScoreSize)*VtScoreSize;
		ep = buf + (nn/VtScoreSize)*VtScoreSize;
		while(p < ep) {
			memmove(p, vtzeroscore, VtScoreSize);
			p += VtScoreSize;
		}
		memset(p, 0, buf+nn-p);
		break;
	}
}

uint 
vtzerotruncate(int type, uchar *buf, uint n)
{
	uchar *p;

	if(type == VtRootType){
		if(n < VtRootSize)
			return n;
		return VtRootSize;
	}

	switch(type&7){
	case 0:
		for(p = buf + n; p > buf; p--) {
			if(p[-1] != 0)
				break;
		}
		return p - buf;
	default:
		/* ignore slop at end of block */
		p = buf + (n/VtScoreSize)*VtScoreSize;

		while(p > buf) {
			if(memcmp(p - VtScoreSize, vtzeroscore, VtScoreSize) != 0)
				break;
			p -= VtScoreSize;
		}
		return p - buf;
	}
}
