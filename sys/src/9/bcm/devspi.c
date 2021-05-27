/*
 * minimal spi interface for testing
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define SPIREGS	(VIRTIO+0x204000)

extern int qstate(Queue*);

enum {
	QMAX		= 64*1024,
	Nspislave	= 2,
};

typedef struct Spi Spi;

struct Spi {
	int	csel;
	int	opens;
	QLock;
	Queue	*iq;
	Queue	*oq;
};

Spi spidev[Nspislave];

enum{
	Qdir = 0,
	Qctl,
	Qspi,
};

Dirtab spidir[]={
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"spictl",		{Qctl, 0},	0,	0664,
	"spi0",		{Qspi+0, 0},	0,	0664,
	"spi1",		{Qspi+1, 0}, 0, 0664,
};

#define DEVID(path)	((ulong)path - Qspi)

enum {
	CMclock,
	CMmode,
	CMlossi,
};

Cmdtab spitab[] = {
	{CMclock, "clock", 2},
	{CMmode, "mode", 2},
	{CMlossi, "lossi", 1},
};

static void
spikick(void *a)
{
	Block *b;
	Spi *spi;

	spi = a;
	b = qget(spi->oq);
	if(b == nil)
		return;
	if(waserror()){
		freeb(b);
		nexterror();
	}
	spirw(spi->csel, b->rp, BLEN(b));
	qpass(spi->iq, b);
	poperror();
}

static void
spiinit(void)
{
}

static long
spiread(Chan *c, void *a, long n, vlong off)
{
	Spi *spi;
	u32int *sp;
	char *p, *e;
	char buf[256];

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, spidir, nelem(spidir), devgen);

	if(c->qid.path == Qctl) {
		sp = (u32int *)SPIREGS;
		p = buf;
		e = p + sizeof(buf);
		p = seprint(p, e, "CS: %08x\n", sp[0]);
		p = seprint(p, e, "CLK: %08x\n", sp[2]);
		p = seprint(p, e, "DLEN: %08x\n", sp[3]);
		p = seprint(p, e, "LTOH: %08x\n", sp[4]);
		seprint(p, e, "DC: %08x\n", sp[5]);
		return readstr(off, a, n, buf);
	}

	spi = &spidev[DEVID(c->qid.path)];
	n = qread(spi->iq, a, n);

	return n;
}

static long
spiwrite(Chan*c, void *a, long n, vlong)
{
	Spi *spi;
	Cmdbuf *cb;
	Cmdtab *ct;

	if(c->qid.type & QTDIR)
		error(Eperm);

	if(c->qid.path == Qctl) {
		cb = parsecmd(a, n);
		if(waserror()) {
			free(cb);
			nexterror();
		}
		ct = lookupcmd(cb, spitab, nelem(spitab));
		switch(ct->index) {
		case CMclock:
			spiclock(atoi(cb->f[1]));
			break;
		case CMmode:
			spimode(atoi(cb->f[1]));
			break;
		case CMlossi:
			break;
		}
		poperror();
		return n;
	}

	spi = &spidev[DEVID(c->qid.path)];
	n = qwrite(spi->oq, a, n);

	return n;
}

static Chan*
spiattach(char* spec)
{
	return devattach(L'π', spec);
}

static Walkqid*	 
spiwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, spidir, nelem(spidir), devgen);
}

static int	 
spistat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, spidir, nelem(spidir), devgen);
}

static Chan*
spiopen(Chan* c, int omode)
{
	Spi *spi;

	c = devopen(c, omode, spidir, nelem(spidir), devgen);
	if(c->qid.type & QTDIR)
		return c;

	spi = &spidev[DEVID(c->qid.path)];
	qlock(spi);
	if(spi->opens++ == 0){
		spi->csel = DEVID(c->qid.path);
		if(spi->iq == nil)
			spi->iq = qopen(QMAX, 0, nil, nil);
		else
			qreopen(spi->iq);
		if(spi->oq == nil)
			spi->oq = qopen(QMAX, Qkick, spikick, spi);
		else
			qreopen(spi->oq);
	}
	qunlock(spi);
	c->iounit = qiomaxatomic;
	return c;
}

static void	 
spiclose(Chan *c)
{
	Spi *spi;

	if(c->qid.type & QTDIR)
		return;
	if((c->flag & COPEN) == 0)
		return;
	spi = &spidev[DEVID(c->qid.path)];
	qlock(spi);
	if(--spi->opens == 0){
		qclose(spi->iq);
		qhangup(spi->oq, nil);
		qclose(spi->oq);
	}
	qunlock(spi);
}

Dev spidevtab = {
	L'π',
	"spi",

	devreset,
	spiinit,
	devshutdown,
	spiattach,
	spiwalk,
	spistat,
	spiopen,
	devcreate,
	spiclose,
	spiread,
	devbread,
	spiwrite,
	devbwrite,
	devremove,
	devwstat,
};

