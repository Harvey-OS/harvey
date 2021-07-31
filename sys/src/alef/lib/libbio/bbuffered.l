#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.buffered(Biobufhdr *bp)
{
	switch(bp->state) {
	case Bractive:
		return -bp->icount;

	case Bwactive:
		return bp->bsize + bp->ocount;
	}
	fprint(2, "Bbuffered: unknown state %d\n", bp->state);
	return 0;
}
