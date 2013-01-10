#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ureg.h"

enum {
	Qdir = 0,
	Qctl = 1,
	Qmalloc,
	Qmax,
};

static Dirtab regressdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
	"regressctl",	{ Qctl, 0 },	0,	0666,
	"malloc",	{ Qmalloc, 0 },	0,	0666,
};

int verbose = 0;

static Chan*
regressattach(char* spec)
{
	return devattach('Z', spec);
}

Walkqid*
regresswalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, regressdir, Qmax, devgen);
}

static long
regressstat(Chan* c, uchar* dp, long n)
{
	return devstat(c, dp, n, regressdir, Qmax, devgen);
}

static Chan*
regressopen(Chan* c, int omode)
{
	return devopen(c, omode, regressdir, Qmax, devgen);
}

static void
regressclose(Chan*)
{
}

static long
regressread(Chan *c, void *a, long n, vlong offset)
{
	char *buf, *p;

	switch((ulong)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, regressdir, Qmax, devgen);

	case Qmalloc:
		break;

	default:
		error(Eperm);
		break;
	}

	return n;
}

static long
regresswrite(Chan *c, void *a, long n, vlong offset)
{
	char *p;
	unsigned long amt;

	switch((ulong)c->qid.path){

	case Qmalloc:
		p = a;
		amt = strtoull(p, 0, 0);
		if (verbose)
			print("Malloc %d\n", amt);
		p = malloc(amt);
		if (verbose)
			print("Got %p\n", p);
		free(p);
		if (verbose)
			print("Freed %p\n", p);
		return n;

	case Qctl:
		p = a;
		if (*p == 'v'){
			if (verbose)
				verbose--;
		} else if (*p == 'V')
			verbose++;
		else
			error("Only v or V");
		return n;
		
	default:
		error(Eperm);
		break;
	}
	return 0;
}

Dev regressdevtab = {
	'Z',
	"regress",

	devreset,
	devinit,
	devshutdown,
	regressattach,
	regresswalk,
	regressstat,
	regressopen,
	devcreate,
	regressclose,
	regressread,
	devbread,
	regresswrite,
	devbwrite,
	devremove,
	devwstat,
};

