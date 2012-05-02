/*
 * caches defined by arm v7 architecture
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "arm.h"

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
	"none,",
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
		allcache->info(cp);
		setsways = cp->setsways;
	} else {
		/* select internal cache level */
		cpwrsc(CpIDcssel, CpID, CpIDid, 0, (level - 1) << 1);

		setsways = cprdsc(CpIDcsize, CpID, CpIDid, 0);
		cp->l1ip = cpctget();
		cp->nways = ((setsways >> 3)  & MASK(10)) + 1;
		cp->nsets = ((setsways >> 13) & MASK(15)) + 1;
		cp->log2linelen = (setsways & MASK(2)) + 2 + 2;
	}
	cp->linelen = 1 << cp->log2linelen;
	cp->setsways = setsways;
	cp->setsh = cp->log2linelen;
	cp->waysh = 32 - log2(cp->nways);
}

void
allcacheinfo(Memcache *mc)
{
	int n;
	ulong lvl;

	lvl = cprdsc(CpIDcsize, CpID, CpIDidct, CpIDclvlid);
	n = 1;
	for (lvl &= MASK(21); lvl; lvl >>= 3)
		cacheinfo(n, &mc[n], Intcache, lvl & MASK(3));
//	cacheinfo(2, &mc[2], Extcache, Unified);		/* PL310 */
}

void
prcachecfg(void)
{
	int cache;
	Memcache *mc;

	for (cache = 1; cache < 8 && cachel[cache].type; cache++) {
		mc = &cachel[cache];
		iprint("l%d: %s %-10s %2d ways %4d sets %d bytes/line; can W[",
			mc->level, mc->external? "ext": "int", catype[mc->type],
			mc->nways, mc->nsets, mc->linelen);
		if (mc->linelen != CACHELINESZ)
			iprint(" *should* be %d", CACHELINESZ);
		if (mc->setsways & Cawt)
			iprint("T");
		if (mc->setsways & Cawb)
			iprint("B");
		if (mc->setsways & Cawa)
			iprint("A");
		iprint("]");
		if (cache == 1)
			iprint("; l1-i %s", l1iptype((mc->l1ip >> 14) & MASK(2)));
		iprint("\n");
	}
}
