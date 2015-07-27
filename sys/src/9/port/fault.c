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


#define debug if(0)print

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
	Mach *m = machp();
	Segment *s;
	char *sps;
	int i, color;

if(m->externup->nlocks) print("fault nlocks %d\n", m->externup->nlocks);

	sps = m->externup->psstate;
	m->externup->psstate = "Fault";
	spllo();

	m->pfault++;
	for(i = 0;; i++) {
		s = seg(m->externup, addr, 1);	 /* leaves s->lk qlocked if seg != nil */
		//iprint("seg for %p is %p base %p top %p\n", addr, s, s->base, s->top);
		if(s == 0) {
			//iprint("fault: no seg for %p\n", addr);
			m->externup->psstate = sps;
			return -1;
		}

		if(!read && (s->type&SG_WRITE) == 0) {
			qunlock(&s->lk);
			m->externup->psstate = sps;
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

	m->externup->psstate = sps;
	return 0;
}

static void
faulterror(char *s, Chan *c, int freemem)
{
	Mach *m = machp();
	char buf[ERRMAX];

	if(c && c->path){
		snprint(buf, sizeof buf, "%s accessing %s: %s", s, c->path->s, m->externup->errstr);
		s = buf;
	}

	if(m->externup->nerrlab) {
		postnote(m->externup, 1, s, NDebug);
		error(s);
	}

	pexit(s, freemem);
}

int
fixfault(Segment *s, uintptr_t addr, int read, int dommuput, int color)
{
	Mach *m = machp();
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

	case SG_MMAP:
		print("MMAP fault: req is %p, \n", m->externup->req);
		if(pagedout(*pg) && m->externup->req) {
			print("Fault in mmap'ed page\n");
			// hazardous.
			char f[34];
			snprint(f, sizeof(f), "W%016x%016x", addr, pgsz);
			if (qwrite(m->externup->req, f, sizeof(f)) != sizeof(f))
				error("can't write mmap request");
			/* read in answer here. */
			error("not reading answer yet");
		}
			error("No mmap support yet");
		goto common;

	case SG_DATA:
	case SG_TEXT: 			/* Demand load */
		if(pagedout(*pg))
			pio(s, addr, soff, pg, color);

	common:			/* Demand load/pagein/copy on write */

		if(read){
			/* never copy a non-writeable seg */
			if((s->type & SG_WRITE) == 0){
				mmuattr = PTERONLY|PTEVALID;
				(*pg)->modref = PG_REF;
				break;
			}

			/* delay copy if we are the only user (copy on write when it happens) */
			if(conf.copymode == 0 && s->ref == 1) {

				mmuattr = PTERONLY|PTEVALID;
				(*pg)->modref |= PG_REF;
				break;
			}
		}

		if((s->type & SG_WRITE) == 0)
			error("fixfault: write on read-only\n");

		if((s->type & SG_TYPE) != SG_SHARED){

			lkp = *pg;
			lock(lkp);

			ref = lkp->ref;

			if(ref > 1) {	/* page is shared but segment is not: copy for write */
				int pgref = lkp->ref;
				unlock(lkp);

				debug("fixfault %d: copy on %s, %s(%c%c%c) 0x%p segref %d pgref %d\n",
					m->externup->pid,
					read ? "read " : "write",
					segtypes[s->type & SG_TYPE],
					(s->type & SG_READ) != 0 ? 'r' : '-',
					(s->type & SG_WRITE) != 0 ? 'w' : '-',
					(s->type & SG_EXEC) != 0 ? 'x' : '-',
					addr,
					s->ref,
					pgref
				);
				// No need to zero here as it is copied
				// over.
				new = newpage(0, &s, addr, pgsz, color);
				if(s == 0)
					return -1;
				*pg = new;
				copypage(lkp, *pg);
				putpage(lkp);
			} else {	/* write: don't dirty the image cache */
				if(lkp->image != nil)
					duppage(lkp);

				unlock(lkp);
			}
		}
		mmuattr = PTEVALID|PTEWRITE;
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
		if((s->pseg->attr & SG_WRITE) != 0)
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
	Mach *m = machp();
	Page *newpg;
	KMap *k;
	Chan *c;
	int n, ask;
	uintmem pgsz;
	char *kaddr;
	uint32_t daddr, doff;
	Page *loadrec;

	loadrec = *p;
	daddr = ask = 0;
	c = nil;
	pgsz = m->pgsz[s->pgszi];
	if(loadrec == nil) {	/* from a text/data image */
		// where page begins in file
		daddr = (s->ph.offset + soff) & ~(pgsz-1);
		// where segment begins on the page
		doff = s->ph.offset & (pgsz-1);
		// how much more to read?
		ask = doff+s->ph.filesz - soff;
		if(ask > pgsz)
			ask = pgsz;
		// read offset only if it is the first page
		if(soff > 0)
			doff = 0;

		newpg = lookpage(s->image, daddr);
		if(newpg != nil) {
			*p = newpg;
			return;
		}
		c = s->image->c;
	} else {
		panic("no swap");
	}

	qunlock(&s->lk);

	// For plan 9 a.out format the amount of data
	// we read covered the page; the first parameter
	// of newpage here was 0 -- "don't zero".
	// It is now 1 -- "do zero" because ELF only covers
	// part of the page.
	newpg = newpage(1, 0, addr, pgsz, color);
	k = kmap(newpg);
	kaddr = (char*)VA(k);

	while(waserror()) {
		if(strcmp(m->externup->errstr, Eintr) == 0)
			continue;
		kunmap(k);
		putpage(newpg);
		faulterror(Eioload, c, 0);
	}

	static char *segtypes[]={ "Bad0", "Text", "Data", "Bss", "Stack", "Shared", "Phys" };
	debug(
		"pio %d %s(%c%c%c) addr+doff 0x%p daddr+doff 0x%x ask-doff %d\n",
		m->externup->pid, segtypes[s->type & SG_TYPE],
		(s->type & SG_READ) != 0 ? 'r' : '-',
		(s->type & SG_WRITE) != 0 ? 'w' : '-',
		(s->type & SG_EXEC) != 0 ? 'x' : '-',
		addr+doff, daddr+doff, ask-doff
	);
	n = c->dev->read(c, kaddr+doff, ask-doff, daddr+doff);
	if(n != ask-doff)
		faulterror(Eioload, c, 0);

	poperror();
	kunmap(k);

	qlock(&s->lk);
	if(loadrec == nil) {	/* This is demand load */
		/*
		 *  race, another proc may have gotten here first while
		 *  s->lk was unlocked
		 */
		if(*p == nil) {
			newpg->daddr = daddr;
			cachepage(newpg, s->image);
			*p = newpg;
		} else {
			print("racing on demand load\n");
			putpage(newpg);
		}

	} else {
		panic("no swap");
	}

	if(s->flushme)
		memset((*p)->cachectl, PG_TXTFLUSH, sizeof((*p)->cachectl));
}

/*
 * Called only in a system call
 */
int
okaddr(uintptr_t addr, int32_t len, int write)
{
	Mach *m = machp();
	Segment *s;

	if(len >= 0) {
		for(;;) {
			s = seg(m->externup, addr, 0);
			if(s == 0 || (write && (s->type&SG_WRITE) == 0))
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
	char *t;

	a = PTR2UINT(s);
	while(ROUNDUP(a, BIGPGSZ) != ROUNDUP(a+n-1, BIGPGSZ)){
		/* spans pages; handle this page */
		m = BIGPGSZ - (a & (BIGPGSZ-1));
//		t = memchr(UINT2PTR(a), c, m);
		for(t = UINT2PTR(a); m > 0; m--, t++)
			if (*t == c)
				break;
		if(*t == c)
			return t;
		a += m;
		n -= m;
		/* N.B. You're either going to find the character
		 * or validaddr will error() and bounce you way back
		 * up the call chain. That's why there's no worry about
		 * returning NULL.
		 */
		if((a & KZERO) != KZERO)
			validaddr(UINT2PTR(a), 1, 0);
	}

	/* fits in one page */
	for(t = UINT2PTR(a); n > 0; n--, t++)
		if (*t == c)
			break;
	if(*t != c)
		error("Bogus string");
	return t;
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
