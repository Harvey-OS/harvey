/*
 * PL310 level 2 cache (non-architectural bag on the side)
 *
 * guaranteed to work incorrectly with default settings; must set Sharovr.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "arm.h"

#define NWAYS(l2p)	((l2p)->auxctl & Assoc16way? 16: 8)
#define L2P		((L2pl310 *)soc.l2cache)

enum {
	L2size		= MB,		/* according to the tegra 2 manual */
	Wayszgran	= 16 * KB,	/* granularity of way sizes */
};

typedef struct L2pl310 L2pl310;
typedef struct Pl310op Pl310op;

struct Pl310op {
	ulong	pa;
	ulong	_pad;
	ulong	indexway;
	ulong	way;
};

struct L2pl310 {
	ulong	id;
	ulong	type;
	uchar	_pad0[0x100 - 0x8];
	ulong	ctl;
	ulong	auxctl;

	uchar	_pad1[0x730 - 0x108];	/* boring regs */
	ulong	sync;
	uchar	_pad2[0x740 - 0x734];
	ulong	r3p0sync;		/* workaround for r3p0 bug */
	uchar	_pad3[0x770 - 0x744];
	Pl310op	inv;			/* inv.indexway doesn't exist */
	uchar	_pad4[0x7b0 - 0x780];
	Pl310op	clean;
	uchar	_pad5[0x7f0 - 0x7c0];
	Pl310op	cleaninv;
	uchar	_pad6[0xc00 - 0x7d0];
	ulong	filtstart;
	ulong	filtend;
	uchar	_pad6[0xf40 - 0xc08];
	ulong	debug;
	/* ... */
};

enum {
	/* ctl bits */
	L2enable = 1,

	/* auxctl bits */
	Ipref	= 1<<29,		/* prefetch enables */
	Dpref	= 1<<28,
	Mbo	= 1<<25,
	/*
	 * shared attribute override: do not transform normal memory
	 * non-cacheable transactions into cacheable no-allocate for reads
	 * and write-through no-write-allocate for writes.
	 * presumably we want this so dma and mmio accesses work as expected.
	 */
	Sharovr	= 1<<22, /* shared attribute override (i.e., work right!) */
	Parity	= 1<<21,
	Waycfgshift= 17,
	Waycfgmask = (1<<3) - 1,
	Assoc16way = 1<<16,
	Exclcachecfg = 1<<12,		/* cache line only in l1 or l2 */
	/*
	 * optim'n to 0 cache lines; must be enabled in a9 too:
	 * set CpAClwr0line on all cpus first.
	 */
	Fullline0= 1<<0,

	/* debug bits */
	Wt	= 1<<1,			/* write-through, not write-back */
	Nolinefill= 1<<0,
};

static Lock l2lock;
static int disallowed;			/* by user: *l2off= in plan9.ini */
static int l2ison;
static ulong waysmask, l2nways, l2nsets;

static Cacheimpl l2cacheimpl;

static ulong
lockl2(void)
{
	ulong s;

	/*
	 * avoid deadlocks during start-up by only locking if more
	 * than one CPU has been active since boot.
	 */
	s = splhi();
	if (conf.nmach > 1)
		ilock(&l2lock);
	return s;
}

static void
unlockl2(ulong s)
{
	if (conf.nmach > 1)
		iunlock(&l2lock);
	splx(s);
}

void
l2pl310sync(void)
{
	if (l2ison) {
		coherence();
		L2P->sync = 0;
	}
	coherence();
}

/* call this first to set sets/ways configuration */
void
l2pl310init(void)
{
	int waysz, nways;
	ulong new;
	L2pl310 *l2p = L2P;
	static int configed;

	l2cache = &l2cacheimpl;
	if (disallowed || getconf("*l2off") != nil) {
		iprint("l2 cache (pl310) off by request\n");
		disallowed = 1;
		return;
	}
	if (l2ison || configed)
		return;
	cachedwb();

	/*
	 * default config is:
	 * l2: ext unified, 8 ways 512 sets 32 bytes/line => 128KB
	 * but the tegra 2 manual says (correctly) there's 1MB available.
	 * ways or way-size may be fixed by hardware; the only way to tell
	 * is to try to change the setting and read it back.
	 */
	l2pl310sync();
	l2cache->inv();

	/* figure out number of ways */
	l2pl310sync();
	l2nways = nways = NWAYS(l2p);
	if (!(l2p->auxctl & Assoc16way)) {
		l2p->auxctl |= Assoc16way;
		coherence();
		l2pl310sync();
		nways = NWAYS(l2p);
		/* l2 was set for 8 ways, asked for 16, got nways */
	}
	waysmask = MASK(nways);

	/* figure out way size (and thus number of sets) */
	waysz = L2size / nways;
	new = l2p->auxctl & ~(Waycfgmask << Waycfgshift) |
		(log2(waysz / Wayszgran) + 1) << Waycfgshift;
	l2p->auxctl = new;
	coherence();
	l2pl310sync();
	l2cache->inv();

	l2nsets = waysz / CACHELINESZ;
	if (l2p->auxctl != new)
		iprint("l2 config %#8.8lux didn't stick; is now %#8.8lux\n",
			new, l2p->auxctl);
	configed++;
}

void
l2pl310info(Memcache *cp)
{
	int pow2;
	ulong waysz;
	L2pl310 *l2p = L2P;

	memset(cp, 0, sizeof *cp);
	if (!l2ison)
		return;

	l2pl310init();
	assert((l2p->id >> 24) == 'A');
	assert(((l2p->id >> 6) & MASK(4)) == 3);
	iprint("pl310 l2 cache rtl %ld", l2p->id & MASK(4));
	switch (l2p->id & MASK(4)) {
	case 4:
		iprint(" (r2p0)");
		break;
	}
	iprint("\n");
	cp->level = 2;
	cp->type = Unified;
	cp->external = Extcache;
	cp->setsways = Cara | Cawa | Cawt | Cawb;
	cp->l1ip = 3<<14;				/* PIPT */
	cp->setsh = cp->waysh = 0;			/* bag on the side */

	cp->linelen = CACHELINESZ;
	cp->log2linelen = log2(CACHELINESZ);

	cp->nways = NWAYS(l2p);
	pow2 = ((l2p->auxctl >> Waycfgshift) & Waycfgmask) - 1;
	if (pow2 < 0)
		pow2 = 0;
	waysz = (1 << pow2) * Wayszgran;
	cp->nsets = waysz / CACHELINESZ;
}

void
l2pl310on(void)
{
	ulong ctl;
	L2pl310 *l2p = L2P;

	if (disallowed || getconf("*l2off") != nil) {
		iprint("l2 cache (pl310) disabled\n");
		disallowed = 1;
		return;
	}
	if (l2ison)
		return;

	l2pl310init();
	l2cache->inv();

	/*
	 * drain l1.  can't turn it off (which would make locks not work)
	 * because doing so makes references below to the l2 registers wedge
	 * the system.
	 */
	cacheuwbinv();
	cacheiinv();

	/*
	 * this is only called once, on cpu0 at startup,
	 * so we don't need locks here.
	 * must do all configuration before enabling l2 cache.
	 */
	l2p->filtend = 0;
	coherence();
	l2p->filtstart = 0;		/* no enable bit */
	l2p->debug = 0;			/* write-back, line fills allowed */
	coherence();

	ctl = l2p->auxctl;
	/* don't change number of sets & ways, but reset all else. */
	ctl &= Waycfgmask << Waycfgshift | Assoc16way;
	ctl |= Sharovr;		/* actually work correctly for a change */
	/* disable all prefetching to assist erratum 751473 */
	ctl &= ~(Ipref | Dpref);
	ctl |= Mbo | Parity | Fullline0;
	l2p->auxctl = ctl;
	coherence();

	l2p->ctl |= L2enable;
	coherence();

	l2ison = 1;
}

/* shut it down unconditionally */
void
l2pl310off(void)
{
	ulong s, s2;
	L2pl310 *l2p = L2P;

	if (!l2ison) {			/* probably start-up */
		l2p->ctl = 0;
		l2p->inv.way = MASK(16); /* bitmap of ways */
		coherence();
		while (l2p->inv.way & waysmask)
			;
		l2pl310sync();
		return;
	}
	s2 = splhi();
	l2cache->wbinv();		/* grabs the lock */
	s = lockl2();
	l2p->ctl &= ~L2enable;
	coherence();
	l2ison = 0;
	unlockl2(s);
	splx(s2);
}

static void
applyrange(ulong *reg, void *ava, int len)
{
	uintptr va, endva;

	if (disallowed || !l2ison)
		return;
	if (len < 0)
		panic("l2cache*se called with negative length");
	l2pl310sync();
	endva = (uintptr)ava + len;
	for (va = (uintptr)ava & ~(CACHELINESZ-1); va < endva;
	     va += CACHELINESZ)
		*reg = PADDR(va);
	l2pl310sync();
}

/*
 * va should be the start of a cache line and bytes should be a multiple
 * of cache line size.  Otherwise, the behaviour is unpredictable.
 */
void
l2pl310invse(void *va, int bytes)
{
	ulong s;

	s = lockl2();
	applyrange(&L2P->inv.pa, va, bytes);
	unlockl2(s);
}

void
l2pl310wbse(void *va, int bytes)
{
	ulong s;

	s = lockl2();
	applyrange(&L2P->clean.pa, va, bytes);
	unlockl2(s);
}

void
l2pl310wbinvse(void *va, int bytes)
{
	ulong s;

	s = lockl2();
	applyrange(&L2P->cleaninv.pa, va, bytes);
	unlockl2(s);
}

void
l2pl310inv(void)
{
	ulong s;
	L2pl310 *l2p = L2P;

	if (disallowed)
		return;

	s = lockl2();
	l2p->inv.way = waysmask;
	coherence();
	while (l2p->inv.way & waysmask)
		;
	l2pl310sync();
	unlockl2(s);
}

/*
 * maximum time seen is 2542µs, typical is 625µs.
 */
void
l2pl310wb(void)
{
	ulong s;
	L2pl310 *l2p = L2P;

	if (disallowed || !l2ison)
		return;

	s = lockl2();
	l2p->clean.way = waysmask;
	coherence();
	while (l2p->clean.way & waysmask)
		;
	l2pl310sync();
	unlockl2(s);
}

/*
 * erratum 727915: background wbinv by way can corrupt data.
 * workaround: use cleaninv.indexway (an atomic op) instead.
 */
void
l2pl310wbinv(void)
{
	int set, way;
	ulong s, s2;
	L2pl310 *l2p = L2P;

	if (disallowed || !l2ison)
		return;

	s2 = splhi();
	l2pl310wb();			/* paranoia */
	s = lockl2();
	coherence();
	for (set = 0; set < l2nsets; set++)
		for (way = 0; way < l2nways; way++) {
			l2p->cleaninv.indexway = way<<28 | set<<5;
			coherence();
		}
	l2pl310sync();
	unlockl2(s);
	splx(s2);
}

static Cacheimpl l2cacheimpl = {
	.info	= l2pl310info,
	.on	= l2pl310on,
	.off	= l2pl310off,

	.inv	= l2pl310inv,
	.wb	= l2pl310wb,
	.wbinv	= l2pl310wbinv,

	.invse	= l2pl310invse,
	.wbse	= l2pl310wbse,
	.wbinvse= l2pl310wbinvse,
};
