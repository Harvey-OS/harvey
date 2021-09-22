/*
 * caches defined by arm v7 architecture.
 * learn their characteristics for use by cache primitives.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

/*
 * characteristics of cache levels, kept at low, fixed address (CACHECONF).
 * all offsets are known to cache.v7.s.
 */
struct Lowmemcache {
	uint	l1waysh;		/* shifts for set/way registers */
	uint	l1setsh;
	uint	l2waysh;
	uint	l2setsh;
};

Memcache cachel[8];		/* arm arch v7 supports levels 1-7 */

static char *
l1iptype(uint type)
{
	static char *types[] = {
		"reserved",
		"asid-tagged VIVT",
		"VIPT",
		"PIPT",
	};

	if (type >= nelem(types) || types[type] == nil)
		return "GOK";
	return types[type];
}

static char *catype[] = {
	"no,",
	"i,",
	"d,",
	"split i&d,",
	"unified,",
	"gok,",
	"gok,",
	"gok,",
};

void
cacheinfo(int level, Memcache *cp, int ext, int type)
{
	ulong setsways;

	memset(cp, 0, sizeof *cp);
	if (type == Nocache)
		return;
	cp->level = level;
	cp->type = type;
	cp->external = ext;
	if (level == 2) {			/* external PL310 */
		allcachesinfo(cp);
		setsways = cp->setsways;
	} else {
		/* select internal cache level */
		cpwrsc(CpIDcssel, CpID, CpIDidct, CpIDid, (level - 1) << 1);

		setsways = cprdsc(CpIDcsize, CpID, CpIDidct, CpIDid);
		cp->l1ip = cpctget();
		cp->nways = ((setsways >> 3)  & MASK(10)) + 1;
		cp->nsets = ((setsways >> 13) & MASK(15)) + 1;
		cp->log2linelen = (setsways & MASK(2)) + 2 + 2;
	}
	cp->linelen = 1 << cp->log2linelen;
	cp->setsways = setsways;
	cp->setsh = cp->log2linelen;
	cp->waysh = BI2BY*BY2WD - log2(cp->nways);
}

void
allcacheinfo(Memcache *mc)
{
	int n;
	ulong lvl;

	lvl = cprdsc(CpIDcsize, CpID, CpIDidct, CpIDclvlid);
	n = 1;
	for (lvl &= MASK(21); lvl; n++, lvl >>= 3)
		cacheinfo(n, &mc[n], Intcache, lvl & MASK(3));
//	cacheinfo(2, &mc[2], Extcache, Unified);		/* PL310 */
}

/*
 * cache discovery: learn l1 cache characteristics (on cpu 0 only)
 * for use by cache.v7.s ops.  results go into cachel[], then CACHECONF
 * in low memory.
 * can't be called until l2 is on or ready to be on.
 */
void
cacheinit(void)
{
	Lowmemcache *cacheconf;

	allcacheinfo((Memcache *)DRAMADDR(cachel));
	cacheconf = (Lowmemcache *)DRAMADDR(CACHECONF);
	cacheconf->l1waysh = cachel[1].waysh;
	cacheconf->l1setsh = cachel[1].setsh;
	/* on the tegra 2, l2 is unarchitected */
	cacheconf->l2waysh = cachel[2].waysh;
	cacheconf->l2setsh = cachel[2].setsh;
	/* l1 & l2 must have same cache line size, thus same set shift */
	cacheconf->l2setsh = cacheconf->l1setsh;
}

/* dump cache configs from cachel[] */
void
prcachecfg(void)
{
	int cache;
	Memcache *mc;

	for (cache = 1; cache < 8 && cachel[cache].type; cache++) {
		mc = &cachel[cache];
		print("l%d: %s %-10s %2d ways %4d sets %d bytes/line; can W[",
			mc->level, mc->external? "ext": "int", catype[mc->type],
			mc->nways, mc->nsets, mc->linelen);
		if (mc->linelen != CACHELINESZ)
			print(" *should* be %d", CACHELINESZ);
		if (mc->setsways & Cawt)
			print("T");
		if (mc->setsways & Cawb)
			print("B");
		if (mc->setsways & Cawa)
			print("A");
		print("]");
		if (cache == 1)
			print("; l1-i %s", l1iptype((mc->l1ip >> 14) & MASK(2)));
		print("\n");
	}
}
