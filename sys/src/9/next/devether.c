/*
 *  Next ethernet device
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"io.h"
#include	"devtab.h"

typedef struct Ercvdma		Ercvdma;
typedef struct Exmtdma		Exmtdma;
typedef struct Ebyte		Ebyte;
typedef struct Enet		Enet;
typedef struct Etherbuf		Etherbuf;
typedef struct Ethertype	Ethertype;
typedef struct Trace		Trace;
typedef struct Debqueue		Debqueue;

#define NOW (MACHP(0)->ticks*MS2HZ)

enum
{
	Nr=	16,		/* number of receive buffers */
	Nx=	16, 		/* number of xmit buffers */
	Ndpkt=	16,		/* debug packets/interface */
	Ntypes=	9,		/* types/interface */
};

int	pether;	/* debug flag */

/*
 *  receive DMA
 */
struct Ercvdma {
	ulong	csr;		/* control/status */
	uchar	pad1[0x4140-0x0154];
	ulong	savedbase;
	ulong	lastaddr;	/* limit of current dma */
	ulong	pad2;
	ulong	pad3;
	ulong	base;		/* base of current dma */
	ulong	limit;		/* limit of current dma */
	ulong	chainbase;	/* base of next dma */
	ulong	chainlimit;	/* limit of next dma */
};

/*
 *  transmit DMA
 */
struct Exmtdma {
	ulong	csr;		/* control/status */
	uchar	pad1[0x4100-0x0114];
	ulong	savedbase;	/* goo in case we abort a packet */
	ulong	savedlimit;	/* 	''	*/
	ulong	savedcbase;	/* 	'' 	*/
	ulong	savedclimit;	/* 	'' 	*/
	ulong	baser;		/* base of current dma (read only) */
	ulong	limit;		/* limit of current dma */
	ulong	chainbase;	/* base of next dma */
	ulong	chainlimit;	/* limit of next dma */
	uchar	pad2[0x4310-0x4120];
	ulong	basew;		/* base of current dma (write only) */
};

/*
 *  ethernet byte registers
 */
struct Ebyte {
	uchar	ts;	/* transmitter status */
	uchar	tie;	/* transmitter interrupt enable */
	uchar	rs;	/* receiver status */
	uchar	rie;	/* receiver interrupt enable */
	uchar	tm;	/* transmitter mode */
	uchar	rm;	/* receiver mode */
	uchar	reset;	/* reset control */
	uchar	pad;
	uchar	nid[6];	/* network id */
};

/*
 *  register bit definitions
 */
enum {
	/* enet.ts and enet.tie */
	Xmtrdy=		1<<7,	/* ready for new packet */
	Netbsy=		1<<6,	/* network in use */
	Eschizoid=	1<<5,	/* true when we talk to ourselves */
	Euflow=		1<<3,	/* we screwed up the chaining */
	Ecoll=		1<<2,	/* collision occurred */
	Ecoll16=	1<<1,	/* Error, couldn't xmit packet discarded */
	Eparity=	1<<0,	/* parity error */

	/* enet.rs and enet.rie */
	Pktok=		1<<7,	/* packet successfully received */
	Rstpkt=		1<<4,	/* reset packet received */
	Eshort=		1<<3,	/* Error, short packet received */
	Ealign=		1<<2,	/* Error, bad allignment */
	Ecrc=		1<<1,	/* Error, bad crc */
	Eoflow=		1<<0,	/* Error, packet overflow */

	/* enet.tm */
	Ca=		0xF<<4,	/* collision attempts on last xmt */
	Noloop=		1<<1,	/* loop back control, 0 == looped back */
	Discont=	1<<0, 	/* MA/CD instead of CSMA/CD */

	/* enet.rm */
	Addsize=	1<<4,	/* use a 5 byte address match */
	Enshortpkt=	1<<3,	/* enable short packet */
	Enrst=		1<<2,	/* enable ether reset packets */
	Anone=		0<<0,	/* accept no packets */
	Anormal=	1<<0,	/* accept only broadcasts and my packets */
	ANeXT=		2<<0,	/* accept NeXT multicasts */
	Aall=		3<<0,	/* accept all packets */

	/* enet.reset */
	Reset=		1<<7,	/* if on, the device is useless */

	/* Etherbuf.owner */
	Chip = 0,
	Cpu = 1,
};

/*
 *  Ethernet packet buffer
 */
struct Etherbuf {
	Etherpkt;
	ushort	len;		/* high word of len */
	uchar	owner;		/* 0 is chip, other is CPU */
	uchar	pad1;
	ulong	phys;		/* physical address */
#ifdef TIMINGS
	ushort	timer;
	uchar	pad[6];		/* Etherpkt's must be quad-word aligned */
#else
	uchar	pad[8];		/* Etherpkt's must be quad-word aligned */
#endif
};

/*
 *  one per ethernet packet type
 */
struct Ethertype{
	QLock;
	Netprot;			/* stat info */
	int		type;		/* ethernet type */
	int		prom;		/* promiscuous mode */
	Queue		*q;
	int		inuse;
	Rendez		rc;		/* rendzvous for close */
	Ethertype	*closeline;	/* close list */
};

/*
 *  per ethernet
 */
struct Enet {
	QLock;

	/*
	 *  register pointers
 	 */
	Ercvdma	*r;	/* rcv dma */
	Exmtdma	*x;	/* xmt dma */
	Ebyte	*b;	/* byte registers */

	Rendez	rr;		/* rendezvous for an input buffer */
	Etherbuf *rf;		/* first unprocessed input message */
	Etherbuf *rn;		/* next buffer to be read into */
	Etherbuf *rb;
	int	rcving;

	QLock	xl;
	Rendez	xr;		/* rendezvous for an output buffer */
	Etherbuf *xf;		/* first staged message */	
	Etherbuf *xn;		/* next message to stage */
	Etherbuf *xb;
	int	xmting;

	Ethertype *closeline;	/* channels waiting to close */
	Lock	closepin;	/* lock for closeline */
	Ethertype e[Ntypes];
	int	kstarted;
	uchar	ea[6];
	uchar	bcast[6];

	Queue	self;
	int	prom;		/* number of promiscuous channels */
	int	all;		/* number of channels listening to all packets */
	Network	net;
	Netprot	prot[Ntypes];

	/*
	 *  sadistics
	 */
	int	inpackets;
	int	outpackets;
	int	crcs;		/* input crc errors */
	int	oerrs;		/* output erros */
	int	frames;		/* framing errors */
	int	overflows;	/* packet overflows */
	int	buffs;		/* buffering errors */
	ulong	wait;
};
Enet enet;

/*
 *  for walking xmt/rcv rings
 */
#define RSUCC(x)	((x+1) == enet.rb+Nr ? enet.rb : (x+1))
#define XSUCC(x)	((x+1) == enet.xb+Nx ? enet.xb : (x+1))

/*
 *  predeclared
 */
void	etherkproc(void*);
void	ethersend(Etherbuf*);
static int	etherclonecon(Chan*);
static void	etherstatsfill(Chan*, char*, int);
static void	ethertypefill(Chan*, char*, int);

/*
 *  ether stream module definition
 */
static void etheroput(Queue*, Block*);
static void etherstopen(Queue*, Stream*);
static void etherstclose(Queue*);
Qinfo etherinfo = {
	nullput,
	etheroput,
	etherstopen,
	etherstclose,
	"ether"
};

/*
 *  allocate packet buffers, reset interface
 */
void
etherreset(void)
{
	int i;
	Etherbuf *p, *v;
	ulong l;

	enet.r = (Ercvdma*)ERCVDMA;
	enet.x = (Exmtdma*)EXMTDMA;
	enet.b = (Ebyte*)ENETB;

	/*
	 * Map buffers in virtual space using kmappa to avoid caches.
	 * Remember their physical address.
	 */
	p = xspanalloc(sizeof(Etherbuf)*Nr, BY2PG, 0);
	enet.rb = (Etherbuf*)kmappa((ulong)p);		/* map first */
	for(l=BY2PG; l<sizeof(Etherbuf)*Nr; l+=BY2PG)
		kmappa(((ulong)p) + l);			/* map rest */
	for(v=enet.rb; v<enet.rb + Nr; v++,p++){
		v->owner = Chip;
		v->phys = (ulong)p;
	}

	p = xspanalloc(sizeof(Etherbuf)*Nx, BY2PG, 0);
	enet.xb = (Etherbuf*)kmappa((ulong)p);		/* map first */
	for(l=BY2PG; l<sizeof(Etherbuf)*Nx; l+=BY2PG)
		kmappa(((ulong)p) + l);			/* map rest */
	for(v=enet.xb; v<enet.xb + Nx; v++,p++){
		v->owner = Cpu;
		v->phys = (ulong)p;
	}

	/*
	 *  read address out of chip (BUG: should come from ROM)
	 */

	enet.b->rm = 0;
	enet.b->tm = 0;
	for(i = 0; i < sizeof enet.ea; i++)
		enet.ea[i] = enet.b->nid[i];

	enet.x->csr = Dcreset | Dclrcint;
	enet.r->csr = Dcreset | Dclrcint;
	enet.b->reset = Reset;

	enet.net.name = "ether";
	enet.net.nconv = Ntypes;
	enet.net.devp = &etherinfo;
	enet.net.protop = 0;
	enet.net.listen = 0;
	enet.net.clone = etherclonecon;
	enet.net.ninfo = 2;
	enet.net.info[0].name = "stats";
	enet.net.info[0].fill = etherstatsfill;
	enet.net.info[1].name = "type";
	enet.net.info[1].fill = ethertypefill;
	for(i = 0; i < Ntypes; i++)
		netadd(&enet.net, &enet.e[i], i);

	memset(enet.bcast, 0xff, sizeof enet.bcast);
}

void
etherinit(void)
{
	/*
 	 *  init pointers and ownership
	 */
	enet.rf = enet.rn = enet.rb;
	enet.xf = enet.xn = enet.xb;
	enet.xmting = enet.rcving = 0;

	/*
	 *  turn off interrupt sources
	 */
	enet.b->rs = Pktok | Rstpkt | Eshort | Ealign | Ecrc | Eoflow;
	enet.b->ts = Euflow | Ecoll | Ecoll16 | Eparity;

	/*
	 *  enable for all sorts of interrupts
	 */
	enet.b->tie = Euflow | Ecoll | Ecoll16 | Eparity;
	enet.b->rie = Eshort | Ealign | Ecrc | Eoflow;
	enet.b->tm = Noloop;
	enet.b->rm = Anormal;

	/*
	 *  enable the DMA engine
	 */
	enet.b->reset = 0;

	if(enet.kstarted == 0)
		kproc("ether", etherkproc, 0);
}

Chan *
etherattach(char *spec)
{
	return devattach('l', spec);
}

Chan *
etherclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
etherwalk(Chan *c, char *name)
{
	return netwalk(c, name, &enet.net);
}

void
etherstat(Chan *c, char *dp)
{
	netstat(c, dp, &enet.net);
}

Chan *
etheropen(Chan *c, int omode)
{
	return netopen(c, omode, &enet.net);
}

void
ethercreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
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
	netwstat(c, dp, &enet.net);
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
	USED(offset);
	return netread(c, a, n, offset,  &enet.net);
}

long
etherwrite(Chan *c, char *a, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, a, n, 0);
}

/*
 *  user level network interface routines
 */
static void
etherstatsfill(Chan *c, char* p, int n)
{
	char buf[256];

	USED(c);
	sprint(buf, "in: %d\nout: %d\nwait: %lud\ncrc: %d\noverflows: %d\nframing %d\nbuff: %d\noerr: %d\naddr: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
		enet.inpackets, enet.outpackets, enet.wait, enet.crcs,
		enet.overflows, enet.frames, enet.buffs, enet.oerrs,
		enet.ea[0], enet.ea[1], enet.ea[2], enet.ea[3], enet.ea[4], enet.ea[5]);
	strncpy(p, buf, n);
}
static void
ethertypefill(Chan *c, char* p, int n)
{
	char buf[16];
	Ethertype *e;

	e = &enet.e[STREAMID(c->qid.path)];
	sprint(buf, "%d", e->type);
	strncpy(p, buf, n);
}

static int
etherclonecon(Chan *c)
{
	Ethertype *e;

	USED(c);
	for(e = enet.e; e < &enet.e[Ntypes]; e++){
		qlock(e);
		if(e->inuse || e->q){
			qunlock(e);
			continue;
		}
		e->inuse = 1;
		netown(e, u->p->user, 0);
		qunlock(e);
		return e - enet.e;
	}
	exhausted("ether channels");
	return 0;
}

/*
 *  find a stream and pass an input packet up
 *  it
 */
void
etherup(Etherpkt *p, int len)
{
	Block *bp;
	Ethertype *e;
	int t;

	t = (p->type[0]<<8) | p->type[1];
	for(e = &enet.e[0]; e < &enet.e[Ntypes]; e++){
		if(e->q==0 || (t!=e->type && e->type!=-1) || e->q->next->len>Streamhi)
			continue;

		/*
		 *  only a trace channel gets packets destined for other machines
		 */
		if(e->type!=-1 && p->d[0]!=0xff
		&& (*p->d != *enet.ea || memcmp(p->d, enet.ea, sizeof(p->d)) != 0))
			continue;

		if(!waserror()){
			bp = allocb(len);
			memmove(bp->rptr, (uchar *)p, len);
			bp->wptr += len;
			bp->flags |= S_DELIM;
			PUTNEXT(e->q, bp);
		}
		poperror();
		qunlock(e);
	}
}

/*
 *  loop forever, reading input packets and sending them
 *  upstream.
 */
static int
isinput(void *arg)
{
	USED(arg);	/* not */
	return enet.rf->owner==Cpu || enet.rcving==0;
}
void
etherkproc(void *arg)
{
	Ethertype *e;
	Etherbuf *next;
	Block *bp;

	USED(arg);	/* not */

	if(waserror()){
		print("someone noted etherkproc\n");
		enet.kstarted = 0;
		nexterror();
	}
	enet.kstarted = 1;

	for(;;){
		sleep(&enet.rr, isinput, 0);
		if(enet.closeline){
			/*
			 *  close ethertypes requesting it
			 */
			lock(&enet.closepin);
			for(e = enet.closeline; e; e = e->closeline){
				e->q = 0;
				wakeup(&e->rc);
			}
			enet.closeline = 0;
			unlock(&enet.closepin);
		}
		while(bp = getq(&enet.self)){
			enet.inpackets++;
			etherup((Etherpkt*)bp->rptr, BLEN(bp));
			freeb(bp);
		}
		while(enet.rf->owner == Cpu){
			enet.inpackets++;
			etherup(enet.rf, enet.rf->len);
			enet.rf->owner = Chip;
			enet.rf = RSUCC(enet.rf);
		}
		if(enet.rcving==0){
			/*
			 *  get input going again
			 */
			next = RSUCC(enet.rn);
			if(enet.rn->owner==Chip && next->owner==Chip){
				enet.rcving = 1;
				enet.r->base = enet.rn->phys;
				enet.r->limit = enet.rn->phys+sizeof(Etherpkt);
				enet.r->chainbase = next->phys;
				enet.r->chainlimit = next->phys+sizeof(Etherpkt);
				enet.r->csr = Dsetchain | Dseten;
			}
		}
	}
}


/*
 *  rcv DMA interrupt
 */
void
etherdmarintr(void)
{
	ulong csr;
	Etherbuf *next;
	int len;
	ulong lastaddr, savedbase;

	lastaddr = enet.r->lastaddr;
	savedbase = enet.r->savedbase;

	/*
	 *  packet has been received, calculate length,
	 *  wakeup kproc
	 */
	len = (lastaddr&0x3FFFFFFF)-(savedbase&0x3FFFFFFF)- 4;
	if(len<0 || len>sizeof(Etherpkt)){
		/*
		 *  A second CINT interrupt for a packet.
		 *  I don't know why it happens -- presotto.
		 */
		csr = enet.r->csr;
		if(csr & Dcint)
			enet.r->csr = Dclrcint;
		if((csr&Den) == 0){
			enet.r->csr = Dcreset | Dclrcint;
			enet.rcving = 0;
			wakeup(&enet.rr);
		}
		return;
	}

	enet.rn->owner = Cpu;
	enet.rn->len = len;
#ifdef TIMINGS
	enet.rn->timer = timings.ercv;
#endif
	enet.rn = RSUCC(enet.rn);

	csr = enet.r->csr;
	if(csr & Dcint){
		/*
		 *  a chain interrupt
		 */
		next = RSUCC(enet.rn);
		if(next->owner == Chip){
			/*
			 *  continue chain
			 */
			enet.r->chainbase = next->phys;
			enet.r->chainlimit = next->phys+sizeof(Etherpkt);
			enet.r->csr = Dclrcint | Dsetchain;
		} else {
			/*
			 *  no free bufs, terminate chain
			 */
			enet.r->csr = Dclrcint;
		}
	}
	if((csr&Den) == 0){
		/*
		 *  end of DMA, reset interrupt
		 */
		enet.r->csr = Dcreset | Dclrcint;
		enet.rcving = 0;
	}
	wakeup(&enet.rr);
}

/*
 *  other rcv interupts
 */
void
etherrintr(void)
{
	ushort status;

	status = enet.b->rs;
	enet.b->rs = Pktok | Rstpkt | Eshort | Ealign | Ecrc | Eoflow;
	if(status & (Eshort|Eoflow)){
		if(status & Ecrc)
			enet.crcs++;
		if(status & (Eshort|Ealign))
			enet.frames++;
		if(status & Eoflow)
			enet.overflows++;
	}
}

/*
 *  xmit dma complete
 */
void
etherdmaxintr(void)
{
	int i;

	if(enet.x->csr&Dcint)
		enet.x->csr = Dclrcint;

	/* give the dma time to raise Xmtrdy (so we can avoid a second intr) */
	for(i = 0; i < 20 && (enet.b->ts&Xmtrdy) == 0; i++)
		;

	if(enet.xmting && (enet.b->ts&Xmtrdy)){
		/*
		 *  give buffer back to Cpu
		 */
		enet.xf->owner = Cpu;
		enet.xf = XSUCC(enet.xf);

		if(enet.xf->owner != Chip){
			/*
			 *  reset the interrupt
			 */
			enet.x->csr = Dcreset;
			enet.xmting = 0;
		} else {
			/*
			 *  wakeup anyone waiting for a buffer.
			 *  xmit next packet
			 */
			wakeup(&enet.xr);
			ethersend(enet.xf);
		}
	}
}

/*
 *  other flavor xmit intrs
 */
void
etherxintr(void)
{
	ushort status;

	status = enet.b->ts;
	enet.b->ts = Euflow | Ecoll | Ecoll16 | Eparity;
	if(status & (Euflow|Ecoll|Ecoll16|Eparity)){
/*		print("etherxintr %ux\n", status); /**/
		if(status & Euflow)
			enet.buffs++;
		if(status & Ecoll16)
			enet.oerrs++;
	}
}

/*
 *  print a packet preceded by a message
 */
void
sprintpacket(char *buf, char tag, Etherbuf *b)
{
	int i;

	sprint(buf, "%c: %.4d d(%.2ux%.2ux%.2ux%.2ux%.2ux%.2ux)s(%.2ux%.2ux%.2ux%.2ux%.2ux%.2ux)t(%.2ux %.2ux)d(",
		tag, b->len,
		b->d[0], b->d[1], b->d[2], b->d[3], b->d[4], b->d[5],
		b->s[0], b->s[1], b->s[2], b->s[3], b->s[4], b->s[5],
		b->type[0], b->type[1]);
	for(i=0; i<60 && i<b->len; i++)
		sprint(buf+strlen(buf), "%.2ux", b->data[i]);
	sprint(buf+strlen(buf), ")");
}

/*
 *  open a ether line discipline
 */
void
etherstopen(Queue *q, Stream *s)
{
	Ethertype *et;

	et = &enet.e[s->id];
	RD(q)->ptr = WR(q)->ptr = et;
	et->type = 0;
	et->q = RD(q);
	et->inuse = 1;
}

/*
 *  close ether line discipline
 *
 *  the lock is to synchronize changing the ethertype with
 *  sending packets up the stream on interrupts.
 */
static int
isclosed(void *x)
{
	return ((Ethertype *)x)->q == 0;
}
static void
etherstclose(Queue *q)
{
	Ethertype *et;

	et = (Ethertype *)(q->ptr);
	if(et->prom){
		qlock(&enet);
		enet.prom--;
		if(enet.prom == 0)
			enet.b->rm = Anormal;
		qunlock(&enet);
	}
	if(et->type == -1){
		qlock(&enet);
		enet.all--;
		qunlock(&enet);
	}

	/*
	 *  mark as closing and wait for kproc to close us
	 */
	lock(&enet.closepin);
	et->closeline = enet.closeline;
	enet.closeline = et;
	unlock(&enet.closepin);
	wakeup(&enet.rr);
	sleep(&et->rc, isclosed, et);

	et->type = 0;
	et->q = 0;
	et->prom = 0;
	et->inuse = 0;
	netdisown(et);
}

/*
 *  the ``connect'' control message specifyies the type
 */
static int
isobuf(void *x)
{
	USED(x);
	return enet.xn->owner == Cpu;
}
static void
etheroput(Queue *q, Block *bp)
{
	int len, n;
	Ethertype *e;
	Etherpkt *p;
	Block *nbp;

	if(bp->type == M_CTL){
		e = q->ptr;
		qlock(&enet);
		if(streamparse("connect", bp)){
			if(e->type == -1)
				enet.all--;
			e->type = strtol((char *)bp->rptr, 0, 0);
			if(e->type == -1)
				enet.all++;
		} else if(streamparse("promiscuous", bp)) {
			e->prom = 1;
			enet.prom++;
			if(enet.prom == 1)
				enet.b->rm = Aall;
		}
		qunlock(&enet);
		freeb(bp);
		return;
	}

	/*
	 *  give packet a local address, return upstream if destined for
	 *  this machine.
	 */
	if(BLEN(bp) < ETHERHDRSIZE){
		bp = pullup(bp, ETHERHDRSIZE);
		if(bp == 0)
			return;
	}
	p = (Etherpkt *)bp->rptr;
	memmove(p->s, enet.ea, sizeof(enet.ea));
	if(*p->d == 0xff || enet.prom || enet.all){
		len = blen(bp);
		nbp = copyb(bp, len);
		nbp = expandb(nbp, len >= ETHERMINTU ? len : ETHERMINTU);
		if(nbp){
			nbp->wptr = nbp->rptr+len;
			putq(&enet.self, nbp);
			wakeup(&enet.rr);
		}
	} else if(*p->d == *enet.ea && memcmp(enet.ea, p->d, sizeof(enet.ea)) == 0){
		len = blen(bp);
		bp = expandb(bp, len >= ETHERMINTU ? len : ETHERMINTU);
		if(bp){
			putq(&enet.self, bp);
			wakeup(&enet.rr);
		}
		return;
	}

	/*
	 *  only one transmitter at a time
	 */
	qlock(&enet.xl);
	if(waserror()){
		qunlock(&enet.xl);
		freeb(bp);
		nexterror();
	}

	/*
	 *  Wait till we get an output buffer
	 */
	if(enet.xn->owner != Cpu)
		sleep(&enet.xr, isobuf, (void *)0);
	p = enet.xn;

	/*
	 *  copy message into buffer
	 */
	len = 0;
	for(nbp = bp; nbp; nbp = nbp->next){
		if(sizeof(Etherpkt) - len >= (n = BLEN(nbp))){
			memmove(((uchar *)p)+len, nbp->rptr, n);
			len += n;
		} else
			print("no room damn it\n");
		if(bp->flags & S_DELIM)
			break;
	}

	/*
	 *  pad the packet (zero the pad)
	 */
	if(len < ETHERMINTU){
		memset(((char*)p)+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}
	enet.xn->len = len;

	/*
	 *  give to Chip
	 */
	splhi();		/* sync with interrupt routine */
	enet.xn->owner = Chip;
	if(enet.xmting == 0)
		ethersend(enet.xn);
	spllo();

	/*
	 *  send
	 */
	enet.xn = XSUCC(enet.xn);
	qunlock(&enet.xl);
	freeb(bp);
	poperror();

	enet.outpackets++;
}

static char	xxbuf[1000];

void
ethersend(Etherbuf *b)
{
	ulong a;

	if(enet.x->csr & Den)
		panic("ethersend");
	/*
	 *  turn off everything
	 */
	enet.x->csr = Dinit | Dcreset | Dclrcint;
	enet.x->csr = 0;

	/*
	 *  set up a new xmit (chip is wrong by 15)
	 */
	a = b->phys;
	enet.x->basew = a;
	enet.x->savedbase = a;
	a = (a + b->len + 15) | 0x80000000;
	enet.x->limit = a;
	enet.x->savedlimit = a;

	/*
	 *  start xmit
	 */
	enet.x->csr = Dseten;
	enet.xmting = 1;
}

void
ethertoggle(void)
{
	pether ^= 1;
}
