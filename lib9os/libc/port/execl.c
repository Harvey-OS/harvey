#include <u.h>
#include <libc.h>

int
execl(char *f, ...)
{

	return exec(f, &f+1);
}
