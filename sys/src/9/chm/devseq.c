#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"devtab.h"

#define	DEBUG	if(0)kprint

typedef struct Hio	 Hio;
typedef struct Hstate	 Hstate;
typedef struct Rdesc	 Rdesc;
typedef struct Xdesc	 Xdesc;
typedef struct Ethertype Ethertype;

enum
{
	Eor=	1<<31,		/* end of ring */
	Eorp=	1<<30,
	Rxie=	1<<29,
	Empty=	1<<14,		/* no data here */
};

struct Rdesc
{
	ulong	addr;		/* addr */
	ulong	count;		/* eor / empty / count:13 */
	Rdesc*	next;
	uchar*	base;
};

enum
{
	Eox=	1<<31,		/* end of transmission */
	Eoxp=	1<<30,		/* end of transmitted packet */
	Busy=	1<<24,
	Xie=	1<<29,		/* interrupt enable */
};

struct Xdesc
{
	ulong	addr;		/* addr */
	ulong	count;		/* eox / eop / busy / xie / count:13 */
	Xdesc*	next;
	uchar*	base;
};

/*
 * SEEQ 8003 interfaced to HPC3 (very different from IP20)
 */
struct Hio
{
	ulong	unused[20480];
	ulong	crbp;		/* current receive buf desc ptr */
	ulong	nrbdp;		/* next receive buf desc ptr */
	ulong	unused[1022];
	ulong	rbc;		/* receive byte count */
	ulong	rstat;		/* receiver status */
	ulong	rgio;		/* receive gio fifo ptr */
	ulong	rdev;		/* receive device fifo ptr */
	ulong	unused;
	ulong	ctl;		/* interrupt, channel reset, buf oflow */
	ulong	dmacfg;		/* dma configuration */
	ulong	piocfg;		/* pio configuration */
	ulong 	unused[1016];
	ulong	cxbdp;		/* current xmit buf desc ptr */
	ulong	nxbdp;		/* next xmit buffer desc. pointer */
	ulong	unused[1022];
	ulong	xbc;		/* xmit byte count */
	ulong	xstat;
	ulong	xgio;		/* xmit gio fifo ptr */
	ulong	xdev;		/* xmit device fifo ptr */
	ulong	unused[1020];
	ulong	crbdp;		/* current receive descriptor ptr */
	ulong	unused[2047];
	ulong	cpfxbdp;	/* current/previous packet 1st xmit */
	ulong	ppfxbdp;	/* desc ptr */
	ulong	unused[59390];
	ulong	eaddr[6];	/* seeq station address wo */
	ulong	csr;		/* seeq receiver cmd/status reg */
	ulong	csx;		/* seeq transmitter cmd/status reg */
};

enum
{			/* ctl */
	Cover=	0x08,		/* receive buffer overflow */
	Cnormal=0x00,		/* 1=normal, 0=loopback */
	Cint=	0x02,		/* interrupt (write 1 to clear) */
	Creset=	0x01,		/* ethernet channel reset */

			/* xstat */
	Xdma=	0x200,		/* dma active */
	Xold=	0x080,		/* register has been read */
	Xok=	0x008,		/* transmission was successful */
	Xmaxtry=0x004,		/* transmission failed after 16 attempts */
	Xcoll=	0x002,		/* transmission collided */
	Xunder=	0x001,		/* transmitter underflowed */

			/* csx */
	Xreg0=	0x00,		/* access reg bank 0 incl station addr */
	XIok=	0x08,
	XImaxtry=0x04,
	XIcoll=	0x02,
	XIunder=0x01,

			/* rstat */
	Rlshort=0x800,		/* [small len in received frame] */
	Rdma=	0x200,		/* dma active */
	Rold=	0x80,		/* register has been read */
	Rok=	0x20,		/* received good frame */
	Rend=	0x10,		/* received end of frame */
	Rshort=	0x08,		/* received short frame */
	Rdrbl=	0x04,		/* dribble error */
	Rcrc=	0x02,		/* CRC error */
	Rover=	0x01,		/* overflow error */

			/* csr */
	Rsmb=	0xc0,		/* receive station/broadcast/multicast frames */
	Rsb=	0x80,		/* receive station/broadcast frames */
	Rprom=	0x40,		/* receive all frames */
	RIok=	0x20,	 	/* interrupt on good frame */
	RIend=	0x10,		/* interrupt on end of frame */
	RIshort=0x08,		/* interrupt on short frame */
	RIdrbl=	0x04,		/* interrupt on dribble error */
	RIcrc=	0x02,		/* interrupt on CRC error */
	RIover=	0x01,		/* interrupt on overflow error */

	HPC_MODNORM=	0x0,	/* mode: 0=normal, 1=loopback */
	HPC_FIX_INTR=	0x8000,	/* start timeout counter after */
	HPC_FIX_EOP=	0x4000,	/* rcv_eop_intr/eop_in_chip is set */ 
	HPC_FIX_RXDC=	0x2000,	/* clear eop status upon rxdc */
};

/*
 *  one per ethernet packet type
 */
struct Ethertype
{
	QLock;
	Netprot;			/* stat info */
	int		type;		/* ethernet type */
	int		prom;		/* promiscuous mode */
	Queue*		q;
	int		inuse;
	Rendez		rc;		/* rendzvous for close */
	Ethertype*	closeline;	/* close list */
};

enum
{
	Ntypes=	9		/* max number of ethernet packet types */
};

struct Hstate
{
	QLock;
	int	inited;
	uchar	ea[6];		/* ethernet address */
	int	prom;		/* # of promiscuous channels */
	int	all;		/* # of chans listening to all packets */
	int	wedged;		/* the device is wedged */
	Network	net;

	uchar*	rbase;
	Rdesc*	rring;
	Rdesc*	rhead;

	Rendez	tr;		/* rendezvous for an output buffer */
	QLock	trlock;		/* semaphore on tr */
	Rendez	rr;		/* rendezvous for an input buffer */
	QLock	rlock;		/* semaphore on rr */
	QLock	tlock;		/* semaphore on Xdesc chain */
	int	tfree;		/* count of available Xdesc's */
	uchar*	tbase;
	Xdesc*	tring;
	Xdesc*	thead;		/* scavenge transmitted blocks from here */
	Xdesc*	ttail;		/* append blocks to be transmitted here */

	Ethertype*closeline;	/* channels waiting to close */
	Lock	closepin;	/* lock for closeline */
	Ethertype e[Ntypes];
	int	debug;
	int	kstarted;
	uchar	bcast[6];

	Queue	self;		/* packets turned around at the interface */

	/* sadistics */
	int	inpackets;
	int	inchars;
	int	outpackets;
	int	outchars;
	int	over;		/* input overflow */
	int	shrt;		/* short packet (status) */
	int	lshrt;		/* short packet (len) */
	int	drbl;		/* dribble error */
	int	crc;		/* crc error */
	int	maxtry;		/* transmission failure (16 tries) */
	int	under;		/* transmitter underflow */
};

#define	NXRING	64
#define	NRRING	128
#define	RBSIZE	(sizeof(Etherpkt)+3)

#define	DEV	IO(Hio, HPC3_ETHER)

static 	void	etherstart(int);
static 	void	etherhardreset(void);

static Hstate ether;

/*
 *  predeclared
 */
static	int	seqclonecon(Chan*);
static	void	seqkproc(void *);
static	void	seqstart(int);
static	void	seqstatsfill(Chan*, char*, int);
static	void	seqtypefill(Chan*, char*, int);
static	void	sequp(void*, int);
static	void	xintr(int);
	void	seqintr(Ureg*, void*);

/*
 *  seq stream module definition
 */
static void seqoput(Queue*, Block*);
static void seqstopen(Queue*, Stream*);
static void seqstclose(Queue*);
Qinfo seqinfo = { nullput, seqoput, seqstopen, seqstclose, "seq" };

ulong
rdreg(ulong *p)
{
	ulong s, v;

	s = splhi();
	v = DEV->piocfg;
	USED(v);
	v = *p;
	splx(s);
	return v;
}

/*
 *  open a seq line discipline
 */
static void
seqstopen(Queue *q, Stream *s)
{
	Ethertype *e;

	e = &ether.e[s->id];
	RD(q)->ptr = WR(q)->ptr = e;
	e->type = 0;
	e->q = RD(q);
	e->inuse = 1;
}

/*
 *  close seq line discipline
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
seqstclose(Queue *q)
{
	Ethertype *e;

	e = q->ptr;
	if(e->prom){
		qlock(&ether);
		ether.prom--;
		if(ether.prom == 0)
			seqstart(0);
		qunlock(&ether);
	}
	if(e->type == -1){
		qlock(&ether);
		ether.all--;
		qunlock(&ether);
	}

	/*
	 *  mark as closing and wait for kproc to close us
	 */
	lock(&ether.closepin);
	e->closeline = ether.closeline;
	ether.closeline = e;
	unlock(&ether.closepin);
	wakeup(&ether.rr);
	sleep(&e->rc, isclosed, e);
	
	e->type = 0;
	e->q = 0;
	e->prom = 0;
	e->inuse = 0;
	netdisown(e);
}

static int
isobuf(void *arg)
{
	return ether.tfree > (long)arg;
}

static void
seqoput(Queue *q, Block *bp)
{
	Hio *dev;
	Xdesc *xp;
	uchar *dp;
	Block *nbp;
	Etherpkt *p;
	Ethertype *e;
	int len, k, n, s;

	dev = DEV;

	if(bp->type == M_CTL){
		e = q->ptr;
		qlock(&ether);
		if(streamparse("connect", bp)){
			if(e->type == -1)
				ether.all--;
			e->type = strtol((char *)bp->rptr, 0, 0);
			if(e->type == -1)
				ether.all++;
		}
		else
		if(streamparse("promiscuous", bp)){
			e->prom = 1;
			if(++ether.prom == 1)
				seqstart(1);
		}
		qunlock(&ether);
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
	memmove(p->s, ether.ea, sizeof(ether.ea));
	len = blen(bp);
	if(len < ETHERMINTU){
		bp = expandb(bp, ETHERMINTU);
		len = ETHERMINTU;
	}else if(len > ETHERMAXTU){
		print("seqoput len=%d\n", len);
		freeb(bp);
	}
	if(*p->d == 0xff || ether.prom || ether.all){
		nbp = copyb(bp, len);
		if(nbp->next)
			nbp = expandb(nbp, len);
		if(nbp){
			nbp->wptr = nbp->rptr+len;
			putq(&ether.self, nbp);
			wakeup(&ether.rr);
		}
	}
	else
	if(*p->d == *ether.ea && memcmp(ether.ea, p->d, sizeof(ether.ea)) == 0){
		if(bp->next)
			bp = expandb(bp, len);
		if(bp){
			putq(&ether.self, bp);
			wakeup(&ether.rr);
		}
		return;
	}
	if(ether.wedged){
		freeb(bp);
		return;
	}

	/*
	 *  only one transmitter at a time
	 */
	qlock(&ether.trlock);
	qlock(&ether.tlock);
	while(!isobuf(&ether)){
		qunlock(&ether.tlock);
		sleep(&ether.tr, isobuf, 0);
		qlock(&ether.tlock);
	}
	if(ether.wedged){
		qunlock(&ether.tlock);
		qlock(&ether.trlock);
		freeb(bp);
		return;
	}
	++ether.outpackets;
	--ether.tfree;
	ether.outchars += len;
	xp = IO(Xdesc, ether.ttail->next);
	dp = xp->base;
	for(nbp=bp; nbp; nbp=nbp->next){
		n = BLEN(nbp);
		if(n == 0)
			continue;
		memmove(dp, nbp->rptr, n);
		dp += n;
	}
	n = dp - xp->base;
	xp->addr = PADDR(xp->base);		/* end of transmission */
	xp->count = Eox|Eoxp|Busy|Xie|n;	/* end of packet */

	s = splhi();
	ether.tfree--;
	k = rdreg(&dev->xstat);
	if(k & Xdma){
		DEBUG("\tappend\n");
		ether.ttail->count &= ~(Eox|Xie);
	}
	else
		xintr(k);
	splx(s);
	ether.ttail = xp;

	qunlock(&ether.tlock);
	qunlock(&ether.trlock);
	freeb(bp);
}

static
void
initmem(void)
{
	int i;
	Hio *dev;
	Xdesc *xp;
	Rdesc *rp;

	dev = DEV;

	dev->csx = Xreg0;

	/* allocate receiver ring */
	rp = xspanalloc(NRRING*sizeof(Rdesc), BY2PG, 0);
	dcflush(rp, NRRING*sizeof(Rdesc));
	rp = IO(Rdesc, rp);
	memset(rp, 0, NRRING*sizeof(Rdesc));
	ether.rring = rp;
	ether.rhead = rp;
	ether.rbase = xspanalloc(NRRING*BY2PG/2, BY2PG, 0);
	dcflush(ether.rbase, NRRING*BY2PG/2);
	ether.rbase = IO(uchar, ether.rbase);
	memset(ether.rbase, 0, NRRING*BY2PG/2);
	for(i=0; i<NRRING; i++, rp++){
		rp->base = ether.rbase + i*(BY2PG/2);
		rp->next = (Rdesc*)PADDR(rp+1);
	}
	--rp;
	rp->next = (Rdesc*)PADDR(ether.rring);

	/* allocate transmitter ring */
	xp = xspanalloc(NXRING*sizeof(Xdesc), BY2PG, 0);
	dcflush(xp, NXRING*sizeof(Xdesc));
	xp = IO(Xdesc, xp);
	memset(xp, 0, NXRING*sizeof(Xdesc));
	ether.tring = xp;
	ether.ttail = xp;
	ether.tbase = xspanalloc(NXRING*BY2PG/2, BY2PG, 0);
	dcflush(ether.tbase, NXRING*BY2PG/2);
	ether.tbase = IO(uchar, ether.tbase);
	for(i=0; i<NXRING; i++, xp++){
		xp->base = ether.tbase + i*(BY2PG/2);
		xp->next = (Rdesc*)PADDR(xp+1);
	}
	--xp;
	xp->next = (Rdesc*)PADDR(ether.tring);
}

void
seqreset(void)
{
	Hio *dev = DEV;
	int i;
	Xdesc *xp;
	Rdesc *rp;

	if(!ether.inited){
		/* physical ethernet address from nvram */
		getnveaddr(ether.ea);

		/*
		 * plan 9 network interface
		 */
		ether.net.name = "ether";
		ether.net.nconv = Ntypes;
		ether.net.devp = &seqinfo;
		ether.net.protop = 0;
		ether.net.listen = 0;
		ether.net.clone = seqclonecon;
		ether.net.ninfo = 2;
		ether.net.info[0].name = "stats";
		ether.net.info[0].fill = seqstatsfill;
		ether.net.info[1].name = "type";
		ether.net.info[1].fill = seqtypefill;
		for(i = 0; i < Ntypes; i++)
			netadd(&ether.net, &ether.e[i], i);
		memset(ether.bcast, 0xff, sizeof ether.bcast);

		initmem();
	}

	/* hardware reset */
	dev->rstat = 0;
	dev->xstat = 0;
	dev->ctl = Cnormal | Creset | Cint;
	delay(10);
	dev->ctl = Cnormal;
	dev->csx = 0;
	dev->csr = 0;

	dev->dmacfg |= HPC_FIX_INTR | HPC_FIX_EOP | HPC_FIX_RXDC;

	/* receiver */
	rp = ether.rhead;
	ether.rring = rp;
	for(i=0; i<NRRING; i++) {
		rp->addr = PADDR(rp->base);
		rp->count = Rxie|Empty|RBSIZE;
		rp = IO(Rdesc, rp->next);
	}
	rp->count |= Eor;
	dev->crbdp = PADDR(rp);
	dev->nrbdp = PADDR(rp->next);

	/* transmitter */
	xp = ether.ttail;
	for(i=0; i<NXRING; i++) {
		xp->count = Eox|Eoxp;
		xp->addr = 0;
		xp = IO(Xdesc, xp->next);
	}
	ether.tfree = NXRING-1;
	ether.thead = IO(Xdesc, ether.ttail->next);
	dev->nxbdp = PADDR(ether.thead);

	if(!ether.inited) {
		setvector(IVENET, seqintr, 0);
		ether.inited = 1;
	}
}

static void
seqstart(int prom)
{
	Hio *dev = DEV;
	int i;
	/*
	 *   wait till both receiver and transmitter are
	 *   quiescent
	 */
	qlock(&ether.tlock);
	qlock(&ether.rlock);

	seqreset();

	for(i=0; i<6; i++)
		dev->eaddr[i] = ether.ea[i];

	dev->csx = XIok | XImaxtry | XIcoll | XIunder;
	dev->csr = (prom ? Rprom : Rsb) | RIok|RIend|RIshort|RIdrbl|RIcrc;
	dev->rstat = Rdma;
	ether.wedged = 0;

	qunlock(&ether.rlock);
	qunlock(&ether.tlock);
}

void
seqinit(void)
{
}

Chan*
seqattach(char *spec)
{
	if(ether.kstarted == 0){
		ether.kstarted = -1;
		kproc("seqkproc", seqkproc, 0);
		seqstart(0);
		print("ether: %.2x%.2x%.2x%.2x%.2x%.2x\n",
			ether.ea[0], ether.ea[1], ether.ea[2],
			ether.ea[3], ether.ea[4], ether.ea[5]);
	}
	return devattach('l', spec);
}

Chan*
seqclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
seqwalk(Chan *c, char *name)
{
	return netwalk(c, name, &ether.net);
}

void	 
seqstat(Chan *c, char *dp)
{
	netstat(c, dp, &ether.net);
}

Chan*
seqopen(Chan *c, int omode)
{
	return netopen(c, omode, &ether.net);
}

void	 
seqcreate(Chan*, char*, int, ulong)
{
	error(Eperm);
}

void	 
seqclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long	 
seqread(Chan *c, void *a, long n, ulong offset)
{
	return netread(c, a, n, offset,  &ether.net);
}

long	 
seqwrite(Chan *c, void *a, long n, ulong)
{
	return streamwrite(c, a, n, 0);
}

void	 
seqremove(Chan*)
{
	error(Eperm);
}

void	 
seqwstat(Chan *c, char *dp)
{
	netwstat(c, dp, &ether.net);
}

/*
 *  user level network interface routines
 */
static void
seqstatsfill(Chan *c, char* p, int n)
{
	char buf[512];

	USED(c);
	sprint(buf, "in: %d\ninch: %d\nout: %d\noutch: %d\n"
		    "short: %d\nlshort: %d\ndribble: %d\n"
		    "crc: %d\nover: %d\nmaxtry: %d\nunder"
		    ": %d\naddr: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
		ether.inpackets, ether.inchars, ether.outpackets,
		ether.outchars, ether.shrt, ether.lshrt, ether.drbl,
		ether.crc, ether.over, ether.maxtry, ether.under,
		ether.ea[0], ether.ea[1], ether.ea[2], ether.ea[3],
		ether.ea[4], ether.ea[5]);
	strncpy(p, buf, n);
}

static void
seqtypefill(Chan *c, char* p, int n)
{
	char buf[16];
	Ethertype *e;

	e = &ether.e[STREAMID(c->qid.path)];
	sprint(buf, "%d", e->type);
	strncpy(p, buf, n);
}

static int
seqclonecon(Chan *c)
{
	Ethertype *e;

	USED(c);
	for(e = ether.e; e < &ether.e[Ntypes]; e++){
		qlock(e);
		if(e->inuse || e->q){
			qunlock(e);
			continue;
		}
		e->inuse = 1;
		netown(e, u->p->user, 0);
		qunlock(e);
		return e - ether.e;
	}
	error(Enodev);
	return -1;		/* never reached */
}

void
seqintr(Ureg*, void*)
{
	Hio *dev = DEV;
	int status;

	status = dev->ctl;
	DEBUG("%d int=0x%.2ux, x=0x%.2ux, r=0x%.4ux\n",
		m->ticks, status&0xff, (dev->xstat>>16)&0xff, dev->rstat&0xffff);
	if(status & Cint)
		dev->ctl = Cnormal | Cint; 
	if(ether.kstarted > 0){
		if(status & Cover){
			dev->ctl = Cnormal | Cover; 
			++ether.over;
		}
		if(status & Cint){
			status = dev->xstat;
			if(!(status & Xdma))
				xintr(status);
			if(ether.rr.p)
				wakeup(&ether.rr);
		}
	}
}

static void
xintr(int xstat)
{
	int freed;
	Hio *dev = DEV;
	Xdesc *xp, *xcur;

	DEBUG("%d xintr xstat=%ux\n", m->ticks, xstat);/**/
	if((xstat & (Xold|Xok)) == 0){
		dev->xstat = Xok;
		if(xstat & Xmaxtry)
			print("%d maxtry\n", m->ticks);
		if(xstat & Xunder)
			print("%d underflow\n", m->ticks);
		if(xstat & (Xmaxtry|Xunder))
			ether.wedged = 1;
	}

	xcur = IO(void, rdreg(&dev->nxbdp)&~0xf);
	if(xcur->count&Busy){
		dev->nxbdp = PADDR(xcur);
		dev->xstat = Xdma;
	}

	freed = 0;
	xp = ether.thead;
	while(xp != xcur) {
		if((xp->count&Busy) == 0)
			break;
		xp->count = Eox|Eoxp;
		xp->addr = 0;
		++freed;
		xp = IO(Xdesc, xp->next);
	}
	ether.tfree += freed;
	ether.thead = xp;
	if(freed != 0)
		wakeup(&ether.tr);
}

static int
iskpwork(void*)
{
	Hio *dev;
	Rdesc *rp, *prp;

	dev = DEV;
	if(!(dev->rstat & Rdma) || ether.wedged)
		return 1;

	prp = ether.rring;
	rp = IO(Rdesc, prp->next);
	if((rp->count&Empty) == 0)
		return 1;

	if(ether.self.first)
		return 1;

	if(ether.closeline)
		return 1;
	return 0;
}

static void
seqkproc(void*)
{
	Hio *dev;
	Block *bp;
	Etherpkt *p;
	Ethertype *e;
	Rdesc *rp, *prp;
	int len, status;

	dev = DEV;
	ether.kstarted = 1;
	while(waserror())
		print("seqkproc err %s\n", u->error);

	for(;;){
		status = dev->rstat;
		if(!(status & Rdma) || ether.wedged){
			DEBUG("%d rstat=%.4ux, wedged=%d: RESTART\n",
				m->ticks, status&0xffff, ether.wedged);
			seqstart(ether.prom);
			wakeup(&ether.tr);
			continue;
		}
		/*
		 *  receiver
		 */
		qlock(&ether.rlock);
		prp = ether.rring;
		rp = IO(Rdesc, prp->next);
		while((rp->count&Empty) == 0) {
			p = (Etherpkt*)(rp->base+2);
			len = RBSIZE-(rp->count&0x3fff)-3;
			if(len < 2)	/* ?? */
				status = Rlshort;
			else
				status = rp->base[len+2];
			if((status & Rok) == 0) {
				if(status & Rshort)
					++ether.shrt;
				else if(status & Rdrbl)
					++ether.drbl;
				else if(status & Rcrc)
					++ether.crc;
				else if(status & Rover)
					++ether.over;
			}
			else {
				++ether.inpackets;
				ether.inchars += len;
				DEBUG("%d rcv=0x%.ux %d\n", m->ticks, status, len-2);
				sequp(p, len);
			}
			rp->count = Rxie|Eor|Empty|RBSIZE;
	
			if((prp->count & Eor) == 0) {
				print("seq: lost Eor\n");
				seqstart(0);
				return;
			}
	
			prp->count &= ~Eor;
			prp = rp;
			rp = IO(Rdesc,rp->next);
		}
		ether.rring = prp;
		while(bp = getq(&ether.self)){
			sequp(bp->rptr, BLEN(bp));
			freeb(bp);
		}
		qunlock(&ether.rlock);
		/*
		 *  close ethertypes requesting it
		 */
		if(ether.closeline){
			lock(&ether.closepin);
			for(e = ether.closeline; e; e = e->closeline){
				e->q = 0;
				wakeup(&e->rc);
			}
			ether.closeline = 0;
			unlock(&ether.closepin);
		}
		tsleep(&ether.rr, iskpwork, 0, 1000);
	}
}

/*
 *  send a packet upstream
 */
static void
sequp(uchar *data, int len)
{
	int t;
	Block *bp;
	Ethertype *e;
	uchar hdr[16];
	Etherpkt *p;

	if(len <= 0)
		return;
	memmove(hdr, data, 16);
	p = (Etherpkt *)data;

	t = (p->type[0]<<8) | p->type[1];
	for(e = &ether.e[0]; e < &ether.e[Ntypes]; e++){
		/*
		 *  check for open, the right type, and flow control
		 */
		if(e->q==0)
			continue;
		if(t!=e->type && e->type!=-1)
			continue;
		if(e->q->next->len>Streamhi)
			continue;

		/*
		 *  only a trace channel gets packets destined for other machines
		 */
		if(e->type!=-1)
		if(p->d[0]!=0xff)
		if(*p->d != *ether.ea || memcmp(p->d, ether.ea, sizeof(p->d))!=0)
			continue;

		if(!waserror()){
			bp = allocb(len);
			memmove(bp->rptr, data, len);
			bp->wptr += len;
			bp->flags |= S_DELIM;
			PUTNEXT(e->q, bp);
			poperror();
		}
	}
}
