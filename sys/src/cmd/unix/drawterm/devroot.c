#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

enum{
	Qdir=	0,
	Qbin,
	Qdev,
	Qenv,
	Qproc,
	Qnet,
	Qmnt
};

extern ulong	bootlen;
extern uchar	bootcode[];

static Dirtab rootdir[]={
	"bin",		{Qbin|CHDIR},	0,	0777,
	"dev",		{Qdev|CHDIR},	0,	0777,
	"env",		{Qenv|CHDIR},	0,	0777,
	"proc",		{Qproc|CHDIR},	0,	0777,
	"net",		{Qnet|CHDIR},	0,	0777,
	"mnt",		{Qmnt|CHDIR},	0,	0777,
};

#define Nrootdir	(sizeof(rootdir)/sizeof(rootdir[0]))

void
rootreset(void)
{
}

void
rootinit(void)
{
}

Chan*
rootattach(void *spec)
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
	return devwalk(c, name, rootdir, Nrootdir, devgen);
}

void	 
rootstat(Chan *c, char *dp)
{
	devstat(c, dp, rootdir, Nrootdir, devgen);
}

Chan*
rootopen(Chan *c, int omode)
{
	return devopen(c, omode, rootdir, Nrootdir, devgen);
}

void	 
rootcreate(Chan *c, char *name, int n, ulong offset)
{
	error(Eperm);
}

/*
 * ns_remove() knows this is a nop
 */
void	 
rootclose(Chan *c)
{
}


long	 
rootread(Chan *c, void *buf, long n, ulong offset)
{
	ulong t;

	t = c->qid.path & ~CHDIR;
	switch(t){
	case Qdir:
		return devdirread(c, buf, n, rootdir, Nrootdir, devgen);
	default:
		return 0;
	}
}

long	 
rootwrite(Chan *c, void *buf, long n, ulong offset)
{
	USED(c);
	USED(buf);
	USED(n);
	USED(offset);

	error(Eperm);
	return 0;
}

void	 
rootremove(Chan *c)
{
	USED(c);

	error(Eperm);
}

void	 
rootwstat(Chan *c, char *buf)
{
	USED(c);
	USED(buf);

	error(Eperm);
}
