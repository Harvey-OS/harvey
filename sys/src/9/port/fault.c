/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * Fault calls fixfault which ends up calling newpage, which
 * might fail to allocate a page for the right color. So, we
 * might enter a loop and retry forever.
 * We first try with the desired color, and then with any
 * other one, if we failed for some time.
 */
int
fault(uintptr_t addr, int read)
{
	Segment *s;
	char *sps;
	int i, color;

if(up->nlocks) print("fault nlocks %d\n", up->nlocks);

	sps = up->psstate;
	up->psstate = "Fault";
	spllo();

	m->pfault++;
	for(i = 0;; i++) {
		s = seg(up, addr, 1);	 /* leaves s->lk qlocked if seg != nil */
		if(s == 0) {
			up->psstate = sps;
			return -1;
		}

		if(!read && (s->type&SG_RONLY)) {
			qunlock(&s->lk);
			up->psstate = sps;
			return -1;
		}

		color = s->color;
		if(i > 3)
			color = -1;
		if(fixfault(s, addr, read, 1, color) == 0)
			break;

		/*
		 * See the comment in newpage that describes
		 * how to get here.
		 */

		if(i > 0 && (i%1000) == 0)
			print("fault: tried %d times\n", i);
	}

	up->psstate = sps;
	return 0;
}

static void
faulterror(char *s, Chan *c, int freemem)
{
	char buf[ERRMAX];

	if(c && c->path){
		snprint(buf, sizeof buf, "%s accessing %s: %s", s, c->path->s, up->errstr);
		s = buf;
	}
	if(up->nerrlab) {
		postnote(up, 1, s, NDebug);
		error(s);
	}
	pexit(s, freemem);
}


int
fixfault(Segment *s, uintptr_t addr, int read, int dommuput, int color)
{
	int type;
	int ref;
	Pte **p, *etp;
	uintptr_t soff;
	uintmem pgsz;
	uint mmuattr;
	Page **pg, *lkp, *new;
	Page *(*fn)(Segment*, uintptr_t);

	pgsz = m->pgsz[s->pgszi];
	addr &= ~(pgsz-1);
	soff = addr-s->base;
	p = &s->map[soff/PTEMAPMEM];
	if(*p == 0)
		*p = ptealloc(s);

	etp = *p;
	pg = &etp->pages[(soff&(PTEMAPMEM-1))/pgsz];
	type = s->type&SG_TYPE;

	if(pg < etp->first)
		etp->first = pg;
	if(pg > etp->last)
		etp->last = pg;

	mmuattr = 0;
	switch(type) {
	default:
		panic("fault");
		break;

	case SG_TEXT: 			/* Demand load */
		if(pagedout(*pg))
			pio(s, addr, soff, pg, color);

		mmuattr = PTERONLY|PTEVALID;
		(*pg)->modref = PG_REF;
		break;

	case SG_BSS:
	case SG_SHARED:			/* Zero fill on demand */
	case SG_STACK:
		if(*pg == 0) {
			new = newpage(1, &s, addr, pgsz, color);
			if(s == 0)
				return -1;

			*pg = new;
		}
		goto common;

	case SG_DATA:
	common:			/* Demand load/pagein/copy on write */
		if(pagedout(*pg))
			pio(s, addr, soff, pg, color);

		/*
		 *  It's only possible to copy on write if
		 *  we're the only user of the segment.
		 */
		if(read && conf.copymode == 0 && s->ref == 1) {
			mmuattr = PTERONLY|PTEVALID;
			(*pg)->modref |= PG_REF;
			break;
		}

		lkp = *pg;
		lock(lkp);

		ref = lkp->ref;
		if(ref > 1) {
			unlock(lkp);

			new = newpage(0, &s, addr, pgsz, color);
			if(s == 0)
				return -1;
			*pg = new;
			copypage(lkp, *pg);
			putpage(lkp);
		}
		else {
			/* save a copy of the original for the image cache */
			if(lkp->image != nil)
				duppage(lkp);

			unlock(lkp);
		}
		mmuattr = PTEWRITE|PTEVALID;
		(*pg)->modref = PG_MOD|PG_REF;
		break;

	case SG_PHYSICAL:
		if(*pg == 0) {
			fn = s->pseg->pgalloc;
			if(fn)
				*pg = (*fn)(s, addr);
			else {
				new = smalloc(sizeof(Page));
				new->va = addr;
				new->pa = s->pseg->pa+(addr-s->base);
				new->ref = 1;
				new->pgszi = s->pseg->pgszi;
				*pg = new;
			}
		}

		mmuattr = PTEVALID;
		if((s->pseg->attr & SG_RONLY) == 0)
			mmuattr |= PTEWRITE;
		if((s->pseg->attr & SG_CACHED) == 0)
			mmuattr |= PTEUNCACHED;
		(*pg)->modref = PG_MOD|PG_REF;
		break;
	}
	qunlock(&s->lk);

	if(dommuput){
		assert(segppn(s, (*pg)->pa) == (*pg)->pa);
		mmuput(addr, *pg, mmuattr);
	}
	return 0;
}

void
pio(Segment *s, uintptr_t addr, uint32_t soff, Page **p, int color)
{
	Page *new;
	KMap *k;
	Chan *c;
	int n, ask;
	uintmem pgsz;
	char *kaddr;
	uint32_t daddr;
	Page *loadrec;

	loadrec = *p;
	daddr = ask = 0;
	c = nil;
	pgsz = m->pgsz[s->pgszi];
	if(loadrec == nil) {	/* from a text/data image */
		daddr = s->fstart+soff;
		new = lookpage(s->image, daddr);
		if(new != nil) {
			*p = new;
			return;
		}

		c = s->image->c;
		ask = s->flen-soff;
		if(ask > pgsz)
			ask = pgsz;
	}
	else
		panic("no swap");

	qunlock(&s->lk);

	new = newpage(0, 0, addr, pgsz, color);
	k = kmap(new);
	kaddr = (char*)VA(k);

	while(waserror()) {
		if(strcmp(up->errstr, Eintr) == 0)
			continue;
		kunmap(k);
		putpage(new);
		faulterror(Eioload, c, 0);
	}

	n = c->dev->read(c, kaddr, ask, daddr);
	if(n != ask)
		faulterror(Eioload, c, 0);
	if(ask < pgsz)
		memset(kaddr+ask, 0, pgsz-ask);

	poperror();
	kunmap(k);

	qlock(&s->lk);
	if(loadrec == nil) {	/* This is demand load */
		/*
		 *  race, another proc may have gotten here first while
		 *  s->lk was unlocked
		 */
		if(*p == nil) {
			new->daddr = daddr;
			cachepage(new, s->image);
			*p = new;
		}
		else
			putpage(new);
	}
	else
		panic("no swap");

	if(s->flushme)
		memset((*p)->cachectl, PG_TXTFLUSH, sizeof((*p)->cachectl));
}

/*
 * Called only in a system call
 */
int
okaddr(uintptr_t addr, int32_t len, int write)
{
	Segment *s;

	if(len >= 0) {
		for(;;) {
			s = seg(up, addr, 0);
			if(s == 0 || (write && (s->type&SG_RONLY)))
				break;

			if(addr+len > s->top) {
				len -= s->top - addr;
				addr = s->top;
				continue;
			}
			return 1;
		}
	}
	return 0;
}

void*
validaddr(void* addr, int32_t len, int write)
{
	if(!okaddr(PTR2UINT(addr), len, write)){
		pprint("suicide: invalid address %#p/%ld in sys call pc=%#p\n",
			addr, len, userpc(nil));
		pexit("Suicide", 0);
	}

	return UINT2PTR(addr);
}

/*
 * &s[0] is known to be a valid address.
 * Assume 2M pages, so it works for both 2M and 1G pages.
 * Note this won't work for 4*KiB pages!
 */
void*
vmemchr(void *s, int c, int n)
{
	int m;
	uintptr_t a;
	void *t;

	a = PTR2UINT(s);
	while(ROUNDUP(a, BIGPGSZ) != ROUNDUP(a+n-1, BIGPGSZ)){
		/* spans pages; handle this page */
		m = BIGPGSZ - (a & (BIGPGSZ-1));
		t = memchr(UINT2PTR(a), c, m);
		if(t)
			return t;
		a += m;
		n -= m;
		if((a & KZERO) != KZERO)
			validaddr(UINT2PTR(a), 1, 0);
	}

	/* fits in one page */
	return memchr(UINT2PTR(a), c, n);
}

Segment*
seg(Proc *p, uintptr_t addr, int dolock)
{
	Segment **s, **et, *n;

	et = &p->seg[NSEG];
	for(s = p->seg; s < et; s++) {
		n = *s;
		if(n == 0)
			continue;
		if(addr >= n->base && addr < n->top) {
			if(dolock == 0)
				return n;

			qlock(&n->lk);
			if(addr >= n->base && addr < n->top)
				return n;
			qunlock(&n->lk);
		}
	}

	return 0;
}
