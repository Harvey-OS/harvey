#include <u.h>
#include <libc.h>

int
fork(void)
{
	return rfork(RFPROC|RFFDG|RFREND);
}
