#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

enum{
	Qdir=	0,
	Qbin,
	Qdev,
	Qenv,
	Qproc,
	Qnet,
	Qboot,		/* readable files */

	Nfiles=13,	/* max root files */	
};

extern ulong	bootlen;
extern uchar	bootcode[];

Dirtab rootdir[Nfiles]={
		"bin",		{Qbin|CHDIR},	0,	0777,
		"dev",		{Qdev|CHDIR},	0,	0777,
		"env",		{Qenv|CHDIR},	0,	0777,
		"proc",		{Qproc|CHDIR},	0,	0777,
		"net",		{Qnet|CHDIR},	0,	0777,
};

static uchar	*rootdata[Nfiles];
static int	nroot = Qboot - 1;

/*
 *  add a root file
 */
void
addrootfile(char *name, uchar *contents, ulong len)
{
	Dirtab *d;
	

	if(nroot >= Nfiles)
		panic("too many root files");
	rootdata[nroot] = contents;
	d = &rootdir[nroot];
	strcpy(d->name, name);
	d->length = len;
	d->perm = 0555;
	d->qid.path = nroot+1;
	nroot++;
}

void
rootreset(void)
{
	addrootfile("boot", bootcode, bootlen);	/* always have a boot file */
}

void
rootinit(void)
{
}

Chan*
rootattach(char *spec)
{
	return devattach('/', spec);
}

Chan*
rootclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
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

void	 
rootstat(Chan *c, char *dp)
{
	devstat(c, dp, rootdir, nroot, devgen);
}

Chan*
rootopen(Chan *c, int omode)
{
	return devopen(c, omode, rootdir, nroot, devgen);
}

void	 
rootcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

/*
 * sysremove() knows this is a nop
 */
void	 
rootclose(Chan *c)
{
	USED(c);
}

long	 
rootread(Chan *c, void *buf, long n, ulong offset)
{
	ulong t;
	Dirtab *d;
	uchar *data;

	t = c->qid.path & ~CHDIR;
	if(t == Qdir)
		return devdirread(c, buf, n, rootdir, nroot, devgen);
	if(t < Qboot)
		return 0;

	d = &rootdir[t-1];
	data = rootdata[t-1];
	if(offset >= d->length)
		return 0;
	if(offset+n > d->length)
		n = d->length - offset;
	memmove(buf, data+offset, n);
	return n;
}

long	 
rootwrite(Chan *c, void *buf, long n, ulong offset)
{
	USED(c, buf, n, offset);
	error(Egreg);
	return 0;	/* not reached */
}

void	 
rootremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
rootwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
