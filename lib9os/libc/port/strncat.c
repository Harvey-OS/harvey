#include <u.h>
#include <libc.h>

char*
strncat(char *s1, char *s2, long n)
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
