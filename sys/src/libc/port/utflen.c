#include <u.h>
#include <libc.h>

int
utflen(char *s)
{
	int c;
	long n;
	Rune rune;

	for(n = 0; ; n++) {
		c = *(uchar*)s;
		if(c < Runeself) {		/* ascii? */
			if(c == 0)
				return n;
			s++;
		} else
			s += chartorune(&rune, s);
	}
}
