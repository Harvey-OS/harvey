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

Dirtab rootdir[Nfiles];

static uchar	*rootdata[Nfiles];
static int	nroot = 0;

typedef struct Recover Recover;
struct Recover
{
	int	len;
	char	*req;
	Recover	*next;
};

struct
{
	Lock;
	QLock;
	Rendez;
	Recover	*q;
}reclist;

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
	d->qid.path = nroot+1;
	if(perm & CHDIR)
		d->qid.path |= CHDIR;
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
	addroot(name, nil, 0, CHDIR|0555);
}

static void
rootreset(void)
{
	addrootdir("bin");
	addrootdir("dev");
	addrootdir("env");
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

static int
rootwalk(Chan *c, char *name)
{
	if(strcmp(name, "..") == 0) {
		c->qid.path = Qdir|CHDIR;
		return 1;
	}
	if((c->qid.path & ~CHDIR) != Qdir)
		return 0;
	return devwalk(c, name, rootdir, nroot, devgen);
}

static void
rootstat(Chan *c, char *dp)
{
	devstat(c, dp, rootdir, nroot, devgen);
}

static Chan*
rootopen(Chan *c, int omode)
{
	switch(c->qid.path & ~CHDIR) {
	default:
		break;
	}

	return devopen(c, omode, rootdir, nroot, devgen);
}

/*
 * sysremove() knows this is a nop
 */
static void
rootclose(Chan *c)
{
	switch(c->qid.path) {
	default:
		break;
	}
}

static int
rdrdy(void*)
{
	return reclist.q != 0;
}

static long
rootread(Chan *c, void *buf, long n, vlong off)
{
	ulong t;
	Dirtab *d;
	uchar *data;
	ulong offset = off;

	t = c->qid.path & ~CHDIR;
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
	switch(c->qid.path & ~CHDIR){
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
	rootattach,
	devclone,
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
