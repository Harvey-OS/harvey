#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

static int
dupgen(Chan *c, Dirtab*, int, int s, Dir *dp)
{
	char buf[8];
	Fgrp *fgrp = up->fgrp;
	Chan *f;
	static int perm[] = { 0400, 0200, 0600, 0 };

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#d", 0, eve, 0555, dp);
		return 1;
	}
	if(s > fgrp->maxfd)
		return -1;
	if((f=fgrp->fd[s]) == 0)
		return 0;
	sprint(buf, "%d", s);
	devdir(c, (Qid){s, 0}, buf, 0, eve, perm[f->mode&3], dp);
	return 1;
}

static Chan*
dupattach(char *spec)
{
	return devattach('d', spec);
}

static int
dupwalk(Chan *c, char *name)
{
	return devwalk(c, name, (Dirtab *)0, 0, dupgen);
}

static void
dupstat(Chan *c, char *db)
{
	devstat(c, db, (Dirtab *)0, 0L, dupgen);
}

static Chan*
dupopen(Chan *c, int omode)
{
	Chan *f;

	if(c->qid.path == CHDIR){
		if(omode != 0)
			error(Eisdir);
		c->mode = 0;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	fdtochan(c->qid.path, openmode(omode), 0, 0);	/* error check only */
	f = up->fgrp->fd[c->qid.path];
	cclose(c);
	incref(f);
	if(omode & OCEXEC)
		f->flag |= CCEXEC;
	return f;
}

static void
dupclose(Chan*)
{
}

static long
dupread(Chan *c, void *va, long n, vlong)
{
	char *a = va;

	if(c->qid.path != CHDIR)
		panic("dupread");
	return devdirread(c, a, n, (Dirtab *)0, 0L, dupgen);
}

static long
dupwrite(Chan*, void*, long, vlong)
{
	panic("dupwrite");
	return 0;		/* not reached */
}

Dev dupdevtab = {
	'd',
	"dup",

	devreset,
	devinit,
	dupattach,
	devclone,
	dupwalk,
	dupstat,
	dupopen,
	devcreate,
	dupclose,
	dupread,
	devbread,
	dupwrite,
	devbwrite,
	devremove,
	devwstat,
};
