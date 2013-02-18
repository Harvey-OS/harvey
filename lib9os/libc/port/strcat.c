#include <u.h>
#include <libc.h>

char*
strcat(char *s1, char *s2)
{

	strcpy(strchr(s1, 0), s2);
	return s1;
}
