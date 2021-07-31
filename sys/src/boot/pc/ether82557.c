/*
 * Intel 82557 Fast Ethernet PCI Bus LAN Controller
 * as found on the Intel EtherExpress PRO/100B. This chip is full
 * of smarts, unfortunately none of them are in the right place.
 * To do:
 *	the PCI scanning code could be made common to other adapters;
 *	PCI code needs rewritten to handle byte, word, dword accesses
 *	  and using the devno as a bus+dev+function triplet.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"

enum {
	Nrfd		= 4,		/* receive frame area */

	NullPointer	= 0xFFFFFFFF,	/* 82557 NULL pointer */
};

enum {					/* CSR */
	Status		= 0x00,		/* byte or word (word includes Ack) */
	Ack		= 0x01,		/* byte */
	CommandR	= 0x02,		/* byte or word (word includes Interrupt) */
	Interrupt	= 0x03,		/* byte */
	Pointer		= 0x04,		/* dword */
	Port		= 0x08,		/* dword */
	Fcr		= 0x0C,		/* Flash control register */
	Ecr		= 0x0E,		/* EEPROM control register */
	Mcr		= 0x10,		/* MDI control register */
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

	Etherpkt;
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
	RfdF		= 0x00004000,
	RfdEOF		= 0x00008000,
};

typedef struct Cb {
	int	command;
	ulong	link;
	uchar	data[24];	/* CbIAS + CbConfigure */
} Cb;

typedef struct TxCB {
	int	command;
	ulong	link;
	ulong	tbd;
	ushort	count;
	uchar	threshold;
	uchar	number;
} TxCB;

enum {					/* action command */
	CbOK		= 0x00002000,	/* DMA completed OK */
	CbC		= 0x00008000,	/* execution Complete */

	CbNOP		= 0x00000000,
	CbIAS		= 0x00010000,	/* Indvidual Address Setup */
	CbConfigure	= 0x00020000,
	CbMAS		= 0x00030000,	/* Multicast Address Setup */
	CbTransmit	= 0x00040000,
	CbDump		= 0x00060000,
	CbDiagnose	= 0x00070000,
	CbCommand	= 0x00070000,	/* mask */

	CbSF		= 0x00080000,	/* CbTransmit */

	CbI		= 0x20000000,	/* Interrupt after completion */
	CbS		= 0x40000000,	/* Suspend after completion */
	CbEL		= 0x80000000,	/* End of List */
};

enum {					/* CbTransmit count */
	CbEOF		= 0x00008000,
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	int	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;

	int	eepromsz;		/* address size in bits */
	ushort*	eeprom;

	int	ctlrno;
	char*	type;

	uchar	configdata[24];

	Rfd	rfd[Nrfd];
	int	rfdl;
	int	rfdx;

	Block*	cbqhead;
	Block*	cbqtail;
	int	cbqbusy;
} Ctlr;

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

static uchar configdata[24] = {
	0x16,				/* byte count */
	0x44,				/* Rx/Tx FIFO limit */
	0x00,				/* adaptive IFS */
	0x00,	
	0x04,				/* Rx DMA maximum byte count */
	0x84,				/* Tx DMA maximum byte count */
	0x33,				/* late SCB, CNA interrupts */
	0x01,				/* discard short Rx frames */
	0x00,				/* 503/MII */

	0x00,	
	0x2E,				/* normal operation, NSAI */
	0x00,				/* linear priority */
	0x60,				/* inter-frame spacing */
	0x00,	
	0xF2,	
	0x48,				/* promiscuous mode off */
	0x00,	
	0x40,	
	0xF2,				/* transmit padding enable */
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
custart(Ctlr* ctlr)
{
	if(ctlr->cbqhead == 0){
		ctlr->cbqbusy = 0;
		return;
	}
	ctlr->cbqbusy = 1;

	csr32w(ctlr, Pointer, PADDR(ctlr->cbqhead->rp));
	while(csr8r(ctlr, CommandR))
		;
	csr8w(ctlr, CommandR, CUstart);
}

static void
action(Ctlr* ctlr, Block* bp)
{
	Cb *cb;

	cb = (Cb*)bp->rp;
	cb->command |= CbEL;

	if(ctlr->cbqhead){
		ctlr->cbqtail->next = bp;
		cb = (Cb*)ctlr->cbqtail->rp;
		cb->link = PADDR(bp->rp);
		cb->command &= ~CbEL;
	}
	else
		ctlr->cbqhead = bp;
	ctlr->cbqtail = bp;

	if(ctlr->cbqbusy == 0)
		custart(ctlr);
}

static void
attach(Ether* ether)
{
	int status;
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	status = csr16r(ctlr, Status);
	if((status & RUstatus) == RUidle){
		csr32w(ctlr, Pointer, PADDR(&ctlr->rfd[ctlr->rfdx]));
		while(csr8r(ctlr, CommandR))
			;
		csr8w(ctlr, CommandR, RUstart);
	}
}

static void
configure(void* arg, int promiscuous)
{
	Ctlr *ctlr;
	Block *bp;
	Cb *cb;

	ctlr = ((Ether*)arg)->ctlr;

	bp = allocb(sizeof(Cb));
	cb = (Cb*)bp->rp;
	bp->wp += sizeof(Cb);

	cb->command = CbConfigure;
	cb->link = NullPointer;
	memmove(cb->data, ctlr->configdata, sizeof(ctlr->configdata));
	if(promiscuous)
		cb->data[15] |= 0x01;
	action(ctlr, bp);
}

static void
transmit(Ether* ether)
{
	Block *bp;
	TxCB *txcb;
	RingBuf *tb;

	for(tb = &ether->tb[ether->ti]; tb->owner == Interface; tb = &ether->tb[ether->ti]){
		bp = allocb(tb->len+sizeof(TxCB));
		txcb = (TxCB*)bp->wp;
		bp->wp += sizeof(TxCB);

		txcb->command = CbTransmit;
		txcb->link = NullPointer;
		txcb->tbd = NullPointer;
		txcb->count = CbEOF|tb->len;
		txcb->threshold = 2;
		txcb->number = 0;

		memmove(bp->wp, tb->pkt, tb->len);
		memmove(bp->wp+Eaddrlen, ether->ea, Eaddrlen);
		bp->wp += tb->len;

		action(ether->ctlr, bp);

		tb->owner = Host;
		ether->ti = NEXT(ether->ti, ether->ntb);
	}
}

static void
interrupt(Ureg*, void* arg)
{
	Rfd *rfd;
	Block *bp;
	Ctlr *ctlr;
	Ether *ether;
	int status;
	RingBuf *rb;

	ether = arg;
	ctlr = ether->ctlr;

	for(;;){
		status = csr16r(ctlr, Status);
		csr8w(ctlr, Ack, (status>>8) & 0xFF);

		if((status & (StatCX|StatFR|StatCNA|StatRNR)) == 0)
			return;

		if(status & StatFR){
			rfd = &ctlr->rfd[ctlr->rfdx];
			while(rfd->field & RfdC){
				rb = &ether->rb[ether->ri];
				if(rb->owner == Interface){
					rb->owner = Host;
					rb->len = rfd->count & 0x3FFF;
					memmove(rb->pkt, rfd->d, rfd->count & 0x3FFF);
					ether->ri = NEXT(ether->ri, ether->nrb);
				}

				/*
				 * Reinitialise the frame for reception and bump
				 * the receive frame processing index;
				 * bump the sentinel index, mark the new sentinel
				 * and clear the old sentinel suspend bit;
				 * set bp and rfd for the next receive frame to
				 * process.
				 */
				rfd->field = 0;
				rfd->count = 0;
				ctlr->rfdx = NEXT(ctlr->rfdx, Nrfd);

				rfd = &ctlr->rfd[ctlr->rfdl];
				ctlr->rfdl = NEXT(ctlr->rfdl, Nrfd);
				ctlr->rfd[ctlr->rfdl].field |= RfdS;
				rfd->field &= ~RfdS;

				rfd = &ctlr->rfd[ctlr->rfdx];
			}
			status &= ~StatFR;
		}

		if(status & StatRNR){
			while(csr8r(ctlr, CommandR))
				;
			csr8w(ctlr, CommandR, RUresume);

			status &= ~StatRNR;
		}

		if(status & StatCNA){
			while(bp = ctlr->cbqhead){
				if((((Cb*)bp->rp)->command & CbC) == 0)
					break;
				ctlr->cbqhead = bp->next;
				freeb(bp);
			}
			custart(ctlr);

			status &= ~StatCNA;
		}

		if(status & (StatCX|StatFR|StatCNA|StatRNR|StatMDI|StatSWI))
			panic("%s#%d: status %uX\n", ctlr->type,  ctlr->ctlrno, status);
	}
}

static void
ctlrinit(Ctlr* ctlr)
{
	int i;
	Rfd *rfd;
	ulong link;

	link = NullPointer;
	for(i = Nrfd-1; i >= 0; i--){
		rfd = &ctlr->rfd[i];

		rfd->field = 0;
		rfd->link = link;
		link = PADDR(rfd);
		rfd->rbd = NullPointer;
		rfd->count = 0;
		rfd->size = sizeof(Etherpkt);
	}
	ctlr->rfd[Nrfd-1].link = PADDR(&ctlr->rfd[0]);

	ctlr->rfdl = 0;
	ctlr->rfd[0].field |= RfdS;
	ctlr->rfdx = 2;

	memmove(ctlr->configdata, configdata, sizeof(configdata));
}

static int
miir(Ctlr* ctlr, int phyadd, int regadd)
{
	int mcr, timo;

	csr32w(ctlr, Mcr, MDIread|(phyadd<<21)|(regadd<<16));
	mcr = 0;
	for(timo = 64; timo; timo--){
		mcr = csr32r(ctlr, Mcr);
		if(mcr & MDIready)
			break;
		microdelay(1);
	}

	if(mcr & MDIready)
		return mcr & 0xFFFF;

	return -1;
}

static int
miiw(Ctlr* ctlr, int phyadd, int regadd, int data)
{
	int mcr, timo;

	csr32w(ctlr, Mcr, MDIwrite|(phyadd<<21)|(regadd<<16)|(data & 0xFFFF));
	mcr = 0;
	for(timo = 64; timo; timo--){
		mcr = csr32r(ctlr, Mcr);
		if(mcr & MDIready)
			break;
		microdelay(1);
	}

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
		goto reread;
	}

	return data;
}

static void
i82557pci(void)
{
	Pcidev *p;
	Ctlr *ctlr;

	p = nil;
	while(p = pcimatch(p, 0x8086, 0)){
		switch(p->did){
		default:
			continue;
		case 0x1031:		/* Intel 82562EM */
		case 0x1050:		/* Intel 82562EZ */
		case 0x2449:		/* Intel 82562ET */
		case 0x1209:		/* Intel 82559ER */
		case 0x1229:		/* Intel 8255[789] */
		case 0x1030:		/* Intel 82559 InBusiness 10/100  */
		case 0x1039:		/* Intel 82801BD PRO/100 VE */
		case 0x103A:		/* Intel 82562 PRO/100 VE */
			break;
		}

		/*
		 * bar[0] is the memory-mapped register address (4KB),
		 * bar[1] is the I/O port register address (32 bytes) and
		 * bar[2] is for the flash ROM (1MB).
		 */
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = p->mem[1].bar & ~0x01;
		ctlr->pcidev = p;

		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;

		pcisetbme(p);
	}
}

static void
detach(Ether* ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;

	csr32w(ctlr, Port, 0);
	delay(1);

	while(csr8r(ctlr, CommandR))
		;
}

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
		//print("phy%d: oui %uX reg1 %uX\n", i, oui, miir(ctlr, i, 1));

		if(oui == 0xAA00)
			ctlr->eeprom[6] = 0x07<<8;
		else if(oui == 0x80017){
			if(x & 0x01)
				ctlr->eeprom[6] = 0x0A<<8;
			else
				ctlr->eeprom[6] = 0x04<<8;
		}
		return i;
	}
	return -1;
}

int
i82557reset(Ether* ether)
{
	int anar, anlpar, bmcr, bmsr, force, i, phyaddr, x;
	unsigned short sum;
	Block *bp;
	uchar ea[Eaddrlen];
	Ctlr *ctlr;
	Cb *cb;


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
	 * Perform a software reset after which need to ensure busmastering
	 * is still enabled. The EtherExpress PRO/100B appears to leave
	 * the PCI configuration alone (see the 'To do' list above) so punt
	 * for now.
	 * Load the RUB and CUB registers for linear addressing (0).
	 */
	ether->ctlr = ctlr;
	ether->port = ctlr->port;
	ether->irq = ctlr->pcidev->intl;
	ether->tbdf = ctlr->pcidev->tbdf;
	ctlr->ctlrno = ether->ctlrno;
	ctlr->type = ether->type;

	csr32w(ctlr, Port, 0);
	delay(1);

	while(csr8r(ctlr, CommandR))
		;
	csr32w(ctlr, Pointer, 0);
	csr8w(ctlr, CommandR, LoadRUB);
	while(csr8r(ctlr, CommandR))
		;
	csr8w(ctlr, CommandR, LoadCUB);

	/*
	 * Initialise the action and receive frame areas.
	 */
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
		print("#l%d: EEPROM checksum - 0x%4.4uX\n", ether->ctlrno, sum);

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
			force = 0;
			for(i = 0; i < ether->nopt; i++){
				if(cistrcmp(ether->opt[i], "fullduplex") == 0){
					force = 1;
					bmcr |= 0x0100;
					ctlr->configdata[19] |= 0x40;
				}
				else if(cistrcmp(ether->opt[i], "speed") == 0){
					force = 1;
					x = strtol(&ether->opt[i][6], 0, 0);
					if(x == 10)
						bmcr &= ~0x2000;
					else if(x == 100)
						bmcr |= 0x2000;
					else
						force = 0;
				}
			}
			if(force)
				miiw(ctlr, phyaddr, 0x00, bmcr);
		}

		ctlr->configdata[8] = 1;
		ctlr->configdata[15] &= ~0x80;
	}
	else{
		ctlr->configdata[8] = 0;
		ctlr->configdata[15] |= 0x80;
	}

	/*
	 * Load the chip configuration
	 */
	configure(ether, 0);

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to loading
	 * the station address with the Individual Address Setup command.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0){
		for(i = 0; i < Eaddrlen/2; i++){
			x = ctlr->eeprom[i];
			ether->ea[2*i] = x & 0xFF;
			ether->ea[2*i+1] = (x>>8) & 0xFF;
		}
	}

	bp = allocb(sizeof(Cb));
	cb = (Cb*)bp->rp;
	bp->wp += sizeof(Cb);

	cb->command = CbIAS;
	cb->link = NullPointer;
	memmove(cb->data, ether->ea, Eaddrlen);
	action(ctlr, bp);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->detach = detach;

	return 0;
}
