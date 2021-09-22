#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

int _noprint;

static Lock fmtl;

void
_fmtlock(void)
{
	if (!noprint)
		ilock(&fmtl);
}

void
_fmtunlock(void)
{
	if (!noprint)
		iunlock(&fmtl);
}

int
_efgfmt(Fmt*)
{
	return -1;
}
