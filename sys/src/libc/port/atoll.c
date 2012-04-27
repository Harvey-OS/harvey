#include <u.h>
#include <libc.h>

vlong
atoll(char *s)
{
	return strtoll(s, nil, 0);
}
