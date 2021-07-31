#include <alef.h>

byte*
strncat(byte *s1, byte *s2, int n)
{
	byte *os1;

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
