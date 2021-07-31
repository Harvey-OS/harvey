#include <alef.h>

byte*
strcat(byte *s1, byte *s2)
{

	strcpy(strchr(s1, 0), s2);
	return s1;
}
