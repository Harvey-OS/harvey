#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

int	canflush(Proc *p, Segment*);
void	executeio(void);
int	needpages(void*);
void	pageout(Proc *p, Segment*);
void	pagepte(int, Page**);
void	pager(void*);

enum
{
	Maxpages = SEGMAXSIZE/BY2PG,	/* Max # of pageouts per segment pass */
};

	Image 	swapimage;
static 	int	swopen;
static	Page	**iolist;
static	int	ioptr;

void
swapinit(void)
{
	swapalloc.swmap = xalloc(conf.nswap);
	swapalloc.top = &swapalloc.swmap[conf.nswap];
	swapalloc.alloc = swapalloc.swmap;
	swapalloc.last = swapalloc.swmap;
	swapalloc.free = conf.nswap;
	iolist = xalloc(Maxpages*sizeof(Page*));
	if(swapalloc.swmap == 0 || iolist == 0)
		panic("swapinit: not enough memory");
}

ulong
newswap(void)
{
	uchar *look;

	lock(&swapalloc);
	if(swapalloc.free == 0)
		panic("out of swap space");

	look = memchr(swapalloc.last, 0, swapalloc.top-swapalloc.last);
	if(look == 0)
		panic("inconsistent swap");
	
	*look = 1;
	swapalloc.last = look;
	swapalloc.free--;
	unlock(&swapalloc);
	return (look-swapalloc.swmap) * BY2PG; 
}

void
putswap(Page *p)
{
	uchar *idx;

	lock(&swapalloc);
	idx = &swapalloc.swmap[((ulong)p)/BY2PG];
	if(--(*idx) == 0) {
		swapalloc.free++;
		if(idx < swapalloc.last)
			swapalloc.last = idx;
	}
	unlock(&swapalloc);
}

void
dupswap(Page *p)
{
	lock(&swapalloc);
	swapalloc.swmap[((ulong)p)/BY2PG]++;
	unlock(&swapalloc);
}

void
kickpager(void)
{
	static int started;

	if(started)
		wakeup(&swapalloc.r);
	else {
		kproc("pager", pager, 0);
		started = 1;
	}
}

void
pager(void *junk)
{
	Proc *p, *ep;
	Segment *s;
	int i;

	if(waserror()) 
		panic("pager: os error\n");

	USED(junk);
	p = proctab(0);
	ep = &p[conf.nproc];

loop:
	u->p->psstate = "Idle";
	sleep(&swapalloc.r, needpages, 0);

	for(;;) {
		p++;
		if(p >= ep)
			p = proctab(0);

		if(p->state == Dead || p->kp)
			continue;

		if(swapimage.c) {
			for(i = 0; i < NSEG; i++) {
				if(!needpages(junk))
					goto loop;

				if(s = p->seg[i]) {
					switch(s->type&SG_TYPE) {
					default:
						break;
					case SG_TEXT:
						pageout(p, s);
						break;
					case SG_DATA:
					case SG_BSS:
					case SG_STACK:
					case SG_SHARED:
						u->p->psstate = "Pageout";
						pageout(p, s);
						if(ioptr != 0) {
							u->p->psstate = "I/O";
							executeio();
						}
					}
				}
			}
			continue;
		}

		if(palloc.freecount < swapalloc.highwater) {
			if(!cpuserver)
				freebroken();	/* can use the memory */

			/* Emulate the old system if no swap channel */
			print("no physical memory\n");
			tsleep(&swapalloc.r, return0, 0, 1000);
			wakeup(&palloc.r);
		}
	}
	goto loop;
}

void			
pageout(Proc *p, Segment *s)
{
	int type, i;
	Pte *l;
	Page **pg, *entry;

	if(!canqlock(&s->lk))	/* We cannot afford to wait, we will surely deadlock */
		return;

	if(s->steal) {		/* Protected by /dev/proc */
		qunlock(&s->lk);
		return;
	}

	if(!canflush(p, s)) {	/* Able to invalidate all tlbs with references */
		qunlock(&s->lk);
		putseg(s);
		return;
	}

	if(waserror()) {
		qunlock(&s->lk);
		putseg(s);
		return;
	}

	/* Pass through the pte tables looking for memory pages to swap out */
	type = s->type&SG_TYPE;
	for(i = 0; i < SEGMAPSIZE; i++) {
		l = s->map[i];
		if(l == 0)
			continue;
		for(pg = l->first; pg < l->last; pg++) {
			entry = *pg;
			if(pagedout(entry))
				continue;

			if(entry->modref & PG_REF) {
				entry->modref &= ~PG_REF;
				continue;
			}

			pagepte(type, pg);

			if(ioptr >= Maxpages)
				goto out;
		}
	}
out:
	poperror();
	qunlock(&s->lk);
	putseg(s);
	wakeup(&palloc.r);
}

int
canflush(Proc *p, Segment *s)
{
	int i;
	Proc *ep;

	lock(s);
	if(s->ref == 1) {		/* Easy if we are the only user */
		s->ref++;
		unlock(s);
		return canpage(p);
	}
	s->ref++;
	unlock(s);

	/* Now we must do hardwork to ensure all processes which have tlb
	 * entries for this segment will be flushed if we suceed in pageing it out
	 */
	p = proctab(0);
	ep = &p[conf.nproc];
	while(p < ep) {
		if(p->state != Dead) {
			for(i = 0; i < NSEG; i++)
				if(p->seg[i] == s)
					if(!canpage(p))
						return 0;
		}
		p++;
	}
	return 1;						
}

void
pagepte(int type, Page **pg)
{
	ulong daddr;
	Page *outp;

	outp = *pg;
	switch(type) {
	case SG_TEXT:					/* Revert to demand load */
		putpage(outp);
		*pg = 0;
		break;

	case SG_DATA:
	case SG_BSS:
	case SG_STACK:
	case SG_SHARED:
	case SG_SHDATA:
		daddr = newswap();
		cachedel(&swapimage, daddr);
		lock(outp);
		outp->ref++;
		uncachepage(outp);
		unlock(outp);

		/* Enter swap page into cache before segment is unlocked so that
		 * a fault will cause a cache recovery rather than a pagein on a
		 * partially written block.
		 */
		outp->daddr = daddr;
		cachepage(outp, &swapimage);
		*pg = (Page*)(daddr|PG_ONSWAP);

		/* Add me to IO transaction list */
		iolist[ioptr++] = outp;
	}
}

void
executeio(void)
{
	Page *out;
	int i, n;
	Chan *c;
	char *kaddr;
	KMap *k;

	c = swapimage.c;

	for(i = 0; i < ioptr; i++) {
		out = iolist[i];
		k = kmap(out);
		kaddr = (char*)VA(k);

		if(waserror())
			panic("executeio: page out I/O error");

		n = (*devtab[c->type].write)(c, kaddr, BY2PG, out->daddr);
		if(n != BY2PG)
			nexterror();

		kunmap(k);
		poperror();

		/* Free up the page after I/O */
		lock(out);
		out->ref--;
		unlock(out);
		putpage(out);
	}
	ioptr = 0;
}

int
needpages(void *p)
{
	USED(p);
	return palloc.freecount < swapalloc.headroom;
}

void
setswapchan(Chan *c)
{
	if(swapimage.c) {
		if(swapalloc.free != conf.nswap)
			error(Einuse);
		close(swapimage.c);
	}
	incref(c);
	swapimage.c = c;
}
