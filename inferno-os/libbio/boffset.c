#include	"lib9.h"
#include	<bio.h>

long
Boffset(Biobuf *bp)
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
