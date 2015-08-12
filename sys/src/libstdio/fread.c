/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- fread
 */
#include "iolib.h"

#define BIGN (BUFSIZ / 2)

int32_t
fread(void* p, int32_t recl, int32_t nrec, FILE* f)
{
	char* s;
	int n, d, c;

	s = (char*)p;
	n = recl * nrec;
	while(n > 0) {
		d = f->wp - f->rp;
		if(d > 0) {
			if(d > n)
				d = n;
			memmove(s, f->rp, d);
			f->rp += d;
		} else {
			if(n >= BIGN && f->state == RD &&
			   !(f->flags & STRING) && f->buf != f->unbuf) {
				d = read(f->fd, s, n);
				if(d <= 0) {
					f->state = (d == 0) ? END : ERR;
					goto ret;
				}
			} else {
				c = _IO_getc(f);
				if(c == EOF)
					goto ret;
				*s = c;
				d = 1;
			}
		}
		s += d;
		n -= d;
	}
ret:
	return (s - (char*)p) / (recl ? recl : 1);
}
