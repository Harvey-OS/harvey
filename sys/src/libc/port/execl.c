#include <u.h>
#include <libc.h>

execl(char *f, ...)
{

	return exec(f, &f+1);
}
