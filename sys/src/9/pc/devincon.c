#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"

/* Centronix parallel (printer) port */

typedef struct Lptinfo	Lptinfo;

struct Lptinfo
{
	int	base;		/* port number */
	int	ivec;		/* interrupt number */
};

static Lptinfo lptinfo[] = {
	0x3bc,	Parallelvec,	/* lpt1 (safari) */
	0x378,	Parallelvec,	/* lpt1 (`official') */
	0x278,	Int0vec+5,	/* lpt2 (`official') */
};
#define NDEV	(sizeof lptinfo/sizeof lptinfo[0])

extern int	incondev;	/* from config */

/* offsets, and bits in the registers */
enum
{
	/* data latch register */
	Qdlr=		0x0,
	/* printer status register */
	Qpsr=		0x1,
	Fbusy=		0x80,
	Fintbar=	0x40,
	/* printer control register */
	Qpcr=		0x2,
	Fie=		0x10,
	Fsi=		0x08,
	Finitbar=	0x04,
	Faf=		0x02,
	Fstrobe=	0x01,

	/* other `registers' */
	Qstats=		0x03,
	Qdebug=		0x04
};

/*
 *	af  si  dlr<7:0>
 *	 0   x  xxxxxxxx	wr ctl/data char (si is dcto)
 *	 1   0  xxxxxxxx	wr cmd
 *	 1   1	xxxxxx00	rd ctl/data char
 *	 1   1	xxxxxx01	rd status
 *	 1   1	xxxxxx11	nop (multiplexer to normal state)
 *
 *	 psr<7:3> multiplexed as follows:
 *
 *	 		bc7  bc6  bc5  bc4  bc3
 *	 rd, st- low	bsy   d8   d7   d6   d5
 *	 rd, st- high	 d4   d3   d2   d1   d0
 *	 other		bsy  int-
 */

typedef struct Incon	Incon;

#define NOW (MACHP(0)->ticks*MS2HZ)

#define DPRINT if(incondebug)kprint

static void	inconintr(Ureg *ur);

enum {
	Minstation=	2,	/* lowest station # to poll */
	Maxstation=	15,	/* highest station # to poll */
	Nincon=		1,	/* number of incons */
	Nin=		32,	/* Blocks in the input ring */
	Bsize=		128,	/* size of an input ring block */
	Mfifo=		0xff	/* a mask, must be 2^n-1, must be > Nin */
};

struct Incon {
	QLock;

	QLock	xmit;		/* transmit lock */
	QLock	reslock;	/* reset lock */
	int	dev;		/* base address of port */
	int	station;	/* station number */
	int	state;		/* chip state */
	Rendez	r;		/* output process */
	Rendez	kr;		/* input kernel process */
	ushort	chan;		/* current input channel */
	Queue	*rq;		/* read queue */
	int	kstarted;	/* true if kernel process started */

	/*  input blocks */

	Block	*inb[Nin];
	ushort	wi;		
	ushort	ri;

	/* statistics */

	ulong	overflow;	/* overflow errors */
	ulong	pack0;		/* channel 0 */
	ulong	crc;		/* crc errors */
	ulong	in;		/* bytes in */
	ulong	out;		/* bytes out */
	ulong	wait;		/* wait time in milliseconds */
};

Incon incon[Nincon];

/*
 *  chip state
 */
enum {
	Selecting,
	Selected,
	Notliving,
};

/*
 *  internal chip registers
 */
#define	sel_polln	0
#define	sel_station	1
#define	sel_poll0	2
#define sel_rcv_cnt	3
#define sel_rcv_tim	4
#define sel_tx_cnt	5

/*
 *  CSR bits
 */
#define INCON_RUN	0x80
#define INCON_STOP	0x00
#define ENABLE_IRQ	0x40
#define ENABLE_TX_IRQ	0x20
#define INCON_ALIVE	0x80
#define TX_FULL		0x10
#define TX_EMPTY	0x08
#define RCV_EMPTY	0x04
#define OVERFLOW	0x02
#define CRC_ERROR	0x01

/*
 *  polling constants
 */
#define HT_GNOT	0x30
#define HT_UNIX	0x31
#define ST_UNIX 0x04
#define NCHAN 64

static void inconkproc(void*);

/*
 *  incon stream module definition
 */
static void inconoput(Queue*, Block*);
static void inconstopen(Queue*, Stream*);
static void inconstclose(Queue*);
Qinfo inconinfo =
{
	nullput,
	inconoput,
	inconstopen,
	inconstclose,
	"incon"
};

int incondebug = 0;

Dirtab incondir[]={
	"dlr",		{Qdlr},		1,		0666,
	"psr",		{Qpsr},		5,		0444,
	"pcr",		{Qpcr},		0,		0666,
	"stats",	{Qstats},	0,		0444,
	"debug",	{Qdebug},	0,		0666,
};
#define NINCON	(sizeof incondir/sizeof incondir[0])

static void
reset(int dev)				/* hardware reset */
{
	outb(dev+Qpcr, Finitbar);
	outb(dev+Qpcr, 0);
	outb(dev+Qpcr, Finitbar);
}

static void
wrctl(int dev, int data)		/* write control character */
{
	outb(dev+Qpcr, Finitbar);	/* no interrupts, please */
	outb(dev+Qdlr, data);
	outb(dev+Qpcr, Finitbar|Fstrobe);
	outb(dev+Qpcr, Finitbar|Fie);
}

static void
wrdata(int dev, int data)		/* write data character */
{
	outb(dev+Qpcr, Finitbar|Fsi);
	outb(dev+Qdlr, data);
	outb(dev+Qpcr, Finitbar|Fsi|Fstrobe);
	outb(dev+Qpcr, Finitbar|Fsi|Fie);
}

static void
wrcmd(int dev, int data)		/* write command */
{
	outb(dev+Qpcr, Finitbar|Faf);
	outb(dev+Qdlr, data);
	outb(dev+Qpcr, Finitbar|Faf|Fstrobe);
	outb(dev+Qpcr, Finitbar|Faf|Fie);
}

static int
rdstatus(Incon *ip)			/* read status */
{
	int dev, data;

	dev = ip->dev;

	outb(dev+Qpcr, Finitbar|Faf|Fsi);
	outb(dev+Qdlr, 0x01);
	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
	data = (inb(dev+Qpsr)&0xf8)<<2;
	outb(dev+Qpcr, Finitbar|Faf|Fsi);
	data |= inb(dev+Qpsr)>>3;
	if(data&(OVERFLOW|CRC_ERROR)){
		if(data&OVERFLOW)
			ip->overflow++;
		if(data&CRC_ERROR)
			ip->crc++;
	}
	outb(dev+Qdlr, 0x03);
	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fie);

	return data;
}

static void
iwrcmd(int dev, int data)		/* write command at interrupt level */
{
	outb(dev+Qdlr, data);
	outb(dev+Qpcr, Finitbar|Faf|Fstrobe);
	outb(dev+Qpcr, Finitbar|Faf);
}

static int
irdstatus(Incon *ip)			/* read status at interrupt level */
{
	int dev, data;

	dev = ip->dev;

	outb(dev+Qdlr, 0x01);
	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
	data = (inb(dev+Qpsr)&0xf8)<<2;
	outb(dev+Qpcr, Finitbar|Faf|Fsi);
	data |= inb(dev+Qpsr)>>3;
	if(data&(OVERFLOW|CRC_ERROR)){
		if(data&OVERFLOW)
			ip->overflow++;
		if(data&CRC_ERROR)
			ip->crc++;
	}

	return data;
}

static int
irddata(int dev)			/* read data at interrupt level */
{
	int data;

	outb(dev+Qdlr, 0x00);
	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
	data = (inb(dev+Qpsr)&0xf8)<<2;
	outb(dev+Qpcr, Finitbar|Faf|Fsi);
	data |= inb(dev+Qpsr)>>3;

	return data;
}

static int
irdnext(int dev)			/* read next data at interrupt level */
{
	int data;

	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
	data = (inb(dev+Qpsr)&0xf8)<<2;
	outb(dev+Qpcr, Finitbar|Faf|Fsi);
	data |= inb(dev+Qpsr)>>3;

	return data;
}

static void
irdnop(int dev, int pcr)		/* read nop (mux reset) */
{
	outb(dev+Qdlr, 0x03);
	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
	outb(dev+Qpcr, pcr);
}

int		Irdnext(int);
#define	irdnext	Irdnext

/*
 *  set the incon parameters
 */
void
inconset(Incon *ip, int cnt, int del)
{
	int dev;

	if (cnt<1 || cnt>14 || del<1 || del>15)
		error(Ebadarg);

	dev = ip->dev;
	wrcmd(dev, sel_rcv_cnt | INCON_RUN);
	wrdata(dev, cnt);
	wrcmd(dev, sel_rcv_tim | INCON_RUN);
	wrdata(dev, del);
	wrcmd(dev, INCON_RUN | ENABLE_IRQ);
}

/*
 *  parse a set request
 */
void
inconsetctl(Incon *ip, Block *bp)
{
	char *field[3];
	int n;
	int del;
	int cnt;

	del = 15;
	n = getfields((char *)bp->rptr, field, 3, ' ');
	switch(n){
	default:
		freeb(bp);
		error(Ebadarg);
	case 2:
		del = strtol(field[1], 0, 0);
		if(del<0 || del>15){
			freeb(bp);
			error(Ebadarg);
		}
		/* fall through */
	case 1:
		cnt = strtol(field[0], 0, 0);
		if(cnt<0 || cnt>15){
			freeb(bp);
			error(Ebadarg);
		}
	}
	inconset(ip, cnt, del);
	freeb(bp);
}

static void
nop(void)
{
}

/*
 *  poll for a station number
 */
void 
inconpoll(Incon *ip, int station)
{
	ulong timer;
	int dev;
	char *p;

	dev = ip->dev;

	/*
	 *  get us to a known state
	 */
	ip->state = Notliving;
	wrcmd(dev, INCON_STOP);

	/*
	 * try a station number
	 */
	wrcmd(dev, sel_station);
	wrdata(dev, station);
	wrcmd(dev, sel_poll0);
	wrdata(dev, HT_GNOT);
	wrcmd(dev, sel_rcv_cnt);
	wrdata(dev, 3);
	wrcmd(dev, sel_rcv_tim);
	wrdata(dev, 15);
	wrcmd(dev, sel_tx_cnt);
	wrdata(dev, 1);
	wrcmd(dev, sel_polln);
	wrdata(dev, 0x00);
	wrdata(dev, ST_UNIX);
	wrdata(dev, NCHAN);
	for(p = "gnot"; *p; p++)
		wrdata(dev, *p);
	wrctl(dev, 0);

	/*
	 *  poll and wait for ready (or 1/4 second)
	 */
	ip->state = Selecting;
	wrcmd(dev, INCON_RUN | ENABLE_IRQ);
	timer = NOW + 250;
	while (NOW < timer) {
		nop();
		if(rdstatus(ip) & INCON_ALIVE){
			ip->station = station;
			ip->state = Selected;
			break;
		}
	}
}

/*
 *  reset the chip and find a new station number
 */
void
inconrestart(Incon *ip)
{
	int i;

	if(!canqlock(&ip->reslock))
		return;

	/*
	 *  poll for incon station numbers
	 */
	DPRINT("inconrestart\n");
	for(i = Minstation; i <= Maxstation; i++){
		inconpoll(ip, i);
		if(ip->state == Selected)
			break;
	}
	switch(ip->state) {
	case Selecting:
		DPRINT("incon[%d] not polled\n", ip-incon);
		print("incon[%d] not polled\n", ip-incon);
		break;
	case Selected:
		DPRINT("incon[%d] station %d\n", ip-incon, ip->station);
		print("incon[%d] station %d\n", ip-incon, ip->station);
		inconset(ip, 3, 15);
		break;
	default:
		DPRINT("incon[%d] bollixed\n", ip-incon);
		print("incon[%d] bollixed\n", ip-incon);
		break;
	}
	qunlock(&ip->reslock);
}

/*
 *  reset all incon chips.
 */

void
inconreset(void)
{
	int i;

	/*
	 * state is Selected if we used incon as the boot device
	 * i.e. we've already got a station number.
	 * state is Selecting if this is a first-time open.
	 */
/*	inconset(&incon[0], 3, 15); /**/
	for(i=0; i<Nincon; i++){
		incon[i].dev = lptinfo[incondev].base;
		incon[i].state = Notliving;
		reset(incon[i].dev);
		incon[i].ri = incon[i].wi = 0;
	}
	incon[0].state = Selecting;
	wrcmd(incon[0].dev, INCON_STOP);
}

void
inconinit(void)
{
	wrcmd(incon[0].dev, INCON_STOP);
}

/*
 *  enable the device for interrupts, spec is the device number
 */
Chan*
inconattach(char *spec)
{
	Incon *ip;
	int i;
	Chan *c;

	setvec(lptinfo[incondev].ivec, inconintr);
	i = strtoul(spec, 0, 0);
	if(i >= Nincon)
		error(Ebadarg);
	ip = &incon[i];
	rdstatus(ip);
	if(ip->state != Selected)
		inconrestart(ip);

	c = devattach('i', spec);
	c->dev = i;
	c->qid.path = CHDIR;
	c->qid.vers = 0;
	return c;
}

Chan*
inconclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
inconwalk(Chan *c, char *name)
{
	return devwalk(c, name, incondir, NINCON, streamgen);
}

void	 
inconstat(Chan *c, char *dp)
{
	devstat(c, dp, incondir, NINCON, streamgen);
}

Chan*
inconopen(Chan *c, int omode)
{
	if(c->qid.path != CHDIR && c->qid.path >= Slowqid)
		streamopen(c, &inconinfo);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void	 
inconcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
inconclose(Chan *c)
{
	if(c->qid.path != CHDIR && c->qid.path >= Slowqid)
		streamclose(c);
}

long	 
inconread(Chan *c, void *buf, long n, ulong offset)
{
	char b[256];
	Incon *ip;

	if(c->qid.path == CHDIR)
		return devdirread(c, buf, n, incondir, NINCON, streamgen);
	ip = &incon[c->dev];
	switch(c->qid.path){
	default:
		return streamread(c, buf, n);
	case Qdlr:
	case Qpsr:
	case Qpcr:
		sprint(b, "0x%2.2ux\n", inb(ip->dev + c->qid.path));
		break;
	case Qstats:
		sprint(b, "state: %d\nstation: %d\nin: %d\nout: %d\noverflow: %d\ncrc: %d\nwait: %d\n",
			ip->state, ip->station, ip->in, ip->out, ip->overflow, ip->crc, ip->wait);
		break;
	case Qdebug:
		sprint(b, "%d\n", incondebug);
		break;
	}
	return readstr(offset, buf, n, b);
}

long	 
inconwrite(Chan *c, void *buf, long n, ulong offset)
{
	char b[8];
	Incon *ip;
	int data;

	USED(offset);
	switch(c->qid.path){
	default:
		return streamwrite(c, buf, n, 0);
	case Qdlr:
	case Qpcr:
	case Qdebug:
		break;
	}
	if(n > sizeof b-1)
		n = sizeof b-1;
	memmove(b, buf, n);
	b[n] = 0;
	data = strtoul(b, 0, 0);
	switch(c->qid.path){
	default:
		ip = &incon[c->dev];
		outb(ip->dev + c->qid.path, data);
		break;
	case Qdebug:
		incondebug = data;
		break;
	}
	return n;
}

void	 
inconremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
inconwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

/*
 *	the stream routines
 */

/*
 *  kill off the kernel process
 */
static int
kNotliving(void *arg)
{
	Incon *ip;

	ip = (Incon *)arg;
	return ip->kstarted == 0;
}

static void
inconstclose(Queue * q)
{
	Incon *ip;

	ip = (Incon *)q->ptr;
	qlock(ip);
	ip->rq = 0;
	qunlock(ip);
	wakeup(&ip->kr);
	sleep(&ip->r, kNotliving, ip);
}

/*
 *  create the kernel process for input
 *  first wait for any old ones to die
 */
static void
inconstopen(Queue *q, Stream *s)
{
	Incon *ip;
	char name[32];

	ip = &incon[s->dev];
	sprint(name, "incon%d", s->dev);
	q->ptr = q->other->ptr = ip;
	wakeup(&ip->kr);
	sleep(&ip->r, kNotliving, ip);
	ip->rq = q;
	kproc(name, inconkproc, ip);
}

/*
 *  free all blocks of a message in `q', `bp' is the first block
 *  of the message
 */
static void
freemsg(Queue *q, Block *bp)
{
	for(; bp; bp = getq(q)){
		if(bp->flags & S_DELIM){
			freeb(bp);
			return;
		}
		freeb(bp);
	}
}

static void
showpkt(char *str, int chan, int ctl, Block *bp, int i)
{
	int n;

	n = bp->wptr - bp->rptr;
	kprint("%s(%d)%uo %d", str, chan, ctl, n);
	if(n > 8)
		n = 8;
	for(; i<n; i++)
		kprint(" %2.2ux", bp->rptr[i]);
	kprint("\n");
}

/*
 *  output a block
 *
 *  the first 2 bytes of every message are the channel number,
 *  low order byte first.  the third is a possible trailing control
 *  character.
 */
void
inconoput(Queue *q, Block *bp)
{
	int dev;
	Incon *ip;
	ulong end;
	int status, chan, ctl;
	int n, size;

	ip = (Incon *)q->ptr;

	if(bp->type != M_DATA){
		if(streamparse("inconset", bp))
			inconsetctl(ip, bp);
		else
			freeb(bp);
		return;
	}
	if(BLEN(bp) < 3){
		bp = pullup(bp, 3);
		if(bp == 0){
			print("inconoput pullup failed\n");
			return;
		}
	}

	/*
	 *  get a whole message before handing bytes to the device
	 */
	if(!putq(q, bp))
		return;

	/*
	 *  one transmitter at a time
	 */
	qlock(&ip->xmit);
	dev = ip->dev;

	/*
	 *  parse message
	 */
	bp = getq(q);
	chan = bp->rptr[0] | (bp->rptr[1]<<8);
	ctl = bp->rptr[2];
	bp->rptr += 3;
	if(chan<=0)
		print("bad channel %d\n", chan);

	if(incondebug){
		kprint("0x%.2ux ", rdstatus(ip));
		showpkt("->", chan, ctl, bp, 0);
	}

	/*
	 *  make sure there's an incon out there
	 */
	if(!(rdstatus(ip)&INCON_ALIVE) || ip->state==Notliving){
		DPRINT("inconoput: not ready");
		inconrestart(ip);
		freemsg(q, bp);
		qunlock(&ip->xmit);
		return;
	}

	/*
	 *  send the 8 bit data
	 */
	for(;;){
		/*
		 *  spin till there is room
		 */
		for(n=0, end = NOW+1000; (status=rdstatus(ip)) & TX_FULL; n++){
			nop();	/* make sure we don't optimize too much */
			if(NOW > end){
				print("incon output stuck 0 %.2ux\n", status);
				freemsg(q, bp);
				qunlock(&ip->xmit);
				return;
			}
		}
		ip->wait = (n + ip->wait)>>1;

		/*
		 *  put in next packet
		 */
		n = bp->wptr - bp->rptr;
		if(n > 16)
			n = 16;
		size = n;
		wrctl(dev, chan);
		ip->out += n;
		while(n--){
			wrdata(dev, *bp->rptr++);
		}

		/*
		 *  get next block 
		 */
		if(bp->rptr >= bp->wptr){
			if(bp->flags & S_DELIM){
				freeb(bp);
				break;
			}
			freeb(bp);
			bp = getq(q);
			if(bp==0)
				break;
		}

		/*
		 *  end packet
		 */
		wrctl(dev, 0);
	}

	/*
	 *  send the control byte if there is one
	 */
	if(ctl){
		if(size >= 16){
			wrctl(dev, 0);
			for(end = NOW+1000; rdstatus(ip) & TX_FULL;){
				nop();	/* make sure we don't optimize too much */
				if(NOW > end){
					print("incon output stuck 1\n");
					freemsg(q, bp);
					qunlock(&ip->xmit);
					return;
				}
			}
			wrctl(dev, chan);
		}
		if(rdstatus(ip) & TX_FULL)
			print("inconfull\n");
		ip->out += 1;
		wrctl(dev, ctl);
	}
	wrctl(dev, 0);

	qunlock(&ip->xmit);
	return;
}

/*
 *  return true if the raw fifo is non-empty
 */
static int
notempty(void *arg)
{
	Incon *ip;

	ip = (Incon *)arg;
	return ip->ri!=ip->wi;
}

/*
 *  Read bytes from the raw input circular buffer.
 */
static void
inconkproc(void *arg)
{
	Incon *ip;
	Block *bp;
	int i;
	int locked;

	ip = (Incon *)arg;
	ip->kstarted = 1;

	/*
	 *  create a number of blocks for input
	 */
	for(i = 0; i < Nin; i++){
		bp = ip->inb[i] = allocb(Bsize);
		bp->wptr += 3;
	}

	locked = 0;
	if(waserror()){
		if(locked)
			qunlock(ip);
		ip->kstarted = 0;
		wakeup(&ip->r);
		return;
	}

	for(;;){
		/*
		 *  sleep if input fifo empty
		 */
		sleep(&ip->kr, notempty, ip);

		/*
		 *  die if the device is closed
		 */
		USED(locked);
		qlock(ip);
		locked = 1;
		if(ip->rq == 0){
			qunlock(ip);
			ip->kstarted = 0;
			wakeup(&ip->r);
			poperror();
			return;
		}

		/*
		 *  send blocks upstream and stage new blocks.
		 */
		while(ip->ri != ip->wi){
			bp = ip->inb[ip->ri];
			bp->flags |= S_DELIM;
			ip->in += BLEN(bp);
			PUTNEXT(ip->rq, bp);
			bp = ip->inb[ip->ri] = allocb(Bsize);
			bp->wptr += 3;
			ip->ri = (ip->ri+1)%Nin;
		}
		USED(locked);
		qunlock(ip);
		locked = 0;
	}
}

/*
 *  drop a single packet
 */
static void
droppacket(int dev)
{
	if(irddata(dev) == 0)
		return;
	while(irdnext(dev))
		;
}

/*
 *  flush the input fifo
 */
static void
flushfifo(Incon *ip)
{
	while(!(irdstatus(ip) & RCV_EMPTY))
		droppacket(ip->dev);
}

/*
 *  advance the queue. if we've run out of staged input blocks,
 *  drop the packet and return 0.  otherwise return the next input
 *  block to fill.
 */
static Block *
nextin(Incon *ip, unsigned int c)
{
	Block *bp;
	int next;

	bp = ip->inb[ip->wi];
	bp->rptr[0] = ip->chan;
	bp->rptr[1] = ip->chan>>8;
	bp->rptr[2] = c;
	if(incondebug)
		showpkt("<-", ip->chan, c, bp, 3);

	next = (ip->wi+1)%Nin;
	if(next == ip->ri){
		bp->wptr = bp->rptr+3;
		return bp;
	}
	ip->wi = next;

	return ip->inb[ip->wi];
}

/*
 *  read the packets from the device into the staged input blocks.
 *  we have to do this at interrupt tevel to turn off the interrupts.
 */
static void
rdpackets(Incon *ip)
{
	Block *bp;
	unsigned int c;
	int dev;
	uchar *p;
	int first = ip->wi;

	dev = ip->dev;
	bp = ip->inb[ip->wi];
	if(bp==0){
		flushfifo(ip);
		return;
	}
	p = bp->wptr;
	while(!(irdstatus(ip) & RCV_EMPTY)){
		/*
		 *  get channel number
		 */
		c = irddata(dev);
		if(c == 0){
			droppacket(dev);
			continue;
		}
		if(ip->chan != c){
			if(p - bp->rptr > 3){
				bp->wptr = p;
				bp = nextin(ip, 0);
				p = bp->wptr;
			}
			ip->chan = c;
		}

		/*
		 *  null byte marks end of packet
		 */
		while(c = irdnext(dev)){	/* assign = */
			if((c & 0x100) == 0){
				/*
				 *  control byte ends block
				 */
				bp->wptr = p;
				bp = nextin(ip, c);
				p = bp->wptr;
			}else{
				/*
				 *  data byte, put in local buffer
				 */
				*p++ = c;
			}
		}

		/*
		 *  pass a block on if it doesn't have room for one more
		 *  packet.  this way we don't have to check per byte.
		 */
		if(p + 16 > bp->lim){
			bp->wptr = p;
			bp = nextin(ip, 0);
			p = bp->wptr;
		}
	}	
	bp->wptr = p;
	if(bp->wptr != bp->rptr+3)
		nextin(ip, 0);

	if(first != ip->wi)/**/
		wakeup(&ip->kr);
}

/*
 *  Receive an incon interrupt.  One entry point
 *  for all types of interrupt.  Until we figure out
 *  how to use more than one incon, this routine only
 *  is for incon[0].
 */
static void
inconintr(Ureg *ur)
{
	uchar status;
	int pcr;
	Incon *ip;

	USED(ur);
	ip = &incon[0];
	pcr = inb(ip->dev+Qpcr);
	status = irdstatus(ip);
	if(incondebug){
		kprint("pcr=%2.2x status=%2.2x\n", pcr, status);
		if(pcr & Fstrobe)
			kprint("YIKES!!\n");
	}
	if(!(status & RCV_EMPTY))
		rdpackets(ip);

	/* see if it died underneath us */
	if(!(status&INCON_ALIVE)){
		switch(ip->state){
		case Selected:
			iwrcmd(ip->dev, INCON_STOP);
			DPRINT("Incon died\n");
			print("Incon died\n");
			break;
		case Selecting:
			DPRINT("rejected\n");
			print("rejected\n");
			break;
		default:
			iwrcmd(ip->dev, INCON_STOP);
			DPRINT("state was %d\n", ip->state);
			break;
		}
		ip->state = Notliving;
	}
	irdnop(ip->dev, pcr);
}
