#include <u.h>
#include <libc.h>

void
_assert(char *s)
{
	fprint(2, "assert failed: %s\n", s);
	abort();
}
