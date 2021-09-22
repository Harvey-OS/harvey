/* fake EDF scheduling */
#include	<u.h>
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../port/edf.h"

void
edfrecord(Proc *)
{
}

void
edfstop(Proc *)
{
}

int
edfready(Proc *)
{
	return 0;
}

void
edfrun(Proc*, int)
{
}

Edf *
edflock(Proc*)
{
	return nil;
}

void
edfunlock(void)
{
}
