#include <string.h>

/* Return pointer to first occurrence of s2 in s1, NULL if none */

char
*strstr(const char *s1, const char *s2)
{
	char *p;
	int f, n;

	f = s2[0];
	if(f == 0)
		return (char *)s1;
	n = strlen(s2);
	for(p=strchr(s1, f); p; p=strchr(p+1, f))
		if(strncmp(p, s2, n) == 0)
			return p;
	return NULL;
}

