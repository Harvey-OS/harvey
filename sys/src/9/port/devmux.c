#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

typedef struct Mux Mux;
typedef struct Con Con;
typedef struct Dtq Dtq;

enum
{
	Qdir	= 0,
	Qhead	= 0,
	Qclone,
	Qoffset,

	Nmuxchan=	64,
	Nmux	=	32,
	Maxmsg	=	(32*1024),
	Flowctl	=	Maxmsg/2,
};

struct Dtq
{
	QLock	rd;
	Rendez	r;
	Lock	listlk;
	Block	*list;
	int	ndelim;
	int	nb;
	QLock	flow;
	Rendez	flowr;
};

struct Con
{
	int	ref;
	char	user[NAMELEN];
	ulong	perm;
	Dtq	conq;
};

struct Mux
{
	Ref;
	int	srv;
	char	name[NAMELEN];
	char	user[NAMELEN];
	ulong	perm;
	int	headopen;
	Dtq	headq;
	Con	connects[Nmux];
	Chan	*c;
};

Mux	*muxes;
ulong	muxreadq(Mux *m, Dtq*, char*, ulong);
void	muxwriteq(Dtq*, char*, long, int, int);
void	muxflow(Dtq*);
Block  *muxclq(Dtq *q);

#define NMUX(c)		(((c->qid.path>>8)&0xffff)-1)
#define NQID(m, c)	(Qid){(m+1)<<8|(c)&0xff, 0}
#define DQID(m)		(Qid){(m+1)<<8|CHDIR, 0}
#define NCON(c)		(c->qid.path&0xff)

int
muxgen(Chan *c, Dirtab *tab, int ntab, int s, Dir *dp)
{
	Mux *m;
	int mux;
	Con *cm;
	char buf[10];
	int nq;

	USED(tab, ntab);
	if(c->qid.path == CHDIR) {
		if(s >= Nmuxchan)
			return -1;

		m = &muxes[s];
		if(m->name[0] == '\0')
			return 0;
		if(m->srv)
			devdir(c, NQID(s, 0), m->name, 0, m->user, m->perm, dp);
		else
			devdir(c, DQID(s), m->name, 0, m->user, m->perm, dp);
		return 1;
	}

	if(s >= Nmux+2)
		return -1;

	mux = NMUX(c);
	m = &muxes[mux];

	switch(s) {
	case Qhead:
		devdir(c, NQID(mux, Qhead), "head", m->headq.nb, m->user, m->perm, dp);
		break;
	case Qclone:
		devdir(c, NQID(mux, Qclone), "clone", 0, m->user, m->perm, dp);
		break;
	default:
		nq = s-Qoffset;
		cm = &m->connects[nq];
		if(cm->ref == 0)
			return 0;
		sprint(buf, "%d", nq);
		devdir(c, NQID(mux, Qoffset+s), buf, cm->conq.nb, cm->user, cm->perm, dp);
		break;
	}
	return 1;
}

void
muxinit(void)
{
}

void
muxreset(void)
{
	muxes = xalloc(Nmuxchan*sizeof(Mux));
}

Chan *
muxattach(char *spec)
{
	Chan *c;

	c = devattach('s', spec);
	c->qid.path = CHDIR|Qdir;
	return c;
}

Chan *
muxclone(Chan *c, Chan *nc)
{
	int ncon;
	Mux *m;

	if(c->qid.path == CHDIR)
		return devclone(c, nc);;

	m = &muxes[NMUX(c)];
	ncon = NCON(c);
	c = devclone(c, nc);

	if((c->flag&COPEN) == 0)
		return c;

	switch(ncon) {
	case Qhead:
		incref(m);
		break;
	case Qclone:
		break;
	default:
		lock(m);
		m->connects[ncon-Qoffset].ref++;
		m->ref++;
		unlock(m);
	}
	return c;
}

int
muxwalk(Chan *c, char *name)
{
	if(strcmp(name, "..") == 0) {
		c->qid.path = CHDIR|Qdir;
		return 1;
	}

	return devwalk(c, name, 0, 0, muxgen);
}

void
muxstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, muxgen);
}

Chan *
muxopen(Chan *c, int omode)
{
	Mux *m;
	Con *cm, *e;
	int mux;
	Chan *new;

	c = devopen(c, omode, 0, 0, muxgen);
	if(c->qid.path & CHDIR)
		return c;

	mux = NMUX(c);
	m = &muxes[mux];
	lock(m);
	if(waserror()) {
		c->flag &= ~COPEN;
		unlock(m);
		nexterror();
	}
	if(m->srv) {
		if(m->c == 0)
			error(Eshutdown);
		new = m->c;
		incref(new);
		unlock(m);
		poperror();
		close(c);
		return new;
	}
	switch(NCON(c)) {
	case Qhead:
		if(m->headopen)
			error(Einuse);
		m->headopen = 1;
		m->ref++;
		break;
	case Qclone:
		if(m->headopen == 0)
			error(Emuxshutdown);

		cm = m->connects;
		for(e = &cm[Nmux]; cm < e; cm++)
			if(cm->ref == 0)
				break;
		if(cm == e)
			error(Emuxbusy);
		cm->ref = 1;
		m->ref++;
		strncpy(cm->user, u->p->user, NAMELEN);
		cm->perm = 0600;
		c->qid = NQID(mux, (cm-m->connects)+Qoffset);
		break;
	default:
		cm = &m->connects[NCON(c)-Qoffset];
		cm->ref++;
		m->ref++;
		break;
	}
	unlock(m);
	poperror();
	return c;
}

void
muxcreate(Chan *c, char *name, int omode, ulong perm)
{
	int n;
	Mux *m, *e;

	if(c->qid.path != CHDIR)
		error(Eperm);

	omode = openmode(omode);

	m = muxes;
	for(e = &m[Nmuxchan]; m < e; m++) {
		if(m->name[0] == '\0' && m->ref == 0 && canlock(m)) {
			if(m->ref != 0) {
				unlock(m);
				continue;
			}
			break;
		}	
	}

	if(m == e)
		exhausted("multiplexers");

	strncpy(m->name, name, NAMELEN);
	strncpy(m->user, u->p->user, NAMELEN);
	m->perm = perm&~CHDIR;
	m->srv = 1;
	if(perm&CHDIR)
		m->srv = 0;
	unlock(m);

	n = m - muxes;
	c->qid = (Qid){(CHDIR&perm)|(n+1)<<8, 0};
	c->flag |= COPEN;
	c->mode = omode;
}

void
muxclose(Chan *c)
{
	Block *f1, *f2;
	Con *cm, *e;
	Mux *m;
	int nc;

	if(c->qid.path&CHDIR)
		return;

	m = &muxes[NMUX(c)];
	if(!(c->flag&COPEN) || m->srv)
		return;

	nc = NCON(c);
	f1 = 0;
	f2 = 0;
	switch(nc) {
	case Qhead:
		m->headopen = 0;
		cm = m->connects;
		for(e = &cm[Nmux]; cm < e; cm++)
			if(cm->ref)
				wakeup(&cm->conq.r);
		lock(m);
		if(--m->ref == 0)
			f1 = muxclq(&m->headq);
		unlock(m);
		break;
	case Qclone:
		break;
	default:
		lock(m);
		cm = &m->connects[nc-Qoffset];
		if(--cm->ref == 0)
			f1 = muxclq(&cm->conq);
		if(--m->ref == 0)
			f1 = muxclq(&m->headq);
		unlock(m);
	}
	if(f1)
		freeb(f1);
	if(f2)
		freeb(f2);
}

void
muxremove(Chan *c)
{
	Mux *m;
	Chan *srv;

	if(c->qid.path == CHDIR) 
		error(Eperm);

	m = &muxes[NMUX(c)];
	if((c->qid.path&CHDIR) == 0 && m->srv == 0)
		error(Eperm);
		
	if(strncmp(u->p->user, m->user, NAMELEN))
		error(Eperm);

	srv = 0;
	lock(m);
	if(m->srv) {
		srv = m->c;
		m->c = 0;
	}
	m->name[0] = '\0';
	unlock(m);
	if(srv)
		close(srv);

	muxclose(c);
}

void
muxwstat(Chan *c, char *db)
{
	Mux *m;
	Dir d;
	int nc;

	if(c->qid.path == CHDIR)
		error(Eperm);

	m = &muxes[NMUX(c)];
	if(strncmp(u->p->user, m->user, NAMELEN))
		error(Eperm);

	convM2D(db, &d);
	d.mode &= 0777;
	if(c->qid.path&CHDIR || m->srv) {
		strcpy(m->name, d.name);
		m->perm = d.mode;
		return;
	}
	nc = NCON(c);
	switch(nc) {
	case Qclone:
		error(Eperm);
	case Qhead:
		m->perm = d.mode;
		break;
	default:
		m->connects[nc-Qoffset].perm = d.mode;
		break;
	}
}

long
muxread(Chan *c, void *va, long n, ulong offset)
{
	Mux *m;
	Con *cm;
	int bread;

	USED(offset);
	if(c->qid.path & CHDIR)
		return devdirread(c, va, n, 0, 0, muxgen);

	m = &muxes[NMUX(c)];
	switch(NCON(c)) {
	case Qhead:
		bread = muxreadq(m, &m->headq, va, n);
		break;
	case Qclone:
		error(Eperm);
	default:
		cm = &m->connects[NCON(c)-Qoffset];
		bread = muxreadq(m, &cm->conq, va, n);
		break;
	}

	return bread;
}

Con *
muxhdr(Mux *m, char *h)
{
	Con *c;

	if(h[0] != Tmux || h[2] != 0)
		error(Emuxmsg);

	c = &m->connects[h[1]];
	if(c < m->connects || c > &m->connects[Nmux])	
		error(Emuxmsg);

	if(c->ref == 0)
		return 0;

	return c;
}

long
muxwrite(Chan *c, void *va, long n, ulong offset)
{
	Mux *m;
	Con *cm;
	int muxid, fd;
	char *a, hdr[3], buf[10];

	USED(offset);
	if(c->qid.path&CHDIR)
		error(Eisdir);

	m = &muxes[NMUX(c)];
	if(n > Maxmsg || (m->srv && n >= sizeof(buf)))
		error(Etoobig);

	if(m->srv) {
		memmove(buf, va, n);		/* so we can NUL-terminate */
		buf[n] = 0;
		fd = strtoul(buf, 0, 0);
		fdtochan(fd, -1, 0, 0);		/* error check */
		m->c = u->p->fgrp->fd[fd];
		incref(m->c);
		return n;
	}
	switch(NCON(c)) {
	case Qclone:
		error(Eperm);
	case Qhead:
		if(n < 2)
			error(Emuxmsg);

		a = (char*)va;
		memmove(hdr, a, sizeof(hdr));
		cm = muxhdr(m, hdr);
		if(cm == 0)
			error(Ehungup);

		muxwriteq(&cm->conq, a+sizeof(hdr), n-sizeof(hdr), 0, 0);
		break;
	default:
		if(m->headopen == 0)
			error(Ehungup);

		muxid = NCON(c)-Qoffset;
		muxwriteq(&m->headq, va, n, 1, muxid);
		break;
	}

	return n;
}

void
muxwriteq(Dtq *q, char *va, long n, int addid, int muxid)
{
	Block *head, *tail, *bp;
	ulong l, bwrite;

	head = 0;
	tail = 0;
	if(waserror()) {
		if(head)
			freeb(head);
		nexterror();
	}

	bwrite = 0;
	while(n) {
		if(addid) {
			bp = allocb(n+3);
			bp->wptr[0] = Tmux;
			bp->wptr[1] = muxid;
			bp->wptr[2] = 0;
			bp->wptr += 3;
			bwrite += 3;
			addid = 0;
		}
		else
			bp = allocb(n);
		l = bp->lim - bp->wptr;
		if(l > n)
			l = n;
		memmove(bp->wptr, va, l);	/* Interruptable thru fault */
		va += l;
		bp->wptr += l;
		bwrite += l;
		n -= l;
		if(head == 0)
			head = bp;
		else
			tail->next = bp;
		tail = bp;
	}
	poperror();
	tail->flags |= S_DELIM;

	if(q->nb > Flowctl)
		muxflow(q);

	lock(&q->listlk);
	if(q->list == 0)
		q->list = head;
	else {
		for(tail = q->list; tail->next; tail = tail->next)
			;
		tail->next = head;
	}
	q->ndelim++;
	q->nb += bwrite;
	unlock(&q->listlk);
	wakeup(&q->r);
}

int
muxflw(Dtq *q)
{
	return q->nb < Flowctl;
}

void
muxflow(Dtq *q)
{
	qlock(&q->flow);
	if(waserror()) {
		qunlock(&q->flow);
		nexterror();
	}
	sleep(&q->flowr, muxflw, q);
	poperror();
	qunlock(&q->flow);
}

int
havedata(Dtq *q)
{
	int n;

	lock(&q->listlk);
	n = q->ndelim;
	unlock(&q->listlk);
	return n;
}

ulong
muxreadq(Mux *m, Dtq *q, char *va, ulong n)
{
	int l, nread;
	Block *bp, *f1;

	qlock(&q->rd);
	bp = 0;
	if(waserror()) {
		lock(&q->listlk);
		if(bp) {
			bp->next = q->list;
			q->list = bp;
		}
		unlock(&q->listlk);
		qunlock(&q->rd);
		nexterror();
	}
	while(!havedata(q)) {
		sleep(&q->r, havedata, q);
		if(m->headopen == 0)
			error(Emuxshutdown);
	}

	nread = 0;
	f1 = 0;
	lock(&q->listlk);
	while(n) {
		bp = q->list;
		q->list = bp->next;
		bp->next = 0;
		unlock(&q->listlk);
		if(f1) {
			freeb(f1);
			f1 = 0;
		}
		l = BLEN(bp);
		if(l > n)
			l = n;
		memmove(va, bp->rptr, l);	/* Interruptable thru fault */
		va += l;
		bp->rptr += l;
		n -= l;
		nread += l;
		lock(&q->listlk);
		if(bp->rptr == bp->wptr)
			f1 = bp;
		else {
			bp->next = q->list;
			q->list = bp;
		}
		if(bp->flags&S_DELIM) {
			q->ndelim--;
			break;
		}
	}
	q->nb -= nread;
	unlock(&q->listlk);
	if(f1)
		freeb(f1);
	qunlock(&q->rd);
	poperror();
	if(q->nb < Flowctl)
		wakeup(&q->flowr);
	return nread;
}

Block *
muxclq(Dtq *q)
{
	Block *f;

	f = q->list;
	q->list = 0;
	q->nb = 0;
	q->ndelim = 0;
	return f;
}
