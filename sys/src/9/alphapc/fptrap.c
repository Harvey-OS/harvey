#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

char *fpcause[] =
{
	"invalid operation",
	"division by zero",
	"overflow",
	"underflow",
	"inexact operation",
	"integer overflow",
};
char	*fpexcname(Ureg*, ulong, char*);

void
fptrap(Ureg *ur)
{
	char buf[ERRMAX];
	int i;
	ulong reason;

	ur->pc &= ~2;
	reason = (ulong)ur->a0;
	for (i = 1; i < 6; i++)
		if (reason & (1<<i)) {
			sprint(buf, "fp: %s", fpcause[i-1]);
			goto found;
		}
	sprint(buf, "fp: code 0x%lux", reason);

found:
	fataltrap(ur, buf);
}

char*
fpexcname(Ureg *ur, ulong fcr31, char *buf)
{
	USED(ur, fcr31, buf);
	return buf;
}
