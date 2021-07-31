#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define	DPRINT	if(ldebug)kprint

/* Centronix parallel (printer) port */

typedef struct Plp	Plp;
typedef struct Sdesc	Sdesc;

struct Sdesc
{
	ulong	count;
	ulong	addr;		/* eox / addr:28 */
	ulong	next;
};

enum
{
	Eox=	1<<31		/* end of transmission */
};

struct Plp
{
	uchar	fill0[0xa8];	/* 0000 */
	ulong	bc;		/* 00a8 byte count */
	ulong	cbp; 		/* 00ac current buffer pointer */
	ulong	nbdp; 		/* 00b0 next buffer descriptor pointer */
	ulong	ctrl;		/* 00b4 control/timing register */
	ulong	ptr;		/* 00b8 fifo pointer */
	ulong	fifo;		/* 00bc fifo data */
 	uchar	fill[0x75];	/* 00c0 */
	uchar	ext;		/* 0135 external status/remote */
};

enum
{
			/* ctrl */
	Creset=		0x01,	/* reset parallel port */
	Cint=		0x02,	/* interrupt pending */
	Cpolarity=	0x04,	/* invert strobe polarity */
	Csoftack=	0x08,	/* soft acknowledge */
	Cread=		0x10,	/* dma parallel port to mem */
	Cignack=	0x20,	/* ignore ack input */
	Cflush=		0x40,	/* flush HPC PP buffers */
	Cdma=		0x80,	/* start dma */

			/* ext */
	Einit=		0x01,	/* drives INIT-, pin 16 */
	Efeed=		0x02,	/* drives AF-, pin 14 */
	Eerror=		0x01,	/* ERROR-, pin 15 */
	Eslctin=	0x02,	/* SLCTIN-, pin 17 (should be output?) */
	Epaper=		0x04,	/* PE-, pin 12 */
	Eselect=	0x08	/* SLCT, pin 13 */
};

/* strobe length
 *
 * 	dc = total time for strobe in 30 ns ticks (dc < 127)
 * 	fall = time until strobe falls in 30 ns ticks (time < dc)
 * 	rise = time until strobe rises again in 30 ns ticks (rise < dc)
 *
 *	---------------|             |---------------
 *		       |             |
 *                     |-------------|
 *      <---  fall --->
 *                     <---  rise --->
 *      <------------------  dc -------------------->
 */
#define	TS(t)	((((t)+29)/30)&0x7f)

#define	STROBE(dc, fall, rise)	\
		((TS(dc)<<24) | (TS(dc-fall)<<16) | (TS(dc-fall-rise)<<8))

#define	STMASK	0x7f7f7f00

enum{
	Qdir, Qcsr, Qctl, Qdata, Qqueue
};

Dirtab lptdir[]={
	"lpt1csr",	{Qcsr},		5,		0666,
	"lpt1ctl",	{Qctl},		0,		0222,
	"lpt1data",	{Qdata},	0,		0666,
	"lpt1queue",	{Qqueue},	0,		0444,
};
#define NLPT	(sizeof lptdir/sizeof lptdir[0])
#define	NDEV	1

#define	DEV	IO(Plp, HPC_0_ID_ADDR)

#define	NBUF	8

static int	ldebug;
static ulong	lctlbits;
static uchar *	lbuf;
static Sdesc *	ldesc;
static int	lfill;
static int	lempty;
static QLock	llock;
static Rendez	lrendez;
static Lock	datalock;
static Lock	devlock;

static void	lptflush(void);
static void	lptdma(int);
static int	lptinch(int);
static void	lptio(void*, int, int);
static void	resetlpt(void);
static int	returnx(void*);

typedef struct Qsleep	Qsleep;
typedef struct Qrendez	Qrendez;

struct Qrendez
{
	Qrendez *next;
	int	own;
	Rendez;
};

struct Qsleep
{
	Lock	use;
	int	locked;
	Qrendez *list;
	Qrendez *free;
};

static Qsleep	queuelock;

void
qwakeup(Qsleep *q)
{
	Qrendez *r;

	lock(&q->use);
	if(r = q->list){	/* assign = */
		lock(r);
		r->own |= 1;
		unlock(r);
		wakeup(r);
	}else
		q->locked = 0;
	unlock(&q->use);
}

void
qsleep(Qsleep *q)
{
	Qrendez *r, *rp, *rq;

	lock(&q->use);
	if(!q->locked){
		q->locked = u->p->pid;
		unlock(&q->use);
		return;
	}
	if(r = q->free)	/* assign = */
		q->free = r->next;
	else
		r = smalloc(sizeof(Qrendez));
	memset(r, 0, sizeof(Qrendez));
	if(rp = q->list){	/* assign = */
		while(rp->next)
			rp = rp->next;
		rp->next = r;
	}else
		q->list = r;
	unlock(&q->use);
	if(!waserror()){
		sleep(r, returnx, &r->own);
		poperror();
		lock(&q->use);
		lock(r);
	}else{
		lock(&q->use);
		lock(r);
		r->own |= 2;
	}
	for(rp=0,rq=q->list; rq; rp=rq,rq=rq->next)
		if(r == rq)
			break;
	if(rq == 0)
		panic("qsleep list");
	if(rp)
		rp->next = r->next;
	else
		q->list = r->next;
	r->next = q->free;
	q->free = r;
	switch(r->own){
	case 1:
		q->locked = u->p->pid;
		unlock(r);
		unlock(&q->use);
		return;
	case 2:
		unlock(r);
		unlock(&q->use);
		nexterror();
	case 3:
		unlock(r);
		unlock(&q->use);
		qwakeup(q);
		nexterror();
	}
	panic("qsleep own");
}

void
lptreset(void)
{
	Sdesc *lp;
	int i;

	lbuf = pxspanalloc(NBUF*BY2PG, BY2PG, 128*1024*1024);
	ldesc = pxspanalloc(NBUF*sizeof(Sdesc), BY2PG, 128*1024*1024);

	for(i=0; i<NBUF; i++){
		lp = &ldesc[i];
		lp->addr = Eox | PADDR(lbuf+i*BY2PG);
		lp->next = 0;
	}
	resetlpt();

	*IO(uchar, LIO_0_MASK) |= LIO_CENTR;
	lctlbits = STROBE(1500, 500, 500);
}

void
lptinit(void)
{}

Chan*
lptattach(char *spec)
{
	Chan *c;
	int i  = (spec && *spec) ? strtol(spec, 0, 0) : 1;

	if(i < 1 || i > NDEV)
		error(Ebadarg);
	c = devattach('L', spec);
	c->dev = i-1;
	return c;
}

Chan*
lptclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
lptwalk(Chan *c, char *name)
{
	return devwalk(c, name, lptdir, NLPT, devgen);
}

void
lptstat(Chan *c, char *dp)
{
	devstat(c, dp, lptdir, NLPT, devgen);
}

Chan*
lptopen(Chan *c, int omode)
{
	switch(c->qid.path){
	case Qdata:
		if(!canlock(&datalock))
			error(Einuse);
		break;
	case Qqueue:
		qsleep(&queuelock);
		break;
	}
	return devopen(c, omode, lptdir, NLPT, devgen);
}

void
lptcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
lptclose(Chan *c)
{
	if(c->flag&COPEN)switch(c->qid.path){
	case Qdata:
		unlock(&datalock);
		break;
	case Qqueue:
		qwakeup(&queuelock);
		break;
	}
}

void
lptremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
lptwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

long
lptread(Chan *c, char *a, long n, ulong offset)
{
	char buf[16];
	Plp *dev = DEV;
	int k, m;

	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, a, n, lptdir, NLPT, devgen);
	case Qcsr:
		sprint(buf, "0x%2.2ux\n", dev->ext);
		return readstr(offset, a, n, buf);
	case Qdata:
		qlock(&llock);
		if(waserror()){
			qunlock(&llock);
			nexterror();
		}
		m = 0;
		while(n > 0){
			k = n;
			if(k > BY2PG)
				k = BY2PG;
			lptio(a, k, Cread);
			a += k;
			m += k;
			n -= k;
		}
		poperror();
		qunlock(&llock);
		return m;
	case Qqueue:
		return 0;
	}
	panic("lptread");
	return 0;
}

long
lptwrite(Chan *c, char *a, long n, ulong offset)
{
	char buf[64], *p;
	Plp *dev = DEV;
	int k, m;

	USED(offset);
	switch(c->qid.path & ~CHDIR){
	case Qcsr:
		if(n > sizeof buf-1)
			n = sizeof buf-1;
		memmove(buf, a, n);
		buf[n] = 0;
		dev->ext = strtoul(buf, 0, 0);
		return n;
	case Qctl:
		if(n > sizeof buf-1)
			n = sizeof buf-1;
		memmove(buf, a, n);
		buf[n] = 0;
		if(p = strchr(buf, '\n'))	/* assign = */
			*p = 0;
		if(strcmp(buf, "debug") == 0 || strcmp(buf, "debon") == 0)
			ldebug = 1;
		else if(strcmp(buf, "deboff") == 0)
			ldebug = 0;
		else if(strcmp(buf, "+feed") == 0)
			dev->ext = Efeed | Einit;
		else if(strcmp(buf, "-feed") == 0)
			dev->ext = Einit;
		else if(strcmp(buf, "+ignack") == 0)
			lctlbits |= Cignack;
		else if(strcmp(buf, "-ignack") == 0)
			lctlbits &= ~Cignack;
		else if(strcmp(buf, "+softack") == 0)
			lctlbits |= Csoftack;
		else if(strcmp(buf, "-softack") == 0)
			lctlbits &= ~Csoftack;
		else if(strcmp(buf, "reset") == 0)
			resetlpt();
		else if(strcmp(buf, "softack") == 0){
			dev->ctrl |= Csoftack;
			dev->ctrl &= ~Csoftack;
		}
		else if(n > 6 && strncmp(buf, "strobe", 6) == 0){
			ulong dc=9999, fall=9999, rise=9999;
			p = &buf[6];
			if(*p)
				dc = strtoul(p, &p, 0);
			if(*p)
				fall = strtoul(p+1, &p, 0);
			if(*p)
				rise = strtoul(p+1, &p, 0);
			if(dc < 30*127 && fall+rise < dc){
				lctlbits &= ~STMASK;
				lctlbits |= STROBE(dc, rise, fall);
			}
		}else if(n > 4 && strncmp(buf, "inch", 4) == 0){
			ulong a=0;
			p = &buf[4];
			if(*p)
				a = strtoul(p, &p, 0);
			a = lptinch(a);
			USED(a);
		}
		else
			error(Ebadctl);
		return n;
	case Qdata:
		qlock(&llock);
		if(waserror()){
			qunlock(&llock);
			nexterror();
		}
		m = 0;
		while(n > 0){
			k = n;
			if(k > BY2PG)
				k = BY2PG;
			lptio(a, k, 0);
			a += k;
			m += k;
			n -= k;
		}
		poperror();
		qunlock(&llock);
		return m;
	}
	panic("lptwrite");
	return 0;
}

static int
returnx(ulong *p)
{
	return *p;
}

static void
resetlpt(void)
{
	Plp *dev = DEV;
	int s;

	s = splhi();
	lock(&devlock);
	dev->ctrl &= ~Cdma;
	dev->ctrl |= Creset;	/* reset HPC logic */
	dev->ctrl &= ~Creset;
	unlock(&devlock);
	splx(s);

	dev->ext = Efeed;	/* reset printer */
	Xdelay(700);
	dev->ext = Efeed | Einit;

	dev->ctrl |= Csoftack;	/* generate softack */
	dev->ctrl &= ~Csoftack;

	lfill = 0;
	lempty = 0;
}

static int
lptinch(int c)
{
	Sdesc *dp = ldesc;
	Plp *dev = DEV;
	int s;

	*lbuf = c;
	dp->count = 1;
 	dev->nbdp = (ulong)dp;
	dev->ext = Efeed | Einit;
	s = splhi();
	lock(&devlock);
	dev->ctrl = Cdma | lctlbits;
	for(c=0; c<512; c++)
		if(!(dev->ctrl & Cdma))
			break;
	dev->ctrl &= ~Cdma;
	dev->ext = Einit;
	c = dev->ext;
	dev->ext = Efeed | Einit;
	c = ((c&0x0f)<<4) | (dev->ext&0x0f);
	unlock(&devlock);
	splx(s);
	DPRINT("lptinch: %d\n", c);
	return c;
}

static int
lnotfull(void *arg)
{
	USED(arg);
	return (lfill+1)%NBUF != lempty;
}

static int
lisempty(void *arg)
{
	USED(arg);
	return lfill == lempty;
}

static void
lptio(void *buf, int n, int rflag)
{
	Plp *dev = DEV;
	int s, lnext;
	Sdesc *dp;
	uchar *lb;

	DPRINT("lptio: lfill=%d, n=%d, rflag=0x%ux\n", lfill, n, rflag);
	lnext = (lfill+1)%NBUF;
	if(rflag){
		while(lfill != lempty)
			sleep(&lrendez, lisempty, 0);
	}else{
		if(lnext == lempty)
			sleep(&lrendez, lnotfull, 0);
	}
	dp = &ldesc[lfill];
	lb = lbuf + lfill*BY2PG;
	if(n > BY2PG)
		n = BY2PG;
	if(!rflag)
		memmove(lb, buf, n);
	dp->count = n;
	dp->addr = Eox | PADDR(lb);
	dp->next = 0;
	lfill = lnext;
	s = splhi();
	lock(&devlock);
	if(!(dev->ctrl & Cdma))
		lptdma(rflag);
	unlock(&devlock);
	splx(s);
	if(rflag){
		sleep(&lrendez, lisempty, 0);
		memmove(buf, lb, n);
	}
}

static void
lptdma(int rflag)
{
	Plp *dev = DEV;
	Sdesc *dp;

	if(lfill == lempty)
		return;
	dp = &ldesc[lempty];
	DPRINT("lptdma: lempty=%d, count=%d, addr=0x%ux, next=0x%ux\n",
		lempty, dp->count, dp->addr, dp->next);
	dev->cbp = 0x0BADBAD0;	/* ? */
	dev->bc = 0x1BAD;	/* ? */
 	dev->nbdp = (ulong)dp;
	DPRINT("lptdma: cbp=0x%ux, bc=%d, nbdp=0x%ux\n",
		dev->cbp, dev->bc, dev->nbdp);
	dev->ctrl = Cdma | rflag | lctlbits;
	wbflush();
	dev->ext = (rflag ? 0 : Efeed) | Einit;
}

static void
lptflush(void)
{
	Plp *dev = DEV;
	uchar *p;
	int fcnt;
	ulong fdata;

	if(dev->ctrl & Cread){	/* cf. scsi.c */
		fcnt = (dev->ptr>>16)&0x7f;
		if(0 < fcnt && fcnt < 4){
			fdata = dev->fifo;
			dev->ctrl &= ~(Cflush | Cdma);
			if(dev->nbdp == PADDR(ldesc))
				p = KADDR1(ldesc->addr);
			else
				p = KADDR1(dev->cbp);
			switch(fcnt){
			case 3:
				p[2] = fdata>>8;
			case 2:
				p[1] = fdata>>16;
			case 1:
				p[0] = fdata>>24;
			}
		}else{
			fcnt = 512;	/* pessimistic */
			dev->ctrl |= Cflush;
			while((dev->ctrl & Cdma) && --fcnt > 0) ;
			if(fcnt == 0)
				panic("lpt lptflush read");
			dev->ctrl &= ~(Cflush | Cdma);
		}
	}else
		dev->ctrl &= ~Cdma;
	lempty = (lempty+1)%NBUF;
}

void
lptintr(void)
{
	Plp *dev = DEV;

	dev->ext = Efeed | Einit;
	DPRINT("lptintr: ctrl=0x%ux\n", dev->ctrl);
	lock(&devlock);
	lptflush();
	if(lfill != lempty)
		lptdma(0);
	unlock(&devlock);
	if(lrendez.p)
		wakeup(&lrendez);
}
