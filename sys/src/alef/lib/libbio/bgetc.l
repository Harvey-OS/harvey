#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.getc(Biobufhdr *bp)
{
	int i;

	bp->lock();
	rescue {
		bp->unlock();
		return Beof;
	}

loop:
	i = bp->icount;
	if(i != 0) {
		bp->icount = i+1;
		bp->unlock();
		return bp->ebuf[i];
	}
	if(bp->state != Bractive)
		raise;
	/*
	 * get next buffer, try to keep Bungetsize
	 * characters pre-catenated from the previous
	 * buffer to allow that many ungets.
	 */
	memmove(bp->bbuf-Bungetsize, bp->ebuf-Bungetsize, Bungetsize);
	i = read(bp->fid, bp->bbuf, bp->bsize);
	if(i <= 0) {
		if(i < 0)
			bp->state = Binactive;
		raise;
	}
	if(i < bp->bsize)
		memmove(bp->ebuf-i-Bungetsize, bp->bbuf-Bungetsize, i+Bungetsize);
	bp->icount = -i;
	bp->off += i;
	goto loop;
}

int
Biobufhdr.ungetc(Biobufhdr *bp)
{
	bp->lock();
	if(bp->state != Bractive) {
		bp->unlock();
		return Beof;
	}
	bp->icount--;
	bp->unlock();
	return 1;
}
