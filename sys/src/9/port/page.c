#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define	pghash(daddr)	palloc.hash[(daddr>>PGSHIFT)&(PGHSIZE-1)]

static	Lock pglock;
struct	Palloc palloc;

void
pageinit(void)
{
	Page *p;
	ulong np, hw, hr, vmem, pmem;

	np = palloc.np0+palloc.np1;
	palloc.head = xalloc(np*sizeof(Page));
	if(palloc.head == 0)
		panic("pageinit");

	p = palloc.head;
	while(palloc.np0 > 0) {
		p->prev = p-1;
		p->next = p+1;
		p->pa = palloc.p0;
		palloc.p0 += BY2PG;
		palloc.np0--;
		p++;
	}
	while(palloc.np1 > 0) {
		p->prev = p-1;
		p->next = p+1;
		p->pa = palloc.p1;
		palloc.p1 += BY2PG;
		palloc.np1--;
		p++;
	}
	palloc.tail = p - 1;
	palloc.head->prev = 0;
	palloc.tail->next = 0;

	palloc.user = p - palloc.head;
	palloc.freecount = palloc.user;
	pmem = palloc.user*BY2PG/1024;
	vmem = pmem + (conf.nswap*BY2PG)/1024;

	/* Pageing numbers */
	swapalloc.highwater = (palloc.freecount*5)/100;
	swapalloc.headroom = swapalloc.highwater + (swapalloc.highwater/4);

	hw = swapalloc.highwater*BY2PG;
	hr = swapalloc.headroom*BY2PG;
	
	print("%lud free pages, %dK bytes, swap %dK, highwater %dK, headroom %dK\n", 
	palloc.user, pmem, vmem, hw/1024, hr/1024);/**/
}

Page*
newpage(int clear, Segment **s, ulong va)
{
	Page *p;
	KMap *k;
	int hw, i, dontalloc;

retry:
	lock(&palloc);

	hw = swapalloc.highwater;
	while((palloc.freecount < hw && u->p->kp == 0) || palloc.freecount == 0) {
		palloc.wanted++;
		unlock(&palloc);
		dontalloc = 0;
		if(s && *s) {
			qunlock(&((*s)->lk));
			*s = 0;
			dontalloc = 1;
		}
		qlock(&palloc.pwait);			/* Hold memory requesters here */

		while(waserror())			/* Ignore interrupts */
			;

		kickpager();
		tsleep(&palloc.r, ispages, 0, 1000);

		poperror();

		qunlock(&palloc.pwait);

		/*
		 * If called from fault and we lost the segment from underneath
		 * don't waste time allocating and freeing a page. Fault will call
		 * newpage again when it has reacquired the segment locks
		 */
		if(dontalloc)
			return 0;

		lock(&palloc);
		palloc.wanted--;
	}

	p = palloc.head;
	if(palloc.head = p->next)		/* = Assign */
		palloc.head->prev = 0;
	else
		palloc.tail = 0;

	palloc.freecount--;
	unlock(&palloc);

	lock(p);
	if(p->ref != 0) {	/* lookpage has priority on steal */
		unlock(p);
		goto retry;
	}
	uncachepage(p);
	p->ref++;
	p->va = va;
	p->modref = 0;
	for(i = 0; i < MAXMACH; i++)
		p->cachectl[i] = PG_NOFLUSH;
	unlock(p);

	if(clear){
		k = kmap(p);
		memset((void*)VA(k), 0, BY2PG);
		kunmap(k);
	}

	return p;
}

int
ispages(void *p)
{
	USED(p);
	return palloc.freecount >= swapalloc.highwater;
}

void
putpage(Page *p)
{
	if(onswap(p)) {
		putswap(p);
		return;
	}

	lock(p);
	if(--p->ref == 0) {
		lock(&palloc);
		if(p->image && p->image != &swapimage) {
			if(palloc.tail) {
				p->prev = palloc.tail;
				palloc.tail->next = p;
			}
			else {
				palloc.head = p;
				p->prev = 0;
			}
			palloc.tail = p;
			p->next = 0;
		}
		else {
			if(palloc.head) {
				p->next = palloc.head;
				palloc.head->prev = p;
			}
			else {
				palloc.tail = p;
				p->next = 0;
			}
			palloc.head = p;
			p->prev = 0;
		}

		palloc.freecount++;	/* Release people waiting for memory */
		unlock(&palloc);
	}
	unlock(p);
}

void
simpleputpage(Page *pg)			/* Always call with palloc locked */
{
	if(pg->ref != 1)
		panic("simpleputpage");
	pg->ref = 0;
	palloc.freecount++;
	if(palloc.head == 0) {
		palloc.head = palloc.tail = pg;
		pg->prev = pg->next = 0;
		return;
	}
	pg->next = palloc.head;
	palloc.head->prev = pg;
	pg->prev = 0;
	palloc.head = pg;
}

void
duppage(Page *p)				/* Always call with p locked */
{
	Page *np;

	/* No dup for swap pages */
	if(p->image == &swapimage) {
		uncachepage(p);	
		return;
	}

	lock(&palloc);
	/* No freelist cache when memory is very low */
	if(palloc.freecount < swapalloc.highwater) {
		unlock(&palloc);
		uncachepage(p);	
		return;
	}

	np = palloc.head;		/* Allocate a new page from freelist */
	if(palloc.head = np->next)	/* = Assign */
		palloc.head->prev = 0;
	else
		palloc.tail = 0;

	if(palloc.tail) {		/* Link back onto tail to give us lru */
		np->prev = palloc.tail;
		palloc.tail->next = np;
		np->next = 0;
		palloc.tail = np;
	}
	else {
		palloc.head = palloc.tail = np;
		np->prev = np->next = 0;
	}

	unlock(&palloc);

	lock(np);				/* Cache the new version */
	if(np->ref != 0) {			/* Stolen by lookpage */
		uncachepage(p);
		unlock(np);
		return;
	}
	
	uncachepage(np);
	np->va = p->va;
	np->daddr = p->daddr;
	copypage(p, np);
	cachepage(np, p->image);
	unlock(np);
	uncachepage(p);
}

void
copypage(Page *f, Page *t)
{
	KMap *ks, *kd;

	ks = kmap(f);
	kd = kmap(t);
	memmove((void*)VA(kd), (void*)VA(ks), BY2PG);
	kunmap(ks);
	kunmap(kd);
}

void
uncachepage(Page *p)			/* Always called with a locked page */
{
	Page **l, *f;

	if(p->image == 0)
		return;

	lock(&palloc.hashlock);
	l = &pghash(p->daddr);
	for(f = *l; f; f = f->hash) {
		if(f == p) {
			*l = p->hash;
			break;
		}
		l = &f->hash;
	}
	unlock(&palloc.hashlock);
	putimage(p->image);
	p->image = 0;
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
	for(f = *l; f; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			*l = f->hash;
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
	for(f = pghash(daddr); f; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			unlock(&palloc.hashlock);

			lock(f);
			if(f->image != i || f->daddr != daddr) {
				unlock(f);
				return 0;
			}

			lock(&palloc);
			if(++f->ref == 1) {
				if(f->prev) 
					f->prev->next = f->next;
				else
					palloc.head = f->next;

				if(f->next)
					f->next->prev = f->prev;
				else
					palloc.tail = f->prev;
				palloc.freecount--;
			}
			unlock(&palloc);

			unlock(f);
			return f;	
		}
	}
	unlock(&palloc.hashlock);
	return 0;
}

Pte*
ptecpy(Pte *old)
{
	Pte *new;
	Page **src, **dst;

	new = ptealloc();
	dst = &new->pages[old->first-old->pages];
	new->first = dst;
	for(src = old->first; src <= old->last; src++, dst++)
		if(*src) {
			if(onswap(*src))
				dupswap(*src);
			else {
				lock(*src);
				(*src)->ref++;
				unlock(*src);
			}
			new->last = dst;
			*dst = *src;
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
	Page **pg, **ptop;

	switch(s->type&SG_TYPE) {
	case SG_PHYSICAL:
		ptop = &p->pages[PTEPERTAB];
		for(pg = p->pages; pg < ptop; pg++)
			if(*pg) {
				(*s->pgfree)(*pg);
				*pg = 0;
			}
		break;
	default:
		for(pg = p->first; pg <= p->last; pg++)
			if(*pg) {
				putpage(*pg);
				*pg = 0;
			}
	}
	free(p);
}
