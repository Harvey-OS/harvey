#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

enum{
	Qdir,
	Qbin,
	Qdev,
	Qenv,
	Qproc,
	Qnet,

	Qboot,
	Qcfs,
	Qfs,
};

extern long	cfslen;
extern ulong	cfscode[];
extern long	fslen;
extern ulong	fscode[];
extern ulong	bootlen;
extern uchar	bootcode[];

Dirtab rootdir[]={
	"bin",		{Qbin|CHDIR},	0,			0777,
	"boot",		{Qboot},	0,			0777,
	"dev",		{Qdev|CHDIR},	0,			0777,
	"env",		{Qenv|CHDIR},	0,			0777,
	"proc",		{Qproc|CHDIR},	0,			0777,
	"net",		{Qnet|CHDIR},	0,			0777,
};
#define	NROOT	(sizeof rootdir/sizeof(Dirtab))
Dirtab rootpdir[]={
	"cfs",		{Qcfs},		0,			0777,
	"fs",		{Qfs},		0,			0777,
};
Dirtab *rootmap[sizeof rootpdir/sizeof(Dirtab)];
int	nroot;

int
rootgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	USED(ntab);

	if(i >= nroot)
		return -1;

	if(i < NROOT)
		tab = &rootdir[i];
	else
		tab = rootmap[i - NROOT];

	devdir(c, tab->qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

void
rootreset(void)
{
	int i;

	i = 0;
	if(cfslen)
		rootmap[i++] = &rootpdir[0];
	if(fslen)
		rootmap[i++] = &rootpdir[1];
	nroot = NROOT + i;
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
	return devwalk(c, name, rootdir, nroot, rootgen);
}

void	 
rootstat(Chan *c, char *dp)
{
	devstat(c, dp, rootdir, nroot, rootgen);
}

Chan*
rootopen(Chan *c, int omode)
{
	return devopen(c, omode, rootdir, nroot, rootgen);
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

	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, rootdir, nroot, rootgen);

	case Qboot:		/* boot */
		if(offset >= bootlen)
			return 0;
		if(offset+n > bootlen)
			n = bootlen - offset;
		memmove(buf, bootcode+offset, n);
		return n;

	case Qcfs:		/* cfs */
		if(offset >= cfslen)
			return 0;
		if(offset+n > cfslen)
			n = cfslen - offset;
		memmove(buf, ((char*)cfscode)+offset, n);
		return n;

	case Qfs:		/* fs */
		if(offset >= fslen)
			return 0;
		if(offset+n > fslen)
			n = fslen - offset;
		memmove(buf, ((char*)fscode)+offset, n);
		return n;

	case Qdev:
		return 0;
	}
	return 0;
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
