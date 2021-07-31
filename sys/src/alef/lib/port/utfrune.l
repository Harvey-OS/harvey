#include <alef.h>

char*
utfrune(char *s, int c)
{
	int c1;
	Rune r;
	int n;

	if(c < Runesync)		/* not part of utf sequence */
		return strchr(s, c);

	for(;;) {
		c1 = *s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return nil;
			if(c1 == c)
				return s;
			s++;
			continue;
		}
		n = chartorune(&r, s);
		if(r == c)
			return s;
		s += n;
	}
	return nil;
}
