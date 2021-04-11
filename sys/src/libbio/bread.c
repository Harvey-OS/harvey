/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>

i32
Bread(Biobufhdr *bp, void *ap, i32 count)
{
	i32 c;
	u8 *p;
	int i, n, ic;

	p = ap;
	c = count;
	ic = bp->icount;

	while(c > 0) {
		n = -ic;
		if(n > c)
			n = c;
		if(n == 0) {
			if(bp->state != Bractive)
				break;
			i = read(bp->fid, bp->bbuf, bp->bsize);
			if(i <= 0) {
				bp->state = Bracteof;
				if(i < 0)
					bp->state = Binactive;
				break;
			}
			bp->gbuf = bp->bbuf;
			bp->offset += i;
			if(i < bp->bsize) {
				memmove(bp->ebuf-i, bp->bbuf, i);
				bp->gbuf = bp->ebuf-i;
			}
			ic = -i;
			continue;
		}
		memmove(p, bp->ebuf+ic, n);
		c -= n;
		ic += n;
		p += n;
	}
	bp->icount = ic;
	if(count == c && bp->state == Binactive)
		return -1;
	return count-c;
}
