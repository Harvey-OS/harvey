/*
 * National Semiconductor DP83932
 * Systems-Oriented Network Interface Controller
 * (SONIC)
 *
 * To do:
 *	really fix for multiple controllers
 *	generalise again
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "devtab.h"

#include "machine.h"
#include "io.h"

enum {
	Nrb		= 16,		/* receive buffers */
	Ntb		= 8,		/* transmit buffers */
};

enum {
	Htx	= 0x0001,	/* halt transmission */
	Txp	= 0x0002,	/* transmit packet(s) */
	Rxdis	= 0x0004,	/* receiver disable */
	Rxen	= 0x0008,	/* receiver enable */
	Stp	= 0x0010,	/* stop timer */
	St	= 0x0020,	/* start timer */
	Rst	= 0x0080,	/* software reset */
	Rrra	= 0x0100,	/* read RRA */
	Lcam	= 0x0200,	/* load CAM */

	Dw32	= 0x0020,	/* data width select */
	Sterm	= 0x0400,	/* synchronous termination */

	Prx	= 0x0001,	/* packet received ok */
	Fae	= 0x0004,	/* frame alignment error */
	Crc	= 0x0008,	/* CRC error */
	Lpkt	= 0x0040,	/* last packet in rba */
	Bc	= 0x0080,	/* broadcast packet received */
	Pro	= 0x1000,	/* physical promiscuous mode */
	Brd	= 0x2000,	/* accept broadcast packets */
	Rnt	= 0x4000,	/* accept runt packets */
	Err	= 0x8000,	/* accept packets with errors */

	Ptx	= 0x0001,	/* packet transmitted ok */
	Pintr	= 0x8000,	/* programmable interrupt */

	Rfo	= 0x0001,	/* receive fifo overrun */
	MpTally	= 0x0002,	/* missed packet tally counter rollover */
	FaeTally= 0x0004,	/* frame alignment error tally counter rollover */
	CrcTally= 0x0008,	/* Crc tally counter rollover */
	Rbae	= 0x0010,	/* receive buffer area exceeded */
	Rbe	= 0x0020,	/* receive buffer exhausted */
	Rde	= 0x0040,	/* receive descriptors exhausted */
	Txer	= 0x0100,	/* transmit error */
	Txdn	= 0x0200,	/* transmission done */
	Pktrx	= 0x0400,	/* packet received */
	Pint	= 0x0800,	/* programmed interrupt */
	Lcd	= 0x1000,	/* load CAM done */
	Hbl	= 0x2000,	/* CD heartbeat lost */
	Br	= 0x4000,	/* bus retry occurred */
	AllIntr	= 0x7771,	/* all of the above */
};

/*
 * Receive Resource Descriptor.
 */
typedef struct {
	ulong	ptr0;		/* buffer pointer in the RRA */
	ulong	ptr1;
	ulong	wc0;		/* buffer word count in the RRA */
	ulong	wc1;
} RXrsc;

/*
 * Receive Packet Descriptor.
 */
typedef struct {
	ulong	status;		/* receive status */
	ulong	count;		/* packet byte count */
	ulong	ptr0;		/* buffer pointer */
	ulong	ptr1;
	ulong	seqno;		/*  */
	ulong	link;		/* descriptor link and EOL */
	ulong	owner;		/* in use */
} RXpkt;

/*
 * Transmit Packet Descriptor.
 */
typedef struct {
	ulong	status;		/* transmit status */
	ulong	config;		/*  */
	ulong	size;		/* byte count of entire packet */
	ulong	count;		/* fragment count */
	ulong	ptr0;		/* packet pointer */
	ulong	ptr1;
	ulong	fsize;		/* fragment size */
	ulong	link;		/* descriptor link */
} TXpkt;

enum {
	Eol		= 1,	/* end of list bit in descriptor link */
	Host		= 0,	/* descriptor belongs to host */
	Interface	= ~0,	/* descriptor belongs to interface */
};

/*
 * CAM Descriptor
 */
typedef struct {
	ulong	cep;		/* CAM entry pointer */
	ulong	cap0;		/* CAM address port 0 */
	ulong	cap1;		/* CAM address port 1 */
	ulong	cap2;		/* CAM address port 2 */
	ulong	ce;		/* CAM enable */
} Cam;

enum {
	NCtlr		= 1,		/* max. number of controllers */
	NType		= 9,		/* types/interface */
};

typedef struct Ctlr Ctlr;
typedef struct Type Type;

/*
 * one per ethernet packet type
 */
struct Type {
	QLock;
	Netprot;		/* stat info */
	int	type;		/* ethernet type */
	int	prom;		/* promiscuous mode */
	Queue	*q;
	int	inuse;
	Ctlr	*ctlr;

	Rendez	cr;		/* rendezvous for close */
	Type	*clist;		/* close list */
};

/*
 * Software controller struct. This in in two parts,
 * the second part being the memory descriptor area
 * used by the SONIC.
 */
struct Ctlr {
	QLock;

	Sonic	*sonic;		/* SONIC registers */

	uchar	ea[6];		/* ethernet address */
	uchar	ba[6];		/* broadcast address */

	Rendez	rr;		/* rendezvous for a receive buffer */
	int	rh;		/* first receive buffer belonging to host */
	int	ri;		/* first receive buffer belonging to interface */	

	Rendez	tr;		/* rendezvous for a transmit buffer */
	QLock	tlock;		/* semaphore on th */
	int	th;		/* first transmit buffer belonging to host */	
	int	ti;		/* first transmit buffer belonging to interface */	

	Type	type[NType];
	int	all;		/* number of channels listening to all packets */
	Type	*clist;		/* channels waiting to close */
	Lock	clock;		/* lock for clist */
	int	prom;		/* number of promiscuous channels */
	int	kproc;		/* true if kproc started */
	char	name[NAMELEN];	/* name of kproc */
	Network	net;

	Queue	lbq;		/* software loopback packet queue */

	int	inpackets;
	int	outpackets;
	int	crcs;		/* input crc errors */
	int	oerrs;		/* output errors */
	int	frames;		/* framing errors */
	int	overflows;	/* packet overflows */
	int	buffs;		/* buffering errors */

	RXrsc	rra[Nrb];	/* receive resource area */
	RXpkt	rda[Nrb];	/* receive descriptor area */
	uchar	rb[Nrb][sizeof(Etherpkt)+4];	/* receive buffer area */
	TXpkt	tda[Ntb];	/* transmit descriptor area */
	uchar	tb[Ntb][sizeof(Etherpkt)];	/* transmit buffer area */
	Cam	cda;		/* CAM descriptor area */
};

static struct Ctlr *softctlr[NCtlr];

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
#define LS16(addr)	(PADDR(addr) & 0xFFFF)
#define MS16(addr)	((PADDR(addr)>>16) & 0xFFFF)

void
etherinit(void)
{
}

Chan*
etherclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
etherwalk(Chan *c, char *name)
{
	return netwalk(c, name, &softctlr[0]->net);
}

void
etherstat(Chan *c, char *dp)
{
	netstat(c, dp, &softctlr[0]->net);
}

Chan*
etheropen(Chan *c, int omode)
{
	return netopen(c, omode, &softctlr[0]->net);
}

void
ethercreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
etherclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
etherread(Chan *c, void *a, long n, ulong offset)
{
	return netread(c, a, n, offset, &softctlr[0]->net);
}

long
etherwrite(Chan *c, char *a, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, a, n, 0);
}

void
etherremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
etherwstat(Chan *c, char *dp)
{
	netwstat(c, dp, &softctlr[0]->net);
}

static int
isobuf(void *arg)
{
	Ctlr *ctlr = arg;

	return ctlr->tda[ctlr->th].status == Host;
}

static void
etheroput(Queue *q, Block *bp)
{
	Ctlr *ctlr;
	Type *tp;
	Etherpkt *p;
	int len, n;
	Block *nbp;
	uchar *tb;
	TXpkt *txpkt;

	tp = q->ptr;
	ctlr = tp->ctlr;
	if(bp->type == M_CTL){
		qlock(ctlr);
		if(streamparse("connect", bp)){
			if(tp->type == -1)
				ctlr->all--;
			tp->type = strtol((char*)bp->rptr, 0, 0);
			if(tp->type == -1)
				ctlr->all++;
		}
		else if(streamparse("promiscuous", bp)) {
			tp->prom = 1;
			ctlr->prom++;
			if(ctlr->prom == 1)
				ctlr->sonic->rcr |= Pro;
		}
		qunlock(ctlr);
		freeb(bp);
		return;
	}

	/*
	 * give packet a local address, return upstream if destined for
	 * this machine.
	 */
	if(BLEN(bp) < ETHERHDRSIZE && (bp = pullup(bp, ETHERHDRSIZE)) == 0)
		return;
	p = (Etherpkt*)bp->rptr;
	memmove(p->s, ctlr->ea, sizeof(ctlr->ea));
	if(memcmp(ctlr->ea, p->d, sizeof(ctlr->ea)) == 0){
		len = blen(bp);
		if(bp = expandb(bp, len >= ETHERMINTU ? len: ETHERMINTU)){
			putq(&ctlr->lbq, bp);
			wakeup(&ctlr->rr);
		}
		return;
	}
	if(memcmp(ctlr->ba, p->d, sizeof(ctlr->ba)) == 0 || ctlr->prom || ctlr->all){
		len = blen(bp);
		nbp = copyb(bp, len);
		if(nbp = expandb(nbp, len >= ETHERMINTU ? len: ETHERMINTU)){
			nbp->wptr = nbp->rptr+len;
			putq(&ctlr->lbq, nbp);
			wakeup(&ctlr->rr);
		}
	}

	/*
	 * only one transmitter at a time
	 */
	qlock(&ctlr->tlock);
	if(waserror()){
		freeb(bp);
		qunlock(&ctlr->tlock);
		nexterror();
	}

	/*
	 * wait till we get an output buffer.
	 */
	sleep(&ctlr->tr, isobuf, ctlr);

	tb = ctlr->tb[ctlr->th];

	/*
	 * copy message into buffer
	 */
	len = 0;
	for(nbp = bp; nbp; nbp = nbp->next){
		if(sizeof(Etherpkt) - len >= (n = BLEN(nbp))){
			memmove(tb+len, nbp->rptr, n);
			len += n;
		}
		if(bp->flags & S_DELIM)
			break;
	}

	/*
	 * pad the packet (zero the pad)
	 */
	if(len < ETHERMINTU){
		memset(tb+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}

	/*
	 * Set up the transmit descriptor and 
	 * start the transmission.
	 * Take the Eol bit off the previous descriptor,
	 * so that if it is still being transmitted
	 * the SONIC will just chain through to this one
	 * without interrupting.
	 */
	ctlr->outpackets++;

	txpkt = &ctlr->tda[ctlr->th];
	txpkt->size = txpkt->fsize = len;
	txpkt->link |= Eol;
	txpkt->status = Interface;
	ctlr->tda[PREV(ctlr->th, Ntb)].link &= ~Eol;

	ctlr->th = NEXT(ctlr->th, Ntb);
	ctlr->sonic->cr = Txp;

	freeb(bp);
	qunlock(&ctlr->tlock);
	poperror();
}

/*
 * open an ether line discipline
 *
 * the lock is to synchronize changing the ethertype with
 * sending packets up the stream on interrupts.
 */
static void
etherstopen(Queue *q, Stream *s)
{
	Ctlr *ctlr = softctlr[0];
	Type *tp;

	tp = &ctlr->type[s->id];
	RD(q)->ptr = WR(q)->ptr = tp;
	tp->type = 0;
	tp->q = RD(q);
	tp->inuse = 1;
	tp->ctlr = ctlr;
}

/*
 * Close ether line discipline
 *
 * The lock is to synchronize changing the Type with
 * sending packets up the stream on interrupts.
 */
static int
isclosed(void *arg)
{
	return ((Type*)arg)->q == 0;
}

static void
etherstclose(Queue *q)
{
	Type *tp = (Type*)(q->ptr);
	Ctlr *ctlr = tp->ctlr;

	if(tp->prom){
		qlock(ctlr);
		ctlr->prom--;
		if(ctlr->prom == 0)
			ctlr->sonic->rcr &= ~Pro;
		qunlock(ctlr);
	}
	if(tp->type == -1){
		qlock(ctlr);
		ctlr->all--;
		qunlock(ctlr);
	}

	/*
	 * Mark as closing and wait for kproc
	 * to close us.
	 */
	lock(&ctlr->clock);
	tp->clist = ctlr->clist;
	ctlr->clist = tp;
	unlock(&ctlr->clock);
	wakeup(&ctlr->rr);
	sleep(&tp->cr, isclosed, tp);

	tp->type = 0;
	tp->q = 0;
	tp->prom = 0;
	tp->inuse = 0;
	netdisown(tp);
	tp->ctlr = 0;
}

static Qinfo info = {
	nullput,
	etheroput,
	etherstopen,
	etherstclose,
	"ether"
};

static int
clonecon(Chan *c)
{
	Ctlr *ctlr = softctlr[0];
	Type *tp;

	USED(c);
	for(tp = ctlr->type; tp < &ctlr->type[NType]; tp++){
		qlock(tp);
		if(tp->inuse || tp->q){
			qunlock(tp);
			continue;
		}
		tp->inuse = 1;
		netown(tp, u->p->user, 0);
		qunlock(tp);
		return tp - ctlr->type;
	}
	exhausted("ether channels");
	return 0;
}

static void
statsfill(Chan *c, char *p, int n)
{
	Ctlr *ctlr = softctlr[0];
	char buf[256];

	USED(c);
	sprint(buf, "in: %d\nout: %d\ncrc errs %d\noverflows: %d\nframe errs %d\nbuff errs: %d\noerrs %d\naddr: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
		ctlr->inpackets, ctlr->outpackets, ctlr->crcs,
		ctlr->overflows, ctlr->frames, ctlr->buffs, ctlr->oerrs,
		ctlr->ea[0], ctlr->ea[1], ctlr->ea[2],
		ctlr->ea[3], ctlr->ea[4], ctlr->ea[5]);
	strncpy(p, buf, n);
}

static void
typefill(Chan *c, char *p, int n)
{
	char buf[16];
	Type *tp;

	tp = &softctlr[0]->type[STREAMID(c->qid.path)];
	sprint(buf, "%d", tp->type);
	strncpy(p, buf, n);
}

static void
etherup(Ctlr *ctlr, void *data, int len)
{
	Etherpkt *p;
	int t;
	Type *tp;
	Block *bp;

	p = data;
	t = (p->type[0]<<8)|p->type[1];
	for(tp = &ctlr->type[0]; tp < &ctlr->type[NType]; tp++){
		/*
		 * Check for open, the right type, and flow control.
		 */
		if(tp->q == 0)
			continue;
		if(t != tp->type && tp->type != -1)
			continue;
		if(tp->q->next->len > Streamhi)
			continue;

		/*
		 * Only a trace channel gets packets destined for other machines.
		 */
		if(tp->type != -1 && p->d[0] != 0xFF
		  && (*p->d != *ctlr->ea || memcmp(p->d, ctlr->ea, sizeof(p->d))))
			continue;

		if(waserror() == 0){
			bp = allocb(len);
			memmove(bp->rptr, p, len);
			bp->wptr += len;
			bp->flags |= S_DELIM;
			PUTNEXT(tp->q, bp);
			poperror();
		}
	}
}

static void
etherintr(int ctlrno, Ureg *ur)
{
	Ctlr *ctlr = softctlr[ctlrno];
	Sonic *sonic = ctlr->sonic;
	ulong isr;
	TXpkt *txpkt;

	USED(ur);

	while(isr = (sonic->isr & AllIntr)){
		sonic->isr = isr;
	
		/*
		 * Transmission complete, for good or bad.
		 */
		if(isr & (Txdn|Txer)){
			txpkt = &ctlr->tda[ctlr->ti];
			while(txpkt->status != Host){
				if(txpkt->status == Interface){
					sonic->ctda = LS16(txpkt);
					sonic->cr = Txp;
					break;
				}
	
				if((txpkt->status & Ptx) == 0)
					ctlr->oerrs++;
	
				txpkt->status = Host;
				ctlr->ti = NEXT(ctlr->ti, Ntb);
				txpkt = &ctlr->tda[ctlr->ti];
			}
			wakeup(&ctlr->tr);
			isr &= ~(Txdn|Txer);
		}
	
		/*
		 * A packet arrived or we ran out of descriptors.
		 * Notify the kproc.
		 */
		if(isr & (Pktrx|Rde)){
			wakeup(&ctlr->rr);
			isr &= ~(Pktrx|Rde);
		}
	
		/*
		 * We get a 'load CAM done' interrupt
		 * after initialisation. Ignore it.
		 */
		if(isr & Lcd)
			isr &= ~Lcd;
	
		/*
		 * Warnings that something is afoot.
		 */
		if(isr & Hbl){
			print("sonic: cd heartbeat lost\n");
			isr &= ~Hbl;
		}
		if(isr & Br){
			print("sonic: bus retry occurred\n");
			isr &= ~Br;
		}
	
		if(isr & AllIntr)
			print("sonic %ux\n", isr);
		}
}

static Vector vector = {
	IRQSONIC, 0, etherintr, "ether0",
};

static void
reset(Ctlr *ctlr)
{
	Sonic *sonic = ctlr->sonic;
	int i;

	/*
	 * Reset the SONIC, toggle the Rst bit.
	 * Set the data config register for synchronous termination
	 * and 32-bit data-path width.
	 * Clear the descriptor and buffer area.
	 */
	sonic->cr = Rst;
	sonic->dcr = Sterm|Dw32;
	sonic->cr = 0;

	/*
	 * Initialise the receive resource area (RRA) and
	 * the receive descriptor area (RDA).
	 *
	 * We use a simple scheme of one packet per descriptor.
	 * We achieve this by setting the EOBC register to be
	 * 2 (16-bit words) less than the buffer size;
	 * thus the size of the receive buffers must be sizeof(Etherpkt)+4.
	 * Set up the receive descriptors as a ring.
	 */
	for(i = 0; i < Nrb; i++){
		ctlr->rra[i].wc0 = (sizeof(ctlr->rb[0])/2) & 0xFFFF;
		ctlr->rra[i].wc1 = ((sizeof(ctlr->rb[0])/2)>>16) & 0xFFFF;

		ctlr->rda[i].link = LS16(&ctlr->rda[NEXT(i, Nrb)]);
		ctlr->rda[i].owner = Interface;

		ctlr->rra[i].ptr0 = ctlr->rda[i].ptr0 = LS16(ctlr->rb[i]);
		ctlr->rra[i].ptr1 = ctlr->rda[i].ptr1 = MS16(ctlr->rb[i]);
	}

	/*
	 * Terminate the receive descriptor ring
	 * and load the SONIC registers to describe the RDA.
	 */
	ctlr->rda[Nrb-1].link |= Eol;

	sonic->crda = LS16(ctlr->rda);
	sonic->urda = MS16(ctlr->rda);
	sonic->eobc = sizeof(ctlr->rb[0])/2 - 2;

	/*
	 * Load the SONIC registers to describe the RRA.
	 * We set the rwp to beyond the area delimited by rsa and
	 * rea. This means that since we've already allocated all
	 * the buffers, we'll never get a 'receive buffer area
	 * exhausted' interrupt and the rrp will just wrap round.
	 * Tell the SONIC to load the RRA and wait for
	 * it to complete.
	 */
	sonic->urra = MS16(&ctlr->rra[0]);
	sonic->rsa = sonic->rrp = LS16(&ctlr->rra[0]);
	sonic->rea = LS16(&ctlr->rra[Nrb]);
	sonic->rwp = LS16(&ctlr->rra[Nrb+1]);

	sonic->cr = Rrra;
	while(sonic->cr & Rrra)
		;

	/*
	 * Initialise the transmit descriptor area (TDA).
	 * Each descriptor describes one packet, we make no use
	 * of having the packet in multiple fragments.
	 * The descriptors are linked in a ring; overlapping transmission
	 * with buffer queueing will cause some packets to
	 * go out back-to-back.
	 *
	 * Load the SONIC registers to describe the TDA.
	 */
	for(i = 0; i < Ntb; i++){
		ctlr->tda[i].status = Host;
		ctlr->tda[i].config = 0;
		ctlr->tda[i].count = 1;
		ctlr->tda[i].ptr0 = LS16(ctlr->tb[i]);
		ctlr->tda[i].ptr1 = MS16(ctlr->tb[i]);
		ctlr->tda[i].link = LS16(&ctlr->tda[NEXT(i, Ntb)]);
	}

	sonic->ctda = LS16(&ctlr->tda[0]);
	sonic->utda = MS16(&ctlr->tda[0]);

	/*
	 * Initialise the software receive and transmit
	 * ring indexes.
	 */
	ctlr->rh = ctlr->ri = ctlr->th = ctlr->ti = 0;

	/*
	 * Initialise the CAM descriptor area (CDA).
	 * We only have one ethernet address to load,
	 * broadcast is defined by the SONIC as all 1s.
	 *
	 * Load the SONIC registers to describe the CDA.
	 * Tell the SONIC to load the CDA and wait for it
	 * to complete.
	 */
	ctlr->cda.cep = 0;
	ctlr->cda.cap0 = (ctlr->ea[1]<<8)|ctlr->ea[0];
	ctlr->cda.cap1 = (ctlr->ea[3]<<8)|ctlr->ea[2];
	ctlr->cda.cap2 = (ctlr->ea[5]<<8)|ctlr->ea[4];
	ctlr->cda.ce = 1;

	sonic->cdp = LS16(&ctlr->cda);
	sonic->cdc = 1;

	sonic->cr = Lcam;
	while(sonic->cr & Lcam)
		;

	/*
	 * Configure the receive control, transmit control
	 * and interrupt-mask registers.
	 * The SONIC is now initialised, but not enabled.
	 */
	sonic->rcr = Err|Rnt|Brd;
	sonic->tcr = 0;
	sonic->imr = AllIntr;
}

static void
enable(Ctlr *ctlr)
{
	/*
	 * Enable the SONIC for receiving packets.
	 * Should be called after reset.
	 */
	ctlr->sonic->cr = Rxen;
}

void
etherreset(void)
{
	Ctlr *ctlr;
	int i;
	extern Eeprom eeprom;

	/*
	 * Map the device registers and allocate
	 * memory for the receive/transmit rings.
	 * Set the physical ethernet address and
	 * prime the interrupt handler.
	 */
	if(softctlr[0] == 0){
		softctlr[0] = xspanalloc(sizeof(Ctlr), BY2PG, 64*1024);
		memmove(softctlr[0]->ea, eeprom.ea, sizeof(softctlr[0]->ea));
		softctlr[0]->sonic = SONICADDR;
		mmuiomap((ulong)softctlr[0]->sonic, 1);
		setvector(&vector);
	}
	ctlr = softctlr[0];

	reset(ctlr);

	memset(ctlr->ba, 0xFF, sizeof(ctlr->ba));

	ctlr->net.name = "ether";
	ctlr->net.nconv = NType;
	ctlr->net.devp = &info;
	ctlr->net.protop = 0;
	ctlr->net.listen = 0;
	ctlr->net.clone = clonecon;
	ctlr->net.ninfo = 2;
	ctlr->net.info[0].name = "stats";
	ctlr->net.info[0].fill = statsfill;
	ctlr->net.info[1].name = "type";
	ctlr->net.info[1].fill = typefill;
	for(i = 0; i < NType; i++)
		netadd(&ctlr->net, &ctlr->type[i], i);
}

static int
isinput(void *arg)
{
	Ctlr *ctlr = arg;

	return ctlr->lbq.first || ctlr->rda[ctlr->rh].owner == Host || ctlr->clist;
}

static void
etherkproc(void *arg)
{
	Ctlr *ctlr = arg;
	RXpkt *rxpkt;
	Block *bp;
	Type *tp;

	if(waserror()){
		print("%s noted\n", ctlr->name);
		reset(ctlr);
		ctlr->kproc = 0;
		nexterror();
	}

	for(;;){
		sleep(&ctlr->rr, isinput, ctlr);

		/*
		 * Process any internal loopback packets.
		 */
		while(bp = getq(&ctlr->lbq)){
			ctlr->inpackets++;
			etherup(ctlr, bp->rptr, BLEN(bp));
			freeb(bp);
		}

		/*
		 * Process any received packets.
		 */
		rxpkt = &ctlr->rda[ctlr->rh];
		while(rxpkt->owner == Host){
			ctlr->inpackets++;

			/*
			 * If the packet was received OK, pass it up,
			 * otherwise log the error.
			 * SONIC gives us the CRC in the packet, so
			 * remember to subtract it from the length.
			 */
			if(rxpkt->status & Prx)
				etherup(ctlr, ctlr->rb[ctlr->rh], (rxpkt->count & 0xFFFF)-4);
			else if(rxpkt->status & Fae)
				ctlr->frames++;
			else if(rxpkt->status & Crc)
				ctlr->crcs++;
			else
				ctlr->buffs++;

			/*
			 * Finished with this packet, it becomes the
			 * last free packet in the ring, so give it Eol,
			 * and take the Eol bit off the previous packet.
			 * Move the ring index on.
			 */
			rxpkt->link |= Eol;
			rxpkt->owner = Interface;
			ctlr->rda[PREV(ctlr->rh, Nrb)].link &= ~Eol;
			ctlr->rh = NEXT(ctlr->rh, Nrb);

			rxpkt = &ctlr->rda[ctlr->rh];
		}

		/*
		 * Close Types requesting it.
		 */
		if(ctlr->clist){
			lock(&ctlr->clock);
			for(tp = ctlr->clist; tp; tp = tp->clist){
				tp->q = 0;
				wakeup(&tp->cr);
			}
			ctlr->clist = 0;
			unlock(&ctlr->clock);
		}
	}
}

Chan*
etherattach(char *spec)
{
	Ctlr *ctlr = softctlr[0];

	/*
	 * Start the kproc and
	 * put the receiver online.
	 */	
	if(ctlr->kproc == 0){
		sprint(ctlr->name, "ether%dkproc", 0);
		kproc(ctlr->name, etherkproc, ctlr);
		ctlr->kproc = 1;
		enable(ctlr);
		print("sonic ether: %2.2ux %2.2ux %2.2ux %2.2ux %2.2ux %2.2ux\n",
		     ctlr->ea[0], ctlr->ea[1], ctlr->ea[2], ctlr->ea[3], ctlr->ea[4], ctlr->ea[5]); 
	}
	return devattach('l', spec);
}
