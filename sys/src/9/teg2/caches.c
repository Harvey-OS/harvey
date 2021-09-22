/*
 * operations on all memory data or unified caches.
 * i-caches are not handled here.
 *
 * there are only three cache operations that we care about:
 * force cache contents to memory (before dma out or shutdown),
 * ignore cache contents in favour of memory (initialisation, after dma in),
 * both (update page tables and force cpu to read new contents, mainly for
 * cpu errata).
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void
allcachesinfo(Memcache *cp)
{
	memset(cp, 0, sizeof *cp);
	cp->setsways = Cara | Cawa | Cawt | Cawb;
	cp->l1ip = 3<<14;				/* PIPT */
	cp->log2linelen = log2(CACHELINESZ);
}

void
allcacheson(void)
{
	l2pl310init();
}

void
allcachesoff(void)
{
	if (l2cache)
		l2cache->off();
}

void
allcachesinvse(void *va, int bytes)
{
	int s;

	s = splhi();
	if (l2cache)
		l2cache->invse(va, bytes);
	cachedinvse(va, bytes);
	splx(s);
}

void
allcacheswbse(void *va, int bytes)
{
	int s;

	s = splhi();
	cachedwbse(va, bytes);
	if (l2cache)
		l2cache->wbse(va, bytes);
	splx(s);
}

void
allcacheswbinvse(void *va, int bytes)
{
	int s;

	s = splhi();
	cachedwbse(va, bytes);
	if (l2cache)
		l2cache->wbinvse(va, bytes);
	cachedwbinvse(va, bytes);		/* NB: wbinv */
	splx(s);
}


void
allcachesinv(void)
{
	int s;

	s = splhi();
	if (l2cache)
		l2cache->inv();
	cachedinv();
	splx(s);
}

void
allcacheswb(void)
{
	int s;

	s = splhi();
	cachedwb();
	if (l2cache)
		l2cache->wb();
	splx(s);
}

void
allcacheswbinv(void)
{
	int s;

	s = splhi();
	cachedwb();
	if (l2cache)
		l2cache->wbinv();
	cachedwbinv();			/* NB: wbinv */
	splx(s);
}
