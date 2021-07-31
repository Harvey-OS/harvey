#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"

typedef struct Hs	Hs;

enum {
	Maxburst=	1023,		/* maximum transmit burst size */
	Intvec=		0xd0,		/* vme vector for interrupts */
	Intlevel=	5,		/* level to interrupt on */
	Nhs=		1,

	/*
	 * csr flags
	 */
	ALIVE		= 0x0001,
	IENABLE		= 0x0004,
	EXOFLOW		= 0x0008,
	IRQ		= 0x0010,
	EMUTE		= 0x0020,
	EPARITY		= 0x0040,
	EFRAME		= 0x0080,
	EROFLOW		= 0x0100,
	REF		= 0x0800,
	XFF		= 0x4000,
	XHF		= 0x8000,

	/*
	 * csr reset flags
	 */
	FORCEW		= 0x0008,
	NORESET		= 0xFF00,
	RESET		= 0x0000,

	/*
	 * data flags
	 */
	CTL		= 0x0100,
	CHNO		= 0x0200,
	TXEOD		= 0x0400,
	NND		= 0x8000,
};

#define NOW (MACHP(0)->ticks*MS2HZ)
#define IPL(x)		((x)<<5)

struct Hs {
	QLock;

	QLock	xmit;
	HSdev	*addr;
	int	vec;		/* interupt vector */
	Rendez	r;		/* output process */
	Rendez	kr;		/* input kernel process */
	int	chan;		/* current input channel */
	Queue	*rq;		/* read queue */
	uchar	buf[1024];	/* bytes being collected */
	uchar	*wptr;		/* pointer into buf */
	ulong	time;		/* time byte in buf[0] arrived */
	int	kstarted;	/* true if kernel process started */
	int	started;

	/* statistics */

	ulong	parity;		/* parity errors */
	ulong	rintr;		/* rcv interrupts */
	ulong	tintr;		/* transmit interrupts */
	ulong	in;		/* bytes in */
	ulong	out;		/* bytes out */
};

Hs hs[Nhs];

static void hsintr(int);
static void hskproc(void*);

/*
 *  hs stream module definition
 */
static void hsoput(Queue*, Block*);
static void hsstopen(Queue*, Stream*);
static void hsstclose(Queue*);
Qinfo hsinfo =
{
	nullput,
	hsoput,
	hsstopen,
	hsstclose,
	"hs"
};

void
hsrestart(Hs *hp)
{
	HSdev *addr;

	addr = hp->addr;

	addr->csr = RESET;
	wbflush();
	delay(20);

	/*
	 *  set interrupt vector
	 *  turn on device
	 *  set forcew to a known value
	 *  interrupt on level `Intlevel'
	 */
	addr->vector = hp->vec;
	addr->csr = NORESET|IPL(Intlevel)|IENABLE|ALIVE;
	wbflush();
	delay(1);
	addr->csr = NORESET|IPL(Intlevel)|FORCEW|IENABLE|ALIVE;
	wbflush();
	delay(1);
	hp->started = 1;
}

/*
 *  reset all vme boards
 */
void
hsreset(void)
{
	int i;
	Hs *hp;

	for(i=0; i<Nhs; i++){
		hp = &hs[i];
		hp->addr = HSDEV+i;
		hp->vec = Intvec+i;
		hp->addr->csr = RESET;
		setvmevec(hp->vec, hsintr);
	}	
	wbflush();
	delay(20);
}

void
hsinit(void)
{
}

/*
 *  enable the device for interrupts, spec is the device number
 */
Chan*
hsattach(char *spec)
{
	Hs *hp;
	int i;
	Chan *c;

	i = strtoul(spec, 0, 0);
	if(i >= Nhs)
		error(Ebadarg);
	hp = &hs[i];
	if(!hp->started)
		hsrestart(hp);

	c = devattach('h', spec);
	c->dev = i;
	c->qid.path = CHDIR;
	return c;
}

Chan*
hsclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
hswalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, streamgen);
}

void	 
hsstat(Chan *c, char *dp)
{
	devstat(c, dp, 0, 0, streamgen);
}

Chan*
hsopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}else
		streamopen(c, &hsinfo);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void	 
hscreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c);
	USED(name);
	USED(omode);
	USED(perm);
	error(Eperm);
}

void	 
hsclose(Chan *c)
{
	if(c->qid.path != CHDIR)
		streamclose(c);
}

long	 
hsread(Chan *c, void *buf, long n, ulong offset)
{
	USED(offset);
	return streamread(c, buf, n);
}

long	 
hswrite(Chan *c, void *buf, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, buf, n, 0);
}

void	 
hsremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
hswstat(Chan *c, char *dp)
{
	USED(c);
	USED(dp);
	error(Eperm);
}

/*
 *	the stream routines
 */

/*
 *  create the kernel process for input
 */
static void
hsstopen(Queue *q, Stream *s)
{
	Hs *hp;
	char name[32];

	hp = &hs[s->dev];
	sprint(name, "hs%d", s->dev);
	q->ptr = q->other->ptr = hp;
	hp->rq = q;
	kproc(name, hskproc, hp);
}

/*
 *  ask kproc to die and wait till it happens
 */
static int
kdead(void *arg)
{
	Hs *hp;

	hp = (Hs *)arg;
	return hp->kstarted == 0;
}
static void
hsstclose(Queue * q)
{
	Hs *hp;

	hp = (Hs *)q->ptr;
	qlock(hp);
	hp->rq = 0;
	qunlock(hp);
	while(waserror())
		;
	while(hp->kstarted){
		wakeup(&hp->kr);
		sleep(&hp->r, kdead, hp);
	}
	poperror();
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
 *  return true if the output fifo is at least half empty.
 *  the implication is that it can take at least another 1000 byte burst.
 */
static int
halfempty(void *arg)
{
	HSdev *addr;

	addr = (HSdev*)arg;
	return addr->csr & XHF;
}

/*
 *  output a block
 *
 *  the first 2 bytes of every message are the channel number,
 *  low order byte first.  the third is a possible trailing control
 *  character.
 */
void
hsoput(Queue *q, Block *bp)
{
	HSdev *addr;
	Hs *hp;
	int burst;
	int chan;
	int ctl;
	int n;

	if(bp->type != M_DATA){
		freeb(bp);
		return;
	}
	if(BLEN(bp) < 3){
		bp = pullup(bp, 3);
		if(bp == 0){
			print("hsoput pullup failed\n");
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
	hp = (Hs *)q->ptr;
	qlock(&hp->xmit);
	if(waserror()){
		qunlock(&hp->xmit);
		nexterror();
	}
	addr = hp->addr;

	/*
	 *  parse message
	 */
	bp = getq(q);
	chan = CHNO | bp->rptr[0] | (bp->rptr[1]<<8);
	ctl = bp->rptr[2];
	bp->rptr += 3;

	/*
	 *  send the 8 bit data, burst are up to Maxburst (9-bit) bytes long
	 */
	if(!(addr->csr & XHF))
		sleep(&hp->r, halfempty, addr);
/*	print("->%.2uo\n", CHNO|chan);/**/
	addr->data = CHNO|chan;
	burst = Maxburst;
	while(bp){
		if(burst == 0){
			addr->data = TXEOD;
/*			print("->%.2uo\n", TXEOD); /**/
			if(!(addr->csr & XHF))
				sleep(&hp->r, halfempty, addr);
/*			print("->%.2uo\n", CHNO|chan); /**/
			addr->data = CHNO|chan;
			burst = Maxburst;
		}
		n = bp->wptr - bp->rptr;
		if(n > burst)
			n = burst;
		burst -= n;
		while(n--){
/*			print("->%.2uo\n", *bp->rptr); /**/
			addr->data = *bp->rptr++;
		}
		if(bp->rptr >= bp->wptr){
			if(bp->flags & S_DELIM){
				freeb(bp);
				break;
			}
			freeb(bp);
			bp = getq(q);
		}
	}

	/*
	 *  send the control byte if there is one
	 */
	if(ctl){
/*		print("->%.2uo\n", CTL|ctl); /**/
		addr->data = CTL|ctl;
	}

	/*
	 *  start the fifo emptying
	 */
/*	print("->%.2uo\n", TXEOD); /**/
	addr->data = TXEOD;

	qunlock(&hp->xmit);
	poperror();
}

/*
 *  return true if the input fifo is non-empty
 */
static int
notempty(void *arg)
{
	HSdev *addr;

	addr = (HSdev *)arg;
	return addr->csr & REF;
}

/*
 *  fill a block with what is currently buffered and send it upstream
 */
static void
upstream(Hs *hp, unsigned int ctl)
{
	int n;
	Block *bp;

	n = hp->wptr - hp->buf;
	bp = allocb(3 + n);
	bp->wptr[0] = hp->chan;
	bp->wptr[1] = hp->chan>>8;
	bp->wptr[2] = ctl;
	if(n)
		memmove(&bp->wptr[3], hp->buf, n);
	bp->wptr += 3 + n;
	bp->flags |= S_DELIM;
	PUTNEXT(hp->rq, bp);
	hp->wptr = hp->buf;
	hp->time = 0;
}

/*
 *  Read bytes from the input fifo.  Since we take an interrupt every
 *  time the fifo goes non-empty, we need to waste time to let the
 *  fifo fill up.
 */
static void
hskproc(void *arg)
{
	Hs *hp;
	HSdev *addr;
	unsigned int c;
	int locked;

	hp = (Hs *)arg;
	addr = hp->addr;
	hp->kstarted = 1;
	hp->wptr = hp->buf;

	locked = 0;
	if(waserror()){
		if(locked)
			qunlock(hp);
		hp->kstarted = 0;
		wakeup(&hp->r);
		return;
	}

	for(;;){
		/*
		 *  die if the device is closed
		 */
		USED(locked);		/* so locked = 0 and locked = 1 stay */
		qlock(hp);
		locked = 1;
		if(hp->rq == 0){
			qunlock(hp);
			hp->kstarted = 0;
			wakeup(&hp->r);
			poperror();
			return;
		}

		/*
		 *  0xFFFF means an empty fifo
		 */
		while((c = addr->data) != 0xFFFF){
			hp->in++;
			if(c & CHNO){
				c &= 0x1FF;
				if(hp->chan == c)
					continue;
				/*
				 *  new channel, put anything saved upstream
				 */
				if(hp->wptr - hp->buf != 0)
					upstream(hp, 0);
				hp->chan = c;
			} else if(c & NND){
				/*
				 *  ctl byte, this ends a message
				 */
				upstream(hp, c);
			} else {
				/*
				 *  data byte, put in local buffer
				 */
				if(hp->time == 0)
					hp->time = NOW + 100;
				*hp->wptr++ = c;
				if(hp->wptr == &hp->buf[sizeof hp->buf])
					upstream(hp, 0);
			}
		}
		if(hp->time && NOW >= hp->time)
			upstream(hp, 0);
		USED(locked);
		qunlock(hp);
		locked = 0;

		/*
		 *  Sleep if input fifo empty. Make sure we don't hold onto
		 *  any byte for more than 1/10 second.
		 */
		if(!(addr->csr & REF)){
			if(hp->wptr == hp->buf)
				tsleep(&hp->kr, notempty, addr, 2000);
			else
				tsleep(&hp->kr, notempty, addr, 100);
		}
	}
}

/*
 *  only one flavor interrupt.   we have to use the less than half full
 *  and not empty bits to figure out whom to wake.
 */
static void
hsintr(int vec)
{
	ushort csr;
	Hs *hp;

	hp = &hs[vec - Intvec];
	if(hp < hs || hp > &hs[Nhs]){
		print("bad hs vec\n");
		return;
	}
	csr = hp->addr->csr;

	if(csr & REF){
		hp->rintr++;
		wakeup(&hp->kr);
	}
	if(csr & XHF){
		hp->tintr++;
		wakeup(&hp->r);
	}
	if((csr^XFF) & (XFF|EROFLOW|EFRAME|EPARITY|EXOFLOW)){
		hp->parity++;
		hsrestart(hp);
		print("hs %d: reset, csr = 0x%ux\n",
			vec - Intvec, csr);
	}
}
