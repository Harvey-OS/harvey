#include <alef.h>

int
utflen(byte *s)
{
	Rune rune;
	int n, c;

	n = 0;
	for(;;) {
		c = *s;
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
