#include <u.h>
#include <libc.h>

strlen(char *s)
{

	return strchr(s, 0) - s;
}
