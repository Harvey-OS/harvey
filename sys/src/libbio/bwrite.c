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

long
Bwrite(Biobufhdr *bp, void *ap, long count)
{
	long c;
	uchar *p;
	int i, n, oc;
	char errbuf[ERRMAX];

	p = ap;
	c = count;
	oc = bp->ocount;

	while(c > 0) {
		n = -oc;
		if(n > c)
			n = c;
		if(n == 0) {
			if(bp->state != Bwactive)
				return Beof;
			i = write(bp->fid, bp->bbuf, bp->bsize);
			if(i != bp->bsize) {
				errstr(errbuf, sizeof errbuf);
				if(strstr(errbuf, "interrupt") == nil)
					bp->state = Binactive;
				errstr(errbuf, sizeof errbuf);
				return Beof;
			}
			bp->offset += i;
			oc = -bp->bsize;
			continue;
		}
		memmove(bp->ebuf+oc, p, n);
		oc += n;
		c -= n;
		p += n;
	}
	bp->ocount = oc;
	return count-c;
}
