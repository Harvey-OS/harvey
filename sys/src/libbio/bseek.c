#include	<u.h>
#include	<libc.h>
#include	<bio.h>

long
Bseek(Biobufhdr *bp, long offset, int base)
{
	long n;

	switch(bp->state) {
	default:
		fprint(2, "Bseek: unknown state %d\n", bp->state);
		n = Beof;
		break;

	case Bracteof:
		bp->state = Bractive;

	case Bractive:
		if(base == 1) {
			offset += Boffset(bp);
			base = 0;
		}
		n = seek(bp->fid, offset, base);
		bp->icount = 0;
		break;

	case Bwactive:
		Bflush(bp);
		n = seek(bp->fid, offset, base);
		break;
	}
	bp->offset = n;
	return n;
}

long
Boffset(Biobufhdr *bp)
{
	long n;

	switch(bp->state) {
	default:
		fprint(2, "Boffset: unknown state %d\n", bp->state);
		n = Beof;
		break;

	case Bracteof:
	case Bractive:
		n = bp->offset + bp->icount;
		break;

	case Bwactive:
		n = bp->offset + (bp->bsize + bp->ocount);
		break;
	}
	return n;
}
