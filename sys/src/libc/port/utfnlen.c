#include <u.h>
#include <libc.h>

int
utfnlen(char *s, long m)
{
	int c;
	long n;
	Rune rune;
	char *es;

	es = s + m;
	for(n = 0; s < es; n++) {
		c = *(uchar*)s;
		if(c < Runeself){		/* ascii? */
			if(c == '\0')
				break;
			s++;
		} else {
			if(!fullrune(s, es-s))
				break;
			s += chartorune(&rune, s);
		}
	}
	return n;
}
