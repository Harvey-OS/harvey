/*
 * stubs for dumb mp-safe synchronous i8250 uart printing.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

int polledprint = 0;		/* flag: prn, prs, prf, etc. not available */

void
pr(uchar)
{
}

void
prn(char *, int)
{
}

void
prsnolock(char *)
{
}

void
prs(char *)
{
}

int
prf(char *, ...)
{
	return -1;
}
