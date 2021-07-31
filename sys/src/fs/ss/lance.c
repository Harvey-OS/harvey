#include "all.h"
#include "io.h"
#include "mem.h"

typedef	struct	Msg	Msg;
typedef	struct	Ctlr	Ctlr;

enum
{
	MaxCtlr = 1,			/* Max. configurable controllers */
	Maxrb =	128,			/* Max buffers in a ring */
};

#define NEXT(x, l)	(((x)+1)%(l))
#define OFFSETOF(t, m)	((unsigned)&(((t *)0)->m))
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

/*
 * Communication with the lance is via a transmit and receive ring of
 * message descriptors.  The Initblock contains pointers to and sizes of
 * these rings.  The rings must be in RAM addressible by the lance
 */
struct	Msg
{
	ushort	laddr;		/* low order piece of address */
	ushort	flags;		/* flags and high order piece of address */
	short	size;		/* size of buffer */
	ushort	cntflags;	/* (rcv)count of bytes in buffer; (xmt) more flags */
};

/*
 * Message descriptor flags.
 */
#define ENP	0x0100	/* end of packet */
#define STP	0x0200	/* start of packet */
#define BUF	0x0400	/* receive: out of buffers while reading a packet */
#define DEF	0x0400	/* transmit: deferred while transmitting packet */
#define CRC	0x0800	/* receive: CRC error reading packet */
#define ONE	0x0800	/* transmit: one retry to transmit the packet */
#define OFLO	0x1000	/* receive: lost some of the packet */
#define MORE	0x1000	/* transmit: more than 1 retry to send the packet */
#define FRAM	0x2000	/* incoming packet not multiple of 8 bits+CRC error */
#define ERR	0x4000	/* error summary, the OR of all error bits */
#define OWN	0x8000	/* 1 means that the buffer can be used by the chip */

/*
 *  lance memory map
 */
struct	Lanmem
{
	/*
	 *  initialization block
	 */
	ushort	mode;		/* chip control (see below) */
	ushort	etheraddr[3];	/* the ethernet physical address */
	ushort	multi[4];	/* multicast addresses, 1 bit for each of 64 */
	ushort	rdralow;	/* receive buffer ring */
	ushort	rdrahigh;	/* (top three bits define size of ring) */
	ushort	tdralow;	/* transmit buffer ring */
	ushort	tdrahigh;	/* (top three bits define size of ring) */
	
	/*
	 *  ring buffers
	 *  first receive, then transmit
	 *  these must be aligned on a quad-short
	 *  boundary
	 */
	Msg	rmr[Maxrb];		/* receive message ring */
	Msg	tmr[Maxrb];		/* transmit message ring */
};

/*
 *  Some macros for dealing with lance memory addresses.  The lance splits
 *  its 24 bit addresses across two 16 bit registers.
 */
#define HADDR(a) ((((ulong)(a))>>16)&0xFF)
#define LADDR(a) (((ulong)a)&0xFFFF)

/*
 * The following functions exist to sidestep a quirk in the SGI IO3 lance
 * interface. In all other processors, the lance's initialization block and
 * descriptor rings look like normal memory. In the SGI IO3, the CPU sees a
 * 6 byte pad twixt all lance memory shorts. Therefore, we use the following
 * macros to compute the address whenever accessing the lance memory to make
 * the code portable. Sic transit gloria.
 */
#define MPs(c, a) (*(short *)(c->lanceram + c->sep*(OFFSETOF(Lanmem, a)/2)))
#define MPus(c, a) (*(ushort *)(c->lanceram + c->sep*(OFFSETOF(Lanmem, a)/2)))

/*
 * Software Controller
 */
struct	Ctlr
{
	Lance;
	QLock;			/* keep readers and writers separate */
	int	open;
	int	init;
	Rendez	rr;		/* rendezvous for an input buffer */
	ushort	rl;		/* first rcv Message belonging to Lance */	
	ushort	rc;		/* first rcv Message belonging to CPU */

	Rendez	tr;		/* rendezvous for an output buffer */
	ushort	tl;		/* first xmt Message belonging to Lance */	
	ushort	tc;		/* first xmt Message belonging to CPU */	
	Ifc	ifc;
};

/*
 * Lance CSR0, this is the register we play with most often. We leave
 * this register pointed to by RAP in normal operation.
 */
#define INIT	0x0001			/* read initialisation block */
#define STRT	0x0002			/* enable chip to send and receive */
#define STOP	0x0004			/* disable chip from external activity */
#define TDMD	0x0008			/* transmit demand - don't wait for poll */
#define TXON	0x0010			/* transmitter is enabled */
#define RXON	0x0020			/* receiver is enabled */
#define INEA	0x0040			/* interrupt enable */
#define INTR	0x0080			/* interrupt pending */
#define IDON	0x0100			/* initialisation done */
#define TINT	0x0200			/* transmitter interrupt pending */
#define RINT	0x0400			/* receiver interrupt pending */
#define MERR	0x0800			/* memory error */
#define MISS	0x1000			/* missed packet */
#define CERR	0x2000			/* collision error (no heartbeat) */
#define BABL	0x4000			/* transmitter packet too long */
#define ERR0	0x8000			/* BABL|CERR|MISS|MERR */

static	Ctlr	ctlr[MaxCtlr];

static
void
stats(Ctlr *c)
{
	Ifc *ifc;

	print("lance stats %d\n", c-ctlr);

	ifc = &c->ifc;
	print("	work = %F pkts\n", (Filta){&ifc->work, 1});
	print("	rate = %F tBps\n", (Filta){&ifc->rate, 1000});
	print("	err  =    %3ld rc %3ld sum\n",
		ifc->rcverr, ifc->sumerr);
}

static
void
cmd_stats(int argc, char *argv[])
{
	Ctlr *c;

	USED(argc);
	USED(argv);

	for(c = &ctlr[0]; c < &ctlr[MaxCtlr]; c++)
		if(c->init)
			stats(c);
}

static
void
qrxb(Ctlr *c)
{

	MPs(c, rmr[c->rc].size) = -sizeof(Enpkt);
	MPus(c, rmr[c->rc].cntflags) = 0;
	MPus(c, rmr[c->rc].laddr) = LADDR(&c->lrp[c->rc]);
	MPus(c, rmr[c->rc].flags) = HADDR(&c->lrp[c->rc])|OWN;
	wbflush();
	c->rc = NEXT(c->rc, c->nrrb);
	c->rl = NEXT(c->rl, c->nrrb);
}

static
int
anyrxb(Ctlr *c)
{
	return c->rl != c->rc && (MPus(c, rmr[c->rl].flags) & OWN) == 0;
}

static
void
input(void)
{
	int l, type;
	Ctlr *c;
	Enpkt *p;
	Ifc *ifc;

	c = getarg();
	ifc = &c->ifc;

	print("lance input %d: %E %I\n", c-ctlr, c->ifc.ea, c->ifc.ipa);

loop:
	qlock(c);
	while(anyrxb(c) == 0) {
		qunlock(c);
		sleep(&c->rr, anyrxb, c);
		goto loop;
	}

	if((MPus(c, rmr[c->rl].flags) & ERR) == 0) {
		l = MPus(c, rmr[c->rl].cntflags) - 4;
		p = &c->rp[c->rl];
		type = nhgets(p->type);
		switch(type) {
		default:
			break;
		case Arptype:
			arpreceive(p, l, ifc);
			break;
		case Iptype:
			ipreceive(p, l, ifc);
			ifc->rxpkt++;
			ifc->work.count++;
			ifc->rate.count += l;
			break;
		}
	} else
		ifc->rcverr++;
	qrxb(c);
	qunlock(c);

	goto loop;
}

static
void
qtxb(Ctlr *c, int mtu)
{
	MPs(c, tmr[c->tc].size) = -mtu;
	MPus(c, tmr[c->tc].cntflags) = 0;
	MPus(c, tmr[c->tc].laddr) = LADDR(&c->ltp[c->tc]);
	MPus(c, tmr[c->tc].flags) = OWN|STP|ENP|HADDR(&c->ltp[c->tc]);
	wbflush();
	c->tc = NEXT(c->tc, c->ntrb);
	*c->rdp = INEA|TDMD;
	wbflush();
}

static
int
anytxb(Ctlr *c)
{
	return c->tl != NEXT(c->tc, c->ntrb);
}

static
void
output(void)
{
	Enpkt *p;
	Msgbuf *mb;
	Ctlr *c;
	int n;

	c = getarg();
	print("lance output %d\n", c-ctlr);

loop:
	mb = recv(c->ifc.reply, 0);
	if(mb == 0)
		goto loop;

	memmove(((Enpkt*)(mb->data))->s, c->ifc.ea, Easize);
	n = mb->count;
	if(n > ETHERMAXTU) {
		print("lance: pkt too big\n");
		mbfree(mb);
		goto loop;
	}

	while(anytxb(c) == 0)
		sleep(&c->tr, anytxb, c);

	qlock(c);
	p = &c->tp[c->tc];
	memmove(p, mb->data, n);
	if(n < ETHERMINTU) {
		memset((char*)p+n, 0, ETHERMINTU-n);
		n = ETHERMINTU;
	}
	qtxb(c, n);

	c->ifc.work.count++;
	c->ifc.rate.count += n;
	c->ifc.txpkt++;
	qunlock(c);

	mbfree(mb);
	goto loop;
}

void
lancestart(void)
{
	Ctlr *c;
	Ifc *ifc;

	for(c = &ctlr[0]; c < &ctlr[MaxCtlr]; c++) {
		if(c->init) {
			ifc = &c->ifc;
			lock(ifc);
			if(ifc->reply == 0) {
				dofilter(&ifc->work);
				dofilter(&ifc->rate);
				ifc->reply = newqueue(Nqueue);
				cmd_install("statl", "-- lance stats", cmd_stats);
			}
			getipa(ifc);
			unlock(ifc);
			qlock(c);
			qunlock(c);
			userinit(output, c, "lao");
			userinit(input, c, "lai");

			ifc->next = enets;
			enets = ifc;
		}
	}
	arpstart();
}

static
void
reset(Ctlr *c)
{
	if(c->open == 0) {
		c->open = 1;
		lancesetup(c);
	}
	/*
	 *  stop the lance
	 */
	*c->rap = 0;
	*c->rdp = STOP;
}

int
lanceinit(uchar ctlrno)
{
	Ctlr *c;
	int i;

	print("lance init %d\n", ctlrno);
	if(ctlrno >= MaxCtlr)
		return -1;

	c = &ctlr[ctlrno];
	reset(c);
	c->rl = 0;
	c->rc = 0;
	c->tl = 0;
	c->tc = 0;

	/*
	 * create the initialization block
	 */
	MPus(c, mode) = 0;

	/*
	 * set ether addr from the value in the id prom.
	 * the id prom has them in reverse order, the init
	 * structure wants them in byte swapped order
	 */
	memmove(c->ifc.ea, c->ea, Easize);
	MPus(c, etheraddr[0]) = (c->ifc.ea[1]<<8)|c->ifc.ea[0];
	MPus(c, etheraddr[1]) = (c->ifc.ea[3]<<8)|c->ifc.ea[2];
	MPus(c, etheraddr[2]) = (c->ifc.ea[5]<<8)|c->ifc.ea[4];

	/*
	 * ignore multicast addresses
	 */
	MPus(c, multi[0]) = 0;
	MPus(c, multi[1]) = 0;
	MPus(c, multi[2]) = 0;
	MPus(c, multi[3]) = 0;

	/*
	 * set up rcv message ring
	 */
	for(i = 0; i < c->nrrb; i++){
		MPs(c, rmr[i].size) = -sizeof(Enpkt);
		MPus(c, rmr[i].cntflags) = 0;
		MPus(c, rmr[i].laddr) = LADDR(&c->lrp[i]);
		MPus(c, rmr[i].flags) = HADDR(&c->lrp[i]);
	}
	MPus(c, rdralow) = LADDR(c->lm->rmr);
	MPus(c, rdrahigh) = (c->lognrrb<<13)|HADDR(c->lm->rmr);

	/*
	 * give the lance all the rcv buffers except one (as a sentinel)
	 */
	c->rc = c->nrrb - 1;
	for(i = 0; i < c->rc; i++)
		MPus(c, rmr[i].flags) |= OWN;

	/*
	 * set up xmit message ring
	 */
	for(i = 0; i < c->ntrb; i++){
		MPs(c, tmr[i].size) = 0;
		MPus(c, tmr[i].cntflags) = 0;
		MPus(c, tmr[i].laddr) = LADDR(&c->ltp[i]);
		MPus(c, tmr[i].flags) = HADDR(&c->ltp[i]);
	}
	MPus(c, tdralow) = LADDR(c->lm->tmr);
	MPus(c, tdrahigh) = (c->logntrb<<13)|HADDR(c->lm->tmr);

	/*
	 * point lance to the initialization block
	 */
	*c->rap = 1;
	*c->rdp = LADDR(c->lm);
	wbflush();
	*c->rap = 2;
	*c->rdp = HADDR(c->lm);

	/*
	 * The lance byte swaps the ethernet packet unless we tell it not to
	 */
	wbflush();
	*c->rap = 3;
	*c->rdp = c->busctl;

	/*
	 *  initialize lance, turn on interrupts, turn on transmit and rcv.
	 */
	wbflush();
	*c->rap = 0;
	*c->rdp = INEA|INIT|STRT;

	c->init = 1;

	return 0;
}

void
lanceintr(uchar ctlrno)
{
	Ctlr *c;
	int csr;

	if(ctlrno >= MaxCtlr)
		panic("lance intr %d\n", ctlrno);
	c = &ctlr[ctlrno];
	if(!c->init) {
		c->rap = LANCERAP;
		c->rdp = LANCERDP;
		*c->rap = 0;
		csr = *c->rdp;
		USED(csr);
		*c->rdp = IDON|INEA|TINT|RINT|BABL|CERR|MISS|MERR;
		if(*c->rdp & (BABL|MISS|MERR|CERR))
			*c->rdp = BABL|CERR|MISS|MERR;
		return;
	}

	*c->rap = 0;
	csr = *c->rdp;

	/*
	 *  turn off the interrupt and any error indicators
	 */
	*c->rdp = IDON|INEA|TINT|RINT|BABL|CERR|MISS|MERR;
	wbflush();

	/*
	 *  see if an error occurred
	 */
	if(csr & (BABL|MISS|MERR))
		print("lance error #%ux\n", csr);

	/*
	 *  look for rcv'd packets
	 */
	if(c->rl != c->rc && (MPus(c, rmr[c->rl].flags) & OWN) == 0)
		wakeup(&c->rr);

	/*
	 *  look for xmitt'd packets
	 */
	while(c->tl != c->tc && (MPus(c, tmr[c->tl].flags) & OWN) == 0) {
		if(MPus(c, tmr[c->tl].flags) & ERR)
			c->ifc.txerr++;
		c->tl = NEXT(c->tl, c->ntrb);
		wakeup(&c->tr);
	}
}
