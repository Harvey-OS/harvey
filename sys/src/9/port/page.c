/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum {
	Nstartpgs = 32,
	Nminfree = 3,
	Nfreepgs = 512,
};

typedef struct Pgnd Pgnd;
enum {
	Punused = 0,
	Pused,
	Pfreed,
};

struct Pgnd {
	uintmem pa;
	int sts;
};

#define pghash(daddr) pga.hash[(daddr >> PGSHFT) & (PGHSIZE - 1)]
Pgalloc pga; /* new allocator */

char *
seprintpagestats(char *s, char *e)
{
	int i;

	lock(&pga);
	for(i = 0; i < sys->npgsz; i++)
		if(sys->pgsz[i] != 0)
			s = seprint(s, e, "%uld/%d %dK user pages avail\n",
				    pga.pgsza[i].freecount,
				    pga.pgsza[i].npages.ref, sys->pgsz[i] / KiB);
	unlock(&pga);
	return s;
}

/*
 * Preallocate some pages:
 *  some 2M ones will be used by the first process.
 *  some 1G ones will be allocated for each domain so processes may use them.
 */
void
pageinit(void)
{
	int si, i, color;
	Page *pg;

	pga.userinit = 1;
	DBG("pageinit: npgsz = %d\n", sys->npgsz);
	/*
	 * Don't pre-allocate 4K pages, we are not using them anymore.
	 */
	for(si = 1; si < sys->npgsz; si++) {
		for(i = 0; i < Nstartpgs; i++) {
			if(si < 2)
				color = -1;
			else
				color = i;
			pg = pgalloc(sys->pgsz[si], color);
			if(pg == nil) {
				DBG("pageinit: pgalloc failed. breaking.\n");
				break; /* don't consume more memory */
			}
			DBG("pageinit: alloced pa %#P sz %#ux color %d\n",
			    pg->pa, sys->pgsz[si], pg->color);
			lock(&pga);
			pg->ref = 0;
			pagechainhead(pg);
			unlock(&pga);
		}
	}

	pga.userinit = 0;
}

int
getpgszi(usize size)
{
	int si;

	for(si = 0; si < sys->npgsz; si++)
		if(size == sys->pgsz[si])
			return si;
	print("getpgszi: size %#ulx not found\n", size);
	return -1;
}

Page *
pgalloc(usize size, int color)
{
	Page *pg;
	int si;

	si = getpgszi(size);
	if((pg = malloc(sizeof(Page))) == nil) {
		DBG("pgalloc: malloc failed\n");
		return nil;
	}
	memset(pg, 0, sizeof *pg);
	if((pg->pa = physalloc(size, &color, pg)) == 0) {
		DBG("pgalloc: physalloc failed: size %#ulx color %d\n", size, color);
		free(pg);
		return nil;
	}
	pg->pgszi = si; /* size index */
	incref(&pga.pgsza[si].npages);
	pg->color = color;
	return pg;
}

void
pgfree(Page *pg)
{
	decref(&pga.pgsza[pg->pgszi].npages);
	physfree(pg->pa, sys->pgsz[pg->pgszi]);
	free(pg);
}

void
pageunchain(Page *p)
{
	Pgsza *pa;

	if(canlock(&pga))
		panic("pageunchain");
	pa = &pga.pgsza[p->pgszi];
	if(p->prev)
		p->prev->next = p->next;
	else
		pa->head = p->next;
	if(p->next)
		p->next->prev = p->prev;
	else
		pa->tail = p->prev;
	p->prev = p->next = nil;
	pa->freecount--;
}

void
pagechaintail(Page *p)
{
	Pgsza *pa;

	if(canlock(&pga))
		panic("pagechaintail");
	pa = &pga.pgsza[p->pgszi];
	if(pa->tail) {
		p->prev = pa->tail;
		pa->tail->next = p;
	} else {
		pa->head = p;
		p->prev = 0;
	}
	pa->tail = p;
	p->next = 0;
	pa->freecount++;
}

void
pagechainhead(Page *p)
{
	Pgsza *pa;

	if(canlock(&pga))
		panic("pagechainhead");
	pa = &pga.pgsza[p->pgszi];
	if(pa->head) {
		p->next = pa->head;
		pa->head->prev = p;
	} else {
		pa->tail = p;
		p->next = 0;
	}
	pa->head = p;
	p->prev = 0;
	pa->freecount++;
}

static Page *
findpg(Page *pl, int color)
{
	Page *p;

	for(p = pl; p != nil; p = p->next)
		if(color == NOCOLOR || p->color == color)
			return p;
	return nil;
}
int trip;
/*
 * can be called with up == nil during boot.
 */
Page *
newpage(int clear, Segment **s, uintptr_t va, usize size, int color)
{
	Page *p;
	KMap *k;
	uint8_t ct;
	Pgsza *pa;
	int i, dontalloc, si;
	//	static int once;

	si = getpgszi(size);
	//iprint("(remove this print and diea)newpage, size %x, si %d\n", size, si);
	pa = &pga.pgsza[si];

	lock(&pga);
	/*
	 * Beware, new page may enter a loop even if this loop does not
	 * loop more than once, if the segment is lost and fault calls us
	 * again. Either way, we accept any color if we failed a couple of times.
	 */
	for(i = 0;; i++) {
		if(i > 3)
			color = NOCOLOR;

		/*
		 * 1. try to reuse a free one.
		 */
		p = findpg(pa->head, color);
		if(p != nil)
			break;

		/*
		 * 2. try to allocate a new one from physical memory
		 */
		p = pgalloc(size, color);
		if(p != nil) {
			pagechainhead(p);
			break;
		}

		/*
		 * 3. out of memory, try with the pager.
		 * but release the segment (if any) while in the pager.
		 */
		unlock(&pga);

		dontalloc = 0;
		if(s && *s) {
			qunlock(&((*s)->lk));
			*s = 0;
			dontalloc = 1;
		}

		/*
		 * Try to get any page of the desired color
		 * or any color for NOCOLOR.
		 */
		kickpager(si, color);

		/*
		 * If called from fault and we lost the segment from
		 * underneath don't waste time allocating and freeing
		 * a page. Fault will call newpage again when it has
		 * reacquired the segment locks
		 */
		if(dontalloc)
			return 0;

		lock(&pga);
	}

	assert(p != nil);
	ct = PG_NEWCOL;

	pageunchain(p);

	lock(p);
	if(p->ref != 0)
		panic("newpage pa %#ullx", p->pa);

	uncachepage(p);
	p->ref++;
	p->va = va;
	p->modref = 0;
	for(i = 0; i < nelem(p->cachectl); i++)
		p->cachectl[i] = ct;
	unlock(p);
	unlock(&pga);

	if(clear) {
		k = kmap(p);
		if(VA(k) == 0xfffffe007d800000ULL)
			trip++;
		//	if (trip) die("trip before memset");
		// This will frequently die if we use 3K-1 (3071 -- 0xbff)
		// it will not if we use 3070.
		// The fault is a null pointer deref.
		//memset((void*)VA(k), 0, machp()->pgsz[p->pgszi]);
		// thinking about it, using memset is stupid.
		// Don't get upset about this loop;
		// we make it readable, compilers optimize it.
		int i;
		uint64_t *v = (void *)VA(k);
		if(1)
			for(i = 0; i < sys->pgsz[p->pgszi] / sizeof(*v); i++)
				v[i] = 0;
		//if (trip) die("trip");
		kunmap(k);
	}
	DBG("newpage: va %#p pa %#ullx pgsz %#ux color %d\n",
	    p->va, p->pa, sys->pgsz[p->pgszi], p->color);

	return p;
}

void
putpage(Page *p)
{
	Pgsza *pa;
	int rlse;

	lock(&pga);
	lock(p);

	if(p->ref == 0)
		panic("putpage");

	if(--p->ref > 0) {
		unlock(p);
		unlock(&pga);
		return;
	}
	rlse = 0;
	if(p->image != nil)
		pagechaintail(p);
	else {
		/*
		 * Free pages if we have plenty in the free list.
		 */
		pa = &pga.pgsza[p->pgszi];
		if(pa->freecount > Nfreepgs)
			rlse = 1;
		else
			pagechainhead(p);
	}
	if(pga.r.p != nil)
		wakeup(&pga.r);
	unlock(p);
	if(rlse)
		pgfree(p);
	unlock(&pga);
}

/*
 * Get an auxiliary page.
 * Don't do so if less than Nminfree pages.
 * Only used by cache.
 * The interface must specify page size.
 */
Page *
auxpage(usize size)
{
	Page *p;
	Pgsza *pa;
	int si;

	si = getpgszi(size);
	lock(&pga);
	pa = &pga.pgsza[si];
	p = pa->head;
	if(pa->freecount < Nminfree) {
		unlock(&pga);
		return nil;
	}
	pageunchain(p);
	lock(p);
	if(p->ref != 0)
		panic("auxpage");
	p->ref++;
	uncachepage(p);
	unlock(p);
	unlock(&pga);

	return p;
}

static int dupretries = 15000;

int
duppage(Page *p) /* Always call with p locked */
{
	Proc *up = externup();
	Pgsza *pa;
	Page *np;
	int color;
	int retries;

	retries = 0;
retry:

	if(retries++ > dupretries) {
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

	/*
	 *  normal lock ordering is to call
	 *  lock(&pga) before lock(p).
	 *  To avoid deadlock, we have to drop
	 *  our locks and try again.
	 */
	if(!canlock(&pga)) {
		unlock(p);
		if(up)
			sched();
		lock(p);
		goto retry;
	}

	pa = &pga.pgsza[p->pgszi];
	/* No freelist cache when memory is very low */
	if(pa->freecount < Nminfree) {
		unlock(&pga);
		uncachepage(p);
		return 1;
	}

	color = p->color;
	for(np = pa->head; np; np = np->next)
		if(np->color == color)
			break;

	/* No page of the correct color */
	if(np == 0) {
		unlock(&pga);
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
	 *
	 * What I know is that not doing the pagechaintail, but
	 * doing it at the end, to prevent the race, leads to a
	 * deadlock, even following the pga, pg lock ordering. -nemo
	 */
	lock(np);
	unlock(&pga);

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

	if(f->pgszi != t->pgszi || t->pgszi < 0)
		panic("copypage");
	ks = kmap(f);
	kd = kmap(t);
	memmove((void *)VA(kd), (void *)VA(ks), sys->pgsz[t->pgszi]);
	kunmap(ks);
	kunmap(kd);
}

void
uncachepage(Page *p) /* Always called with a locked page */
{
	Page **l, *f;

	if(p->image == 0)
		return;

	lock(&pga.hashlock);
	l = &pghash(p->daddr);
	for(f = *l; f; f = f->hash) {
		if(f == p) {
			*l = p->hash;
			break;
		}
		l = &f->hash;
	}
	unlock(&pga.hashlock);
	putimage(p->image);
	p->image = 0;
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
	lock(&pga.hashlock);
	p->image = i;
	l = &pghash(p->daddr);
	p->hash = *l;
	*l = p;
	unlock(&pga.hashlock);
}

void
cachedel(Image *i, uint32_t daddr)
{
	Page *f, **l;

	lock(&pga.hashlock);
	l = &pghash(daddr);
	for(f = *l; f; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			lock(f);
			if(f->image == i && f->daddr == daddr) {
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
	unlock(&pga.hashlock);
}

Page *
lookpage(Image *i, uint32_t daddr)
{
	Page *f;

	lock(&pga.hashlock);
	for(f = pghash(daddr); f; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			unlock(&pga.hashlock);

			lock(&pga);
			lock(f);
			if(f->image != i || f->daddr != daddr) {
				unlock(f);
				unlock(&pga);
				return 0;
			}
			if(++f->ref == 1)
				pageunchain(f);
			unlock(&pga);
			unlock(f);

			return f;
		}
	}
	unlock(&pga.hashlock);

	return nil;
}

/*
 * Called from imagereclaim, to try to release Images.
 * The argument shows the preferred image to release pages from.
 * All images will be tried, from lru to mru.
 */
uint64_t
pagereclaim(Image *i)
{
	Page *p;
	uint64_t ticks;

	lock(&pga);
	ticks = fastticks(nil);

	/*
	 * All the pages with images backing them are at the
	 * end of the list (see putpage) so start there and work
	 * backward.
	 */
	for(p = pga.pgsza[0].tail; p && p->image == i; p = p->prev) {
		if(p->ref == 0 && canlock(p)) {
			if(p->ref == 0) {
				uncachepage(p);
			}
			unlock(p);
		}
	}
	ticks = fastticks(nil) - ticks;
	unlock(&pga);

	return ticks;
}

Pte *
ptecpy(Segment *s, Pte *old)
{
	Pte *new;
	Page **src, **dst;

	new = ptealloc(s);
	dst = &new->pages[old->first - old->pages];
	new->first = dst;
	for(src = old->first; src <= old->last; src++, dst++)
		if(*src) {
			if(onswap(*src))
				panic("ptecpy: no swap");
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

Pte *
ptealloc(Segment *s)
{
	Pte *new;

	new = smalloc(sizeof(Pte) + sizeof(Page *) * s->ptepertab);
	new->first = &new->pages[s->ptepertab];
	new->last = new->pages;
	return new;
}

void
freepte(Segment *s, Pte *p)
{
	int ref;
	void (*fn)(Page *);
	Page *pt, **pg, **ptop;

	switch(s->type & SG_TYPE) {
	case SG_PHYSICAL:
		fn = s->pseg->pgfree;
		ptop = &p->pages[s->ptepertab];
		if(fn) {
			for(pg = p->pages; pg < ptop; pg++) {
				if(*pg == 0)
					continue;
				(*fn)(*pg);
				*pg = 0;
			}
			break;
		}
		for(pg = p->pages; pg < ptop; pg++) {
			pt = *pg;
			if(pt == 0)
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
			if(*pg) {
				putpage(*pg);
				*pg = 0;
			}
	}
	free(p);
}
