#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.flush(Biobufhdr *bp)
{
	int n, c;

	switch(bp->state) {
	case Bwactive:
		bp->lock();
		n = bp->bsize+bp->ocount;
		if(n == 0) {
			bp->unlock();
			return 0;
		}
		c = write(bp->fid, bp->bbuf, n);
		if(n == c) {
			bp->off += n;
			bp->ocount = -bp->bsize;
			bp->unlock();
			return 0;
		}
		bp->state = Binactive;
		bp->ocount = 0;
		bp->unlock();
		break;
	}
	return Beof;
}

int
Biobufhdr.iflush(Biobufhdr *bp)
{
	int n, c;

	switch(bp->state) {
	case Bwactive:
		n = bp->bsize+bp->ocount;
		if(n == 0)
			return 0;
		c = write(bp->fid, bp->bbuf, n);
		if(n == c) {
			bp->off += n;
			bp->ocount = -bp->bsize;
			return 0;
		}
		bp->state = Binactive;
		bp->ocount = 0;
		break;
	}
	return Beof;
}
