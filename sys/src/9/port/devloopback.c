#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

typedef struct Link	Link;
typedef struct Loop	Loop;

struct Link
{
	Lock;

	int	ref;

	long	packets;	/* total number of packets sent */
	long	bytes;		/* total number of bytes sent */
	int	indrop;		/* enable dropping on iq overflow */
	long	soverflows;	/* packets dropped because iq overflowed */
	long	droprate;	/* drop 1/droprate packets in tq */
	long	drops;		/* packets deliberately dropped */

	vlong	delay0ns;	/* nanosec of delay in the link */
	long	delaynns;	/* nanosec of delay per byte */

	Block	*tq;		/* transmission queue */
	Block	*tqtail;
	vlong	tout;		/* time the last packet in tq is really out */
	vlong	tin;		/* time the head packet in tq enters the remote side  */

	long	limit;		/* queue buffering limit */
	Queue	*oq;		/* output queue from other side & packets in the link */
	Queue	*iq;

	Timer	ci;		/* time to move packets from  next packet from oq */
};

struct Loop
{
	QLock;
	int	ref;
	int	minmtu;		/* smallest block transmittable */
	Loop	*next;
	ulong	path;
	Link	link[2];
};

static struct
{
	Lock;
	ulong	path;
} loopbackalloc;

enum
{
	Qtopdir=	1,		/* top level directory */

	Qloopdir,			/* loopback* directory */

	Qportdir,			/* directory each end of the loop */
	Qctl,
	Qstatus,
	Qstats,
	Qdata,

	MaxQ,

	Nloopbacks	= 5,

	Statelen	= 23*1024,	/* status buffer size */

	Tmsize		= 8,
	Delayn 		= 10000,	/* default delays in ns */
	Delay0 		= 2500000,

	Loopqlim	= 32*1024,	/* default size of queues */
};

static Dirtab loopportdir[] =
{
	"ctl",		{Qctl},		0,			0222,
	"status",	{Qstatus},	0,			0444,
	"stats",	{Qstats},	0,			0444,
	"data",		{Qdata},	0,			0666,
};
static Dirtab loopdirs[MaxQ];

static Loop	loopbacks[Nloopbacks];

#define TYPE(x) 	(((ulong)(x))&0xff)
#define ID(x) 		(((ulong)(x))>>8)
#define QID(x,y) 	((((ulong)(x))<<8)|((ulong)(y)))

static void	looper(Loop *lb);
static long	loopoput(Loop *lb, Link *link, Block *bp);
static void	ptime(uchar *p, vlong t);
static vlong	gtime(uchar *p);
static void	closelink(Link *link, int dofree);
static void	pushlink(Link *link, vlong now);
static void	freelb(Loop *lb);
static void	linkintr(Ureg*, Timer *ci);

static void
loopbackinit(void)
{
	int i;

	for(i = 0; i < Nloopbacks; i++)
		loopbacks[i].path = i;

	/* invert directory tables for non-directory entries */
	for(i=0; i<nelem(loopportdir); i++)
		loopdirs[loopportdir[i].qid.path] = loopportdir[i];
}

static Chan*
loopbackattach(char *spec)
{
	Loop *volatile lb;
	Queue *q;
	Chan *c;
	int chan;
	int dev;

	dev = 0;
	if(spec != nil){
		dev = atoi(spec);
		if(dev >= Nloopbacks)
			error(Ebadspec);
	}

	c = devattach('X', spec);
	lb = &loopbacks[dev];

	qlock(lb);
	if(waserror()){
		lb->ref--;
		qunlock(lb);
		nexterror();
	}

	lb->ref++;
	if(lb->ref == 1){
		for(chan = 0; chan < 2; chan++){
			lb->link[chan].ci.mode = Trelative;
			lb->link[chan].ci.a = &lb->link[chan];
			lb->link[chan].ci.f = linkintr;
			lb->link[chan].limit = Loopqlim;
			q = qopen(lb->link[chan].limit, 0, 0, 0);
			lb->link[chan].iq = q;
			if(q == nil){
				freelb(lb);
				exhausted("memory");
			}
			q = qopen(lb->link[chan].limit, 0, 0, 0);
			lb->link[chan].oq = q;
			if(q == nil){
				freelb(lb);
				exhausted("memory");
			}
			lb->link[chan].indrop = 1;

			lb->link[chan].delaynns = Delayn;
			lb->link[chan].delay0ns = Delay0;
		}
	}
	poperror();
	qunlock(lb);

	mkqid(&c->qid, QID(0, Qtopdir), 0, QTDIR);
	c->aux = lb;
	c->dev = dev;
	return c;
}

static int
loopbackgen(Chan *c, char*, Dirtab*, int, int i, Dir *dp)
{
	Dirtab *tab;
	int len, type;
	Qid qid;

	type = TYPE(c->qid.path);
	if(i == DEVDOTDOT){
		switch(type){
		case Qtopdir:
		case Qloopdir:
			snprint(up->genbuf, sizeof(up->genbuf), "#X%ld", c->dev);
			mkqid(&qid, QID(0, Qtopdir), 0, QTDIR);
			devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
			break;
		case Qportdir:
			snprint(up->genbuf, sizeof(up->genbuf), "loopback%ld", c->dev);
			mkqid(&qid, QID(0, Qloopdir), 0, QTDIR);
			devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
			break;
		default:
			panic("loopbackgen %llux", c->qid.path);
		}
		return 1;
	}

	switch(type){
	case Qtopdir:
		if(i != 0)
			return -1;
		snprint(up->genbuf, sizeof(up->genbuf), "loopback%ld", c->dev);
		mkqid(&qid, QID(0, Qloopdir), 0, QTDIR);
		devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
		return 1;
	case Qloopdir:
		if(i >= 2)
			return -1;
		snprint(up->genbuf, sizeof(up->genbuf), "%d", i);
		mkqid(&qid, QID(i, QID(0, Qportdir)), 0, QTDIR);
		devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
		return 1;
	case Qportdir:
		if(i >= nelem(loopportdir))
			return -1;
		tab = &loopportdir[i];
		mkqid(&qid, QID(ID(c->qid.path), tab->qid.path), 0, QTFILE);
		devdir(c, qid, tab->name, tab->length, eve, tab->perm, dp);
		return 1;
	default:
		/* non directory entries end up here; must be in lowest level */
		if(c->qid.type & QTDIR)
			panic("loopbackgen: unexpected directory");	
		if(i != 0)
			return -1;
		tab = &loopdirs[type];
		if(tab == nil)
			panic("loopbackgen: unknown type: %d", type);
		len = tab->length;
		devdir(c, c->qid, tab->name, len, eve, tab->perm, dp);
		return 1;
	}
}


static Walkqid*
loopbackwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;
	Loop *lb;

	wq = devwalk(c, nc, name, nname, nil, 0, loopbackgen);
	if(wq != nil && wq->clone != nil && wq->clone != c){
		lb = c->aux;
		qlock(lb);
		lb->ref++;
		if((c->flag & COPEN) && TYPE(c->qid.path) == Qdata)
			lb->link[ID(c->qid.path)].ref++;
		qunlock(lb);
	}
	return wq;
}

static int
loopbackstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, nil, 0, loopbackgen);
}

/*
 *  if the stream doesn't exist, create it
 */
static Chan*
loopbackopen(Chan *c, int omode)
{
	Loop *lb;

	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	lb = c->aux;
	qlock(lb);
	if(TYPE(c->qid.path) == Qdata){
		if(lb->link[ID(c->qid.path)].ref){
			qunlock(lb);
			error(Einuse);
		}
		lb->link[ID(c->qid.path)].ref++;
	}
	qunlock(lb);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	c->iounit = qiomaxatomic;
	return c;
}

static void
loopbackclose(Chan *c)
{
	Loop *lb;
	int ref, chan;

	lb = c->aux;

	qlock(lb);

	/*
	 * closing either side hangs up the stream
	 */
	if((c->flag & COPEN) && TYPE(c->qid.path) == Qdata){
		chan = ID(c->qid.path);
		if(--lb->link[chan].ref == 0){
			qhangup(lb->link[chan ^ 1].oq, nil);
			looper(lb);
		}
	}


	/*
	 *  if both sides are closed, they are reusable
	 */
	if(lb->link[0].ref == 0 && lb->link[1].ref == 0){
		for(chan = 0; chan < 2; chan++){
			closelink(&lb->link[chan], 0);
			qreopen(lb->link[chan].iq);
			qreopen(lb->link[chan].oq);
			qsetlimit(lb->link[chan].oq, lb->link[chan].limit);
			qsetlimit(lb->link[chan].iq, lb->link[chan].limit);
		}
	}
	ref = --lb->ref;
	if(ref == 0)
		freelb(lb);
	qunlock(lb);
}

static void
freelb(Loop *lb)
{
	int chan;

	for(chan = 0; chan < 2; chan++)
		closelink(&lb->link[chan], 1);
}

/*
 * called with the Loop qlocked,
 * so only pushlink can mess with the queues
 */
static void
closelink(Link *link, int dofree)
{
	Queue *iq, *oq;
	Block *bp;

	ilock(link);
	iq = link->iq;
	oq = link->oq;
	bp = link->tq;
	link->tq = nil;
	link->tqtail = nil;
	link->tout = 0;
	link->tin = 0;
	timerdel(&link->ci);
	iunlock(link);
	if(iq != nil){
		qclose(iq);
		if(dofree){
			ilock(link);
			free(iq);
			link->iq = nil;
			iunlock(link);
		}
	}
	if(oq != nil){
		qclose(oq);
		if(dofree){
			ilock(link);
			free(oq);
			link->oq = nil;
			iunlock(link);
		}
	}
	freeblist(bp);
}

static long
loopbackread(Chan *c, void *va, long n, vlong offset)
{
	Loop *lb;
	Link *link;
	char *buf;
	long rv;

	lb = c->aux;
	switch(TYPE(c->qid.path)){
	default:
		error(Eperm);
		return -1;	/* not reached */
	case Qtopdir:
	case Qloopdir:
	case Qportdir:
		return devdirread(c, va, n, nil, 0, loopbackgen);
	case Qdata:
		return qread(lb->link[ID(c->qid.path)].iq, va, n);
	case Qstatus:
		link = &lb->link[ID(c->qid.path)];
		buf = smalloc(Statelen);
		rv = snprint(buf, Statelen, "delay %lld %ld\n", link->delay0ns, link->delaynns);
		rv += snprint(buf+rv, Statelen-rv, "limit %ld\n", link->limit);
		rv += snprint(buf+rv, Statelen-rv, "indrop %d\n", link->indrop);
		snprint(buf+rv, Statelen-rv, "droprate %ld\n", link->droprate);
		rv = readstr(offset, va, n, buf);
		free(buf);
		break;
	case Qstats:
		link = &lb->link[ID(c->qid.path)];
		buf = smalloc(Statelen);
		rv = snprint(buf, Statelen, "packets: %ld\n", link->packets);
		rv += snprint(buf+rv, Statelen-rv, "bytes: %ld\n", link->bytes);
		rv += snprint(buf+rv, Statelen-rv, "dropped: %ld\n", link->drops);
		snprint(buf+rv, Statelen-rv, "soft overflows: %ld\n", link->soverflows);
		rv = readstr(offset, va, n, buf);
		free(buf);
		break;
	}
	return rv;
}

static Block*
loopbackbread(Chan *c, long n, ulong offset)
{
	Loop *lb;

	lb = c->aux;
	if(TYPE(c->qid.path) == Qdata)
		return qbread(lb->link[ID(c->qid.path)].iq, n);

	return devbread(c, n, offset);
}

static long
loopbackbwrite(Chan *c, Block *bp, ulong off)
{
	Loop *lb;

	lb = c->aux;
	if(TYPE(c->qid.path) == Qdata)
		return loopoput(lb, &lb->link[ID(c->qid.path) ^ 1], bp);
	return devbwrite(c, bp, off);
}

static long
loopbackwrite(Chan *c, void *va, long n, vlong off)
{
	Loop *lb;
	Link *link;
	Cmdbuf *volatile cb;
	Block *volatile bp;
	vlong d0, d0ns;
	long dn, dnns;

	switch(TYPE(c->qid.path)){
	case Qdata:
		bp = allocb(n);
		if(waserror()){
			freeb(bp);
			nexterror();
		}
		memmove(bp->wp, va, n);
		poperror();
		bp->wp += n;
		return loopbackbwrite(c, bp, off);
	case Qctl:
		lb = c->aux;
		link = &lb->link[ID(c->qid.path)];
		cb = parsecmd(va, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		if(cb->nf < 1)
			error("short control request");
		if(strcmp(cb->f[0], "delay") == 0){
			if(cb->nf != 3)
				error("usage: delay latency bytedelay");
			d0ns = strtoll(cb->f[1], nil, 10);
			dnns = strtol(cb->f[2], nil, 10);

			/*
			 * it takes about 20000 cycles on a pentium ii
			 * to run pushlink; perhaps this should be accounted.
			 */

			ilock(link);
			link->delay0ns = d0ns;
			link->delaynns = dnns;
			iunlock(link);
		}else if(strcmp(cb->f[0], "indrop") == 0){
			if(cb->nf != 2)
				error("usage: indrop [01]");
			ilock(link);
			link->indrop = strtol(cb->f[1], nil, 0) != 0;
			iunlock(link);
		}else if(strcmp(cb->f[0], "droprate") == 0){
			if(cb->nf != 2)
				error("usage: droprate ofn");
			ilock(link);
			link->droprate = strtol(cb->f[1], nil, 0);
			iunlock(link);
		}else if(strcmp(cb->f[0], "limit") == 0){
			if(cb->nf != 2)
				error("usage: limit maxqsize");
			ilock(link);
			link->limit = strtol(cb->f[1], nil, 0);
			qsetlimit(link->oq, link->limit);
			qsetlimit(link->iq, link->limit);
			iunlock(link);
		}else if(strcmp(cb->f[0], "reset") == 0){
			if(cb->nf != 1)
				error("usage: reset");
			ilock(link);
			link->packets = 0;
			link->bytes = 0;
			link->indrop = 0;
			link->soverflows = 0;
			link->drops = 0;
			iunlock(link);
		}else
			error("unknown control request");
		poperror();
		free(cb);
		break;
	default:
		error(Eperm);
	}

	return n;
}

static long
loopoput(Loop *lb, Link *link, Block *volatile bp)
{
	long n;

	n = BLEN(bp);

	/* make it a single block with space for the loopback timing header */
	if(waserror()){
		freeb(bp);
		nexterror();
	}
	bp = padblock(bp, Tmsize);
	if(bp->next)
		bp = concatblock(bp);
	if(BLEN(bp) < lb->minmtu)
		bp = adjustblock(bp, lb->minmtu);
	poperror();
	ptime(bp->rp, todget(nil));

	link->packets++;
	link->bytes += n;

	qbwrite(link->oq, bp);

	looper(lb);
	return n;
}

static void
looper(Loop *lb)
{
	vlong t;
	int chan;

	t = todget(nil);
	for(chan = 0; chan < 2; chan++)
		pushlink(&lb->link[chan], t);
}

static void
linkintr(Ureg*, Timer *ci)
{
	Link *link;

	link = ci->a;
	pushlink(link, ci->ns);
}

/*
 * move blocks between queues if they are ready.
 * schedule an interrupt for the next interesting time.
 *
 * must be called with the link ilocked.
 */
static void
pushlink(Link *link, vlong now)
{
	Block *bp;
	vlong tout, tin;

	/*
	 * put another block in the link queue
	 */
	ilock(link);
	if(link->iq == nil || link->oq == nil){
		iunlock(link);
		return;

	}
	timerdel(&link->ci);

	/*
	 * put more blocks into the xmit queue
	 * use the time the last packet was supposed to go out
	 * as the start time for the next packet, rather than
	 * the current time.  this more closely models a network
	 * device which can queue multiple output packets.
	 */
	tout = link->tout;
	if(!tout)
		tout = now;
	while(tout <= now){
		bp = qget(link->oq);
		if(bp == nil){
			tout = 0;
			break;
		}

		/*
		 * can't send the packet before it gets queued
		 */
		tin = gtime(bp->rp);
		if(tin > tout)
			tout = tin;
		tout = tout + (BLEN(bp) - Tmsize) * link->delayn;

		/*
		 * drop packets
		 */
		if(link->droprate && nrand(link->droprate) == 0)
			link->drops++;
		else{
			ptime(bp->rp, tout + link->delay0ns);
			if(link->tq == nil)
				link->tq = bp;
			else
				link->tqtail->next = bp;
			link->tqtail = bp;
		}
	}

	/*
	 * record the next time a packet can be sent,
	 * but don't schedule an interrupt if none is waiting
	 */
	link->tout = tout;
	if(!qcanread(link->oq))
		tout = 0;

	/*
	 * put more blocks into the receive queue
	 */
	tin = 0;
	while(bp = link->tq){
		tin = gtime(bp->rp);
		if(tin > now)
			break;
		bp->rp += Tmsize;
		link->tq = bp->next;
		bp->next = nil;
		if(!link->indrop)
			qpassnolim(link->iq, bp);
		else if(qpass(link->iq, bp) < 0)
			link->soverflows++;
		tin = 0;
	}
	if(bp == nil && qisclosed(link->oq) && !qcanread(link->oq) && !qisclosed(link->iq))
		qhangup(link->iq, nil);
	link->tin = tin;
	if(!tin || tin > tout && tout)
		tin = tout;

	link->ci.ns = tin - now;
	if(tin){
		if(tin < now)
			panic("loopback unfinished business");
		timeradd(&link->ci);
	}
	iunlock(link);
}

static void
ptime(uchar *p, vlong t)
{
	ulong tt;

	tt = t >> 32;
	p[0] = tt >> 24;
	p[1] = tt >> 16;
	p[2] = tt >> 8;
	p[3] = tt;
	tt = t;
	p[4] = tt >> 24;
	p[5] = tt >> 16;
	p[6] = tt >> 8;
	p[7] = tt;
}

static vlong
gtime(uchar *p)
{
	ulong t1, t2;

	t1 = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	t2 = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
	return ((vlong)t1 << 32) | t2;
}

Dev loopbackdevtab = {
	'X',
	"loopback",

	devreset,
	loopbackinit,
	devshutdown,
	loopbackattach,
	loopbackwalk,
	loopbackstat,
	loopbackopen,
	devcreate,
	loopbackclose,
	loopbackread,
	loopbackbread,
	loopbackwrite,
	loopbackbwrite,
	devremove,
	devwstat,
};
