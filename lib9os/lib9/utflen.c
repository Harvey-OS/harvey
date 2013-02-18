#include "lib9.h"

int
utflen(char *s)
{
	int c;
	long n;
	Rune rune;

	n = 0;
	for(;;) {
		c = *(uchar*)s;
		if(c < Runeself) {
			if(c == 0)
				return n;
			s++;
		} else
			s += chartorune(&rune, s);
		n++;
	}
	return 0;
}
