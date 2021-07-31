#include <alef.h>

int
strlen(byte *s)
{
	return strchr(s, 0) - s;
}
