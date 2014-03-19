/*
 * Intel 82557 Fast Ethernet PCI Bus LAN Controller
 * as found on the Intel EtherExpress PRO/100B. This chip is full
 * of smarts, unfortunately they're not all in the right place.
 * To do:
 *	the PCI scanning code could be made common to other adapters;
 *	auto-negotiation, full-duplex;
 *	optionally use memory-mapped registers;
 *	detach for PCI reset problems (also towards loadable drivers).
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
	/*
	 * these were both 64.  increased them to try to improve lookout's
	 * reliability as a pxe booter.
	 */
	Nrfd		= 128,		/* receive frame area */
	Ncb		= 128,		/* maximum control blocks queued */

	NullPointer	= 0xFFFFFFFF,	/* 82557 NULL pointer */
};

enum {					/* CSR */
	Status		= 0x00,		/* byte or word (word includes Ack) */
	Ack		= 0x01,		/* byte */
	CommandR	= 0x02,		/* byte or word (word includes Interrupt) */
	Interrupt	= 0x03,		/* byte */
	General		= 0x04,		/* dword */
	Port		= 0x08,		/* dword */
	Fcr		= 0x0C,		/* Flash control register */
	Ecr		= 0x0E,		/* EEPROM control register */
	Mcr		= 0x10,		/* MDI control register */
	Gstatus		= 0x1D,		/* General status register */
};

enum {					/* Status */
	RUidle		= 0x0000,
	RUsuspended	= 0x0004,
	RUnoresources	= 0x0008,
	RUready		= 0x0010,
	RUrbd		= 0x0020,	/* bit */
	RUstatus	= 0x003F,	/* mask */

	CUidle		= 0x0000,
	CUsuspended	= 0x0040,
	CUactive	= 0x0080,
	CUstatus	= 0x00C0,	/* mask */

	StatSWI		= 0x0400,	/* SoftWare generated Interrupt */
	StatMDI		= 0x0800,	/* MDI r/w done */
	StatRNR		= 0x1000,	/* Receive unit Not Ready */
	StatCNA		= 0x2000,	/* Command unit Not Active (Active->Idle) */
	StatFR		= 0x4000,	/* Finished Receiving */
	StatCX		= 0x8000,	/* Command eXecuted */
	StatTNO		= 0x8000,	/* Transmit NOT OK */
};

enum {					/* Command (byte) */
	CUnop		= 0x00,
	CUstart		= 0x10,
	CUresume	= 0x20,
	LoadDCA		= 0x40,		/* Load Dump Counters Address */
	DumpSC		= 0x50,		/* Dump Statistical Counters */
	LoadCUB		= 0x60,		/* Load CU Base */
	ResetSA		= 0x70,		/* Dump and Reset Statistical Counters */

	RUstart		= 0x01,
	RUresume	= 0x02,
	RUabort		= 0x04,
	LoadHDS		= 0x05,		/* Load Header Data Size */
	LoadRUB		= 0x06,		/* Load RU Base */
	RBDresume	= 0x07,		/* Resume frame reception */
};

enum {					/* Interrupt (byte) */
	InterruptM	= 0x01,		/* interrupt Mask */
	InterruptSI	= 0x02,		/* Software generated Interrupt */
};

enum {					/* Ecr */
	EEsk		= 0x01,		/* serial clock */
	EEcs		= 0x02,		/* chip select */
	EEdi		= 0x04,		/* serial data in */
	EEdo		= 0x08,		/* serial data out */

	EEstart		= 0x04,		/* start bit */
	EEread		= 0x02,		/* read opcode */
};

enum {					/* Mcr */
	MDIread		= 0x08000000,	/* read opcode */
	MDIwrite	= 0x04000000,	/* write opcode */
	MDIready	= 0x10000000,	/* ready bit */
	MDIie		= 0x20000000,	/* interrupt enable */
};

typedef struct Rfd {
	int	field;
	ulong	link;
	ulong	rbd;
	ushort	count;
	ushort	size;

	uchar	data[1700];
} Rfd;

enum {					/* field */
	RfdCollision	= 0x00000001,
	RfdIA		= 0x00000002,	/* IA match */
	RfdRxerr	= 0x00000010,	/* PHY character error */
	RfdType		= 0x00000020,	/* Type frame */
	RfdRunt		= 0x00000080,
	RfdOverrun	= 0x00000100,
	RfdBuffer	= 0x00000200,
	RfdAlignment	= 0x00000400,
	RfdCRC		= 0x00000800,

	RfdOK		= 0x00002000,	/* frame received OK */
	RfdC		= 0x00008000,	/* reception Complete */
	RfdSF		= 0x00080000,	/* Simplified or Flexible (1) Rfd */
	RfdH		= 0x00100000,	/* Header RFD */

	RfdI		= 0x20000000,	/* Interrupt after completion */
	RfdS		= 0x40000000,	/* Suspend after completion */
	RfdEL		= 0x80000000,	/* End of List */
};

enum {					/* count */
	RfdF		= 0x4000,
	RfdEOF		= 0x8000,
};

typedef struct Cb Cb;
typedef struct Cb {
	ushort	status;
	ushort	command;
	ulong	link;
	union {
		uchar	data[24];	/* CbIAS + CbConfigure */
		struct {
			ulong	tbd;
			ushort	count;
			uchar	threshold;
			uchar	number;

			ulong	tba;
			ushort	tbasz;
			ushort	pad;
		};
	};

	Block*	bp;
	Cb*	next;
} Cb;

enum {					/* action command */
	CbU		= 0x1000,	/* transmit underrun */
	CbOK		= 0x2000,	/* DMA completed OK */
	CbC		= 0x8000,	/* execution Complete */

	CbNOP		= 0x0000,
	CbIAS		= 0x0001,	/* Individual Address Setup */
	CbConfigure	= 0x0002,
	CbMAS		= 0x0003,	/* Multicast Address Setup */
	CbTransmit	= 0x0004,
	CbDump		= 0x0006,
	CbDiagnose	= 0x0007,
	CbCommand	= 0x0007,	/* mask */

	CbSF		= 0x0008,	/* Flexible-mode CbTransmit */

	CbI		= 0x2000,	/* Interrupt after completion */
	CbS		= 0x4000,	/* Suspend after completion */
	CbEL		= 0x8000,	/* End of List */
};

enum {					/* CbTransmit count */
	CbEOF		= 0x8000,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	Lock	slock;			/* attach */
	int	state;

	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;

	int	eepromsz;		/* address size in bits */
	ushort*	eeprom;

	Lock	miilock;

	int	tick;

	Lock	rlock;			/* registers */
	int	command;		/* last command issued */

	Block*	rfdhead;		/* receive side */
	Block*	rfdtail;
	int	nrfd;

	Lock	cblock;			/* transmit side */
	int	action;
	int	nop;
	uchar	configdata[24];
	int	threshold;
	int	ncb;
	Cb*	cbr;
	Cb*	cbhead;
	Cb*	cbtail;
	int	cbq;
	int	cbqmax;
	int	cbqmaxhw;

	Lock	dlock;			/* dump statistical counters */
	ulong	dump[17];
} Ctlr;

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

static uchar configdata[24] = {
	0x16,				/* byte count */
	0x08,				/* Rx/Tx FIFO limit */
	0x00,				/* adaptive IFS */
	0x00,	
	0x00,				/* Rx DMA maximum byte count */
//	0x80,				/* Tx DMA maximum byte count */
	0x00,				/* Tx DMA maximum byte count */
	0x32,				/* !late SCB, CNA interrupts */
	0x03,				/* discard short Rx frames */
	0x00,				/* 503/MII */

	0x00,	
	0x2E,				/* normal operation, NSAI */
	0x00,				/* linear priority */
	0x60,				/* inter-frame spacing */
	0x00,	
	0xF2,	
	0xC8,				/* 503, promiscuous mode off */
	0x00,	
	0x40,	
	0xF3,				/* transmit padding enable */
	0x80,				/* full duplex pin enable */
	0x3F,				/* no Multi IA */
	0x05,				/* no Multi Cast ALL */
};

#define csr8r(c, r)	(inb((c)->port+(r)))
#define csr16r(c, r)	(ins((c)->port+(r)))
#define csr32r(c, r)	(inl((c)->port+(r)))
#define csr8w(c, r, b)	(outb((c)->port+(r), (int)(b)))
#define csr16w(c, r, w)	(outs((c)->port+(r), (ushort)(w)))
#define csr32w(c, r, l)	(outl((c)->port+(r), (ulong)(l)))

static void
command(Ctlr* ctlr, int c, int v)
{
	int timeo;

	ilock(&ctlr->rlock);

	/*
	 * Only back-to-back CUresume can be done
	 * without waiting for any previous command to complete.
	 * This should be the common case.
	 * Unfortunately there's a chip errata where back-to-back
	 * CUresumes can be lost, the fix is to always wait.
	if(c == CUresume && ctlr->command == CUresume){
		csr8w(ctlr, CommandR, c);
		iunlock(&ctlr->rlock);
		return;
	}
	 */

	for(timeo = 0; timeo < 100; timeo++){
		if(!csr8r(ctlr, CommandR))
			break;
		microdelay(1);
	}
	if(timeo >= 100){
		ctlr->command = -1;
		iunlock(&ctlr->rlock);
		iprint("i82557: command %#ux %#ux timeout\n", c, v);
		return;
	}

	switch(c){

	case CUstart:
	case LoadDCA:
	case LoadCUB:
	case RUstart:
	case LoadHDS:
	case LoadRUB:
		csr32w(ctlr, General, v);
		break;

	/*
	case CUnop:
	case CUresume:
	case DumpSC:
	case ResetSA:
	case RUresume:
	case RUabort:
	 */
	default:
		break;
	}
	csr8w(ctlr, CommandR, c);
	ctlr->command = c;

	iunlock(&ctlr->rlock);
}

static Block*
rfdalloc(ulong link)
{
	Block *bp;
	Rfd *rfd;

	if(bp = iallocb(sizeof(Rfd))){
		rfd = (Rfd*)bp->rp;
		rfd->field = 0;
		rfd->link = link;
		rfd->rbd = NullPointer;
		rfd->count = 0;
		rfd->size = sizeof(Etherpkt);
	}

	return bp;
}

static void
ethwatchdog(void* arg)
{
	Ether *ether;
	Ctlr *ctlr;
	static void txstart(Ether*);

	ether = arg;
	for(;;){
		tsleep(&up->sleep, return0, 0, 4000);

		/*
		 * Hmmm. This doesn't seem right. Currently
		 * the device can't be disabled but it may be in
		 * the future.
		 */
		ctlr = ether->ctlr;
		if(ctlr == nil || ctlr->state == 0){
			print("%s: exiting\n", up->text);
			pexit("disabled", 0);
		}

		ilock(&ctlr->cblock);
		if(ctlr->tick++){
			ctlr->action = CbMAS;
			txstart(ether);
		}
		iunlock(&ctlr->cblock);
	}
}

static void
attach(Ether* ether)
{
	Ctlr *ctlr;
	char name[KNAMELEN];

	ctlr = ether->ctlr;
	lock(&ctlr->slock);
	if(ctlr->state == 0){
		ilock(&ctlr->rlock);
		csr8w(ctlr, Interrupt, 0);
		iunlock(&ctlr->rlock);
		command(ctlr, RUstart, PADDR(ctlr->rfdhead->rp));
		ctlr->state = 1;

		/*
		 * Start the watchdog timer for the receive lockup errata
		 * unless the EEPROM compatibility word indicates it may be
		 * omitted.
		 */
		if((ctlr->eeprom[0x03] & 0x0003) != 0x0003){
			snprint(name, KNAMELEN, "#l%dwatchdog", ether->ctlrno);
			kproc(name, ethwatchdog, ether);
		}
	}
	unlock(&ctlr->slock);
}

static long
ifstat(Ether* ether, void* a, long n, ulong offset)
{
	char *p;
	int i, len, phyaddr;
	Ctlr *ctlr;
	ulong dump[17];

	ctlr = ether->ctlr;
	lock(&ctlr->dlock);

	/*
	 * Start the command then
	 * wait for completion status,
	 * should be 0xA005.
	 */
	ctlr->dump[16] = 0;
	command(ctlr, DumpSC, 0);
	while(ctlr->dump[16] == 0)
		;

	ether->oerrs = ctlr->dump[1]+ctlr->dump[2]+ctlr->dump[3];
	ether->crcs = ctlr->dump[10];
	ether->frames = ctlr->dump[11];
	ether->buffs = ctlr->dump[12]+ctlr->dump[15];
	ether->overflows = ctlr->dump[13];

	if(n == 0){
		unlock(&ctlr->dlock);
		return 0;
	}

	memmove(dump, ctlr->dump, sizeof(dump));
	unlock(&ctlr->dlock);

	p = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	len = snprint(p, READSTR, "transmit good frames: %lud\n", dump[0]);
	len += snprint(p+len, READSTR-len, "transmit maximum collisions errors: %lud\n", dump[1]);
	len += snprint(p+len, READSTR-len, "transmit late collisions errors: %lud\n", dump[2]);
	len += snprint(p+len, READSTR-len, "transmit underrun errors: %lud\n", dump[3]);
	len += snprint(p+len, READSTR-len, "transmit lost carrier sense: %lud\n", dump[4]);
	len += snprint(p+len, READSTR-len, "transmit deferred: %lud\n", dump[5]);
	len += snprint(p+len, READSTR-len, "transmit single collisions: %lud\n", dump[6]);
	len += snprint(p+len, READSTR-len, "transmit multiple collisions: %lud\n", dump[7]);
	len += snprint(p+len, READSTR-len, "transmit total collisions: %lud\n", dump[8]);
	len += snprint(p+len, READSTR-len, "receive good frames: %lud\n", dump[9]);
	len += snprint(p+len, READSTR-len, "receive CRC errors: %lud\n", dump[10]);
	len += snprint(p+len, READSTR-len, "receive alignment errors: %lud\n", dump[11]);
	len += snprint(p+len, READSTR-len, "receive resource errors: %lud\n", dump[12]);
	len += snprint(p+len, READSTR-len, "receive overrun errors: %lud\n", dump[13]);
	len += snprint(p+len, READSTR-len, "receive collision detect errors: %lud\n", dump[14]);
	len += snprint(p+len, READSTR-len, "receive short frame errors: %lud\n", dump[15]);
	len += snprint(p+len, READSTR-len, "nop: %d\n", ctlr->nop);
	if(ctlr->cbqmax > ctlr->cbqmaxhw)
		ctlr->cbqmaxhw = ctlr->cbqmax;
	len += snprint(p+len, READSTR-len, "cbqmax: %d\n", ctlr->cbqmax);
	ctlr->cbqmax = 0;
	len += snprint(p+len, READSTR-len, "threshold: %d\n", ctlr->threshold);

	len += snprint(p+len, READSTR-len, "eeprom:");
	for(i = 0; i < (1<<ctlr->eepromsz); i++){
		if(i && ((i & 0x07) == 0))
			len += snprint(p+len, READSTR-len, "\n       ");
		len += snprint(p+len, READSTR-len, " %4.4ux", ctlr->eeprom[i]);
	}

	if((ctlr->eeprom[6] & 0x1F00) && !(ctlr->eeprom[6] & 0x8000)){
		phyaddr = ctlr->eeprom[6] & 0x00FF;
		len += snprint(p+len, READSTR-len, "\nphy %2d:", phyaddr);
		for(i = 0; i < 6; i++){
			static int miir(Ctlr*, int, int);

			len += snprint(p+len, READSTR-len, " %4.4ux",
				miir(ctlr, phyaddr, i));
		}
	}

	snprint(p+len, READSTR-len, "\n");
	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static void
txstart(Ether* ether)
{
	Ctlr *ctlr;
	Block *bp;
	Cb *cb;

	ctlr = ether->ctlr;
	while(ctlr->cbq < (ctlr->ncb-1)){
		cb = ctlr->cbhead->next;
		if(ctlr->action == 0){
			bp = qget(ether->oq);
			if(bp == nil)
				break;

			cb->command = CbS|CbSF|CbTransmit;
			cb->tbd = PADDR(&cb->tba);
			cb->count = 0;
			cb->threshold = ctlr->threshold;
			cb->number = 1;
			cb->tba = PADDR(bp->rp);
			cb->bp = bp;
			cb->tbasz = BLEN(bp);
		}
		else if(ctlr->action == CbConfigure){
			cb->command = CbS|CbConfigure;
			memmove(cb->data, ctlr->configdata, sizeof(ctlr->configdata));
			ctlr->action = 0;
		}
		else if(ctlr->action == CbIAS){
			cb->command = CbS|CbIAS;
			memmove(cb->data, ether->ea, Eaddrlen);
			ctlr->action = 0;
		}
		else if(ctlr->action == CbMAS){
			cb->command = CbS|CbMAS;
			memset(cb->data, 0, sizeof(cb->data));
			ctlr->action = 0;
		}
		else{
			print("#l%d: action %#ux\n", ether->ctlrno, ctlr->action);
			ctlr->action = 0;
			break;
		}
		cb->status = 0;

		coherence();
		ctlr->cbhead->command &= ~CbS;
		ctlr->cbhead = cb;
		ctlr->cbq++;
	}

	/*
	 * Workaround for some broken HUB chips
	 * when connected at 10Mb/s half-duplex.
	 */
	if(ctlr->nop){
		command(ctlr, CUnop, 0);
		microdelay(1);
	}
	command(ctlr, CUresume, 0);

	if(ctlr->cbq > ctlr->cbqmax)
		ctlr->cbqmax = ctlr->cbq;
}

static void
configure(Ether* ether, int promiscuous)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->cblock);
	if(promiscuous){
		ctlr->configdata[6] |= 0x80;		/* Save Bad Frames */
		//ctlr->configdata[6] &= ~0x40;		/* !Discard Overrun Rx Frames */
		ctlr->configdata[7] &= ~0x01;		/* !Discard Short Rx Frames */
		ctlr->configdata[15] |= 0x01;		/* Promiscuous mode */
		ctlr->configdata[18] &= ~0x01;		/* (!Padding enable?), !stripping enable */
		ctlr->configdata[21] |= 0x08;		/* Multi Cast ALL */
	}
	else{
		ctlr->configdata[6] &= ~0x80;
		//ctlr->configdata[6] |= 0x40;
		ctlr->configdata[7] |= 0x01;
		ctlr->configdata[15] &= ~0x01;
		ctlr->configdata[18] |= 0x01;		/* 0x03? */
		ctlr->configdata[21] &= ~0x08;
	}
	ctlr->action = CbConfigure;
	txstart(ether);
	iunlock(&ctlr->cblock);
}

static void
promiscuous(void* arg, int on)
{
	configure(arg, on);
}

static void
multicast(void* ether, uchar *addr, int add)
{
	USED(addr);
	/*
	 * TODO: if (add) add addr to list of mcast addrs in controller
	 *	else remove addr from list of mcast addrs in controller
	 * enable multicast input (see CbMAS) instead of promiscuous mode.
	 */
	if (add)
		configure(ether, 1);
}

static void
transmit(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(&ctlr->cblock);
	txstart(ether);
	iunlock(&ctlr->cblock);
}

static void
receive(Ether* ether)
{
	Rfd *rfd;
	Ctlr *ctlr;
	int count;
	Block *bp, *pbp, *xbp;

	ctlr = ether->ctlr;
	bp = ctlr->rfdhead;
	for(rfd = (Rfd*)bp->rp; rfd->field & RfdC; rfd = (Rfd*)bp->rp){
		/*
		 * If it's an OK receive frame
		 * 1) save the count 
		 * 2) if it's small, try to allocate a block and copy
		 *    the data, then adjust the necessary fields for reuse;
		 * 3) if it's big, try to allocate a new Rfd and if
		 *    successful
		 *	adjust the received buffer pointers for the
		 *	  actual data received;
		 *	initialise the replacement buffer to point to
		 *	  the next in the ring;
		 *	initialise bp to point to the replacement;
		 * 4) if there's a good packet, pass it on for disposal.
		 */
		if(rfd->field & RfdOK){
			pbp = nil;
			count = rfd->count & 0x3FFF;
			if((count < ETHERMAXTU/4) && (pbp = iallocb(count))){
				memmove(pbp->rp, bp->rp+offsetof(Rfd, data[0]), count);
				pbp->wp = pbp->rp + count;

				rfd->count = 0;
				rfd->field = 0;
			}
			else if(xbp = rfdalloc(rfd->link)){
				bp->rp += offsetof(Rfd, data[0]);
				bp->wp = bp->rp + count;

				xbp->next = bp->next;
				bp->next = 0;

				pbp = bp;
				bp = xbp;
			}
			if(pbp != nil)
				etheriq(ether, pbp, 1);
		}
		else{
			rfd->count = 0;
			rfd->field = 0;
		}

		/*
		 * The ring tail pointer follows the head with with one
		 * unused buffer in between to defeat hardware prefetch;
		 * once the tail pointer has been bumped on to the next
		 * and the new tail has the Suspend bit set, it can be
		 * removed from the old tail buffer.
		 * As a replacement for the current head buffer may have
		 * been allocated above, ensure that the new tail points
		 * to it (next and link).
		 */
		rfd = (Rfd*)ctlr->rfdtail->rp;
		ctlr->rfdtail = ctlr->rfdtail->next;
		ctlr->rfdtail->next = bp;
		((Rfd*)ctlr->rfdtail->rp)->link = PADDR(bp->rp);
		((Rfd*)ctlr->rfdtail->rp)->field |= RfdS;
		coherence();
		rfd->field &= ~RfdS;

		/*
		 * Finally done with the current (possibly replaced)
		 * head, move on to the next and maintain the sentinel
		 * between tail and head.
		 */
		ctlr->rfdhead = bp->next;
		bp = ctlr->rfdhead;
	}
}

static void
interrupt(Ureg*, void* arg)
{
	Cb* cb;
	Ctlr *ctlr;
	Ether *ether;
	int status;

	ether = arg;
	ctlr = ether->ctlr;

	for(;;){
		ilock(&ctlr->rlock);
		status = csr16r(ctlr, Status);
		csr8w(ctlr, Ack, (status>>8) & 0xFF);
		iunlock(&ctlr->rlock);

		if(!(status & (StatCX|StatFR|StatCNA|StatRNR|StatMDI|StatSWI)))
			break;

		/*
		 * If the watchdog timer for the receiver lockup errata is running,
		 * let it know the receiver is active.
		 */
		if(status & (StatFR|StatRNR)){
			ilock(&ctlr->cblock);
			ctlr->tick = 0;
			iunlock(&ctlr->cblock);
		}

		if(status & StatFR){
			receive(ether);
			status &= ~StatFR;
		}

		if(status & StatRNR){
			command(ctlr, RUresume, 0);
			status &= ~StatRNR;
		}

		if(status & StatCNA){
			ilock(&ctlr->cblock);

			cb = ctlr->cbtail;
			while(ctlr->cbq){
				if(!(cb->status & CbC))
					break;
				if(cb->bp){
					freeb(cb->bp);
					cb->bp = nil;
				}
				if((cb->status & CbU) && ctlr->threshold < 0xE0)
					ctlr->threshold++;

				ctlr->cbq--;
				cb = cb->next;
			}
			ctlr->cbtail = cb;

			txstart(ether);
			iunlock(&ctlr->cblock);

			status &= ~StatCNA;
		}

		if(status & (StatCX|StatFR|StatCNA|StatRNR|StatMDI|StatSWI))
			panic("#l%d: status %#ux\n", ether->ctlrno, status);
	}
}

static void
ctlrinit(Ctlr* ctlr)
{
	int i;
	Block *bp;
	Rfd *rfd;
	ulong link;

	/*
	 * Create the Receive Frame Area (RFA) as a ring of allocated
	 * buffers.
	 * A sentinel buffer is maintained between the last buffer in
	 * the ring (marked with RfdS) and the head buffer to defeat the
	 * hardware prefetch of the next RFD and allow dynamic buffer
	 * allocation.
	 */
	link = NullPointer;
	for(i = 0; i < Nrfd; i++){
		bp = rfdalloc(link);
		if(ctlr->rfdhead == nil)
			ctlr->rfdtail = bp;
		bp->next = ctlr->rfdhead;
		ctlr->rfdhead = bp;
		link = PADDR(bp->rp);
	}
	ctlr->rfdtail->next = ctlr->rfdhead;
	rfd = (Rfd*)ctlr->rfdtail->rp;
	rfd->link = PADDR(ctlr->rfdhead->rp);
	rfd->field |= RfdS;
	ctlr->rfdhead = ctlr->rfdhead->next;

	/*
	 * Create a ring of control blocks for the
	 * transmit side.
	 */
	ilock(&ctlr->cblock);
	ctlr->cbr = malloc(ctlr->ncb*sizeof(Cb));
	if(ctlr->cbr == nil) {
		iunlock(&ctlr->cblock);
		error(Enomem);
	}
	for(i = 0; i < ctlr->ncb; i++){
		ctlr->cbr[i].status = CbC|CbOK;
		ctlr->cbr[i].command = CbS|CbNOP;
		ctlr->cbr[i].link = PADDR(&ctlr->cbr[NEXT(i, ctlr->ncb)].status);
		ctlr->cbr[i].next = &ctlr->cbr[NEXT(i, ctlr->ncb)];
	}
	ctlr->cbhead = ctlr->cbr;
	ctlr->cbtail = ctlr->cbr;
	ctlr->cbq = 0;

	memmove(ctlr->configdata, configdata, sizeof(configdata));
	ctlr->threshold = 80;
	ctlr->tick = 0;

	iunlock(&ctlr->cblock);
}

static int
miir(Ctlr* ctlr, int phyadd, int regadd)
{
	int mcr, timo;

	lock(&ctlr->miilock);
	csr32w(ctlr, Mcr, MDIread|(phyadd<<21)|(regadd<<16));
	mcr = 0;
	for(timo = 64; timo; timo--){
		mcr = csr32r(ctlr, Mcr);
		if(mcr & MDIready)
			break;
		microdelay(1);
	}
	unlock(&ctlr->miilock);

	if(mcr & MDIready)
		return mcr & 0xFFFF;

	return -1;
}

static int
miiw(Ctlr* ctlr, int phyadd, int regadd, int data)
{
	int mcr, timo;

	lock(&ctlr->miilock);
	csr32w(ctlr, Mcr, MDIwrite|(phyadd<<21)|(regadd<<16)|(data & 0xFFFF));
	mcr = 0;
	for(timo = 64; timo; timo--){
		mcr = csr32r(ctlr, Mcr);
		if(mcr & MDIready)
			break;
		microdelay(1);
	}
	unlock(&ctlr->miilock);

	if(mcr & MDIready)
		return 0;

	return -1;
}

static int
hy93c46r(Ctlr* ctlr, int r)
{
	int data, i, op, size;

	/*
	 * Hyundai HY93C46 or equivalent serial EEPROM.
	 * This sequence for reading a 16-bit register 'r'
	 * in the EEPROM is taken straight from Section
	 * 3.3.4.2 of the Intel 82557 User's Guide.
	 */
reread:
	csr16w(ctlr, Ecr, EEcs);
	op = EEstart|EEread;
	for(i = 2; i >= 0; i--){
		data = (((op>>i) & 0x01)<<2)|EEcs;
		csr16w(ctlr, Ecr, data);
		csr16w(ctlr, Ecr, data|EEsk);
		microdelay(1);
		csr16w(ctlr, Ecr, data);
		microdelay(1);
	}

	/*
	 * First time through must work out the EEPROM size.
	 */
	if((size = ctlr->eepromsz) == 0)
		size = 8;

	for(size = size-1; size >= 0; size--){
		data = (((r>>size) & 0x01)<<2)|EEcs;
		csr16w(ctlr, Ecr, data);
		csr16w(ctlr, Ecr, data|EEsk);
		delay(1);
		csr16w(ctlr, Ecr, data);
		microdelay(1);
		if(!(csr16r(ctlr, Ecr) & EEdo))
			break;
	}

	data = 0;
	for(i = 15; i >= 0; i--){
		csr16w(ctlr, Ecr, EEcs|EEsk);
		microdelay(1);
		if(csr16r(ctlr, Ecr) & EEdo)
			data |= (1<<i);
		csr16w(ctlr, Ecr, EEcs);
		microdelay(1);
	}

	csr16w(ctlr, Ecr, 0);

	if(ctlr->eepromsz == 0){
		ctlr->eepromsz = 8-size;
		ctlr->eeprom = malloc((1<<ctlr->eepromsz)*sizeof(ushort));
		if(ctlr->eeprom == nil)
			error(Enomem);
		goto reread;
	}

	return data;
}

static void
i82557pci(void)
{
	Pcidev *p;
	Ctlr *ctlr;
	int i, nop, port;

	p = nil;
	nop = 0;
	while(p = pcimatch(p, 0x8086, 0)){
		switch(p->did){
		default:
			continue;
		case 0x1031:		/* Intel 82562EM */
		case 0x103B:		/* Intel 82562EM */
		case 0x103C:		/* Intel 82562EM */
		case 0x1050:		/* Intel 82562EZ */
		case 0x1039:		/* Intel 82801BD PRO/100 VE */
		case 0x103A:		/* Intel 82562 PRO/100 VE */
		case 0x103D:		/* Intel 82562 PRO/100 VE */
		case 0x1064:		/* Intel 82562 PRO/100 VE */
		case 0x2449:		/* Intel 82562ET */
		case 0x27DC:		/* Intel 82801G PRO/100 VE */
			nop = 1;
			/*FALLTHROUGH*/
		case 0x1209:		/* Intel 82559ER */
		case 0x1229:		/* Intel 8255[789] */
		case 0x1030:		/* Intel 82559 InBusiness 10/100  */
			break;
		}

		if(pcigetpms(p) > 0){
			pcisetpms(p, 0);
	
			for(i = 0; i < 6; i++)
				pcicfgw32(p, PciBAR0+i*4, p->mem[i].bar);
			pcicfgw8(p, PciINTL, p->intl);
			pcicfgw8(p, PciLTR, p->ltr);
			pcicfgw8(p, PciCLS, p->cls);
			pcicfgw16(p, PciPCR, p->pcr);
		}

		/*
		 * bar[0] is the memory-mapped register address (4KB),
		 * bar[1] is the I/O port register address (32 bytes) and
		 * bar[2] is for the flash ROM (1MB).
		 */
		port = p->mem[1].bar & ~0x01;
		if(ioalloc(port, p->mem[1].size, 0, "i82557") < 0){
			print("i82557: port %#ux in use\n", port);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil)
			error(Enomem);
		ctlr->port = port;
		ctlr->pcidev = p;
		ctlr->nop = nop;

		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;

		pcisetbme(p);
	}
}

static char* mediatable[9] = {
	"10BASE-T",				/* TP */
	"10BASE-2",				/* BNC */
	"10BASE-5",				/* AUI */
	"100BASE-TX",
	"10BASE-TFD",
	"100BASE-TXFD",
	"100BASE-T4",
	"100BASE-FX",
	"100BASE-FXFD",
};

static int
scanphy(Ctlr* ctlr)
{
	int i, oui, x;

	for(i = 0; i < 32; i++){
		if((oui = miir(ctlr, i, 2)) == -1 || oui == 0 || oui == 0xFFFF)
			continue;
		oui <<= 6;
		x = miir(ctlr, i, 3);
		oui |= x>>10;
		//print("phy%d: oui %#ux reg1 %#ux\n", i, oui, miir(ctlr, i, 1));

		ctlr->eeprom[6] = i;
		if(oui == 0xAA00)
			ctlr->eeprom[6] |= 0x07<<8;
		else if(oui == 0x80017){
			if(x & 0x01)
				ctlr->eeprom[6] |= 0x0A<<8;
			else
				ctlr->eeprom[6] |= 0x04<<8;
		}
		return i;
	}
	return -1;
}

static void
shutdown(Ether* ether)
{
	Ctlr *ctlr = ether->ctlr;

print("ether82557 shutting down\n");
	csr32w(ctlr, Port, 0);
	delay(1);
	csr8w(ctlr, Interrupt, InterruptM);
}


static int
reset(Ether* ether)
{
	int anar, anlpar, bmcr, bmsr, i, k, medium, phyaddr, x;
	unsigned short sum;
	uchar ea[Eaddrlen];
	Ctlr *ctlr;

	if(ctlrhead == nil)
		i82557pci();

	/*
	 * Any adapter matches if no ether->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(ether->port == 0 || ether->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	/*
	 * Initialise the Ctlr structure.
	 * Perform a software reset after which should ensure busmastering
	 * is still enabled. The EtherExpress PRO/100B appears to leave
	 * the PCI configuration alone (see the 'To do' list above) so punt
	 * for now.
	 * Load the RUB and CUB registers for linear addressing (0).
	 */
	ether->ctlr = ctlr;
	ether->port = ctlr->port;
	ether->irq = ctlr->pcidev->intl;
	ether->tbdf = ctlr->pcidev->tbdf;

	ilock(&ctlr->rlock);
	csr32w(ctlr, Port, 0);
	delay(1);
	csr8w(ctlr, Interrupt, InterruptM);
	iunlock(&ctlr->rlock);

	command(ctlr, LoadRUB, 0);
	command(ctlr, LoadCUB, 0);
	command(ctlr, LoadDCA, PADDR(ctlr->dump));

	/*
	 * Initialise the receive frame, transmit ring and configuration areas.
	 */
	ctlr->ncb = Ncb;
	ctlrinit(ctlr);

	/*
	 * Read the EEPROM.
	 * Do a dummy read first to get the size
	 * and allocate ctlr->eeprom.
	 */
	hy93c46r(ctlr, 0);
	sum = 0;
	for(i = 0; i < (1<<ctlr->eepromsz); i++){
		x = hy93c46r(ctlr, i);
		ctlr->eeprom[i] = x;
		sum += x;
	}
	if(sum != 0xBABA)
		print("#l%d: EEPROM checksum - %#4.4ux\n", ether->ctlrno, sum);

	/*
	 * Eeprom[6] indicates whether there is a PHY and whether
	 * it's not 10Mb-only, in which case use the given PHY address
	 * to set any PHY specific options and determine the speed.
	 * Unfortunately, sometimes the EEPROM is blank except for
	 * the ether address and checksum; in this case look at the
	 * controller type and if it's am 82558 or 82559 it has an
	 * embedded PHY so scan for that.
	 * If no PHY, assume 82503 (serial) operation.
	 */
	if((ctlr->eeprom[6] & 0x1F00) && !(ctlr->eeprom[6] & 0x8000))
		phyaddr = ctlr->eeprom[6] & 0x00FF;
	else
	switch(ctlr->pcidev->rid){
	case 0x01:			/* 82557 A-step */
	case 0x02:			/* 82557 B-step */
	case 0x03:			/* 82557 C-step */
	default:
		phyaddr = -1;
		break;
	case 0x04:			/* 82558 A-step */
	case 0x05:			/* 82558 B-step */
	case 0x06:			/* 82559 A-step */
	case 0x07:			/* 82559 B-step */
	case 0x08:			/* 82559 C-step */
	case 0x09:			/* 82559ER A-step */
		phyaddr = scanphy(ctlr);
		break;
	}
	if(phyaddr >= 0){
		/*
		 * Resolve the highest common ability of the two
		 * link partners. In descending order:
		 *	0x0100		100BASE-TX Full Duplex
		 *	0x0200		100BASE-T4
		 *	0x0080		100BASE-TX
		 *	0x0040		10BASE-T Full Duplex
		 *	0x0020		10BASE-T
		 */
		anar = miir(ctlr, phyaddr, 0x04);
		anlpar = miir(ctlr, phyaddr, 0x05) & 0x03E0;
		anar &= anlpar;
		bmcr = 0;
		if(anar & 0x380)
			bmcr = 0x2000;
		if(anar & 0x0140)
			bmcr |= 0x0100;

		switch((ctlr->eeprom[6]>>8) & 0x001F){

		case 0x04:				/* DP83840 */
		case 0x0A:				/* DP83840A */
			/*
			 * The DP83840[A] requires some tweaking for
			 * reliable operation.
			 * The manual says bit 10 should be unconditionally
			 * set although it supposedly only affects full-duplex
			 * operation (an & 0x0140).
			 */
			x = miir(ctlr, phyaddr, 0x17) & ~0x0520;
			x |= 0x0420;
			for(i = 0; i < ether->nopt; i++){
				if(cistrcmp(ether->opt[i], "congestioncontrol"))
					continue;
				x |= 0x0100;
				break;
			}
			miiw(ctlr, phyaddr, 0x17, x);

			/*
			 * If the link partner can't autonegotiate, determine
			 * the speed from elsewhere.
			 */
			if(anlpar == 0){
				miir(ctlr, phyaddr, 0x01);
				bmsr = miir(ctlr, phyaddr, 0x01);
				x = miir(ctlr, phyaddr, 0x19);
				if((bmsr & 0x0004) && !(x & 0x0040))
					bmcr = 0x2000;
			}
			break;

		case 0x07:				/* Intel 82555 */
			/*
			 * Auto-negotiation may fail if the other end is
			 * a DP83840A and the cable is short.
			 */
			miir(ctlr, phyaddr, 0x01);
			bmsr = miir(ctlr, phyaddr, 0x01);
			if((miir(ctlr, phyaddr, 0) & 0x1000) && !(bmsr & 0x0020)){
				miiw(ctlr, phyaddr, 0x1A, 0x2010);
				x = miir(ctlr, phyaddr, 0);
				miiw(ctlr, phyaddr, 0, 0x0200|x);
				for(i = 0; i < 3000; i++){
					delay(1);
					if(miir(ctlr, phyaddr, 0x01) & 0x0020)
						break;
				}
				miiw(ctlr, phyaddr, 0x1A, 0x2000);
					
				anar = miir(ctlr, phyaddr, 0x04);
				anlpar = miir(ctlr, phyaddr, 0x05) & 0x03E0;
				anar &= anlpar;
				bmcr = 0;
				if(anar & 0x380)
					bmcr = 0x2000;
				if(anar & 0x0140)
					bmcr |= 0x0100;
			}
			break;
		}

		/*
		 * Force speed and duplex if no auto-negotiation.
		 */
		if(anlpar == 0){
			medium = -1;
			for(i = 0; i < ether->nopt; i++){
				for(k = 0; k < nelem(mediatable); k++){
					if(cistrcmp(mediatable[k], ether->opt[i]))
						continue;
					medium = k;
					break;
				}
		
				switch(medium){
				default:
					break;

				case 0x00:			/* 10BASE-T */
				case 0x01:			/* 10BASE-2 */
				case 0x02:			/* 10BASE-5 */
					bmcr &= ~(0x2000|0x0100);
					ctlr->configdata[19] &= ~0x40;
					break;

				case 0x03:			/* 100BASE-TX */
				case 0x06:			/* 100BASE-T4 */
				case 0x07:			/* 100BASE-FX */
					ctlr->configdata[19] &= ~0x40;
					bmcr |= 0x2000;
					break;

				case 0x04:			/* 10BASE-TFD */
					bmcr = (bmcr & ~0x2000)|0x0100;
					ctlr->configdata[19] |= 0x40;
					break;

				case 0x05:			/* 100BASE-TXFD */
				case 0x08:			/* 100BASE-FXFD */
					bmcr |= 0x2000|0x0100;
					ctlr->configdata[19] |= 0x40;
					break;
				}
			}
			if(medium != -1)
				miiw(ctlr, phyaddr, 0x00, bmcr);
		}

		if(bmcr & 0x2000)
			ether->mbps = 100;

		ctlr->configdata[8] = 1;
		ctlr->configdata[15] &= ~0x80;
	}
	else{
		ctlr->configdata[8] = 0;
		ctlr->configdata[15] |= 0x80;
	}

	/*
	 * Workaround for some broken HUB chips when connected at 10Mb/s
	 * half-duplex.
	 * This is a band-aid, but as there's no dynamic auto-negotiation
	 * code at the moment, only deactivate the workaround code in txstart
	 * if the link is 100Mb/s.
	 */
	if(ether->mbps != 10)
		ctlr->nop = 0;

	/*
	 * Load the chip configuration and start it off.
	 */
	if(ether->oq == 0)
		ether->oq = qopen(64*1024, Qmsg, 0, 0);
	configure(ether, 0);
	command(ctlr, CUstart, PADDR(&ctlr->cbr->status));

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to loading
	 * the station address with the Individual Address Setup command.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0){
		for(i = 0; i < Eaddrlen/2; i++){
			x = ctlr->eeprom[i];
			ether->ea[2*i] = x;
			ether->ea[2*i+1] = x>>8;
		}
	}

	ilock(&ctlr->cblock);
	ctlr->action = CbIAS;
	txstart(ether);
	iunlock(&ctlr->cblock);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->ifstat = ifstat;
	ether->shutdown = shutdown;

	ether->promiscuous = promiscuous;
	ether->multicast = multicast;
	ether->arg = ether;

	return 0;
}

void
ether82557link(void)
{
	addethercard("i82557",  reset);
}
