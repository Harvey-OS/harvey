#include "lib9.h"
#include "sys.h"
#include "error.h"

Proc	*up;
static Ref mountid;

void
procinit(void)
{
	chandevinit();

	up = mallocz(sizeof(Proc));
	up->pgrp = newpgrp();
	up->fgrp = newfgrp();
	strcpy(up->user, "none");
	osfillproc(up);

	if(waserror())
		panic("setting root and dot");

	up->slash = namec("#/", Atodir, 0, 0);
	up->dot = clone(up->slash, 0);

	poperror();

	sysbind("#U", "/", MAFTER);

	/* open fd 0,1 and 2 */
	if(sysopen("#c/cons", OREAD) != 0)
		iprint("failed to make fd0 from #c/cons: %r\n");

	sysopen("#c/cons", OWRITE);	
	sysopen("#c/cons", OWRITE);

	sysbind("#c", "/dev", MAFTER);
}

void
panic(char *fmt, ...)
{
	char buf[512];
	char buf2[512];
	va_list va;

	va_start(va, fmt);
	doprint(buf, buf+sizeof(buf), fmt, va);
	va_end(va);
	sprint(buf2, "panic: %s\n", buf);
	write(2, buf2, strlen(buf2));

	abort();
}

Mount*
newmount(Mhead *mh, Chan *to, int flag, char *spec)
{
	Mount *m;

	m = mallocz(sizeof(Mount));
	m->to = to;
	m->head = mh;
	refinc(&to->r);
	m->mountid = refinc(&mountid);
	m->flag = flag;
	if(spec != 0)
		strcpy(m->spec, spec);

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
		free(m);
		m = f;
	}
}


Pgrp*
newpgrp(void)
{
	Pgrp *p;

	p = mallocz(sizeof(Pgrp));
	p->r.ref = 1;
	return p;
}

Fgrp*
newfgrp(void)
{
	Fgrp *f;
	f = mallocz(sizeof(Fgrp));

	f->r.ref = 1;
	return f;
}

void
closepgrp(Pgrp *p)
{
	Mhead **h, **e, *f, *next;
	
	if(refdec(&p->r) == 0) {
		e = &p->mnthash[MNTHASH];
		for(h = p->mnthash; h < e; h++) {
			for(f = *h; f; f = next) {
				cclose(f->from);
				mountfree(f->mount);
				next = f->hash;
				free(f);
			}
		}
		free(p);
	}
}

Fgrp*
dupfgrp(Fgrp *f)
{
	int i;
	Chan *c;
	Fgrp *new;

	new = newfgrp();

	lock(&f->r.l);
	new->maxfd = f->maxfd;
	for(i = 0; i <= f->maxfd; i++) {
		if((c = f->fd[i]) != nil){
			refinc(&c->r);
			new->fd[i] = c;
		}
	}
	unlock(&f->r.l);

	return new;
}

void
closefgrp(Fgrp *f)
{
	int i;
	Chan *c;

	if(refdec(&f->r) == 0) {
		for(i = 0; i <= f->maxfd; i++)
			if((c = f->fd[i]) != nil)
				cclose(c);

		free(f);
	}
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

	rlock(&from->ns);
	order = 0;
	tom = to->mnthash;
	for(i = 0; i < MNTHASH; i++) {
		l = tom++;
		for(f = from->mnthash[i]; f; f = f->hash) {
			mh = mallocz(sizeof(Mhead));
			mh->from = f->from;
			refinc(&mh->from->r);
			*l = mh;
			l = &mh->hash;
			link = &mh->mount;
			for(m = f->mount; m; m = m->next) {
				n = mallocz(sizeof(Mount));
				n->to = m->to;
				refinc(&n->to->r);
				n->head = mh;
				n->flag = m->flag;
				if(m->spec != 0)
					strcpy(n->spec, m->spec);
				m->copy = n;
				pgrpinsert(&order, m);
				*link = n;
				link = &n->next;        
			}
		}
	}
	/*
	 * Allocate mount ids in the same sequence as the parent group
	 */
	lock(&mountid.l);
	for(m = order; m; m = m->order)
		m->copy->mountid = mountid.ref++;
	unlock(&mountid.l);
	runlock(&from->ns);
}
