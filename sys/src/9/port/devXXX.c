/*
 *  template for making a new device
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

enum{
	XXXdirqid,
	XXXdataqid,
};
Dirtab XXXtab[]={
	"data",		XXXdataqid,		0,	0600,
};
#define NXXXtab (sizeof(XXXTab)/sizeof(Dirtab))

void
XXXreset(void)
{
}

void
XXXinit(void)
{
}

Chan *
XXXattach(char *spec)
{
	return devattach('X', spec);
}

Chan *
XXXclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
XXXwalk(Chan *c, char *name)
{
	return devwalk(c, name, XXXtab, (long)NXXXtab, devgen);
}

void
XXXstat(Chan *c, char *db)
{
	devstat(c, db, XXXtab, (long)NXXXtab, devgen);
}

Chan *
XXXopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
XXXcreate(Chan *c, char *name, int omode, ulong perm)
{
	error(Eperm);
}

void
XXXremove(Chan *c)
{
	error(Eperm);
}

void
XXXwstat(Chan *c, char *dp)
{
	error(Eperm);
}

void
XXXclose(Chan *c)
{
}

long
XXXread(Chan *c, void *a, long n, ulong offset)
{
	switch((int)(c->qid.path&~CHDIR)){
	case XXXdirqid:
		return devdirread(c, a, n, XXXtab, NXXXtab, devgen);
	case XXXdataqid:
		break;
	default:
		n=0;
		break;
	}
	return n;
}

long
XXXwrite(Chan *c, char *a, long n, ulong offset)
{
	switch((int)(c->qid.path&~CHDIR)){
	case XXXdataqid:
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}
