#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.write(Biobufhdr *bp, void *ap, int count)
{
	char *p;
	int c, i, n, oc;

	bp->lock();
	rescue {
		bp->unlock();
		return Beof;
	}

	p = ap;
	c = count;
	oc = bp->ocount;

	while(c > 0) {
		n = -oc;
		if(n > c)
			n = c;
		if(n == 0) {
			if(bp->state != Bwactive)
				raise;
			i = write(bp->fid, bp->bbuf, bp->bsize);
			if(i != bp->bsize) {
				bp->state = Binactive;
				raise;
			}
			bp->off += i;
			oc = -bp->bsize;
			continue;
		}
		memmove(bp->ebuf+oc, p, n);
		oc += n;
		c -= n;
		p += n;
	}
	bp->ocount = oc;
	bp->unlock();
	return count-c;
}
