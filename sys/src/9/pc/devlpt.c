#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

/* Centronix parallel (printer) port */

/* base addresses */
static int lptbase[] = {
	0x378,	/* lpt1 */
	0x3bc,	/* lpt2 */
	0x278	/* lpt3 (sic) */
};
#define NDEV	nelem(lptbase)
static int lptallocd[NDEV];

/* offsets, and bits in the registers */
enum
{
	Qdir=		0x8000,
	/* data latch register */
	Qdlr=		0x0,
	/* printer status register */
	Qpsr=		0x1,
	Fnotbusy=	0x80,
	Fack=		0x40,
	Fpe=		0x20,
	Fselect=	0x10,
	Fnoerror=	0x08,
	/* printer control register */
	Qpcr=		0x2,
	Fie=		0x10,
	Fselectin=	0x08,
	Finitbar=	0x04,
	Faf=		0x02,
	Fstrobe=	0x01,
	/* fake `data register' */
	Qdata=		0x3,
};

static int	lptready(void*);
static void	outch(int, int);
static void	lptintr(Ureg*, void*);

static Rendez	lptrendez;

Dirtab lptdir[]={
	".",	{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"dlr",	{Qdlr},			1,	0666,
	"psr",	{Qpsr},			5,	0444,
	"pcr",	{Qpcr},			0,	0222,
	"data",	{Qdata},		0,	0222,
};

static int
lptgen(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid qid;

	if(i == DEVDOTDOT){
		mkqid(&qid, Qdir, 0, QTDIR);
		devdir(c, qid, ".", 0, eve, 0555, dp);
		return 1;
	}
	i++; /* skip first element for . itself */
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	qid = tab->qid;
	qid.path &= ~Qdir;
	if(qid.path < Qdata)
		qid.path += lptbase[c->dev];
	qid.vers = c->dev;
	sprint(up->genbuf, "lpt%lud%s", c->dev+1, tab->name);
	devdir(c, qid, up->genbuf, tab->length, eve, tab->perm, dp);
	return 1;
}

static Chan*
lptattach(char *spec)
{
	Chan *c;
	int i  = (spec && *spec) ? strtol(spec, 0, 0) : 1;
	char name[5];
	static int set;

	if(!set){
		outb(lptbase[i-1]+Qpcr, 0);	/* turn off interrupts */
		set = 1;
		intrenable(IrqLPT, lptintr, 0, BUSUNKNOWN, "lpt");
	}
	if(i < 1 || i > NDEV)
		error(Ebadarg);
	if(lptallocd[i-1] == 0){
		int ecr;
		sprint(name, "lpt%d", i-1);
		if(ioalloc(lptbase[i-1], 3, 0, name) < 0)
			error("lpt port space in use");
		lptallocd[i-1] = 1;
		// Detect ECP - if found, put into PS/2 mode to suit style of driver
		ecr = lptbase[i-1] + 0x402;
		if ((inb(ecr) & 3) == 1) {
			outb(ecr, 0x34);
			if (inb(ecr) == 0x35) {
				outb(ecr, (inb(ecr) & 0x1f) | (1 << 5));
				if(ioalloc(ecr, 1, 0, name) < 0)
					error("lpt ecr port space in use");
			}
		}
	}
	c = devattach('L', spec);
	c->qid.path = Qdir;
	c->dev = i-1;
	return c;
}

static Walkqid*
lptwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, lptdir, nelem(lptdir), lptgen);
}

static int
lptstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, lptdir, nelem(lptdir), lptgen);
}

static Chan*
lptopen(Chan *c, int omode)
{
	return devopen(c, omode, lptdir, nelem(lptdir), lptgen);
}

static void
lptclose(Chan *)
{
}

static long
lptread(Chan *c, void *a, long n, vlong)
{
	char str[16];
	int size;
	ulong o;

	if(c->qid.path == Qdir)
		return devdirread(c, a, n, lptdir, nelem(lptdir), lptgen);
	size = sprint(str, "0x%2.2ux\n", inb(c->qid.path));
	o = c->offset;
	if(o >= size)
		return 0;
	if(o+n > size)
		n = size-c->offset;
	memmove(a, str+o, n);
	return n;
}

static long
lptwrite(Chan *c, void *a, long n, vlong)
{
	char str[16], *p;
	long base, k;

	if(n <= 0)
		return 0;
	if(c->qid.path != Qdata){
		if(n > sizeof str-1)
			n = sizeof str-1;
		memmove(str, a, n);
		str[n] = 0;
		outb(c->qid.path, strtoul(str, 0, 0));
		return n;
	}
	p = a;
	k = n;
	base = lptbase[c->dev];
	if(waserror()){
		outb(base+Qpcr, Finitbar);
		nexterror();
	}
	while(--k >= 0)
		outch(base, *p++);
	poperror();
	return n;
}

static void
outch(int base, int c)
{
	int status, tries;

	for(tries=0;; tries++) {
		status = inb(base+Qpsr);
		if(status&Fnotbusy)
			break;
		if((status&Fpe)==0 && (status&(Fselect|Fnoerror)) != (Fselect|Fnoerror))
			error(Eio);
		if(tries < 10)
			tsleep(&lptrendez, return0, nil, 1);
		else {
			outb(base+Qpcr, Finitbar|Fie);
			tsleep(&lptrendez, lptready, (void *)base, 100);
		}
	}
	outb(base+Qdlr, c);
	outb(base+Qpcr, Finitbar|Fstrobe);
	outb(base+Qpcr, Finitbar);
}

static int
lptready(void *base)
{
	return inb((int)base+Qpsr)&Fnotbusy;
}

static void
lptintr(Ureg *, void *)
{
	wakeup(&lptrendez);
}

Dev lptdevtab = {
	'L',
	"lpt",

	devreset,
	devinit,
	devshutdown,
	lptattach,
	lptwalk,
	lptstat,
	lptopen,
	devcreate,
	lptclose,
	lptread,
	devbread,
	lptwrite,
	devbwrite,
	devremove,
	devwstat,
};
