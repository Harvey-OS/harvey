#include <string.h>

void*
memset(void *ap, int c, size_t n)
{
	char *p;

	p = ap;
	while(n > 0) {
		*p++ = c;
		n--;
	}
	return ap;
}
