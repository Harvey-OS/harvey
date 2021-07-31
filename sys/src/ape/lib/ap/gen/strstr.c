#include <string.h>

/* Return pointer to first occurrence of s2 in s1, NULL if none */

char
*strstr(const char *s1, const char *s2)
{
	char *p, *pa, *pb;
	int c0, c;

	c0 = *s2;
	if(c0 == 0)
		return s1;
	s2++;
	for(p=strchr(s1, c0); p; p=strchr(p+1, c0)) {
		pa = p;
		for(pb=s2;; pb++) {
			c = *pb;
			if(c == 0)
				return p;
			if(c != *++pa)
				break;
		}
	}
	return 0;
}

