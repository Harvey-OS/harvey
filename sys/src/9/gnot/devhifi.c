#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"
#ifdef	SET
#undef	SET
#endif
#include	"isdn.h"
#include	"hifi.h"

#define DPRINT 	if(hifidebug)kprint

#define	BSIZE	4000

typedef struct Hifichan
{
	Isdn *	udev;		/* unite interface */
	Hifi *	hdev;		/* pointer to device */
	uchar	imask;		/* interrupt bit */
	uchar	opreg;		/* sticky bits */
	uchar	tctl;		/* more sticky bits */
	uchar	devaddr;	/* debug access */

	Lock	kctl;		/* kproc start */
	Rendez	kr;		/* kernel process sleep/wakeup */
	QLock;			/* access to struct */
	int	hdlc;		/* hdlc mode enabled */
	int	hangup;		/* hangup pending */
	int	nohup;		/* hangups ignored */

	int	renable;	/* reading enabled */
	Block *	inb[NB];	/* input buffer */
	int	ri;		/* input read pointer */
	int	wi;		/* input write pointer */
	Queue *	rq;		/* read queue */

	int	wenable;	/* writing enabled */
	int	wactive;	/* writing in progress */
	QLock	wlock;		/* write's are atomic */
	Block *	outb[NB];	/* output buffer */
	int	so;		/* output scavenge pointer */
	int	ro;		/* output read pointer */
	int	wo;		/* output write pointer */
	Block *	out;		/* current output block */
	Rendez	tr;		/* if transmitter must wait */
	Rendez	cr;		/* waiting to close */

	/* statistics */

	ulong	inchars;
	ulong	badcrc;
	ulong	abort;
	ulong	overrun;
	ulong	badcount;
	ulong	overflow;
	ulong	toolong;
	ulong	underrun;
	ulong	outchars;
} Hifichan;

Hifichan *	hifichan;
Hifichan *	hifichanN;

static void	devinit(int);
static int	hifiintr(void);
static void	hifikproc(void*);
static void	hifiloopctl(Hifichan*, int);
static int	hifirecv(Hifichan*);
static void	hifirecven(Hifichan*, int);
static int	hifixmit(Hifichan*);
static void	hifixmitdis(Hifichan*, int);
static void	hifixmiten(Hifichan*, int);

enum {
	Qdir, Qdev, Qstats, Qdebug
};

Dirtab hifidir[]={
	"dev",		{Qdev},		0,	0666,
	"stats",	{Qstats},	0,	0444,
	"debug",	{Qdebug},	0,	0666,
};
#define	NHIFI	(sizeof hifidir/sizeof(Dirtab))

/*
 *  stream module definition
 */
static void hifiiput(Queue*, Block*);
static void hifioput(Queue*, Block*);
static void hifistopen(Queue*, Stream*);
static void hifistclose(Queue*);
Qinfo hifiinfo = { hifiiput, hifioput, hifistopen, hifistclose, "hifi" };

int	hifidebug;

void
hifireset(void)
{
	hifichan = xalloc(2*conf.nisdn*sizeof(Hifichan));
	hifichanN = &hifichan[2*conf.nisdn];
}

void
hifiinit(void)
{}

Chan*
hifiattach(char *spec)
{
	int dev;
	Chan *c;
	Hifichan *hp;
	static int intrinited;

	dev = strtoul(spec, 0, 0);
	if(dev >= 2*conf.nisdn)
		error(Enonexist);
		
	c = devattach('H', spec);
	c->dev = dev;
	hp = &hifichan[dev];
	if(!hp->udev){
		DPRINT("hifiattach: init dev=%d\n", c->dev);
		if(!intrinited){
			intrinited = 1;
			DPRINT("addportintr(hifiintr)\n");
			addportintr(hifiintr);
		}
		devinit(c->dev);
	}
	return c;
}

Chan*
hificlone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
hifiwalk(Chan *c, char *name)
{
	return devwalk(c, name, hifidir, NHIFI, streamgen);
}

void	 
hifistat(Chan *c, char *dp)
{
	devstat(c, dp, hifidir, NHIFI, streamgen);
}

Chan*
hifiopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}else switch(STREAMTYPE(c->qid.path)){
	case Qdev:
	case Qstats:
	case Qdebug:
		break;
	default:
		DPRINT("hifiopen dev=%d\n", c->dev);
		streamopen(c, &hifiinfo);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void	 
hificreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
hificlose(Chan *c)
{
	if(c->qid.path != CHDIR)
		streamclose(c);
}

long	 
hifiread(Chan *c, void *buf, long n, ulong offset)
{
	Hifichan *hp = &hifichan[c->dev];
	char nbuf[512], *p;

	if(n <= 0)
		return 0;
	if(c->qid.path == CHDIR){
		return devdirread(c, buf, n, hifidir, NHIFI, streamgen);
	}else switch(STREAMTYPE(c->qid.path)){
	case Qdev:
		sprint(nbuf, "0x%2.2ux = 0x%2.2ux\n",
			hp->devaddr, isdnpeek(hp->udev, (void *)hp->devaddr));
		return readstr(offset, buf, n, nbuf);

	case Qstats:
		p = nbuf;
		p += sprint(p, "inchars  %10lud\n", hp->inchars);
		p += sprint(p, "badcrc   %10lud\n", hp->badcrc);
		p += sprint(p, "abort    %10lud\n", hp->abort);
		p += sprint(p, "overrun  %10lud\n", hp->overrun);
		p += sprint(p, "badcount %10lud\n", hp->badcount);
		p += sprint(p, "overflow %10lud\n", hp->overflow);
		p += sprint(p, "toolong  %10lud\n", hp->toolong);
		p += sprint(p, "underrun %10lud\n", hp->underrun);
		p += sprint(p, "outchars %10lud\n", hp->outchars);
		USED(p);
		return readstr(offset, buf, n, nbuf);

	case Qdebug:
		sprint(nbuf, "%d\n", hifidebug);
		return readstr(offset, buf, n, nbuf);
	}
	if(!hp->renable)
		hifirecven(hp, 1);
	return streamread(c, buf, n);
}

long	 
hifiwrite(Chan *c, void *buf, long n, ulong offset)
{
	Hifichan *hp = &hifichan[c->dev];
	char nbuf[32], *p;

	USED(offset);
	if(n < 0)
		return 0;
	switch(STREAMTYPE(c->qid.path)){
	case Qdev:
		if(n>sizeof nbuf-1)
			n = sizeof nbuf-1;
		memmove(nbuf, buf, n);
		nbuf[n-1] = 0;
		hp->devaddr = (int)hp->hdev + strtoul(nbuf,0,0);
		if(p = strchr(nbuf, '='))	/* assign = */
			isdnpoke(hp->udev, (void *)hp->devaddr, strtoul(++p,0,0));
		return n;
	case Qdebug:
		if (n>sizeof nbuf)
			n = sizeof nbuf;
		memmove(nbuf, buf, n);
		nbuf[n-1] = 0;
		hifidebug = strtoul(nbuf,0,0);
		return n;
	}
	return streamwrite(c, buf, n, 0);
}

void	 
hifiremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
hifiwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

/*
 *	the stream routines
 */

#define	Predicate(name, expr)	static int name(void *arg) { return expr; }
#define	hp	((Hifichan *)arg)

Predicate(kRun, hp->ri != hp->wi || hp->so != hp->ro)

Predicate(tActive, hp->wenable)
Predicate(tDead, hp->wenable == 0)
Predicate(tEmpty, hp->ro == hp->wo)
Predicate(tFull, NEXT(hp->wo) == hp->so)
Predicate(tNotFull, NEXT(hp->wo) != hp->so)

#undef	hp

static void
hifistopen(Queue *q, Stream *s)
{
	Hifichan *hp = &hifichan[s->dev];
	char name[32];

	DPRINT("hifistopen %d\n", s->dev);
	qlock(hp);
	q->ptr = q->other->ptr = hp;
	hp->rq = q;
	if(canlock(&hp->kctl)){
		sprint(name, "hifi%d", s->dev);
		kproc(name, hifikproc, hp);
	}
	qunlock(hp);
}

static void
hifistclose(Queue * q)
{
	Hifichan *hp = (Hifichan *)q->ptr;

	DPRINT("hifistclose %d...\n", hp-hifichan);
	hifirecven(hp, -1);
	qlock(hp);
	hp->rq = 0;
	hp->hangup = 0;
	hp->nohup = 0;
	if(tActive(hp)){
		if(!hp->wactive || waserror()){
			hifixmitdis(hp, 1);
			wakeup(&hp->kr);
		}else{
			sleep(&hp->cr, tDead, hp);
			poperror();
		}
	}
	qunlock(hp);
	DPRINT("hifistclose %d OK\n", hp-hifichan);
}

/*
 *  this is only called by hangup
 */
static void
hifiiput(Queue *q, Block *bp)
{
	Hifichan *hp = (Hifichan *)q->ptr;

	if(bp->type == M_HANGUP){
		DPRINT("hifiiput %d M_HANGUP (%d)\n", hp-hifichan, hp->nohup);
		if(!hp->nohup)
			hifirecven(hp, 0);
	}
	freeb(bp);
}

void
hifioput(Queue *q, Block *bp)
{
	Hifichan *hp = (Hifichan *)q->ptr;

	if(bp->type != M_DATA){
		if(hifidebug){
			char fmt[32];
			sprint(fmt, "hifioput %%d: %%.%ds\n", bp->wptr-bp->rptr);
			kprint(fmt, hp-hifichan, bp->rptr);
		}
		if(streamparse("hdlc", bp)){
			if(streamparse("on", bp)){
				hp->hdlc = 1;
				hifixmiten(hp, 1);
			}else{
				hp->hdlc = 0;
				if(hp->wenable)
					hifixmiten(hp, 1);
			}
		}else if(streamparse("loop", bp)){
			if(streamparse("on", bp))
				hifiloopctl(hp, 1);
			else
				hifiloopctl(hp, 0);
		}else if(streamparse("click!", bp)){
			hifirecven(hp, 0);
		}else if(streamparse("recven", bp)){
			hifirecven(hp, 1);
		}else if(streamparse("nohup", bp)){
			hp->nohup = 1;
		}
		freeb(bp);
		return;
	}
	qlock(&hp->wlock);
	if(!putq(q, bp)){	/* send only whole messages */
		qunlock(&hp->wlock);
		return;
	}
	DPRINT("hifioput %d: [%d] len=%d\n",
		hp-hifichan, TK2MS(MACHP(0)->ticks), q->len);
	if(tFull(hp))
		sleep(&hp->tr, tNotFull, hp);
	hp->outb[hp->wo] = grabq(q);
	hp->wo = NEXT(hp->wo);
	qunlock(&hp->wlock);

	isdnlock(hp->udev);
	if(hifixmit(hp))
		wakeup(&hp->kr);
	isdnunlock(hp->udev);
}

static void
hifirecven(Hifichan *hp, int r)
{
	Hifi *hifi = hp->hdev;
	int i;

	qlock(hp);
	if(!hp->renable && r > 0){
		hp->ri = hp->wi;
		for (i=0; i<NB; i++)
			if(!hp->inb[i])
				hp->inb[i] = allocb(BSIZE);
		hp->renable = 1;
		isdnpoke(hp->udev, &hifi->opreg, hp->opreg |= Enr);
		DPRINT("hifirecven %d: enabled\n", hp-hifichan);
	}else if(hp->renable && r <= 0){
		isdnpoke(hp->udev, &hifi->opreg, hp->opreg &= ~Enr);
		hp->renable = 0;
		hp->hangup = 1;
		wakeup(&hp->kr);
		DPRINT("hifirecven %d: disabled\n", hp-hifichan);
	}
	if(r < 0){
		hp->ri = hp->wi;
		for (i=0; i<NB; i++)
			if(hp->inb[i]){
				freeb(hp->inb[i]);
				hp->inb[i] = 0;
			}
		DPRINT("hifirecven %d: flushed\n", hp-hifichan);
	}
	qunlock(hp);
}

static void
hifikproc(void *arg)
{
	Hifichan *hp = arg;
	Block *bp;
	int i;
Top:
	if(waserror()){
		print("hifikproc: error\n");
		goto Top;
	}
	for(;;){
		qlock(hp);
		while(hp->ri != hp->wi){
			if(hp->rq){
				FLOWCTL(hp->rq, hp->inb[hp->ri]);
			}else
				freeb(hp->inb[hp->ri]);
			if(hp->renable)
				hp->inb[hp->ri] = allocb(BSIZE);
			else
				hp->inb[hp->ri] = 0;
			hp->ri = NEXT(hp->ri);
		}
		if(hp->hangup){
			hp->hangup = 0;
			if(hp->rq){
				bp = allocb(0);
				bp->type = M_HANGUP;
				PUTNEXT(hp->rq, bp);
			}
		}
		i = 0;
		while(hp->so != hp->ro){
			freeb(hp->outb[hp->so]);
			hp->so = NEXT(hp->so);
			i++;
		}
		qunlock(hp);
		if(i)
			wakeup(&hp->tr);
		sleep(&hp->kr, kRun, hp);
	}
}

static void
devinit(int h)
{
	Hifichan *hp = &hifichan[h];
	Hifi *hifi = (h&1) ? hifi1 : hifi0;
	Isdn *ip = &isdndev[h>>1];

	isdnlock(ip);

	hp->udev = ip;
	hp->hdev = hifi;
	hp->imask = (h&1) ? Hint1 : Hint0;

	hp->opreg = 0;
	SET(hifi->opreg, Rres|Tres);	/* master reset */
	SET(hifi->config, Fsen);	/* frame sync enable */
	hp->tctl = 30;
	SET(hifi->tctl, hp->tctl);	/* transmitter interrupt level */
	SET(hifi->rctl, 20);		/* receiver interrupt level */
	SET(hifi->bofctl, Clkxi);	/* transmit in rising edge of CLKX */
	/* SET(hifi->tofctl, Dxi);	/* transmit inverted data */
	SET(hifi->tofctl, 0);		/* transmit normal data */
	SET(hifi->rof_tm, Dri);		/* receive inverted data */
	SET(hifi->config, Fsen|Alt);	/* alternate reg's... */
	SET(hifi->rof_tm, Trans);	/* transparent mode */
	SET(hifi->config, Fsen);	/* normal reg's... */
	SET(hifi->tmask, 0xff);		/* transmit all bits */
	SET(hifi->imask, Undie|Tdie|Teie|Rfie|Reofie); /* interrupt enables */

	isdnunlock(ip);
}

static void
hifiloopctl(Hifichan *hp, int loop)
{
	Isdn *ip = hp->udev;
	Hifi *hifi = hp->hdev;

	DPRINT("hifiloopctl %d: %d\n", hp-hifichan, loop);
	isdnlock(ip);
	if(loop)
		SET(hifi->opreg, hp->opreg |= Lloop);
	else
		SET(hifi->opreg, hp->opreg &= ~Lloop);
	isdnunlock(ip);
}

static int
hifiintr(void)
{
	Isdn *ip;
	Hifichan *hp;
	Hifi *hifi;
	int intr = 0, status, wake = 0, w = 0;

	for(hp=hifichan; hp<hifichanN; hp++){
		if(!(ip = hp->udev))	/* assign = */
			continue;
		if (!(*(ip->asr)&hp->imask))
			continue;
		++intr;
		hifi = hp->hdev;
		status = GET(hifi->istat);
		if(status&(Rf|Reof)){
			status &= ~(Rf|Reof);
			wake += hifirecv(hp);
		}
		if(status&Te){
			status &= ~Te;
			wake += w = hifixmit(hp);
		}
		if(status&Undabt){
			status &= ~Undabt;
			DPRINT("hifiintr: hdlc %d under run\n", hp-hifichan);
			++hp->underrun;
		}
		if(status&Tdone){
			status &= ~Tdone;
			hp->wactive = 0;
			if(!hp->hdlc && (w || hp->out || hp->ro != hp->wo)){
				DPRINT("hifiintr: trans %d under run\n",
					hp-hifichan);
				++hp->underrun;
			}else if(!hp->hdlc || !hp->rq){
				DPRINT("hifiintr: xmtr %d done\n", hp-hifichan);
				hifixmitdis(hp, 0);
				++wake;
				wakeup(&hp->cr);
			}
		}
		if(wake)
			wakeup(&hp->kr);
		if(status)
			DPRINT("hifiintr: status %d 0x%2.2x\n",
				status, hp-hifichan);
	}
	return intr;
}

static int
hifirecv(Hifichan *hp)
{
	Isdn *ip = hp->udev;
	Hifi *hifi = hp->hdev;
	Block *bp = hp->inb[hp->wi];
	uchar *p;
	int status, i, n, m, wake = 0;
Loop:
	status = GET(hifi->rstat);
	if(status == 0)
		return wake;
	n = status&0x7f;
	p = bp->wptr;
	m = bp->lim - p;
	if(hp->hdlc){
		if(n > m){
			hp->toolong++;
			bp->wptr = bp->base;
			bp->rptr = bp->base;
			SET(hifi->opreg, hp->opreg | Rres);
			return wake;
		}
		hp->inchars += n;
		SETADDR(&hifi->data);
		while(--n >= 0)
			*p++ = *(ip->data);
		if(status&Eof){
			if(*--p == 0){
				i = NEXT(hp->wi);
				if(i == hp->ri)
					hp->overflow++;
				else{
					bp->wptr = p;
					bp->flags |= S_DELIM;
					hp->wi = i;
					bp = hp->inb[i];
					++wake;
				}
			}else{
				if(*p & 0x80)
					hp->badcrc++;
				if(*p & 0x40)
					hp->abort++;
				if(*p & 0x20)
					hp->overrun++;
				if(*p & 0x10)
					hp->badcount++;
			}
			bp->wptr = bp->base;
			bp->rptr = bp->base;
		}else
			bp->wptr = p;
	}else{
		if(n > m)
			n = m;
		hp->inchars += n;
		SETADDR(&hifi->data);
		while(--n >= 0)
			*p++ = *(ip->data);
		bp->wptr = p;
		if(p >= bp->lim){
			i = NEXT(hp->wi);
			if(i == hp->ri){
				bp->wptr = bp->base;
				bp->rptr = bp->base;
				return wake;
			}else{
				bp->flags |= S_DELIM;
				hp->wi = i;
				bp = hp->inb[i];
				++wake;
			}
		}
	}
	goto Loop;
}

static int
hifixmit(Hifichan *hp)
{
	Isdn *ip = hp->udev;
	Hifi *hifi = hp->hdev;
	Block *bp;
	uchar *p;
	int n, k, wake = 0;

Loop:
	n = GET(hifi->tstat)&0x7f;
	if(n == 0)
		return wake;
	if(!(bp = hp->out)){	/* assign = */
		if(hp->ro == hp->wo){
			DPRINT("hifixmit %d: [%d] empty\n",
				hp-hifichan, TK2MS(MACHP(0)->ticks));
			return wake;
		}
		hp->out = bp = hp->outb[hp->ro];
	}
	p = bp->rptr;
	k = bp->wptr - p;
	if (n > k)
		n = k;
	hp->outchars += n;
	SETADDR(&hifi->data);
	if(hp->hdlc){
		while(--n >= 0)
			*(ip->data) = *p++;
	}else{
		while(--n >= 0)
			*(ip->data) = ~*p++;
	}
	bp->rptr = p;
	hp->wactive = 1;
	if(!hp->wenable)
		hifixmiten(hp, 0);
	if(bp->rptr == bp->wptr){
		if(hp->hdlc && (bp->flags & S_DELIM))
			SET(hifi->tctl, hp->tctl|Tfc);
		if(!(hp->out = bp = bp->next)){	/* assign = */
			hp->ro = NEXT(hp->ro);
			++wake;
		}
	}
	goto Loop;
}

static void
hifixmiten(Hifichan *hp, int dolock)
{
	Isdn *ip = hp->udev;
	Hifi *hifi = hp->hdev;

	DPRINT("hifixmiten %d: %s\n", hp-hifichan, hp->hdlc?"hdlc":"trans");
	if(dolock)
		isdnlock(ip);
	if(hp->hdlc){
		SET(hifi->tofctl, Dxi|Tlbit);	/* xmit ~data, lsb first */
		SET(hifi->rof_tm, Dri|Rlbit);	/* rcv ~data, lsb first */
		SET(hifi->config, Fsen|Alt);	/* alternate reg's... */
		SET(hifi->rof_tm, 0);		/* hdlc mode */
		SET(hifi->config, Fsen|Flags);	/* normal reg's, send flags */
	}else{
		SET(hifi->tofctl, 0);		/* xmit data, msb first */
		SET(hifi->rof_tm, Dri);		/* rcv ~data, msb first */
		SET(hifi->config, Fsen|Alt);	/* alternate reg's... */
		SET(hifi->rof_tm, Trans);	/* transparent mode */
		SET(hifi->config, Fsen);	/* normal reg's, send ones */
	}
	hp->wenable = 1;
	if(hp->hdev == hifi0){
		ip->venval &= ~DXCODEC;
		if(hp->hdlc)
			ip->venval &= ~DRCODEC;
	}else if(hp->hdlc)
		SET(hifi->config, Flags);	/* no fs on chan. 2 */
	SET(hifi->ttsctl, Dxac);
	SET(hifi->opreg, hp->opreg |= Ent);
	if(dolock)
		isdnunlock(ip);
}

static void
hifixmitdis(Hifichan *hp, int dolock)
{
	Isdn *ip = hp->udev;
	Hifi *hifi = hp->hdev;

	DPRINT("hifixmitdis %d\n", hp-hifichan);
	if(dolock)
		isdnlock(ip);
	SET(hifi->ttsctl, 0);
	if(hp->hdev == hifi0){
		ip->venval |= DXCODEC;
		if(hp->hdlc)
			ip->venval |= DRCODEC;
	}
	SET(hifi->opreg, hp->opreg &= ~Ent);
	hp->out = 0;
	hp->ro = hp->wo;
	hp->wenable = 0;
	/*SET(hifi->opreg, hp->opreg | Tres);*/
	if(dolock)
		isdnunlock(ip);
}
