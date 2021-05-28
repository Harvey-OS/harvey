#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum {
	Whinesecs = 10,		/* frequency of out-of-resources printing */
};

static Ref pgrpid;
static Ref mountid;

void
pgrpnote(ulong noteid, char *a, long n, int flag)
{
	int i;
	Proc *p;
	char buf[ERRMAX];

	if(n >= ERRMAX-1)
		error(Etoobig);

	memmove(buf, a, n);
	buf[n] = 0;
	for(i = 0; (p = psincref(i)) != nil; i++){
		if(p == up || p->state == Dead || p->noteid != noteid || p->kp){
			psdecref(p);
			continue;
		}
		qlock(&p->debug);
		if(p->pid == 0 || p->noteid != noteid){
			qunlock(&p->debug);
			psdecref(p);
			continue;
		}
		if(!waserror()) {
			postnote(p, 0, buf, flag);
			poperror();
		}
		qunlock(&p->debug);
		psdecref(p);
	}
}

Pgrp*
newpgrp(void)
{
	Pgrp *p;

	p = smalloc(sizeof(Pgrp));
	p->ref = 1;
	p->pgrpid = incref(&pgrpid);
	return p;
}

Rgrp*
newrgrp(void)
{
	Rgrp *r;

	r = smalloc(sizeof(Rgrp));
	r->ref = 1;
	return r;
}

void
closergrp(Rgrp *r)
{
	if(decref(r) == 0)
		free(r);
}

void
closepgrp(Pgrp *p)
{
	Mhead **h, **e, *f, *next;

	if(decref(p) != 0)
		return;

	qlock(&p->debug);
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
	qunlock(&p->debug);
	free(p);
}

void
pgrpinsert(Mount **order, Mount *mount)
{
	Mount *f;

	mount->order = 0;
	if(*order == 0) {
		*order = mount;
		return;
	}
	for(f = *order; f; f = f->order) {
		if(mount->mountid < f->mountid) {
			mount->order = f;
			*order = mount;
			return;
		}
		order = &f->order;
	}
	*order = mount;
}

/*
 * pgrpcpy MUST preserve the mountid allocation order of the parent group
 */
void
pgrpcpy(Pgrp *to, Pgrp *from)
{
	int i;
	Mount *n, *mount, **link, *order;
	Mhead *f, **tom, **l, *mh;

	wlock(&from->ns);
	order = 0;
	tom = to->mnthash;
	for(i = 0; i < MNTHASH; i++) {
		l = tom++;
		for(f = from->mnthash[i]; f; f = f->hash) {
			rlock(&f->lock);
			mh = newmhead(f->from);
			*l = mh;
			l = &mh->hash;
			link = &mh->mount;
			for(mount = f->mount; mount != nil; mount = mount->next) {
				n = newmount(mh, mount->to, mount->mflag, mount->spec);
				mount->copy = n;
				pgrpinsert(&order, mount);
				*link = n;
				link = &n->next;
			}
			runlock(&f->lock);
		}
	}
	/*
	 * Allocate mount ids in the same sequence as the parent group
	 */
	lock(&mountid);
	for(mount = order; mount != nil; mount = mount->order)
		mount->copy->mountid = mountid.ref++;
	unlock(&mountid);
	wunlock(&from->ns);
}

Fgrp*
dupfgrp(Fgrp *f)
{
	Fgrp *new;
	Chan *c;
	int i;

	new = smalloc(sizeof(Fgrp));
	if(f == nil){
		new->fd = smalloc(DELTAFD*sizeof(Chan*));
		new->nfd = DELTAFD;
		new->ref = 1;
		return new;
	}

	lock(f);
	/* Make new fd list shorter if possible, preserving quantization */
	new->nfd = f->maxfd+1;
	i = new->nfd%DELTAFD;
	if(i != 0)
		new->nfd += DELTAFD - i;
	new->fd = malloc(new->nfd*sizeof(Chan*));
	if(new->fd == nil){
		unlock(f);
		free(new);
		error("no memory for fgrp");
	}
	new->ref = 1;

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

	if(f == 0)
		return;

	if(decref(f) != 0)
		return;

	/*
	 * If we get into trouble, forceclosefgrp
	 * will bail us out.
	 */
	up->closingfgrp = f;
	for(i = 0; i <= f->maxfd; i++){
		if(c = f->fd[i]){
			f->fd[i] = nil;
			cclose(c);
		}
	}
	up->closingfgrp = nil;

	free(f->fd);
	free(f);
}

/*
 * Called from sleep because up is in the middle
 * of closefgrp and just got a kill ctl message.
 * This usually means that up has wedged because
 * of some kind of deadly embrace with mntclose
 * trying to talk to itself.  To break free, hand the
 * unclosed channels to the close queue.  Once they
 * are finished, the blocked cclose that we've
 * interrupted will finish by itself.
 */
void
forceclosefgrp(void)
{
	int i;
	Chan *c;
	Fgrp *f;

	if(up->procctl != Proc_exitme || up->closingfgrp == nil){
		print("bad forceclosefgrp call");
		return;
	}

	f = up->closingfgrp;
	for(i = 0; i <= f->maxfd; i++){
		if(c = f->fd[i]){
			f->fd[i] = nil;
			ccloseq(c);
		}
	}
}

Mount*
newmount(Mhead *mh, Chan *to, int flag, char *spec)
{
	Mount *mount;

	mount = smalloc(sizeof(Mount));
	mount->to = to;
	mount->head = mh;
	incref(to);
	mount->mountid = incref(&mountid);
	mount->mflag = flag;
	if(spec != 0)
		kstrdup(&mount->spec, spec);

	return mount;
}

void
mountfree(Mount *mount)
{
	Mount *f;

	while(mount != nil) {
		f = mount->next;
		cclose(mount->to);
		mount->mountid = 0;
		free(mount->spec);
		free(mount);
		mount = f;
	}
}

void
resrcwait(char *reason)
{
	ulong now;
	char *p;
	static ulong lastwhine;

	if(up == nil)
		panic("resrcwait");

	p = up->psstate;
	if(reason) {
		up->psstate = reason;
		now = seconds();
		/* don't tie up the console with complaints */
		if(now - lastwhine > Whinesecs) {
			lastwhine = now;
			print("%s\n", reason);
		}
	}

	tsleep(&up->sleep, return0, 0, 300);
	up->psstate = p;
}
