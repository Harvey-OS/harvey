#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

typedef struct Hw Hw;
typedef struct Buffer Buffer;
typedef struct Type Type;
typedef struct Ctlr Ctlr;

struct Hw {
	int	(*reset)(Ctlr*);
	void	(*init)(Ctlr*);
	void	(*mode)(Ctlr*, int);
	void	(*online)(Ctlr*, int);
	void	(*receive)(Ctlr*);
	void	(*transmit)(Ctlr*);
	void	(*intr)(Ureg*);
	void	(*tweek)(Ctlr*);
	int	addr;			/* interface address */
	uchar	*ram;			/* interface shared memory address */
	int	bt16;			/* true if a 16 bit interface */
	int	lvl;			/* interrupt level */
	int	size;
	uchar	tstart;
	uchar	pstart;
	uchar	pstop;
};
static Hw wd8013;

enum {
	Nctlr		= 1,		/* even one of these is too many */
	NType		= 9,		/* types/interface */

	Nrb		= 16,		/* software receive buffers */
	Ntb		= 4,		/* software transmit buffers */
};

#define NEXT(x, l)	(((x)+1)%(l))
#define OFFSETOF(t, m)	((unsigned)&(((t*)0)->m))
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

struct Buffer {
	uchar	owner;
	uchar	busy;
	ushort	len;
	uchar	pkt[sizeof(Etherpkt)];
};

enum {
	Host		= 0,		/* buffer owned by host */
	Interface	= 1,		/* buffer owned by interface */
};

/*
 * one per ethernet packet type
 */
struct Type {
	QLock;
	Netprot;			/* stat info */
	int	type;			/* ethernet type */
	int	prom;			/* promiscuous mode */
	Queue	*q;
	int	inuse;
	Ctlr	*ctlr;
};

/*
 * per ethernet
 */
struct Ctlr {
	QLock;

	Hw	*hw;
	int	present;

	ushort	nrb;		/* number of software receive buffers */
	ushort	ntb;		/* number of software transmit buffers */
	Buffer	*rb;		/* software receive buffers */
	Buffer	*tb;		/* software transmit buffers */

	uchar	ea[6];		/* ethernet address */
	uchar	ba[6];		/* broadcast address */

	Rendez	rr;		/* rendezvous for a receive buffer */
	ushort	rh;		/* first receive buffer belonging to host */
	ushort	ri;		/* first receive buffer belonging to interface */	

	Rendez	tr;		/* rendezvous for a transmit buffer */
	QLock	tlock;		/* semaphore on th */
	ushort	th;		/* first transmit buffer belonging to host */	
	ushort	ti;		/* first transmit buffer belonging to interface */	

	Type	type[NType];
	uchar	prom;		/* true if promiscuous mode */
	uchar	kproc;		/* true if kproc started */
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
};
static Ctlr ctlr[Nctlr];

Chan*
etherattach(char *spec)
{
	if(ctlr[0].present == 0)
		error(Enodev);
	return devattach('l', spec);
}

Chan*
etherclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
etherwalk(Chan *c, char *name)
{
	return netwalk(c, name, &ctlr[0].net);
}

void
etherstat(Chan *c, char *dp)
{
	netstat(c, dp, &ctlr[0].net);
}

Chan*
etheropen(Chan *c, int omode)
{
	return netopen(c, omode, &ctlr[0].net);
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
	return netread(c, a, n, offset, &ctlr[0].net);
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
	netwstat(c, dp, &ctlr[0].net);
}

static int
isobuf(void *arg)
{
	Ctlr *cp = arg;

	return cp->tb[cp->th].owner == Host;
}

static void
etheroput(Queue *q, Block *bp)
{
	Ctlr *cp;
	int len, n;
	Type *tp;
	Etherpkt *p;
	Buffer *tb;
	Block *nbp;

	if(bp->type == M_CTL){
		tp = q->ptr;
		if(streamparse("connect", bp))
			tp->type = strtol((char *)bp->rptr, 0, 0);
		else if(streamparse("promiscuous", bp)) {
			tp->prom = 1;
			(*tp->ctlr->hw->mode)(tp->ctlr, 1);
		}
		freeb(bp);
		return;
	}

	cp = ((Type *)q->ptr)->ctlr;

	/*
	 * give packet a local address, return upstream if destined for
	 * this machine.
	 */
	if(BLEN(bp) < ETHERHDRSIZE && (bp = pullup(bp, ETHERHDRSIZE)) == 0)
		return;
	p = (Etherpkt *)bp->rptr;
	memmove(p->s, cp->ea, sizeof(cp->ea));
	if(memcmp(cp->ea, p->d, sizeof(cp->ea)) == 0){
		len = blen(bp);
		if (bp = expandb(bp, len >= ETHERMINTU ? len: ETHERMINTU)){
			putq(&cp->lbq, bp);
			wakeup(&cp->rr);
		}
		return;
	}
	if(memcmp(cp->ba, p->d, sizeof(cp->ba)) == 0){
		len = blen(bp);
		nbp = copyb(bp, len);
		if(nbp = expandb(nbp, len >= ETHERMINTU ? len: ETHERMINTU)){
			nbp->wptr = nbp->rptr+len;
			putq(&cp->lbq, nbp);
			wakeup(&cp->rr);
		}
	}

	/*
	 * only one transmitter at a time
	 */
	qlock(&cp->tlock);
	if(waserror()){
		freeb(bp);
		qunlock(&cp->tlock);
		nexterror();
	}

	/*
	 * wait till we get an output buffer.
	 * should try to restart.
	 */
	sleep(&cp->tr, isobuf, cp);

	tb = &cp->tb[cp->th];

	/*
	 * copy message into buffer
	 */
	len = 0;
	for(nbp = bp; nbp; nbp = nbp->next){
		if(sizeof(Etherpkt) - len >= (n = BLEN(nbp))){
			memmove(tb->pkt+len, nbp->rptr, n);
			len += n;
		} else
			print("no room damn it\n");
		if(bp->flags & S_DELIM)
			break;
	}

	/*
	 * pad the packet (zero the pad)
	 */
	if(len < ETHERMINTU){
		memset(tb->pkt+len, 0, ETHERMINTU-len);
		len = ETHERMINTU;
	}

	/*
	 * set up the transmit buffer and 
	 * start the transmission
	 */
	cp->outpackets++;
	tb->len = len;
	tb->owner = Interface;
	cp->th = NEXT(cp->th, cp->ntb);
	(*cp->hw->transmit)(cp);

	freeb(bp);
	qunlock(&cp->tlock);
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
	Ctlr *cp = &ctlr[0];
	Type *tp;

	tp = &cp->type[s->id];
	qlock(tp);
	RD(q)->ptr = WR(q)->ptr = tp;
	tp->type = 0;
	tp->q = RD(q);
	tp->inuse = 1;
	tp->ctlr = cp;
	qunlock(tp);
}

/*
 *  close ether line discipline
 *
 *  the lock is to synchronize changing the ethertype with
 *  sending packets up the stream on interrupts.
 */
static void
etherstclose(Queue *q)
{
	Type *tp;

	tp = (Type *)(q->ptr);
	if(tp->prom)
		(*tp->ctlr->hw->mode)(tp->ctlr, 0);
	qlock(tp);
	tp->type = 0;
	tp->q = 0;
	tp->prom = 0;
	tp->inuse = 0;
	netdisown(tp);
	tp->ctlr = 0;
	qunlock(tp);
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
	Ctlr *cp = &ctlr[0];
	Type *tp;

	USED(c);
	for(tp = cp->type; tp < &cp->type[NType]; tp++){
		qlock(tp);
		if(tp->inuse || tp->q){
			qunlock(tp);
			continue;
		}
		tp->inuse = 1;
		netown(tp, u->p->user, 0);
		qunlock(tp);
		return tp - cp->type;
	}
	exhausted("ether channels");
	return 0;
}

static void
statsfill(Chan *c, char *p, int n)
{
	Ctlr *cp = &ctlr[0];
	char buf[256];

	USED(c);
	sprint(buf, "in: %d\nout: %d\ncrc errs %d\noverflows: %d\nframe errs %d\nbuff errs: %d\noerrs %d\naddr: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
		cp->inpackets, cp->outpackets, cp->crcs,
		cp->overflows, cp->frames, cp->buffs, cp->oerrs,
		cp->ea[0], cp->ea[1], cp->ea[2], cp->ea[3], cp->ea[4], cp->ea[5]);
	strncpy(p, buf, n);
}

static void
typefill(Chan *c, char *p, int n)
{
	char buf[16];
	Type *tp;

	tp = &ctlr[0].type[STREAMID(c->qid.path)];
	sprint(buf, "%d", tp->type);
	strncpy(p, buf, n);
}

static void
etherup(Ctlr *cp, void *data, int len)
{
	Etherpkt *p;
	int t;
	Type *tp;
	Block *bp;

	p = data;
	t = (p->type[0]<<8)|p->type[1];
	for(tp = &cp->type[0]; tp < &cp->type[NType]; tp++){
		/*
		 *  check before locking just to save a lock
		 */
		if(tp->q == 0 || (t != tp->type && tp->type != -1))
			continue;

		/*
		 *  only a trace channel gets packets destined for other machines
		 */
		if(tp->type != -1 && p->d[0] != 0xFF && memcmp(p->d, cp->ea, sizeof(p->d)))
			continue;

		/*
		 *  check after locking to make sure things didn't
		 *  change under foot
		 */
		if(canqlock(tp) == 0)
			continue;
		if(tp->q == 0 || tp->q->next->len > Streamhi || (t != tp->type && tp->type != -1)){
			qunlock(tp);
			continue;
		}
		if(waserror() == 0){
			bp = allocb(len);
			memmove(bp->rptr, p, len);
			bp->wptr += len;
			bp->flags |= S_DELIM;
			PUTNEXT(tp->q, bp);
		}
		poperror();
		qunlock(tp);
	}
}

static int
isinput(void *arg)
{
	Ctlr *cp = arg;

	return cp->lbq.first || cp->rb[cp->ri].owner == Host;
}

static void
etherkproc(void *arg)
{
	Ctlr *cp = arg;
	Buffer *rb;
	Block *bp;

	if(waserror()){
		print("%s noted\n", cp->name);
		(*cp->hw->init)(cp);
		cp->kproc = 0;
		nexterror();
	}
	cp->kproc = 1;
	for(;;){
		tsleep(&cp->rr, isinput, cp, 500);
		(*cp->hw->tweek)(cp);

		/*
		 * process any internal loopback packets
		 */
		while(bp = getq(&cp->lbq)){
			cp->inpackets++;
			etherup(cp, bp->rptr, BLEN(bp));
			freeb(bp);
		}

		/*
		 * process any received packets
		 */
		while(cp->rb[cp->rh].owner == Host){
			cp->inpackets++;
			rb = &cp->rb[cp->rh];
			etherup(cp, rb->pkt, rb->len);
			rb->owner = Interface;
			cp->rh = NEXT(cp->rh, cp->nrb);
		}
	}
}

void
etherreset(void)
{
	int i;
	Ctlr *cp;

	cp = &ctlr[0];
	cp->hw = &wd8013;
	if((*cp->hw->reset)(cp) < 0){
		cp->present = 0;
		return;
	}
	cp->present = 1;

	memset(cp->ba, 0xFF, sizeof(cp->ba));

	cp->net.name = "ether";
	cp->net.nconv = NType;
	cp->net.devp = &info;
	cp->net.protop = 0;
	cp->net.listen = 0;
	cp->net.clone = clonecon;
	cp->net.ninfo = 2;
	cp->net.info[0].name = "stats";
	cp->net.info[0].fill = statsfill;
	cp->net.info[1].name = "type";
	cp->net.info[1].fill = typefill;
	for(i = 0; i < NType; i++)
		netadd(&cp->net, &cp->type[i], i);
}

void
etherinit(void)
{
	int ctlrno = 0;
	Ctlr *cp = &ctlr[ctlrno];
	int i;

	if(cp->present == 0)
		return;

	cp->rh = 0;
	cp->ri = 0;
	for(i = 0; i < cp->nrb; i++)
		cp->rb[i].owner = Interface;

	cp->th = 0;
	cp->ti = 0;
	for(i = 0; i < cp->ntb; i++)
		cp->tb[i].owner = Host;

	/*
	 * put the receiver online
	 * and start the kproc
	 */	
	(*cp->hw->online)(cp, 1);
	if(cp->kproc == 0){
		sprint(cp->name, "ether%dkproc", ctlrno);
		kproc(cp->name, etherkproc, cp);
	}
}

typedef struct {
	uchar	msr;			/* 83C584 bus interface */
	uchar	icr;
	uchar	iar;
	uchar	bio;
	uchar	irr;
	uchar	laar;
	uchar	ijr;
	uchar	gp2;
	uchar	lan[6];
	uchar	id;
	uchar	cksum;

	union {				/* DP8390/83C690 LAN controller */
		struct {			/* Page0, read */
			uchar	cr;
			uchar	clda0;
			uchar	clda1;
			uchar	bnry;
			uchar	tsr;
			uchar	ncr;
			uchar	fifo;
			uchar	isr;
			uchar	crda0;
			uchar	crda1;
			uchar	pad0x0A;
			uchar	pad0x0B;
			uchar	rsr;
			uchar	cntr0;
			uchar	cntr1;
			uchar	cntr2;
		} r;
		struct {			/* Page0, write */
			uchar	cr;
			uchar	pstart;
			uchar	pstop;
			uchar	bnry;
			uchar	tpsr;
			uchar	tbcr0;
			uchar	tbcr1;
			uchar	isr;
			uchar	rsar0;
			uchar	rsar1;
			uchar	rbcr0;
			uchar	rbcr1;
			uchar	rcr;
			uchar	tcr;
			uchar	dcr;
			uchar	imr;
		} w;
		struct {			/* Page1, read/write */
			uchar	cr;
			uchar	par[6];
			uchar	curr;
			uchar	mar[8];
		};
		struct {			/* Page2, read */
			uchar	cr;
			uchar	pstart;
			uchar	pstop;
			uchar	dummy1[1];
			uchar	tstart;
			uchar	next;
			uchar	block;
			uchar	enh;
			uchar	dummy2[4];
			uchar	rcon;
			uchar	tcon;
			uchar	dcon;
			uchar	intmask;
		} r2;
		struct {			/* Page2, write */
			uchar	cr;
			uchar	trincrl;
			uchar	trincrh;
			uchar	dummy1[2];
			uchar	next;
			uchar	block;
			uchar	enh;
		} w2;
	};
} Wd8013;

enum {
	MENB		= 0x40,			/* memory enable */

	/* bit definitions for laar */
	ZeroWS16	= (1<<5),		/* zero wait states for 16-bit ops */
	L16EN		= (1<<6),		/* enable 16-bit LAN operation */
	M16EN		= (1<<7),		/* enable 16-bit memory access */

	/* bit defintitions for DP8390/83C690 cr */
	Page0		= (0<<6),
	Page1		= (1<<6),
	Page2		= (2<<6),
	RD2		= (4<<3),
	TXP		= (1<<2),
	STA		= (1<<1),
	STP		= (1<<0),
};

#define IN(hw, m)	inb((hw)->addr+OFFSETOF(Wd8013, m))
#define OUT(hw, m, x)	outb((hw)->addr+OFFSETOF(Wd8013, m), (x))

typedef struct {
	uchar	status;
	uchar	next;
	uchar	len0;
	uchar	len1;
	uchar	data[256-4];
} Ring;

static void
wd8013dumpregs(Hw *hw)
{
	print("msr=#%2.2ux\n", IN(hw, msr));
	print("icr=#%2.2ux\n", IN(hw, icr));
	print("iar=#%2.2ux\n", IN(hw, iar));
	print("bio=#%2.2ux\n", IN(hw, bio));
	print("irr=#%2.2ux\n", IN(hw, irr));
	print("laar=#%2.2ux\n", IN(hw, laar));
	print("ijr=#%2.2ux\n", IN(hw, ijr));
	print("gp2=#%2.2ux\n", IN(hw, gp2));
	print("lan0=#%2.2ux\n", IN(hw, lan[0]));
	print("lan1=#%2.2ux\n", IN(hw, lan[1]));
	print("lan2=#%2.2ux\n", IN(hw, lan[2]));
	print("lan3=#%2.2ux\n", IN(hw, lan[3]));
	print("lan4=#%2.2ux\n", IN(hw, lan[4]));
	print("lan5=#%2.2ux\n", IN(hw, lan[5]));
	print("id=#%2.2ux\n", IN(hw, id));
}

/* mapping from configuration bits to interrupt level */
int intrmap[] =
{
	9, 3, 5, 7, 10, 11, 15, 4,
};

/*
 *  get configuration parameters, enable memory
 */
static int
wd8013reset(Ctlr *cp)
{
	Hw *hw = cp->hw;
	int i;
	uchar msr, icr, laar, irr;
	ulong ram;

	/* find the ineterface */
	SET(msr, icr, irr, laar);
	for(hw->addr = 0x200; hw->addr < 0x400; hw->addr += 0x20){
		msr = IN(hw, msr);
		icr = IN(hw, icr);
		irr = IN(hw, irr);
		laar = IN(hw, laar);
		if((msr&0x80) || (icr&0xf0) || irr == 0xff || laar == 0xff)
			continue;	/* nothing there */

		/* ethernet address */
		for(i = 0; i < sizeof(cp->ea); i++)
			cp->ea[i] = IN(hw, lan[i]);

		/* look for an elite ether address */
		if(cp->ea[0] == 0x00 && cp->ea[1] == 0x00 && cp->ea[2] == 0xC0)
			break;
	}
	if(hw->addr >= 0x400)
		return -1;

	if(cp->rb == 0){
		cp->rb = xspanalloc(sizeof(Buffer)*Nrb, BY2PG, 0);
		cp->nrb = Nrb;
		cp->tb = xspanalloc(sizeof(Buffer)*Ntb, BY2PG, 0);
		cp->ntb = Ntb;
	}

	/* 16 bit operation? */
	hw->bt16 = icr & 0x1;

	/* address of interface RAM */
	ram = KZERO | ((msr & 0x3f) << 13);
	if(hw->bt16)
		ram |= (laar & 0x3f)<<19;
	else
		ram |= 0x80000;
	hw->ram = (uchar*)ram;

	/* interrupt level */
	hw->lvl = intrmap[((irr>>5) & 0x3) | (icr & 0x4)];

	/* ram size */
	if(icr&(1<<3))
		hw->size = 32*1024;
	else
		hw->size = 8*1024;
	if(hw->bt16)
		hw->size <<= 1;
	hw->pstart = HOWMANY(sizeof(Etherpkt), 256);
	hw->pstop = HOWMANY(hw->size, 256);

print("elite at %lux width %d addr %lux size %d lvl %d\n", hw->addr, hw->bt16?16:8,
	hw->ram, hw->size, hw->lvl);/**/

	/* enable interface RAM, set interface width */
	OUT(hw, msr, MENB|msr);
	if(hw->bt16)
		OUT(hw, laar, laar|L16EN|M16EN|ZeroWS16);

	(*hw->init)(cp);
	setvec(Int0vec + hw->lvl, hw->intr);
	return 0;
}

static void
dp8390rinit(Ctlr *cp)
{
	Hw *hw = cp->hw;

	OUT(hw, w.cr, Page0|RD2|STP);
	OUT(hw, w.bnry, hw->pstart);
	OUT(hw, w.cr, Page1|RD2|STP);
	OUT(hw, curr, hw->pstart+1);
	OUT(hw, w.cr, Page0|RD2|STA);
}

/*
 * we leave the chip idling on internal loopback
 * and pointing to Page0.
 */
static void
dp8390init(Ctlr *cp)
{
	Hw *hw = cp->hw;
	int i;

	OUT(hw, w.cr, Page0|RD2|STP);
	if(hw->bt16)
		OUT(hw, w.dcr, (1<<0)|(3<<5));	/* 16 bit interface, 12 byte DMA burst */
	else
		OUT(hw, w.dcr, 0x48);		/* FT1|LS */
	OUT(hw, w.rbcr0, 0);
	OUT(hw, w.rbcr1, 0);
	if(cp->prom)
		OUT(hw, w.rcr, 0x14);		/* PRO|AB */
	else
		OUT(hw, w.rcr, 0x04);		/* AB */
	OUT(hw, w.tcr, 0x20);			/* LB0 */

	OUT(hw, w.bnry, hw->pstart);
	OUT(hw, w.pstart, hw->pstart);
	OUT(hw, w.pstop, hw->pstop);
	OUT(hw, w.isr, 0xFF);
	OUT(hw, w.imr, 0x1F);			/* OVWE|TXEE|RXEE|PTXE|PRXE */

	OUT(hw, w.cr, Page1|RD2|STP);
	for(i = 0; i < sizeof(cp->ea); i++)
		OUT(hw, par[i], cp->ea[i]);
	OUT(hw, curr, hw->pstart+1);

	OUT(hw, w.cr, Page0|RD2|STA);
	OUT(hw, w.tpsr, 0);
}

void
wd8013mode(Ctlr *cp, int on)
{
	qlock(cp);
	if(on){
		cp->prom++;
		if(cp->prom == 1)
			OUT(cp->hw, w.rcr, 0x14);/* PRO|AB */
	}
	else {
		cp->prom--;
		if(cp->prom == 0)
			OUT(cp->hw, w.rcr, 0x04);/* AB */
	}
	qunlock(cp);
}

static void
wd8013online(Ctlr *cp, int on)
{
	USED(on);
	OUT(cp->hw, w.tcr, 0);
}

static void
wd8013tweek(Ctlr *cp)
{
	uchar msr;
	Hw *hw = cp->hw;
	int s;

	s = splhi();
	msr = IN(hw, msr);
	if((msr & MENB) == 0){
		/* board has reset itself, start again */
		delay(100);

		(*hw->reset)(cp);
		etherinit();

		wakeup(&cp->tr);
		wakeup(&cp->rr);
	}
	splx(s);
}

/*
 *  hack to keep away from the card's memory while it is receiving
 *  a packet.  This is only a problem on the NCR 3170 safari.
 *
 *  we peek at the DMA registers and, if they are changing, wait.
 */
void
waitfordma(Hw *hw)
{
	uchar a,b,c;

	for(;;delay(10)){
		a = IN(hw, r.clda0);
		b = IN(hw, r.clda0);
		if(a != b)
			continue;
		c = IN(hw, r.clda0);
		if(c != b)
			continue;
		break;
	}
}

static void
wd8013receive(Ctlr *cp)
{
	Hw *hw = cp->hw;
	Buffer *rb;
	uchar bnry, curr, next;
	Ring *p;
	int i, len;

	bnry = IN(hw, r.bnry);
	next = bnry+1;
	if(next >= hw->pstop)
		next = hw->pstart;
	for(i = 0; ; i++){
		OUT(hw, w.cr, Page1|RD2|STA);
		curr = IN(hw, curr);
		OUT(hw, w.cr, Page0|RD2|STA);
		if(next == curr)
			break;
		if(strcmp(machtype, "NCRD.0") == 0)
			waitfordma(hw);
		cp->inpackets++;
		p = &((Ring*)hw->ram)[next];
		len = ((p->len1<<8)|p->len0)-4;
		if(p->next < hw->pstart || p->next >= hw->pstop || len < 60){
			/*print("%d/%d : #%2.2ux #%2.2ux  #%2.2ux #%2.2ux\n", next, len,
				p->status, p->next, p->len0, p->len1);/**/
			dp8390rinit(cp);
			break;
		}
		rb = &cp->rb[cp->ri];
		if(rb->owner == Interface){
			rb->len = len;
			if((p->data+len) >= (hw->ram+hw->size)){
				len = (hw->ram+hw->size) - p->data;
				memmove(rb->pkt+len,
					&((Ring*)hw->ram)[hw->pstart],
					(p->data+rb->len) - (hw->ram+hw->size));
			}
			memmove(rb->pkt, p->data, len);
			rb->owner = Host;
			cp->ri = NEXT(cp->ri, cp->nrb);
		}
		p->status = 0;
		next = p->next;
		bnry = next-1;
		if(bnry < hw->pstart)
			bnry = hw->pstop-1;
		OUT(hw, w.bnry, bnry);
	}
}

static void
wd8013transmit(Ctlr *cp)
{
	Hw *hw;
	Buffer *tb;
	int s;

	s = splhi();
	hw = cp->hw;
	tb = &cp->tb[cp->ti];
	if(tb->busy == 0 && tb->owner == Interface){
		memmove(hw->ram, tb->pkt, tb->len);
		OUT(hw, w.tbcr0, tb->len & 0xFF);
		OUT(hw, w.tbcr1, (tb->len>>8) & 0xFF);
		OUT(hw, w.cr, Page0|RD2|TXP|STA);
		tb->busy = 1;
	}
	splx(s);
}

static void
wd8013intr(Ureg *ur)
{
	Ctlr *cp = &ctlr[0];
	Hw *hw = cp->hw;
	Buffer *tb;
	uchar isr;

	USED(ur);
	while(isr = IN(hw, r.isr)){
		OUT(hw, w.isr, isr);
		/*
		 * we have received packets.
		 */
		if(isr & (0x04|0x01)){		/* Rxe|Prx - packet received */
			(*cp->hw->receive)(cp);
			wakeup(&cp->rr);
		}
		if(isr & 0x10)			/* Ovw - overwrite warning */
			cp->overflows++;
		if(isr & 0x08)			/* Txe - transmit error */
			cp->oerrs++;
		if(isr & 0x04){			/* Rxe - receive error */
			cp->frames += IN(hw, r.cntr0);
			cp->crcs += IN(hw, r.cntr1);
			cp->buffs += IN(hw, r.cntr2);
		}
		if(isr & 0x02)			/* Ptx - packet transmitted */
			cp->outpackets++;
		/*
		 * a packet completed transmission, successfully or
		 * not. start transmission on the next buffered packet,
		 * and wake the output routine.
		 */
		if(isr & (0x08|0x02)){
			tb = &cp->tb[cp->ti];
			tb->owner = Host;
			tb->busy = 0;
			cp->ti = NEXT(cp->ti, cp->ntb);
			(*cp->hw->transmit)(cp);
			wakeup(&cp->tr);
		}
	}
}

static Hw wd8013 =
{
	wd8013reset,
	dp8390init,
	wd8013mode,
	wd8013online,
	wd8013receive,
	wd8013transmit,
	wd8013intr,
	wd8013tweek,
};
