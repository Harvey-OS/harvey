#include <alef.h>

char*
strrchr(char *s, char c)
{
	char *r;

	if(c == 0)
		return strchr(s, 0);
	r = nil;
	while(s = strchr(s, c))
		r = s++;
	return r;
}
