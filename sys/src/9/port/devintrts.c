/*
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

enum {
	Qdir,
	Qintrts,
};

static Dirtab intrtsdir[] = {
	"intrts",	{ Qintrts, 0 },		0,	0666,
};

void (*saveintrts)(void) = nil;

/* interrupt timestamps, trap() fills intrts each interrupt */
uvlong	intrts;
static	struct {
	Lock;
	int	vno;		/* vector to save timestamps for */
	int	n;		/* number of valid timestamps in ts[] */
	uvlong	ts[128];	/* time stamps */
} tsalloc;

/* called with interrupts off by interrupt routine */
void
_saveintrts(void)
{
	ilock(&tsalloc);
	if(tsalloc.n < nelem(tsalloc.ts))
		tsalloc.ts[tsalloc.n++] = m->intrts;
	iunlock(&tsalloc);
}

/* read interrupt timestamps */
static long
readintrts(void *buf, int n)
{
	n /= sizeof(uvlong);
	if(n <= 0)
		return 0;
	ilock(&tsalloc);
	if(n > tsalloc.n)
		n = tsalloc.n;
	memmove(buf, tsalloc.ts, n*sizeof(uvlong));
	tsalloc.n = 0;
	iunlock(&tsalloc);
	return n*sizeof(uvlong);
}


static void
intrtsreset(void)
{
	saveintrts = _saveintrts;
}

static Chan*
intrtsattach(char* spec)
{
	return devattach('Z', spec);	/* everything good was taken */
}

int
intrtswalk(Chan* c, char* name)
{
	return devwalk(c, name, intrtsdir, nelem(intrtsdir), devgen);
}

static void
intrtsstat(Chan* c, char* dp)
{
	devstat(c, dp, intrtsdir, nelem(intrtsdir), devgen);
}

static Chan*
intrtsopen(Chan* c, int omode)
{
	return devopen(c, omode, intrtsdir, nelem(intrtsdir), devgen);
}

static void
intrtsclose(Chan*)
{
}

static long
intrtsread(Chan* c, void* a, long n, vlong)
{
	switch(c->qid.path & ~CHDIR){

	case Qdir:
		return devdirread(c, a, n, intrtsdir, nelem(intrtsdir), devgen);

	case Qintrts:
		return readintrts(a, n);
	}

	error(Eperm);
	return 0;
}

static long
intrtswrite(Chan*, void*, long, vlong)
{
	error(Eperm);
	return 0;
}

Dev intrtsdevtab = {
	'Z',
	"intrts",

	intrtsreset,
	devinit,
	intrtsattach,
	devclone,
	intrtswalk,
	intrtsstat,
	intrtsopen,
	devcreate,
	intrtsclose,
	intrtsread,
	devbread,
	intrtswrite,
	devbwrite,
	devremove,
	devwstat,
};
