#include <string.h>
#define	N	10000

static void*
memccpy(void *a1, void *a2, int c, unsigned long n)
{
	char *s1, *s2;

	s1 = a1;
	s2 = a2;
	while(n > 0) {
		if((*s1++ = *s2++) == c)
			return s1;
		n--;
	}
	return 0;
}

char*
strcpy(char *s1, const char *s2)
{
	char *os1;

	os1 = s1;
	while(!memccpy(s1, s2, 0, N)) {
		s1 += N;
		s2 += N;
	}
	return os1;
}
