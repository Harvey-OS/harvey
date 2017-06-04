#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"


enum {
	Qdir = 0,
	Qmax,
};

static Dirtab ufsdir[Qmax] = {
	{".",	{ Qdir, 0, QTDIR },	0,	0555},
};


static Chan*
ufsattach(char* spec)
{
	return devattach('U', spec);
}

Walkqid*
ufswalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, ufsdir, Qmax, devgen);
}

static int32_t
ufsstat(Chan* c, uint8_t* dp, int32_t n)
{
	return devstat(c, dp, n, ufsdir, Qmax, devgen);
}

static Chan*
ufsopen(Chan* c, int omode)
{
	return devopen(c, omode, ufsdir, Qmax, devgen);
}

static void
ufsclose(Chan*unused)
{
}

static int32_t
ufsread(Chan *c, void *a, int32_t n, int64_t offset)
{
	switch ((uint32_t)c->qid.path) {

	case Qdir:
		return devdirread(c, a, n, ufsdir, Qmax, devgen);

	default:
		error(Eperm);
		break;
	}

	return n;
}

static int32_t
ufswrite(Chan *c, void *a, int32_t n, int64_t offset)
{
	switch ((uint32_t)c->qid.path) {

	default:
		error(Eperm);
		break;
	}
	return 0;
}

Dev ufsdevtab = {
	.dc = 'U',
	.name = "ufs",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = ufsattach,
	.walk = ufswalk,
	.stat = ufsstat,
	.open = ufsopen,
	.create = devcreate,
	.close = ufsclose,
	.read = ufsread,
	.bread = devbread,
	.write = ufswrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
