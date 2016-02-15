/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <lib.h>
#include <mem.h>
#include <dat.h>
#include <fns.h>
#include <error.h>

enum
{
	Incr = 16,
	Maxatomic = 64*KiB,
};

typedef struct ZPipe	ZPipe;
typedef struct Zq Zq;

struct Zq
{
	Lock;			/* to protect Zq */
	QLock	rlck;		/* one reader at a time */
	Kzio*	io;		/* io[] */
	Kzio*	ep;		/* end pointer */
	int	closed;		/* queue is closed */
	int	waiting;	/* reader is waiting */
	Kzio*	rp;		/* read pointer */
	Kzio*	wp;		/* write pointer */
	Rendez	rr;		/* reader rendez */
};

struct ZPipe
{
	QLock;
	ZPipe	*next;
	int	ref;
	uint32_t	path;
	Zq	q[2];
	int	qref[2];
};

struct
{
	Lock;
	uint32_t	path;
} zpalloc;

enum
{
	Qdir,
	Qdata0,
	Qdata1,
};

Dirtab zpdir[] =
{
	".",		{Qdir,0,QTDIR},	0,		DMDIR|0500,
	"data",		{Qdata0},	0,		0600,
	"data1",	{Qdata1},	0,		0600,
};
#define NZPDIR 3

#define ZPTYPE(x)	(((unsigned)x)&0x1f)
#define ZPID(x)	((((unsigned)x))>>5)
#define ZPQID(i, t)	((((unsigned)i)<<5)|(t))
#define ZQLEN(q)	((q)->wp - (q)->rp)

static int
zqnotempty(void *x)
{
	Zq *q;

	q = x;
	return ZQLEN(q) != 0 || q->closed != 0;
}

static void
zqdump(Zq *q)
{
	Kzio *io;

	if(DBGFLG == 0)
		return;
	print("zq %#p: io %#p rp %ld wp %ld ep %ld\n",
		q, q->io, q->rp - q->io, q->wp - q->io, q->ep - q->io);
	for(io = q->rp; io != nil && io < q->wp; io++)
		print("\tio[%ld] = %Z\n", io - q->io, io);
	print("\n");
}

/*
 * BUG: alloczio in here could be allocating data
 * in the kernel that is not needed. In fact, such data
 * might be in the kernel already. It's only that we don't
 * have a way to reference more than once to the same source
 * data (no reference counters).
 */
static int
zqread(Zq *q, Kzio io[], int nio, usize count)
{
	Proc *up = externup();
	int i;
	int32_t tot, nr;
	Kzio *qio;
	Segment *s;
	char *p;

	DBG("zqread %ld\n", count);
	qlock(&q->rlck);
	lock(q);
	if(waserror()){
		unlock(q);
		qunlock(&q->rlck);
		nexterror();
	}
	while(q->closed == 0 && ZQLEN(q) == 0){
		q->waiting++;
		unlock(q);
		sleep(&q->rr, zqnotempty, q);
		lock(q);
	}
	i = 0;
	for(tot = 0; ZQLEN(q) > 0 && i < nio && tot < count; tot += nr){
		qio = q->rp;
		nr = qio->Zio.size;
		if(tot + nr > count){
			if(i > 0)
				break;
			io[i] = *qio;
			nr = count - tot;
			io[i].Zio.size = nr;
			s = getzkseg();
			if(s == nil){
				DBG("zqread: bytes thrown away\n");
				goto Consume; /* we drop bytes! */
			}
			qio->Zio.size -= nr;
			qio->Zio.data = alloczio(s, qio->Zio.size);
			p = io[i].Zio.data;
			memmove(qio->Zio.data, p + io[i].Zio.size, qio->Zio.size);
			DBG("zqread: copy %#Z %#Z\n", qio, io);
			qio->seg = s;
		}else
			io[i] = *qio;
	Consume:
		i++;
		q->rp++;
	}
	if(q->rp == q->wp)
		q->rp = q->wp = q->io;
	zqdump(q);
	poperror();
	unlock(q);
	qunlock(&q->rlck);
	return i;
}

/*
 * BUG: no flow control here.
 * We queue as many io[]s as we want.
 * Perhaps it would be better to do flow control,
 * but the process feeding the queue would run out
 * of buffering at some point, which also provides
 * flow control somehow.
 */
static int32_t
zqwrite(Zq *q, Kzio io[], int nio)
{
	Proc *up = externup();
	int i, ei, ri, wi, awake;

	lock(q);
	if(waserror()){
		unlock(q);
		nexterror();
	}

	DBG("zqwrite io%#p[%d]\n", io, nio);
	if(DBGFLG)
		for(i = 0; i < nio; i++)
			print("\tio%#p[%d] = %Z\n", io, i, &io[i]);
	if(q->closed)
		error("queue is closed");
	if(q->wp + nio > q->ep){
		if(q->rp > q->io){
			memmove(q->io, q->rp, ZQLEN(q)*sizeof q->io[0]);
			q->wp = q->io + ZQLEN(q);
			q->rp = q->io;
		}
		if(q->wp + nio > q->ep){
			ei = q->ep - q->io;
			ei += Incr;
			ri = q->rp - q->io;
			wi = q->wp - q->io;
			q->io = realloc(q->io, ei*sizeof q->io[0]);
			if(q->io == nil)
				panic("zqwrite: no memory");
			q->ep = q->io + ei;
			q->rp = q->io + ri;
			q->wp = q->io + wi;
			DBG("zqwrite: io %#p rp %#p wp %#p ep %#p\n",
				q->io, q->rp, q->wp, q->ep);
		}
		assert(q->wp + nio <= q->ep);
	}
	memmove(q->wp, io, nio*sizeof io[0]);
	q->wp += nio;
	awake = q->waiting;
	if(awake)
		q->waiting--;
	zqdump(q);
	poperror();
	unlock(q);
	if(awake)
		wakeup(&q->rr);
	return nio;
}

static void
zqflush(Zq *q)
{
	lock(q);
	for(;q->rp < q->wp; q->rp++){
		qlock(&q->rp->seg->lk);
		zputaddr(q->rp->seg, PTR2UINT(q->rp->Zio.data));
		qunlock(&q->rp->seg->lk);
		putseg(q->rp->seg);
	}
	q->rp = q->wp = q->io;
	unlock(q);
}

static void
zqclose(Zq *q)
{
	q->closed = 1;
	zqflush(q);
	wakeup(&q->rr);
}

static void
zqhangup(Zq *q)
{
	q->closed = 1;
	wakeup(&q->rr);
}

static void
zqreopen(Zq *q)
{
	q->closed = 0;
}

/*
 *  create a zp, no streams are created until an open
 */
static Chan*
zpattach(char *spec)
{
	ZPipe *p;
	Chan *c;

	c = devattach(L'∏', spec);
	p = malloc(sizeof(ZPipe));
	if(p == 0)
		exhausted("memory");
	p->ref = 1;

	lock(&zpalloc);
	p->path = ++zpalloc.path;
	unlock(&zpalloc);

	mkqid(&c->qid, ZPQID(2*p->path, Qdir), 0, QTDIR);
	c->aux = p;
	c->devno = 0;
	return c;
}

static int
zpgen(Chan *c, char* d, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid q;
	int len;
	ZPipe *p;

	if(i == DEVDOTDOT){
		devdir(c, c->qid, "#∏", 0, eve, DMDIR|0555, dp);
		return 1;
	}
	i++;	/* skip . */
	if(tab==0 || i>=ntab)
		return -1;

	tab += i;
	p = c->aux;
	switch((uint32_t)tab->qid.path){
	case Qdata0:
		len = ZQLEN(&p->q[0]);
		break;
	case Qdata1:
		len = ZQLEN(&p->q[1]);
		break;
	default:
		len = tab->length;
		break;
	}
	mkqid(&q, ZPQID(ZPID(c->qid.path), tab->qid.path), 0, QTFILE);
	devdir(c, q, tab->name, len, eve, tab->perm, dp);
	return 1;
}


static Walkqid*
zpwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;
	ZPipe *p;

	wq = devwalk(c, nc, name, nname, zpdir, NZPDIR, zpgen);
	if(wq != nil && wq->clone != nil && wq->clone != c){
		p = c->aux;
		qlock(p);
		p->ref++;
		if(c->flag & COPEN){
			print("channel open in zpwalk\n");
			switch(ZPTYPE(c->qid.path)){
			case Qdata0:
				p->qref[0]++;
				break;
			case Qdata1:
				p->qref[1]++;
				break;
			}
		}
		qunlock(p);
	}
	return wq;
}

static int32_t
zpstat(Chan *c, uint8_t *db, int32_t n)
{
	ZPipe *p;
	Dir dir;

	p = c->aux;

	switch(ZPTYPE(c->qid.path)){
	case Qdir:
		devdir(c, c->qid, ".", 0, eve, DMDIR|0555, &dir);
		break;
	case Qdata0:
		devdir(c, c->qid, "data", ZQLEN(&p->q[0]), eve, 0600, &dir);
		break;
	case Qdata1:
		devdir(c, c->qid, "data1", ZQLEN(&p->q[1]), eve, 0600, &dir);
		break;
	default:
		panic("zpstat");
	}
	n = convD2M(&dir, db, n);
	if(n < BIT16SZ)
		error(Eshortstat);
	return n;
}

/*
 *  if the stream doesn't exist, create it
 */
static Chan*
zpopen(Chan *c, int omode)
{
	ZPipe *p;

	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	p = c->aux;
	qlock(p);
	switch(ZPTYPE(c->qid.path)){
	case Qdata0:
		p->qref[0]++;
		break;
	case Qdata1:
		p->qref[1]++;
		break;
	}
	qunlock(p);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	c->iounit = Maxatomic;	/* should we care? */
	return c;
}

static void
zpclose(Chan *c)
{
	ZPipe *p;

	p = c->aux;
	qlock(p);

	if(c->flag & COPEN){
		/*
		 *  closing either side hangs up the stream
		 */
		switch(ZPTYPE(c->qid.path)){
		case Qdata0:
			p->qref[0]--;
			if(p->qref[0] == 0){
				zqhangup(&p->q[1]);
				zqclose(&p->q[0]);
			}
			break;
		case Qdata1:
			p->qref[1]--;
			if(p->qref[1] == 0){
				zqhangup(&p->q[0]);
				zqclose(&p->q[1]);
			}
			break;
		}
	}

	/*
	 *  if both sides are closed, they are reusable
	 */
	if(p->qref[0] == 0 && p->qref[1] == 0){
		zqreopen(&p->q[0]);
		zqreopen(&p->q[1]);
	}

	/*
	 *  free the structure on last close
	 */
	p->ref--;
	if(p->ref == 0){
		qunlock(p);
		free(p);
	} else
		qunlock(p);
}

static int32_t
zpread(Chan *c, void *va, int32_t n, int64_t  mm)
{
	ZPipe *p;
	Kzio io[32];	/* might read less than we could */
	int nio;

	p = c->aux;

	switch(ZPTYPE(c->qid.path)){
	case Qdir:
		return devdirread(c, va, n, zpdir, NZPDIR, zpgen);
	case Qdata0:
		nio = zqread(&p->q[0], io, nelem(io), n);
		return readzio(io, nio, va, n);
	case Qdata1:
		nio = zqread(&p->q[0], io, nelem(io), n);
		return readzio(io, nio, va, n);
	default:
		panic("zpread");
	}
	return -1;	/* not reached */
}

static int
zpzread(Chan *c, Kzio io[], int nio, usize n, int64_t offset)
{
	ZPipe *p;

	p = c->aux;

	switch(ZPTYPE(c->qid.path)){
	case Qdir:
		return devzread(c, io, nio, n, offset);
	case Qdata0:
		return zqread(&p->q[0], io, nio, n);
	case Qdata1:
		return zqread(&p->q[0], io, nio, n);
	default:
		panic("zpread");
	}
	return -1;	/* not reached */
}


/*
 *  a write to a closed zp should cause a note to be sent to
 *  the process.
 *  If the data is already in a SG_ZIO segment, we shouldn't
 *  be copying it again, probably.
 */
static int32_t
zpwrite(Chan *c, void *va, int32_t n, int64_t mm)
{
	Proc *up = externup();
	ZPipe *p;
	Kzio io;	/* might write less than we could */
	int32_t tot, nw;
	Segment *s;
	Zq *q;
	char *cp;

	if(n <= 0)
		return n;
	p = c->aux;
	switch(ZPTYPE(c->qid.path)){
	case Qdata0:
		q = &p->q[1];
		break;
	case Qdata1:
		q = &p->q[0];
		break;
	default:
		q = nil;
		panic("zpwrite");
	}

	s = getzkseg();
	if(waserror()){
		putseg(s);
		nexterror();
	}
	cp = va;
	for(tot = 0; tot < n; tot += nw){
		nw = n;
		if(nw > Maxatomic)
			nw = Maxatomic;
		io.Zio.data = alloczio(s, nw);
		memmove(io.Zio.data, cp + tot, nw);
		io.seg = s;
		incref(&s->r);
		io.Zio.size = nw;
		DBG("zpwrite: copy %Z %#p\n", &io, cp+tot);
		zqwrite(q, &io, 1);
	}
	poperror();
	putseg(s);
	return n;
}

static int
zpzwrite(Chan *c, Kzio io[], int nio, int64_t mm)
{
	ZPipe *p;

	p = c->aux;

	switch(ZPTYPE(c->qid.path)){
	case Qdata0:
		zqwrite(&p->q[1], io, nio);
		break;

	case Qdata1:
		zqwrite(&p->q[0], io, nio);
		break;

	default:
		panic("zpwrite");
	}

	return nio;
}


Dev zpdevtab = {
	L'∏',
	"zp",

	devreset,
	devinit,
	devshutdown,
	zpattach,
	zpwalk,
	zpstat,
	zpopen,
	devcreate,
	zpclose,
	zpread,
	devbread,
	zpwrite,
	devbwrite,
	devremove,
	devwstat,
	nil,		/* power */
	nil,		/* config */
	zpzread,
	zpzwrite,
};
