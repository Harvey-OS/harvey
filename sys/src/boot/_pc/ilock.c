#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

void
ilock(Lock *lk)
{
	if(lk->locked != 0)
		panic("ilock");
	lk->spl = splhi();
	lk->locked = 1;
}

void
iunlock(Lock *lk)
{
	if(lk->locked != 1)
		panic("iunlock");
	lk->locked = 0;
	splx(lk->spl);
}
