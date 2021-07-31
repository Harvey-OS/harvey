/*
 * AM79C970
 * PCnet-PCI Single-Chip Ethernet Controller for PCI Local Bus
 * To do:
 *	finish this rewrite
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

enum {
	Lognrdre	= 6,
	Nrdre		= (1<<Lognrdre),/* receive descriptor ring entries */
	Logntdre	= 4,
	Ntdre		= (1<<Logntdre),/* transmit descriptor ring entries */

	Rbsize		= ETHERMAXTU+4,	/* ring buffer size (+4 for CRC) */
};

enum {					/* DWIO I/O resource map */
	Aprom		= 0x0000,	/* physical address */
	Rdp		= 0x0010,	/* register data port */
	Rap		= 0x0014,	/* register address port */
	Sreset		= 0x0018,	/* software reset */
	Bdp		= 0x001C,	/* bus configuration register data port */
};

enum {					/* CSR0 */
	Init		= 0x0001,	/* begin initialisation */
	Strt		= 0x0002,	/* enable chip */
	Stop		= 0x0004,	/* disable chip */
	Tdmd		= 0x0008,	/* transmit demand */
	Txon		= 0x0010,	/* transmitter on */
	Rxon		= 0x0020,	/* receiver on */
	Iena		= 0x0040,	/* interrupt enable */
	Intr		= 0x0080,	/* interrupt flag */
	Idon		= 0x0100,	/* initialisation done */
	Tint		= 0x0200,	/* transmit interrupt */
	Rint		= 0x0400,	/* receive interrupt */
	Merr		= 0x0800,	/* memory error */
	Miss		= 0x1000,	/* missed frame */
	Cerr		= 0x2000,	/* collision */
	Babl		= 0x4000,	/* transmitter timeout */
	Err		= 0x8000,	/* Babl|Cerr|Miss|Merr */
};
	
enum {					/* CSR3 */
	Bswp		= 0x0004,	/* byte swap */
	Emba		= 0x0008,	/* enable modified back-off algorithm */
	Dxmt2pd		= 0x0010,	/* disable transmit two part deferral */
	Lappen		= 0x0020,	/* look-ahead packet processing enable */
};

enum {					/* CSR4 */
	ApadXmt		= 0x0800,	/* auto pad transmit */
};

enum {					/* CSR15 */
	Prom		= 0x8000,	/* promiscuous mode */
};

typedef struct {			/* Initialisation Block */
	ushort	mode;
	uchar	rlen;			/* upper 4 bits */
	uchar	tlen;			/* upper 4 bits */
	uchar	padr[6];
	uchar	res[2];
	uchar	ladr[8];
	ulong	rdra;
	ulong	tdra;
} Iblock;

typedef struct {			/* descriptor ring entry */
	ulong	addr;
	ulong	md1;			/* status|bcnt */
	ulong	md2;			/* rcc|rpc|mcnt */
	Block*	bp;
} Dre;

enum {					/* md1 */
	Enp		= 0x01000000,	/* end of packet */
	Stp		= 0x02000000,	/* start of packet */
	RxBuff		= 0x04000000,	/* buffer error */
	Def		= 0x04000000,	/* deferred */
	Crc		= 0x08000000,	/* CRC error */
	One		= 0x08000000,	/* one retry needed */
	Oflo		= 0x10000000,	/* overflow error */
	More		= 0x10000000,	/* more than one retry needed */
	Fram		= 0x20000000,	/* framing error */
	RxErr		= 0x40000000,	/* Fram|Oflo|Crc|RxBuff */
	TxErr		= 0x40000000,	/* Uflo|Lcol|Lcar|Rtry */
	Own		= 0x80000000,
};

enum {					/* md2 */
	Rtry		= 0x04000000,	/* failed after repeated retries */
	Lcar		= 0x08000000,	/* loss of carrier */
	Lcol		= 0x10000000,	/* late collision */
	Uflo		= 0x40000000,	/* underflow error */
	TxBuff		= 0x80000000,	/* buffer error */
};

typedef struct Ctlr {
	Lock;
	int	port;

	int	init;			/* initialisation in progress */
	Iblock	iblock;

	Dre*	rdr;			/* receive descriptor ring */
	int	rdrx;

	Dre*	tdr;			/* transmit descriptor ring */
	int	tdrh;			/* host index into tdr */
	int	tdri;			/* interface index into tdr */
	int	ntq;			/* descriptors active */

	ulong	rxbuff;			/* receive statistics */
	ulong	crc;
	ulong	oflo;
	ulong	fram;

	ulong	rtry;			/* transmit statistics */
	ulong	lcar;
	ulong	lcol;
	ulong	uflo;
	ulong	txbuff;

	ulong	merr;			/* bobf is such a whiner */
	ulong	miss;
	ulong	babl;
} Ctlr;

#define csr32r(c, r)	(inl((c)->port+(r)))
#define csr32w(c, r, l)	(outl((c)->port+(r), (ulong)(l)))

static void
attach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;

	ilock(ctlr);
	if(ctlr->init){
		iunlock(ctlr);
		return;
	}
	csr32w(ctlr, Rdp, Iena|Strt);
	iunlock(ctlr);
}

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	char *p;
	int len;
	Ctlr *ctlr;

	ctlr = ether->ctlr;

	ether->crcs = ctlr->crc;
	ether->frames = ctlr->fram;
	ether->buffs = ctlr->rxbuff+ctlr->txbuff;
	ether->overflows = ctlr->oflo;

	if(n == 0)
		return 0;

	p = malloc(READSTR);
	len = snprint(p, READSTR, "Rxbuff: %ld\n", ctlr->rxbuff);
	len += snprint(p+len, READSTR-len, "Crc: %ld\n", ctlr->crc);
	len += snprint(p+len, READSTR-len, "Oflo: %ld\n", ctlr->oflo);
	len += snprint(p+len, READSTR-len, "Fram: %ld\n", ctlr->fram);
	len += snprint(p+len, READSTR-len, "Rtry: %ld\n", ctlr->rtry);
	len += snprint(p+len, READSTR-len, "Lcar: %ld\n", ctlr->lcar);
	len += snprint(p+len, READSTR-len, "Lcol: %ld\n", ctlr->lcol);
	len += snprint(p+len, READSTR-len, "Uflo: %ld\n", ctlr->uflo);
	len += snprint(p+len, READSTR-len, "Txbuff: %ld\n", ctlr->txbuff);
	len += snprint(p+len, READSTR-len, "Merr: %ld\n", ctlr->merr);
	len += snprint(p+len, READSTR-len, "Miss: %ld\n", ctlr->miss);
	snprint(p+len, READSTR-len, "Babl: %ld\n", ctlr->babl);

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
ringinit(Ctlr* ctlr)
{
	Dre *dre;

	/*
	 * Initialise the receive and transmit buffer rings.
	 * The ring entries must be aligned on 16-byte boundaries.
	 *
	 * This routine is protected by ctlr->init.
	 */
	if(ctlr->rdr == 0){
		ctlr->rdr = xspanalloc(Nrdre*sizeof(Dre), 0x10, 0);
		for(dre = ctlr->rdr; dre < &ctlr->rdr[Nrdre]; dre++){
			dre->bp = iallocb(Rbsize);
			dre->addr = PADDR(dre->bp->rp);
			dre->md2 = 0;
			dre->md1 = Own|(-Rbsize & 0xFFFF);
		}
	}
	ctlr->rdrx = 0;

	if(ctlr->tdr == 0)
		ctlr->tdr = xspanalloc(Ntdre*sizeof(Dre), 0x10, 0);
	memset(ctlr->tdr, 0, Ntdre*sizeof(Dre));
	ctlr->tdrh = ctlr->tdri = 0;
}

static void
promiscuous(void* arg, int on)
{
	Ether *ether;
	int x;
	Ctlr *ctlr;

	ether = arg;
	ctlr = ether->ctlr;

	/*
	 * Put the chip into promiscuous mode. First must wait until
	 * anyone transmitting is done, then stop the chip and put
	 * it in promiscuous mode. Restarting is made harder by the chip
	 * reloading the transmit and receive descriptor pointers with their
	 * base addresses when Strt is set (unlike the older Lance chip),
	 * so the rings must be re-initialised.
	 */
	ilock(ctlr);
	if(ctlr->init){
		iunlock(ctlr);
		return;
	}
	ctlr->init = 1;
	iunlock(ctlr);

	while(ctlr->ntq)
		;

	csr32w(ctlr, Rdp, Stop);

	csr32w(ctlr, Rap, 15);
	x = csr32r(ctlr, Rdp) & ~Prom;
	if(on)
		x |= Prom;
	csr32w(ctlr, Rdp, x);
	csr32w(ctlr, Rap, 0);

	ringinit(ctlr);

	ilock(ctlr);
	ctlr->init = 0;
	csr32w(ctlr, Rdp, Iena|Strt);
	iunlock(ctlr);
}

static void
txstart(Ether* ether)
{
	Ctlr *ctlr;
	Block *bp;
	Dre *dre;

	ctlr = ether->ctlr;

	if(ctlr->init)
		return;

	while(ctlr->ntq < (Ntdre-1)){
		bp = qget(ether->oq);
		if(bp == nil)
			break;

		/*
		 * Give ownership of the descriptor to the chip,
		 * increment the software ring descriptor pointer
		 * and tell the chip to poll.
		 * There's no need to pad to ETHERMINTU
		 * here as ApadXmit is set in CSR4.
		 */
		dre = &ctlr->tdr[ctlr->tdrh];
		dre->bp = bp;
		dre->addr = PADDR(bp->rp);
		dre->md2 = 0;
		dre->md1 = Own|Stp|Enp|(-BLEN(bp) & 0xFFFF);
		ctlr->ntq++;
		csr32w(ctlr, Rdp, Iena|Tdmd);
		ctlr->tdrh = NEXT(ctlr->tdrh, Ntdre);
	}
}

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(ctlr);
	txstart(ether);
	iunlock(ctlr);
}

static void
interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Ether *ether;
	int csr0, len;
	Dre *dre;
	Block *bp;

	ether = arg;
	ctlr = ether->ctlr;

	/*
	 * Acknowledge all interrupts and whine about those that shouldn't
	 * happen.
	 */
intrloop:
	csr0 = csr32r(ctlr, Rdp) & 0xFFFF;
	csr32w(ctlr, Rdp, Babl|Cerr|Miss|Merr|Rint|Tint|Iena);
	if(csr0 & Merr)
		ctlr->merr++;
	if(csr0 & Miss)
		ctlr->miss++;
	if(csr0 & Babl)
		ctlr->babl++;
	//if(csr0 & (Babl|Miss|Merr))
	//	print("#l%d: csr0 = 0x%uX\n", ether->ctlrno, csr0);
	if(!(csr0 & (Rint|Tint)))
		return;

	/*
	 * Receiver interrupt: run round the descriptor ring logging
	 * errors and passing valid receive data up to the higher levels
	 * until a descriptor is encountered still owned by the chip.
	 */
	if(csr0 & Rint){
		dre = &ctlr->rdr[ctlr->rdrx];
		while(!(dre->md1 & Own)){
			if(dre->md1 & RxErr){
				if(dre->md1 & RxBuff)
					ctlr->rxbuff++;
				if(dre->md1 & Crc)
					ctlr->crc++;
				if(dre->md1 & Oflo)
					ctlr->oflo++;
				if(dre->md1 & Fram)
					ctlr->fram++;
			}
			else if(bp = iallocb(Rbsize)){
				len = (dre->md2 & 0x0FFF)-4;
				dre->bp->wp = dre->bp->rp+len;
				etheriq(ether, dre->bp, 1);
				dre->bp = bp;
				dre->addr = PADDR(bp->rp);
			}

			/*
			 * Finished with this descriptor, reinitialise it,
			 * give it back to the chip, then on to the next...
			 */
			dre->md2 = 0;
			dre->md1 = Own|(-Rbsize & 0xFFFF);

			ctlr->rdrx = NEXT(ctlr->rdrx, Nrdre);
			dre = &ctlr->rdr[ctlr->rdrx];
		}
	}

	/*
	 * Transmitter interrupt: wakeup anyone waiting for a free descriptor.
	 */
	if(csr0 & Tint){
		lock(ctlr);
		while(ctlr->ntq){
			dre = &ctlr->tdr[ctlr->tdri];
			if(dre->md1 & Own)
				break;
	
			if(dre->md1 & TxErr){
				if(dre->md2 & Rtry)
					ctlr->rtry++;
				if(dre->md2 & Lcar)
					ctlr->lcar++;
				if(dre->md2 & Lcol)
					ctlr->lcol++;
				if(dre->md2 & Uflo)
					ctlr->uflo++;
				if(dre->md2 & TxBuff)
					ctlr->txbuff++;
				ether->oerrs++;
			}
	
			freeb(dre->bp);
	
			ctlr->ntq--;
			ctlr->tdri = NEXT(ctlr->tdri, Ntdre);
		}
		txstart(ether);
		unlock(ctlr);
	}
	goto intrloop;
}

typedef struct Adapter {
	int	port;
	int	irq;
	int	tbdf;
} Adapter;
static Block* adapter;

static void
amd79c970adapter(Block** bpp, int port, int irq, int tbdf)
{
	Block *bp;
	Adapter *ap;

	bp = allocb(sizeof(Adapter));
	ap = (Adapter*)bp->rp;
	ap->port = port;
	ap->irq = irq;
	ap->tbdf = tbdf;

	bp->next = *bpp;
	*bpp = bp;
}

static void
amd79c970pci(void)
{
	Pcidev *p;
	int port;

	p = nil;
	while(p = pcimatch(p, 0x1022, 0x2000)){
		port = p->mem[0].bar & ~0x01;
		if(ioalloc(port, p->mem[0].size, 0, "amd79c970") < 0){
			print("amd79c970: port 0x%uX in use\n", port);
			continue;
		}
		amd79c970adapter(&adapter, port, p->intl, p->tbdf);
		pcisetbme(p);
	}
}

static int
reset(Ether* ether)
{
	int port, x;
	Block *bp, **bpp;
	Adapter *ap;
	uchar ea[Eaddrlen];
	Ctlr *ctlr;
	static int scandone;

	if(scandone == 0){
		amd79c970pci();
		scandone = 1;
	}

	/*
	 * Any adapter matches if no port is supplied,
	 * otherwise the ports must match.
	 */
	port = 0;
	bpp = &adapter;
	for(bp = *bpp; bp; bp = bp->next){
		ap = (Adapter*)bp->rp;
		if(ether->port == 0 || ether->port == ap->port){
			port = ap->port;
			ether->irq = ap->irq;
			ether->tbdf = ap->tbdf;
			*bpp = bp->next;
			freeb(bp);
			break;
		}
		bpp = &bp->next;
	}
	if(port == 0)
		return -1;

	/*
	 * Allocate a controller structure and start to initialise it.
	 */
	ether->ctlr = malloc(sizeof(Ctlr));
	ctlr = ether->ctlr;
	ilock(ctlr);
	ctlr->init = 1;
	ctlr->port = port;

	/*
	 * How to tell what mode the chip is in at this point - if it's in WORD
	 * mode then the only 32-bit access allowed is a write to the RDP, which
	 * forces the chip to DWORD mode; if it's in DWORD mode then only DWORD
	 * accesses are allowed?
	 * Assuming a DWORD write is done to the RDP, what will be overwritten as
	 * the RAP can't reliably be accessed?
	 *
	 * Force DWORD mode by writing to RDP, doing a reset then writing to RDP
	 * again. The value of RAP after a reset is 0, so the second DWORD write
	 * will be to CSR0.
	 * Set the software style in BCR20 to be PCnet-PCI to ensure 32-bit access.
	 * Set the auto pad transmit in CSR4.
	 */
	csr32w(ctlr, Rdp, 0x00);
	csr32r(ctlr, Sreset);
	csr32w(ctlr, Rdp, Stop);

	csr32w(ctlr, Rap, 20);
	csr32w(ctlr, Bdp, 0x0002);

	csr32w(ctlr, Rap, 4);
	x = csr32r(ctlr, Rdp) & 0xFFFF;
	csr32w(ctlr, Rdp, ApadXmt|x);

	csr32w(ctlr, Rap, 0);

	/*
	 * Check if the adapter's station address is to be over-ridden.
	 * If not, read it from the I/O-space and set in ether->ea prior to loading the
	 * station address in the initialisation block.
	 */
	memset(ea, 0, Eaddrlen);
	if(!memcmp(ea, ether->ea, Eaddrlen)){
		x = csr32r(ctlr, Aprom);
		ether->ea[0] = x;
		ether->ea[1] = x>>8;
		ether->ea[2] = x>>16;
		ether->ea[3] = x>>24;
		x = csr32r(ctlr, Aprom+4);
		ether->ea[4] = x;
		ether->ea[5] = x>>8;
	}

	/*
	 * Start to fill in the initialisation block
	 * (must be DWORD aligned).
	 */
	ctlr->iblock.rlen = Lognrdre<<4;
	ctlr->iblock.tlen = Logntdre<<4;
	memmove(ctlr->iblock.padr, ether->ea, sizeof(ctlr->iblock.padr));

	ringinit(ctlr);
	ctlr->iblock.rdra = PADDR(ctlr->rdr);
	ctlr->iblock.tdra = PADDR(ctlr->tdr);

	/*
	 * Point the chip at the initialisation block and tell it to go.
	 * Mask the Idon interrupt and poll for completion. Strt and interrupt
	 * enables will be set later when attaching to the network.
	 */
	x = PADDR(&ctlr->iblock);
	csr32w(ctlr, Rap, 1);
	csr32w(ctlr, Rdp, x & 0xFFFF);
	csr32w(ctlr, Rap, 2);
	csr32w(ctlr, Rdp, (x>>16) & 0xFFFF);
	csr32w(ctlr, Rap, 3);
	csr32w(ctlr, Rdp, Idon);
	csr32w(ctlr, Rap, 0);
	csr32w(ctlr, Rdp, Init);

	while(!(csr32r(ctlr, Rdp) & Idon))
		;
	csr32w(ctlr, Rdp, Idon|Stop);
	ctlr->init = 0;
	iunlock(ctlr);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->port = port;
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;

	ether->arg = ether;
	ether->promiscuous = promiscuous;

	return 0;
}

void
ether79c970link(void)
{
	addethercard("AMD79C970",  reset);
}
