#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.getrune(Biobufhdr *bp)
{
	int c, i;
	Rune rune;
	byte str[4];

	c = bp->getc();
	if(c < Runeself) {		/* one char */
		bp->runesize = 1;
		return c;
	}
	str[0] = c;

	for(i=1;;) {
		c = bp->getc();
		if(c < 0)
			return c;
		str[i++] = c;

		if(fullrune(str, i)) {
			chartorune(&rune, str);
			bp->runesize = i;
			return rune;
		}
	}
}

int
Biobufhdr.ungetrune(Biobufhdr *bp)
{
	bp->lock();
	if(bp->state != Bractive) {
		bp->unlock();
		return Beof;
	}
	bp->icount -= bp->runesize;
	bp->runesize = 0;
	bp->unlock();
	return 1;
}
