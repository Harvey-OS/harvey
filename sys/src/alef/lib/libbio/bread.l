#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.read(Biobufhdr *bp, void *ap, int count)
{
	byte *p;
	int c, i, n, ic;

	bp->lock();
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
				if(i < 0)
					bp->state = Binactive;
				break;
			}
			bp->off += i;
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
	bp->unlock();
	return count-c;
}
