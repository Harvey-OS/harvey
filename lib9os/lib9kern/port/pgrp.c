#include	"dat.h"
#include	"fns.h"
#include	"error.h"

static Ref	pgrpid;
static Ref	mountid;

Pgrp*
newpgrp(void)
{
	Pgrp *p;

	p = malloc(sizeof(Pgrp));
	if(p == nil)
		p9error(Enomem);
	p->r.ref = 1;
	p->pgrpid = incref(&pgrpid);
	p->progmode = 0644;
	return p;
}

void
closepgrp(Pgrp *p)
{
	Mhead **h, **e, *f, *next;

	if(p == nil || decref(&p->r) != 0)
		return;

	wlock(&p->ns);
	p->pgrpid = -1;
	e = &p->mnthash[MNTHASH];
	for(h = p->mnthash; h < e; h++) {
		for(f = *h; f; f = next) {
			wlock(&f->lock);
			cclose(f->from);
			mountfree(f->mount);
			f->mount = nil;
			next = f->hash;
			wunlock(&f->lock);
			putmhead(f);
		}
	}
	wunlock(&p->ns);
	cclose(p->dot);
	cclose(p->slash);
	free(p);
}

void
pgrpinsert(Mount **order, Mount *m)
{
	Mount *f;

	m->order = 0;
	if(*order == 0) {
		*order = m;
		return;
	}
	for(f = *order; f; f = f->order) {
		if(m->mountid < f->mountid) {
			m->order = f;
			*order = m;
			return;
		}
		order = &f->order;
	}
	*order = m;
}

/*
 * pgrpcpy MUST preserve the mountid allocation order of the parent group
 */
void
pgrpcpy(Pgrp *to, Pgrp *from)
{
	int i;
	Mount *n, *m, **link, *order;
	Mhead *f, **tom, **l, *mh;

	wlock(&from->ns);
	if(waserror()){
		wunlock(&from->ns);
		nexterror();
	}
	order = 0;
	tom = to->mnthash;
	for(i = 0; i < MNTHASH; i++) {
		l = tom++;
		for(f = from->mnthash[i]; f; f = f->hash) {
			rlock(&f->lock);
			if(waserror()){
				runlock(&f->lock);
				nexterror();
			}
			mh = malloc(sizeof(Mhead));
			if(mh == nil)
				p9error(Enomem);
			mh->from = f->from;
			mh->r.ref = 1;
			incref(&mh->from->r);
			*l = mh;
			l = &mh->hash;
			link = &mh->mount;
			for(m = f->mount; m; m = m->next) {
				n = newmount(mh, m->to, m->mflag, m->spec);
				m->copy = n;
				pgrpinsert(&order, m);
				*link = n;
				link = &n->next;	
			}
			poperror();
			runlock(&f->lock);
		}
	}
	/*
	 * Allocate mount ids in the same sequence as the parent group
	 */
	lock(&mountid.lk);
	for(m = order; m; m = m->order)
		m->copy->mountid = mountid.ref++;
	unlock(&mountid.lk);

	to->progmode = from->progmode;
	to->slash = cclone(from->slash);
	to->dot = cclone(from->dot);
	to->nodevs = from->nodevs;
	poperror();
	wunlock(&from->ns);
}

Fgrp*
newfgrp(Fgrp *old)
{
	Fgrp *new;
	int n;

	new = malloc(sizeof(Fgrp));
	if(new == nil)
		p9error(Enomem);
	new->r.ref = 1;
	n = DELTAFD;
	if(old != nil){
		lock(&old->l);
		if(old->maxfd >= n)
			n = (old->maxfd+1 + DELTAFD-1)/DELTAFD * DELTAFD;
		new->maxfd = old->maxfd;
		unlock(&old->l);
	}
	new->nfd = n;
	new->fd = malloc(n*sizeof(Chan*));
	if(new->fd == nil){
		free(new);
		p9error(Enomem);
	}
	return new;
}

Fgrp*
dupfgrp(Fgrp *f)
{
	int i;
	Chan *c;
	Fgrp *new;
	int n;

	new = malloc(sizeof(Fgrp));
	if(new == nil)
		p9error(Enomem);
	new->r.ref = 1;
	lock(&f->l);
	n = DELTAFD;
	if(f->maxfd >= n)
		n = (f->maxfd+1 + DELTAFD-1)/DELTAFD * DELTAFD;
	new->nfd = n;
	new->fd = malloc(n*sizeof(Chan*));
	if(new->fd == nil){
		unlock(&f->l);
		free(new);
		p9error(Enomem);
	}
	new->maxfd = f->maxfd;
	new->minfd = f->minfd;
	for(i = 0; i <= f->maxfd; i++) {
		if(c = f->fd[i]){
			incref(&c->r);
			new->fd[i] = c;
		}
	}
	unlock(&f->l);

	return new;
}

void
closefgrp(Fgrp *f)
{
	int i;
	Chan *c;

	if(f != nil && decref(&f->r) == 0) {
		for(i = 0; i <= f->maxfd; i++)
			if(c = f->fd[i])
				cclose(c);
		free(f->fd);
		free(f);
	}
}

Mount*
newmount(Mhead *mh, Chan *to, int flag, char *spec)
{
	Mount *m;

	m = malloc(sizeof(Mount));
	if(m == nil)
		p9error(Enomem);
	m->to = to;
	m->head = mh;
	incref(&to->r);
	m->mountid = incref(&mountid);
	m->mflag = flag;
	if(spec != 0)
		kstrdup(&m->spec, spec);

	return m;
}

void
mountfree(Mount *m)
{
	Mount *f;

	while(m) {
		f = m->next;
		cclose(m->to);
		m->mountid = 0;
		free(m->spec);
		free(m);
		m = f;
	}
}

void
closesigs(Skeyset *s)
{
	int i;

	if(s == nil || decref(&s->r) != 0)
		return;
	for(i=0; i<s->nkey; i++)
		freeskey(s->keys[i]);
	free(s);
}

void
freeskey(Signerkey *key)
{
	if(key == nil || decref(&key->r) != 0)
		return;
	free(key->owner);
	(*key->pkfree)(key->pk);
	free(key);
}
