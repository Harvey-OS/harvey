#include	<u.h>
#include	<libc.h>
#include	<bio.h>

long
Bread(Biobufhdr *bp, void *ap, long count)
{
	long c;
	uchar *p;
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
			bp->offset += i;
			if(i < bp->bsize)
				memmove(bp->ebuf-i, bp->bbuf, i);
			ic = -i;
			continue;
		}
		memmove(p, bp->ebuf+ic, n);
		c -= n;
		ic += n;
		p += n;
	}
	bp->icount = ic;
	return count-c;
}
