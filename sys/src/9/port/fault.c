#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

void	faulterror(char*);

int
fault(ulong addr, int read)
{
	Segment *s;
	char *sps;

	sps = u->p->psstate;
	u->p->psstate = "Fault";
	spllo();

	m->pfault++;
	for(;;) {
		s = seg(u->p, addr, 1);
		if(s == 0) {
			u->p->psstate = sps;
			return -1;
		}

		if(!read && (s->type&SG_RONLY)) {
			qunlock(&s->lk);
			u->p->psstate = sps;
			return -1;
		}

		if(fixfault(s, addr, read, 1) == 0)
			break;
	}

	u->p->psstate = sps;
	return 0;
}

int
fixfault(Segment *s, ulong addr, int read, int doputmmu)
{
	ulong mmuphys=0, soff;
	Page **pg, *lkp, *new;
	Pte **p, *etp;
	int type;

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

	case SG_TEXT:
		if(pagedout(*pg)) 		/* Demand load */
			pio(s, addr, soff, pg);
		
		mmuphys = PPN((*pg)->pa) | PTERONLY|PTEVALID;
		(*pg)->modref = PG_REF;
		break;

	case SG_SHDATA:				/* Shared data */
		if(pagedout(*pg))
			pio(s, addr, soff, pg);

		goto done;

	case SG_BSS:
	case SG_SHARED:				/* Zero fill on demand */
	case SG_STACK:	
		if(*pg == 0) {
			new = newpage(1, &s, addr);
			if(s == 0)
				return -1;

			*pg = new;
		}
		/* NO break */

	case SG_DATA:				/* Demand load/pagein/copy on write */
		if(pagedout(*pg))
			pio(s, addr, soff, pg);

		if(type == SG_SHARED)
			goto done;

		if(read && conf.copymode == 0) {
			mmuphys = PPN((*pg)->pa) | PTERONLY|PTEVALID;
			(*pg)->modref |= PG_REF;
			break;
		}

		lkp = *pg;
		lock(lkp);
		if(lkp->ref > 1) {
			unlock(lkp);
			new = newpage(0, &s, addr);
			if(s == 0)
				return -1;
			*pg = new;
			copypage(lkp, *pg);
			putpage(lkp);
		}
		else {
			/* put a duplicate of a text page back onto the free list */
			if(lkp->image)     
				duppage(lkp);	
		
			unlock(lkp);
		}
	done:
		mmuphys = PPN((*pg)->pa) | PTEWRITE|PTEVALID;
		(*pg)->modref = PG_MOD|PG_REF;
		break;

	case SG_PHYSICAL:
		if(*pg == 0)
			*pg = (*s->pgalloc)(addr);

		mmuphys = PPN((*pg)->pa) | PTEWRITE|PTEUNCACHED|PTEVALID;
		(*pg)->modref = PG_MOD|PG_REF;
		break;
	}

	if(s->flushme)
		memset((*pg)->cachectl, PG_TXTFLUSH, sizeof(new->cachectl));

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

	loadrec = *p;
	if(loadrec == 0) {
		daddr = s->fstart+soff;		/* Compute disc address */
		new = lookpage(s->image, daddr);
	}
	else {
		daddr = swapaddr(loadrec);
		new = lookpage(&swapimage, daddr);
		if(new)
			putswap(loadrec);
	}

	if(new) {				/* Page found from cache */
		*p = new;
		return;
	}

	qunlock(&s->lk);

	new = newpage(0, 0, addr);
	k = kmap(new);
	kaddr = (char*)VA(k);
	
	if(loadrec == 0) {			/* This is demand load */
		c = s->image->c;
		while(waserror()) {
			if(strcmp(u->error, Eintr) == 0)
				continue;
			kunmap(k);
			putpage(new);
			faulterror("sys: demand load I/O error");
		}

		ask = s->flen-soff;
		if(ask > BY2PG)
			ask = BY2PG;

		n = (*devtab[c->type].read)(c, kaddr, ask, daddr);
		if(n != ask)
			error(Eioload);
		if(ask < BY2PG)
			memset(kaddr+ask, 0, BY2PG-ask);

		poperror();
		kunmap(k);
		qlock(&s->lk);
		if(*p == 0) { 		/* Someone may have got there first */
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
			faulterror("sys: page in I/O error");
		}

		n = (*devtab[c->type].read)(c, kaddr, BY2PG, daddr);
		if(n != BY2PG)
			error(Eioload);

		poperror();
		kunmap(k);
		qlock(&s->lk);

		if(pagedout(*p)) {
			new->daddr = daddr;
			cachepage(new, &swapimage);
			putswap(*p);
			*p = new;
		}
		else
			putpage(new);
	}
}

void
faulterror(char *s)
{
	if(u->nerrlab) {
		postnote(u->p, 1, s, NDebug);
		error(s);
	}
	pexit(s, 0);
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
			s = seg(u->p, addr, 0);
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

	a = (ulong)s;
	m = BY2PG - (a & (BY2PG-1));
	if(m < n){
		t = vmemchr(s, c, m);
		if(t)
			return t;
		if(!(a & KZERO))
			validaddr(a+m, 1, 0);
		return vmemchr((void*)(a+m), c, n-m);
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
	for(s = p->seg; s < et; s++)
		if(n = *s){
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
