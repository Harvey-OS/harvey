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
#include	"unite.h"

#define DPRINT 	if(isdndebug)kprint

#define	BSIZE	200	/* should be 260, but for xalloc */

Isdn *	isdndev;
Isdn *	isdndevN;

static void	devinit(int);
static void	devctl(Isdn*, int, int);
static void	devlctl(Isdn*, int);
static void	devb1xb2(Isdn*, int);
static void	isdnkproc(void*);
static int	isdnintr(void);
static int	isdnInfo(Isdn*);
static int	isdnrecv(Isdn*);
static int	isdnxmit(Isdn*);

enum {
	Qdir, Qloop, QInfo, Qb1xb2, Qstats, Qdev, Qdebug
};

Dirtab isdndir[]={
	"loop",		{Qloop},	5,	0666,
	"info",		{QInfo},	1,	0666,
	"b1xb2",	{Qb1xb2},	1,	0666,
	"stats",	{Qstats},	0,	0444,
	"dev",		{Qdev},		0,	0666,
	"debug",	{Qdebug},	0,	0666,
};
#define	NISDN	(sizeof isdndir/sizeof(Dirtab))

/*
 *  I.430 stream module definition
 */
static void isdniput(Queue*, Block*);
static void isdnoput(Queue*, Block*);
static void isdnstopen(Queue*, Stream*);
static void isdnstclose(Queue*);
Qinfo isdninfo = { isdniput, isdnoput, isdnstopen, isdnstclose, "isdn" };

int	venval;
int	isdndebug;

void
isdnreset(void)
{
	isdndev = xalloc(conf.nisdn*sizeof(Isdn));
	isdndevN = &isdndev[conf.nisdn];
}

void
isdninit(void)
{}

Chan*
isdnattach(char *spec)
{
	Chan *c;
	int dev;

	if(spec && *spec){
		if(*spec < '0' || *spec > '9')
			error(Enonexist);
		dev = strtoul(spec, 0, 0);
	}else
		dev = 0;
	if(dev >= conf.nisdn)
		error(Enonexist);
	if(!isdndev[dev].asr){
		addportintr(isdnintr);
		devinit(dev);
	}
	c = devattach('I', spec);
	c->dev = dev;
	return c;
}

Chan*
isdnclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
isdnwalk(Chan *c, char *name)
{
	return devwalk(c, name, isdndir, NISDN, streamgen);
}

void	 
isdnstat(Chan *c, char *dp)
{
	devstat(c, dp, isdndir, NISDN, streamgen);
}

Chan*
isdnopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}else switch(STREAMTYPE(c->qid.path)){
	case Qloop:
	case QInfo:
	case Qb1xb2:
	case Qstats:
	case Qdev:
	case Qdebug:
		break;
	default:
		DPRINT("isdnopen dev=%d\n", c->dev);
		streamopen(c, &isdninfo);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void	 
isdncreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
isdnclose(Chan *c)
{
	if(c->qid.path != CHDIR)
		streamclose(c);
}

long	 
isdnread(Chan *c, void *buf, long n, ulong offset)
{
	Isdn *ip = &isdndev[c->dev];
	char nbuf[512], *p;
	int k;

	if(n <= 0)
		return 0;
	if(c->qid.path == CHDIR){
		return devdirread(c, buf, n, isdndir, NISDN, streamgen);
	}else switch(STREAMTYPE(c->qid.path)){
	case Qloop:
		sprint(nbuf, "0x%2.2ux ", ip->lctl);
		return readstr(offset, buf, n, nbuf);
	case QInfo:
		if(offset>0)
			return 0;
		k = isdnpeek(ip, &unite->listat)&Rss;
		*(char *)buf = "024L"[k>>2];
		return 1;
	case Qb1xb2:
		if(offset>0)
			return 0;
		k = isdnpeek(ip, &unite->stictl)&B1xb2;
		*(char *)buf = "01"[k != 0];
		return 1;
	case Qstats:
		p = nbuf;
		p += sprint(p, "inchars  %10lud\n", ip->inchars);
		p += sprint(p, "badcrc   %10lud\n", ip->badcrc);
		p += sprint(p, "abort    %10lud\n", ip->abort);
		p += sprint(p, "overrun  %10lud\n", ip->overrun);
		p += sprint(p, "badcount %10lud\n", ip->badcount);
		p += sprint(p, "overflow %10lud\n", ip->overflow);
		p += sprint(p, "toolong  %10lud\n", ip->toolong);
		p += sprint(p, "underrun %10lud\n", ip->underrun);
		p += sprint(p, "outchars %10lud\n", ip->outchars);
		USED(p);
		return readstr(offset, buf, n, nbuf);
	case Qdev:
		k = isdnpeek(ip, (void *)ip->devaddr);
		sprint(nbuf, "0x%2.2ux = 0x%2.2ux\n", ip->devaddr, k);
		return readstr(offset, buf, n, nbuf);
	case Qdebug:
		sprint(nbuf, "%d\n", isdndebug);
		return readstr(offset, buf, n, nbuf);
	}
	return streamread(c, buf, n);
}

long	 
isdnwrite(Chan *c, void *buf, long n, ulong offset)
{
	Isdn *ip = &isdndev[c->dev];
	char nbuf[32], *p;

	if(n <= 0)
		return 0;
	switch(STREAMTYPE(c->qid.path)){
	case Qloop:
		if(offset>0)
			return 0;
		if (n>sizeof nbuf)
			n = sizeof nbuf;
		memmove(nbuf, buf, n);
		nbuf[n-1] = 0;
		devlctl(ip, strtoul(nbuf,0,0));
		return n;
	case QInfo:
		if(offset>0)
			return 0;
		switch(*(char *)buf){
		case '0':
		case '1':
		case '3':
			devctl(ip, Tss, *(char *)buf - '0');
			break;
		default:
			error(Ebadarg);
		}
		return n;
	case Qb1xb2:
		if(offset>0)
			return 0;
		switch(*(char *)buf){
		case '0':
		case '1':
			devb1xb2(ip, *(char *)buf - '0');
			break;
		default:
			error(Ebadarg);
		}
		return n;
	case Qdev:
		if (n>sizeof nbuf)
			n = sizeof nbuf;
		memmove(nbuf, buf, n);
		nbuf[n-1] = 0;
		ip->devaddr = strtoul(nbuf,0,0);
		if (p = strchr(nbuf, '='))	/* assign = */
			isdnpoke(ip, (void *)ip->devaddr, strtoul(++p,0,0));
		return n;
	case Qdebug:
		if (n>sizeof nbuf)
			n = sizeof nbuf;
		memmove(nbuf, buf, n);
		nbuf[n-1] = 0;
		isdndebug = strtoul(nbuf,0,0);
		return n;
	}
	return streamwrite(c, buf, n, 0);
}

void	 
isdnremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
isdnwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

/*
 *	to sleep, perchance to dream...
 */

#define	Predicate(name, expr)	static int name(void *arg) { return expr; }
#define	ip	((Isdn *)arg)

Predicate(kLive, ip->kstart != 0)
Predicate(kDead, ip->kstart == 0)

Predicate(kRun, !ip->rq || ip->ri != ip->wi || ip->so != ip->ro)

Predicate(lActive, (ip->listat&Rss)>>1 == 4)

Predicate(tEmpty, ip->ro == ip->wo)
Predicate(tFull, NEXT(ip->wo) == ip->so)
Predicate(tNotFull, NEXT(ip->wo) != ip->so)

#undef	ip

/*
 *	the stream routines
 */

static void
isdnstopen(Queue *q, Stream *s)
{
	Isdn *ip = &isdndev[s->dev];
	char name[32];
	int rss;

	DPRINT("isdnstopen %d...", s->dev);
	qlock(&ip->kctl);
	if (ip->kstart) {
		qunlock(&ip->kctl);
		DPRINT("active\n", s->dev);
		return;
	}
	q->ptr = q->other->ptr = ip;
	ip->rq = q;
	ip->hangup = 0;
	rss = (isdnpeek(ip, &unite->listat)&Rss)>>1;
	if (ip->lctl & Lld)		/* if loopback D, */
		devctl(ip, Tss, 0);	/* transmit INFO 0 */
	else if (rss == 2 || rss ==4)	/* else if line active */
		devctl(ip, Tss, 3);	/* transmit INFO 3 */
	else
		devctl(ip, Tss, 1);	/* transmit INFO 1 */
	sprint(name, "isdn%d", s->dev);
	kproc(name, isdnkproc, ip);
	if (waserror()) {
		qunlock(&ip->kctl);
		DPRINT("aborted\n");
		nexterror();
	}
	sleep(&ip->kctlr, kLive, ip);
	qunlock(&ip->kctl);
	poperror();
	DPRINT("started\n");
	if (!(ip->lctl & Lld)) {
		DPRINT("waiting for INFO 4\n");
		sleep(&ip->ar, lActive, ip);
		DPRINT("got INFO 4\n");
	}
}

static void
isdnstclose(Queue * q)
{
	Isdn *ip = (Isdn *)q->ptr;
	DPRINT("isdnstclose %d...", ip-isdndev);
	qlock(&ip->kctl);
	qlock(ip);
	ip->rq = 0;
	qunlock(ip);
	wakeup(&ip->kr);
	if (waserror()) {
		qunlock(&ip->kctl);
		DPRINT("aborted\n");
		nexterror();
	}
	sleep(&ip->kctlr, kDead, ip);
	qunlock(&ip->kctl);
	poperror();
	DPRINT("stopped\n");
}

static void
showframe(char *t, uchar *buf, int n)
{
	kprint("I %s [", t);
	while (--n >= 0)
		kprint(" %2.2ux", *buf++);
	kprint(" ]\n");
}

/*
 *  this is only called by hangup
 */
static void
isdniput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

/*
 *  output a block
 */
static void
isdnoput(Queue *q, Block *bp)
{
	Isdn *ip = (Isdn *)q->ptr;
	uchar set, clr;

	if(bp->type != M_DATA){
		set = clr = 0;
		if(streamparse("ear", bp)){
			if(streamparse("on", bp))
				set = DRCODEC;
			else if(streamparse("off", bp))
				clr = DRCODEC;
		}else if(streamparse("mic", bp)){
			if(streamparse("on", bp))
				set = DXCODEC;
			else if(streamparse("off", bp))
				clr = DXCODEC;
		}
		freeb(bp);
		isdnlock(ip);
		ip->venval &= ~clr;
		ip->venval |= set;
		*(ip->asr) = ip->venval;
		isdnunlock(ip);
		return;
	}
	qlock(&ip->wlock);
	if(!putq(q, bp)){	/* send only whole messages */
		qunlock(&ip->wlock);
		return;
	}
	DPRINT("isdnoput: len=%d\n", q->len);
	if (tFull(ip))
		sleep(&ip->tr, tNotFull, ip);
	ip->outb[ip->wo] = grabq(q);
	ip->wo = NEXT(ip->wo);
	qunlock(&ip->wlock);

	isdnlock(ip);
	if (isdnxmit(ip))
		wakeup(&ip->kr);
	isdnunlock(ip);
}

static void
isdnkproc(void *arg)
{
	Isdn *ip = (Isdn *)arg;
	Block *bp;
	int i;

	if(waserror()){
		devctl(ip, Enr, 0);
		ip->kstart = 0;
		wakeup(&ip->kctlr);
		return;
	}
	/*
	 *  create a number of blocks for input
	 */
	for(i=0; i<NB; i++)
		if (!ip->inb[i])
			ip->inb[i] = allocb(BSIZE);
	ip->ri = 0;
	ip->wi = 0;
	devctl(ip, 0, Enr);
	ip->kstart = 1;
	wakeup(&ip->kctlr);
	for(;;){
		qlock(ip);
		if(!ip->rq){
			qunlock(ip);
			break;
		}
		while(ip->ri != ip->wi){
			FLOWCTL(ip->rq, ip->inb[ip->ri]);
			ip->inb[ip->ri] = allocb(BSIZE);
			ip->ri = NEXT(ip->ri);
		}
		if(ip->hangup){
			ip->hangup = 0;
			bp = allocb(0);
			bp->type = M_HANGUP;
			PUTNEXT(ip->rq, bp);
		}
		i = 0;
		while(ip->so != ip->ro){
			freeb(ip->outb[ip->so]);
			ip->so = NEXT(ip->so);
			i++;
		}
		qunlock(ip);
		if (i)
			wakeup(&ip->tr);
		sleep(&ip->kr, kRun, ip);
	}
	devctl(ip, Enr, 0);
	sleep(&ip->kr, tEmpty, ip);
	while(ip->so != ip->ro){
		freeb(ip->outb[ip->so]);
		ip->so = NEXT(ip->so);
	}
	poperror();
	ip->kstart = 0;
	wakeup(&ip->kctlr);
}

void
isdnlock(Isdn *ip)
{
	int s;

	s = spl2();
	lock(&ip->dev);
	ip->pri = s;
}

void
isdnunlock(Isdn *ip)
{
	int s;

	s = ip->pri;
	unlock(&ip->dev);
	splx(s);
}

static void
devinit(int h)
{
	Isdn *ip = &isdndev[h];

	isdnlock(ip);
	ip->asr = ISDNDEV0+2*h;
	ip->data = ip->asr+1;
	ip->venval = DRCODEC | DXCODEC;
	SET(unite->reset, Mres);
	ip->lctl = 0;		/* no loopback or 1's transmission */
	SET(unite->lctl, ip->lctl);
	ip->stictl = Prye;	/* S/T interface normal */
	SET(unite->stictl, ip->stictl);
	SET(unite->itl, 0x88);	/* interrupt on 8 rcv/xmit char's */
	SET(unite->hcr, Tpol|Ipol|Clkmux|B2f|B1f);
	ip->tctl = Ent;		/* enable transmitter, send INFO 0 */
	ip->imask = Richgie|Teie;
	SET(unite->tctl, ip->tctl);
	SET(unite->imask, ip->imask);
	ip->listat = GET(unite->listat);
	isdnunlock(ip);
}

static void
devctl(Isdn *ip, int clear, int set)
{
	isdnlock(ip);
	ip->tctl &= ~clear;
	ip->tctl |= set;
	if (clear & Ent)
		ip->imask &= ~Teie;
	if (set & Ent)
		ip->imask |= Teie;
	if (clear & Enr)
		ip->imask &= ~(Rfie|Reofie);
	if (set & Enr)
		ip->imask |= (Rfie|Reofie);
	ip->imask |= Richgie;
	SET(unite->tctl, ip->tctl);
	SET(unite->imask, ip->imask);
	isdnunlock(ip);
}

int
isdnpeek(Isdn *ip, void *arg)
{
	int r;

	isdnlock(ip);
	SETADDR(arg);
	r = *(ip->data);
	isdnunlock(ip);
	return r;
}

void
isdnpoke(Isdn *ip, void *arg, int data)
{
	isdnlock(ip);
	SETADDR(arg);
	*(ip->data) = data;
	isdnunlock(ip);
}

static void
devlctl(Isdn *ip, int val)
{
	isdnlock(ip);
	ip->lctl = val;
	SET(unite->lctl, val);
	ip->stictl &= ~(Clrmm|Prye);
	if (val & Lld)
		ip->stictl |= Clrmm;
	else
		ip->stictl |= Prye;
	SET(unite->stictl, ip->stictl);
	isdnunlock(ip);
}

static void
devb1xb2(Isdn *ip, int val)
{
	isdnlock(ip);
	if(val)
		ip->stictl |= B1xb2;
	else
		ip->stictl &= ~B1xb2;
	SET(unite->stictl, ip->stictl);
	isdnunlock(ip);
}

static int
isdnintr(void)
{
	int status, wake = 0, intr = 0;
	Isdn *ip;

	for(ip=isdndev; ip<isdndevN; ip++){
		if (!ip->asr)
			continue;
		if (!(*(ip->asr)&Uint))
			continue;
		++intr;
		status = GET(unite->istat);
		if (status & Richg)
			wake += isdnInfo(ip);
		if (status & (Rfull|Reof))
			wake += isdnrecv(ip);
		if (status & Tempty)
			wake += isdnxmit(ip);
		if (wake)
			wakeup(&ip->kr);
	}
	return intr;
}

static int
isdnInfo(Isdn *ip)
{
	int oldstat, newstat;

	newstat = GET(unite->listat);
	DPRINT("isdnInfo: old=0x%2.2ux, new=0x%2.2ux\n",
		ip->listat, newstat);
	oldstat = (ip->listat&Rss)>>1;
	ip->listat = newstat;
	if (!ip->rq) {
		ip->imask &= ~Richgie;
		SET(unite->imask, ip->imask);
		return 0;
	}
	newstat = (newstat&Rss)>>1;
	ip->tctl &= ~Tss;
	if (newstat == 2 || newstat == 4)
		ip->tctl |= 3;	/* transmit INFO 3 */
	else
		ip->tctl |= 1;	/* transmit INFO 1 */
	SET(unite->tctl, ip->tctl);
	DPRINT("isdnInfo: tctl=0x%2.2ux\n", ip->tctl);
	if(newstat == 4){
		wakeup(&ip->ar);
		return 0;
	}
	if(oldstat == 4){
		ip->hangup = 1;
		return 1;
	}
	return 0;
}

static int
isdnrecv(Isdn *ip)
{
	Block *bp = ip->inb[ip->wi];
	uchar status, *p;
	int i, n, wake = 0;
Loop:
	status = GET(unite->rstat);
	if (status&Overrun) {
		ip->overrun++;
		goto Reset;
	}
	if (!(n = status&Rqs))
		return wake;
	p = bp->wptr;
	if (p+n > bp->lim) {
		ip->toolong++;
		goto Reset;
	}
	ip->inchars += n;
	SETADDR(&unite->data);
	while (--n >= 0)
		*p++ = *(ip->data);
	if (status&Eof) {
		if (*--p == 0) {
			i = NEXT(ip->wi);
			if (i == ip->ri)
				ip->overflow++;
			else {
				bp->wptr = p;
				bp->flags |= S_DELIM;
				ip->wi = i;
				bp = ip->inb[i];
				++wake;
			}
		} else {
			if (*p & 0x80)
				ip->badcrc++;
			if (*p & 0x40)
				ip->abort++;
			if (*p & 0x20)
				ip->overrun++;
			if (*p & 0x10)
				ip->badcount++;
		}
		bp->wptr = bp->base;
		bp->rptr = bp->base;
	} else
		bp->wptr = p;
	goto Loop;
Reset:
	bp->wptr = bp->base;
	bp->rptr = bp->base;
	SET(unite->reset, Rres);
	return wake;
}

static int
isdnxmit(Isdn *ip)
{
	Block *bp;
	uchar status, *p;
	int n, k, wake = 0;
Loop:
	status = GET(unite->tstat);
	if (status&Undabt) {
		ip->underrun++;
		SET(unite->reset, Tres);
		if (ip->out) {
			ip->ro = NEXT(ip->ro);
			ip->out = 0;
			++wake;
		}
	}
	if (!(n = status&Tqs))
		return wake;
	if (!(bp = ip->out)) {
		if (ip->ro == ip->wo)
			return wake;
		ip->out = bp = ip->outb[ip->ro];
	}
	p = bp->rptr;
	k = bp->wptr - p;
	if (n > k)
		n = k;
	ip->outchars += n;
	SETADDR(&unite->data);
	while (--n >= 0)
		*(ip->data) = *p++;
	bp->rptr = p;
	if (bp->rptr == bp->wptr) {
		if (bp->flags & S_DELIM)
			SET(unite->tctl, ip->tctl|Tfc);
		if (!(ip->out = bp = bp->next)) {
			ip->ro = NEXT(ip->ro);
			++wake;
		}
	}
	goto Loop;
}
