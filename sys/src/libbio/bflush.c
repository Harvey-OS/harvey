#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int
Bflush(Biobufhdr *bp)
{
	int n, c;

	switch(bp->state) {
	case Bwactive:
		n = bp->bsize+bp->ocount;
		if(n == 0)
			return 0;
		c = write(bp->fid, bp->bbuf, n);
		if(n == c) {
			bp->offset += n;
			bp->ocount = -bp->bsize;
			return 0;
		}
		bp->state = Binactive;
		bp->ocount = 0;
		break;

	case Bracteof:
		bp->state = Bractive;

	case Bractive:
		bp->icount = 0;
		bp->gbuf = bp->ebuf;
		return 0;
	}
	return Beof;
}
