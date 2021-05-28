#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

enum
{
	Nfreepgs = 0, // 1*GiB/PGSZ
};

#define pghash(daddr)	palloc.hash[(daddr>>PGSHFT)&(PGHSIZE-1)]

struct	Palloc palloc;

static	uint	highwater;	/* TO DO */

char*
seprintpagestats(char *s, char *e)
{
	Pallocpg *pg;
	int i;

	lock(&palloc);
	for(i = 0; i < nelem(palloc.avail); i++){
		pg = &palloc.avail[i];
		if(pg->freecount != 0)
			s = seprint(s, e, "%lud/%lud %dK user pages avail\n",
				pg->freecount,
				pg->count, (1<<i)/KiB);
	}
	unlock(&palloc);
	return s;
}

void
pageinit(void)
{
	uintmem avail;
	uvlong pkb, kkb, kmkb, mkb;

	avail = sys->pmpaged;
	palloc.user = avail/PGSZ;

	/* user, kernel, kernel malloc area, memory */
	pkb = palloc.user*PGSZ/KiB;
	kkb = ROUNDUP((uintptr)end - KTZERO, PGSZ)/KiB;
	kmkb = ROUNDUP(sys->vmend - (uintptr)end, PGSZ)/KiB;
	mkb = sys->pmoccupied/KiB;

	/* Paging numbers */
	highwater = (palloc.user*5)/100;
	if(highwater >= 64*MiB/PGSZ)
		highwater = 64*MiB/PGSZ;

	print("%lldM memory: %lldK+%lldM kernel, %lldM user, %lldM lost\n",
		mkb/KiB, kkb, kmkb/KiB, pkb/KiB, (vlong)(mkb-kkb-kmkb-pkb)/KiB);
}

Page*
pgalloc(uint lg2size, int color)
{
	Page *pg;

	if((pg = malloc(sizeof(Page))) == nil){
		DBG("pgalloc: malloc failed\n");
		return nil;
	}
	memset(pg, 0, sizeof *pg);
	if((pg->pa = physalloc(1<<lg2size, &color, pg)) == 0){
		DBG("pgalloc: physalloc failed: size %#ux color %d\n", 1<<lg2size, color);
		free(pg);
		return nil;
	}
	pg->lg2size = lg2size;
	palloc.avail[pg->lg2size].count++;
	pg->color = color;
	return pg;
}

void
pgfree(Page* pg)
{
	palloc.avail[pg->lg2size].count--;
	physfree(pg->pa, pagesize(pg));
	free(pg);
}

void
pageunchain(Page *p)
{
	Pallocpg *pg;

	if(canlock(&palloc))
		panic("pageunchain (palloc %#p)", &palloc);
	pg = &palloc.avail[p->lg2size];
	if(p->prev)
		p->prev->next = p->next;
	else
		pg->head = p->next;
	if(p->next)
		p->next->prev = p->prev;
	else
		pg->tail = p->prev;
	p->prev = p->next = nil;
	pg->freecount--;
}

void
pagechaintail(Page *p)
{
	Pallocpg *pg;

	if(canlock(&palloc))
		panic("pagechaintail");
	pg = &palloc.avail[p->lg2size];
	if(pg->tail) {
		p->prev = pg->tail;
		pg->tail->next = p;
	}
	else {
		pg->head = p;
		p->prev = nil;
	}
	pg->tail = p;
	p->next = nil;
	pg->freecount++;
}

void
pagechainhead(Page *p)
{
	Pallocpg *pg;

	if(canlock(&palloc))
		panic("pagechainhead");
	pg = &palloc.avail[p->lg2size];
	if(pg->head) {
		p->next = pg->head;
		pg->head->prev = p;
	}
	else {
		pg->tail = p;
		p->next = nil;
	}
	pg->head = p;
	p->prev = nil;
	pg->freecount++;
}

static Page*
findpg(Page *pl, int color)
{
	Page *p;

	for(p = pl; p != nil; p = p->next)
		if(color == NOCOLOR || p->color == color)
			return p;
	return nil;
}

/*
 * allocate and return a new page for the given virtual address in segment s;
 * return nil iff s was locked on entry and had to be unlocked to wait for memory.
 */
Page*
newpage(int clear, Segment *s, uintptr va, uint lg2pgsize, int color, int locked)
{
	Page *p;
	KMap *k;
	uchar ct;
	Pallocpg *pg;
	int i, hw, dontalloc;
	static int once;

	lock(&palloc);
	pg = &palloc.avail[lg2pgsize];
	hw = highwater;
	for(i = 0;; i++) {
		if(pg->freecount > hw)
			break;
		if(up != nil && up->kp && pg->freecount > 0)
			break;

		if(i > 3)
			color = NOCOLOR;

		p = findpg(pg->head, color);
		if(p != nil)
			break;

		p = pgalloc(lg2pgsize, color);
		if(p != nil){
			pagechainhead(p);
			break;
		}

		unlock(&palloc);
		dontalloc = 0;
		if(locked){
			qunlock(&s->lk);
			locked = 0;
			dontalloc = 1;
		}
		qlock(&palloc.pwait);	/* Hold memory requesters here */

		while(waserror())	/* Ignore interrupts */
			;

		kickpager(lg2pgsize, color);
		tsleep(&palloc.r, ispages, pg, 1000);

		poperror();

		qunlock(&palloc.pwait);

		/*
		 * If called from fault and we lost the segment from
		 * underneath don't waste time allocating and freeing
		 * a page. Fault will call newpage again when it has
		 * reacquired the segment locks
		 */
		if(dontalloc)
			return nil;

		lock(&palloc);
	}

	/* First try for our colour */
	for(p = pg->head; p; p = p->next)
		if(color == NOCOLOR || p->color == color)
			break;

	ct = PG_NOFLUSH;
	if(p == nil) {
		p = pg->head;
		p->color = color;
		ct = PG_NEWCOL;
	}

	pageunchain(p);

	lock(p);
	if(p->ref != 0)
		panic("newpage: p->ref %d != 0", p->ref);

	uncachepage(p);
	p->ref++;
	p->va = va;
	p->modref = 0;
	mmucachectl(p, ct);
	unlock(p);
	unlock(&palloc);

	if(clear) {
		k = kmap(p);
		memset((void*)VA(k), 0, pagesize(p));
		kunmap(k);
	}

	return p;
}

int
ispages(void *a)
{
	return ((Pallocpg*)a)->freecount > highwater;
}

void
putpage(Page *p)
{
	Pallocpg *pg;
	int rlse;

	lock(&palloc);
	lock(p);

	if(p->ref == 0)
		panic("putpage");

	if(--p->ref > 0) {
		unlock(p);
		unlock(&palloc);
		return;
	}

	rlse = 0;
	if(p->image != nil)
		pagechaintail(p);
	else{
		/*
		 * Free pages if we have plenty in the free list.
		 */
		pg = &palloc.avail[p->lg2size];
		if(pg->freecount > Nfreepgs)
			rlse = 1;
		else
			pagechainhead(p);
	}

	if(palloc.r.p != nil)
		wakeup(&palloc.r);

	unlock(p);
	if(rlse)
		pgfree(p);
	unlock(&palloc);
}

Page*
auxpage(uint lg2size)
{
	Page *p;
	Pallocpg *pg;

	lock(&palloc);
	pg = &palloc.avail[lg2size];
	p = pg->head;
	if(pg->freecount <= highwater) {
		/* memory's tight, don't use it for file cache */
		unlock(&palloc);
		return nil;
	}
	pageunchain(p);

	lock(p);
	if(p->ref != 0)
		panic("auxpage");
	p->ref++;
	uncachepage(p);
	unlock(p);
	unlock(&palloc);

	return p;
}

static int dupretries = 15000;

int
duppage(Page *p)				/* Always call with p locked */
{
	Pallocpg *pg;
	Page *np;
	int color;
	int retries;

	retries = 0;
retry:
	/* don't dup shared page */
	if(p->ref != 1)
		return 0;

	if(retries++ > dupretries){
		print("duppage %d, up %#p\n", retries, up);
		dupretries += 100;
		if(dupretries > 100000)
			panic("duppage\n");
		uncachepage(p);
		return 1;
	}

	/* don't dup pages with no image */
	if(p->ref == 0 || p->image == nil || p->image->notext)
		return 0;

	/* don't dup large pages TO DO? */
	if(p->lg2size != PGSHFT){
		uncachepage(p);
		return 1;
	}

	/*
	 *  normal lock ordering is to call
	 *  lock(&palloc) before lock(p).
	 *  To avoid deadlock, we have to drop
	 *  our locks and try again.
	 */
	if(!canlock(&palloc)){
		unlock(p);
		if(up)
			sched();
		lock(p);
		goto retry;
	}

	pg = &palloc.avail[p->lg2size];
	/* No freelist cache when memory is relatively low */
	if(pg->freecount <= highwater) {
		unlock(&palloc);
		uncachepage(p);
		return 1;
	}

	color = p->color;
	for(np = pg->head; np; np = np->next)
		if(np->color == color)
			break;

	/* No page of the correct color */
	if(np == nil) {
		unlock(&palloc);
		uncachepage(p);
		return 1;
	}

	pageunchain(np);
	pagechaintail(np);

/*
* XXX - here's a bug? - np is on the freelist but it's not really free.
* when we unlock palloc someone else can come in, decide to
* use np, and then try to lock it.  they succeed after we've
* run copypage and cachepage and unlock(np).  then what?
* they call pageunchain before locking(np), so it's removed
* from the freelist, but still in the cache because of
* cachepage below.  if someone else looks in the cache
* before they remove it, the page will have a nonzero ref
* once they finally lock(np).
*/
	lock(np);
	unlock(&palloc);

	/* Cache the new version */
	uncachepage(np);
	np->va = p->va;
	np->daddr = p->daddr;
	copypage(p, np);
	cachepage(np, p->image);
	unlock(np);
	uncachepage(p);

	return 0;
}

void
copypage(Page *f, Page *t)
{
	KMap *ks, *kd;

	if(f->lg2size != t->lg2size)
		panic("copypage");
	ks = kmap(f);
	kd = kmap(t);
	memmove((void*)VA(kd), (void*)VA(ks), pagesize(t));
	kunmap(ks);
	kunmap(kd);
}

void
uncachepage(Page *p)			/* Always called with a locked page */
{
	Page **l, *f;

	if(p->image == nil)
		return;

	lock(&palloc.hashlock);
	l = &pghash(p->daddr);
	for(f = *l; f != nil; f = f->hash) {
		if(f == p) {
			*l = p->hash;
			break;
		}
		l = &f->hash;
	}
	unlock(&palloc.hashlock);
	putimage(p->image);
	p->image = nil;
	p->daddr = 0;
}

void
cachepage(Page *p, Image *i)
{
	Page **l;

	/* If this ever happens it should be fixed by calling
	 * uncachepage instead of panic. I think there is a race
	 * with pio in which this can happen. Calling uncachepage is
	 * correct - I just wanted to see if we got here.
	 */
	if(p->image)
		panic("cachepage");

	incref(i);
	lock(&palloc.hashlock);
	p->image = i;
	l = &pghash(p->daddr);
	p->hash = *l;
	*l = p;
	unlock(&palloc.hashlock);
}

void
cachedel(Image *i, ulong daddr)
{
	Page *f, **l;

	lock(&palloc.hashlock);
	l = &pghash(daddr);
	for(f = *l; f != nil; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			lock(f);
			if(f->image == i && f->daddr == daddr){
				*l = f->hash;
				putimage(f->image);
				f->image = nil;
				f->daddr = 0;
			}
			unlock(f);
			break;
		}
		l = &f->hash;
	}
	unlock(&palloc.hashlock);
}

Page *
lookpage(Image *i, ulong daddr)
{
	Page *f;

	lock(&palloc.hashlock);
	for(f = pghash(daddr); f != nil; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			unlock(&palloc.hashlock);

			lock(&palloc);
			lock(f);
			if(f->image != i || f->daddr != daddr) {
				unlock(f);
				unlock(&palloc);
				return 0;
			}
			if(++f->ref == 1)
				pageunchain(f);
			unlock(&palloc);
			unlock(f);

			return f;
		}
	}
	unlock(&palloc.hashlock);

	return nil;
}

uvlong
pagereclaim(int npages)
{
	Page *p;
	uvlong ticks;
	int i;

	lock(&palloc);
	ticks = fastticks(nil);

	/*
	 * All the pages with images backing them are at the
	 * end of the list (see putpage) so start there and work
	 * backward.
	 */
	for(i = 0; i < nelem(palloc.avail); i++) {
		for(p = palloc.avail[i].tail; p != nil && p->image != nil && npages > 0; p = p->prev) {
			if(p->ref == 0 && canlock(p)) {
				if(p->ref == 0) {
					npages--;
					uncachepage(p);
					pageunchain(p);
					pagechainhead(p);
				}
				unlock(p);
			}
		}
	}
	ticks = fastticks(nil) - ticks;
	unlock(&palloc);

	return ticks;
}

Pte*
ptecpy(Pte *old)
{
	Pte *new;
	Page **src, **dst, *pg;

	new = ptealloc();
	dst = &new->pages[old->first-old->pages];
	new->first = dst;
	for(src = old->first; src <= old->last; src++, dst++){
		if((pg = *src) != nil){
			lock(pg);
			pg->ref++;
			unlock(pg);
			new->last = dst;
			*dst = pg;
		}
	}

	return new;
}

Pte*
ptealloc(void)
{
	Pte *new;

	new = smalloc(sizeof(Pte));
	new->first = &new->pages[PTEPERTAB];
	new->last = new->pages;
	return new;
}

void
freepte(Segment *s, Pte *p)
{
	int ref;
	void (*fn)(Page*);
	Page *pt, **pg, **ptop;

	switch(s->type&SG_TYPE) {
	case SG_PHYSICAL:
		fn = s->pseg->pgfree;
		ptop = &p->pages[PTEPERTAB];
		if(fn) {
			for(pg = p->pages; pg < ptop; pg++) {
				if(*pg == nil)
					continue;
				(*fn)(*pg);
				*pg = nil;
			}
			break;
		}
		for(pg = p->pages; pg < ptop; pg++) {
			pt = *pg;
			if(pt == nil)
				continue;
			lock(pt);
			ref = --pt->ref;
			unlock(pt);
			if(ref == 0)
				free(pt);
		}
		break;
	default:
		for(pg = p->first; pg <= p->last; pg++)
			if(*pg != nil) {
				putpage(*pg);
				*pg = nil;
			}
	}
	free(p);
}
