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
#include "all.h"
#include "io.h"
#include "mem.h"

#include "../ip/ip.h"
#include "etherif.h"

enum {
	Nrfd		= 64,		/* receive frame area */
	Ncb		= 64,		/* maximum control blocks queued */

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

	EEaddrsz	= 6,		/* bits of address */
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

	Msgbuf*	mb;
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

typedef struct Ctlr {
	Lock	slock;			/* attach */
	int	state;

	int	port;
	ushort	eeprom[0x40];

	Lock	miilock;

	Rendez	timer;			/* watchdog timer for receive lockup errata */
	int	tick;
	char	wname[NAMELEN];

	Lock	rlock;			/* registers */
	int	command;		/* last command issued */

	Msgbuf*	rfdhead;		/* receive side */
	Msgbuf*	rfdtail;
	int	nrfd;

	Lock	cblock;			/* transmit side */
	int	action;
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

	while(csr8r(ctlr, CommandR))
		;

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

static Msgbuf*
rfdalloc(ulong link)
{
	Msgbuf *mb;
	Rfd *rfd;

	if(mb = mballoc(sizeof(Rfd), 0, Maeth2)){
		rfd = (Rfd*)mb->data;
		rfd->field = 0;
		rfd->link = link;
		rfd->rbd = NullPointer;
		rfd->count = 0;
		rfd->size = sizeof(Enpkt);
	}

	return mb;
}

static int
return0(void*)
{
	return 0;
}

static void
watchdog(void)
{
	Ether *ether;
	Ctlr *ctlr;
	static void txstart(Ether*);

	ether = getarg();
	for(;;){
		ctlr = ether->ctlr;
		tsleep(&ctlr->timer, return0, 0, 4000);

		/*
		 * Hmmm. This doesn't seem right. Currently
		 * the device can't be disabled but it may be in
		 * the future.
		 */
		ctlr = ether->ctlr;
		if(ctlr == nil || ctlr->state == 0)
			continue;

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

	ctlr = ether->ctlr;
	lock(&ctlr->slock);
	if(ctlr->state == 0){
		ilock(&ctlr->rlock);
		csr8w(ctlr, Interrupt, 0);
		iunlock(&ctlr->rlock);
		command(ctlr, RUstart, PADDR(ctlr->rfdhead->data));
		ctlr->state = 1;

		/*
		 * Start the watchdog timer for the receive lockup errata
		 * unless the EEPROM compatibility word indicates it may be
		 * omitted.
		 */
		if((ctlr->eeprom[0x03] & 0x0003) != 0x0003){
			sprint(ctlr->wname, "ether%dwatchdog", ether->ctlrno);
			userinit(watchdog, ether, ctlr->wname);
		}
	}
	unlock(&ctlr->slock);
}

static void
txstart(Ether* ether)
{
	Ctlr *ctlr;
	Msgbuf *mb;
	Cb *cb;

	ctlr = ether->ctlr;
	while(ctlr->cbq < (ctlr->ncb-1)){
		cb = ctlr->cbhead->next;
		if(ctlr->action == 0){
			mb = etheroq(ether);
			if(mb == nil)
				break;

			cb->command = CbS|CbSF|CbTransmit;
			cb->tbd = PADDR(&cb->tba);
			cb->count = 0;
			cb->threshold = ctlr->threshold;
			cb->number = 1;
			cb->tba = PADDR(mb->data);
			cb->mb = mb;
			cb->tbasz = mb->count;
		}
		else if(ctlr->action == CbConfigure){
			cb->command = CbS|CbConfigure;
			memmove(cb->data, ctlr->configdata, sizeof(ctlr->configdata));
			ctlr->action = 0;
		}
		else if(ctlr->action == CbIAS){
			cb->command = CbS|CbIAS;
			memmove(cb->data, ether->ea, Easize);
			ctlr->action = 0;
		}
		else if(ctlr->action == CbMAS){
			cb->command = CbS|CbMAS;
			memset(cb->data, 0, sizeof(cb->data));
			ctlr->action = 0;
		}
		else{
			print("#l%d: action 0x%ux\n", ether->ctlrno, ctlr->action);
			ctlr->action = 0;
			break;
		}
		cb->status = 0;

		//coherence();
		ctlr->cbhead->command &= ~CbS;
		ctlr->cbhead = cb;
		ctlr->cbq++;
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
	Msgbuf *mb, *pmb, *xmb;

	ctlr = ether->ctlr;
	mb = ctlr->rfdhead;
	for(rfd = (Rfd*)mb->data; rfd->field & RfdC; rfd = (Rfd*)mb->data){
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
		 *	initialise mb to point to the replacement;
		 * 4) if there's a good packet, pass it on for disposal.
		 */
		if(rfd->field & RfdOK){
			pmb = nil;
			count = rfd->count & 0x3FFF;
			if((count < ETHERMAXTU/4) && (pmb = mballoc(count, 0, Maeth3))){
				memmove(pmb->data, mb->data+sizeof(Rfd)-sizeof(rfd->data), count);
				pmb->count = count;

				rfd->count = 0;
				rfd->field = 0;
			}
			else if(xmb = rfdalloc(rfd->link)){
				mb->data += sizeof(Rfd)-sizeof(rfd->data);
				mb->count = count;

				xmb->next = mb->next;
				mb->next = 0;

				pmb = mb;
				mb = xmb;
			}
			if(pmb != nil)
				etheriq(ether, pmb);
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
		rfd = (Rfd*)ctlr->rfdtail->data;
		ctlr->rfdtail = ctlr->rfdtail->next;
		ctlr->rfdtail->next = mb;
		((Rfd*)ctlr->rfdtail->data)->link = PADDR(mb->data);
		((Rfd*)ctlr->rfdtail->data)->field |= RfdS;
		//coherence();
		rfd->field &= ~RfdS;

		/*
		 * Finally done with the current (possibly replaced)
		 * head, move on to the next and maintain the sentinel
		 * between tail and head.
		 */
		ctlr->rfdhead = mb->next;
		mb = ctlr->rfdhead;
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
				if(cb->mb){
					mbfree(cb->mb);
					cb->mb = nil;
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
			panic("#l%d: status %ux\n", ether->ctlrno, status);
	}
}

static void
ctlrinit(Ctlr* ctlr)
{
	int i;
	Msgbuf *mb;
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
		mb = rfdalloc(link);
		if(ctlr->rfdhead == nil)
			ctlr->rfdtail = mb;
		mb->next = ctlr->rfdhead;
		ctlr->rfdhead = mb;
		link = PADDR(mb->data);
	}
	ctlr->rfdtail->next = ctlr->rfdhead;
	rfd = (Rfd*)ctlr->rfdtail->data;
	rfd->link = PADDR(ctlr->rfdhead->data);
	rfd->field |= RfdS;
	ctlr->rfdhead = ctlr->rfdhead->next;

	/*
	 * Create a ring of control blocks for the
	 * transmit side.
	 */
	ilock(&ctlr->cblock);
	ctlr->cbr = ialloc(ctlr->ncb*sizeof(Cb), 0);
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
	int i, op, data;

	/*
	 * Hyundai HY93C46 or equivalent serial EEPROM.
	 * This sequence for reading a 16-bit register 'r'
	 * in the EEPROM is taken straight from Section
	 * 3.3.4.2 of the Intel 82557 User's Guide.
	 */
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

	for(i = EEaddrsz-1; i >= 0; i--){
		data = (((r>>i) & 0x01)<<2)|EEcs;
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

	return data;
}

typedef struct Adapter {
	int	port;
	int	irq;
	int	tbdf;
} Adapter;
static Msgbuf* adapter;

static void
i82557adapter(Msgbuf** mbb, int port, int irq, int tbdf)
{
	Msgbuf *mb;
	Adapter *ap;

	mb = mballoc(sizeof(Adapter), 0, Maeth1);
	ap = (Adapter*)mb->data;
	ap->port = port;
	ap->irq = irq;
	ap->tbdf = tbdf;

	mb->next = *mbb;
	*mbb = mb;
}

static void
i82557pci(void)
{
	Pcidev *p;

	p = nil;
	while(p = pcimatch(p, 0x8086, 0x1229)){
		/*
		 * bar[0] is the memory-mapped register address (4KB),
		 * bar[1] is the I/O port register address (32 bytes) and
		 * bar[2] is for the flash ROM (1MB).
		 */
		i82557adapter(&adapter, p->mem[1].bar & ~0x01, p->intl, p->tbdf);
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

int
etheri82557reset(Ether* ether)
{
	int anar, anlpar, bmcr, bmsr, i, k, medium, phyaddr, port, x;
	unsigned short sum;
	Msgbuf *mb, **mbb;
	Adapter *ap;
	uchar ea[Easize];
	Ctlr *ctlr;
	static int scandone;

	if(scandone == 0){
		i82557pci();
		scandone = 1;
	}

	/*
	 * Any adapter matches if no port is supplied,
	 * otherwise the ports must match.
	 */
	port = 0;
	mbb = &adapter;
	for(mb = *mbb; mb; mb = mb->next){
		ap = (Adapter*)mb->data;
		if(ether->port == 0 || ether->port == ap->port){
			port = ap->port;
			ether->irq = ap->irq;
			ether->tbdf = ap->tbdf;
			*mbb = mb->next;
			mbfree(mb);
			break;
		}
		mbb = &mb->next;
	}
	if(port == 0)
		return -1;

	/*
	 * Allocate a controller structure and start to initialise it.
	 * Perform a software reset after which should ensure busmastering
	 * is still enabled. The EtherExpress PRO/100B appears to leave
	 * the PCI configuration alone (see the 'To do' list above) so punt
	 * for now.
	 * Load the RUB and CUB registers for linear addressing (0).
	 */
	ether->ctlr = ialloc(sizeof(Ctlr), 0);
	ctlr = ether->ctlr;
	ctlr->port = port;

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
	 */
	sum = 0;
	for(i = 0; i < 0x40; i++){
		x = hy93c46r(ctlr, i);
		ctlr->eeprom[i] = x;
		sum += x;
	}
	if(sum != 0xBABA)
		print("#l%d: EEPROM checksum - 0x%4.4ux\n", ether->ctlrno, sum);

	/*
	 * Eeprom[6] indicates whether there is a PHY and whether
	 * it's not 10Mb-only, in which case use the given PHY address
	 * to set any PHY specific options and determine the speed.
	 * If no PHY, assume 82503 (serial) operation.
	 */
	if((ctlr->eeprom[6] & 0x1F00) && !(ctlr->eeprom[6] & 0x8000)){
		phyaddr = ctlr->eeprom[6] & 0x00FF;

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
		else
			ether->mbps = 10;

		ctlr->configdata[8] = 1;
		ctlr->configdata[15] &= ~0x80;
	}
	else{
		ctlr->configdata[8] = 0;
		ctlr->configdata[15] |= 0x80;
	}

	/*
	 * Load the chip configuration and start it off.
	 */
	configure(ether, 0);
	command(ctlr, CUstart, PADDR(&ctlr->cbr->status));

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to loading
	 * the station address with the Individual Address Setup command.
	 */
	memset(ea, 0, Easize);
	if(memcmp(ea, ether->ea, Easize) == 0){
		for(i = 0; i < Easize/2; i++){
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
	ether->port = port;
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;

	return 0;
}
