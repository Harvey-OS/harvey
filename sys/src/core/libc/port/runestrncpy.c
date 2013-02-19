#include <u.h>
#include <libc.h>

Rune*
runestrncpy(Rune *s1, Rune *s2, long n)
{
	int i;
	Rune *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	return os1;
}
