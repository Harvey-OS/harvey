/*
 *  EISA PLANET fiber interface -
 *	media access routines
 */
#include "all.h"
#include "mem.h"
#include "io.h"

#define ROUND(t, s, sz)		(t)((((ulong)s)+(sz-1))&~(sz-1))

enum
{
	MAXEISA		= 8,
	Fintlevel	= Int0vec+9,

	RXempty 	= (1<<0),	/* SR bits */
	RXflag  	= (1<<1),
	TXfull		= (1<<2),
	TXflag  	= (1<<3),
	Carrier		= (1<<4),

	Config		= (0<<2),	/* RD IO port addresses */
	SR		= (1<<2),	/* RD */
	CLRint		= (1<<2),	/* WR */
	TXreset		= (2<<2),	/* WR */
	RXreset		= (3<<2),	/* WR */
	Data		= (4<<2),	/* RD/WR */
	RXflagw		= (5<<2),	/* WR */
	TXgo		= (6<<2),	/* WR */
	RXgo		= (7<<2),	/* WR */

	ID0		= 0x19,		/* EISA board ID */
	ID1		= 0x2c,
	ID2		= 0x01,
	ID3		= 0x01,

	Hsync0		= 0x12345678,
	Tsync		= 0xDEADBEEF,

	PKsync		= 0,		/* Frame types */
	PKsrep		= 1,
	PKresync	= 2,
	PKdata		= 3,
	PKaresREQ	= 4,
	PKaresREP	= 5,

	IAMhost		= 1,		/* Type of connection */
	IAMswitch	= 2,
	IAMgateway	= 3,

	Niface		= 1,

	NBlock		= 64,		/* Fifo block size */
	BY2BLOCK	= NBlock*BY2V,	/* Bytes per block */
	RXBlock2Flag	= 0x04040404,	/* CY474 flag programming */
	TXBlock2Flag	= 0x05050505,	/* CY474 flag programming */

	IFdown		= 1,		/* Not framed */
	IFup		= 2,
	IFshutdown	= 3,		/* Something bad happened */

	SMEM		= 4*1024,
	NFILMB		= 32,
};

typedef struct Dma Dma;
typedef struct Preamble Preamble;
typedef struct Epilog Epilog;
typedef struct If If;

struct Dma
{
	char*	addr;			/* Destination address */
	int	count;			/* Bytes remaining in transfer */
	ulong	csum;			/* Current checksum */
	int	hdr;			/* Header tx is pending */
	int	ien;			/* Eligible for interrupt dma */
	Msgbuf*	bp;			/* Destination data buffer */
};

struct Preamble
{
	uchar	Hsync0[4];		/* Header sync */
	uchar	type;			/* Packet type */
	uchar	iftype;
	uchar	hlen[2];		/* Header length */
	uchar	len[4];			/* Packet length in vlongs */
	uchar	addr[4];		/* MAC Source/Protocol Destination */
};

struct Epilog
{
	uchar	csum[4];		/* Checksum */
	uchar	Tsync[4];		/* Trailer sync */
};

struct If
{
	Lock;				/* Held by process writing to fifo */
	Msgbuf*		thead;
	Msgbuf*		ttail;
	Dma		txdma;		/* Sofware dma structures */

	Msgbuf*		rhead;
	Msgbuf*		rtail;
	Rendez		rr;

	Dma		rxdma;
	int		state;		/* Interface availability */
	Lock		syncl;
	int		syncup;		/* Set to wakeup a sync process */
	Rendez		sr;
	Preamble*	pre;		/* Canned preamble/epilog */
	Epilog*		post;
	uchar*		rcvpre;
	uchar*		rcvpost;
	uchar*		junk;
	ulong		txflagw;	/* to program cypress flag words */
	ulong		rxflagw;
	ulong		hwaddr;		/* Address of fiber controller */
	ulong		address;

	/* File system interface */
	Ifc		ifc;
	char		onam[NAMELEN];
	char		inam[NAMELEN];
};
If	iface[Niface];

struct
{
	Lock;
	uchar	space[SMEM];
} pool;

ulong	frblock(ulong, void*, ulong);
ulong	fwblock(ulong, void*, ulong);
void	fiberint(Ureg*, void*);
void	findsync(If*);

#define IP(v)		(v>>24), (v>>16)&0xff, (v>>8)&0xff, v&0xff

void
ifoff(If *pif)
{
	print("fiber%d: if shutdown: check fiber connection\n", pif-iface);
	pif->state = IFshutdown;
}

static
void
output(void)
{
	If *c;
	int n;
	Msgbuf *mb;

	c = getarg();
	print("fiber output %s\n", c->onam);

	for(;;) {
		mb = recv(c->ifc.reply, 0);
		if(mb == 0)
			continue;

		n = mb->count;
		filoput(mb);

		c->ifc.work.count++;
		c->ifc.rate.count += n;
		c->ifc.txpkt++;
	}
}

static int
gotpkt(void *c)
{
	return ((If*)c)->rhead != 0;
}

static
void
input(void)
{
	If *c;
	Msgbuf *mb;

	c = getarg();
	print("fiber input %s\n", c->inam);

	findsync(c);

	for(;;) {
		sleep(&c->rr, gotpkt, c);

		ilock(c);
		mb = c->rhead;
		c->rhead = mb->next;
		iunlock(c);

		c->ifc.work.count++;
		c->ifc.rate.count += mb->count;
	
		filrecv(mb, &c->ifc);
	}
}

static char*
istate[] =
{
	[IFup]		"Up",
	[IFdown]	"Down",
	[IFshutdown]	"Shutdown"
};

static void
statf(If *ctlr)
{
	Ifc *ifc;

	print("fiber stats %d port 0x%4.4ux addr %d.%d.%d.%d %s\n",
		ctlr-iface, ctlr->hwaddr,
		IP(ctlr->address), istate[ctlr->state]);

	ifc = &ctlr->ifc;
	print("	work = %F pkts\n", (Filta){&ifc->work, 1});
	print("	rate = %F tBps\n", (Filta){&ifc->rate, 1000});
	print("	err  =    %3ld rc %3ld sum\n", ifc->rcverr, ifc->sumerr);
}

static void
cmd_statf(int argc, char *argv[])
{
	int i;

	USED(argc, argv);

	for(i = 0; i < Niface; i++)
		if(iface[i].hwaddr)
			statf(&iface[i]);
}

void
ifinit(int ctlr)
{
	int i;
	If *pif;
	ulong port;
	uchar sr, *p;
	static int boottime;
	extern void filproc(void);

	if(ctlr < 0 || ctlr > Niface)
		panic("ifinit: bad interface #: %d", ctlr);

	if(boottime == 0) {
		userinit(filproc, 0, "filt");
		cmd_install("statf", "-- planet fiber stats", cmd_statf);
		boottime = 1;
	}

	pif = &iface[ctlr];

	p = pool.space;
	pif->rcvpre = (void*)p;
	p = ROUND(uchar*, p+sizeof(Preamble), BY2V);
	pif->rcvpost = (void*)p;
	p = ROUND(uchar*, p+sizeof(Epilog), BY2V);
	pif->junk = (void*)p;
	p = ROUND(uchar*, p+BY2BLOCK, BY2V);

	/*
	 * Build canned preamble/epilog in aligned buffers
	 */
	pif->pre = (void*)p;
	p = ROUND(uchar*, p+sizeof(Preamble), BY2V);
	hnputl(pif->pre->Hsync0, Hsync0);

	pif->post = (void*)p;
	p = ROUND(uchar*, p+sizeof(Epilog), BY2V);
	hnputl(pif->post->Tsync, Tsync);
	if(p - pool.space >= SMEM)
		panic("fiber: not enough SMEM");

	/*
	 * Need to sync up with the other end
	 */
	pif->state = IFdown;

	pif->rxflagw = RXBlock2Flag;
	pif->txflagw = TXBlock2Flag;

	port = 0;
	for(i = 1; i < MAXEISA; i++) {
		port = i<<12;
		outb(port+0xC80, 0xff);
		if(inb(port+0xC80) == ID0 && inb(port+0xC81) == ID1 &&
		   inb(port+0xC82) == ID2 && inb(port+0xC83) == ID3)
			break;
	}

	if(i >= MAXEISA) {
		print("fiber: No interface found\n");
		return;
	}

	getipa(&pif->ifc);
	print("fiber%d: port 0x%lux addr %I\n", ctlr, port, pif->ifc.ipa);
	pif->hwaddr = port;

	sr = inl(port+SR);
	if((sr & 0x10) == 0) {
		print("fiber%d: lost carrier - check cables/remote power\n", ctlr);
		return;
	}

	outl(port+CLRint, 0);
	/*
	 * Try and make the fifo's empty
	 */
	for(i = 0; i < 10; i++) {
		outl(pif->hwaddr+RXreset, 0);
		outl(pif->hwaddr+RXflagw, pif->rxflagw);
 		outl(pif->hwaddr+RXgo, 0);

		outl(pif->hwaddr+TXreset, 0);
		outl(pif->hwaddr+Data, pif->txflagw);
		outl(pif->hwaddr+TXgo, 0);

		delay(100);

		sr = inl(pif->hwaddr+SR);
		if(sr & RXempty)
			break;
	}
	if(i >= 10)
		print("fiber%d: reset failed\n", ctlr);
	else
	if(sr & RXflag)
		print("fiber%d: stuck RXflag\n", ctlr);
	else
	if((sr & TXflag) == 0)
		print("fiber%d: stuck TXflag\n", ctlr);

	setvec(Fintlevel, fiberint, pif);

	pif->address = nhgetl(pif->ifc.ipa);

	dofilter(&pif->ifc.work);
	dofilter(&pif->ifc.rate);
	pif->ifc.reply = newqueue(Nqueue);

	sprint(pif->onam, "fo%4.4ux", pif->hwaddr);
	userinit(output, pif, pif->onam);
	sprint(pif->inam, "fi%4.4ux", pif->hwaddr);
	userinit(input, pif, pif->inam);
}

/*
 * Send a sync message and pad it to a block to ensure an interrupt
 */
void
sendsync(If *f, ulong sync, uchar type)
{
	int i;
	uchar *p;
	ulong port;

	port = f->hwaddr+Data;
	p = (uchar*)f->pre;

	/*
	 * Pad sync to a block to ensure an interrupt
	 */
	i = BY2BLOCK - sizeof(Preamble);
	i /= BY2V;

	f->pre->type = type;
	f->pre->iftype = IAMhost;
	hnputl(f->pre->len, sync);
	hnputs(f->pre->hlen, i);
	hnputl(f->pre->addr, f->address);

	outsl(port, p, sizeof(Preamble)/BY2WD);
	i *= 2;
	while(i--)
		outl(port, i);
}

static void
freset(If *pif)
{
	pif->state = IFdown;

	outl(pif->hwaddr+RXreset, 0);
	outl(pif->hwaddr+RXflagw, pif->rxflagw);
 	outl(pif->hwaddr+RXgo, 0);

	sendsync(pif, 0, PKresync);
}

void
rdtrailer(ulong port, Epilog *pp)
{
	int t;

	t = 1000000;
	while(inl(port+SR) & RXempty)
		if(t-- == 0)
			break;
	if(t == 0)
		print("fiber: trailer timeout\n");

	insl(port+Data, pp->csum, 1);

	t = 1000000;
	while(inl(port+SR) & RXempty)
		if(t-- == 0)
			break;
	if(t == 0)
		print("fiber: trailer timeout\n");

	insl(port+Data, pp->Tsync, 1);
}

/*
 * rxdiscard is called when a buffer cannot be allocated at interrupt time
 */
static void
rxdiscard(If *pif)
{
	Dma *r;
	ulong ts;
	Epilog *pp;
	ulong port;

	r = &pif->rxdma;
	port = pif->hwaddr;

	for(;;) {
		if((inl(port+SR) & RXflag) == 0)
			return;

		frblock(port+Data, pif->junk, 0);
		r->count -= NBlock;
		if(r->count == 0)		/* Complete frame */
			break;
	}

	pp = (Epilog*)pif->rcvpost;
	rdtrailer(port, pp);
	ts = nhgetl(pp->Tsync);
	if(ts != Tsync) {
		pif->ifc.rcverr++;
		print("fiber%d: bad trailer (r#%.8lux)\n", pif-iface, ts);
		freset(pif);
	}
}

static void
rxdma(If *pif)
{
	Dma *r;
	Msgbuf *bp;
	Epilog *pp;
	ulong port;
	ulong t, crc;

	r = &pif->rxdma;
	if(r->addr == 0) {
		rxdiscard(pif);
		return;
	}
	port = pif->hwaddr;

	while((inl(port+SR) & RXflag) && r->count) {
		r->csum = frblock(port+Data, r->addr, r->csum);
		r->addr += BY2BLOCK;
		r->count -= NBlock;
	}

	if(r->count != 0)
		return;

	pp = (Epilog*)pif->rcvpost;
	rdtrailer(port, pp);

	t = nhgetl(pp->Tsync);
	if(t != Tsync) {
		pif->ifc.rcverr++;
		print("fiber%d: bad trailer (r#%.8lux)\n", pif-iface, t);
		freset(pif);
		mbfree(r->bp);
		return;
	}

	bp = r->bp;
	bp->count = r->addr - bp->data;

	crc = *(ulong*)pp->csum;
	if(crc != r->csum) {
		pif->ifc.sumerr++;
		print("fiber%d: crc error (r#%.8lux/h#%.8lux/#%.8lux)\n",
				pif - iface, r->csum, crc, crc^r->csum);
		mbfree(r->bp);
		return;
	}

	/* Must be splhi */
	lock(pif);
	bp->next = 0;
	if(pif->rhead == 0)
		pif->rhead = bp;
	else
		pif->rtail->next = bp;
	pif->rtail = bp;
	unlock(pif);

	wakeup(&pif->rr);
}

static void
rcvint(If *pif, ulong port)
{
	Dma *r;
	ulong h1;
	Preamble *pp;
	int hlen, len;
 
	insl(port+Data, pif->rcvpre, sizeof(Preamble)/BY2WD);
	pp = (Preamble*)pif->rcvpre;

	h1 = nhgetl(pp->Hsync0);
	if(h1 != Hsync0) {
		pif->ifc.rcverr++;
		if(pif->state == IFup)
			print("fiber%d: bad frame sync (1#%.8lux)\n",
				pif-iface, h1);
		freset(pif);
		return;
	}

	switch(pp->type) {
	default:
		pif->ifc.rcverr++;
		if(pif->state == IFup)
			print("fiber%d: frame type %d\n", pif-iface, pp->type);
		freset(pif);
		break;
	case PKdata:
		r = &pif->rxdma;
		len = nhgetl(pp->len);
		if(len > 4096)
			print("fiber%d: len %d\n", pif-iface, len);
		r->count = len;
		r->csum = 0;
		r->bp = mballoc(LARGEBUF, 0, Mbfil);
		r->addr = 0;
		if(r->bp != 0)
			r->addr = r->bp->data;
		rxdma(pif);
		break;
	case PKresync:
		pif->state = IFdown;
		print("fiber%d: if remote sync\n", pif-iface);
		/* Fall through */
	case PKsync:
		hlen = nhgets(pp->hlen)*2;
		while(hlen--)
			inl(port+Data);
		sendsync(pif, nhgetl(pp->len), PKsrep);
		break;
	case PKsrep:
		hlen = nhgets(pp->hlen)*2;
		while(hlen--)
			inl(port+Data);
		pif->syncup = 1;
		wakeup(&pif->sr);
		break;
	}
}

int
syncint(void *a)
{
	If *pif;

	pif = a;
	return pif->syncup == 1;
}

/* 
 * Called under the transmitter qlock
 */
void
findsync(If *pif)
{
	int i;
	ulong sr, sync, port;

	if(!canlock(&pif->syncl))
		return;

	port = pif->hwaddr;
	i = pif - iface;

	print("fiber%d: if down\n", i);
	sr = inl(port+SR);
	if((sr & 0x10) == 0)
		print("fiber%d: lost carrier\n", i);

	sync = 0;
	pif->syncup = 0;
	for(;;) {
		/* Try and make the fifo empty */
		for(i = 0; i < 10; i++) {
			outl(pif->hwaddr+RXreset, 0);
			outl(pif->hwaddr+RXflagw, pif->rxflagw);
 			outl(pif->hwaddr+RXgo, 0);

			outl(pif->hwaddr+TXreset, 0);
			outl(pif->hwaddr+Data, pif->txflagw);
			outl(pif->hwaddr+TXgo, 0);
			if(inl(port+SR) & RXempty)
				break;

			tsleep(&pif->sr, no, 0, 100);
		}
		if(i >= 10) {
			ifoff(pif);
			return;
		}

		sendsync(pif, sync++, PKsync);

		tsleep(&pif->sr, syncint, pif, 100);
		if(pif->syncup)
			break;
	}
	pif->state = IFup;
	print("fiber%d: if sync up\n", pif - iface);
	unlock(&pif->syncl);
}

void
txdma(If *pif)
{
	Dma *t;
	ulong x, port;
	Preamble *pp;

	port = pif->hwaddr;
	if((inl(port+SR) & TXflag) == 0)
		return;

	t = &pif->txdma;

	if(t->hdr) {
		pp = pif->pre;
		pp->type = PKdata;
		hnputl(pp->len, t->count);
		/* XXX temporary crap code */
		*(ulong*)pp->addr = *(ulong*)t->addr;
		outsl(port+Data, pp, sizeof(Preamble)/BY2WD);
		t->hdr = 0;
	}

	while(inl(port+SR) & TXflag) {
		t->csum = fwblock(port+Data, t->addr, t->csum);
		t->count -= NBlock;
		if(t->count == 0)
			break;
		t->addr += BY2BLOCK;
	}

	if(t->count != 0)
		return;

	*((ulong*)pif->post->csum) = t->csum;
	x = 1000000;
	while((inl(port+SR) & TXflag) == 0)
		if(x-- == 0)
			break;

	if(x == 0)
		print("filber: epilog timeout\n");

	outsl(port+Data, pif->post, BY2V/BY2WD);
	if(t->bp->flags & BFREE)
		mbfree(t->bp);
}

void
txint(If *pif)
{
	Dma *t;
	int len;
	Msgbuf *bp;

	if(pif->state != IFup)
		return;

	lock(pif);
	bp = pif->thead;
	if(bp != 0)
		pif->thead = bp->next;
	unlock(pif);
	if(bp == 0)
		return;
	
	/* Compute word length rounded to blocks */
	len = ROUND(int, bp->count, BY2BLOCK);
	len /= BY2V;

	t = &pif->txdma;
	t->count = len;
	t->addr = bp->data;
	t->csum = 0;
	t->bp = bp;
	t->hdr = 1;

	txdma(pif);
}

void
fiberint(Ureg *ur, void *ctlr)
{
	If *pif;
	ulong t, port;

	USED(ur);

	pif = ctlr;
	port = pif->hwaddr;

	outl(port+CLRint, 0);

	t = 1000;
	while(inl(port+SR) & RXflag) {
		if(pif->rxdma.count == 0)
			rcvint(pif, port);
		else
			rxdma(pif);

		if(t-- == 0)
			break;
	}
	if(t == 0)
		print("fiber: rx intr timeout\n");

	t = 1000;
	while(inl(port+SR) & TXflag) {
		if(pif->txdma.count)
			txdma(pif);
		else {
			txint(pif);
			if(pif->txdma.count == 0)
				break;
		}

		if(t-- == 0)
			break;
	}
	if(t == 0)
		print("fiber: tx intr timeout\n");
}

void
ifwrite(If *pif, Msgbuf *bp, int dofree)
{
	ulong s, port;

	if(pif->state != IFup)
		findsync(pif);

	if(dofree)
		bp->flags |= BFREE;

	bp->next = 0;
	ilock(pif);
	if(pif->thead == 0)
		pif->thead = bp;
	else
		pif->ttail->next = bp;
	pif->ttail = bp;
	iunlock(pif);

	port = pif->hwaddr;
	s = splhi();
	while(inl(port+SR) & TXflag) {
		if(pif->txdma.count)
			txdma(pif);
		else {
			txint(pif);
			if(pif->txdma.count == 0)
				break;
		}

		if(pif->state != IFup)
			break;
	}
	splx(s);
}

ulong
ifaddr(If *pif)
{
	return pif->address;
}

void*
ifroute(ulong addr)
{
	USED(addr);
	return &iface[0];
}

ulong
ifunroute(ulong addr)
{
	USED(addr);
	return iface[0].address;
}
