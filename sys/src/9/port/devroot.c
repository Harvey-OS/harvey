#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum{
	Qdir=	0,

	Nfiles=32,	/* max root files */
};

extern ulong	bootlen;
extern uchar	bootcode[];

Dirtab rootdir[Nfiles] = {
	".",	{Qdir, 0, QTDIR},	0,		DMDIR|0555,
};

static uchar	*rootdata[Nfiles];
static int	nroot = 1;

/*
 *  add a root file
 */
static void
addroot(char *name, uchar *contents, ulong len, int perm)
{
	Dirtab *d;

	if(nroot >= Nfiles)
		panic("too many root files");
	rootdata[nroot] = contents;
	d = &rootdir[nroot];
	strcpy(d->name, name);
	d->length = len;
	d->perm = perm;
	d->qid.type = 0;
	d->qid.vers = 0;
	d->qid.path = nroot+1;
	if(perm & DMDIR)
		d->qid.type |= QTDIR;
	nroot++;
}

/*
 *  add a root file
 */
void
addrootfile(char *name, uchar *contents, ulong len)
{
	addroot(name, contents, len, 0555);
}

/*
 *  add a root file
 */
static void
addrootdir(char *name)
{
	addroot(name, nil, 0, DMDIR|0555);
}

static void
rootreset(void)
{
	addrootdir("bin");
	addrootdir("dev");
	addrootdir("env");
	addrootdir("fd");
	addrootdir("mnt");
	addrootdir("net");
	addrootdir("net.alt");
	addrootdir("proc");
	addrootdir("root");
	addrootdir("srv");

	addrootfile("boot", bootcode, bootlen);	/* always have a boot file */
}

static Chan*
rootattach(char *spec)
{
	return devattach('/', spec);
}

static Walkqid*
rootwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c,  nc, name, nname, rootdir, nroot, devgen);
}

static int
rootstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, rootdir, nroot, devgen);
}

static Chan*
rootopen(Chan *c, int omode)
{
	switch((ulong)c->qid.path) {
	default:
		break;
	}

	return devopen(c, omode, rootdir, nroot, devgen);
}

/*
 * sysremove() knows this is a nop
 */
static void
rootclose(Chan*)
{
}

static long
rootread(Chan *c, void *buf, long n, vlong off)
{
	ulong t;
	Dirtab *d;
	uchar *data;
	ulong offset = off;

	t = c->qid.path;
	switch(t){
	case Qdir:
		return devdirread(c, buf, n, rootdir, nroot, devgen);
	}

	d = &rootdir[t-1];
	data = rootdata[t-1];
	if(offset >= d->length)
		return 0;
	if(offset+n > d->length)
		n = d->length - offset;
	memmove(buf, data+offset, n);
	return n;
}

static long
rootwrite(Chan *c, void*, long, vlong)
{
	switch((ulong)c->qid.path){
	default:
		error(Egreg);
	}
	return 0;
}

Dev rootdevtab = {
	'/',
	"root",

	rootreset,
	devinit,
	devshutdown,
	rootattach,
	rootwalk,
	rootstat,
	rootopen,
	devcreate,
	rootclose,
	rootread,
	devbread,
	rootwrite,
	devbwrite,
	devremove,
	devwstat,
};
