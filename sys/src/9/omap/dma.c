/*
 * omap3530 system dma controller
 *
 * terminology: a block consist of frame(s), a frame consist of elements
 * (uchar, ushort, or ulong sized).
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

enum {
	Nirq	= 4,
	Baseirq	= 12,

	Nchan	= 32,
};

/*
 * has a sw reset bit
 * dma req lines 1, 2, 6, 63 are available for `system expansion'
 */

typedef struct Regs Regs;
typedef struct Dchan Dchan;
struct Regs {
	uchar	_pad0[8];
	/* bitfield of intrs pending, by Dchan; write 1s to clear */
	ulong	irqsts[Nirq];
	ulong	irqen[Nirq];	/* bitfield of intrs enabled, by Dchan */
	ulong	syssts;		/* 1<<0 is Resetdone */
	ulong	syscfg;		/* 1<<1 is Softreset */
	uchar	_pad1[0x64 - 0x30];

	ulong	caps[5];	/* caps[1] not defined */
	ulong	gcr;		/* knobs */
	ulong	_pad2;

	struct Dchan {
		ulong	ccr;	/* chan ctrl: incr, etc. */
		ulong	clnkctrl; /* link ctrl */
		ulong	cicr;	/* intr ctrl */
		ulong	csr;	/* status */
		ulong	csdp;	/* src & dest params */
		ulong	cen;	/* element # */
		ulong	cfn;	/* frame # */
		ulong	cssa;	/* src start addr */
		ulong	cdsa;	/* dest start addr */
		ulong	csei;	/* src element index */
		ulong	csfi;	/* src frame index | pkt size */
		ulong	cdei;	/* dest element index */
		ulong	cdfi;	/* dest frame index | pkt size */
		ulong	csac;	/* src addr value (read-only?) */
		ulong	cdac;	/* dest addr value */
		ulong	ccen;	/* curr transferred element # (in frame) */
		ulong	ccfn;	/* curr transferred frame # (in xfer) */
		ulong	color;
		uchar	_pad3[24];
	} chan[Nchan];
};

enum {
	/* cicr/csr bits */
	Blocki	= 1 << 5,

	/* ccr bits */
	Enable	= 1 << 7,
};

typedef struct Xfer Xfer;
static struct Xfer {
	Rendez	*rend;
	int	*done;		/* flag to set on intr */
} xfer[Nirq];

int
isdmadone(int irq)
{
	Dchan *cp;
	Regs *regs = (Regs *)PHYSSDMA;

	cp = regs->chan + irq;
	return cp->csr & Blocki;
}

static void
dmaintr(Ureg *, void *a)
{
	int i = (int)a;			/* dma request & chan # */
	Dchan *cp;
	Regs *regs = (Regs *)PHYSSDMA;

	assert(i >= 0 && i < Nirq);

	*xfer[i].done = 1;
	assert(xfer[i].rend != nil);
	wakeup(xfer[i].rend);

	cp = regs->chan + i;
	if(!(cp->csr & Blocki))
		iprint("dmaintr: req %d: Blocki not set; csr %#lux\n",
			i, cp->csr);
	cp->csr |= cp->csr;			/* extinguish intr source */
	coherence();
	regs->irqsts[i] = regs->irqsts[i];	/* extinguish intr source */
	coherence();
	regs->irqen[i] &= ~(1 << i);
	coherence();

	xfer[i].rend = nil;
	coherence();
}

void
zerowds(ulong *wdp, int cnt)
{
	while (cnt-- > 0)
		*wdp++ = 0;
}

static int
istestdmadone(void *arg)
{
	return *(int *)arg;
}

void
dmainit(void)
{
	int n;
	char name[16];
	Dchan *cp;
	Regs *regs = (Regs *)PHYSSDMA;

	if (probeaddr((uintptr)&regs->syssts) < 0)
		panic("dmainit: no syssts reg");
	regs->syssts = 0;
	coherence();
	regs->syscfg |= 1<<1;		/* Softreset */
	coherence();
	while(!(regs->syssts & (1<<0)))	/* Resetdone? */
		;

	for (n = 0; n < Nchan; n++) {
		cp = regs->chan + n;
		cp->ccr = 0;
		cp->clnkctrl = 0;
		cp->cicr = 0;
		cp->csr = 0;
		cp->csdp = 0;
		cp->cen = cp->cfn = 0;
		cp->cssa = cp->cdsa = 0;
		cp->csei = cp->csfi = 0;
		cp->cdei = cp->cdfi = 0;
//		cp->csac = cp->cdac = 0;		// ro
		cp->ccen = cp->ccfn = 0;
		cp->color = 0;
	}
	zerowds((void *)regs->irqsts, sizeof regs->irqsts / sizeof(ulong));
	zerowds((void *)regs->irqen,  sizeof regs->irqen / sizeof(ulong));
	coherence();

	regs->gcr = 65;			/* burst size + 1 */
	coherence();

	for (n = 0; n < Nirq; n++) {
		snprint(name, sizeof name, "dma%d", n);
		intrenable(Baseirq + n, dmaintr, (void *)n, nil, name);
	}
}

enum {
	Testbyte	= 0252,
	Testsize	= 256,
	Scratch		= MB,
};

/*
 * try to confirm sane operation
 */
void
dmatest(void)
{
	int n, done;
	uchar *bp;
	static ulong pat = 0x87654321;
	static Rendez trendez;

	if (up == nil)
		panic("dmatest: up not set yet");
	bp = (uchar *)KADDR(PHYSDRAM + 128*MB);
	memset(bp, Testbyte, Scratch);
	done = 0;
	dmastart((void *)PADDR(bp), Postincr, (void *)PADDR(&pat), Const,
		Testsize, &trendez, &done);
	sleep(&trendez, istestdmadone, &done);
	cachedinvse(bp, Scratch);

	if (((ulong *)bp)[0] != pat)
		panic("dmainit: copied incorrect data %#lux != %#lux",
			((ulong *)bp)[0], pat);
	for (n = Testsize; n < Scratch && bp[n] != Testbyte; n++)
		;
	if (n >= Scratch)
		panic("dmainit: ran wild over memory, clobbered ≥%,d bytes", n);
	if (bp[n] == Testbyte && n != Testsize)
		iprint("dma: %d-byte dma stopped after %d bytes!\n",
			Testsize, n);
}

/* addresses are physical */
int
dmastart(void *to, int tmode, void *from, int fmode, uint len, Rendez *rend,
	int *done)
{
	int irq, chan;
	uint ruplen;
	Dchan *cp;
	Regs *regs = (Regs *)PHYSSDMA;
	static Lock alloclck;

	/* allocate free irq (and chan) */
	ilock(&alloclck);
	for (irq = 0; irq < Nirq && xfer[irq].rend != nil; irq++)
		;
	if (irq >= Nirq)
		panic("dmastart: no available irqs; too many concurrent dmas");
	chan = irq;
	xfer[irq].rend = rend;			/* for wakeup at intr time */
	xfer[irq].done = done;
	*done = 0;
	iunlock(&alloclck);

	ruplen = ROUNDUP(len, sizeof(ulong));
	assert(to != from);

	cp = regs->chan + chan;
	cp->ccr &= ~Enable;			/* paranoia */
	cp->cicr = 0;
	regs->irqen[irq] &= ~(1 << chan);
	coherence();

	cp->csdp = 2;				/* 2 = log2(sizeof(ulong)) */
	cp->cssa = (uintptr)from;
	cp->cdsa = (uintptr)to;
	cp->ccr = tmode << 14 | fmode << 12;
	cp->csei = cp->csfi = cp->cdei = cp->cdfi = 1;
	cp->cen = ruplen / sizeof(ulong);	/* ulongs / frame */
	cp->cfn = 1;				/* 1 frame / xfer */
	cp->cicr = Blocki;			/* intr at end of block */

	regs->irqen[irq] |= 1 << chan;
	coherence();

	cp->ccr |= Enable;			/* fire! */
	coherence();

	return irq;
}
