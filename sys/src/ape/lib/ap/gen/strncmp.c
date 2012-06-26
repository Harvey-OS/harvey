#include <string.h>

int
strncmp(const char *s1, const char *s2, size_t n)
{
	unsigned c1, c2;
	long nn;

	nn = n;
	while(nn > 0) {
		c1 = *s1++;
		c2 = *s2++;
		nn--;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}
