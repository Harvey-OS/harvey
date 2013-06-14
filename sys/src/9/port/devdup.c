#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Qid is (2*fd + (file is ctl))+1 */

static int
dupgen(Chan *c, char *, Dirtab*, int, int s, Dir *dp)
{
	Fgrp *fgrp = up->fgrp;
	Chan *f;
	static int perm[] = { 0400, 0200, 0600, 0 };
	int p;
	Qid q;

	if(s == DEVDOTDOT){
		devdir(c, c->qid, ".", 0, eve, DMDIR|0555, dp);
		return 1;
	}
	if(s == 0)
		return 0;
	s--;
	if(s/2 > fgrp->maxfd)
		return -1;
	if((f=fgrp->fd[s/2]) == nil)
		return 0;
	if(s & 1){
		p = 0400;
		snprint(up->genbuf, sizeof up->genbuf, "%dctl", s/2);
	}else{
		p = perm[f->mode&3];
		snprint(up->genbuf, sizeof up->genbuf, "%d", s/2);
	}
	mkqid(&q, s+1, 0, QTFILE);
	devdir(c, q, up->genbuf, 0, eve, p, dp);
	return 1;
}

static Chan*
dupattach(char *spec)
{
	return devattach('d', spec);
}

static Walkqid*
dupwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, (Dirtab *)0, 0, dupgen);
}

static int
dupstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, (Dirtab *)0, 0L, dupgen);
}

static Chan*
dupopen(Chan *c, int omode)
{
	Chan *f;
	int fd, twicefd;

	if(c->qid.type & QTDIR){
		if(omode != 0)
			error(Eisdir);
		c->mode = 0;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	if(c->qid.type & QTAUTH)
		error(Eperm);
	twicefd = c->qid.path - 1;
	fd = twicefd/2;
	if((twicefd & 1)){
		/* ctl file */
		f = c;
		f->mode = openmode(omode);
		f->flag |= COPEN;
		f->offset = 0;
	}else{
		/* fd file */
		f = fdtochan(fd, openmode(omode), 0, 1);
		cclose(c);
	}
	if(omode & OCEXEC)
		f->flag |= CCEXEC;
	return f;
}

static void
dupclose(Chan*)
{
}

static long
dupread(Chan *c, void *va, long n, vlong offset)
{
	char *a = va;
	char buf[256];
	int fd, twicefd;

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, (Dirtab *)0, 0L, dupgen);
	twicefd = c->qid.path - 1;
	fd = twicefd/2;
	if(twicefd & 1){
		c = fdtochan(fd, -1, 0, 1);
		procfdprint(c, fd, 0, buf, sizeof buf);
		cclose(c);
		return readstr((ulong)offset, va, n, buf);
	}
	panic("dupread");
	return 0;
}

static long
dupwrite(Chan*, void*, long, vlong)
{
	error(Eperm);
	return 0;		/* not reached */
}

Dev dupdevtab = {
	'd',
	"dup",

	devreset,
	devinit,
	devshutdown,
	dupattach,
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
