/*
 * intel/amd ahci (advanced host controller interface) sata controller
 * bootstrap driver
 * copyright © 2007, 2008 coraid, inc.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "error.h"
#include "sd.h"
#include "ahci.h"

#define	dprint(...)	if(debug == 1)	print(__VA_ARGS__); else USED(debug)
#define	idprint(...)	if(prid == 1)	print(__VA_ARGS__); else USED(prid)
#define	aprint(...)	if(datapi == 1)	print(__VA_ARGS__); else USED(datapi)

enum {
	NCtlr	= 2,
	NCtlrdrv= 8,
	NDrive	= NCtlr*NCtlrdrv,

	Read	= 0,
	Write,
};

/* pci space configuration */
enum {
	Pmap	= 0x90,
	Ppcs	= 0x91,
	Prev	= 0xa8,
};

enum {
	Tesb,
	Tich,
	Tsb600,
	Tunk,
};

#define Intel(x)	((x)->pci->vid == 0x8086)

static char *tname[] = {
	"63xxesb",
	"ich",
	"sb600",
	"unk",
};

enum {
	Dnull,
	Dmissing,
	Dnew,
	Dready,
	Derror,
	Dreset,
	Doffline,
	Dportreset,
	Dlast,
};

static char *diskstates[Dlast] = {
	"null",
	"missing",
	"new",
	"ready",
	"error",
	"reset",
	"offline",
	"portreset",
};

extern SDifc sdiahciifc;
typedef struct Ctlr Ctlr;

enum {
	DMautoneg,
	DMsatai,
	DMsataii,
};

static char *modename[] = {
	"auto",
	"satai",
	"sataii",
};

typedef struct {
	Lock;

	Ctlr	*ctlr;
	SDunit	*unit;
	char	name[10];
	Aport	*port;
	Aportm	portm;
	Aportc	portc;		/* redundant ptr to port and portm. */

	uchar	mediachange;
	uchar	state;
	uchar	smartrs;

	uvlong	sectors;
	ulong	intick;
	ulong	lastseen;
	int	wait;
	uchar	mode;		/* DMautoneg, satai or sataii. */
	uchar	active;

	char	serial[20+1];
	char	firmware[8+1];
	char	model[40+1];

	ushort	info[0x200];

	int	driveno;	/* ctlr*NCtlrdrv + unit */
	int	portno;	/* ctlr port # != drive # when not all ports enabled. */
} Drive;

struct Ctlr {
	Lock;

	int	type;
	int	enabled;
	SDev	*sdev;
	Pcidev	*pci;

	uchar	*mmio;
	ulong	*lmmio;
	Ahba	*hba;

	Drive	rawdrive[NCtlrdrv];
	Drive*	drive[NCtlrdrv];
	int	ndrive;
	int	mport;
};

static	Ctlr	iactlr[NCtlr];
static	SDev	sdevs[NCtlr];
static	int	niactlr;

static	int	prid = 0;
static	int	datapi = 0;

static char stab[] = {
[0]	'i', 'm',
[8]	't', 'c', 'p', 'e',
[16]	'N', 'I', 'W', 'B', 'D', 'C', 'H', 'S', 'T', 'F', 'X'
};

static void
serrstr(ulong r, char *s, char *e)
{
	int i;

	e -= 3;
	for(i = 0; i < nelem(stab) && s < e; i++)
		if(r & (1<<i) && stab[i]){
			*s++ = stab[i];
			if(SerrBad & (1<<i))
				*s++ = '*';
		}
	*s = 0;
}

static char ntab[] = "0123456789abcdef";

static void
preg(uchar *reg, int n)
{
	int i;
	char buf[25*3+1], *e;

	e = buf;
	for(i = 0; i < n; i++){
		*e++ = ntab[reg[i] >> 4];
		*e++ = ntab[reg[i] & 0xf];
		*e++ = ' ';
	}
	*e++ = '\n';
	*e = 0;
	dprint(buf);
}

static void
dreg(char *s, Aport *p)
{
	dprint("ahci: %stask=%lux; cmd=%lux; ci=%lux; is=%lux\n",
		s, p->task, p->cmd, p->ci, p->isr);
}

static void
esleep(int ms)
{
	delay(ms);
}

typedef struct {
	Aport	*p;
	int	i;
} Asleep;

static int
ahciclear(void *v)
{
	Asleep *s;

	s = v;
	return (s->p->ci & s->i) == 0;
}

static void
aesleep(Aportm *, Asleep *a, int ms)
{
	ulong start;

	start = m->ticks;
	while((a->p->ci & a->i) != 0)
		if(TK2MS(m->ticks - start) >= ms)
			break;
}

static int
ahciwait(Aportc *c, int ms)
{
	Aport *p;
	Asleep as;

	p = c->p;
	p->ci = 1;
	as.p = p;
	as.i = 1;
	aesleep(c->m, &as, ms);
	if((p->task & 1) == 0 && p->ci == 0)
		return 0;
	dreg("ahciwait timeout ", c->p);
	return -1;
}

static int
setfeatures(Aportc *pc, uchar f)
{
	uchar *c;
	Actab *t;
	Alist *l;

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0xef;
	c[3] = f;
	c[7] = 0xa0;		/* obsolete device bits */

	l = pc->m->list;
	l->flags = Lwrite|0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	return ahciwait(pc, 500);
}

static int
setudmamode(Aportc *pc, uchar f)
{
	uchar *c;
	Actab *t;
	Alist *l;

	/* hack */
	if((pc->p->sig >> 16) == 0xeb14)
		return 0;

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0xef;
	c[3] = 3;		/* set transfer mode */
	c[7] = 0xa0;		/* obsolete device bits */
	c[12] = 0x40 | f;	/* sector count */

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	return ahciwait(pc, 500);
}

static void
asleep(int ms)
{
	delay(ms);
}

static int
ahciportreset(Aportc *c)
{
	ulong *cmd, i;
	Aport *p;

	p = c->p;
	cmd = &p->cmd;
	*cmd &= ~(Afre|Ast);
	for(i = 0; i < 500; i += 25){
		if((*cmd & Acr) == 0)
			break;
		asleep(25);
	}
	p->sctl = 1 | (p->sctl & ~7);
	delay(1);
	p->sctl &= ~7;
	return 0;
}

static ushort
gbit16(void *a)
{
	uchar *i;

	i = a;
	return i[1]<<8 | i[0];
}

static ulong
gbit32(void *a)
{
	ulong j;
	uchar *i;

	i = a;
	j  = i[3] << 24;
	j |= i[2] << 16;
	j |= i[1] << 8;
	j |= i[0];
	return j;
}

static uvlong
gbit64(void *a)
{
	uchar *i;

	i = a;
	return (uvlong) gbit32(i+4)<<32 | gbit32(a);
}

static int
ahciidentify0(Aportc *pc, void *id, int atapi)
{
	uchar *c;
	Actab *t;
	Alist *l;
	Aprdt *p;
	static uchar tab[] = { 0xec, 0xa1, };

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = tab[atapi];
	c[7] = 0xa0;		/* obsolete device bits */

	l = pc->m->list;
	l->flags = 1<<16 | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	memset(id, 0, 0x100);
	p = &t->prdt;
	p->dba = PCIWADDR(id);
	p->dbahi = 0;
	p->count = 1<<31 | (0x200-2) | 1;

	return ahciwait(pc, 500);
}

static vlong
ahciidentify(Aportc *pc, ushort *id)
{
	int i, sig;
	vlong s;
	Aportm *m;

	m = pc->m;
	m->feat = 0;
	m->smart = 0;
	i = 0;
	sig = pc->p->sig >> 16;
	if(sig == 0xeb14){
		m->feat |= Datapi;
		i = 1;
	}
	if(ahciidentify0(pc, id, i) == -1)
		return -1;

	i = gbit16(id+83) | gbit16(id+86);
	if(i & (1<<10)){
		m->feat |= Dllba;
		s = gbit64(id+100);
	}else
		s = gbit32(id+60);

	if(m->feat & Datapi){
		i = gbit16(id+0);
		if(i & 1)
			m->feat |= Datapi16;
	}

	i = gbit16(id+83);
	if((i>>14) == 1) {
		if(i & (1<<3))
			m->feat |= Dpower;
		i = gbit16(id+82);
		if(i & 1)
			m->feat |= Dsmart;
		if(i & (1<<14))
			m->feat |= Dnop;
	}
	return s;
}

static int
ahciquiet(Aport *a)
{
	ulong *p, i;

	p = &a->cmd;
	*p &= ~Ast;
	for(i = 0; i < 500; i += 50){
		if((*p & Acr) == 0)
			goto stop;
		asleep(50);
	}
	return -1;
stop:
	if((a->task & (ASdrq|ASbsy)) == 0){
		*p |= Ast;
		return 0;
	}

	*p |= Aclo;
	for(i = 0; i < 500; i += 50){
		if((*p & Aclo) == 0)
			goto stop1;
		asleep(50);
	}
	return -1;
stop1:
	/* extra check */
	dprint("ahci: clo clear %lx\n", a->task);
	if(a->task & ASbsy)
		return -1;
	*p |= Ast;
	return 0;
}

static int
ahciidle(Aport *port)
{
	ulong *p, i, r;

	p = &port->cmd;
	if((*p & Arun) == 0)
		return 0;
	*p &= ~Ast;
	r = 0;
	for(i = 0; i < 500; i += 25){
		if((*p & Acr) == 0)
			goto stop;
		asleep(25);
	}
	r = -1;
stop:
	if((*p & Afre) == 0)
		return r;
	*p &= ~Afre;
	for(i = 0; i < 500; i += 25){
		if((*p & Afre) == 0)
			return 0;
		asleep(25);
	}
	return -1;
}

/*
 * §6.2.2.1  first part; comreset handled by reset disk.
 *	- remainder is handled by configdisk.
 *	- ahcirecover is a quick recovery from a failed command.
 */
int
ahciswreset(Aportc *pc)
{
	int i;

	i = ahciidle(pc->p);
	pc->p->cmd |= Afre;
	if(i == -1)
		return -1;
	if(pc->p->task & (ASdrq|ASbsy))
		return -1;
	return 0;
}

int
ahcirecover(Aportc *pc)
{
	ahciswreset(pc);
	pc->p->cmd |= Ast;
	if(setudmamode(pc, 5) == -1)
		return -1;
	return 0;
}

static void*
malign(int size, int align)
{
	void *v;

	v = xspanalloc(size, align, 0);
	memset(v, 0, size);
	return v;
}

static void
setupfis(Afis *f)
{
	f->base = malign(0x100, 0x100);
	f->d = f->base + 0;
	f->p = f->base + 0x20;
	f->r = f->base + 0x40;
	f->u = f->base + 0x60;
	f->devicebits = (ulong*)(f->base + 0x58);
}

static void
ahciwakeup(Aport *p)
{
	ushort s;

	s = p->sstatus;
	if((s & 0xF00) != 0x600)
		return;
	if((s & 7) != 1){		/* not (device, no phy) */
		print("ahci: slumbering drive unwakeable %ux\n", s);
		return;
	}
	p->sctl = 3*Aipm | 0*Aspd | Adet;
	delay(1);
	p->sctl &= ~7;
//	print("ahci: wake %ux -> %ux\n", s, p->sstatus);
}

static int
ahciconfigdrive(Ahba *h, Aportc *c, int mode)
{
	Aportm *m;
	Aport *p;

	p = c->p;
	m = c->m;

	if(m->list == 0){
		setupfis(&m->fis);
		m->list = malign(sizeof *m->list, 1024);
		m->ctab = malign(sizeof *m->ctab, 128);
	}

	if(p->sstatus & 3 && h->cap & Hsss){
		dprint("ahci: configdrive: spinning up ... [%lux]\n",
			p->sstatus);
		p->cmd |= Apod|Asud;
		asleep(400);
	}

	p->serror = SerrAll;

	p->list = PCIWADDR(m->list);
	p->listhi = 0;
	p->fis = PCIWADDR(m->fis.base);
	p->fishi = 0;
	p->cmd |= Afre | Ast;

	if((p->sstatus & 0xF0F) == 0x601) /* drive coming up in slumbering? */
		ahciwakeup(p);

	/* disable power managment sequence from book. */
	p->sctl = (3*Aipm) | (mode*Aspd) | (0*Adet);
	p->cmd &= ~Aalpe;

	p->ie = IEM;

	return 0;
}

static int
ahcienable(Ahba *h)
{
	h->ghc |= Hie;
	return 0;
}

static int
ahcidisable(Ahba *h)
{
	h->ghc &= ~Hie;
	return 0;
}

static int
countbits(ulong u)
{
	int i, n;

	n = 0;
	for(i = 0; i < 32; i++)
		if(u & (1<<i))
			n++;
	return n;
}

static int
ahciconf(Ctlr *c)
{
	ulong u;
	Ahba *h;
	static int count;

	h = c->hba = (Ahba*)c->mmio;
	u = h->cap;

	if((u & Hsam) == 0)
		h->ghc |= Hae;

	print("ahci%d port %#p: hba sss %ld; ncs %ld; coal %ld; mports %ld; "
		"led %ld; clo %ld; ems %ld;\n", count++, h,
		(u>>27) & 1, (u>>8) & 0x1f, (u>>7) & 1,	u & 0x1f, (u>>25) & 1,
		(u>>24) & 1, (u>>6) & 1);
	return countbits(h->pi);
}

static int
ahcihbareset(Ahba *h)
{
	int wait;

	h->ghc |= 1;
	for(wait = 0; wait < 500; wait += 100){
		if(h->ghc == 0)
			return 0;
		delay(100);
	}
	return -1;
}

static void
idmove(char *p, ushort *a, int n)
{
	int i;
	char *op, *e;

	op = p;
	for(i = 0; i < n/2; i++){
		*p++ = a[i] >> 8;
		*p++ = a[i];
	}
	*p = 0;
	while(p > op && *--p == ' ')
		*p = 0;
	e = p;
	for (p = op; *p == ' '; p++)
		;
	memmove(op, p, n - (e - p));
}

static char*
dnam(Drive *d)
{
	char *s;

	s = d->name;
	if(d->unit && d->unit->name)
		s = d->unit->name;
	return s;
}

static int
identify(Drive *d)
{
	ushort *id;
	vlong osectors, s;
	uchar oserial[21];
	SDunit *u;

	id = d->info;
	s = ahciidentify(&d->portc, id);
	if(s == -1){
		d->state = Derror;
		return -1;
	}
	osectors = d->sectors;
	memmove(oserial, d->serial, sizeof d->serial);

	d->sectors = s;
	d->smartrs = 0;

	idmove(d->serial, id+10, 20);
	idmove(d->firmware, id+23, 8);
	idmove(d->model, id+27, 40);

	u = d->unit;
	memset(u->inquiry, 0, sizeof u->inquiry);
	u->inquiry[2] = 2;
	u->inquiry[3] = 2;
	u->inquiry[4] = sizeof u->inquiry - 4;
	memmove(u->inquiry+8, d->model, 40);

	if(osectors != s || memcmp(oserial, d->serial, sizeof oserial) != 0){
		d->mediachange = 1;
		u->sectors = 0;
	}

	return 0;
}

static void
clearci(Aport *p)
{
	if(p->cmd & Ast) {
		p->cmd &= ~Ast;
		p->cmd |= Ast;
	}
}

static void
updatedrive(Drive *d)
{
	ulong cause, serr, s0, pr, ewake;
	char *name;
	Aport *p;
	static ulong last;

	pr = 1;
	ewake = 0;
	p = d->port;
	cause = p->isr;
	serr = p->serror;
	p->isr = cause;
	name = "??";
	if(d->unit && d->unit->name)
		name = d->unit->name;

	if(p->ci == 0){
		d->portm.flag |= Fdone;
		pr = 0;
	}else if(cause & Adps)
		pr = 0;
	if(cause&Ifatal){
		ewake = 1;
		dprint("ahci: updatedrive: fatal\n");
	}
	if(cause & Adhrs){
		if(p->task & (1<<5|1)){
			dprint("ahci: Adhrs cause %lux serr %lux task %lux\n",
				cause, serr, p->task);
			d->portm.flag |= Ferror;
			ewake = 1;
		}
		pr = 0;
	}

	if(pr)
		dprint("%s: upd %lux ta %lux\n", name, cause, p->task);
	if(cause & (Aprcs|Aifs)){
		s0 = d->state;
		switch(p->sstatus & 7){
		case 0:				/* no device */
			d->state = Dmissing;
			break;
		case 1:				/* device but no phy comm. */
			if((p->sstatus & 0xF00) == 0x600)
				d->state = Dnew; /* slumbering */
			else
				d->state = Derror;
			break;
		case 3:				/* device & phy comm. estab. */
			/* power mgnt crap for surprise removal */
			p->ie |= Aprcs|Apcs;	/* is this required? */
			d->state = Dreset;
			break;
		case 4:				/* phy off-line */
			d->state = Doffline;
			break;
		}
		dprint("%s: %s → %s [Apcrs] %lux\n", name,
			diskstates[s0], diskstates[d->state], p->sstatus);
		/* print pulled message here. */
		if(s0 == Dready && d->state != Dready)
			idprint("%s: pulled\n", name);
		if(d->state != Dready)
			d->portm.flag |= Ferror;
		ewake = 1;
	}
	p->serror = serr;
	if(ewake)
		clearci(p);
	last = cause;
}

static void
pstatus(Drive *d, ulong s)
{
	/*
	 * bogus code because the first interrupt is currently dropped.
	 * likely my fault.  serror may be cleared at the wrong time.
	 */
	switch(s){
	case 0:			/* no device */
		d->state = Dmissing;
		break;
	case 1:			/* device but no phy. comm. */
		break;
	case 2:			/* should this be missing?  need testcase. */
		dprint("ahci: pstatus 2\n");
		/* fallthrough */
	case 3:			/* device & phy. comm. */
		d->wait = 0;
		d->state = Dnew;
		break;
	case 4:			/* offline */
		d->state = Doffline;
		break;
	case 6:			/* ? not sure this makes sense. TODO */
		d->state = Dnew;
		break;
	}
}

static int
configdrive(Drive *d)
{
	if(ahciconfigdrive(d->ctlr->hba, &d->portc, d->mode) == -1)
		return -1;
	ilock(d);
	pstatus(d, d->port->sstatus & 7);
	iunlock(d);
	return 0;
}

static void
resetdisk(Drive *d)
{
	uint state, det, stat;
	Aport *p;

	p = d->port;
	det = p->sctl & 7;
	stat = p->sstatus & 7;
	state = (p->cmd>>28) & 0xf;
	dprint("ahci: resetdisk: icc %ux  det %d sdet %d\n", state, det, stat);
	if(stat != 3){
		ilock(d);
		d->state = Dportreset;
		iunlock(d);
		return;
	}
	ilock(d);
	state = d->state;
	if(d->state != Dready || d->state != Dnew)
		d->portm.flag |= Ferror;
	clearci(p);			/* satisfy sleep condition. */
	iunlock(d);

	qlock(&d->portm);

	if(p->cmd & Ast && ahciswreset(&d->portc) == -1){
		ilock(d);
		d->state = Dportreset;	/* get a bigger stick. */
		iunlock(d);
	} else {
		ilock(d);
		d->state = Dmissing;
		iunlock(d);

		configdrive(d);
	}
	dprint("resetdisk: %s → %s\n", diskstates[state], diskstates[d->state]);
	qunlock(&d->portm);
}

static int
newdrive(Drive *d)
{
	char *name;
	Aportc *c;
	Aportm *m;

	c = &d->portc;
	m = &d->portm;

	name = d->unit->name;
	if(name == 0)
		name = "??";

	if(d->port->task == 0x80)
		return -1;
	qlock(c->m);
	if(setudmamode(c, 5) == -1){
		dprint("%s: can't set udma mode\n", name);
		goto lose;
	}
	if(identify(d) == -1){
		dprint("%s: identify failure\n", name);
		goto lose;
	}
	if(m->feat & Dpower && setfeatures(c, 0x85) == -1){
		m->feat &= ~Dpower;
		if(ahcirecover(c) == -1)
			goto lose;
	}
	if (d->sectors == 0) {
		idprint("%s: no sectors\n", d->unit->name);
		goto lose;
	}

	ilock(d);
	d->state = Dready;
	iunlock(d);

	qunlock(c->m);

	idprint("%s: %sLBA %,llud sectors: %s %s %s %s\n", d->unit->name,
		(m->feat & Dllba? "L": ""), d->sectors, d->model, d->firmware,
		d->serial, d->mediachange? "[mediachange]": "");
	return 0;

lose:
	idprint("sdiachi: %s can't be initialized\n", name);
	ilock(d);
	d->state = Dnull;
	iunlock(d);
	qunlock(c->m);
	return -1;
}

enum {
	Nms		= 256,
	Mphywait	=  2*1024/Nms - 1,
	Midwait		= 16*1024/Nms - 1,
	Mcomrwait	= 64*1024/Nms - 1,
};

static void
westerndigitalhung(Drive *d)
{
	if((d->portm.feat & Datapi) == 0 && d->active &&
	    TK2MS(m->ticks - d->intick) > 5000){
		dprint("%s: drive hung; resetting [%lux] ci=%lx\n",
			d->unit->name, d->port->task, d->port->ci);
		d->state = Dreset;
	}
}

static ushort olds[NCtlr*NCtlrdrv];

static int
doportreset(Drive *d)
{
	int i;

	i = -1;
	qlock(&d->portm);
	if(ahciportreset(&d->portc) == -1)
		dprint("ahci: doportreset: fails\n");
	else
		i = 0;
	qunlock(&d->portm);
	dprint("ahci: portreset → %s  [task %lux]\n", diskstates[d->state],
		d->port->task);
	return i;
}

static void
checkdrive(Drive *d, int i)
{
	ushort s;
	char *name;

	ilock(d);
	name = d->unit->name;
	s = d->port->sstatus;
	if(s)
		d->lastseen = m->ticks;
	if(s != olds[i]){
		dprint("%s: status: %04ux -> %04ux: %s\n",
			name, olds[i], s, diskstates[d->state]);
		olds[i] = s;
		d->wait = 0;
	}
	westerndigitalhung(d);
	switch(d->state){
	case Dnull:
	case Dready:
		break;
	case Dmissing:
	case Dnew:
		switch(s & 0x107){
		case 1:		/* no device (pm), device but no phy. comm. */
			ahciwakeup(d->port);
			/* fall through */
		case 0:		/* no device */
			break;
		default:
			dprint("%s: unknown status %04ux\n", name, s);
			/* fall through */
		case 0x100:		/* active, no device */
			if(++d->wait&Mphywait)
				break;
reset:
			if(++d->mode > DMsataii)
				d->mode = 0;
			if(d->mode == DMsatai){	/* we tried everything */
				d->state = Dportreset;
				goto portreset;
			}
			dprint("%s: reset; new mode %s\n", name,
				modename[d->mode]);
			iunlock(d);
			resetdisk(d);
			ilock(d);
			break;
		case 0x103:		/* active, device, phy. comm. */
			if((++d->wait&Midwait) == 0){
				dprint("%s: slow reset %04ux task=%lux; %d\n",
					name, s, d->port->task, d->wait);
				goto reset;
			}
			s = (uchar)d->port->task;
			if(s == 0x7f || ((d->port->sig >> 16) != 0xeb14 &&
			    (s & ~0x17) != (1<<6)))
				break;
			iunlock(d);
			newdrive(d);
			ilock(d);
			break;
		}
		break;
	case Doffline:
		if(d->wait++ & Mcomrwait)
			break;
		/* fallthrough */
	case Derror:
	case Dreset:
		dprint("%s: reset [%s]: mode %d; status %04ux\n",
			name, diskstates[d->state], d->mode, s);
		iunlock(d);
		resetdisk(d);
		ilock(d);
		break;
	case Dportreset:
portreset:
		if(d->wait++ & 0xff && (s & 0x100) == 0)
			break;
		/* device is active */
		dprint("%s: portreset [%s]: mode %d; status %04ux\n",
			name, diskstates[d->state], d->mode, s);
		d->portm.flag |= Ferror;
		clearci(d->port);
		if((s & 7) == 0){	/* no device */
			d->state = Dmissing;
			break;
		}
		iunlock(d);
		doportreset(d);
		ilock(d);
		break;
	}
	iunlock(d);
}

static void
iainterrupt(Ureg*, void *a)
{
	int i;
	ulong cause, m;
	Ctlr *c;
	Drive *d;

	c = a;
	ilock(c);
	cause = c->hba->isr;
	for(i = 0; i < c->mport; i++){
		m = 1 << i;
		if((cause & m) == 0)
			continue;
		d = c->rawdrive + i;
		ilock(d);
		if(d->port->isr && c->hba->pi & m)
			updatedrive(d);
		c->hba->isr = m;
		iunlock(d);
	}
	iunlock(c);
}

static int
iaverify(SDunit *u)
{
	int i, reset;
	Ctlr *c;
	Drive *d;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	ilock(c);
	ilock(d);
	d->unit = u;
	iunlock(d);
	iunlock(c);

	reset = 0;
	/* we may be waiting for a cd drive to spin up, so be a bit patient. */
	for(i = 0; i < 50; i++){
		checkdrive(d, d->driveno);
		switch(d->state){
		case Dnew:
			if(d->port->task & 0x80){
				d->state = Dportreset;
				reset = 1;
			}
			break;
		case Dmissing:
			if(reset || d->port->sstatus & 0x733)
				break;
			/*
			 * don't print messages about missing drives;
			 * it's just noise.
			 */
			break;
		case Dnull:
		case Doffline:
			if(0)print("sdiahci: drive %d in state %s after %d resets\n",
				d->driveno, diskstates[d->state], i);
			return 1;
		case Dready:
			if(d->portm.feat & Datapi)
				return scsiverify(d->unit);
			return 1;
		}
		if(d->state != Dnew && i >= 10)
			break;
		/* Dnew means there's something there. it's just really slow. */
		delay(100);
	}
	if(d->state != Dmissing)
		print("sdiahci: drive %d won't come up; "
			"in state %s after %d ms.\n",
			d->driveno, diskstates[d->state], i*100);
	return 0;
}

static int
iaenable(SDev *s)
{
	Ctlr *c;

	c = s->ctlr;
	ilock(c);
	if(!c->enabled) {
		if(c->ndrive == 0)
			panic("iaenable: zero s->ctlr->ndrive");
		pcisetbme(c->pci);
		setvec(c->pci->intl+VectorPIC, iainterrupt, c);
		/* supposed to squelch leftover interrupts here. */
		ahcienable(c->hba);
		c->enabled = 1;
	}
	iunlock(c);
	return 1;
}

static int
iadisable(SDev *s)
{
	Ctlr *c;

	c = s->ctlr;
	ilock(c);
	ahcidisable(c->hba);
//	intrdisable(c->irq, iainterrupt, c, c->tbdf, name);
	c->enabled = 0;
	iunlock(c);
	return 1;
}

static int
iaonline(SDunit *unit)
{
	int r;
	Ctlr *c;
	Drive *d;

	c = unit->dev->ctlr;
	d = c->drive[unit->subno];
	r = 0;

	if(d->portm.feat & Datapi && d->mediachange){
		r = scsionline(unit);
		if(r > 0)
			d->mediachange = 0;
		return r;
	}

	ilock(d);
	if(d->mediachange){
		r = 2;
		d->mediachange = 0;
		/* devsd resets this after online is called; why? */
		unit->sectors = d->sectors;
		unit->secsize = 512;
	} else if(d->state == Dready)
		r = 1;
	iunlock(d);
	return r;
}

/* returns locked list! */
static Alist*
ahcibuild(Aportm *m, uchar *cmd, void *data, int n, vlong lba)
{
	uchar *c, acmd, dir, llba;
	Alist *l;
	Actab *t;
	Aprdt *p;
	static uchar tab[2][2] = { 0xc8, 0x25, 0xca, 0x35, };

	dir = *cmd != 0x28;
	llba = m->feat & Dllba? 1: 0;
	acmd = tab[dir][llba];
	qlock(m);
	l = m->list;
	t = m->ctab;
	c = t->cfis;

	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = acmd;
	c[3] = 0;

	c[4] = lba;		/* sector	lba low	7:0 */
	c[5] = lba >> 8;	/* cylinder low	lba mid	15:8 */
	c[6] = lba >> 16;	/* cylinder hi	lba hi	23:16 */
	c[7] = 0xa0 | 0x40;	/* obsolete device bits + lba */
	if(llba == 0)
		c[7] |= (lba>>24) & 7;

	c[8] = lba >> 24;	/* sector (exp)		lba 	31:24 */
	c[9] = lba >> 32;	/* cylinder low (exp)	lba	39:32	 */
	c[10] = lba >> 48;	/* cylinder hi (exp)	lba	48:40 */
	c[11] = 0;		/* features (exp); */

	c[12] = n;		/* sector count */
	c[13] = n >> 8;		/* sector count (exp) */
	c[14] = 0;		/* r */
	c[15] = 0;		/* control */

	*(ulong*)(c+16) = 0;

	l->flags = 1<<16 | Lpref | 0x5;	/* Lpref ?? */
	if(dir == Write)
		l->flags |= Lwrite;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	p = &t->prdt;
	p->dba = PCIWADDR(data);
	p->dbahi = 0;
	p->count = 1<<31 | (512*n - 2) | 1;

	return l;
}

static Alist*
ahcibuildpkt(Aportm *m, SDreq *r, void *data, int n)
{
	int fill, len;
	uchar *c;
	Actab *t;
	Alist *l;
	Aprdt *p;

	qlock(m);
	l = m->list;
	t = m->ctab;
	c = t->cfis;

	fill = m->feat & Datapi16? 16: 12;
	if((len = r->clen) > fill)
		len = fill;
	memmove(t->atapi, r->cmd, len);
	memset(t->atapi + len, 0, fill - len);

	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0xa0;
	if(n != 0)
		c[3] = 1;	/* dma */
	else
		c[3] = 0;	/* features (exp); */

	c[4] = 0;		/* sector	lba low	7:0 */
	c[5] = n;		/* cylinder low	lba mid	15:8 */
	c[6] = n >> 8;		/* cylinder hi	lba hi	23:16 */
	c[7] = 0xa0;		/* obsolete device bits */

	*(ulong*)(c+8) = 0;
	*(ulong*)(c+12) = 0;
	*(ulong*)(c+16) = 0;

	l->flags = 1<<16 | Lpref | Latapi | 0x5;
	if(r->write != 0 && data)
		l->flags |= Lwrite;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	if(data == 0)
		return l;

	p = &t->prdt;
	p->dba = PCIWADDR(data);
	p->dbahi = 0;
	p->count = 1<<31 | (n - 2) | 1;

	return l;
}

static int
waitready(Drive *d)
{
	ulong s, i, δ;

	/* don't wait long; we're only the bootstrap */
	for(i = 0; i < 8000; i += 250){
		if(d->state == Dreset || d->state == Dportreset ||
		    d->state == Dnew)
			return 1;
		if(d->lastseen == 0)
			δ = 0;
		else
			δ = TK2MS(m->ticks - d->lastseen);
		if(d->state == Dnull || δ > 10*1000)
			return -1;
		ilock(d);
		s = d->port->sstatus;
		iunlock(d);
		if((s & Imask) == 0 && δ > 1500)
			return -1;
		if(d->state == Dready && (s & Smask) == Sphylink)
			return 0;
		esleep(250);
	}
	print("%s: not responding; offline\n", dnam(d));
	ilock(d);
	d->state = Doffline;
	iunlock(d);
	return -1;
}

static int
iariopkt(SDreq *r, Drive *d)
{
	int n, count, try, max, flag, task;
	char *name;
	uchar *cmd, *data;
	Aport *p;
	Asleep as;

	cmd = r->cmd;
	name = d->unit->name;
	p = d->port;

	aprint("%02ux %02ux %c %d %p\n", cmd[0], cmd[2], "rw"[r->write],
		r->dlen, r->data);
//	if(cmd[0] == 0x5a && (cmd[2] & 0x3f) == 0x3f)
//		return sdmodesense(r, cmd, d->info, sizeof d->info);
	r->rlen = 0;
	count = r->dlen;
	max = 65536;

	try = 0;
retry:
	if(waitready(d) == -1)
		return SDeio;
	data = r->data;
	n = count;
	if(n > max)
		n = max;
	d->active++;
	ahcibuildpkt(&d->portm, r, data, n);
	ilock(d);
	d->portm.flag = 0;
	iunlock(d);
	p->ci = 1;

	as.p = p;
	as.i = 1;
	d->intick = m->ticks;

	while(ahciclear(&as) == 0)
		;

	if (d->port == nil)
		panic("iariopkt: nil d->port");
	ilock(d);
	flag = d->portm.flag;
	task = d->port->task;
	iunlock(d);

	if(task & (Efatal<<8) || task & (ASbsy|ASdrq) && d->state == Dready){
		d->port->ci = 0;
		ahcirecover(&d->portc);
		task = d->port->task;
	}
	d->active--;
	qunlock(&d->portm);
	if(flag == 0){
		if(++try == 10){
			print("%s: bad disk\n", name);
			r->status = SDcheck;
			return SDcheck;
		}
		print("%s: retry\n", name);
		esleep(1000);
		goto retry;
	}
	if(flag & Ferror){
		if((task & Eidnf) == 0)
			print("%s: i/o error %ux\n", name, task);
		r->status = SDcheck;
		return SDcheck;
	}

	data += n;

	r->rlen = data - (uchar*)r->data;
	r->status = SDok;
	return SDok;
}

static int
iario(SDreq *r)
{
	int n, count, max, flag, task;
	vlong lba;
	char *name;
	uchar *cmd, *data;
	Aport *p;
	Asleep as;
	Ctlr *c;
	Drive *d;
	SDunit *unit;

	unit = r->unit;
	c = unit->dev->ctlr;
	d = c->drive[unit->subno];
	if(d->portm.feat & Datapi)
		return iariopkt(r, d);
	cmd = r->cmd;
	name = d->unit->name;
	p = d->port;

//	if((i = sdfakescsi(r, d->info, sizeof d->info)) != SDnostatus){
//		r->status = i;
//		return i;
//	}

	if(*cmd != 0x28 && *cmd != 0x2a){
		print("%s: bad cmd 0x%.2ux\n", name, cmd[0]);
		r->status = SDcheck;
		return SDcheck;
	}

	lba   = cmd[2]<<24 | cmd[3]<<16 | cmd[4]<<8 | cmd[5];
	count = cmd[7]<<8  | cmd[8];
	if(r->data == nil)
		return SDok;
	if (unit->secsize <= 0)
		unit->secsize = 512;
	if(r->dlen < count * unit->secsize)
		count = r->dlen / unit->secsize;
	max = 128;

	if(waitready(d) == -1)
		return SDeio;
	data = r->data;
	while(count > 0){
		n = count;
		if(n > max)
			n = max;
		d->active++;
		ahcibuild(&d->portm, cmd, data, n, lba);
		ilock(d);
		d->portm.flag = 0;
		iunlock(d);
		p->ci = 1;

		as.p = p;
		as.i = 1;
		d->intick = m->ticks;

		while(ahciclear(&as) == 0)
			;

		if (d->port == nil)
			panic("iario: nil d->port");
		ilock(d);
		flag = d->portm.flag;
		task = d->port->task;
		iunlock(d);

		if(task & (Efatal<<8) ||
		    task & (ASbsy|ASdrq) && d->state == Dready){
			d->port->ci = 0;
			ahcirecover(&d->portc);
			task = d->port->task;
		}
		d->active--;
		qunlock(&d->portm);
		if(flag == 0 || flag & Ferror){
			print("%s: i/o error %ux @%lld\n", name, task, lba);
			r->status = SDeio;
			return SDeio;
		}

		count -= n;
		lba += n;
		data += n * unit->secsize;
	}
	r->rlen = data - (uchar*)r->data;
	r->status = SDok;
	return SDok;
}

/*
 * configure drives 0-15 as ahci sata  (c.f. errata)
 */
static int
iaahcimode(Pcidev *p)
{
	uint u;

	u = pcicfgr16(p, 0x92);
	dprint("ahci: %ux: iaahcimode %.2ux %.4ux\n",
		p->tbdf, pcicfgr8(p, 0x91), u);
	pcicfgw16(p, 0x92, u | 0xf);
	return 0;
}

enum {
	Ghc	= 0x04/4,	/* global host control */
	Pi	= 0x0c/4,	/* ports implemented */

	Cmddec	= 1<<15,	/* enable command block decode */

	/* Ghc bits */
	Ahcien	= 1<<31,	/* ahci enable */
};

static void
iasetupahci(Ctlr *c)
{
	pcicfgw16(c->pci, 0x40, pcicfgr16(c->pci, 0x40) & ~Cmddec);
	pcicfgw16(c->pci, 0x42, pcicfgr16(c->pci, 0x42) & ~Cmddec);

	c->lmmio[Ghc] |= Ahcien;
	c->lmmio[Pi] = (1 << 6) - 1;	/* 5 ports (supposedly ro pi reg) */

	/* enable ahci mode; from ich9 datasheet */
	pcicfgw16(c->pci, 0x90, 1<<6 | 1<<5);
}

static int
didtype(Pcidev *p)
{
	switch(p->vid){
	case 0x8086:
		if((p->did & 0xfffc) == 0x2680)
			return Tesb;
		/*
		 * 0x27c4 is the intel 82801 in compatibility (not sata) mode.
		 */
		if ((p->did & 0xfffb) == 0x27c1 ||	/* 82801g[bh]m ich7 */
		    p->did == 0x2821 ||			/* 82801h[roh] */
		    (p->did & 0xfffe) == 0x2824 ||	/* 82801h[b] */
		    (p->did & 0xfeff) == 0x2829 ||	/* ich8/9m */
		    (p->did & 0xfffe) == 0x2922 ||	/* ich9 */
		    p->did == 0x3a02 ||			/* 82801jd/do */
		    (p->did & 0xfefe) == 0x3a22 ||	/* ich10, pch */
		    (p->did & 0xfff7) == 0x3b28)	/* pchm */
			return Tich;
		break;
	case 0x1002:					/* ATI */
		if (p->did == 0x4380 || p->did == 0x4390 || p->did == 0x4391)
			return Tsb600;
		break;
	}
	if(p->ccrb == Pcibcstore && p->ccru == 6 && p->ccrp == 1)
		return Tunk;
	return -1;
}

static SDev*
iapnp(void)
{
	int i, n, nunit, type;
	ulong io;
	Ctlr *c;
	Drive *d;
	Pcidev *p;
	SDev *head, *tail, *s;
	static int done;

	if (done || getconf("*noahciload") != nil)
		return nil;

	/* native access to disks seems to interfere with bios loading */
	if (biosload)
		return nil;

	done = 1;
	memset(olds, 0xff, sizeof olds);
	p = nil;
	head = tail = nil;
loop:
	while((p = pcimatch(p, 0, 0)) != nil){
		if((type = didtype(p)) == -1)
			continue;
		if(niactlr == NCtlr){
			print("ahci: iapnp: %s: too many controllers\n",
				tname[type]);
			break;
		}
		c = iactlr + niactlr;
		s = sdevs + niactlr;
		memset(c, 0, sizeof *c);
		memset(s, 0, sizeof *s);
		c->pci = p;
		c->type = type;
		io = p->mem[Abar].bar & ~0xf;
		io = upamalloc(io, p->mem[Abar].size, 0);
		if(io == 0){
			print("%s: address %#lux in use, did %#ux\n",
				tname[c->type], io, p->did);
			continue;
		}

		/* ugly hack: get this in compatibility mode; see memory.c:271 */
		if(p->mem[Abar].bar == 0) {
			if(0)print("%s: did %#ux has zero bar\n", tname[c->type],
				p->did);
			continue;
		}
		if(io == 0x40000000) {
			print("%s: did %#ux is in non-sata mode.  bar %#lux\n",
				tname[c->type], p->did, p->mem[Abar].bar);
			continue;
		}

		c->mmio = KADDR(io);
		c->lmmio = (ulong*)c->mmio;
		if(Intel(c) && p->did != 0x2681)
			iasetupahci(c);
		nunit = ahciconf(c);
//		ahcihbareset((Ahba*)c->mmio);
		if(Intel(c) && iaahcimode(p) == -1)
			break;
		if(nunit < 1){
//			vunmap(c->mmio, p->mem[Abar].size);
			continue;
		}
		niactlr++;
		i = (c->hba->cap>>21) & 1;
		print("%s: sata-%s with %d ports\n", tname[c->type],
			"I\0II"+i*2, nunit);
		s->ifc = &sdiahciifc;
		s->ctlr = c;
		s->nunit = nunit;
		s->idno = 'E';
		c->sdev = s;
		c->ndrive = nunit;
		c->mport = c->hba->cap & 0x1f;

		/* map the drives -- they don't all need to be enabled. */
		memset(c->rawdrive, 0, sizeof c->rawdrive);
		n = 0;
		for(i = 0; i < NCtlrdrv; i++) {
			d = c->rawdrive+i;
			d->portno = i;
			d->driveno = -1;
			d->sectors = 0;
			d->serial[0] = ' ';
			d->ctlr = c;
			if((c->hba->pi & (1<<i)) == 0)
				continue;
			d->port = (Aport*)(c->mmio + 0x80*i + 0x100);
			d->portc.p = d->port;
			d->portc.m = &d->portm;
			d->driveno = n++;
			c->drive[d->driveno] = d;
		}
		for(i = 0; i < n; i++)
			if(ahciidle(c->drive[i]->port) == -1){
				print("%s: port %d wedged; abort\n",
					tname[c->type], i);
				goto loop;
			}
		for(i = 0; i < n; i++){
			c->drive[i]->mode = DMsatai;
			configdrive(c->drive[i]);
		}

		if(head)
			tail->next = s;
		else
			head = s;
		tail = s;
	}
	return head;
}

static SDev*
iaid(SDev* sdev)
{
	int i;
	Ctlr *c;

	for(; sdev; sdev = sdev->next){
		if(sdev->ifc != &sdiahciifc)
			continue;
		c = sdev->ctlr;
		for(i = 0; i < NCtlr; i++)
			if(c == iactlr + i)
				sdev->idno = 'E' + i;
	}
	return nil;
}

SDifc sdiahciifc = {
	"iahci",

	iapnp,
	nil,			/* legacy */
	iaid,
	iaenable,
	iadisable,

	iaverify,
	iaonline,
	iario,
	nil,
	nil,

	scsibio,
};
