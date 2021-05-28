/*
 * Xilinx Local Link FIFOs for Temac, in pairs (rx and tx).
 */
#include "include.h"

enum {
	Reset	= 0xa5,		/* magic [tr]dfr & llr value */

	/* dmacr; copied from dma.c */
	Sinc	= 1<<31,	/* source increment */
	Dinc	= 1<<30,	/* dest increment */

	/* field masks */

	Bytecnt	= (1<<11) - 1,
	Wordcnt	= (1<<9) - 1,
};

enum {				/* register's bits */
	/* isr, ier registers (*?e->*ee) */
	Rpure	= 1<<31,	/* rx packet underrun read error */
	Rpore	= 1<<30,	/* rx packet overrun read error */
	Rpue	= 1<<29,	/* rx packet underrun error */
	Tpoe	= 1<<28,	/* tx packet overrun error */
	Tc	= 1<<27,	/* tx complete */
	Rc	= 1<<26,	/* rx complete */
	Tse	= 1<<25,	/* tx size error */
	Trc	= 1<<24,	/* tx reset complete */
	Rrc	= 1<<23,	/* rx reset complete */
};

typedef struct Llfiforegs Llfiforegs;
typedef struct Llfifosw Llfifosw;

struct Llfiforegs {
	ulong	isr;		/* intr status */
	ulong	ier;		/* intr enable */

	ulong	tdfr;		/* tx data fifo reset */
	ulong	tdfv;		/* tx data fifo vacancy (words free) */
	ulong	tdfd;		/* tx data fifo write port */
	ulong	tlf;		/* tx length fifo */

	ulong	rdfr;		/* rx data fifo reset */
	ulong	rdfo;		/* rx data fifo occupancy */
	ulong	rdfd;		/* rx data fifo read port */
	ulong	rlf;		/* tx length fifo */

	ulong	llr;		/* locallink reset */
};
struct Llfifosw {
	Llfiforegs *regs;
};

static Llfiforegs *frp = (Llfiforegs *)Llfifo;
static Ether *llether;

/*
 * as of dma controller v2, keyhole operations are on ulongs,
 * but otherwise it's as if memmove were used.
 * addresses need not be word-aligned, though registers are.
 */
static void
fifocpy(void *vdest, void *vsrc, uint bytes, ulong flags)
{
	int words;
	ulong *dest, *dstbuf, *src;
	/* +2*BY2WD is slop for alignment */
	static uchar buf[ETHERMAXTU+8+2*BY2WD];

	dest = vdest;
	src = vsrc;
	assert(bytes <= sizeof buf);
	words = bytes / BY2WD;
	if (bytes % BY2WD != 0)
		words++;

	switch (flags & (Sinc | Dinc)) {
	case Sinc | Dinc:
		memmove(vdest, vsrc, bytes);
		break;
	case Sinc:				/* mem to register */
		src = (ulong *)ROUNDUP((uvlong)buf, BY2WD);
		memmove(src, vsrc, bytes);	/* ensure src alignment */
		assert((uintptr)src  % BY2WD == 0);
		assert((uintptr)dest % BY2WD == 0);
		while (words-- > 0)
			*dest = *src++;
		break;
	case Dinc:				/* register to mem */
		dest = dstbuf = (ulong *)ROUNDUP((uvlong)buf, BY2WD);
		assert((uintptr)src  % BY2WD == 0);
		assert((uintptr)dest % BY2WD == 0);
		while (words-- > 0)
			*dest++ = *src;
		memmove(vdest, dstbuf, bytes);	/* ensure dest alignment */
		break;
	case 0:				/* register-to-null or vice versa */
		while (words-- > 0)
			*dest = *src;
		break;
	}
}

static void
discardinpkt(int len)		/* discard the rx fifo's packet */
{
	ulong null;

	fifocpy(&null, &frp->rdfd, len, 0);
	coherence();
}

int
llfifointr(ulong bit)
{
	ulong len, sts;
	Ether *ether;
	RingBuf *rb;
	static uchar zaddrs[Eaddrlen * 2];

	sts = frp->isr;
	if (sts == 0)
		return 0;			/* not for me */
	ether = llether;
	/* it's important to drain all packets in the rx fifo */
	while ((frp->rdfo & Wordcnt) != 0) {
		assert((frp->rdfo & ~Wordcnt) == 0);
		len = frp->rlf & Bytecnt;	/* read rlf from fifo */
		assert((len & ~Bytecnt) == 0);
		assert(len > 0 && len <= ETHERMAXTU);
		rb = &ether->rb[ether->ri];
		if (rb->owner == Interface) {
			/* from rx fifo into ring buffer */
			fifocpy(rb->pkt, &frp->rdfd, len, Dinc);
			if (memcmp(rb->pkt, zaddrs, sizeof zaddrs) == 0) {
				iprint("ether header with all-zero mac "
					"addresses\n");
				continue;
			}
			rb->len = len;
			rb->owner = Host;
			coherence();
			ether->ri = NEXT(ether->ri, ether->nrb);
			coherence();
		} else {
			discardinpkt(len);
			/* not too informative during booting */
			iprint("llfifo: no buffer for input pkt\n");
		}
	}

	if (sts & Tc)
		ether->tbusy = 0;
	ether->transmit(ether);

	frp->isr = sts;			/* extinguish intr source */
	coherence();

	intrack(bit);
	sts &= ~(Tc | Rc);
	if (sts)
		iprint("llfifo isr %#lux\n", sts);
	return 1;
}

void
llfiforeset(void)
{
	frp->tdfr = Reset;
	frp->rdfr = Reset;
	coherence();
	while ((frp->isr & (Trc | Rrc)) != (Trc | Rrc))
		;
}

void
llfifoinit(Ether *ether)
{
	llether = ether;
	frp->ier = 0;
	frp->isr = frp->isr;	/* extinguish intr source */
	coherence();

	intrenable(Intllfifo, llfifointr);
	coherence();
	frp->ier = Rc | Tc;
	coherence();
}

void
llfifotransmit(uchar *ubuf, unsigned len)
{
	int wds;

	llether->tbusy = 1;

	assert(len <= ETHERMAXTU);
	wds = ROUNDUP(len, BY2WD) / BY2WD;

	/* wait for tx fifo to drain */
	while ((frp->tdfv & Wordcnt) < wds)
		;

	/* to tx fifo */
	assert((frp->tdfv & ~Wordcnt) == 0);
	fifocpy(&frp->tdfd, ubuf, len, Sinc);
	coherence();
	frp->tlf = len;			/* send packet in tx fifo to ether */
	coherence();
}
