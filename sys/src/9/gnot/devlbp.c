#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	<libg.h>
#include	<gnot.h>

#include	"devtab.h"

#define	GSHORT(p)		(((p)[0]<<0) | ((p)[1]<<8))
#define	GLONG(p)		((GSHORT(p)<<0) | (GSHORT(p+2)<<16))
#define	PSHORT(p, v)		((p)[0]=(v), (p)[1]=((v)>>8))
#define	PLONG(p, v)		(PSHORT(p, (v)), PSHORT(p+2, (v)>>16))

#define	ISKVA(va)	((ulong)(va)&KZERO)

typedef struct Lbpfifo {
	unsigned long fifo;
	unsigned short csr;
	unsigned short hblank;
} Lbpfifo;

enum Csrin {
	BINT = 0x0001, VEND = 0x0002, EFL = 0x0004, HFL = 0x0008,
	FFL = 0x0010, VSTARTL = 0x0020,
	PPRDYL = 0x0100, RDY = 0x0200, SBSY = 0x0400, SCIN = 0x0800,
	SCLKIN = 0x1000, VSREQ = 0x2000,
};

enum Csrout {
	GO = 0x0001, EOL = 0x0002, EFINTEN = 0x0004, HFINTEN = 0x0008,
	VCLRL = 0x0010, FOURHDPI = 0x0020,
	CPRDYL = 0x0100, PRNT = 0x0200, CBSY = 0x0400, SC = 0x0800,
	SCLK = 0x1000, VSYNC = 0x2000,
};

#define DPRINT	kprint

#define	LBPDEV	((Lbpfifo *)&PORT[56])

static ushort csrwval;

#define CSRW(val)	(dev->csr = csrwval  =  (val))
#define CSRSET(val)	(dev->csr = csrwval |=  (val))
#define CSRCLR(val)	(dev->csr = csrwval &= ~(val))

#define CMDW(val)	(csrwval &= ~(CBSY|SC|SCLK), CSRSET(val))

enum {
	Inited = 1,
	Cmdlocked=2,
	Bitlocked=4,
	Bitmapped=8
};

typedef struct Lbp {
	int	fifosize;
	ulong	flags;
	Lock	mlock;		/* monitor proc active */
	Rendez	ready;		/* to wake monitor proc */
	QLock	cmdlock;	/* to send commands */
	int	cmdreply;
	QLock	pagelock;	/* to print */
	Rendez	printing;	/* to wake printing proc */
	Rectangle rect;		/* page size */
	ulong *left, lmask, rmask;
	int nhblank, width, nvblank, nlines;
	Segment	*s;		/* bitmap may not span segments (BUG) */
	Page	*xmap[256];
	Page	**x, **xend;
	KMap	*kmap;
	int ihl, maxhl, lcount;
	long startclicks, stopclicks;
} Lbp;

enum Lbpsize {
	NHBLANK = 255, NVBLANK = 84,
	LBPXMAX = 2424, LBPYMAX = 3262, LBPDOTS = 300,
};

typedef struct Lbpconf {
	int nhblank, nvblank, xmax, ymax, res;
} Lbpconf;

static Lbpconf lbpiconf = {
	NHBLANK, NVBLANK, LBPXMAX, LBPYMAX, LBPDOTS,
};
static Lbpconf	lbpconf;
static Lbp	lbp;
static int	cprint0, cprint1;
static ulong	pingval;

enum {
	Qdir, Qconf, Qdcmd, Qrcmd, Qmon, Qping, Qbit, Qpage
};

Dirtab lbpdir[]={
	"conf",		{Qconf},	sizeof(Lbpconf), 0666,
	"dcmd",		{Qdcmd},	1,	0666,
	"rcmd",		{Qrcmd},	1,	0666,
	"mon",		{Qmon},		0,	0666,
	"ping",		{Qping},	0,	0666,
	"bit",		{Qbit},		0,	0666,
	"page",		{Qpage},	0,	0666
};
#define	NLBP	(sizeof lbpdir/sizeof(Dirtab))

static	int	lbpcmd(int);
static	long	lbppage(Point, GBitmap *, Rectangle);
static	void	lbpmap(Point, GBitmap *, Rectangle);
static	Page *	addr2page(ulong);
static	void	validbitmap(GBitmap *);
static	int	fillfifo(int);
static	int	lbpintr(void);

static void
lbpsetconf(Lbpconf *cp)
{
	lbpconf = *cp;
	lbp.rect.max.x = lbpconf.xmax;
	lbp.rect.max.y = lbpconf.ymax;
	return;
}

void
lbpreset(void)
{
	lbpsetconf(&lbpiconf);
}

void
lbpinit(void)
{}

Chan *
lbpattach(char *param)
{
	Lbpfifo *dev = LBPDEV;
	int i, hsize=0, fsize=0;

	addportintr(lbpintr);
	if(lbp.fifosize<=0){
		CSRCLR(VCLRL);
		CSRSET(VCLRL);
		for(i=0; i<20000; i+=4){
			dev->fifo = 0;
			if(!hsize && (dev->csr&HFL) == 0)
				hsize = i+4;
			if((dev->csr&FFL) == 0){
				fsize = i+4;
				break;
			}
		}
		/*pprint("lbpattach: hsize=%d, fsize=%d\n", hsize, fsize);*/
		lbp.fifosize = fsize;
	}
	return devattach('l', param);
}

Chan *
lbpclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
lbpwalk(Chan *c, char *name)
{
	return devwalk(c, name, lbpdir, NLBP, devgen);
}

void
lbpstat(Chan *c, char *db)
{
	devstat(c, db, lbpdir, NLBP, devgen);
}

Chan *
lbpopen(Chan *c, int omode)
{
	switch ((int)(c->qid.path & ~CHDIR)) {
	/*case Qcmd:*/
	case Qbit:
	case Qpage:
		if (!(lbp.flags & Inited)) {
			lbp.cmdreply = lbpcmd(0x40);
			lbp.flags |= Inited;
		}
		break;
	}
	return devopen(c, omode, lbpdir, NLBP, devgen);
}

void
lbpcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
lbpclose(Chan *c)
{
	USED(c);
}

long
lbpread(Chan *c, void *va, long n, ulong offset)
{
	uchar *a = (uchar *)va;
	if (n == 0)
		return 0;
	switch ((int)(c->qid.path & ~CHDIR)) {
	case Qdir:
		return devdirread(c, va, n, lbpdir, NLBP, devgen);
	case Qconf:
		if (offset >= 20)
			return 0;
		if (n < 20)
			error(Etoosmall);
		PLONG(&a[ 0], lbpconf.nhblank);
		PLONG(&a[ 4], lbpconf.nvblank);
		PLONG(&a[ 8], lbpconf.xmax);
		PLONG(&a[12], lbpconf.ymax);
		PLONG(&a[16], lbpconf.res);
		n = 20;
		break;
	case Qdcmd:
		*a = cprint1;
		n = 1;
		break;
	case Qrcmd:
		if (lbp.cmdreply > 0)
			*a = lbp.cmdreply, n = 1;
		else
			n = 0;
		break;
	case Qmon:
		if (canlock(&lbp.mlock)) {
			sleep(&lbp.ready, return0, 0);
			unlock(&lbp.mlock);
		} else
			error(Einuse);
		n = 0;
		break;
	case Qping:
		if (n != sizeof(ulong))
			n = 0;
		else
			*(ulong *)a = pingval;
		break;
	case Qbit:
	case Qpage:
		n = 0;
		break;
	}
	return n;
}

#define OKRECT(r)	((r).min.x<(r).max.x && (r).min.y<(r).max.y)
#define PTINRECT(p, r)	((r).min.x<=(p).x && (r).min.y<=(p).y && \
			 (p).x<=(r).max.x && (p).y<=(r).max.y)
#define RINRECT(r, s)	(PTINRECT((r).min, s) && PTINRECT((r).max, s))

long
lbpwrite(Chan *c, void *va, long n, ulong offset)
{
	uchar *a = (uchar *)va;
	extern GBitmap *id2bit(int);
	Point p; GBitmap *bp; Rectangle r;
	Lbpconf tc;
	int k, m = 0;
	if (n == 0)
		return 0;
	switch ((int)c->qid.path) {
	case Qconf:
		if (offset >= 20)
			return 0;
		if (n < 20)
			error(Etoosmall);
		tc.nhblank = GLONG(&a[ 0]);
		tc.nvblank = GLONG(&a[ 4]);
		tc.xmax    = GLONG(&a[ 8]);
		tc.ymax    = GLONG(&a[12]);
		tc.res     = GLONG(&a[16]);
		lbpsetconf(&tc);
		m = 20;
		break;
	case Qdcmd:
		cprint1 = *a ? PRNT : 0;
		m = 1;
		break;
	case Qrcmd:
		lbp.cmdreply = lbpcmd(*a);
		m = 1;
		break;
	case Qmon:
		wakeup(&lbp.ready);
		m = 0;
		break;
	case Qping:
		if (n != sizeof(ulong))
			m = 0;
		else {
			Lbpfifo *dev = LBPDEV;
			dev->fifo = *(ulong *)a;
			pingval = dev->fifo;
			m = sizeof(ulong);
		}
		break;
	case Qbit:
	case Qpage:
		if (n != 28)
			error(Ebadarg);
		p.x = GLONG(&a[0]);
		p.y = GLONG(&a[4]);
		if (c->qid.path == Qbit)
			bp = id2bit(GLONG(&a[8]));
		else {
			bp = (GBitmap *)GLONG(&a[8]);
			validbitmap(bp);
		}
		r.min.x = GLONG(&a[12]);
		r.min.y = GLONG(&a[16]);
		r.max.x = GLONG(&a[20]);
		r.max.y = GLONG(&a[24]);
		DPRINT("lbpwrite: ckpage...");
		if ((k = lbp.rect.min.x - p.x) > 0)
			r.min.x += k, p.x += k;
		if ((k = lbp.rect.min.y - p.y) > 0)
			r.min.y += k, p.y += k;
		if ((k = Dx(r) + p.x - lbp.rect.max.x) > 0)
			r.max.x -= k;
		if ((k = Dy(r) + p.y - lbp.rect.max.y) > 0)
			r.max.y -= k;
		if (!OKRECT(r) || !RINRECT(r, bp->r))
			error(Ebadblt);
		DPRINT("OK\n");
		m = lbppage(p, bp, r);
		/*pprint("lbpwrite: %d clicks\n", m);*/
		break;
	default:
		panic("lbpwrite");
	}
	return m;
}

void
lbpremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
lbpwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

static short poot, hoot;
#define MICROSECOND	(poot=0, hoot=0); USED(poot, hoot)
#define TLIMIT		0x3fffff

static void
lbpwait(int set, int clear, long timeout)
{
	Lbpfifo *dev = LBPDEV;
	int status; long i=0;

	if(timeout <= 0)
		timeout = TLIMIT;
	timeout += m->ticks;
	do{
		status = dev->csr;
		if((status & set) == set && (status & clear) == 0){
			DPRINT("OK lbpwait(pid=%d,i=%d)...", u->p->pid, i);
			return;
		}
		if((i++ % 32768) == 0)
			DPRINT("lbpwait(pid=%d,i=%d)...", u->p->pid, i-1);
		sched();
		if(u->p->notepending){
			u->p->notepending = 0;
			DPRINT("intr lbpwait(pid=%d,i=%d)\n", u->p->pid, i);
			error(Eintr);
		}
	}while(m->ticks < timeout);
	DPRINT("timeout lbpwait(pid=%d,i=%d)\n", u->p->pid, i);
	error(Eio);
}

static int
lbpcmd(int cmd)
{
	Lbpfifo *dev = LBPDEV;
	int mask = 0x80, status;

	qlock(&lbp.cmdlock);
	if (waserror()) {
		qunlock(&lbp.cmdlock);
		nexterror();
	}
	lbpwait(0, (PPRDYL|SBSY|SC|SCLK), 600);

	CMDW(CBSY);	/* cbsy asserted */
	do {
		if (cmd & mask) {
			CMDW(CBSY|SCLK|SC);	/* clock up, data 1 */
			MICROSECOND;
			CMDW(CBSY|SC);		/* clock down, data 1 */
			MICROSECOND;
		} else {
			CMDW(CBSY|SCLK);	/* clock up, data 0 */
			MICROSECOND;
			CMDW(CBSY);		/* clock down, data 0 */
			MICROSECOND;
		}
	} while (mask >>= 1);
	CMDW(0);	/* release cbsy, clock, data */

	lbpwait(SBSY, 0, 120);	/* wait for sbsy */
	sched();		/* wait >1 msec */

	status = 0;
	mask = 0x80; do {
		int temp;
		CMDW(SCLK); 	/* clock up */
		MICROSECOND;
		CMDW(0);	/* clock down */
		MICROSECOND;
		temp = dev->csr;
		if (temp & SC)
			status |= mask;
	} while (mask >>= 1);

	DPRINT("lbp: cmd = 0x%x, status = 0x%x\n", cmd, status);
	poperror();
	qunlock(&lbp.cmdlock);
	return status;
}

#define	WORDSIZE	32

/*extern ulong	_topbits[];*/

static ulong _topbits[]={
	0x00000000, 0x80000000, 0xc0000000, 0xe0000000,
	0xf0000000, 0xf8000000, 0xfc000000, 0xfe000000,
	0xff000000, 0xff800000, 0xffc00000, 0xffe00000,
	0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000,
	0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000,
	0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
	0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0,
	0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe,
	0xffffffff,
};

#define	PBASE(va)	(((ulong)(va)) & ~(BY2PG-1))
#define	POFFSET(va)	(((ulong)(va)) &  (BY2PG-1))

static void
lbpmap(Point p, GBitmap *bp, Rectangle r)
{
	ulong *left, *right; ulong j;
	int i, lbits, rbits;

	if ((lbits = r.min.x%WORDSIZE) < 0)
		lbits += WORDSIZE;
	if ((rbits = r.max.x%WORDSIZE) < 0)
		rbits += WORDSIZE;
	lbp.lmask = lbits ? ~_topbits[lbits] : 0;
	lbp.rmask = rbits ? _topbits[rbits] : 0;
	lbp.nhblank = 0xff00-(lbpconf.nhblank - lbits + p.x);
	if (lbp.nhblank < 0)
		lbp.nhblank = 0;
	else if (lbp.nhblank > 0xfeff)
		lbp.nhblank = 0xfeff;
	left = gaddr(bp, r.min);
	right = gaddr(bp, Pt(r.max.x-1, r.min.y));
	lbp.left = left;
	lbp.maxhl = right - left;
	lbp.width = bp->width;
	lbp.nvblank = lbpconf.nvblank + p.y;
	lbp.nlines = lbp.nvblank + Dy(r);
	DPRINT("lbpmap: nvblank=%d, nlines=%d\n", lbp.nvblank, lbp.nlines);
	lbp.ihl = 0;
	lbp.lcount = 0;

	lbp.x = lbp.xend = lbp.xmap;
	lbp.s = 0;
	if (ISKVA(left))
		return;
	lbp.s = seg(u->p, (ulong)left, 1);
	if (!lbp.s)
		error(Ebadarg);
	lbp.s->steal++;
	qunlock(&lbp.s->lk);
	if (waserror()){
		qlock(&lbp.s->lk);
		lbp.s->steal--;
		qunlock(&lbp.s->lk);
		error(Ebadarg);
	}

	for (i=r.min.y; i<r.max.y; i++, left+=bp->width, right+=bp->width) {
		for (j=PBASE(left); j<=PBASE(right); j+=BY2PG) {
			if (lbp.xend > lbp.xmap && j <= lbp.xend[-1]->va)
				continue;
			/*DPRINT("probing 0x%lux\n", j);*/
			*lbp.xend++ = addr2page(j);
		}
	}
	poperror();
}

Page *
addr2page(ulong addr)
{
	Segment *s; long soff; Pte *pte; Page *pg = 0;

	poot = *(ulong *)addr;	/* fault it if necessary */
	s = seg(u->p, addr, 1);
	if(s != lbp.s)
		error(Ebadarg);
	soff = addr-s->base;
	pte = s->map[soff/PTEMAPMEM];
	if(pte)
		pg = pte->pages[(soff&(PTEMAPMEM-1))/BY2PG];

	if(pg == 0){
		qunlock(&s->lk);
		pprint("nonresident page addr %lux (complain to rob)\n", addr);
		error(Eio);
	}
	qunlock(&s->lk);
	return pg;
}

static void
validbitmap(GBitmap *bp)
{
	ulong tl, br;
	validaddr((ulong)bp, sizeof(GBitmap), 0);
	if (((ulong)bp->base&3) || bp->width<=0 || bp->ldepth!=0 || !OKRECT(bp->r))
		error(Ebadbitmap);
	tl = (ulong)gaddr(bp, bp->r.min);
	br = (ulong)gaddr(bp, bp->r.max);
	validaddr(tl, br-tl, 0);
}

static long
lbppage(Point p, GBitmap *bp, Rectangle r)
{
	extern QLock bitlock;
	Lbpfifo *dev = LBPDEV;
	long dt;

	if (csrwval & (PRNT|GO)) {
		cprint0 = PRNT;
		if (!(csrwval & PRNT))
			CSRSET(PRNT);
	}
	DPRINT("lbppage: qlock %d...\n", u->p->pid);
	qlock(&lbp.pagelock);
	DPRINT("...%d OK\n", u->p->pid);
	if(waserror()){
		if(lbp.flags & Bitmapped){
			lbp.flags &= ~Bitmapped;
			kunmap(lbp.kmap);
			qlock(&lbp.s->lk);
			lbp.s->steal--;
			qunlock(&lbp.s->lk);
		}
		if(lbp.flags & Bitlocked){
			lbp.flags &= ~Bitlocked;
			qunlock(&bitlock);
		}
		if(lbp.flags & Cmdlocked){
			lbp.flags &= ~Cmdlocked;
			qunlock(&lbp.cmdlock);
		}
		qunlock(&lbp.pagelock);
		nexterror();
	}
	cprint0 = lbp.pagelock.head ? PRNT : 0;

	if (dev->csr & PPRDYL) {
		CSRW(0);
		pprint("printer power not ready\n");
		error(Eio);
	}
	if (!(dev->csr & RDY)) {
		DPRINT("lbpprt: not ready, waiting...");
		wakeup(&lbp.ready);
		lbpwait(RDY, 0, 0x7fffffffL);	/* wait for rdy */
		DPRINT("OK\n");
	}

	qlock(&lbp.cmdlock);
	lbp.flags |= Cmdlocked;
	if(ISKVA(bp->base)){
		qlock(&bitlock);
		lbp.flags |= Bitlocked;
	}
	lbpmap(p, bp, r);
	if(!ISKVA(bp->base)){
		lbp.kmap = kmap(lbp.xmap[0]);
		lbp.flags |= Bitmapped;
	}
	DPRINT("lbpprt: filling fifo...");
	CSRCLR(VCLRL);			/* clear fifo */
	CSRSET(VCLRL);
	dev->hblank = lbp.nhblank;	/* left margin */
	if (fillfifo(lbp.fifosize))		/* did all the data fit ? */
		CSRSET(EFINTEN);	/* yes, interrupt when fifo empty */
	else
		CSRSET(HFINTEN|EFINTEN);/* no, interrupt when half-empty */
	DPRINT("OK\n");
	CSRSET(PRNT);		/* assert prnt */
	DPRINT("(PRNT 0x%4.4x)...", csrwval);
	lbpwait(VSREQ, 0, 0L);	/* wait for vsreq */
	DPRINT("VSREQ+...");

	DPRINT("(VSYNC)...");
	CSRSET(VSYNC);		/* now give him vsync */

	lbpwait(0, VSREQ, 0L);	/* wait for ack (vsreq goes away) */
	DPRINT("VSREQ-...");
	csrwval &= ~(FOURHDPI|PRNT|VSYNC);
	if (lbpconf.res == 400)
		CSRSET(FOURHDPI|cprint0|cprint1|GO);
	else
		CSRSET(cprint0|cprint1|GO);
	DPRINT("lbpprt: ready, ticks=%d\n", m->ticks);
	qunlock(&lbp.cmdlock);
	lbp.flags &= ~Cmdlocked;
	lbp.startclicks = 0;
	lbp.stopclicks = 0;
	sleep(&lbp.printing, return0, 0);
	if(ISKVA(bp->base)){
		qunlock(&bitlock);
		lbp.flags &= ~Bitlocked;
	}else{
		kunmap(lbp.kmap);
		qlock(&lbp.s->lk);
		lbp.s->steal--;
		qunlock(&lbp.s->lk);
		lbp.flags &= ~Bitmapped;
	}
	dt = lbp.stopclicks - lbp.startclicks;
	DPRINT("lbpprt: lcount=%d, nlines=%d\n", lbp.lcount, lbp.nlines);
	DPRINT("lbpprt: start=%d, stop=%d, dt=%d\n",
		lbp.startclicks, lbp.stopclicks, dt);
	qunlock(&lbp.pagelock);
	poperror();
	return dt;
}

static int
fillfifo(int nfifo)
{
	Lbpfifo *dev = LBPDEV;
	ulong *up, *kp;
	int nout, npg;
	if (lbp.lcount < lbp.nvblank) {
		nout = lbp.nvblank - lbp.lcount;
		if (nout > nfifo)
			nout = nfifo;
		nfifo -= nout;
		lbp.lcount += nout;
		CSRSET(EOL);
		while (--nout >= 0)
			*(char *)&dev->fifo = 0;
		CSRCLR(EOL);
	}
	while (nfifo > sizeof(ulong)) {
		up = lbp.left + lbp.ihl;
		if (ISKVA(up))
			kp = up;
		else {
			while (PBASE(up) > (*lbp.x)->va)
				++lbp.x;
			lbp.kmap->pa = (*lbp.x)->pa;
			putkmmu(lbp.kmap->va, PPN(lbp.kmap->pa) | PTEVALID | PTEKERNEL);
			kp = (ulong *)(lbp.kmap->va + POFFSET(up));
		}
		if (lbp.ihl == 0 && lbp.lmask) {
			dev->fifo = *kp & lbp.lmask;
			++lbp.ihl, nfifo -= sizeof(ulong);
		} else if (lbp.ihl == lbp.maxhl && lbp.rmask) {
			dev->fifo = *kp & lbp.rmask;
			++lbp.ihl, nfifo -= sizeof(ulong);
		} else {
			nout = lbp.maxhl - lbp.ihl + (lbp.rmask == 0);
			npg = (BY2PG - POFFSET(up))/BY2WD;
			if (nout > npg)
				nout = npg;
			if (nout > nfifo/sizeof(ulong))
				nout = nfifo/sizeof(ulong);
			lbp.ihl += nout;
			nfifo -= nout*sizeof(ulong);
			while(--nout >= 0)
				dev->fifo = *kp++;
		}
		if (lbp.ihl > lbp.maxhl) {
			CSRSET(EOL);
			*(char *)&dev->fifo = 0;
			CSRCLR(EOL);
			--nfifo;
			lbp.ihl = 0;
			lbp.left += lbp.width;
			if (++lbp.lcount >= lbp.nlines)
				return 1;
		}
	}
	return 0;
}

/*static int
fillfifo(int nfifo)
{
	Lbpfifo *dev = LBPDEV;
	ulong *up;
	int nout, npg;
	if (lbp.lcount < lbp.nvblank) {
		nout = lbp.nvblank - lbp.lcount;
		if (nout > nfifo)
			nout = nfifo;
		nfifo -= nout;
		lbp.lcount += nout;
		CSRSET(EOL);
		while (--nout >= 0)
			*(char *)&dev->fifo = 0;
		CSRCLR(EOL);
	}
	while (nfifo > sizeof(ulong)) {
		up = lbp.left + lbp.ihl;
		if (lbp.ihl == 0 && lbp.lmask) {
			dev->fifo = *up & lbp.lmask;
			++lbp.ihl, nfifo -= sizeof(ulong);
		} else if (lbp.ihl == lbp.maxhl && lbp.rmask) {
			dev->fifo = *up & lbp.rmask;
			++lbp.ihl, nfifo -= sizeof(ulong);
		} else {
			nout = lbp.maxhl - lbp.ihl + (lbp.rmask == 0);
			if (nout > nfifo/sizeof(ulong))
				nout = nfifo/sizeof(ulong);
			lbp.ihl += nout;
			nfifo -= nout*sizeof(ulong);
			while (--nout >= 0)
				dev->fifo = *up++;
		}
		if (lbp.ihl > lbp.maxhl) {
			CSRSET(EOL);
			*(char *)&dev->fifo = 0;
			CSRCLR(EOL);
			--nfifo;
			lbp.ihl = 0;
			lbp.left += lbp.width;
			if (++lbp.lcount >= lbp.nlines)
				return 1;
		}
	}
	return 0;
}
*/

static int
lbpintr(void)
{
	Lbpfifo *dev = LBPDEV;
	ushort csr = dev->csr;
	if (!(csr & BINT)) {
		/*print("lbpintr: no BINT 0x%x\n", csr);*/
		return 1/*0*/;
	}
	if (!lbp.printing.p) {
		CSRCLR(HFINTEN|EFINTEN|GO);
		print("spurious lbpintr: 0x%4.4ux\n", csr);
		return 1;
	}
	if (!(csr & EFL)) {
		lbp.stopclicks = m->ticks;
		CSRCLR(HFINTEN|EFINTEN|GO);
		wakeup(&lbp.printing);
	} else if (csr & HFL) {
		if (!lbp.startclicks)
			lbp.startclicks = m->ticks;
		if (fillfifo(lbp.fifosize/2)) {	/* did all the data fit ? */
			csrwval &= ~HFINTEN;
			CSRSET(EFINTEN);	/* yes, interrupt when fifo empty */
		}
	} else {
		print("null lbpintr: 0x%4.4ux\n", csr);
	}
	return 1;
}
