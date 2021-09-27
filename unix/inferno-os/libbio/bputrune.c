#include	"lib9.h"
#include	<bio.h>

int
Bputrune(Biobuf *bp, long c)
{
	Rune rune;
	char str[4];
	int n;

	rune = c;
	if(rune < Runeself) {
		Bputc(bp, rune);
		return 1;
	}
	n = runetochar(str, &rune);
	if(n == 0)
		return Bbad;
	if(Bwrite(bp, str, n) != n)
		return Beof;
	return n;
}
