/*
 * page faults, including demand loading
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum {
	Maxloops = 0,		/* if > 0, max. loops in fault() */
};

/*
 * we are supposed to get page faults only when in user mode
 * or processing system calls in kernel mode.
 * in kernel mode, we must not be holding Locks at the time.
 */
int
fault(uintptr addr, int read)
{
	int rv, loops;
	Segment *s;
	char *sps;

	if(up == nil)
		panic("fault: nil up");
	if(up->nlocks)
		iprint("fault: nlocks %d addr %#p\n", up->nlocks, addr);

	sps = up->psstate;
	up->psstate = "Fault";
	spllo();

	m->pfault++;
	rv = -1;
	loops = 0;
	for(;;) {
		s = seg(up, addr, 1);	/* leaves s->lk qlocked iff s != nil */
		if(s == nil)
			break;

		if(!read && (s->type&SG_RONLY)) { /* writing read-only page? */
			qunlock(&s->lk);
			break;
		}

		if(fixfault(s, addr, read, 1) == 0) {
			/* all better; fixfault qunlocked s->lk */
			rv = 0;
			break;
		}
		/*
		 * See the comment in newpage that describes
		 * how to get here.
		 */
		if (Maxloops && ++loops > Maxloops)
			panic("fault: va %#p not fixed after %d iterations",
				addr, Maxloops);
	}
	USED(loops);

	up->psstate = sps;
	return rv;
}

static void
faulterror(char *s, Chan *c, int freemem)
{
	char buf[ERRMAX];

	if(c && c->path){
		snprint(buf, sizeof buf, "%s accessing %s: %s",
			s, c->path->s, up->errstr);
		s = buf;
	}
	if(up->nerrlab) {
		postnote(up, 1, s, NDebug);
		error(s);
	}
	pexit(s, freemem);
}

int
fixfault(Segment *s, uintptr addr, int read, int dommuput)
{
	int type, ref;
	Pte **p, *etp;
	Page **pg, *lkp, *new;
	Page *(*fn)(Segment*, uintptr);
	uintptr mmuphys, pgsize, soff;

	pgsize = 1LL<<s->lg2pgsize;
	addr &= ~(pgsize-1);
	soff = addr-s->base;
	p = &s->map[soff/s->ptemapmem];
	if(*p == 0)
		*p = ptealloc();

	etp = *p;
	pg = &etp->pages[(soff&(s->ptemapmem-1)) >> s->lg2pgsize];
	type = s->type&SG_TYPE;

	if(pg < etp->first)
		etp->first = pg;
	if(pg > etp->last)
		etp->last = pg;

	mmuphys = 0;
	switch(type) {
	default:
		panic("fault: bad segment type %d", type);
		break;

	case SG_TEXT: 			/* Demand load */
		if(pagedout(*pg))
			pio(s, addr, soff, pg);

		mmuphys = PPN((*pg)->pa) | PTERONLY|PTEVALID;
		(*pg)->modref = PG_REF;
		break;

	case SG_STACK:
		/*
		 * if addr is too near stack seg base, iprint a warning.
		 * could this be one of ghostscript's bugs?
		 */
		if (addr >= s->base && addr < s->base + MB)
			iprint("fault: address %#p within a MB of stack base\n",
				addr);
	case SG_BSS:
	case SG_SHARED:			/* Zero fill on demand */
		if(*pg == 0) {
			new = newpage(Zeropage, s, addr, 1);
			if(new == nil)
				return -1;

			*pg = new;
		}
		goto common;

	case SG_DATA:
	common:			/* Demand load/pagein/copy on write */
		if(pagedout(*pg))
			pio(s, addr, soff, pg);

		/*
		 *  It's only possible to copy on write if
		 *  we're the only user of the segment.
		 */
		if(read && sys->copymode == 0 && s->ref == 1) {
			/* read-only unshared page loaded, not copy-on-reference */
			mmuphys = PPN((*pg)->pa)|PTERONLY|PTEVALID;
			(*pg)->modref |= PG_REF;
			break;
		}

		/* write access or copy on reference or shared segment */
		lkp = *pg;
		lock(lkp);

		ref = lkp->ref;
		if(ref <= 0)
			panic("fault: page->ref %d <= 0", ref);

		/* if unshared page, duplicate it */
		if(ref == 1 && lkp->image != nil){
			duppage(lkp);
			ref = lkp->ref;
		}
		unlock(lkp);

		/* if page is now shared, allocate & populate a copy */
		if(ref > 1) {
			new = newpage(Nozeropage, s, addr, 1);
			if(new == nil)
				return -1;
			*pg = new;
			copypage(lkp, *pg);
			putpage(lkp);
		}
		mmuphys = PPN((*pg)->pa) | PTEWRITE | PTEVALID;
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
				new->lg2size = s->pseg->lg2pgsize;
				if(new->lg2size == 0)
					new->lg2size = PGSHFT;	/* TO DO */
				*pg = new;
			}
		}

		mmuphys = PPN((*pg)->pa) |PTEWRITE|PTEUNCACHED|PTEVALID;
		(*pg)->modref = PG_MOD|PG_REF;
		break;
	}
	qunlock(&s->lk);

	if(dommuput)
		mmuput(addr, mmuphys, *pg);

	return 0;
}

void
pio(Segment *s, uintptr addr, uintptr soff, Page **p)
{
	Page *new, *loadrec;
	KMap *k;
	Chan *c;
	int n, ask;
	char *kaddr;
	ulong daddr;
	uintptr pgsize;

	pgsize = 1LL<<s->lg2pgsize;
	loadrec = *p;
	if(!pagedout(*p) || loadrec != nil)
		return;
	/* demand load from a text/data image */
	daddr = s->fstart+soff;
	new = lookpage(s->image, daddr);
	if(new != nil) {
		*p = new;
		return;
	}

	c = s->image->c;
	ask = s->flen-soff;
	if(ask > pgsize)
		ask = pgsize;
	qunlock(&s->lk);

	new = newpage(Nozeropage, s, addr, 0);
	if(new == nil)
		panic("pio");	/* can't happen, s wasn't locked */

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
	if(ask < pgsize)
		memset(kaddr+ask, 0, pgsize-ask);

	poperror();
	kunmap(k);

	qlock(&s->lk);
	/*
	 *  race, another proc may have read the page in while
	 *  s->lk was unlocked
	 */
	if(*p == nil) {
		new->daddr = daddr;
		cachepage(new, s->image);
		*p = new;
	}
	else
		putpage(new);

	if(s->flushme)
		memset((*p)->cachectl, PG_TXTFLUSH, sizeof((*p)->cachectl));
}

/*
 * Called only in a system call
 */
int
okaddr(uintptr addr, long len, int write)
{
	Segment *s;

	if(len >= 0 && (uvlong)addr+len >= addr)
		for(;;) {
			s = seg(up, addr, 0);
			if(s == 0 || (write && (s->type&SG_RONLY)))
				break;

			if((uvlong)addr+len > s->top) {
				len -= s->top - addr;
				addr = s->top;
				continue;
			}
			return 1;
		}
	return 0;
}

void*
validaddr(void* addr, long len, int write)
{
	if(!okaddr((uintptr)addr, len, write)){
		pprint("suicide: invalid address %#p/%ld in sys call pc=%#p\n",
			addr, len, userpc(nil));
		pexit("Suicide", 0);
	}
	return addr;
}

/*
 * &s[0] is known to be a valid address.
 */
void*
vmemchr(void *s, int c, int n)
{
	int r;
	uintptr a;
	void *t;

	a = (uintptr)s;
	while(ROUNDUP(a, PGSZ) != ROUNDUP(a+n-1, PGSZ)){
		/* spans pages; handle this page */
		r = PGSZ - (a & (PGSZ-1));
		t = memchr((void *)a, c, r);
		if(t)
			return t;
		a += r;
		n -= r;
		if((a & KZERO) != KZERO)
			validaddr((void *)a, 1, 0);
	}

	/* fits in one page */
	return memchr((void *)a, c, n);
}

/* if dolock and we return non-nil n, &n->lk will be qlocked. */
Segment*
seg(Proc *p, uintptr addr, int dolock)
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
