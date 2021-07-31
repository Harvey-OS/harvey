#include	<string.h>

void*
memchr(const void *ap, int c, size_t n)
{
	char *sp;

	sp = ap;
	while(n > 0) {
		if(*sp++ == c)
			return sp-1;
		n--;
	}
	return 0;
}
