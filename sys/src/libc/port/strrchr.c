#include <u.h>
#include <libc.h>

char*
strrchr(char *s, int c)
{
	char *r;

	if(c == 0)
		return strchr(s, 0);
	r = 0;
	while(s = strchr(s, c))
		r = s++;
	return r;
}
