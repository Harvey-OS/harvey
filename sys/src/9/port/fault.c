#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

int
fault(ulong addr, int read)
{
	Segment *s;
	char *sps;

	sps = up->psstate;
	up->psstate = "Fault";
	spllo();

	m->pfault++;
	for(;;) {
		s = seg(up, addr, 1);		/* leaves s->lk qlocked if seg != nil */
		if(s == 0) {
			up->psstate = sps;
			return -1;
		}

		if(!read && (s->type&SG_RONLY)) {
			qunlock(&s->lk);
			up->psstate = sps;
			return -1;
		}

		if(fixfault(s, addr, read, 1) == 0)
			break;
	}

	up->psstate = sps;
	return 0;
}

static void
faulterror(char *s, Chan *c, int freemem)
{
	char buf[ERRMAX];

	if(c && c->name){
		snprint(buf, sizeof buf, "%s accessing %s: %s", s, c->name->s, up->errstr);
		s = buf;
	}
	if(up->nerrlab) {
		postnote(up, 1, s, NDebug);
		error(s);
	}
	pexit(s, freemem);
}

int
fixfault(Segment *s, ulong addr, int read, int doputmmu)
{
	int type;
	int ref;
	Pte **p, *etp;
	ulong mmuphys=0, soff;
	Page **pg, *lkp, *new;
	Page *(*fn)(Segment*, ulong);

	addr &= ~(BY2PG-1);
	soff = addr-s->base;
	p = &s->map[soff/PTEMAPMEM];
	if(*p == 0)
		*p = ptealloc();

	etp = *p;
	pg = &etp->pages[(soff&(PTEMAPMEM-1))/BY2PG];
	type = s->type&SG_TYPE;

	if(pg < etp->first)
		etp->first = pg;
	if(pg > etp->last)
		etp->last = pg;

	switch(type) {
	default:
		panic("fault");
		break;

	case SG_TEXT: 			/* Demand load */
		if(pagedout(*pg))
			pio(s, addr, soff, pg);

		mmuphys = PPN((*pg)->pa) | PTERONLY|PTEVALID;
		(*pg)->modref = PG_REF;
		break;

	case SG_BSS:
	case SG_SHARED:			/* Zero fill on demand */
	case SG_STACK:
		if(*pg == 0) {
			new = newpage(1, &s, addr);
			if(s == 0)
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
		if(read && conf.copymode == 0 && s->ref == 1) {
			mmuphys = PPN((*pg)->pa)|PTERONLY|PTEVALID;
			(*pg)->modref |= PG_REF;
			break;
		}

		lkp = *pg;
		lock(lkp);

		if(lkp->image == &swapimage)
			ref = lkp->ref + swapcount(lkp->daddr);
		else
			ref = lkp->ref;
		if(ref > 1) {
			unlock(lkp);

			if(swapfull()){
				qunlock(&s->lk);
				pprint("swap space full\n");
				faulterror(Enoswap, nil, 1);
			}

			new = newpage(0, &s, addr);
			if(s == 0)
				return -1;
			*pg = new;
			copypage(lkp, *pg);
			putpage(lkp);
		}
		else {
			/* save a copy of the original for the image cache */
			if(lkp->image && !swapfull())
				duppage(lkp);

			unlock(lkp);
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
				*pg = new;
			}
		}

		mmuphys = PPN((*pg)->pa) |PTEWRITE|PTEUNCACHED|PTEVALID;
		(*pg)->modref = PG_MOD|PG_REF;
		break;
	}
	qunlock(&s->lk);

	if(doputmmu)
		putmmu(addr, mmuphys, *pg);

	return 0;
}

void
pio(Segment *s, ulong addr, ulong soff, Page **p)
{
	Page *new;
	KMap *k;
	Chan *c;
	int n, ask;
	char *kaddr;
	ulong daddr;
	Page *loadrec;

retry:
	loadrec = *p;
	if(loadrec == 0) {	/* from a text/data image */
		daddr = s->fstart+soff;
		new = lookpage(s->image, daddr);
		if(new != nil) {
			*p = new;
			return;
		}
	}
	else {			/* from a swap image */
		daddr = swapaddr(loadrec);
		new = lookpage(&swapimage, daddr);
		if(new != nil) {
			putswap(loadrec);
			*p = new;
			return;
		}
	}


	qunlock(&s->lk);

	new = newpage(0, 0, addr);
	k = kmap(new);
	kaddr = (char*)VA(k);

	if(loadrec == 0) {			/* This is demand load */
		c = s->image->c;
		while(waserror()) {
			if(strcmp(up->errstr, Eintr) == 0)
				continue;
			kunmap(k);
			putpage(new);
			faulterror("sys: demand load I/O error", c, 0);
		}

		ask = s->flen-soff;
		if(ask > BY2PG)
			ask = BY2PG;

		n = devtab[c->type]->read(c, kaddr, ask, daddr);
		if(n != ask)
			faulterror(Eioload, c, 0);
		if(ask < BY2PG)
			memset(kaddr+ask, 0, BY2PG-ask);

		poperror();
		kunmap(k);
		qlock(&s->lk);

		/*
		 *  race, another proc may have gotten here first while
		 *  s->lk was unlocked
		 */
		if(*p == 0) { 
			new->daddr = daddr;
			cachepage(new, s->image);
			*p = new;
		}
		else
			putpage(new);
	}
	else {				/* This is paged out */
		c = swapimage.c;
		if(waserror()) {
			kunmap(k);
			putpage(new);
			qlock(&s->lk);
			qunlock(&s->lk);
			faulterror("sys: page in I/O error", c, 0);
		}

		n = devtab[c->type]->read(c, kaddr, BY2PG, daddr);
		if(n != BY2PG)
			faulterror(Eioload, c, 0);

		poperror();
		kunmap(k);
		qlock(&s->lk);

		/*
		 *  race, another proc may have gotten here first
		 *  (and the pager may have run on that page) while
		 *  s->lk was unlocked
		 */
		if(*p != loadrec){
			if(!pagedout(*p)){
				/* another process did it for me */
				putpage(new);
				goto done;
			} else {
				/* another process and the pager got in */
				putpage(new);
				goto retry;
			}
		}

		new->daddr = daddr;
		cachepage(new, &swapimage);
		*p = new;
		putswap(loadrec);
	}

done:
	if(s->flushme)
		memset((*p)->cachectl, PG_TXTFLUSH, sizeof((*p)->cachectl));
}

/*
 * Called only in a system call
 */
int
okaddr(ulong addr, ulong len, int write)
{
	Segment *s;

	if((long)len >= 0) {
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
	pprint("suicide: invalid address 0x%lux in sys call pc=0x%lux\n", addr, userpc());
	return 0;
}

void
validaddr(ulong addr, ulong len, int write)
{
	if(!okaddr(addr, len, write))
		pexit("Suicide", 0);
}

/*
 * &s[0] is known to be a valid address.
 */
void*
vmemchr(void *s, int c, int n)
{
	int m;
	char *t;
	ulong a;

    loop:
	a = (ulong)s;
	m = BY2PG - (a & (BY2PG-1));
	if(m < n){
		t = memchr(s, c, m);
		if(t)
			return t;
		if((a & KZERO) != KZERO)
			validaddr(a+m, 1, 0);
		s = (void*)(a+m);
		n -= m;
		goto loop;
	}
	/*
	 * All in one page
	 */
	return memchr(s, c, n);
}

Segment*
seg(Proc *p, ulong addr, int dolock)
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
