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
	Qtsleep,
	Qlock,
	Qwalk,
	Qmax,
};

static Dirtab regressdir[Qmax] = {
	{".", {Qdir, 0, QTDIR}, 0, 0555},
	{"regressctl", {Qctl, 0}, 0, 0666},
	{"malloc", {Qmalloc, 0}, 0, 0666},
	{"tsleep", {Qtsleep, 0}, 0, 0666},
	{"qlock", {Qlock, 0}, 0, 0666},
	{"walk", {Qwalk, 0}, 0, 0666},
};

int verbose = 0;
static QLock testlock;

static Chan *
regressattach(char *spec)
{
	return devattach('R', spec);
}

Walkqid *
regresswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, regressdir, Qmax, devgen);
}

static i32
regressstat(Chan *c, u8 *dp, i32 n)
{
	return devstat(c, dp, n, regressdir, Qmax, devgen);
}

static Chan *
regressopen(Chan *c, int omode)
{
	return devopen(c, omode, regressdir, Qmax, devgen);
}

static void
regressclose(Chan *unused)
{
}

static i32
regressread(Chan *c, void *a, i32 n, i64 offset)
{
	switch((u32)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, regressdir, Qmax, devgen);

	case Qmalloc:
	case Qtsleep:
		break;

	case Qlock:
		qlock(&testlock);
		// sanity test code here.
		qunlock(&testlock);
		n = 1;
		break;

	default:
		error(Eperm);
		break;
	}

	return n;
}

static i32
regresswrite(Chan *c, void *a, i32 n, i64 offset)
{
	Proc *up = externup();
	char *p;
	unsigned long amt;
	u64 addr;

	switch((u32)c->qid.path){

	case Qmalloc:
		p = a;
		amt = strtoull(p, 0, 0);
		if(verbose)
			print("Malloc %d\n", amt);
		p = malloc(amt);
		if(verbose)
			print("Got %p\n", p);
		free(p);
		if(verbose)
			print("Freed %p\n", p);
		return n;

	case Qtsleep:
		p = a;
		amt = strtoull(p, 0, 0);
		if(verbose)
			print("tsleep %d\n", amt);
		tsleep(&up->sleep, return0, 0, amt);
		if(verbose)
			print("done tsleep\n");
		return n;

	case Qctl:
		p = a;
		if(*p == 'v'){
			if(verbose)
				verbose--;
		} else if(*p == 'V')
			verbose++;
		else
			error("Only v or V");
		return n;

	case Qwalk:
		p = a;
		addr = strtoull(p, 0, 0);
		dumpmmuwalk(KADDR(cr3get()), addr);
		return n;

	default:
		error(Eperm);
		break;
	}
	return 0;
}

Dev regressdevtab = {
	.dc = L'R',
	.name = "regress",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = regressattach,
	.walk = regresswalk,
	.stat = regressstat,
	.open = regressopen,
	.create = devcreate,
	.close = regressclose,
	.read = regressread,
	.bread = devbread,
	.write = regresswrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};

static char *
devname(void)
{
        return regressdevtab.name;
}

static int
devdc(void)
{
        return regressdevtab.dc;
}

