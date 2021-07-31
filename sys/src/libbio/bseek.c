#include	<u.h>
#include	<libc.h>
#include	<bio.h>

long
Bseek(Biobufhdr *bp, long offset, int base)
{
	long n, d;

	switch(bp->state) {
	default:
		fprint(2, "Bseek: unknown state %d\n", bp->state);
		return Beof;

	case Bracteof:
		bp->state = Bractive;
		bp->icount = 0;
		bp->gbuf = bp->ebuf;

	case Bractive:
		n = offset;
		if(base == 1) {
			n += Boffset(bp);
			base = 0;
		}

		/*
		 * try to seek within buffer
		 */
		if(base == 0) {
			d = n - Boffset(bp);
			bp->icount += d;
			if(d >= 0) {
				if(bp->icount <= 0)
					return n;
			} else {
				if(bp->ebuf - bp->gbuf >= -bp->icount)
					return n;
			}
		}

		/*
		 * reset the buffer
		 */
		n = seek(bp->fid, n, base);
		bp->icount = 0;
		bp->gbuf = bp->ebuf;
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
