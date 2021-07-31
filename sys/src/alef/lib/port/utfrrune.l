#include <alef.h>

byte*
utfrrune(byte *s, int c)
{
	int c1;
	Rune r;
	byte *s1;

	if(c < Runesync)		/* not part of utf sequence */
		return strrchr(s, c);

	s1 = nil;
	for(;;) {
		c1 = *s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return s1;
			if(c1 == c)
				s1 = s;
			s++;
			continue;
		}
		c1 = chartorune(&r, s);
		if(r == c)
			s1 = s;
		s += c1;
	}
	return nil;
}
