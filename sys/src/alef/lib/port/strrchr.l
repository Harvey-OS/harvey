#include <alef.h>

byte*
strrchr(byte *s, byte c)
{
	byte *r;

	if(c == 0)
		return strchr(s, 0);
	r = nil;
	while(s = strchr(s, c))
		r = s++;
	return r;
}
