#include <u.h>
#include <libc.h>

char*
strncpy(char *s1, char *s2, long n)
{
	int i;
	char *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	return os1;
}
