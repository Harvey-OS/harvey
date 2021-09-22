#include <u.h>
#include <libc.h>

char*
utfrune(char *s, long c)
{
	Rune r;
	int n;

	if(c < Runesync)		/* not part of utf sequence */
		return strchr(s, c);

	for(;;) {
		r = *(uchar*)s;
		if(r < Runeself) {	/* one-byte rune */
			if(r == '\0')
				return 0;
			n = 1;
		} else
			n = chartorune(&r, s);
		if(r == c)
			return s;
		s += n;
	}
}
