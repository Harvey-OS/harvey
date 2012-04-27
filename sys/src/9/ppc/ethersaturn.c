#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"
#include "msaturn.h"

#include "etherif.h"

enum{
	Etcr = Saturn + 0x0c00,
	Etsr = Saturn + 0x0c02,
	Ercr = Saturn + 0x0c04,
	Ersr = Saturn + 0x0c06,
	Eisr = Saturn + 0x0d04,
	Eimr = Saturn + 0x0d06,
	Emacaddr0 = Saturn + 0x0e02,
	Miicr = Saturn + 0x0f02,
	Miiwdr = Saturn + 0x0f04,
	Miirdr = Saturn + 0x0f06,

	Ethermem = 0xf2c00000,
	Etherfsize = 0x2000,
	Nrx = 14,
	Ntx = 2,		// Nrx + Ntx must be 16

	Ersr_rxfpmask = 0xf,
	Ersr_rxevent = RBIT(0, ushort),
	Etcr_txfpmask = 0xf,
	Ercr_rxenab = RBIT(0, ushort),
	Ercr_auienab = RBIT(2, ushort),
	Etcr_txstart = RBIT(1, ushort),
	Etcr_retries = 0xf<<8,
	Ei_txecall = RBIT(0, ushort),
	Ei_txretry = RBIT(2, ushort),
	Ei_txdefer = RBIT(3, ushort),
	Ei_txcrs = RBIT(4, ushort),
	Ei_txdone = RBIT(5, ushort),
	Ei_rxcrcerr = RBIT(8, ushort),
	Ei_rxdrib = RBIT(9, ushort),
	Ei_rxdone = RBIT(10, ushort),
	Ei_rxshort = RBIT(11, ushort),
	Ei_rxlong = RBIT(12, ushort),

	Miicr_regshift = 6,
	Miicr_read = RBIT(10, ushort),
	Miicr_preambledis = RBIT(12, ushort),
	Miicr_accack = RBIT(14, ushort),
	Miicr_accbsy = RBIT(15, ushort),
};

typedef struct {
	Lock;
	int		txbusy;
	int		txempty;
	int		txfull;
	int		ntx;			/* number of entries in transmit ring */
	int		rxlast;

	int		active;
	ulong	interrupts;	/* statistics */
	ulong	overflows;
} Ctlr;

static ushort*etcr=(ushort*)Etcr;
static ushort*etsr=(ushort*)Etsr;
static ushort*ercr=(ushort*)Ercr;
static ushort*ersr=(ushort*)Ersr;
static ushort*eimr=(ushort*)Eimr;
static ushort*eisr=(ushort*)Eisr;
static ushort*miicr=(ushort*)Miicr;
static ushort*miirdr=(ushort*)Miirdr;

static void
txfill(Ether*ether, Ctlr*ctlr)
{
	int len;
	Block *b;
	ushort*dst;

	while(ctlr->ntx<Ntx){
		if((b=qget(ether->oq)) == nil)
			break;

		len = BLEN(b);
		dst = (ushort*)(Ethermem+(ctlr->txempty+Nrx)*Etherfsize);
		*dst = len;
		memmove(&dst[1], b->rp, len);
		ctlr->ntx++;
		ctlr->txempty++;
		if(ctlr->txempty==Ntx)
			ctlr->txempty = 0;
		freeb(b);
	}
}

static void
txrestart(Ctlr*ctlr)
{
	if(ctlr->ntx==0 || ctlr->txbusy)
		return;
	ctlr->txbusy = 1;
	*etcr = Etcr_txstart|Etcr_retries|(ctlr->txfull+Nrx);
}

static void interrupt(Ureg*, void*);

static void
transmit(Ether*ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	ilock(ctlr);
	txfill(ether, ctlr);
	txrestart(ctlr);
	iunlock(ctlr);

}

static void
interrupt(Ureg*, void*arg)
{
	Ctlr*ctlr;
	Ether*ether = arg;
	Etherpkt*pkt;
	ushort ie;
	int rx, len;
	Block *b;

	ctlr = ether->ctlr;
	if(!ctlr->active)
		return;	/* not ours */
	ctlr->interrupts++;

	ilock(ctlr);
	ie = *eisr;
	*eisr = ie;
	intack();

	if(ie==0)
		iprint("interrupt: no interrupt source?\n");

	if(ie&Ei_txdone){
		if((*etcr&Etcr_txstart)==0){
			if(ctlr->txbusy){
				ctlr->txbusy = 0;
				ctlr->ntx--;
				ctlr->txfull++;
				if(ctlr->txfull==Ntx)
					ctlr->txfull = 0;
			}
			txrestart(ctlr);
			txfill(ether, ctlr);
			txrestart(ctlr);
		}
		else
			iprint("interrupt: bogus tx interrupt\n");
		ie &= ~Ei_txdone;
	}

	if(ie&Ei_rxdone){
		rx=*ersr&Ersr_rxfpmask;
		while(ctlr->rxlast!=rx){

			ctlr->rxlast++;
			if(ctlr->rxlast >= Nrx)
				ctlr->rxlast = 0;

			pkt = (Etherpkt*)(Ethermem+ctlr->rxlast*Etherfsize);
			len = *(ushort*)pkt;
			if((b = iallocb(len+sizeof(ushort))) != nil){
				memmove(b->wp, pkt, len+sizeof(ushort));
				b->rp += sizeof(ushort);
				b->wp = b->rp + len;
				etheriq(ether, b, 1);
			}else
				ether->soverflows++;
			rx=*ersr&Ersr_rxfpmask;
		}
		ie &= ~Ei_rxdone;
	}

	if(ie&Ei_txretry){
		iprint("ethersaturn: txretry!\n");
		ie &= ~Ei_txretry;
		ctlr->txbusy = 0;
		txrestart(ctlr);
	}

	ie &= ~Ei_txcrs;
	if(ie)
		iprint("interrupt: unhandled interrupts %.4uX\n", ie);
	iunlock(ctlr);
}

static int
reset(Ether* ether)
{
	Ctlr*ctlr;

	*ercr = 0;
	ctlr = malloc(sizeof(*ctlr));
	memset(ctlr, 0, sizeof(*ctlr));
	ctlr->active = 1;

	ether->ctlr = ctlr;
	ether->transmit = transmit;
	ether->interrupt = interrupt;
	ether->irq = Vecether;
	ether->arg = ether;
	memmove(ether->ea, (ushort*)Emacaddr0, Eaddrlen);

	*ercr = Ercr_rxenab|Ercr_auienab|(Nrx-1);
	*eimr = Ei_rxdone|Ei_txretry|Ei_txdone;
	
	iprint("reset: ercr %.4uX\n", *ercr);
	return 0;
}

void
ethersaturnlink(void)
{
	addethercard("saturn", reset);
}

