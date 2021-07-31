#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.seek(Biobufhdr *bp, int offset, int base)
{
	int n;

	switch(bp->state) {
	default:
		fprint(2, "Bseek: unknown state %d\n", bp->state);
		n = Beof;
		break;

	case Bractive:
		if(base == 1) {
			offset += bp->offset();
			base = 0;
		}
		n = seek(bp->fid, offset, base);
		bp->icount = 0;
		break;

	case Bwactive:
		bp->flush();
		n = seek(bp->fid, offset, base);
		break;
	}
	bp->off = n;
	return n;
}

int
Biobufhdr.offset(Biobufhdr *bp)
{
	int n;

	bp->lock();
	switch(bp->state) {
	default:
		fprint(2, "Boffset: unknown state %d\n", bp->state);
		n = Beof;
		break;

	case Bractive:
		n = bp->off + bp->icount;
		break;

	case Bwactive:
		n = bp->off + (bp->bsize + bp->ocount);
		break;
	}
	bp->unlock();
	return n;
}
