#include <string.h>

int
strcoll(const char *s1, const char *s2)
{
	/* BUG: supposed to pay attention to LC_COLLATE of current locale */
	return strcmp(s1, s2);
}
