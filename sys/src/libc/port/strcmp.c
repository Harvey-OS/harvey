/*
 * strcmp - string compare.  must treat bytes as unsigned so utf sorts right.
 */
#include <u.h>
#include <libc.h>

int
strcmp(char *s1, char *s2)
{
	uchar c1, c2;

	/* skip any identical prefixes */
	while ((c1 = *s1++) == (c2 = *s2++))
		if(c1 == 0)			/* end of both? */
			return 0;

	/* compare first differing chars */
	return c1 > c2? 1: -1;
}
