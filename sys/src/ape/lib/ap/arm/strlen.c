#include <string.h>

size_t
strlen(const char *s)
{

	return strchr(s, 0) - s;
}
