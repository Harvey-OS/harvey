#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"


int
dupgen(Chan *c, Dirtab *tab, int ntab, int s, Dir *dp)
{
	char buf[8];
	Fgrp *fgrp = u->p->fgrp;
	Chan *f;
	static int perm[] = { 0400, 0200, 0600, 0 };

	USED(tab);
	USED(ntab);
	if(s >= NFD)
		return -1;
	if((f=fgrp->fd[s]) == 0)
		return 0;
	sprint(buf, "%ld", s);
	devdir(c, (Qid){s, 0}, buf, 0, eve, perm[f->mode&3], dp);
	return 1;
}

void
dupinit(void)
{
}

void
dupreset(void)
{
}

Chan *
dupattach(char *spec)
{
	return devattach('d', spec);
}

Chan *
dupclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
dupwalk(Chan *c, char *name)
{
	return devwalk(c, name, (Dirtab *)0, 0, dupgen);
}

void
dupstat(Chan *c, char *db)
{
	devstat(c, db, (Dirtab *)0, 0L, dupgen);
}

Chan *
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
	f = u->p->fgrp->fd[c->qid.path];
	close(c);
	incref(f);
	if(omode & OCEXEC)
		f->flag |= CCEXEC;
	return f;
}

void
dupcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
dupremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
dupwstat(Chan *c, char *dp)
{
	USED(c);
	USED(dp);
	error(Egreg);
}

void
dupclose(Chan *c)
{
	USED(c);
}

long
dupread(Chan *c, void *va, long n, ulong offset)
{
	char *a = va;

	USED(offset);
	if(c->qid.path != CHDIR)
		panic("dupread");
	return devdirread(c, a, n, (Dirtab *)0, 0L, dupgen);
}

long
dupwrite(Chan *c, void *va, long n, ulong offset)
{
	USED(c, va, n, offset);
	panic("dupwrite");
	return 0;		/* not reached */
}
