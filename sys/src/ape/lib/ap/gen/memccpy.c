#include <string.h>

void*
memccpy(void *a1, void *a2, int c, size_t n)
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
