#include <alef.h>

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
byte*
strstr(byte *s1, byte *s2)
{
	byte *p;
	int f, n;

	f = s2[0];
	if(f == 0)
		return s1;
	n = strlen(s2);
	for(p=strchr(s1, f); p; p=strchr(p+1, f))
		if(strncmp(p, s2, n) == 0)
			return p;
	return nil;
}
