#include	<u.h>
#include	<libc.h>

void*
memchr(void *ap, int c, ulong n)
{
	uchar *sp;

	sp = ap;
	c &= 0xFF;
	while(n > 0) {
		if(*sp++ == c)
			return sp-1;
		n--;
	}
	return 0;
}
