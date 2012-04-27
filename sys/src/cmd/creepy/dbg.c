#include <u.h>
#include <libc.h>
#include <thread.h>

char dbg[256];
static char sdbg[256];
static Ref nodbg;

void
nodebug(void)
{
	incref(&nodbg);
	if(nodbg.ref == 1)
		memmove(sdbg, dbg, sizeof dbg);
	memset(dbg, 0, sizeof dbg);
}

void
debug(void)
{
	if(decref(&nodbg) == 0)
		memmove(dbg, sdbg, sizeof dbg);
}

int
setdebug(void)
{
	int r;

	r = nodbg.ref;
	if(r > 0)
		memmove(dbg, sdbg, sizeof dbg);
	return r;
}

void
rlsedebug(int r)
{
	nodbg.ref = r;
	if(r > 0)
		memset(dbg, 0, sizeof dbg);
}

