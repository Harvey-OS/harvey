#include	<string.h>

int
memcmp(const void *a1, const void *a2, size_t n)
{
	char *s1, *s2;
	unsigned c1, c2;

	s1 = a1;
	s2 = a2;
	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		n--;
	}
	return 0;
}
