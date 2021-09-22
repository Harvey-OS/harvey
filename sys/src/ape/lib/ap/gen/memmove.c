#include <string.h>
#include <inttypes.h>

void*
memmove(void *a1, const void *a2, size_t n)
{
	char *s1, *s2;
	extern void abort(void);

	if((intptr_t)n < 0)
		abort();
	s1 = (char *)a1;
	s2 = (char *)a2;
	if(a1 > a2)
		goto back;
	while(n > 0) {
		*s1++ = *s2++;
		n--;
	}
	return a1;

back:
	s1 += n;
	s2 += n;
	while(n > 0) {
		*--s1 = *--s2;
		n--;
	}
	return a1;
}

void*
memcpy(void *a1, const void *a2, size_t n)
{
	return memmove(a1, a2, n);
}
