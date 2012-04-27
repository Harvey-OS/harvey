#include <u.h>
#include <libc.h>

static Lock fmtl;

void
__fmtlock(void)
{
	lock(&fmtl);
}

void
__fmtunlock(void)
{
	unlock(&fmtl);
}
