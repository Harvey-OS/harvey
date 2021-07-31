#include <string.h>

char*
strncat(char *s1, const char *s2, size_t n)
{
	char *os1;

	os1 = s1;
	while(*s1++)
		;
	s1--;
	while(*s1++ = *s2++)
		if(--n < 0) {
			s1[-1] = 0;
			break;
		}
	return os1;
}
