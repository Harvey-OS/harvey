/*
 * Xilinx XPS Central DMA Controller v2.00b.
 *
 * addresses not need be aligned and lengths need not
 * be multiples of 4, though `keyhole' operations (Sinc and Dinc
 * are not both set) still operate on longs.
 *
 * dma transfers bypass all the usual processor (I & D) caches.
 * memory-to-memory dma on the virtex 5 takes about 1µs per byte.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"

enum {
	Ndma = 2,
	Rst = 0xa,		/* magic rst value */

	/* dmacr */
	Sinc	= 1<<31,	/* source increment */
	Dinc	= 1<<30,	/* dest increment */
	/* low bits are bytes per transfer; use 4 */

	/* dmasr */
	Dmabsy	= 1<<31,	/* in progress */
	Dbe	= 1<<30,	/* dma bus error */

	/* isr */
	De	= 1<<1,		/* dma error */
	Dd	= 1<<0,		/* dma done */

	/* ier */
	Deie	= 1<<1,		/* dma error interrupt enable */
	Ddie	= 1<<0,		/* dma done interrupt enable */
};

struct Dma {
	ulong	rst;	/* sw reset (wo) */
	ulong	dmacr;	/* control */
	ulong	sa;	/* source addr */
	ulong	da;	/* dest addr */
	ulong	length;	/* bytes to transfer */
	ulong	dmasr;	/* status (ro) */
	ulong	pad[5];
	ulong	isr;	/* interrupt status (read, toggle on write) */
	ulong	ier;	/* interrupt enable */
};

typedef struct {
	Dma	*regs;
	Lock;
	Rendez;
	int	done;		/* set true briefly upon dma completion */
	int	busy;		/* dma channel in use? */
	int	chan;

	int	flags;
	void	*dest;
	uintptr	len;

	char	name[16];
	void	(*donenotify)(int);
} Ctlr;

int dmaready;			/* flag: ready for general use? */

static int dmainited;
static Ctlr dmas[Ndma];

static	void	dmaprocinit(Ctlr *);
static int	dmaintr(ulong bit);

static void
dmadump(Ctlr *ctlr, Dma *dma)
{
	if (!dmaready)
		return;
	iprint("len\t%#lux\n",		ctlr->len);
	iprint("\n");
	iprint("dmacr\t%#lux\n",	dma->dmacr);
	iprint("sa\t%#lux\n",		dma->sa);
	iprint("da\t%#lux\n",		dma->da);
	iprint("length\t%#lux\n",	dma->length);
	iprint("dmasr\t%#lux\n",	dma->dmasr);
	iprint("isr\t%#lux\n",		dma->isr);
	iprint("ier\t%#lux\n",		dma->ier);
}

static void
dmaresetdump(ulong bit, Dma *dma)
{
	Ctlr *ctlr;

	if (!dmaready)
		return;
	ctlr = (bit == Intdma? dmas: &dmas[1]);
	iprint("\n%s chan %d reset dump:\n", ctlr->name, ctlr->chan);
	dmadump(ctlr, dma);
}

static void
dmareset(ulong bit)
{
	Dma *dma;

	dma = (Dma *)(bit == Intdma? Dmactlr: Dmactlr2);
//	dmaresetdump(bit, dma);
	if (dmaready)			/* normal case */
		dmaintr(bit);		/* try this instead of a reset */
	else {				/* early: just whack it */
		dma->ier = 0;
		barriers();
		dma->rst = Rst;
		barriers();
		while (dma->dmasr & Dmabsy)
			;
	}
}

static int
dmaisdone(void *vctlr)
{
	return ((Ctlr *)vctlr)->done;
}

/*
 * we wait for wakeups from dmaintr so that ctlr->donenotify can
 * be called from a non-interrupt context.
 */
static void
dmawaitproc(void *vctlr)
{
	int n;
	Ctlr *ctlr;

	ctlr = vctlr;
	n = ctlr - dmas;
	for (;;) {
		sleep(ctlr, dmaisdone, ctlr);
		ilock(ctlr);
		ctlr->done = 0;		/* acknowledge previous transfer done */
		barriers();
		iunlock(ctlr);
		ctlr->donenotify(n);
	}
}

static int
dmaintr(ulong bit)
{
	ulong sts;
	Ctlr *ctlr;
	Dma *dma;

	if (bit == Intdma)
		ctlr = &dmas[0];
	else if (bit == Intdma2)
		ctlr = &dmas[1];
	else {
		SET(ctlr);
		panic("dmaintr: bit %#lux not Intdma*", bit);
	}

	if (!dmainited)
		panic("dmaintr: dma not yet initialised");
	dma = ctlr->regs;
	sts = dma->isr;
	if ((sts & (Dd | De)) == 0)
		return 0;		/* spurious */

	if (dma->dmasr & Dbe) {
		iprint("dmaintr: bus error, resetting\n");
		dmareset(bit);
		return 0;
	} else if (sts & De) {
		iprint("dmaintr: error, resetting\n");
		dmareset(bit);
		return 0;
	} else if (!(sts & Dd))
		iprint("dmaintr: not done\n");
	else if (dma->dmasr & Dmabsy)
		iprint("dmaintr: still busy\n");
	else if (ctlr->done)
		iprint("dmaintr: ctlr->done already set\n");
	else {
		ctlr->done = 1;		/* signal dmawaitproc */
		ctlr->busy = 0;
		barriers();
		wakeup(ctlr);		/* awaken dmawaitproc */
	}
	dma->isr = sts;			/* extinguish intr source */
	barriers();
	intrack(bit);
	return 1;
}

/* have to delay starting kprocs until later */
static void
dma1init(Ctlr *ctlr, Dma *dma)
{
	int chan;
	ulong bit;

	ctlr->regs = dma;
	chan = ctlr - dmas;
	ctlr->chan = chan;
	snprint(ctlr->name, sizeof ctlr->name, "dma%d", chan);
	bit = dma == (Dma *)Dmactlr? Intdma: Intdma2;
	dmareset(bit);
	intrenable(bit, dmaintr, ctlr->name);
}

void
dma0init(void)
{
	int i;

	for (i = 0; i < nelem(dmas); i++)
		dma1init(&dmas[i], (Dma *)(i == 0? Dmactlr: Dmactlr2));
	dmainited = 1;
	/* have to delay starting kprocs until later */
}

/* finally start kprocs and exercise the dma controllers */
void
dmainit(void)
{
	int i;

	for (i = 0; i < nelem(dmas); i++)
		dmaprocinit(&dmas[i]);
	dmaready = 1;
}

/*
 * it's convenient if this works even with interrupts masked,
 * though the time-out won't work in that situation.
 */
void
dmawait(int n)
{
	uvlong end;
	Ctlr *ctlr;
	Dma *dma;
	static int first = 1;

	ctlr = &dmas[n];
	ilock(ctlr);
	if (first) {
		first = 0;
		ctlr->done = 1;		/* suppress "no dma interrupt" msg */
	}
	if (!ctlr->busy) {
		iunlock(ctlr);
		return;
	}
	iunlock(ctlr);

	dma = ctlr->regs;
	if (dma == nil)
		panic("dmawait: ctlr->regs not yet set");
	end = m->fastclock + 2;				/* ticks */
	while (dma->dmasr & Dmabsy && m->fastclock < end)
		;

	ilock(ctlr);
	if (dma->dmasr & Dmabsy) {
		iprint("dma still busy after 2 ticks\n");
		return;
	}
	ctlr->busy = 0;				/* previous transfer done */
	barriers();
	iunlock(ctlr);
}

void
dmatestwait(int n)
{
	uvlong end;
	Ctlr *ctlr;
	Dma *dma;
	static int first = 1;

	ctlr = &dmas[n];
	ilock(ctlr);
	if (first) {
		ctlr->done = 1;		/* suppress "no dma interrupt" msg */
		first = 0;
	}
	if (ctlr->done) {
		ctlr->busy = 0;
		barriers();
	}
	iunlock(ctlr);

	dma = ctlr->regs;
	if (dma == nil)
		panic("dmatestwait: ctlr->regs not yet set");
	end = m->fastclock + 2;
	while (dma && dma->dmasr & Dmabsy && m->fastclock < end)
		;

	ilock(ctlr);
	if (dma && dma->dmasr & Dmabsy)
		iprint("dma still busy after 2 ticks\n");
	else {
		ctlr->busy = 0;
		ctlr->done = 0;		/* acknowledge previous transfer done */
		barriers();
	}
	iunlock(ctlr);
}

static void
dmanoop(int)
{
}

static void
dmacpy(int n, void *dest, void *src, ulong len, ulong flags)
{
	if (dmastart(n, dest, src, len, flags, dmanoop))
		dmatestwait(n);
	else
		iprint("can't start dma%d\n", n);
}

static int pass;

static int
trydma(Ctlr *ctlr, uintptr adest, uintptr asrc, int size)
{
	void *dest = (void *)adest, *src = (void *)asrc;

	ilock(ctlr);
	assert(size > 4);
	barriers();
	dcflush(PTR2UINT(src), size);

	memset(dest, 0, size);
	barriers();
	dcflush(PTR2UINT(dest), size);

	ctlr->busy = 0;
	iunlock(ctlr);

	dmacpy(ctlr - dmas, dest, src, size, Sinc | Dinc);
	barriers();
	dcflush(PTR2UINT(dest), size);

	/* leave initialised for next use */
	ilock(ctlr);
	ctlr->busy = 0;
	iunlock(ctlr);

	if (memcmp(src, dest, size) != 0) {
		print("\n%s: pass %d: copied dram incorrectly!\n",
			ctlr->name, pass);

		print("%ux: ", (ushort)(uintptr)src);
		dump(src, size / BY2WD);

		print("%ux: ", (ushort)(uintptr)dest);
		dump(dest, size / BY2WD);

		panic("buggy %s", ctlr->name);
	}
	pass++;
	return 0;
}

/* assumes dmaprocinit has been started */
static void
dmatest(Ctlr *ctlr, uintptr testpat, uintptr testtarg, uint size)
{
	int fail;

	assert((uintptr)testtarg % BY2WD == 0);
	assert((uintptr)testpat  % BY2WD == 0);
	barriers();
	pass = 0;
	fail  = trydma(ctlr, testtarg, testpat, size);
	fail |= trydma(ctlr, testtarg+BY2WD, testpat, size-BY2WD);
	fail |= trydma(ctlr, testtarg, testpat+BY2WD, size-BY2WD);
	fail |= trydma(ctlr, testtarg+BY2WD, testpat+BY2WD, size-BY2WD);
	barriers();
	if (fail)
		print("failed...");
	else
		print("ok...");
}

enum {
	Size	= 32,
	Pat	= PHYSSRAM + 64*1024,	/* step on middle of sram (9load) */
	Targ	= Pat + 2*Size,
};

static ulong testtarg[8];

static void
dmaprocinit(Ctlr *ctlr)
{
	char name[32];
	static ulong testpat[8] = {
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
		0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
	};

	snprint(name, sizeof name, "%swait", ctlr->name);
	kproc(name, dmawaitproc, ctlr);

	/*
	 * verify correct functioning
	 */
	print("%s: ", ctlr->name);
	print("dram ");
	dmatest(ctlr, (uintptr)testpat, (uintptr)testtarg, sizeof testpat);
	print("sram ");
	dmatest(ctlr, Pat, Targ, Size);
	print("\n");
}

int
dmastart(int n, void *dest, void *src, ulong len, ulong flags,
	void (*donenotify)(int))
{
	Ctlr *ctlr;
	Dma *dma;

	if (!dmainited) {
		iprint("dmastart: dma not yet inited\n");
		return 0;
	}
	ctlr = &dmas[n];
	dma = ctlr->regs;
	if (dma->dmasr & Dmabsy) {
		iprint("dmastart: dma still in progress for %s\n", ctlr->name);
		dmawait(n);
	}

	ilock(ctlr);
	if (ctlr->busy) {
		iunlock(ctlr);
		iprint("dmastart: ctlr busy for %s\n", ctlr->name);
		dmawait(n);
		ilock(ctlr);
	}
	assert(!(dma->dmasr & Dmabsy));
	assert(!ctlr->busy);
	ctlr->donenotify = donenotify;
	ctlr->flags = flags;
	ctlr->dest = dest;
	ctlr->len = len;
	ctlr->busy = 1;
	barriers();

	if (flags & Sinc)
		dcflush(PTR2UINT(src), len);
	if (flags & Dinc)
		dcflush(PTR2UINT(dest), len);
	dma->dmacr = flags;		/* granularity in v2 is 1 byte */
	dma->sa = PADDR(src);
	dma->da = PADDR(dest);
	dma->ier = Deie | Ddie;
	barriers();
	dma->length = len;		/* start dma */
	barriers();

	iunlock(ctlr);
	return 1;
}
