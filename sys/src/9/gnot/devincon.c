#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"
#include	"ureg.h"

typedef struct Incon	Incon;
typedef struct Device	Device;

#define NOW (MACHP(0)->ticks*MS2HZ)

#define MICROSECOND USED(NOW)

#define DPRINT if(0)

enum {
	Minstation=	2,	/* lowest station # to poll */
	Maxstation=	15,	/* highest station # to poll */
	Nincon=		1,	/* number of incons */
	Nin=		32,	/* Blocks in the input ring */
	Bsize=		128,	/* size of an input ring block */
	Mfifo=		0xff,	/* a mask, must be 2^n-1, must be > Nin */

	Qstats=		1,	/* qid of the statistics file */
};

/*
 *  incon datakit board
 */
struct Device {
	uchar	cdata;
#define	cpolln	cdata
	uchar	u0;
	uchar	cstatus;
	uchar	u1;
	uchar	creset;
	uchar	u2;
	uchar	csend;
	uchar	u3;
	ushort	data_cntl;	/* data is high byte, cntl is low byte */
	uchar	status;
#define cmd	status
	uchar	u5;
	uchar	reset;
	uchar	u6;
	uchar	send;
	uchar	u7;
};
#define	INCON	((Device *)0x40700000)

struct Incon {
	QLock;

	QLock	xmit;		/* transmit lock */
	QLock	reslock;	/* reset lock */
	Device	*dev;
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
#define ST_UNIX 0x04
#define NCHAN 16

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

int incondebug;

Dirtab incondir[]={
	"stats",		{Qstats},	0,		0444,
};

/*
 *  set the incon parameters
 */
void
inconset(Incon *ip, int cnt, int del)
{
	Device *dev;

	if (cnt<1 || cnt>14 || del<1 || del>15)
		error(Ebadarg);

	dev = ip->dev;
	dev->cmd = sel_rcv_cnt | INCON_RUN;
	MICROSECOND;
	*(uchar *)&dev->data_cntl = cnt;
	MICROSECOND;
	dev->cmd = sel_rcv_tim | INCON_RUN;
	MICROSECOND;
	*(uchar *)&dev->data_cntl = del;
	MICROSECOND;
	dev->cmd = INCON_RUN | ENABLE_IRQ;
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
	n = getfields((char *)bp->rptr, field, 3, " ");
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
	Device *dev;

	dev = ip->dev;

	/*
	 *  get us to a known state
	 */
	ip->state = Notliving;
	dev->cmd = INCON_STOP;

	/*
	 * try a station number
	 */
	dev->cmd = sel_station;
	*(uchar *)&dev->data_cntl = station;
	dev->cmd = sel_poll0;
	*(uchar *)&dev->data_cntl = HT_GNOT;
	dev->cmd = sel_rcv_cnt;
	*(uchar *)&dev->data_cntl = 3;
	dev->cmd = sel_rcv_tim;
	*(uchar *)&dev->data_cntl = 15;
	dev->cmd = sel_tx_cnt;
	*(uchar *)&dev->data_cntl = 1;
	dev->cmd = sel_polln;
	*(uchar *)&dev->data_cntl = 0x00;
	*(uchar *)&dev->data_cntl = ST_UNIX;
	*(uchar *)&dev->data_cntl = NCHAN;
	*(uchar *)&dev->data_cntl = 'g';
	*(uchar *)&dev->data_cntl = 'n';
	*(uchar *)&dev->data_cntl = 'o';
	*(uchar *)&dev->data_cntl = 't';
	dev->cpolln = 0;

	/*
	 *  poll and wait for ready (or 1/4 second)
	 */
	ip->state = Selecting;
	dev->cmd = INCON_RUN | ENABLE_IRQ;
	timer = NOW + 250;
	while (NOW < timer) {
		nop();
		if(dev->status & INCON_ALIVE){
			ip->station = station;
			ip->state = Selected;
			break;
		}
	}
}

/*
 *  reset the chip and find a new staion number
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
	for(i = Minstation; i <= Maxstation; i++){
		inconpoll(ip, i);
		if(ip->state == Selected)
			break;
	}
	switch(ip->state) {
	case Selecting:
		print("incon[%d] not polled\n", ip-incon);
		break;
	case Selected:
		print("incon[%d] station %d\n", ip-incon, ip->station);
		inconset(ip, 3, 15);
		break;
	default:
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

	incon[0].dev = INCON;
	incon[0].state = Selected;
	incon[0].ri = incon[0].wi = 0;
/*	inconset(&incon[0], 3, 15); /**/
	for(i=1; i<Nincon; i++){
		incon[i].dev = INCON+i;
		incon[i].state = Notliving;
		incon[i].dev->cmd = INCON_STOP;
		incon[i].ri = incon[i].wi = 0;
	}
}

void
inconinit(void)
{
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

	i = strtoul(spec, 0, 0);
	if(i >= Nincon)
		error(Ebadarg);
	ip = &incon[i];
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
	return devwalk(c, name, incondir, 1, streamgen);
}

void	 
inconstat(Chan *c, char *dp)
{
	devstat(c, dp, incondir, 1, streamgen);
}

Chan*
inconopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR || c->qid.path == Qstats){
		if(omode != OREAD)
			error(Eperm);
	}else
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
	if(c->qid.path != CHDIR)
		streamclose(c);
}

long	 
inconread(Chan *c, void *buf, long n, ulong offset)
{
	char b[256];
	Incon *i;

	if(c->qid.path == CHDIR)
		return devdirread(c, buf, n, incondir, 1, streamgen);
	else if(c->qid.path == Qstats){
		i = &incon[c->dev];
		sprint(b, "in: %d\nout: %d\noverflow: %d\ncrc: %d\nwait: %d\n", i->in,
			i->out, i->overflow, i->crc, i->wait);
		return readstr(offset, buf, n, b);
	} else
		return streamread(c, buf, n);
}

long	 
inconwrite(Chan *c, void *buf, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, buf, n, 0);
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
 * for sleeping while kproc dies
 */
static int
kNotliving(void *arg)
{
	Incon *ip;

	ip = (Incon *)arg;
	return ip->kstarted == 0;
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
 *  kill off the kernel process
 */
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
	Device *dev;
	Incon *ip;
	ulong start, end;
	int chan;
	int ctl;
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

	if(incondebug)
		print("->(%d)%uo %d\n", chan, ctl, bp->wptr - bp->rptr);

	/*
	 *  make sure there's an incon out there
	 */
	if(!(dev->status&INCON_ALIVE) || ip->state==Notliving){
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
		start = NOW;
		for(n = 0, end = start+1000; dev->status & TX_FULL; n++){
			nop();	/* make sure we don't optimize too much */
			if(NOW > end){
				print("incon output stuck\n");
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
		dev->cdata = chan;
		ip->out += n;
		while(n--){
			*(uchar *)&dev->data_cntl = *bp->rptr++;
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
		dev->cdata = 0;
	}

	/*
	 *  send the control byte if there is one
	 */
	if(ctl){
		if(size >= 16){
			dev->cdata = 0;
			MICROSECOND;
			for(end = NOW+1000; dev->status & TX_FULL;){
				nop();	/* make sure we don't optimize too much */
				if(NOW > end){
					print("incon output stuck\n");
					freemsg(q, bp);
					qunlock(&ip->xmit);
					return;
				}
			}
			dev->cdata = chan;
			MICROSECOND;
		}
		if(dev->status & TX_FULL)
			print("inconfull\n");
		dev->cdata = ctl;
	}
	MICROSECOND;
	dev->cdata = 0;

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
		 *  send blocks upstream and stage new blocks.  if the block is small
		 *  (< 64 bytes) copy into a smaller buffer.
		 */
		while(ip->ri != ip->wi){
			bp = ip->inb[ip->ri];
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
droppacket(Device *dev)
{
	int i;
	int c;

	for(i = 0; i < 17; i++){
		c = dev->data_cntl;
		if(c==0)
			break;
	}
}

/*
 *  flush the input fifo
 */
static void
flushfifo(Device *dev)
{
	while(!(dev->status & RCV_EMPTY))
		droppacket(dev);
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
		print("<-(%d)%uo %d\n", ip->chan, c, bp->wptr-bp->rptr);

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
	Device *dev;
	uchar *p;
	int first = ip->wi;

	dev = ip->dev;
	bp = ip->inb[ip->wi];
	if(bp==0){
		flushfifo(ip->dev);
		return;
	}
	p = bp->wptr;
	while(!(dev->status & RCV_EMPTY)){
		/*
		 *  get channel number
		 */
		c = (dev->data_cntl)>>8;
		if(c == 0){
			droppacket(dev);
			continue;
		}
		if(ip->chan != c){
			if(p - bp->rptr > 3){
				bp->wptr = p;
				bp = nextin(ip, 0);
				if(bp==0){
					flushfifo(ip->dev);
					return;
				}
				p = bp->wptr;
			}
			ip->chan = c;
		}

		/*
		 *  null byte marks end of packet
		 */
		for(;;){
			if((c=dev->data_cntl)&1) {
				/*
				 *  data byte, put in local buffer
				 */
				*p++ = c>>8;
			} else if (c>>=8) {
				/*
				 *  control byte ends block
				 */
				bp->wptr = p;
				bp = nextin(ip, c);
				if(bp==0){
					flushfifo(ip->dev);
					return;
				}
				p = bp->wptr;
			} else {
				/* end of packet */
				break;
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
void
inconintr(Ureg *ur)
{
	uchar status;
	Incon *ip;

	USED(ur);
	ip = &incon[0];

	status = ip->dev->status;
	if(!(status & RCV_EMPTY))
		rdpackets(ip);

	/* check for exceptional conditions */
	if(status&(OVERFLOW|CRC_ERROR)){
		if(status&OVERFLOW)
			ip->overflow++;
		if(status&CRC_ERROR)
			ip->crc++;
	}

	/* see if it died underneath us */
	if(!(status&INCON_ALIVE)){
		switch(ip->state){
		case Selected:
			ip->dev->cmd = INCON_STOP;
			print("Incon died\n");
			break;
		case Selecting:
			print("rejected\n");
			break;
		default:
			ip->dev->cmd = INCON_STOP;
			break;
		}
		ip->state = Notliving;
	}
}

void
incontoggle(void)
{
	incondebug ^= 1;
}
