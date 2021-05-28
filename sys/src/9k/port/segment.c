#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

Segment *
newseg(int type, uintptr base, uintptr top)
{
	Segment *s;
	int mapsize;
	usize size;

	if((base|top) & (PGSZ-1))
		panic("newseg");

	size = (top-base)/PGSZ;
	if(size > (SEGMAPSIZE*PTEPERTAB))
		error(Enovmem);

	s = smalloc(sizeof(Segment));
	s->ref = 1;
	s->type = type;
	s->base = base;
	s->top = top;
	s->size = size;
	s->lg2pgsize = PGSHFT;
	s->ptemapmem = PTEPERTAB<<s->lg2pgsize;
	s->sema.prev = &s->sema;
	s->sema.next = &s->sema;

	mapsize = HOWMANY(size, PTEPERTAB);
	if(mapsize > nelem(s->ssegmap)){
		mapsize *= 2;
		if(mapsize > SEGMAPSIZE)
			mapsize = SEGMAPSIZE;
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

	if(s == nil)
		return;

	i = s->image;
	if(i != nil) {
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
		if(*pp != nil)
			freepte(s, *pp);

	qunlock(&s->lk);
	if(s->map != s->ssegmap)
		free(s->map);
	if(s->profile != nil)
		free(s->profile);
	free(s);
}

void
relocateseg(Segment *s, uintptr offset)
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
		n = newseg(s->type, s->base, s->top);
		break;

	case SG_BSS:		/* Just copy on write */
		if(share)
			goto sameseg;
		n = newseg(s->type, s->base, s->top);
		break;

	case SG_DATA:		/* Copy on write plus demand load info */
		if(segno == TSEG){
			poperror();
			qunlock(&s->lk);
			return data2txt(s);
		}

		if(share)
			goto sameseg;
		n = newseg(s->type, s->base, s->top);

		incref(s->image);
		n->image = s->image;
		n->fstart = s->fstart;
		n->flen = s->flen;
		n->lg2pgsize = s->lg2pgsize;
		n->color = s->color;
		break;
	}
	size = s->mapsize;
	for(i = 0; i < size; i++)
		if((pte = s->map[i]) != nil)
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
	uintptr soff;
	Page **pg;

	if(s->color == NOCOLOR)
		s->color = p->color;

	if(p->va < s->base || p->va >= s->top)
		panic("segpage");

	soff = p->va - s->base;
	pte = &s->map[soff/s->ptemapmem];
	if(*pte == 0)
		*pte = ptealloc();

	pg = &(*pte)->pages[(soff&(s->ptemapmem-1))>>s->lg2pgsize];
	*pg = p;
	if(pg < (*pte)->first)
		(*pte)->first = pg;
	if(pg > (*pte)->last)
		(*pte)->last = pg;
}

/*
 *  called with s->lk locked
 */
void
mfreeseg(Segment *s, uintptr start, uintptr top)
{
	int i, j, size;
	usize pages;
	uintptr soff;
	Page *pg;
	Page *list;

	pages = (top-start)>>s->lg2pgsize;
	soff = start-s->base;
	j = (soff&(s->ptemapmem-1))>>s->lg2pgsize;

	size = s->mapsize;
	list = nil;
	for(i = soff/s->ptemapmem; i < size; i++) {
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
			 * putpage on the whole list. If swapping were implemented,
			 * paged-out pages can't be in a TLB and could be disposed of here.
			 */
			if(pg != nil){
				pg->next = list;
				list = pg;
				s->map[i]->pages[j] = nil;
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
isoverlap(Proc* p, uintptr va, usize len)
{
	int i;
	Segment *ns;
	uintptr newtop;

	newtop = va+len;
	for(i = 0; i < NSEG; i++) {
		ns = p->seg[i];
		if(ns == nil)
			continue;
		if(newtop > ns->base && va < ns->top)
			return ns;
	}
	return nil;
}

void
segclock(uintptr pc)
{
	Segment *s;

	s = up->seg[TSEG];
	if(s == nil || s->profile == nil)
		return;

	s->profile[0] += TK2MS(1);
	if(pc >= s->base && pc < s->top) {
		pc -= s->base;
		s->profile[pc>>LRESPROF] += TK2MS(1);
	}
}
