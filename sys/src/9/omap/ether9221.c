/*
 * SMSC 9221 Ethernet driver
 * specifically for the ISEE IGEPv2 board,
 * where it is assigned to Chip Select 5,
 * its registers are at 0x2c000000 (inherited from u-boot),
 * and irq is 34 from gpio pin 176, thus gpio module 6.
 *
 * it's slow due to the use of fifos instead of buffer rings.
 * the slow system dma just makes it worse.
 *
 * igepv2 u-boot uses pin 64 on gpio 3 as an output pin to reset the 9221.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"

/* currently using kprocs is a lot slower than not (87 s. to boot vs 60) */
#undef USE_KPROCS

enum {
	Vid9221	= 0x9221,
	Slop	= 4,			/* beyond ETHERMAXTU */
};

typedef struct Regs Regs;
struct Regs {
	/* fifo ports */
	ulong	rxdata;
	uchar	_pad0[0x20 - 4];
	ulong	txdata;
	uchar	_pad1[0x40 - 0x24];
	ulong	rxsts;
	ulong	rxstspeek;
	ulong	txsts;
	ulong	txstspeek;

	/* control & status */
	ushort	rev;			/* chip revision */
	ushort	id;			/* chip id, 0x9221 */
	ulong	irqcfg;
	ulong	intsts;
	ulong	inten;
	ulong	_pad2;
	ulong	bytetest;
	ulong	fifoint;		/* fifo level interrupts */
	ulong	rxcfg;
	ulong	txcfg;
	ulong	hwcfg;
	ulong	rxdpctl;		/* rx data path control */
	ulong	rxfifoinf;
	ulong	txfifoinf;
	ulong	pmtctl;			/* power mgmt. control */
	ulong	gpiocfg;
	ulong	gptcfg;			/* timer */
	ulong	gptcnt;
	ulong	_pad3;
	ulong	wordswap;
	ulong	freerun; 		/* counters */
	ulong	rxdrop;

	/*
	 * mac registers are accessed indirectly via the mac csr registers.
	 * phy registers are doubly indirect, via the mac csr mii_acc &
	 * mii_data mac csr registers.
	 */
	ulong	maccsrcmd;		/* mac csr synchronizer */
	ulong	maccsrdata;
	ulong	afccfg;			/* automatic flow control cfg. */
	ulong	eepcmd;			/* eeprom */
	ulong	eepdata;
	/* 0xb8 */
};

enum {
	Nstatistics	= 128,
};

enum {
	/* txcmda bits */
	Intcompl	= 1<<31,
	Bufendalign	= 3<<24,	/* mask */
	Datastoff	= 037<<16,	/* mask */
	Firstseg	= 1<<13,
	Lastseg		= 1<<12,
	Bufsize		= MASK(11),

	/* txcmdb bits */
	Pkttag		= MASK(16) << 16,
	Txcksumen	= 1<<14,
	Addcrcdis	= 1<<13,
	Framepaddis	= 1<<12,
	Pktlen		= (1<<1) - 1,	/* mask */

	/* txcfg bits */
	Txsdump		= 1<<15,	/* flush tx status fifo */
	Txddump		= 1<<14,	/* flush tx data fifo */
	Txon		= 1<<1,
	Stoptx		= 1<<0,

	/* hwcfg bits */
	Mbo		= 1<<20,	/* must be one */
	Srstto		= 1<<1,		/* soft reset time-out */
	Srst		= 1<<0,

	/* rxcfg bits */
	Rxdmacntshift	= 16,		/* ulong count, 12 bits wide */
	Rxdmacntmask	= MASK(12) << Rxdmacntshift,
	Rxdump		= 1<<15,	/* flush rx fifos */

	/* rxsts bits */
	Rxpktlenshift	= 16,		/* byte count */
	Rxpktlenmask	= MASK(14) << Rxpktlenshift,
	Rxerr		= 1<<15,

	/* rxfifoinf bits */
	Rxstsusedshift	= 16,		/* ulong count */
	Rxstsusedmask	= MASK(8) << Rxstsusedshift,
	Rxdatausedmask	= MASK(16),	/* byte count */

	/* txfifoinf bits */
	Txstsusedshift	= 16,		/* ulong count */
	Txstsusedmask	= MASK(8) << Txstsusedshift,
	Txdatafreemask	= MASK(16),	/* byte count */

	/* pmtctl bits */
	Dready		= 1<<0,

	/* maccsrcmd bits */
	Csrbusy		= 1<<31,
	Csrread		= 1<<30,	/* not write */
	Csraddrshift	= 0,
	Csraddrmask	= MASK(8) - 1,

	/* mac registers' indices */
	Maccr		= 1,
	Macaddrh,
	Macaddrl,
	Machashh,
	Machashl,
	Macmiiacc,			/* for doubly-indirect phy access */
	Macmiidata,
	Macflow,
	Macvlan1,
	Macvlan2,
	Macwuff,
	Macwucsr,
	Maccoe,

	/* Maccr bits */
	Rxall		= 1<<31,
	Rcvown		= 1<<23,	/* don't receive own transmissions */
	Fdpx		= 1<<20,	/* full duplex */
	Mcpas		= 1<<19,	/* pass all multicast */
	Prms		= 1<<18,	/* promiscuous */
	Ho		= 1<<15,	/* hash-only filtering */
	Hpfilt		= 1<<13,	/* hash/perfect filtering */
	Padstr		= 1<<8,		/* strip padding & fcs (crc) */
	Txen		= 1<<3,
	Rxen		= 1<<2,

	/* irqcfg bits */
	Irqdeasclr	= 1<<14,	/* deassertion intv'l clear */
	Irqdeassts	= 1<<13,	/* deassertion intv'l status */
	Irqint		= 1<<12,	/* intr being asserted? (ro) */
	Irqen		= 1<<8,
	Irqpol		= 1<<4,		/* irq output is active high */
	Irqpushpull	= 1<<0,		/* irq output is push/pull driver */

	/* intsts/inten bits */
	Swint		= 1<<31,	/* generate an interrupt */
	Txstop		= 1<<25,
	Rxstop		= 1<<24,
	Txioc		= 1<<21,
	Rxdma		= 1<<20,
	Gptimer		= 1<<19,
	Phy		= 1<<18,
	Rxe		= 1<<14,	/* errors */
	Txe		= 1<<13,
	Tdfo		= 1<<10,	/* tx data fifo overrun */
	Tdfa		= 1<<9,		/* tx data fifo available */
	Tsff		= 1<<8,		/* tx status fifo full */
	Tsfl		= 1<<7,		/* tx status fifo level */
	Rsff		= 1<<4,		/* rx status fifo full */
	Rsfl		= 1<<3,		/* rx status fifo level */

	/* eepcmd bits */
	Epcbusy		= 1<<31,
	Epccmdshift	= 28,		/* interesting one is Reload (7) */
	Epctimeout	= 1<<9,
	Epcmacloaded	= 1<<8,
	Epcaddrshift	= 0,
};

enum {
	Rxintrs		= Rsff | Rsfl | Rxe,
	Txintrs		= Tsff | Tsfl | Txe | Txioc,
};

/* wake-up frame filter */
struct Wakeup {
	ulong	bytemask[4];		/* index is filter # */
	uchar	filt0cmd;		/* filter 0 command */
	uchar	_pad0;
	uchar	filt1cmd;
	uchar	_pad1;
	uchar	filt2cmd;
	uchar	_pad2;
	uchar	filt3cmd;
	uchar	_pad3;
	uchar	offset[4];		/* index is filter # */
	ushort	crc16[4];		/* " */
};

typedef struct Ctlr Ctlr;
struct Ctlr {
	int	port;
	Ctlr*	next;
	Ether*	edev;
	Regs*	regs;
	int	active;
	int	started;
	int	inited;
	int	id;
	int	cls;
	ushort	eeprom[0x40];

	QLock	alock;			/* attach */
	int	nrb;			/* how many this Ctlr has in the pool */

	int*	nic;
	Lock	imlock;
	int	im;			/* interrupt mask */

//	Mii*	mii;
//	Rendez	lrendez;
	int	lim;

	int	link;

	QLock	slock;
	uint	statistics[Nstatistics];
	uint	lsleep;
	uint	lintr;
	uint	rsleep;
	uint	rintr;
	int	tsleep;
	uint	tintr;

	uchar	ra[Eaddrlen];		/* receive address */
	ulong	mta[128];		/* multicast table array */

	Rendez	rrendez;
	int	gotinput;
	int	rdcpydone;

	Rendez	trendez;
	int	gotoutput;
	int	wrcpydone;

	Lock	tlock;
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static Ctlr *smcctlrhead, *smcctlrtail;

static char* statistics[Nstatistics] = { "dummy", };

static uchar mymac[] = { 0xb0, 0x0f, 0xba, 0xbe, 0x00, 0x00, };

static void etherclock(void);
static void smcreceive(Ether *edev);
static void smcinterrupt(Ureg*, void* arg);

static Ether *thisether;
static int attached;

static void
smconce(Ether *edev)
{
	static int beenhere;
	static Lock l;

	ilock(&l);
	if (!beenhere && edev != nil) {
		beenhere = 1;
		/* simulate interrupts if we don't know the irq */
		if (edev->irq < 0) {		/* poll as backup */
			thisether = edev;
			addclock0link(etherclock, 1000/HZ);
			iprint(" polling");
		}
	}
	iunlock(&l);
}

/*
 * indirect (mac) register access
 */

static void
macwait(Regs *regs)
{
	long bound;

	for (bound = 400*Mhz; regs->maccsrcmd & Csrbusy && bound > 0; bound--)
		;
	if (bound <= 0)
		iprint("smc: mac registers didn't come ready\n");
}

static ulong
macrd(Regs *regs, uchar index)
{
	macwait(regs);
	regs->maccsrcmd = Csrbusy | Csrread | index;
	coherence();		/* back-to-back write/read delay per §6.2.1 */
	macwait(regs);
	return regs->maccsrdata;
}

static void
macwr(Regs *regs, uchar index, ulong val)
{
	macwait(regs);
	regs->maccsrdata = val;
	regs->maccsrcmd = Csrbusy | index;	/* fire */
	macwait(regs);
}


static long
smcifstat(Ether* edev, void* a, long n, ulong offset)
{
	Ctlr *ctlr;
	char *p, *s;
	int i, l, r;

	ctlr = edev->ctlr;
	qlock(&ctlr->slock);
	p = malloc(READSTR);
	l = 0;
	for(i = 0; i < Nstatistics; i++){
		// read regs->rxdrop TODO
		r = 0;
		if((s = statistics[i]) == nil)
			continue;
		switch(i){
		default:
			ctlr->statistics[i] += r;
			if(ctlr->statistics[i] == 0)
				continue;
			l += snprint(p+l, READSTR-l, "%s: %ud %ud\n",
				s, ctlr->statistics[i], r);
			break;
		}
	}

	l += snprint(p+l, READSTR-l, "lintr: %ud %ud\n",
		ctlr->lintr, ctlr->lsleep);
	l += snprint(p+l, READSTR-l, "rintr: %ud %ud\n",
		ctlr->rintr, ctlr->rsleep);
	l += snprint(p+l, READSTR-l, "tintr: %ud %ud\n",
		ctlr->tintr, ctlr->tsleep);

	l += snprint(p+l, READSTR-l, "eeprom:");
	for(i = 0; i < 0x40; i++){
		if(i && ((i & 0x07) == 0))
			l += snprint(p+l, READSTR-l, "\n       ");
		l += snprint(p+l, READSTR-l, " %4.4uX", ctlr->eeprom[i]);
	}
	l += snprint(p+l, READSTR-l, "\n");
	USED(l);

	n = readstr(offset, a, n, p);
	free(p);
	qunlock(&ctlr->slock);

	return n;
}

static void
smcpromiscuous(void* arg, int on)
{
	int rctl;
	Ctlr *ctlr;
	Ether *edev;
	Regs *regs;

	edev = arg;
	ctlr = edev->ctlr;
	regs = ctlr->regs;
	rctl = macrd(regs, Maccr);
	if(on)
		rctl |= Prms;
	else
		rctl &= ~Prms;
	macwr(regs, Maccr, rctl);
}

static void
smcmulticast(void*, uchar*, int)
{
	/* nothing to do, we allow all multicast packets in */
}

static int
iswrcpydone(void *arg)
{
	return ((Ctlr *)arg)->wrcpydone;
}

static int
smctxstart(Ctlr *ctlr, uchar *ubuf, uint len)
{
	uint wds, ruplen;
	ulong *wdp, *txdp;
	Regs *regs;
	static ulong buf[ROUNDUP(ETHERMAXTU, sizeof(ulong)) / sizeof(ulong)];

	if (!ctlr->inited) {
		iprint("smctxstart: too soon to send\n");
		return -1;		/* toss it */
	}
	regs = ctlr->regs;

	/* is there room for a packet in the tx data fifo? */
	if (len < ETHERMINTU)
		iprint("sending too-short (%d) pkt\n", len);
	else if (len > ETHERMAXTU)
		iprint("sending jumbo (%d) pkt\n", len);

	ruplen = ROUNDUP(len, sizeof(ulong));
	coherence();	/* back-to-back read/read delay per §6.2.2 */
	if ((regs->txfifoinf & Txdatafreemask) < ruplen + 2*sizeof(ulong))
		return -1;	/* not enough room for data + command words */

	if ((uintptr)ubuf & MASK(2)) {		/* ensure word alignment */
		memmove(buf, ubuf, len);
		ubuf = (uchar *)buf;
	}

	/* tx cmd a: length is bytes in this buffer */
	txdp = &regs->txdata;
	*txdp = Intcompl | Firstseg | Lastseg | len;
	/* tx cmd b: length is bytes in this packet (could be multiple buf.s) */
	*txdp = len;

	/* shovel pkt into tx fifo, which triggers transmission due to Txon */
	wdp = (ulong *)ubuf;
	for (wds = ruplen / sizeof(ulong) + 1; --wds > 0; )
		*txdp = *wdp++;

	regs->intsts = Txintrs;		/* dismiss intr */
	coherence();
	regs->inten |= Txintrs;
	coherence();		/* back-to-back write/read delay per §6.2.1 */
	return 0;
}

static void
smctransmit(Ether* edev)
{
	Block *bp;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if (ctlr == nil)
		panic("smctransmit: nil ctlr");
	ilock(&ctlr->tlock);
	/*
	 * Try to fill the chip's buffers back up, via the tx fifo.
	 */
	while ((bp = qget(edev->oq)) != nil)
		if (smctxstart(ctlr, bp->rp, BLEN(bp)) < 0) {
			qputback(edev->oq, bp);	/* retry the block later */
			iprint("smctransmit: tx data fifo full\n");
			break;
		} else
			freeb(bp);
	iunlock(&ctlr->tlock);
}

static void
smctransmitcall(Ether *edev)		/* called from devether.c */
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	ctlr->gotoutput = 1;
#ifdef USE_KPROCS
	wakeup(&ctlr->trendez);
#else
	smctransmit(edev);
#endif
}

static int
smcrim(void* ctlr)
{
	return ((Ctlr*)ctlr)->gotinput;
}

static void
smcrproc(void* arg)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		ctlr->rsleep++;
		sleep(&ctlr->rrendez, smcrim, ctlr);

		/* process any newly-arrived packets and pass to etheriq */
		ctlr->gotinput = 0;
		smcreceive(edev);
	}
}

static int
smcgotout(void* ctlr)
{
	return ((Ctlr*)ctlr)->gotoutput;
}

static void
smctproc(void* arg)
{
	Ctlr *ctlr;
	Ether *edev;

	edev = arg;
	ctlr = edev->ctlr;
	for(;;){
		ctlr->tsleep++;
		sleep(&ctlr->trendez, smcgotout, ctlr);

		/* process any newly-arrived packets and pass to etheriq */
		ctlr->gotoutput = 0;
		smctransmit(edev);
	}
}

void	gpioirqclr(void);

static void
smcattach(Ether* edev)
{
#ifdef USE_KPROCS
	char name[KNAMELEN];
#endif
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	qlock(&ctlr->alock);
	if(waserror()){
		qunlock(&ctlr->alock);
		nexterror();
	}
	if (!ctlr->inited) {
		ctlr->inited = 1;
#ifdef USE_KPROCS
		snprint(name, KNAMELEN, "#l%drproc", edev->ctlrno);
		kproc(name, smcrproc, edev);

		snprint(name, KNAMELEN, "#l%dtproc", edev->ctlrno);
		kproc(name, smctproc, edev);
#endif

iprint("smcattach:");
#ifdef USE_KPROCS
iprint(" with kprocs");
#else
iprint(" no kprocs");
#endif
iprint(", no dma");
		/* can now accept real or simulated interrupts */

		smconce(edev);
		attached = 1;
iprint("\n");
	}
	qunlock(&ctlr->alock);
	poperror();
}

static int
isrdcpydone(void *arg)
{
	return ((Ctlr *)arg)->rdcpydone;
}

static void
smcreceive(Ether *edev)
{
	uint wds, len, sts;
	ulong *wdp, *rxdp;
	Block *bp;
	Ctlr *ctlr;
	Regs *regs;

	ctlr = edev->ctlr;
	regs = ctlr->regs;
	coherence();		/* back-to-back read/read delay per §6.2.2 */
	/*
	 * is there a full packet in the rx data fifo?
	 */
	while (((regs->rxfifoinf & Rxstsusedmask) >> Rxstsusedshift) != 0) {
		coherence();
		sts = regs->rxsts;		/* pop rx status */
		if(sts & Rxerr)
			iprint("smcreceive: rx error\n");
		len = (sts & Rxpktlenmask) >> Rxpktlenshift;
		if (len > ETHERMAXTU + Slop)
			iprint("smcreceive: oversized rx pkt (%d)\n", len);
		else if (len < ETHERMINTU)
			iprint("smcreceive: too-short (%d) pkt\n", len);
		wds = ROUNDUP(len, sizeof(ulong)) / sizeof(ulong);
		if (wds > 0) {
			/* copy aligned words from rx fifo into a Block */
			bp = iallocb(len + sizeof(ulong) /* - 1 */);
			if (bp == nil)
				panic("smcreceive: nil Block*");

			/* bp->rp should be 32-byte aligned, more than we need */
			assert(((uintptr)bp->rp & (sizeof(ulong) - 1)) == 0);
			wdp = (ulong *)bp->rp;
			rxdp = &regs->rxdata;
			wds = ROUNDUP(len, sizeof(ulong)) / sizeof(ulong) + 1;
			while (--wds > 0)
				*wdp++ = *rxdp;
			bp->wp = bp->rp + len;

			/* and push the Block upstream */
			if (ctlr->inited)
				etheriq(edev, bp, 1);
			else
				freeb(bp);

			regs->intsts = Rxintrs;		/* dismiss intr */
			coherence();
			regs->inten |= Rxintrs;
		}
		coherence();
	}
	regs->inten |= Rxintrs;
	coherence();
}

/*
 * disable the stsclr bits in inten and write them to intsts to ack and dismiss
 * the interrupt source.
 */
void
ackintr(Regs *regs, ulong stsclr)
{
	if (stsclr == 0)
		return;

	regs->inten &= ~stsclr;
	coherence();

//	regs->intsts = stsclr;		/* acknowledge & clear intr(s) */
//	coherence();
}

static void
smcinterrupt(Ureg*, void* arg)
{
	int junk;
	unsigned intsts, intr;
	Ctlr *ctlr;
	Ether *edev;
	Regs *regs;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(&ctlr->imlock);
	regs = ctlr->regs;

	gpioirqclr();

	coherence();		/* back-to-back read/read delay per §6.2.2 */
	intsts = regs->intsts;
	coherence();

	intsts &= ~MASK(3);		/* ignore gpio bits */
	if (0 && intsts == 0) {
		coherence();
		iprint("smc: interrupt without a cause; insts %#ux (vs inten %#lux)\n",
			intsts, regs->inten);
	}

	intr = intsts & Rxintrs;
	if(intr) {
		/* disable interrupt sources; kproc/smcreceive will reenable */
		ackintr(regs, intr);

		ctlr->rintr++;
		ctlr->gotinput = 1;
#ifdef USE_KPROCS
		wakeup(&ctlr->rrendez);
#else
		smcreceive(edev);
#endif
	}

	while(((regs->txfifoinf & Txstsusedmask) >> Txstsusedshift) != 0) {
		/* probably indicates tx completion, just toss it */
		junk = regs->txsts;		/* pop tx sts */
		USED(junk);
		coherence();
	}

	intr = intsts & Txintrs;
	if (ctlr->gotoutput || intr) {
		/* disable interrupt sources; kproc/smctransmit will reenable */
		ackintr(regs, intr);

		ctlr->tintr++;
		ctlr->gotoutput = 1;
#ifdef USE_KPROCS
		wakeup(&ctlr->trendez);
#else
		smctransmit(edev);
#endif
	}

	iunlock(&ctlr->imlock);
}

static void
etherclock(void)
{
	smcinterrupt(nil, thisether);
}

static int
smcmii(Ctlr *)
{
	return 0;
}

static int
smcdetach(Ctlr* ctlr)
{
	Regs *regs;

	if (ctlr == nil || ctlr->regs == nil)
		return -1;
	regs = ctlr->regs;
	/* verify that it's real by reading a few registers */
	switch (regs->id) {
	case Vid9221:
		break;
	default:
		print("smc: unknown chip id %#ux\n", regs->id);
		return -1;
	}
	regs->inten = 0;		/* no interrupts */
	regs->intsts = ~0;		/* clear any pending */
	regs->gptcfg = 0;
	coherence();
	regs->rxcfg = Rxdump;
	regs->txcfg = Txsdump | Txddump;
	regs->irqcfg &= ~Irqen;
	coherence();
	return 0;
}

static void
smcshutdown(Ether* ether)
{
	smcdetach(ether->ctlr);
}

static void
powerwait(Regs *regs)
{
	long bound;

	regs->bytetest = 0;			/* bring power on */
	for (bound = 400*Mhz; !(regs->pmtctl & Dready) && bound > 0; bound--)
		;
	if (bound <= 0)
		iprint("smc: pmtctl didn't come ready\n");
}

static int
smcreset(Ctlr* ctlr)
{
	int r;
	Regs *regs;
	static char zea[Eaddrlen];

	regs = ctlr->regs;
	powerwait(regs);

	if(smcdetach(ctlr))
		return -1;

	/* verify that it's real by reading a few registers */
	switch (regs->id) {
	case Vid9221:
		break;
	default:
		print("smc: unknown chip id %#ux\n", regs->id);
		return -1;
	}
	if (regs->bytetest != 0x87654321) {
		print("smc: bytetest reg %#p (%#lux) != 0x87654321\n",
			&regs->bytetest, regs->bytetest);
		return -1;
	}

#ifdef TODO			/* read MAC from EEPROM */
//	int ctrl, i, pause, swdpio, txcw;
	/*
	 * Snarf and set up the receive addresses.
	 * There are 16 addresses. The first should be the MAC address.
	 * The others are cleared and not marked valid (MS bit of Rah).
	 */
	for(i = Ea; i < Eaddrlen/2; i++){
		ctlr->ra[2*i] = ctlr->eeprom[i];
		ctlr->ra[2*i+1] = ctlr->eeprom[i]>>8;
	}

	/*
	 * Clear the Multicast Table Array.
	 * It's a 4096 bit vector accessed as 128 32-bit registers.
	 */
	memset(ctlr->mta, 0, sizeof(ctlr->mta));
	for(i = 0; i < 128; i++)
		csr32w(ctlr, Mta+i*4, 0);
#endif
	regs->hwcfg |= Mbo;

	/* don't overwrite existing ea */
//	if (memcmp(edev->ea, zea, Eaddrlen) == 0)
//		memmove(edev->ea, ctlr->ra, Eaddrlen);

	r = ctlr->ra[3]<<24 | ctlr->ra[2]<<16 | ctlr->ra[1]<<8 | ctlr->ra[0];
	macwr(regs, Macaddrl, r);
	macwr(regs, Macaddrh, ctlr->ra[5]<<8 | ctlr->ra[4]);

	/* turn on the controller */
	macwr(regs, Maccoe, 0);
	regs->inten = 0;		/* no interrupts yet */
	regs->intsts = ~0;		/* clear any pending */
	regs->gptcfg = 0;
	coherence();
	regs->rxcfg = Rxdump;
	regs->txcfg = Txsdump | Txddump | Txon;
	regs->fifoint = 72<<24;		/* default values */
	macwr(regs, Maccr, Rxall | Rcvown | Fdpx | Mcpas | Txen | Rxen);
	coherence();		/* back-to-back write/read delay per §6.2.1 */
	regs->irqcfg = 1<<24 | Irqen | Irqpushpull;  /* deas for 10µs (linux) */
	coherence();		/* back-to-back write/read delay per §6.2.1 */
	regs->inten = Rxintrs | Txintrs;
	coherence();

	if(smcmii(ctlr) < 0)
		return -1;
	return 0;
}

static void
smcpci(void)
{
	Ctlr *ctlr;
	static int beenhere;

	if (beenhere)
		return;
	beenhere = 1;

	if (probeaddr(PHYSETHER) < 0)
		return;
	ctlr = malloc(sizeof(Ctlr));
	ctlr->id = Vid9221<<16 | 0x0424;	/* smsc 9221 */
	ctlr->port = PHYSETHER;
	ctlr->nic = (int *)PHYSETHER;
	ctlr->regs = (Regs *)PHYSETHER;

	if(smcreset(ctlr)){
		free(ctlr);
		return;
	}
	if(smcctlrhead != nil)
		smcctlrtail->next = ctlr;
	else
		smcctlrhead = ctlr;
	smcctlrtail = ctlr;
}

static int
smcpnp(Ether* edev)
{
	Ctlr *ctlr;
	static char zea[Eaddrlen];

	if(smcctlrhead == nil)
		smcpci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = smcctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	ctlr->edev = edev;			/* point back to Ether* */
	edev->port = ctlr->port;
	edev->irq = 34;
// TODO: verify speed (100Mb/s) and duplicity (full-duplex)
	edev->mbps = 100;

	/* don't overwrite existing ea */
	if (memcmp(edev->ea, zea, Eaddrlen) == 0)
		memmove(edev->ea, ctlr->ra, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = smcattach;
	edev->transmit = smctransmitcall;
	edev->interrupt = smcinterrupt;
	edev->ifstat = smcifstat;
/*	edev->ctl = smcctl;			/* no ctl msgs supported */

	edev->arg = edev;
	edev->promiscuous = smcpromiscuous;
	edev->multicast = smcmulticast;
	edev->shutdown = smcshutdown;
	return 0;
}

void
ether9221link(void)
{
	addethercard("9221", smcpnp);
}
