#include <u.h>
#include <libc.h>

long
strlen(char *s)
{

	return strchr(s, 0) - s;
}
