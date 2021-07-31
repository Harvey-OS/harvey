#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

static void	imagereclaim(void);
static void	imagechanreclaim(void);

#include "io.h"

/*
 * Attachable segment types
 */
static Physseg physseg[10] = {
	{ SG_SHARED,	"shared",	0,	SEGMAXSIZE,	0, 	0 },
	{ SG_BSS,	"memory",	0,	SEGMAXSIZE,	0,	0 },
	{ 0,		0,		0,	0,		0,	0 },
};

static Lock physseglock;

#define NFREECHAN	64
#define IHASHSIZE	64
#define ihash(s)	imagealloc.hash[s%IHASHSIZE]
static struct Imagealloc
{
	Lock;
	Image	*free;
	Image	*hash[IHASHSIZE];
	QLock	ireclaim;	/* mutex on reclaiming free images */

	Chan	**freechan;	/* free image channels */
	int	nfreechan;	/* number of free channels */
	int	szfreechan;	/* size of freechan array */
	QLock	fcreclaim;	/* mutex on reclaiming free channels */
}imagealloc;

Segment* (*_globalsegattach)(Proc*, char*);

void
initseg(void)
{
	Image *i, *ie;

	imagealloc.free = xalloc(conf.nimage*sizeof(Image));
	if (imagealloc.free == nil)
		panic("initseg: no memory");
	ie = &imagealloc.free[conf.nimage-1];
	for(i = imagealloc.free; i < ie; i++)
		i->next = i+1;
	i->next = 0;
	imagealloc.freechan = malloc(NFREECHAN * sizeof(Chan*));
	imagealloc.szfreechan = NFREECHAN;
}

Segment *
newseg(int type, ulong base, ulong size)
{
	Segment *s;
	int mapsize;

	if(size > (SEGMAPSIZE*PTEPERTAB))
		error(Enovmem);

	s = smalloc(sizeof(Segment));
	s->ref = 1;
	s->type = type;
	s->base = base;
	s->top = base+(size*BY2PG);
	s->size = size;
	s->sema.prev = &s->sema;
	s->sema.next = &s->sema;

	mapsize = ROUND(size, PTEPERTAB)/PTEPERTAB;
	if(mapsize > nelem(s->ssegmap)){
		mapsize *= 2;
		if(mapsize > (SEGMAPSIZE*PTEPERTAB))
			mapsize = (SEGMAPSIZE*PTEPERTAB);
		s->map = smalloc(mapsize*sizeof(Pte*));
		s->mapsize = mapsize;
	}
	else{
		s->map = s->ssegmap;
		s->mapsize = nelem(s->ssegmap);
	}

	return s;
}

void
putseg(Segment *s)
{
	Pte **pp, **emap;
	Image *i;

	if(s == 0)
		return;

	i = s->image;
	if(i != 0) {
		lock(i);
		lock(s);
		if(i->s == s && s->ref == 1)
			i->s = 0;
		unlock(i);
	}
	else
		lock(s);

	s->ref--;
	if(s->ref != 0) {
		unlock(s);
		return;
	}
	unlock(s);

	qlock(&s->lk);
	if(i)
		putimage(i);

	emap = &s->map[s->mapsize];
	for(pp = s->map; pp < emap; pp++)
		if(*pp)
			freepte(s, *pp);

	qunlock(&s->lk);
	if(s->map != s->ssegmap)
		free(s->map);
	if(s->profile != 0)
		free(s->profile);
	free(s);
}

void
relocateseg(Segment *s, ulong offset)
{
	Page **pg, *x;
	Pte *pte, **p, **endpte;

	endpte = &s->map[s->mapsize];
	for(p = s->map; p < endpte; p++) {
		if(*p == 0)
			continue;
		pte = *p;
		for(pg = pte->first; pg <= pte->last; pg++) {
			if(x = *pg)
				x->va += offset;
		}
	}
}

Segment*
dupseg(Segment **seg, int segno, int share)
{
	int i, size;
	Pte *pte;
	Segment *n, *s;

	SET(n);
	s = seg[segno];

	qlock(&s->lk);
	if(waserror()){
		qunlock(&s->lk);
		nexterror();
	}
	switch(s->type&SG_TYPE) {
	case SG_TEXT:		/* New segment shares pte set */
	case SG_SHARED:
	case SG_PHYSICAL:
		goto sameseg;

	case SG_STACK:
		n = newseg(s->type, s->base, s->size);
		break;

	case SG_BSS:		/* Just copy on write */
		if(share)
			goto sameseg;
		n = newseg(s->type, s->base, s->size);
		break;

	case SG_DATA:		/* Copy on write plus demand load info */
		if(segno == TSEG){
			poperror();
			qunlock(&s->lk);
			return data2txt(s);
		}

		if(share)
			goto sameseg;
		n = newseg(s->type, s->base, s->size);

		incref(s->image);
		n->image = s->image;
		n->fstart = s->fstart;
		n->flen = s->flen;
		break;
	}
	size = s->mapsize;
	for(i = 0; i < size; i++)
		if(pte = s->map[i])
			n->map[i] = ptecpy(pte);

	n->flushme = s->flushme;
	if(s->ref > 1)
		procflushseg(s);
	poperror();
	qunlock(&s->lk);
	return n;

sameseg:
	incref(s);
	poperror();
	qunlock(&s->lk);
	return s;
}

void
segpage(Segment *s, Page *p)
{
	Pte **pte;
	ulong off;
	Page **pg;

	if(p->va < s->base || p->va >= s->top)
		panic("segpage");

	off = p->va - s->base;
	pte = &s->map[off/PTEMAPMEM];
	if(*pte == 0)
		*pte = ptealloc();

	pg = &(*pte)->pages[(off&(PTEMAPMEM-1))/BY2PG];
	*pg = p;
	if(pg < (*pte)->first)
		(*pte)->first = pg;
	if(pg > (*pte)->last)
		(*pte)->last = pg;
}

Image*
attachimage(int type, Chan *c, ulong base, ulong len)
{
	Image *i, **l;

	/* reclaim any free channels from reclaimed segments */
	if(imagealloc.nfreechan)
		imagechanreclaim();

	lock(&imagealloc);

	/*
	 * Search the image cache for remains of the text from a previous
	 * or currently running incarnation
	 */
	for(i = ihash(c->qid.path); i; i = i->hash) {
		if(c->qid.path == i->qid.path) {
			lock(i);
			if(eqqid(c->qid, i->qid) &&
			   eqqid(c->mqid, i->mqid) &&
			   c->mchan == i->mchan &&
			   c->type == i->type) {
				goto found;
			}
			unlock(i);
		}
	}

	/*
	 * imagereclaim dumps pages from the free list which are cached by image
	 * structures. This should free some image structures.
	 */
	while(!(i = imagealloc.free)) {
		unlock(&imagealloc);
		imagereclaim();
		sched();
		lock(&imagealloc);
	}

	imagealloc.free = i->next;

	lock(i);
	incref(c);
	i->c = c;
	i->type = c->type;
	i->qid = c->qid;
	i->mqid = c->mqid;
	i->mchan = c->mchan;
	l = &ihash(c->qid.path);
	i->hash = *l;
	*l = i;
found:
	unlock(&imagealloc);

	if(i->s == 0) {
		/* Disaster after commit in exec */
		if(waserror()) {
			unlock(i);
			pexit(Enovmem, 1);
		}
		i->s = newseg(type, base, len);
		i->s->image = i;
		i->ref++;
		poperror();
	}
	else
		incref(i->s);

	return i;
}

static struct {
	int	calls;			/* times imagereclaim was called */
	int	loops;			/* times the main loop was run */
	uvlong	ticks;			/* total time in the main loop */
	uvlong	maxt;			/* longest time in main loop */
} irstats;

static void
imagereclaim(void)
{
	int n;
	Page *p;
	uvlong ticks;

	irstats.calls++;
	/* Somebody is already cleaning the page cache */
	if(!canqlock(&imagealloc.ireclaim))
		return;

	lock(&palloc);
	ticks = fastticks(nil);
	n = 0;
	/*
	 * All the pages with images backing them are at the
	 * end of the list (see putpage) so start there and work
	 * backward.
	 */
	for(p = palloc.tail; p && p->image && n<1000; p = p->prev) {
		if(p->ref == 0 && canlock(p)) {
			if(p->ref == 0) {
				n++;
				uncachepage(p);
			}
			unlock(p);
		}
	}
	ticks = fastticks(nil) - ticks;
	unlock(&palloc);
	irstats.loops++;
	irstats.ticks += ticks;
	if(ticks > irstats.maxt)
		irstats.maxt = ticks;
	//print("T%llud+", ticks);
	qunlock(&imagealloc.ireclaim);
}

/*
 *  since close can block, this has to be called outside of
 *  spin locks.
 */
static void
imagechanreclaim(void)
{
	Chan *c;

	/* Somebody is already cleaning the image chans */
	if(!canqlock(&imagealloc.fcreclaim))
		return;

	/*
	 * We don't have to recheck that nfreechan > 0 after we
	 * acquire the lock, because we're the only ones who decrement 
	 * it (the other lock contender increments it), and there's only
	 * one of us thanks to the qlock above.
	 */
	while(imagealloc.nfreechan > 0){
		lock(&imagealloc);
		imagealloc.nfreechan--;
		c = imagealloc.freechan[imagealloc.nfreechan];
		unlock(&imagealloc);
		cclose(c);
	}

	qunlock(&imagealloc.fcreclaim);
}

void
putimage(Image *i)
{
	Chan *c, **cp;
	Image *f, **l;

	if(i->notext)
		return;

	lock(i);
	if(--i->ref == 0) {
		l = &ihash(i->qid.path);
		mkqid(&i->qid, ~0, ~0, QTFILE);
		unlock(i);
		c = i->c;

		lock(&imagealloc);
		for(f = *l; f; f = f->hash) {
			if(f == i) {
				*l = i->hash;
				break;
			}
			l = &f->hash;
		}

		i->next = imagealloc.free;
		imagealloc.free = i;

		/* defer freeing channel till we're out of spin lock's */
		if(imagealloc.nfreechan == imagealloc.szfreechan){
			imagealloc.szfreechan += NFREECHAN;
			cp = malloc(imagealloc.szfreechan*sizeof(Chan*));
			if(cp == nil)
				panic("putimage");
			memmove(cp, imagealloc.freechan, imagealloc.nfreechan*sizeof(Chan*));
			free(imagealloc.freechan);
			imagealloc.freechan = cp;
		}
		imagealloc.freechan[imagealloc.nfreechan++] = c;
		unlock(&imagealloc);

		return;
	}
	unlock(i);
}

long
ibrk(ulong addr, int seg)
{
	Segment *s, *ns;
	ulong newtop, newsize;
	int i, mapsize;
	Pte **map;

	s = up->seg[seg];
	if(s == 0)
		error(Ebadarg);

	if(addr == 0)
		return s->base;

	qlock(&s->lk);

	/* We may start with the bss overlapping the data */
	if(addr < s->base) {
		if(seg != BSEG || up->seg[DSEG] == 0 || addr < up->seg[DSEG]->base) {
			qunlock(&s->lk);
			error(Enovmem);
		}
		addr = s->base;
	}

	newtop = PGROUND(addr);
	newsize = (newtop-s->base)/BY2PG;
	if(newtop < s->top) {
		/*
		 * do not shrink a segment shared with other procs, as the
		 * to-be-freed address space may have been passed to the kernel
		 * already by another proc and is past the validaddr stage.
		 */
		if(s->ref > 1){
			qunlock(&s->lk);
			error(Einuse);
		}
		mfreeseg(s, newtop, (s->top-newtop)/BY2PG);
		s->top = newtop;
		s->size = newsize;
		qunlock(&s->lk);
		flushmmu();
		return 0;
	}

	for(i = 0; i < NSEG; i++) {
		ns = up->seg[i];
		if(ns == 0 || ns == s)
			continue;
		if(newtop >= ns->base && newtop < ns->top) {
			qunlock(&s->lk);
			error(Esoverlap);
		}
	}

	if(newsize > (SEGMAPSIZE*PTEPERTAB)) {
		qunlock(&s->lk);
		error(Enovmem);
	}
	mapsize = ROUND(newsize, PTEPERTAB)/PTEPERTAB;
	if(mapsize > s->mapsize){
		map = smalloc(mapsize*sizeof(Pte*));
		memmove(map, s->map, s->mapsize*sizeof(Pte*));
		if(s->map != s->ssegmap)
			free(s->map);
		s->map = map;
		s->mapsize = mapsize;
	}

	s->top = newtop;
	s->size = newsize;
	qunlock(&s->lk);
	return 0;
}

/*
 *  called with s->lk locked
 */
void
mfreeseg(Segment *s, ulong start, int pages)
{
	int i, j, size;
	ulong soff;
	Page *pg;
	Page *list;

	soff = start-s->base;
	j = (soff&(PTEMAPMEM-1))/BY2PG;

	size = s->mapsize;
	list = nil;
	for(i = soff/PTEMAPMEM; i < size; i++) {
		if(pages <= 0)
			break;
		if(s->map[i] == 0) {
			pages -= PTEPERTAB-j;
			j = 0;
			continue;
		}
		while(j < PTEPERTAB) {
			pg = s->map[i]->pages[j];
			/*
			 * We want to zero s->map[i]->page[j] and putpage(pg),
			 * but we have to make sure other processors flush the
			 * entry from their TLBs before the page is freed.
			 * We construct a list of the pages to be freed, zero
			 * the entries, then (below) call procflushseg, and call
			 * putpage on the whole list.
			 *
			 * Swapped-out pages don't appear in TLBs, so it's okay
			 * to putswap those pages before procflushseg.
			 */
			if(pg){
				if(onswap(pg))
					putswap(pg);
				else{
					pg->next = list;
					list = pg;
				}
				s->map[i]->pages[j] = 0;
			}
			if(--pages == 0)
				goto out;
			j++;
		}
		j = 0;
	}
out:
	/* flush this seg in all other processes */
	if(s->ref > 1)
		procflushseg(s);

	/* free the pages */
	for(pg = list; pg != nil; pg = list){
		list = list->next;
		putpage(pg);
	}
}

Segment*
isoverlap(Proc *p, ulong va, int len)
{
	int i;
	Segment *ns;
	ulong newtop;

	newtop = va+len;
	for(i = 0; i < NSEG; i++) {
		ns = p->seg[i];
		if(ns == 0)
			continue;
		if((newtop > ns->base && newtop <= ns->top) ||
		   (va >= ns->base && va < ns->top))
			return ns;
	}
	return nil;
}

int
addphysseg(Physseg* new)
{
	Physseg *ps;

	/*
	 * Check not already entered and there is room
	 * for a new entry and the terminating null entry.
	 */
	lock(&physseglock);
	for(ps = physseg; ps->name; ps++){
		if(strcmp(ps->name, new->name) == 0){
			unlock(&physseglock);
			return -1;
		}
	}
	if(ps-physseg >= nelem(physseg)-2){
		unlock(&physseglock);
		return -1;
	}

	*ps = *new;
	unlock(&physseglock);

	return 0;
}

int
isphysseg(char *name)
{
	Physseg *ps;
	int rv = 0;

	lock(&physseglock);
	for(ps = physseg; ps->name; ps++){
		if(strcmp(ps->name, name) == 0){
			rv = 1;
			break;
		}
	}
	unlock(&physseglock);
	return rv;
}

ulong
segattach(Proc *p, ulong attr, char *name, ulong va, ulong len)
{
	int sno;
	Segment *s, *os;
	Physseg *ps;

	if(va != 0 && va >= USTKTOP)
		error(Ebadarg);

	validaddr((ulong)name, 1, 0);
	vmemchr(name, 0, ~0);

	for(sno = 0; sno < NSEG; sno++)
		if(p->seg[sno] == nil && sno != ESEG)
			break;

	if(sno == NSEG)
		error(Enovmem);

	/*
	 *  first look for a global segment with the
	 *  same name
	 */
	if(_globalsegattach != nil){
		s = (*_globalsegattach)(p, name);
		if(s != nil){
			p->seg[sno] = s;
			return s->base;
		}
	}

	len = PGROUND(len);
	if(len == 0)
		error(Ebadarg);

	/*
	 * Find a hole in the address space.
	 * Starting at the lowest possible stack address - len,
	 * check for an overlapping segment, and repeat at the
	 * base of that segment - len until either a hole is found
	 * or the address space is exhausted.  Ensure that we don't
	 * map the zero page.
	 */
	if(va == 0) {
		for (os = p->seg[SSEG]; os != nil; os = isoverlap(p, va, len)) {
			va = os->base;
			if(len >= va)
				error(Enovmem);
			va -= len;
		}
		va &= ~(BY2PG-1);
	} else {
		va &= ~(BY2PG-1);
		if(va == 0 || va >= USTKTOP)
			error(Ebadarg);
	}

	if(isoverlap(p, va, len) != nil)
		error(Esoverlap);

	for(ps = physseg; ps->name; ps++)
		if(strcmp(name, ps->name) == 0)
			goto found;

	error(Ebadarg);
found:
	if(len > ps->size)
		error(Enovmem);

	attr &= ~SG_TYPE;		/* Turn off what is not allowed */
	attr |= ps->attr;		/* Copy in defaults */

	s = newseg(attr, va, len/BY2PG);
	s->pseg = ps;
	p->seg[sno] = s;

	return va;
}

void
pteflush(Pte *pte, int s, int e)
{
	int i;
	Page *p;

	for(i = s; i < e; i++) {
		p = pte->pages[i];
		if(pagedout(p) == 0)
			memset(p->cachectl, PG_TXTFLUSH, sizeof(p->cachectl));
	}
}

long
syssegflush(ulong *arg)
{
	Segment *s;
	ulong addr, l;
	Pte *pte;
	int chunk, ps, pe, len;

	addr = arg[0];
	len = arg[1];

	while(len > 0) {
		s = seg(up, addr, 1);
		if(s == 0)
			error(Ebadarg);

		s->flushme = 1;
	more:
		l = len;
		if(addr+l > s->top)
			l = s->top - addr;

		ps = addr-s->base;
		pte = s->map[ps/PTEMAPMEM];
		ps &= PTEMAPMEM-1;
		pe = PTEMAPMEM;
		if(pe-ps > l){
			pe = ps + l;
			pe = (pe+BY2PG-1)&~(BY2PG-1);
		}
		if(pe == ps) {
			qunlock(&s->lk);
			error(Ebadarg);
		}

		if(pte)
			pteflush(pte, ps/BY2PG, pe/BY2PG);

		chunk = pe-ps;
		len -= chunk;
		addr += chunk;

		if(len > 0 && addr < s->top)
			goto more;

		qunlock(&s->lk);
	}
	flushmmu();
	return 0;
}

void
segclock(ulong pc)
{
	Segment *s;

	s = up->seg[TSEG];
	if(s == 0 || s->profile == 0)
		return;

	s->profile[0] += TK2MS(1);
	if(pc >= s->base && pc < s->top) {
		pc -= s->base;
		s->profile[pc>>LRESPROF] += TK2MS(1);
	}
}

