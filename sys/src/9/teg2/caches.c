/*
 * operations on all memory data or unified caches, a no-op cache,
 * and an l1-only cache ops cache.
 * i-caches are not handled here.
 *
 * there are only three cache operations that we care about:
 * force cache contents to memory (before dma out or shutdown),
 * ignore cache contents in favour of memory (initialisation, after dma in),
 * both (update page tables and force cpu to read new contents).
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

static Cacheimpl allcaches, nullcaches, l1caches;

void
cachesinfo(Memcache *cp)
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
	allcache = &allcaches;
	nocache = &nullcaches;
	l1cache = &l1caches;
}

void
cachesoff(void)
{
	l2cache->off();
}

void
cachesinvse(void *va, int bytes)
{
	int s;

	s = splhi();
	l2cache->invse(va, bytes);
	cachedinvse(va, bytes);
	splx(s);
}

void
cacheswbse(void *va, int bytes)
{
	int s;

	s = splhi();
	cachedwbse(va, bytes);
	l2cache->wbse(va, bytes);
	splx(s);
}

void
cacheswbinvse(void *va, int bytes)
{
	int s;

	s = splhi();
	cachedwbse(va, bytes);
	l2cache->wbinvse(va, bytes);
	cachedwbinvse(va, bytes);
	splx(s);
}


void
cachesinv(void)
{
	int s;

	s = splhi();
	l2cache->inv();
	cachedinv();
	splx(s);
}

void
cacheswb(void)
{
	int s;

	s = splhi();
	cachedwb();
	l2cache->wb();
	splx(s);
}

void
cacheswbinv(void)
{
	int s;

	s = splhi();
	cachedwb();
	l2cache->wbinv();
	cachedwbinv();
	splx(s);
}

static Cacheimpl allcaches = {
	.info	= cachesinfo,
	.on	= allcacheson,
	.off	= cachesoff,

	.inv	= cachesinv,
	.wb	= cacheswb,
	.wbinv	= cacheswbinv,

	.invse	= cachesinvse,
	.wbse	= cacheswbse,
	.wbinvse= cacheswbinvse,
};


/*
 * null cache ops
 */

void
nullinfo(Memcache *cp)
{
	memset(cp, 0, sizeof *cp);
	cp->log2linelen = 2;
}

void
nullon(void)
{
	nocache = &nullcaches;
}

void
nullop(void)
{
}

void
nullse(void *, int)
{
}

static Cacheimpl nullcaches = {
	.info	= nullinfo,
	.on	= nullon,
	.off	= nullop,

	.inv	= nullop,
	.wb	= nullop,
	.wbinv	= nullop,

	.invse	= nullse,
	.wbse	= nullse,
	.wbinvse= nullse,
};

/*
 * l1-only ops
 */

void
l1cachesinfo(Memcache *)
{
}

void
l1cacheson(void)
{
	l1cache = &l1caches;
}

static Cacheimpl l1caches = {
	.info	= l1cachesinfo,
	.on	= l1cacheson,
	.off	= nullop,

	.inv	= cachedinv,
	.wb	= cachedwb,
	.wbinv	= cachedwbinv,

	.invse	= cachedinvse,
	.wbse	= cachedwbse,
	.wbinvse= cachedwbinvse,
};
