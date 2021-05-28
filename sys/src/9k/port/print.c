#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

static Lock fmtl;

void
_fmtlock(void)
{
	lock(&fmtl);
}

void
_fmtunlock(void)
{
	unlock(&fmtl);
}

int
_efgfmt(Fmt*)
{
	return -1;
}

int
mregfmt(Fmt* f)
{
	Mreg mreg;

	mreg = va_arg(f->args, Mreg);
	if(sizeof(Mreg) == sizeof(uvlong))
		return fmtprint(f, "%#16.16llux", (uvlong)mreg);
	return fmtprint(f, "%#8.8ux", (uint)mreg);
}

void
fmtinit(void)
{
	quotefmtinstall();
	archfmtinstall();
}
