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
