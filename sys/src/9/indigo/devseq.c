#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"devtab.h"

#define	DEBUG	if(0)kprint

typedef struct Ethertype	Ethertype;
typedef struct Hio	Hio;
typedef struct Hstate	Hstate;
typedef struct Rdesc	Rdesc;
typedef struct Xdesc	Xdesc;

enum
{
	Empty=	1<<31,		/* no data here */
	Eor=	1<<31		/* end of ring */
};

struct Rdesc
{
	ulong	count;		/* empty / count:13 */
	ulong	addr;		/* eor / addr:28 */
	Rdesc *	next;
	uchar *	base;
};

enum
{
	Eoxp=	1<<31,		/* end of transmission or packet */
	Busy=	1<<30,
	Xie=	1<<15		/* interrupt enable */
};

struct Xdesc
{
	ulong	count;		/* eop / busy / xie / count:13 */
	ulong	addr;		/* eox / addr:28 */
	Xdesc *	next;
	uchar *	base;
};

/*
 * SEEQ 8003 interfaced to HPC
 */
struct Hio
{
	ulong	unused00[4];
	ulong	nxbdp;		/* next xmit buffer desc. pointer */
	ulong	unused05[3];
	ulong	cxbdp;		/* current xmit buf desc ptr */
	ulong	cpfxbdp;	/* current packet 1st xmit buf desc. ptr */
	ulong	unused10[1];
	ulong	intdelay;	/* interrupt delay count */
	ulong	unused12[1];
	ulong	xstat;		/* transmitter status */
	ulong	rstat;		/* receiver status */
	ulong	ctl;		/* interrupt, channel reset, buf oflow */
	ulong	unused16[4];
	ulong	nrbdp;		/* next receive buf desc ptr */
	ulong	crbdp;		/* current receive buf desc ptr */
	ulong	unused22[42];
	ulong	eaddr[6];	/* seeq station address */
	ulong	csr;		/* seeq receiver cmd/status reg */
	ulong	csx;		/* seeq transmitter cmd/status reg */
};


enum
{			/* ctl */
	Cover=	0x08,		/* receive buffer overflow */
	Cnormal=0x04,		/* 1=normal, 0=loopback */
	Cint=	0x02,		/* interrupt (write 1 to clear) */
	Creset=	0x01,		/* ethernet channel reset */

			/* xstat */
	Xold=	0x800000,	/* register has been read */
	Xdma=	0x400000,	/* dma active */
	Xok=	0x080000,	/* transmission was successful */
	Xmaxtry=0x040000,	/* transmission failed after 16 attempts */
	Xcoll=	0x020000,	/* transmission collided */
	Xunder=	0x010000,	/* transmitter underflowed */

			/* csx */
	XIok=	0x08,
	XImaxtry=0x04,
	XIcoll=	0x02,
	XIunder=0x01,

			/* rstat */
	Rdma=	0x4000,		/* dma active */
	Rlshort=0x100,		/* [small len in received frame] */
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
	RIover=	0x01		/* interrupt on overflow error */
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
	Queue		*q;
	int		inuse;
	Rendez		rc;		/* rendzvous for close */
	Ethertype	*closeline;	/* close list */
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
	int	prom;		/* number of promiscuous channels */
	int	all;		/* number of channels listening to all packets */
	int	wedged;		/* the device is wedged */
	Network	net;

	Rendez	rr;		/* rendezvous for kproc */
	QLock	rlock;		/* semaphore on Rdesc chain */
	uchar *	rbase;
	Rdesc *	rring;

	Rendez	tr;		/* rendezvous for an output buffer */
	QLock	trlock;		/* semaphore on tr */
	QLock	tlock;		/* semaphore on Xdesc chain */
	int	tfree;		/* count of available Xdesc's */
	uchar *	tbase;
	Xdesc *	tring;
	Xdesc *	thead;		/* scavenge transmitted blocks from here */
	Xdesc *	ttail;		/* append blocks to be transmitted here */

	Ethertype *closeline;	/* channels waiting to close */
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

#define	DEV	IO(Hio, HPC_0_ID_ADDR)

static Hstate l;

/*
 *  predeclared
 */
static int	seqclonecon(Chan*);
static void	seqkproc(void *);
static void	seqstart(int);
static void	seqstatsfill(Chan*, char*, int);
static void	seqtypefill(Chan*, char*, int);
static void	sequp(void*, int, int);
static void	xintr(int);

/*
 *  seq stream module definition
 */
static void seqoput(Queue*, Block*);
static void seqstopen(Queue*, Stream*);
static void seqstclose(Queue*);
Qinfo seqinfo = { nullput, seqoput, seqstopen, seqstclose, "seq" };

/*
 *  open a seq line discipline
 */
static void
seqstopen(Queue *q, Stream *s)
{
	Ethertype *e;

	e = &l.e[s->id];
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
		qlock(&l);
		l.prom--;
		if(l.prom == 0)
			seqstart(0);
		qunlock(&l);
	}
	if(e->type == -1){
		qlock(&l);
		l.all--;
		qunlock(&l);
	}

	/*
	 *  mark as closing and wait for kproc to close us
	 */
	lock(&l.closepin);
	e->closeline = l.closeline;
	l.closeline = e;
	unlock(&l.closepin);
	wakeup(&l.rr);
	sleep(&e->rc, isclosed, e);
	
	e->type = 0;
	e->q = 0;
	e->prom = 0;
	e->inuse = 0;
	netdisown(e);
}

/*
 *  the ``connect'' control message specifyies the type
 */
static int
isobuf(void *x)
{
	return l.wedged || l.tfree > (long)x;
}

static void
seqoput(Queue *q, Block *bp)
{
	int len, k, n, s;
	uchar *dp;
	Etherpkt *p;
	Ethertype *e;
	Block *nbp;
	Xdesc *xp;
	Hio *dev = DEV;

	if(bp->type == M_CTL){
		e = q->ptr;
		qlock(&l);
		if(streamparse("connect", bp)){
			if(e->type == -1)
				l.all--;
			e->type = strtol((char *)bp->rptr, 0, 0);
			if(e->type == -1)
				l.all++;
		}else if(streamparse("promiscuous", bp)){
			e->prom = 1;
			if(++l.prom == 1)
				seqstart(1);
		}
		qunlock(&l);
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
	memmove(p->s, l.ea, sizeof(l.ea));
	len = blen(bp);
	if(len < ETHERMINTU){
		bp = expandb(bp, ETHERMINTU);
		len = ETHERMINTU;
	}else if(len > ETHERMAXTU){
		print("seqoput len=%d\n", len);
		freeb(bp);
	}
	if(*p->d == 0xff || l.prom || l.all){
		nbp = copyb(bp, len);
		if(nbp->next)
			nbp = expandb(nbp, len);
		if(nbp){
			nbp->wptr = nbp->rptr+len;
			putq(&l.self, nbp);
			wakeup(&l.rr);
		}
	}else if(*p->d == *l.ea && memcmp(l.ea, p->d, sizeof(l.ea)) == 0){
		if(bp->next)
			bp = expandb(bp, len);
		if(bp){
			putq(&l.self, bp);
			wakeup(&l.rr);
		}
		return;
	}
	if(l.wedged){
		freeb(bp);
		return;
	}

	/*
	 *  only one transmitter at a time
	 */
	qlock(&l.trlock);
	qlock(&l.tlock);
	DEBUG("%d tfree=%d, len=%d\n", m->ticks, l.tfree, len);
	while(l.tfree == 0){
		qunlock(&l.tlock);
		sleep(&l.tr, isobuf, 0);
		qlock(&l.tlock);
	}
	if(l.wedged){
		qunlock(&l.tlock);
		qlock(&l.trlock);
		freeb(bp);
		return;
	}
	++l.outpackets;
	--l.tfree;
	l.outchars += len;
	xp = l.ttail->next;
	dp = xp->base;
	for(nbp=bp; nbp; nbp=nbp->next){
		n = BLEN(nbp);
		if(n == 0)
			continue;
		memmove(dp, nbp->rptr, n);
		dp += n;
	}
	xp->addr = Eoxp | PADDR(xp->base);	/* end of transmission */
	xp->count = Eoxp|Busy|Xie | len;	/* end of packet */
	s = splhi();
	k = dev->xstat;
	if(k & Xdma){
		DEBUG("\tappend\n");
		l.ttail->addr &= ~Eoxp;
		l.ttail->count &= ~Xie;
	}else
		xintr(k);
	splx(s);
	l.ttail = xp;

	qunlock(&l.tlock);
	qunlock(&l.trlock);
	freeb(bp);
}

void
seqreset(void)
{
	Hio *dev = DEV;
	int i;
	Xdesc *xp;
	Rdesc *rp;
	Block *bp;

	if(!l.inited){
		/* physical ethernet address from nvram */
		getnveaddr(l.ea);
		/* plan 9 network interface */
		l.net.name = "ether";
		l.net.nconv = Ntypes;
		l.net.devp = &seqinfo;
		l.net.protop = 0;
		l.net.listen = 0;
		l.net.clone = seqclonecon;
		l.net.ninfo = 2;
		l.net.info[0].name = "stats";
		l.net.info[0].fill = seqstatsfill;
		l.net.info[1].name = "type";
		l.net.info[1].fill = seqtypefill;
		for(i = 0; i < Ntypes; i++)
			netadd(&l.net, &l.e[i], i);
		memset(l.bcast, 0xff, sizeof l.bcast);

		/* allocate receiver ring */
		l.rring = rp = KADDR1(PADDR(xspanalloc(NRRING*sizeof(Rdesc), BY2PG, 128*1024*1024)));
		l.rbase = KADDR1(PADDR(xspanalloc(NRRING*BY2PG/2, BY2PG, 128*1024*1024)));
		for(i=0; i<NRRING; i++, rp++){
			rp->base = l.rbase + i*(BY2PG/2);
			rp->next = rp+1;
		}
		--rp;
		rp->next = l.rring;

		/* allocate transmitter ring */
		l.tring = xspanalloc(NXRING*sizeof(Xdesc), BY2PG, 128*1024*1024);
		l.ttail = xp = l.tring;
		l.tbase = xspanalloc(NXRING*BY2PG/2, BY2PG, 128*1024*1024);
		for(i=0; i<NXRING; i++, xp++){
			xp->base = l.tbase + i*(BY2PG/2);
			xp->next = xp+1;
		}
		--xp;
		xp->next = l.ttail;
	}

	/* hardware reset */
	dev->rstat = 0;
	dev->xstat = 0;
	dev->ctl = Cnormal | Creset | Cint;
	Xdelay(10);
	dev->ctl = Cnormal;
	dev->csx = 0;
	dev->csr = 0;
	dev->intdelay = 1 << 24;

	/* receiver */
	rp = l.rring;
	for(i=0; i<NRRING; i++, rp=rp->next){
		rp->addr = PADDR(rp->base);
		rp->count = Empty|RBSIZE;
	}
	rp->addr |= Eor;
	dev->crbdp = PADDR(rp);
	dev->nrbdp = PADDR(rp->next);
	while(bp = getq(&l.self))
		freeb(bp);

	/* transmitter */
	xp = l.ttail;
	for(i=0; i<NXRING; i++, xp=xp->next){
		xp->count = 0;
		xp->addr = Eoxp;
	}
	l.tfree = NXRING-1;
	l.thead = l.ttail->next;
	dev->cxbdp = PADDR(l.ttail);
	dev->cpfxbdp = PADDR(l.ttail);
	dev->nxbdp = PADDR(l.thead);

	if(!l.inited){
		*IO(uchar, LIO_0_MASK) |= LIO_ENET;
		l.inited = 1;
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
	qlock(&l.tlock);
	qlock(&l.rlock);

	seqreset();

	for(i=0; i<6; i++)
		dev->eaddr[i] = l.ea[i];

	dev->csx = XIok | XImaxtry | XIcoll | XIunder;
	dev->csr = (prom ? Rprom : Rsb) | RIok|RIend|RIshort|RIdrbl|RIcrc;
	dev->rstat = Rdma;
	l.wedged = 0;

	qunlock(&l.rlock);
	qunlock(&l.tlock);
}

void
seqinit(void)
{
}

Chan*
seqattach(char *spec)
{
	if(l.kstarted == 0){
		l.kstarted = -1;
		kproc("seqkproc", seqkproc, 0);
		seqstart(0);
		print("seq ether: %.2x %.2x %.2x %.2x %.2x %.2x\n",
			l.ea[0], l.ea[1], l.ea[2], l.ea[3], l.ea[4], l.ea[5]);
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
	return netwalk(c, name, &l.net);
}

void	 
seqstat(Chan *c, char *dp)
{
	netstat(c, dp, &l.net);
}

Chan*
seqopen(Chan *c, int omode)
{
	return netopen(c, omode, &l.net);
}

void	 
seqcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
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
	return netread(c, a, n, offset,  &l.net);
}

long	 
seqwrite(Chan *c, void *a, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, a, n, 0);
}

void	 
seqremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
seqwstat(Chan *c, char *dp)
{
	netwstat(c, dp, &l.net);
}

/*
 *  user level network interface routines
 */
static void
seqstatsfill(Chan *c, char* p, int n)
{
	char buf[512];

	USED(c);
	sprint(buf, "in: %d\ninch: %d\nout: %d\noutch: %d\nshort: %d\nlshort: %d\ndribble: %d\ncrc: %d\nover: %d\nmaxtry: %d\nunder: %d\naddr: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
		l.inpackets, l.inchars, l.outpackets, l.outchars, l.shrt, l.lshrt,
		l.drbl, l.crc, l.over, l.maxtry, l.under,
		l.ea[0], l.ea[1], l.ea[2], l.ea[3], l.ea[4], l.ea[5]);
	strncpy(p, buf, n);
}

static void
seqtypefill(Chan *c, char* p, int n)
{
	char buf[16];
	Ethertype *e;

	e = &l.e[STREAMID(c->qid.path)];
	sprint(buf, "%d", e->type);
	strncpy(p, buf, n);
}

static int
seqclonecon(Chan *c)
{
	Ethertype *e;

	USED(c);
	for(e = l.e; e < &l.e[Ntypes]; e++){
		qlock(e);
		if(e->inuse || e->q){
			qunlock(e);
			continue;
		}
		e->inuse = 1;
		netown(e, u->p->user, 0);
		qunlock(e);
		return e - l.e;
	}
	error(Enodev);
	return -1;		/* never reached */
}

static void
ckxdesc(char *title, Xdesc *xp, int pflag)
{
	int (*pfn)(char*, ...);
	Hio *dev;
	Xdesc *nx;
	int i;

	if(xp>=l.tring && xp < &l.tring[NXRING] && (((uchar *)xp)-((uchar *)l.tring))%sizeof(Xdesc) == 0)
		return;
	pfn = pflag ? print : kprint;
	nx = l.tring;
	for(i=0; i<NXRING-1; i++, nx++){
		if(nx->next != nx+1)
			(*pfn)("%d %ux %ux\n", i, nx, nx->next);
	}
	if(nx->next != l.tring)
		(*pfn)("%d %ux %ux\n", i, nx, nx->next);
	dev = DEV;
	if(pflag)
		pfn = (int (*)(char*, ...))panic;
	(*pfn)("%d ckxdesc %s xstat=%.2ux xp=%ux tb=%ux ptrs= %ux %ux %ux\n",
		m->ticks, title, (dev->xstat>>16)&0xff, xp, l.tring,
		dev->nxbdp, dev->cpfxbdp, dev->cxbdp);
}

void
seqintr(void)
{
	Hio *dev = DEV;
	int status;

	status = dev->ctl;
	DEBUG("%d int=0x%.2ux, x=0x%.2ux, r=0x%.4ux\n",
		m->ticks, status&0xff, (dev->xstat>>16)&0xff, dev->rstat&0xffff);
	if(status & Cint)
		dev->ctl = Cnormal | Cint; 
	if(l.kstarted > 0){
		if(status & Cover){
			dev->ctl = Cnormal | Cover; 
			++l.over;
		}
		if(status & Cint){
			status = dev->xstat;
			if(!(status & Xdma))
				xintr(status);
			if(l.rr.p)
				wakeup(&l.rr);
		}
	}
}

static void
xintr(int xstat)
{
	Xdesc *xp, *xcur;
	Hio *dev = DEV;

	DEBUG("%d xintr xstat=%.2ux\n", m->ticks, (xstat>>16)&0xff);
	if(!(xstat & (Xold|Xok))){
		dev->xstat = Xok;
		if(xstat & Xmaxtry){
			kprint("%d maxtry\n", m->ticks);
			++l.maxtry;
		}
		if(xstat & Xunder){
			kprint("%d underflow\n", m->ticks);
			++l.under;
		}
		if(xstat & (Xmaxtry|Xunder)){
			l.wedged = 1;
			return;
		}
	}

	xp = xcur = KADDR(dev->cxbdp);
	ckxdesc("xintr cxbdp", xp, 1);
	if((xp->count&Busy) && !(xp->count&Eoxp)){
		kprint("%d xdrop\n", m->ticks);
		while(!(xp->count&Eoxp)){
			xp = xp->next;
			if(xp == xcur)
				panic("xintr");
		}
		xp = xp->next;
		dev->nxbdp = PADDR(xp);
	}else{
		xp = KADDR(dev->nxbdp);
		ckxdesc("xintr nxbdp", xp, 1);
	}

	if(xp->count&Busy){
		DEBUG("\tXdma\n");
		dev->cxbdp = PADDR(xp);
		dev->cpfxbdp = PADDR(xp);
		dev->xstat = Xdma;
	}
}

static int
iskpwork(void *arg)
{
	Hio *dev = DEV;

	USED(arg);
	if(!(dev->rstat & Rdma) || l.wedged)
		return 1;
	if(!(l.rring->next->count&Empty))
		return 1;
	if(l.self.first)
		return 1;
	if(l.thead != KADDR(dev->cpfxbdp) && (l.thead->count&Busy))
		return 1;
	if(l.closeline)
		return 1;
	return 0;
}

static void
seqkproc(void *arg)
{
	Xdesc *xp, *xcur;
	Rdesc *rp, *prp;
	Block *bp;
	int a, b, len, rem, status;
	uchar *p;
	Hio *dev = DEV;
	int freed;
	Ethertype *e;

	USED(arg);
	l.kstarted = 1;
	while(waserror())
		print("seqkproc err %s\n", u->error);
	for(;;){
		status = dev->rstat;
		if(!(status & Rdma) || l.wedged){
			DEBUG("%d rstat=%.4ux, wedged=%d: RESTART\n",
				m->ticks, status&0xffff, l.wedged);
			seqstart(l.prom);
			wakeup(&l.tr);
			continue;
		}
		/*
		 *  receiver
		 */
		qlock(&l.rlock);
		prp=l.rring;
		for(rp=prp->next; !(rp->count&Empty); prp=rp, rp=rp->next){
			/*
			 * ``The DDM memory controller chip has a bug which
			 * allows the remaining byte count to occasionally
			 * be written into the bufptr field.  This is a
			 * heuristic fix.''
			 */
			rem = rp->count;
			p = rp->base;
			a = (ulong)(rp->base) & 0xffff;
			b = rp->addr & 0xffff;
			if(a != b){
				if((a & 0xff00) != (b & 0xff00))
					rem = b & 0x1fff;
				else
					rem = (rem & 0xff00) | (b & 0xff);
			}
			/* begins with 2 bytes of pad; ends with status */
			len = RBSIZE - rem - 1;
			if(len < 2)	/* ?? */
				status = Rlshort;
			else
				status = p[len];
			if(!(status & Rok)){
				if(status & Rlshort)
					++l.lshrt;
				else if(status & Rshort)
					++l.shrt;
				else if(status & Rdrbl)
					++l.drbl;
				else if(status & Rcrc)
					++l.crc;
				else if(status & Rover)
					++l.over;
			}else{
				++l.inpackets;
				l.inchars += len-2;
				DEBUG("%d rcv=0x%.ux %d\n",
					m->ticks, status, len-2);
				sequp(p, len, 2);
			}
			rp->addr = Eor | PADDR(p);
			rp->count = Empty|RBSIZE;
			prp->addr &= ~Eor;
		}
		l.rring=prp;
		while(bp = getq(&l.self)){
			sequp(bp->rptr, BLEN(bp), 0);
			freeb(bp);
		}
		qunlock(&l.rlock);
		/*
		 *  transmitter
		 */
		qlock(&l.tlock);
		freed = 0;
		xcur = KADDR(dev->cpfxbdp);
		ckxdesc("kproc cpfxbdp", xcur, 1);
		for(xp=l.thead; xp!=xcur; xp=xp->next){
			if(!(xp->count&Busy))
				break;
			xp->count = 0;
			xp->addr = Eoxp;
			++freed;
		}
		l.tfree += freed;
		l.thead = xp;
		qunlock(&l.tlock);
		if(freed){
			DEBUG("%d xfree %d\n", m->ticks, freed);
			wakeup(&l.tr);
		}
		/*
		 *  close ethertypes requesting it
		 */
		if(l.closeline){
			lock(&l.closepin);
			for(e = l.closeline; e; e = e->closeline){
				e->q = 0;
				wakeup(&e->rc);
			}
			l.closeline = 0;
			unlock(&l.closepin);
		}
		tsleep(&l.rr, iskpwork, 0, 1000);
	}
}

/*
 *  send a packet upstream
 */
static void
sequp(uchar *data, int len, int pad)
{
	int t;
	Block *bp;
	Ethertype *e;
	uchar hdr[16];
	Etherpkt *p;

	if(len <= 0)
		return;
	memmove(hdr, data, 16);
	p = (Etherpkt *)&data[pad];

	t = (p->type[0]<<8) | p->type[1];
	for(e = &l.e[0]; e < &l.e[Ntypes]; e++){
		/*
		 *  check for open, the right type, and flow control
		 */
		if(e->q==0 || (t!=e->type && e->type!=-1) || e->q->next->len>Streamhi)
			continue;

		/*
		 *  only a trace channel gets packets destined for other machines
		 */
		if(e->type!=-1 && p->d[0]!=0xff
		&& (*p->d != *l.ea || memcmp(p->d, l.ea, sizeof(p->d))!=0))
			continue;

		if(!waserror()){
			bp = allocb(len);
			memmove(bp->rptr, data, len);
			bp->rptr += pad;
			bp->wptr += len;
			bp->flags |= S_DELIM;
			PUTNEXT(e->q, bp);
			poperror();
		}
	}
}

static void
showqlock(char *title, QLock *q)
{
	Proc *p;

	print("%s %d:", title, q->locked);
	for(p=q->head; p; p=p->qnext)
		print(" %d %s", p->pid, p->text);
	print("\n");
}

void
consdebug(void)
{
	Hio *dev = DEV;

	print("seqdebug tfree=%d rstat=%.2ux xstat=%.2ux tb=%ux n=%ux f=%ux c=%ux\n",
		l.tfree, dev->rstat&0xffff, (dev->xstat>>16)&0xff, l.tring,
		dev->nxbdp, dev->cpfxbdp, dev->cxbdp);
	showqlock("rlock", &l.rlock);
	showqlock("trlock", &l.trlock);
	showqlock("tlock", &l.tlock);
}
