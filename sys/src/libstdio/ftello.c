/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- ftello
 */
#include "iolib.h"
long long
ftello(FILE *f)
{
	long long seekp = seek(f->fd, 0L, 1);
	if(seekp < 0)
		return -1; /* enter error state? */
	switch(f->state) {
	default:
		return seekp;
	case RD:
		return seekp - (f->wp - f->rp);
	case WR:
		return (f->flags & LINEBUF ? f->lp : f->wp) - f->buf + seekp;
	}
}
