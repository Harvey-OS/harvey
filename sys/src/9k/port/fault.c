#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

int
fault(uintptr addr, int read)
{
	Segment *s;
	char *sps;
	int i, color;

	if(up == nil)
		panic("fault: nil up");
	if(up->nlocks)
		print("fault: addr %#p: nlocks %d\n", addr, up->nlocks);

	sps = up->psstate;
	up->psstate = "Fault";
	spllo();

	m->pfault++;
	for(i = 0;; i++) {
		s = seg(up, addr, 1);		/* leaves s->lk qlocked if seg != nil */
		if(s == nil) {
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
		if(fixfault(s, addr, read, 1, color) == 0)	/* qunlocks s->lk */
			break;

		/*
		 * See the comment in newpage that describes
		 * how to get here.
		 */
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

void	(*checkaddr)(ulong, Segment *, Page *);
ulong	addr2check;

int
fixfault(Segment *s, uintptr addr, int read, int dommuput, int color)
{
	int type;
	int ref;
	Pte **p, *etp;
	Page **pg, *lkp, *new;
	Page *(*fn)(Segment*, uintptr);
	uintptr mmuphys, pgsize, soff;

	pgsize = segpgsize(s);
	addr &= ~(pgsize-1);
	soff = addr-s->base;
	p = &s->map[soff/s->ptemapmem];
	if(*p == nil)
		*p = ptealloc();

	etp = *p;
	pg = &etp->pages[(soff&(s->ptemapmem-1))>>s->lg2pgsize];
	type = s->type&SG_TYPE;

	if(pg < etp->first)
		etp->first = pg;
	if(pg > etp->last)
		etp->last = pg;

	mmuphys = 0;
	switch(type) {
	default:
		panic("fault");
		break;

	case SG_TEXT: 			/* Demand load */
		if(pagedout(*pg))
			pio(s, addr, soff, pg, color);

		mmuphys = PPN((*pg)->pa) | PTERONLY|PTEVALID;
		(*pg)->modref = PG_REF;
		break;

	case SG_BSS:
	case SG_SHARED:			/* Zero fill on demand */
	case SG_STACK:
		if(*pg == nil) {
			new = newpage(1, s, addr, s->lg2pgsize, color, 1);
			if(new == nil)
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
		if(read && sys->copymode == 0 && s->ref == 1) {
			mmuphys = PPN((*pg)->pa)|PTERONLY|PTEVALID;
			(*pg)->modref |= PG_REF;
			break;
		}

		lkp = *pg;
		lock(lkp);

		ref = lkp->ref;
		if(ref <= 0)
			panic("fault: page->ref %d <= 0", ref);

		if(ref == 1 && lkp->image != nil){
			duppage(lkp);
			ref = lkp->ref;
		}
		unlock(lkp);

		if(ref > 1) {
			new = newpage(0, s, addr, s->lg2pgsize, color, 1);
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
		if(*pg == nil) {
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

		if (checkaddr && addr == addr2check)
			(*checkaddr)(addr, s, *pg);
		mmuphys = PPN((*pg)->pa) | PTEVALID;
		if((s->pseg->attr & SG_RONLY) == 0)
			mmuphys |= PTEWRITE;
		if((s->pseg->attr & SG_CACHED) == 0)
			mmuphys |= PTEUNCACHED;
		(*pg)->modref = PG_MOD|PG_REF;
		break;
	}
	qunlock(&s->lk);

	if(dommuput)
		mmuput(addr, mmuphys, *pg);

	return 0;
}

void
pio(Segment *s, uintptr addr, uintptr soff, Page **p, int color)
{
	Page *new;
	KMap *k;
	Chan *c;
	int n, ask;
	char *kaddr;
	ulong daddr;
	Page *loadrec;
	uintptr pgsize;

	pgsize = segpgsize(s);
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

	new = newpage(0, s, addr, s->lg2pgsize, color, 0);
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
		mmucachectl(*p, PG_TXTFLUSH);
}

/*
 * Called only in a system call
 */
int
okaddr(uintptr addr, long len, int write)
{
	Segment *s;

	/* second test is paranoia only needed on 64-bit systems */
	if(len >= 0 && addr+len >= addr)
		while ((s = seg(up, addr, 0)) != nil &&
		    (!write || !(s->type&SG_RONLY))) {
			if(addr+len <= s->top)
				return 1;
			len -= s->top - addr;
			addr = s->top;
		}
	return 0;
}

void*
validaddr(void* addr, long len, int write)
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
 */
void*
vmemchr(void *s, int c, int n)
{
	int r;
	uintptr a;
	void *t;

	a = PTR2UINT(s);
	while(ROUNDUP(a, PGSZ) != ROUNDUP(a+n-1, PGSZ)){
		/* spans pages; handle this page */
		r = PGSZ - (a & (PGSZ-1));
		t = memchr(UINT2PTR(a), c, r);
		if(t)
			return t;
		a += r;
		n -= r;
		if(!iskaddr(a))
			validaddr(UINT2PTR(a), 1, 0);
	}

	/* fits in one page */
	return memchr(UINT2PTR(a), c, n);
}

Segment*
seg(Proc *p, uintptr addr, int dolock)
{
	Segment **s, **et, *n;

	et = &p->seg[NSEG];
	for(s = p->seg; s < et; s++) {
		n = *s;
		if(n == nil)
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

extern void checkmmu(uintptr, uintmem);
void
checkpages(void)
{
	int checked;
	uintptr addr, off;
	Pte *p;
	Page *pg;
	Segment **sp, **ep, *s;
	uint pgsize;

	if(up == nil)
		return;

	checked = 0;
	for(sp=up->seg, ep=&up->seg[NSEG]; sp<ep; sp++){
		s = *sp;
		if(s == nil)
			continue;
		qlock(&s->lk);
		pgsize = segpgsize(s);
		for(addr=s->base; addr<s->top; addr+=pgsize){
			off = addr - s->base;
			p = s->map[off/s->ptemapmem];
			if(p == nil)
				continue;
			pg = p->pages[(off&(s->ptemapmem-1))/pgsize];
			if(pg == nil || pagedout(pg))
				continue;
			checkmmu(addr, pg->pa);
			checked++;
		}
		qunlock(&s->lk);
	}
       print("%d %s: checked %d page table entries\n", up->pid, up->text, checked);
}
