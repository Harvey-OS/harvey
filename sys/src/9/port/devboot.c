#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

enum{
	Qdir,
	Qboot,
	Qmem,
};

Dirtab bootdir[]={
	"boot",		{Qboot},	0,	0666,
	"mem",		{Qmem},		0,	0666,
};

#define	NBOOT	(sizeof bootdir/sizeof(Dirtab))

void
bootreset(void)
{
}

void
bootinit(void)
{
}

Chan*
bootattach(char *spec)
{
	return devattach('B', spec);
}

Chan*
bootclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
bootwalk(Chan *c, char *name)
{
	return devwalk(c, name, bootdir, NBOOT, devgen);
}

void	 
bootstat(Chan *c, char *dp)
{
	devstat(c, dp, bootdir, NBOOT, devgen);
}

Chan*
bootopen(Chan *c, int omode)
{
	return devopen(c, omode, bootdir, NBOOT, devgen);
}

void	 
bootcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
bootclose(Chan *c)
{
	USED(c);
}

long	 
bootread(Chan *c, void *buf, long n, ulong offset)
{
	switch(c->qid.path & ~CHDIR){

	case Qdir:
		return devdirread(c, buf, n, bootdir, NBOOT, devgen);

	case Qmem:
		/* kernel memory */
		if(offset>=KZERO && offset<KZERO+conf.npage*BY2PG){
			if(offset+n > KZERO+conf.npage*BY2PG)
				n = KZERO+conf.npage*BY2PG - offset;
			memmove(buf, (char*)offset, n);
			return n;
		}
		error(Ebadarg);
	}

	error(Egreg);
	return 0;	/* not reached */
}

long	 
bootwrite(Chan *c, void *buf, long n, ulong offset)
{
	ulong pc;

	switch(c->qid.path & ~CHDIR){
	case Qmem:
		/* kernel memory */
		if(offset>=KZERO && offset<KZERO+conf.npage*BY2PG){
			if(offset+n > KZERO+conf.npage*BY2PG)
				n = KZERO+conf.npage*BY2PG - offset;
			memmove((char*)offset, buf, n);
			return n;
		}
		error(Ebadarg);

	case Qboot:
		pc = *(ulong*)buf;
		splhi();
		gotopc(pc);
	}
	error(Ebadarg);
	return 0;	/* not reached */
}

void	 
bootremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
bootwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
