#include "all.h"

/*
 * Misc tools.
 */

static Lock	lstatslk;
static Lstat	none;
static Lstat	*lstats;
static int	lstatson;
int fatalaborts;

Alloc pathalloc =
{
	.elsz = sizeof(Path),
	.zeroing = 0,
};

void
fatal(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
	fprint(2, "\n");
	if(fatalaborts)
		abort();
	threadexitsall("fatal");
}

void
lockstats(int on)
{
	if(lstats == nil && on)
		lstats = mallocz(sizeof lstats[0] * Nlstats, 1);
	lstatson = on;
}

void
dumplockstats(void)
{
	static char *tname[] = {"qlock", "rwlock", "lock"};
	int lon, i;
	Lstat *lst;

	lon = lstatson;
	lstatson = 0;
	fprint(2, "locks\tpc\tntimes\tncant\twtime\tmtime\n");
	for(i = 0; i < Nlstats; i++){
		lst = &lstats[i];
		if(lst->ntimes != 0)
			fprint(2, "src -n -s %#ulx %s\t# %s\t%d\t%d\t%ulld\t%ulld\t\n",
				lst->pc, argv0, tname[lst->type], lst->ntimes,
				lst->ncant, lst->wtime, lst->wtime/lst->ntimes);
	}
	lstatson = lon;
}

static Lstat*
getlstat(uintptr pc, int type)
{
	Lstat *lst;
	int i, h;

	h = pc%Nlstats;
	lock(&lstatslk);
	for(i = 0; i < Nlstats; i++){
		lst = &lstats[(h+i)%Nlstats];
		if(lst->pc == 0){
			lst->type = type;
			lst->pc = pc;
		}
		if(lst->pc == pc){
			unlock(&lstatslk);
			return lst;
		}
	}
	unlock(&lstatslk);
	return &none;
}

void
xqlock(QLock *q)
{
	vlong t;
	Lstat *lst;

	lst = nil;
	if(lstats != nil){
		lst = getlstat(getcallerpc(&q), Tqlock);
		ainc(&lst->ntimes);
		if(canqlock(q))
			return;
		ainc(&lst->ncant);
		t = nsec();
	}
	qlock(q);
	if(lstats != nil){
		t = nsec() - t;
		lock(&lstatslk);
		lst->wtime += t;
		unlock(&lstatslk);
	}
}

void
xqunlock(QLock *q)
{
	qunlock(q);
}

void
xrwlock(RWLock *rw, int iswr)
{
	vlong t;
	Lstat *lst;

	lst = nil;
	if(lstats != nil){
		lst = getlstat(getcallerpc(&rw), Trwlock);
		ainc(&lst->ntimes);
		if(iswr){
			if(canwlock(rw))
				return;
		}else
			if(canrlock(rw))
				return;
		ainc(&lst->ncant);
		t = nsec();
	}
	if(iswr)
		wlock(rw);
	else
		rlock(rw);
	if(lstats != nil){
		t = nsec() - t;
		lock(&lstatslk);
		lst->wtime += t;
		unlock(&lstatslk);
	}
}

void
xrwunlock(RWLock *rw, int iswr)
{
	if(iswr)
		wunlock(rw);
	else
		runlock(rw);
}

void*
anew(Alloc *a)
{
	Next *n;

	assert(a->elsz > 0);
	xqlock(a);
	n = a->free;
	if(n != nil){
		a->free = n->next;
		a->nfree--;
	}else{
		a->nalloc++;
		n = mallocz(a->elsz, !a->zeroing);
	}
	xqunlock(a);
	if(a->zeroing)
		memset(n, 0, a->elsz);
	return n;
	
}

void
afree(Alloc *a, void *nd)
{
	Next *n;

	if(nd == nil)
		return;
	n = nd;
	xqlock(a);
	n->next = a->free;
	a->free = n;
	a->nfree++;
	xqunlock(a);
}

static void
xaddelem(Path *p, Memblk *f)
{
	if(p->nf == p->naf){
		p->naf += Incr;
		p->f = realloc(p->f, p->naf*sizeof p->f[0]);
	}
	p->f[p->nf++] = f;
	incref(f);
}

static Path*
duppath(Path *p)
{
	Path *np;
	int i;

	np = newpath(p->f[0]);
	for(i = 1; i < p->nf; i++)
		xaddelem(np, p->f[i]);
	return np;
}

void
ownpath(Path **pp)
{
	Path *p;

	p = *pp;
	if(p->ref > 1){
		*pp = duppath(p);
		putpath(p);
	}
}

Path*
addelem(Path **pp, Memblk *f)
{
	Path *p;

	ownpath(pp);
	p = *pp;
	xaddelem(p, f);
	return p;
}

Path*
dropelem(Path **pp)
{
	Path *p;

	ownpath(pp);
	p = *pp;
	if(p->nf > 0)
		mbput(p->f[--p->nf]);
	return p;
}

Path*
newpath(Memblk *root)
{
	Path *p;

	p = anew(&pathalloc);
	p->ref = 1;
	xaddelem(p, root);
	return p;
}

void
putpath(Path *p)
{
	int i;

	if(p == nil || decref(p) > 0)
		return;
	for(i = 0; i < p->nf; i++)
		mbput(p->f[i]);
	p->nf = 0;
	afree(&pathalloc, p);
}

Path*
clonepath(Path *p)
{
	incref(p);
	return p;
}

int
pathfmt(Fmt *fmt)
{
	Path *p;
	int i;

	
	p = va_arg(fmt->args, Path*);
	if(p == nil)
		return fmtprint(fmt, "/");
	for(i = 1; i < p->nf; i++)
		fmtprint(fmt, "p[%d] = %H", i, p->f[i]);
	return 0;
}
