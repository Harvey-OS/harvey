#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

static Ref pgrpid;
static Ref mountid;

struct
{
	Lock;
	Crypt	*free;
} cryptalloc;

/*
 * crypt entries are allocated from a pool rather than allocated using malloc so
 * the memory can be protected from reading by devproc. The base and top of the
 * crypt arena is stored in palloc for devproc.
 */
Crypt*
newcrypt(void)
{
	Crypt *c;

	lock(&cryptalloc);
	if(cryptalloc.free) {
		c = cryptalloc.free;
		cryptalloc.free = c->next;
		unlock(&cryptalloc);
		return c;
	}

	cryptalloc.free = xalloc(sizeof(Crypt)*conf.nproc);
	if(cryptalloc.free == 0)
		panic("newcrypt");

	for(c = cryptalloc.free; c < cryptalloc.free+conf.nproc-1; c++)
		c->next = c+1;

	palloc.cmembase = (ulong)cryptalloc.free;
	palloc.cmemtop = palloc.cmembase+(sizeof(Crypt)*conf.nproc);
	unlock(&cryptalloc);
	return newcrypt();
}

void
freecrypt(Crypt *c)
{
	lock(&cryptalloc);
	c->next = cryptalloc.free;
	cryptalloc.free = c;
	unlock(&cryptalloc);
}

void
pgrpnote(ulong noteid, char *a, long n, int flag)
{
	Proc *p, *ep;
	char buf[ERRLEN];

	if(n >= ERRLEN-1)
		error(Etoobig);

	memmove(buf, a, n);
	buf[n] = 0;
	p = proctab(0);
	ep = p+conf.nproc;
	for(; p < ep; p++) {
		if(p->state == Dead)
			continue;
		if(p->noteid == noteid && p->kp == 0) {
			qlock(&p->debug);
			if(p->pid == 0 || p->noteid != noteid){
				qunlock(&p->debug);
				continue;
			}
			if(!waserror()) {
				postnote(p, 0, buf, flag);
				poperror();
			}
			qunlock(&p->debug);
		}
	}
}

Pgrp*
newpgrp(void)
{
	Pgrp *p;

	p = smalloc(sizeof(Pgrp));
	p->ref = 1;
	p->crypt = newcrypt();
	p->pgrpid = incref(&pgrpid);
	return p;
}

void
closepgrp(Pgrp *p)
{
	Mhead **h, **e, *f, *next;
	
	if(decref(p) == 0){
		qlock(&p->debug);
		p->pgrpid = -1;

		e = &p->mnthash[MNTHASH];
		for(h = p->mnthash; h < e; h++) {
			for(f = *h; f; f = next) {
				close(f->from);
				mountfree(f->mount);
				next = f->hash;
				free(f);
			}
		}
		qunlock(&p->debug);
		freecrypt(p->crypt);
		free(p);
	}
}

void
pgrpcpy(Pgrp *to, Pgrp *from)
{
	Mhead **h, **e, *f, **tom, **l, *mh;
	Mount *n, *m, **link;

	rlock(&from->ns);

	*to->crypt = *from->crypt;
	e = &from->mnthash[MNTHASH];
	tom = to->mnthash;
	for(h = from->mnthash; h < e; h++) {
		l = tom++;
		for(f = *h; f; f = f->hash) {
			mh = smalloc(sizeof(Mhead));
			mh->from = f->from;
			incref(mh->from);
			*l = mh;
			l = &mh->hash;
			link = &mh->mount;
			for(m = f->mount; m; m = m->next) {
				n = newmount(mh, m->to);
				*link = n;
				link = &n->next;	
			}
		}
	}
	runlock(&from->ns);
}

Fgrp*
dupfgrp(Fgrp *f)
{
	Fgrp *new;
	Chan *c;
	int i;

	new = smalloc(sizeof(Fgrp));
	new->ref = 1;

	lock(f);
	new->maxfd = f->maxfd;
	for(i = 0; i <= f->maxfd; i++) {
		if(c = f->fd[i]){
			incref(c);
			new->fd[i] = c;
		}
	}
	unlock(f);

	return new;
}

void
closefgrp(Fgrp *f)
{
	int i;
	Chan *c;

	if(decref(f) == 0) {
		for(i = 0; i <= f->maxfd; i++)
			if(c = f->fd[i])
				close(c);

		free(f);
	}
}


Mount*
newmount(Mhead *mh, Chan *to)
{
	Mount *m;

	m = smalloc(sizeof(Mount));
	m->to = to;
	m->head = mh;
	incref(to);
	m->mountid = incref(&mountid);
	return m;
}

void
mountfree(Mount *m)
{
	Mount *f;

	while(m) {
		f = m->next;
		close(m->to);
		free(m);
		m = f;
	}
}

void
resrcwait(char *reason)
{
	char *p;

	p = u->p->psstate;
	if(reason) {
		u->p->psstate = reason;
		print("%s\n", reason);
	}
	if(u == 0)
		panic("resrcwait");

	tsleep(&u->p->sleep, return0, 0, 300);
	u->p->psstate = p;
}
