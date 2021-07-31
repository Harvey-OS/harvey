#include <alef.h>

int
strlen(char *s)
{
	return strchr(s, 0) - s;
}
