#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define DPRINT 	if(asyncdebug)kprint

/*
 *  configuration
 */
enum {
	MAXFRAME=	256,	/* also known to tsm8 code */
};

/* input states */
enum
{
	Hunt,
	Framing,
	Framed,
	Data,
	Escape
};

typedef struct Async Async;
struct Async
{
	QLock;
	Async	*list;
	int	id;

	int	inuse;
	Queue	*wq;

	/* output state */
	QLock	xmit;		/* transmit lock */
	int	chan;		/* current urp channel */
	Block	*bp;		/* current output buffer */
	int	count;
	ushort	crc;

	/* input state */
	int	state;		/* input state */
	uchar	buf[MAXFRAME];	/* current input buffer */
	int	icount;
	ushort	icrc;

	/* statistics */
	ulong	chan0;
	ulong	toolong;
	ulong	tooshort;
	ulong	badcrc;
	ulong	badescape;
	ulong	in;		/* bytes in */
	ulong	out;		/* bytes out */
};

int nasync;

/* list of allocated async structures (never freed) */
struct
{
	Lock;
	Async *async;
} asyncalloc;

/*
 *  async stream module definition
 */
static void asynciput(Queue*, Block*);
static void asyncoput(Queue*, Block*);
static void asyncopen(Queue*, Stream*);
static void asyncclose(Queue*);
static void asyncreset(void);
Qinfo asyncinfo =
{
	asynciput,
	asyncoput,
	asyncopen,
	asyncclose,
	"async",
	asyncreset
};

static int	debugcount = 6;
int asyncdebug;
int asyncerror;

static ushort crc_table[256] = {
#include "../port/crc_16.h"
};

#define	BOT	0050		/* begin trailer */
#define	BOTM	0051		/* begin trailer, more data follows */
#define	BOTS	0052		/* seq update alg. on this trailer */

#define	FRAME		0x7e
#define	STUF		0x9d

#define	CRCSTART	(crc_table[0xff])
#define	CRCFUNC(crc,x)	(crc_table[((crc)^(x))&0xff]^((crc)>>8))

void
stasynclink(void)
{
	newqinfo(&asyncinfo);
}

/*
 *  create the async structures
 */
static void
asyncreset(void)
{
}

/*
 *  allocate an async structure
 */
static void
asyncopen(Queue *q, Stream *s)
{
	Async *ap;

	DPRINT("asyncopen %d\n", s->dev);

	for(ap = asyncalloc.async; ap; ap = ap->list){
		qlock(ap);
		if(ap->inuse == 0)
			break;
		qunlock(ap);
	}
	if(ap == 0){
		ap = smalloc(sizeof(Async));
		qlock(ap);
		lock(&asyncalloc);
		ap->list = asyncalloc.async;
		asyncalloc.async = ap;
		ap->id = nasync++;
		unlock(&asyncalloc);
	}
	q->ptr = q->other->ptr = ap;

	ap->inuse = 1;
	ap->bp = 0;
	ap->chan = -1;
	ap->count = 0;
	ap->toolong = 0;
	ap->tooshort = 0;
	ap->badcrc = 0;
	ap->badescape = 0;
	ap->chan0 = 0;
	ap->in = 0;
	ap->out = 0;
	ap->wq = WR(q);
	ap->state = Hunt;
	qunlock(ap);
}

static void
asyncclose(Queue * q)
{
	Async *ap = (Async *)q->ptr;

	DPRINT("asyncstclose %d\n", ap->id);
	qlock(ap);
	ap->inuse = 0;
	qunlock(ap);
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
showframe(char *t, Async *ap, uchar *buf, int n)
{
	kprint("a%d %s [", ap->id, t);
	while (--n >= 0)
		kprint(" %2.2ux", *buf++);
	kprint(" ]\n");
}

void
aswrite(Async *ap)
{
	if(ap->bp->rptr == ap->bp->wptr)
		return;
	if(asyncdebug > 2)
		showframe("out", ap, ap->bp->rptr, BLEN(ap->bp));
	FLOWCTL(ap->wq, ap->bp);
	ap->bp = 0;
	ap->count = 0;
}

void
asputf(Async *ap)
{
	uchar *p;
	int c;

	p = ap->bp->wptr;
	if(ap->count > 0) {
		if(asyncerror)
			ap->crc^=1, asyncerror=0;
		*p++ = c = ap->crc&0xff;
		if(c == FRAME)
			*p++ = 0x00;
		*p++ = c = (ap->crc>>8)&0xff;
		if(c == FRAME)
			*p++ = 0x00;
		ap->count = 0;
	}
	*p++ = FRAME;
	*p++ = FRAME;
	ap->bp->wptr = p;
	aswrite(ap);
}

void
asputc(Async *ap, int c)
{
	int d;
	uchar *p;

	if(ap->bp == 0)
		ap->bp = allocb(2*MAXFRAME+8);	/* worst-case expansion */
	p = ap->bp->wptr;
	if(ap->count <= 0) {
		*p++ = d = 0x80|((ap->chan>>5)&0x7e);
		ap->crc = CRCFUNC(CRCSTART, d);
		*p++ = d = 0x80|((ap->chan<<1)&0x7e);
		ap->crc = CRCFUNC(ap->crc, d);
	}
	*p++ = c;
	if(c == FRAME)
		*p++ = 0x00;
	ap->crc = CRCFUNC(ap->crc, c);
	ap->bp->wptr = p;
	if(++ap->count >= MAXFRAME-4)
		asputf(ap);
}

/*
 *  output a block
 *
 *  the first 2 bytes of every message are the channel number,
 *  low order byte first.  the third is a possible trailing control
 *  character.
 */
void
asyncoput(Queue *q, Block *bp)
{
	Async *ap = (Async *)q->ptr;
	int c, chan, ctl;
	Block *msg;

	if(bp->type != M_DATA){
		if(streamparse("debug", bp)){
			asyncdebug = 3;
			freeb(bp);
		} else {
			PUTNEXT(q, bp);
		}
		return;
	}

	/*
	 *  each datakit message has a 2 byte channel number followed by
	 *  one control byte
	 */
	msg = pullup(bp, 3);
	if(msg == 0){
		print("asyncoput msglen < 3\n");
		return;
	}
	chan = msg->rptr[0] | (msg->rptr[1]<<8);
	ctl = msg->rptr[2];
	msg->rptr += 3;

	qlock(&ap->xmit);
	if(waserror()){
		qunlock(&ap->xmit);
		freeb(msg);
		nexterror();
	}

	/*
	 *  new frame if the channel number has changed
	 */
	if(chan != ap->chan && ap->count > 0)
		asputf(ap);
	ap->chan = chan;

	if(asyncdebug > 1)
		kprint("a%d->(%d)%3.3uo %d\n",
			ap->id, chan, ctl, bp->wptr-bp->rptr);

	/*
	 *  send the 8 bit data
	 */
	for(bp = msg; bp; bp = bp->next){
		while (bp->rptr < bp->wptr) {
			asputc(ap, c = *bp->rptr++);
			if(c == STUF)
				asputc(ap, 0);
		}
	}

	/*
	 *  send the control byte if there is one
	 */
	if(ctl){
		asputc(ap, STUF);
		asputc(ap, ctl);
		switch (ctl) {
		case BOT:
		case BOTM:
		case BOTS:
			break;
		default:
			asputf(ap);
		}
	}
	if(debugcount > 0 && --debugcount == 0)
		asyncdebug = 1;

	freeb(msg);
	qunlock(&ap->xmit);
	poperror();
	return;
}

/*
 *  Read bytes from the raw input.
 */

void
asdeliver(Queue *q, Async *ap)
{
	int chan, c;
	Block *bp = 0;
	uchar *p = ap->buf;
	int n = ap->icount;

	chan = *p++ & 0x7e;
	chan = (chan<<5)|((*p++ & 0x7e)>>1);
	if(chan==0) {
		DPRINT("a%d deliver chan 0\n", ap->id);
		ap->chan0++;
		return;
	}
	for (n-=4; n>0; n--) {
		if(!bp) {
			bp = allocb(n+2);
			bp->flags |= S_DELIM;
			bp->wptr[0] = chan;
			bp->wptr[1] = chan>>8;
			bp->wptr[2] = 0;
			bp->wptr += 3;
		}
		if((c = *p++) == STUF) {
			--n;
			if((c = *p++) != 0) {
				bp->rptr[2] = c;
				if(asyncdebug > 1)
					kprint("a%d<-(%d)%3.3uo %d\n",
						ap->id, chan, bp->rptr[2],
						bp->wptr - bp->rptr - 3);
				PUTNEXT(q, bp);
				bp = 0;
				continue;
			} else
				c = STUF;
		}
		*bp->wptr++ = c;
	}
	if(bp) {
		if(asyncdebug > 1)
			kprint("a%d<-(%d)%3.3uo %d\n",
				ap->id, chan, bp->rptr[2],
				bp->wptr - bp->rptr - 3);
		PUTNEXT(q, bp);
	}
}

static void
asynciput(Queue *q, Block *bp)
{
	int c;
	Async *ap = q->ptr;
	int state = ap->state;

	while(bp->wptr > bp->rptr){
		c = *bp->rptr++;
		switch(state) {
		case Hunt:	/* wait for framing byte */
			if(c == FRAME)
				state = Framing;
			break;
	
		case Framing:	/* saw 1 framing byte after Hunt */
			if(c == FRAME)
				state = Framed;
			else
				state = Hunt;
			break;
	
		case Framed:	/* saw 2 or more framing bytes */
			if(c == FRAME)
				break;
			state = Data;
			ap->icrc = CRCSTART;
			ap->icount = 0;
			goto Datachar;
	
		case Data:	/* mid-frame */
			if(c == FRAME) {
				state = Escape;
				break;
			}
		Datachar:
			if(ap->icount >= MAXFRAME) {
				DPRINT("a%d pkt too long\n", ap->id);
				ap->toolong++;
				state = Hunt;
				break;
			}
			ap->icrc = CRCFUNC(ap->icrc, c);
			ap->buf[ap->icount++] = c;
			break;
	
		case Escape:	/* saw framing byte in Data */
			switch (c) {
			case FRAME:
				if(asyncdebug > 2)
					showframe("in", ap, ap->buf, ap->icount);
				if(ap->icount < 5) {
					DPRINT("a%d pkt too short\n", ap->id);
					if(asyncdebug && asyncdebug<=2)
						showframe("shortin", ap, ap->buf, ap->icount);
					ap->tooshort++;
				} else if(ap->icrc != 0) {
					DPRINT("a%d bad crc\n", ap->id);
					if(asyncdebug && asyncdebug<=2)
						showframe("badin", ap, ap->buf, ap->icount);
					ap->badcrc++;
				} else {
					asdeliver(q, ap);
				}
				state = Framed;
				break;
			case 0:
				c = FRAME;
				state = Data;
				goto Datachar;
			default:
				DPRINT("a%d bad escape\n", ap->id);
				ap->badescape++;
				state = Hunt;
				break;
			}
			break;
		}
	}
	ap->state = state;
	freeb(bp);
}
