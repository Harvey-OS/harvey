#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#define pghash(daddr)	palloc.hash[((daddr)>>PGSHFT) & (PGHSIZE-1)]

enum {
	Maxloops = 0,		/* if > 0, max loops in newpage() */
};

struct	Palloc palloc;

static	uintptr	highwater;

/*
 * Split palloc.mem[i] if it's not all of the same color and we can.
 * Return the new end of the known banks.
 */
static int
splitbank(int i, int e)
{
	Pallocmem *pm;
	uintmem psz;

	if(e >= nelem(palloc.mem))		/* trying to split last slot? */
		return 0;
	pm = &palloc.mem[i];
	psz = 0;
	pm->color  = memcolor(pm->base, &psz);
	if(pm->color < 0){
		pm->color = 0;
		if(i > 0)
			pm->color = pm[-1].color;
		return 0;
	}

	if(psz <= PGSZ || psz >= (pm->limit - pm->base))
		return 0;
	if(i+1 < e)			/* slot available in palloc.mem? */
		memmove(pm+2, pm+1, (e-i-1)*sizeof(Pallocmem));
	/* else just step on the next slot */
	pm[1].base = pm->base + psz;
	pm[1].limit = pm->limit;
	pm->limit = pm[1].base;
	DBG("palloc split[%d] col %d %#P %#P -> %#P\n",
		i, pm->color, pm->base, pm[1].limit, pm->limit);

	return 1;
}

/*
 * add the pages of each bank to the palloc.head list of available
 * pages, built from the Page array.
 */
static void
banksavailpages(int e)
{
	int i;
	uintmem pgno, np;
	Page *p;
	Pallocmem *pm;

	palloc.head = p = palloc.pages;
	if (p == nil)
		panic("banksavailpages: nil palloc.pages");
	for(i=0; i<e; i++){
		pm = &palloc.mem[i];
		np = (pm->limit - pm->base)/PGSZ;
		for(pgno=0; pgno < np; pgno++){
			p->prev = p-1;
			p->next = p+1;
			/*
			 * it's important that pgno be uvlong, so that pgno*PGSZ
			 * doesn't otherwise overflow a ulong.
			 */
			p->pa = pm->base + pgno*PGSZ;
			p->lg2size = PGSHFT;
			p->color = pm->color;
			palloc.freecount++;
			p++;
		}
	}
	palloc.tail = p - 1;
	palloc.head->prev = 0;
	palloc.tail->next = 0;

	palloc.user = p - palloc.pages;		/* # of user pages */
	/* there are no more free pages than user pages */
	if (palloc.freecount > palloc.user)
		palloc.freecount = palloc.user;
}

static void
pagesummary(uintmem pgmem)
{
	uintmem ukb, kkb, kmkb, occkb, usedkb, overshoot;

	/* user, kernel, kernel malloc area, total memory in KiB */
	ukb = palloc.user*(PGSZ/KiB);
	kkb = ROUNDUP((uintptr)end - KTZERO, PGSZ)/KiB;
	/* kmkb includes Page array of size pgmem */
	kmkb = pgmem/KiB + ROUNDUP(sys->vmend - (uintptr)end, PGSZ)/KiB;
	occkb = sys->pmoccupied/KiB;

	usedkb = kkb + kmkb + ukb;
	/* allow for integer truncation */
	if (occkb < usedkb && (usedkb - occkb)/KiB > 1) {
		/* used overshoots occupied: reduce user pages & recompute */
		overshoot = ((usedkb - occkb) * KiB) / PGSZ;
		palloc.user -= overshoot;
		palloc.freecount -= overshoot;
		ukb = palloc.user*(PGSZ/KiB);
		usedkb = kkb + kmkb + ukb;
	}

	print("%lldM memory: %lldK+%lldM kernel, %lldM user",
		(vlong)occkb/KiB, (vlong)kkb, (vlong)kmkb/KiB, (vlong)ukb/KiB);
	if (occkb >= usedkb && (occkb - usedkb)/KiB != 0)
		print(", %lldM unused", (vlong)(occkb - usedkb)/KiB);
	print("\n");
}

/*
 * palloc.mem[] banks *could* overlap what we allocate for .pages,
 * so run through banks, looking for one large enough to hold pgmem
 * and above the kernel's allocation space.
 * ksize is the top of the kernel's allocation space.
 * if not found, just malloc (in kseg0).  if found, tear off beginning of bank.
 * all pages have been mapped in meminit.
 *
 * we call this from meminit(), before palloc regions are sub-allocated.
 */
void
allocpages(uintmem ksize, uintmem pgmem, int forcemalloc)
{
	int i, e;
	Pallocmem *pm;

	for(e = 0; e < nelem(palloc.mem); e++)
		if(palloc.mem[e].base == palloc.mem[e].limit)
			break;
	pm = nil;
	for(i=0; i<e; i++){
		pm = &palloc.mem[i];
		/* enough space for Pages array in this bank? */
		if (pm->base >= ksize && pm->limit - pm->base > pgmem)
			break;
	}
	if (i >= e || pm == nil || forcemalloc)	/* no bank big enough? */
		palloc.pages = malloc(pgmem);	/* zeroes its allocation */
	else {
		/* palloc.mem[i] has room for Pages, which may be GBs */
		palloc.pages = KADDR(pm->base);
		pm->base += pgmem;
		memset(palloc.pages, 0, pgmem);	/* since Page contains a Lock */
	}
	if(palloc.pages == nil)
		panic("pageinit: no memory for user-page descriptors");
	DBG("Pages allocated at %#p (%,llud bytes)\n", palloc.pages, pgmem);
}

void
pageinit(void)
{
	int e, i;
	Pallocmem *pm;
	uintmem np;

	for(e = 0; e < nelem(palloc.mem); e++)
		if(palloc.mem[e].base == palloc.mem[e].limit)
			break;

	/*
	 * Split banks if not of the same color
	 * and the array can hold another item.
	 */
	np = 0;
	for(i=0; i<e; i++){
		pm = &palloc.mem[i];
		if(splitbank(i, e))
			e++;

		DBG("palloc[%d] col %d %#P %#P\n",
			i, pm->color, pm->base, pm->limit);
		np += (pm->limit - pm->base)/PGSZ;
	}

	banksavailpages(e);

	/* Paging numbers */
	/* keep at least 5% of user pages free, */
	highwater = (palloc.user*5ULL)/100;
	if(highwater >= 64*MiB/PGSZ)	/* or at least 64MiB, if 5% is more. */
		highwater = 64*MiB/PGSZ;

	/* pgmem space for palloc.pages was allocated in meminit */
	pagesummary(ROUNDUP(np * sizeof(Page), PGSZ));
}

void
pageunchain(Page *p)
{
	if(canlock(&palloc))
		panic("pageunchain (palloc %#p)", &palloc);
	if(p->prev)
		p->prev->next = p->next;
	else
		palloc.head = p->next;
	if(p->next)
		p->next->prev = p->prev;
	else
		palloc.tail = p->prev;
	p->prev = p->next = nil;
	palloc.freecount--;
}

void
pagechaintail(Page *p)
{
	if(canlock(&palloc))
		panic("pagechaintail");
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
	palloc.freecount++;
}

void
pagechainhead(Page *p)
{
	if(canlock(&palloc))
		panic("pagechainhead");
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
	palloc.freecount++;
}

static void
pagewait(uintptr va, int color)
{
	qlock(&palloc.pwait);	/* Hold memory requesters here */

	while(waserror())	/* Ignore interrupts */
		;

	if(palloc.freecount > 0)
		print("nearly ");
	print("out of physical memory for va %#p; "
		"color %d highwater %llud freecount %llud\n",
		va, color, (uvlong)highwater, (uvlong)palloc.freecount);
	pagereclaim(highwater/2);

	tsleep(&palloc.r, ispages, 0, 1000);
	poperror();

	qunlock(&palloc.pwait);
}

/*
 * allocate and return a new page for the given virtual address in segment s;
 * return nil iff s was locked on entry and had to be unlocked to wait for memory.
 */
Page*
newpage(int clear, Segment *s, uintptr va, int locked)
{
	Page *p;
	uchar ct;
	int color, loops;
	uintptr hw;

	/* if free memory is tight (uncommon), wait for some to be freed. */
	lock(&palloc);
	color = getpgcolor(va);
	hw = highwater;
	loops = 0;
	/* assumes that up will be set by the time freecount <= hw */
	while (palloc.freecount <= hw && (!up->kp || palloc.freecount <= 0)) {
		unlock(&palloc);

		if(locked){
			qunlock(&s->lk);
			pagewait(va, color);
			/*
			 * If called from fault and we lost the segment from
			 * underneath don't waste time allocating and freeing
			 * a page.  Fault will call newpage again when it has
			 * reacquired the segment locks.
			 */
			return nil;
		}
		pagewait(va, color);
		if (Maxloops && ++loops > Maxloops)
			panic("newpage: va %#p: no page after waiting %d iterations",
				va, Maxloops);

		lock(&palloc);
	}
	USED(loops);

	/* First try for our colour */
	/*
	 * if we actually ever have systems with multiple memory domains,
	 * we'll probably want to maintain separate lists for each color
	 * so that we don't spend a long time skipping pages of the wrong color.
	 */
	for(p = palloc.head; p; p = p->next)
		if(p->color == color)
			break;

	ct = PG_NOFLUSH;
	if(p == 0) {
		p = palloc.head;
		assert(p);		/* palloc.head */
		p->color = color;
		ct = PG_NEWCOL;
	}

	pageunchain(p);

	lock(p);
	if(p->ref != 0)
		panic("newpage");

	uncachepage(p);
	p->ref++;
	p->va = va;
	p->modref = 0;
	memset(p->cachectl, ct, sizeof(p->cachectl));
	unlock(p);
	unlock(&palloc);

	if(clear) {
		KMap *k;

		k = kmap(p);
		assert(p->lg2size < 32);	/* else need wide memset */
		memset((void*)VA(k), 0, 1LL<<p->lg2size);
		kunmap(k);
	}

	return p;
}

int
ispages(void*)
{
	return palloc.freecount > highwater;
}

void
putpage(Page *p)
{
	lock(&palloc);
	lock(p);

	if(p->ref == 0)
		panic("putpage");

	if(--p->ref > 0) {
		unlock(p);
		unlock(&palloc);
		return;
	}

	if(p->image != nil)
		pagechaintail(p);
	else
		pagechainhead(p);

	if(palloc.r.p != 0)
		wakeup(&palloc.r);

	unlock(p);
	unlock(&palloc);
}

Page*
auxpage(void)
{
	Page *p;

	lock(&palloc);
	p = palloc.head;
	if(palloc.freecount <= highwater) {
		/* memory's tight, don't use it for file cache */
		unlock(&palloc);
		return 0;
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
			panic("duppage");
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

	/* No freelist cache when memory is relatively low */
	if(palloc.freecount <= highwater) {
		unlock(&palloc);
		uncachepage(p);
		return 1;
	}

	color = getpgcolor(p->va);
	for(np = palloc.head; np; np = np->next)
		if(np->color == color)
			break;

	/* No page of the correct color */
	if(np == 0) {
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
	assert(t->lg2size < 32);	/* else need to change memmove */
	memmove((void*)VA(kd), (void*)VA(ks), 1LL<<t->lg2size);
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
	for(f = *l; f; f = f->hash) {
		if(f->image == i && f->daddr == daddr) {
			lock(f);
			if(f->image == i && f->daddr == daddr){
				*l = f->hash;
				putimage(f->image);
				f->image = 0;
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
	for(f = pghash(daddr); f; f = f->hash) {
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

	return 0;
}

uvlong
pagereclaim(int npages)
{
	Page *p;
	uvlong ticks;

	lock(&palloc);
	ticks = fastticks(nil);

	/*
	 * All the pages with images backing them are at the
	 * end of the list (see putpage) so start there and work
	 * backward.
	 */
	for(p = palloc.tail; p && p->image && npages > 0; p = p->prev) {
		if(p->ref == 0 && canlock(p)) {
			if(p->ref == 0) {
				npages--;
				uncachepage(p);
			}
			unlock(p);
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
