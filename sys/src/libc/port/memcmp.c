#include	<u.h>
#include	<libc.h>

int
memcmp(void *a1, void *a2, ulong n)
{
	uchar *s1, *s2;
	uint c1, c2;

	/* skip any identical prefixes */
	s1 = a1;
	s2 = a2;
	SET(c1, c2);
	for(; n > 0 && (c1 = *s1++) == (c2 = *s2++); n--)
		;
	if (n == 0)
		return 0;

	/* compare first differing chars */
	return c1 > c2? 1: -1;
}
