#include <u.h>
#include <libc.h>
#include <ctype.h>

char*
strupr(char *s)
{
	char *p;

	for(p = s; *p; p++)
		if(*p >= 0 && islower(*p))
			*p = toupper(*p);
	return s;
}

char*
strlwr(char *s)
{
	char *p;

	for(p = s; *p; p++)
		if(*p >= 0 && isupper(*p))
			*p = tolower(*p);
	return s;
}
