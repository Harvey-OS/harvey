#include	<alef.h>
#include	<bio.h>

int
Biobufhdr.putrune(Biobufhdr *bp, int c)
{
	int n;
	Rune rune;
	byte str[4];

	rune = c;
	if(rune < Runeself) {
		bp->putc(rune);
		return 1;
	}
	n = runetochar(str, &rune);
	if(n == 0)
		return Bbad;
	if(bp->write(str, n) != n)
		return Beof;
	return n;
}
