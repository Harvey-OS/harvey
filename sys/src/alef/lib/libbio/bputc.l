#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.putc(Biobufhdr *bp, int c)
{
	int i, j;

	bp->lock();
	rescue {
		bp->unlock();
		return Beof;
	};

loop:
	i = bp->ocount;
	j = i+1;
	if(i != 0) {
		bp->ocount = j;
		bp->ebuf[i] = c;
		bp->unlock();
		return 0;
	}
	if(bp->state != Bwactive)
		raise;
	j = write(bp->fid, bp->bbuf, bp->bsize);
	if(j == bp->bsize) {
		bp->ocount = -bp->bsize;
		bp->off += j;
		goto loop;
	}
	fprint(2, "Bputc: write error\n");
	bp->state = Binactive;
	bp->ocount = 0;
	raise;
	return 0;
}
