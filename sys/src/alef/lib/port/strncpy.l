#include <alef.h>

byte*
strncpy(byte *s1, byte *s2, int n)
{
	int i;
	byte *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	return os1;
}
