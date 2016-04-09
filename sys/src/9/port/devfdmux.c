/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* fdmux is a way to mediate access to a read/write fd to a set of processes.
 * The owner of the mux reads and writes fd[0]. The children read and write fd[1].
 * Access to fd[1] is controlled by which process group a process is in.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

typedef struct Fdmux	Fdmux;
struct Fdmux
{
	QLock;
	Fdmux	*next;
	int	ref;
	uint32_t	path;
	Queue	*q[2];
	int	qref[2];
	int	owner; // pid of owner.
	int	pgrpid; // id of processes allowed to read/write fd[1]. If they do not match, they slep
	Rendez r;
};

struct
{
	Lock;
	uint32_t	path;
} fdmuxalloc;

enum
{
	Qdir,
	Qdata0,
	Qdata1,
	Qctl,
};

Dirtab fdmuxdir[] =
{
	".",		{Qdir,0,QTDIR},	0,		DMDIR|0500,
	"data",		{Qdata0},	0,		0600,
	"data1",	{Qdata1},	0,		0600,
	"ctl",		{Qctl},	0,		0600,
};
#define NFDMUXDIR 4

#define FDMUXTYPE(x)	(((unsigned)x)&0x1f)
#define FDMUXID(x)	((((unsigned)x))>>5)
#define FDMUXQID(i, t)	((((unsigned)i)<<5)|(t))


enum
{
	/* Plan 9 default for nmach > 1 */
	Fdmuxqsize = 256*1024
};

static int testready(void *v)
{
	Chan *c = v;
	Proc *up = externup();
	Fdmux *p;
	p = c->aux;
	if (up->pgrp->pgrpid == p->pgrpid)
		return 1;
	return 0;
}

static void
fdmuxinit(void)
{
}

/*
 *  create a fdmux, no streams are created until an open
 */
static Chan*
fdmuxattach(char *spec)
{
	Fdmux *p;
	Chan *c;
	Proc *up = externup();

	c = devattach('<', spec);
	p = malloc(sizeof(Fdmux));
	if(p == 0)
		exhausted("memory");
	p->ref = 1;
	p->pgrpid = up->pgrp->pgrpid;

	p->q[0] = qopen(Fdmuxqsize, 0, 0, 0);
	if(p->q[0] == 0){
		free(p);
		exhausted("memory");
	}
	p->q[1] = qopen(Fdmuxqsize, 0, 0, 0);
	if(p->q[1] == 0){
		free(p->q[0]);
		free(p);
		exhausted("memory");
	}

	lock(&fdmuxalloc);
	p->path = ++fdmuxalloc.path;
	unlock(&fdmuxalloc);

	mkqid(&c->qid, FDMUXQID(2*p->path, Qdir), 0, QTDIR);
	c->aux = p;
	c->devno = 0;
	return c;
}

static int
fdmuxgen(Chan *c, char* d, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid q;
	int len;
	Fdmux *p;

	if(i == DEVDOTDOT){
		devdir(c, c->qid, "#|", 0, eve, DMDIR|0555, dp);
		return 1;
	}
	i++;	/* skip . */
	if(tab==0 || i>=ntab)
		return -1;

	tab += i;
	p = c->aux;
	switch((uint32_t)tab->qid.path){
	case Qdata0:
		len = qlen(p->q[0]);
		break;
	case Qdata1:
		len = qlen(p->q[1]);
		break;
	default:
		len = tab->length;
		break;
	}
	mkqid(&q, FDMUXQID(FDMUXID(c->qid.path), tab->qid.path), 0, QTFILE);
	devdir(c, q, tab->name, len, eve, tab->perm, dp);
	return 1;
}


static Walkqid*
fdmuxwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;
	Fdmux *p;

	wq = devwalk(c, nc, name, nname, fdmuxdir, NFDMUXDIR, fdmuxgen);
	if(wq != nil && wq->clone != nil && wq->clone != c){
		p = c->aux;
		qlock(p);
		p->ref++;
		if(c->flag & COPEN){
			print("channel open in fdmuxwalk\n");
			switch(FDMUXTYPE(c->qid.path)){
			case Qdata0:
				p->qref[0]++;
				break;
			case Qdata1:
				p->qref[1]++;
				break;
			}
		}
		qunlock(p);
	}
	return wq;
}

static int32_t
fdmuxstat(Chan *c, uint8_t *db, int32_t n)
{
	Fdmux *p;
	Dir dir;

	p = c->aux;

	switch(FDMUXTYPE(c->qid.path)){
	case Qdir:
		devdir(c, c->qid, ".", 0, eve, DMDIR|0555, &dir);
		break;
	case Qdata0:
		devdir(c, c->qid, "data", qlen(p->q[0]), eve, 0600, &dir);
		break;
	case Qdata1:
		devdir(c, c->qid, "data1", qlen(p->q[1]), eve, 0600, &dir);
		break;
	default:
		panic("fdmuxstat");
	}
	n = convD2M(&dir, db, n);
	if(n < BIT16SZ)
		error(Eshortstat);
	return n;
}

/*
 *  if the stream doesn't exist, create it
 */
static Chan*
fdmuxopen(Chan *c, int omode)
{
	Fdmux *p;

	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	p = c->aux;
	qlock(p);
	switch(FDMUXTYPE(c->qid.path)){
	case Qdata0:
		p->qref[0]++;
		break;
	case Qdata1:
		p->qref[1]++;
		break;
	}
	qunlock(p);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	c->iounit = qiomaxatomic;
	return c;
}

static void
fdmuxclose(Chan *c)
{
	Fdmux *p;

	p = c->aux;
	qlock(p);

	if(c->flag & COPEN){
		/*
		 *  closing either side hangs up the stream
		 */
		switch(FDMUXTYPE(c->qid.path)){
		case Qdata0:
			p->qref[0]--;
			if(p->qref[0] == 0){
				qhangup(p->q[1], 0);
				qclose(p->q[0]);
			}
			break;
		case Qdata1:
			// TODO: what if we are the last member of this Pgrp? oopsie.
			// We *could* check the ref I guess.
			p->qref[1]--;
			if(p->qref[1] == 0){
				qhangup(p->q[0], 0);
				qclose(p->q[1]);
			}
			break;
		}
	}


	/*
	 *  if both sides are closed, they are reusable
	 */
	if(p->qref[0] == 0 && p->qref[1] == 0){
		qreopen(p->q[0]);
		qreopen(p->q[1]);
	}

	/*
	 *  free the structure on last close
	 */
	p->ref--;
	if(p->ref == 0){
		qunlock(p);
		free(p->q[0]);
		free(p->q[1]);
		free(p);
	} else
		qunlock(p);
}

static int32_t
fdmuxread(Chan *c, void *va, int32_t n, int64_t m)
{
	Proc *up = externup();
	Fdmux *p;
	char buf[8];

	p = c->aux;

	switch(FDMUXTYPE(c->qid.path)){
	case Qdir:
		return devdirread(c, va, n, fdmuxdir, NFDMUXDIR, fdmuxgen);
	case Qctl:
		snprint(buf, sizeof(buf), "%d", p->pgrpid);
		n = readstr(m, va, n, buf);
		return n;
	case Qdata0:
		return qread(p->q[0], va, n);
	case Qdata1:
		/* TODO: proper locking */
		if (up->pgrp->pgrpid != p->pgrpid)
			tsleep(&p->r, testready, c, 1000);
		return qread(p->q[1], va, n);
	default:
		panic("fdmuxread");
	}
	return -1;	/* not reached */
}

static Block*
fdmuxbread(Chan *c, int32_t n, int64_t offset)
{
	Proc *up = externup();
	Fdmux *p;
	int l;

	Block *b;
	p = c->aux;

	switch(FDMUXTYPE(c->qid.path)){
	case Qctl:
		b = iallocb(8);
		l = snprint((char *)b->wp, 8, "%d", p->pgrpid);
		b->wp += l;
		return b;

	case Qdata0:
		return qbread(p->q[0], n);
	case Qdata1:
		/* TODO: proper locking */
		if (up->pgrp->pgrpid != p->pgrpid)
			tsleep(&p->r, testready, c, 1000);
		return qbread(p->q[1], n);
	}

	return devbread(c, n, offset);
}

/*
 *  a write to a closed fdmux causes a note to be sent to
 *  the process.
 */
static int32_t
fdmuxwrite(Chan *c, void *va, int32_t n, int64_t mm)
{
	Proc *up = externup();
	Fdmux *p;
	char buf[8];
	int id;

	if(0)if(!islo())
		print("fdmuxwrite hi %#p\n", getcallerpc()); // devmnt?

	if(waserror()) {
		/* avoid notes when fdmux is a mounted queue */
		if((c->flag & CMSG) == 0)
			postnote(up, 1, "sys: write on closed fdmux", NUser);
		nexterror();
	}

	p = c->aux;

	switch(FDMUXTYPE(c->qid.path)){
	case Qctl:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, va, n);
		buf[n] = 0;
		id = strtoul(buf, 0, 0);
		// NO checking. How would we know?
		p->pgrpid = id;
		break;
	case Qdata0:
		n = qwrite(p->q[1], va, n);
		break;

	case Qdata1:
		/* TODO: proper locking */
		if (up->pgrp->pgrpid != p->pgrpid)
			tsleep(&p->r, testready, c, 1000);
		n = qwrite(p->q[0], va, n);
		break;

	default:
		panic("fdmuxwrite");
	}

	poperror();
	return n;
}

static int32_t
fdmuxbwrite(Chan *c, Block *bp, int64_t mm)
{
	Proc *up = externup();
	int32_t n;
	Fdmux *p;

	if(waserror()) {
		/* avoid notes when fdmux is a mounted queue */
		if((c->flag & CMSG) == 0)
			postnote(up, 1, "sys: write on closed fdmux", NUser);
		nexterror();
	}

	p = c->aux;
	switch(FDMUXTYPE(c->qid.path)){
	case Qdata0:
		n = qbwrite(p->q[1], bp);
		break;

	case Qdata1:
		/* TODO: proper locking */
		if (up->pgrp->pgrpid != p->pgrpid)
			tsleep(&p->r, testready, c, 1000);
		n = qbwrite(p->q[0], bp);
		break;

	default:
		n = 0;
		panic("fdmuxbwrite");
	}

	poperror();
	return n;
}

Dev fdmuxdevtab = {
	'<',
	"fdmux",

	devreset,
	fdmuxinit,
	devshutdown,
	fdmuxattach,
	fdmuxwalk,
	fdmuxstat,
	fdmuxopen,
	devcreate,
	fdmuxclose,
	fdmuxread,
	fdmuxbread,
	fdmuxwrite,
	fdmuxbwrite,
	devremove,
	devwstat,
};
