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
 * There's no pager process here.
 * One process waiting for memory becomes the pager,
 * during the call to kickpager()
 */

enum
{
	Minpages = 2
};

static QLock	pagerlck;
static struct
{
	uint32_t ntext;
	uint32_t nbig;
	uint32_t nall;
} pstats;

void
swapinit(void)
{
}

void
putswap(Page* p)
{
	panic("putswap");
}

void
dupswap(Page* p)
{
	panic("dupswap");
}

int
swapcount(uint32_t daddr)
{
	USED(daddr);
	return 0;
}

static int
canflush(Proc *p, Segment *s)
{
	int i, x;

	lock(s);
	if(s->ref == 1) {		/* Easy if we are the only user */
		s->ref++;
		unlock(s);
		return canpage(p);
	}
	s->ref++;
	unlock(s);

	/* Now we must do hardwork to ensure all processes which have tlb
	 * entries for this segment will be flushed if we succeed in paging it out
	 */
	for(x = 0; (p = psincref(x)) != nil; x++){
		if(p->state != Dead) {
			for(i = 0; i < NSEG; i++){
				if(p->seg[i] == s && !canpage(p)){
					psdecref(p);
					return 0;
				}
			}
		}
		psdecref(p);
	}
	return 1;
}

static int
pageout(Proc *p, Segment *s)
{
	Proc *up = externup();
	int i, size, n;
	Pte *l;
	Page **pg, *entry;

	if((s->type&SG_TYPE) != SG_LOAD && (s->type&SG_TYPE) != SG_TEXT)
		panic("pageout");

	if(!canqlock(&s->lk))	/* We cannot afford to wait, we will surely deadlock */
		return 0;

	if(s->steal){		/* Protected by /dev/proc */
		qunlock(&s->lk);
		return 0;
	}

	if(!canflush(p, s)){	/* Able to invalidate all tlbs with references */
		qunlock(&s->lk);
		putseg(s);
		return 0;
	}

	if(waserror()){
		qunlock(&s->lk);
		putseg(s);
		return 0;
	}

	/* Pass through the pte tables looking for text memory pages to put */
	n = 0;
	size = s->mapsize;
	for(i = 0; i < size; i++){
		l = s->map[i];
		if(l == 0)
			continue;
		for(pg = l->first; pg < l->last; pg++){
			entry = *pg;
			if(pagedout(entry))
				continue;
			n++;
			if(entry->modref & PG_REF){
				entry->modref &= ~PG_REF;
				continue;
			}
			putpage(*pg);
			*pg = nil;
		}
	}
	poperror();
	qunlock(&s->lk);
	putseg(s);
	return n;
}

static void
pageouttext(int pgszi, int color)
{

	Proc *p;
	Pgsza *pa;
	int i, n, np, x;
	Segment *s;
	int prepaged;

	USED(color);
	pa = &pga.pgsza[pgszi];
	n = x = 0;
	prepaged = 0;

	/*
	 * Try first to steal text pages from non-prepaged processes,
	 * then from anyone.
	 */
Again:
	do{
		if((p = psincref(x)) == nil)
			break;
		np = 0;
		if(p->prepagemem == 0 || prepaged != 0)
		if(p->state != Dead && p->noswap == 0 && canqlock(&p->seglock)){
			for(i = 0; i < NSEG; i++){
				if((s = p->seg[i]) == nil)
					continue;
				if((s->type&SG_TYPE) == SG_TEXT)
					np = pageout(p, s);
			}
			qunlock(&p->seglock);
		}
		/*
		 * else process dead or locked or changing its segments
		 */
		psdecref(p);
		n += np;
		if(np > 0)
			DBG("pager: %d from proc #%d %#p\n", np, x, p);
		x++;
	}while(pa->freecount < Minpages);

	if(pa->freecount < Minpages && prepaged++ == 0)
		goto Again;
}

static void
freepages(int si, int once)
{
	Proc *up = externup();
	Pgsza *pa;
	Page *p;

	for(; si < machp()->npgsz; si++){
		pa = &pga.pgsza[si];
		if(pa->freecount > 0){
			DBG("kickpager() up %#p: releasing %udK pages\n",
				up, machp()->pgsz[si]/KiB);
			lock(&pga);
			if(pa->freecount == 0){
				unlock(&pga);
				continue;
			}
			p = pa->head;
			pageunchain(p);
			unlock(&pga);
			if(p->ref != 0)
				panic("freepages pa %#ullx", p->pa);
			pgfree(p);
			if(once)
				break;
		}
	}
}

static int
tryalloc(int pgszi, int color)
{
	Page *p;

	p = pgalloc(machp()->pgsz[pgszi], color);
	if(p != nil){
		lock(&pga);
		pagechainhead(p);
		unlock(&pga);
		return 0;
	}
	return -1;
}

static int
hascolor(Page *pl, int color)
{
	Page *p;

	lock(&pga);
	for(p = pl; p != nil; p = p->next)
		if(color == NOCOLOR || p->color == color){
			unlock(&pga);
			return 1;
		}
	unlock(&pga);
	return 0;
}

/*
 * Someone couldn't find pages of the given size index and color.
 * (color may be NOCOLOR if the caller is trying to get any page
 * and is desperate).
 * Many processes may be calling this at the same time,
 * The first one operates as a pager and does what it can.
 */
void
kickpager(int pgszi, int color)
{
	Proc *up = externup();
	Pgsza *pa;

	if(DBGFLG>1)
		DBG("kickpager() %#p\n", up);
	if(waserror())
		panic("error in kickpager");
	qlock(&pagerlck);
	pa = &pga.pgsza[pgszi];

	/*
	 * 1. did anyone else release one for us in the mean time?
	 */
	if(hascolor(pa->head, color))
		goto Done;

	/*
	 * 2. try allocating from physical memory
	 */
	tryalloc(pgszi, color);
	if(hascolor(pa->head, color))
		goto Done;

	/*
	 * If pgszi is <= text page size, try releasing text pages.
	 */
	if(machp()->pgsz[pgszi] <= 2*MiB){
		pstats.ntext++;
		DBG("kickpager() up %#p: reclaiming text pages\n", up);
		pageouttext(pgszi, color);
		tryalloc(pgszi, color);
		if(hascolor(pa->head, color)){
			DBG("kickpager() found %uld free\n", pa->freecount);
			goto Done;
		}
	}

	/*
	 * Try releasing memory from bigger pages.
	 */
	pstats.nbig++;
	freepages(pgszi+1, 1);
	tryalloc(pgszi, color);
	if(hascolor(pa->head, color)){
		DBG("kickpager() found %uld free\n", pa->freecount);
		goto Done;
	}

	/*
	 * Really the last resort. Try releasing memory from all pages.
	 */
	pstats.nall++;
	DBG("kickpager() up %#p: releasing all pages\n", up);
	freepages(0, 0);
	tryalloc(pgszi, color);
	if(pa->freecount > 0){
		DBG("kickpager() found %uld free\n", pa->freecount);
		goto Done;
	}

	/*
	 * What else can we do?
	 * But don't panic if we are still trying to get memory of
	 * a particular color and there's none. We'll retry asking
	 * for any color.
	 */
	if(color == NOCOLOR)
		panic("kickpager(): no physical memory");

Done:
	poperror();
	qunlock(&pagerlck);
	if(DBGFLG>1)
		DBG("kickpager() done %#p\n", up);
}

void
pagersummary(void)
{
	print("ntext %uld nbig %uld nall %uld\n",
		pstats.ntext, pstats.nbig, pstats.nall);
	print("no swap\n");
}
