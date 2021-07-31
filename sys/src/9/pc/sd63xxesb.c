/*
 * intel 63[12]?esb ahci sata controller
 * copyright © 2007 coraid, inc.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/sd.h"
#include "ahci.h"

#define	dprint(...)	if(debug == 1)	iprint(__VA_ARGS__); else USED(debug)
#define	idprint(...)	if(prid == 1)	print(__VA_ARGS__); else USED(prid)
#define	aprint(...)	if(datapi == 1)	iprint(__VA_ARGS__); else USED(datapi)

enum{
	NCtlr	= 4,
	NCtlrdrv= 32,
	NDrive	= NCtlr*NCtlrdrv,

	Read	= 0,
	Write,
};

/* pci space configuration */
enum{
	Pmap	= 0x90,
	Ppcs	= 0x91,
	Prev	= 0xa8,
};

enum{
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

extern SDifc sd63xxesbifc;
typedef struct Ctlr Ctlr;

enum{
	DMautoneg,
	DMsatai,
	DMsataii,
};

static char *modename[] = {
	"auto",
	"satai",
	"sataii",
};

static char *flagname[] = {
	"llba",
	"smart",
	"power",
	"nop",
	"atapi",
	"atapi16",
};

typedef struct{
	Lock;

	Ctlr	*ctlr;
	SDunit	*unit;
	char	name[10];
	Aport	*port;
	Aportm	portm;
	Aportc	portc;	/* redundant ptr to port and portm. */

	uchar	mediachange;
	uchar	state;
	uchar	smartrs;

	uvlong	sectors;
	ulong	intick;
	int	wait;
	uchar	mode;	/* DMautoneg, satai or sataii. */
	uchar	active;

	char	serial[20+1];
	char	firmware[8+1];
	char	model[40+1];

	ushort	info[0x200];

	int	driveno;	/* ctlr*NCtlrdrv + unit */
	/* controller port # != driveno when not all ports are enabled */
	int	portno;
}Drive;

struct Ctlr{
	Lock;

	int	irq;
	int	tbdf;
	int	enabled;
	SDev	*sdev;
	Pcidev	*pci;

	uchar	*mmio;
	ulong	*lmmio;
	Ahba	*hba;

	Drive	rawdrive[NCtlrdrv];
	Drive*	drive[NCtlrdrv];
	int	ndrive;
};

static	Ctlr	iactlr[NCtlr];
static	SDev	sdevs[NCtlr];
static	int	niactlr;

static	Drive	*iadrive[NDrive];
static	int	niadrive;

static	int	debug;
static	int	prid = 1;
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
		*e++ = ntab[reg[i]>>4];
		*e++ = ntab[reg[i]&0xf];
		*e++ = ' ';
	}
	*e++ = '\n';
	*e = 0;
	dprint(buf);
}

static void
dreg(char *s, Aport *p)
{
	dprint("%stask=%ux; cmd=%ux; ci=%ux; is=%ux\n", s, p->task, p->cmd,
		p->ci, p->isr);
}

static void
esleep(int ms)
{
	if(waserror())
		return;
	tsleep(&up->sleep, return0, 0, ms);
	poperror();
}

typedef struct{
	Aport	*p;
	int	i;
}Asleep;

static int
ahciclear(void *v)
{
	Asleep *s;

	s = v;
	return (s->p->ci&s->i) == 0;
}

static void
aesleep(Aportm *m, Asleep *a, int ms)
{
	if(waserror())
		return;
	tsleep(m, ahciclear, a, ms);
	poperror();
}

static int
ahciwait(Aportc *c, int ms)
{
	Asleep as;
	Aport *p;

	p = c->p;
	p->ci = 1;
	as.p = p;
	as.i = 1;
	aesleep(c->m, &as, ms);
	if((p->task&1) == 0 && p->ci == 0)
		return 0;
	dreg("ahciwait timeout ", c->p);
	return -1;
}

static int
nop(Aportc *pc)
{
	uchar *c;
	Actab *t;
	Alist *l;

	if((pc->m->feat&Dnop) == 0)
		return -1;

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0x00;
	c[7] = 0xa0;		/* obsolete device bits */

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	return ahciwait(pc, 3*1000);
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
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	return ahciwait(pc, 3*1000);
}

static int
setudmamode(Aportc *pc, uchar f)
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
	c[3] = 3;		/* set transfer mode */
	c[7] = 0xa0;		/* obsolete device bits */
	c[12] = 0x40 | f;	/* sector count */

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	return ahciwait(pc, 3*1000);
}

static void
asleep(int ms)
{
	if(up == nil)
		delay(ms);
	else
		esleep(ms);
}

static int
ahciportreset(Aportc *c)
{
	u32int *cmd, i;
	Aport *p;

	p = c->p;
	cmd = &p->cmd;
	*cmd &= ~(Afre|Ast);
	for(i = 0; i < 500; i += 25){
		if((*cmd&Acr) == 0)
			break;
		asleep(25);
	}
	p->sctl = 1|(p->sctl&~7);
	delay(1);
	p->sctl &= ~7;
	return 0;
}

static int
smart(Aportc *pc, int n)
{
	uchar *c;
	Actab *t;
	Alist *l;

	if((pc->m->feat&Dsmart) == 0)
		return -1;

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0xb0;
	c[3] = 0xd8 + n;	/* able smart */
	c[5] = 0x4f;
	c[6] = 0xc2;
	c[7] = 0xa0;

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	if(ahciwait(pc, 1000) == -1 || pc->p->task & (1|32)){
		dprint("smart fail %ux\n", pc->p->task);
//		preg(pc->m->fis.r, 20);
		return -1;
	}
	if(n)
		return 0;
	return 1;
}

static int
smartrs(Aportc *pc)
{
	uchar *c;
	Actab *t;
	Alist *l;

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0xb0;
	c[3] = 0xda;		/* return smart status */
	c[5] = 0x4f;
	c[6] = 0xc2;
	c[7] = 0xa0;

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	c = pc->m->fis.r;
	if(ahciwait(pc, 1000) == -1 || pc->p->task & (1|32)){
		dprint("smart fail %ux\n", pc->p->task);
		preg(c, 20);
		return -1;
	}
	if(c[5] == 0x4f && c[6] == 0xc2)
		return 1;
	return 0;
}

static int
flushcache(Aportc *pc)
{
	uchar *c, llba;
	Actab *t;
	Alist *l;
	static uchar tab[2] = {0xe7, 0xea};

	llba = pc->m->feat&Dllba? 1: 0;
	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = tab[llba];
	c[7] = 0xa0;

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	if(ahciwait(pc, 60000) == -1 || pc->p->task & (1|32)){
		dprint("flushcache fail %ux\n", pc->p->task);
//		preg( pc->m->fis.r, 20);
		return -1;
	}
	return 0;
}

static ushort
gbit16(void *a)
{
	ushort j;
	uchar *i;

	i = a;
	j  = i[1] << 8;
	j |= i[0];
	return j;
}

static u32int
gbit32(void *a)
{
	u32int j;
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
	return (uvlong)gbit32(i+4) << 32 | gbit32(a);
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

	return ahciwait(pc, 3*1000);
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

	if(m->feat&Datapi){
		i = gbit16(id+0);
		if(i&1)
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
	u32int *p, i;

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
	dprint("clo clear %x\n", a->task);
	if(a->task & ASbsy)
		return -1;
	*p |= Ast;
	return 0;
}

static int
ahcicomreset(Aportc *pc)
{
	uchar *c;
	Actab *t;
	Alist *l;

	dprint("ahcicomreset\n");
	dreg("comreset ", pc->p);
	if(ahciquiet(pc->p) == -1){
		dprint("ahciquiet fails\n");
		return -1;
	}
	dreg("comreset ", pc->p);

	t = pc->m->ctab;
	c = t->cfis;

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x00;
	c[7] = 0xa0;		/* obsolete device bits */
	c[15] = 1<<2;		/* srst */

	l = pc->m->list;
	l->flags = Lclear | Lreset | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	if(ahciwait(pc, 500) == -1){
		dprint("first command in comreset fails\n");
		return -1;
	}
	microdelay(250);
	dreg("comreset ", pc->p);

	memset(c, 0, 0x20);
	c[0] = 0x27;
	c[1] = 0x00;
	c[7] = 0xa0;		/* obsolete device bits */

	l = pc->m->list;
	l->flags = Lwrite | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = 0;

	if(ahciwait(pc, 150) == -1){
		dprint("second command in comreset fails\n");
		return -1;
	}
	dreg("comreset ", pc->p);
	return 0;
}

static int
ahciidle(Aport *port)
{
	u32int *p, i, r;

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
 * § 6.2.2.1  first part; comreset handled by reset disk.
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
	f->devicebits = (u32int*)(f->base + 0x58);
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
		dprint("configdrive:  spinning up ... [%ux]\n", p->sstatus);
		p->cmd |= Apod|Asud;
		asleep(1400);
	}

	p->serror = SerrAll;

	p->list = PCIWADDR(m->list);
	p->listhi = 0;
	p->fis = PCIWADDR(m->fis.base);
	p->fishi = 0;
	p->cmd |= Afre|Ast;

	/* disable power managment sequence from book. */
	p->sctl = (3*Aipm) | (mode*Aspd) | 0*Adet;
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
	Ahba *h;
	u32int u;

	h = c->hba = (Ahba*)c->mmio;
	u = h->cap;

	if((u&Hsam) == 0)
		h->ghc |= Hae;

	print("ahci hba sss %d; ncs %d; coal %d; mports %d; led %d; clo %d; ems %d;\n",
		(u>>27) & 1, (u>>8) & 0x1f, (u>>7) & 1, u & 0x1f, (u>>25) & 1,
		(u>>24) & 1, (u>>6) & 1);
	return countbits(h->pi);
}

static int
ahcihbareset(Ahba *h)
{
	int wait;

	h->ghc |= 1;
	for(wait = 0; wait < 1000; wait += 100){
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
	p = op;
	while(*p == ' ')
		p++;
	memmove(op, p, n - (e - p));
}

static int
identify(Drive *d)
{
	u16int *id;
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

	if((osectors == 0 || osectors != s) &&
	    memcmp(oserial, d->serial, sizeof oserial) != 0){
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
		p->cmd |=  Ast;
	}
}

static void
updatedrive(Drive *d)
{
	u32int cause, serr, s0, pr, ewake;
	char *name;
	Aport *p;
	static u32int last;

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
		wakeup(&d->portm);
		pr = 0;
	}else if(cause & Adps)
		pr = 0;
	if(cause & Ifatal){
		ewake = 1;
		dprint("Fatal\n");
	}
	if(cause & Adhrs){
		if(p->task & 33){
			dprint("Adhrs cause = %ux; serr = %ux; task=%ux\n",
				cause, serr, p->task);
			d->portm.flag |= Ferror;
			ewake = 1;
		}
		pr = 0;
	}
	if(0 && p->task & 1 && last != cause)
		dprint("err ca %ux serr %ux task %ux sstat %ux\n",
			cause, serr, p->task, p->sstatus);

	if(pr)
		dprint("%s: upd %ux ta %ux\n", name, cause, p->task);
	if(cause & (Aprcs|Aifs)){
		s0 = d->state;
		switch(p->sstatus & 7){
		case 0:
			d->state = Dmissing;
			break;
		case 1:
			d->state = Derror;
			break;
		case 3:
			/* power mgnt crap for suprise removal */
			p->ie |= Aprcs|Apcs;	/* is this required? */
			d->state = Dreset;
			break;
		case 4:
			d->state = Doffline;
			break;
		}
		dprint("%s: %s → %s [Apcrs] %ux\n", name, diskstates[s0],
			diskstates[d->state], p->sstatus);
		/* print pulled message here. */
		if(s0 == Dready && d->state != Dready)
			idprint("%s: pulled\n", name);
		if(d->state != Dready)
			d->portm.flag |= Ferror;
		ewake = 1;
	}
	p->serror = serr;
	if(ewake){
		clearci(p);
		wakeup(&d->portm);
	}
	last = cause;
}

static void
pstatus(Drive *d, ulong s)
{
	/*
	 * bogus code because the first interrupt is currently dropped.
	 * likely my fault.  serror is maybe cleared at the wrong time.
	 */
	switch(s){
	case 0:
		d->state = Dmissing;
		break;
	case 2:			/* should this be missing?  need testcase. */
		dprint("pstatus 2\n");
		/* fallthrough */
	case 3:
		d->wait = 0;
		d->state = Dnew;
		break;
	case 4:
		d->state = Doffline;
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
	dprint("resetdisk: icc %ux  det %d sdet %d\n", state, det, stat);
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
	wakeup(&d->portm);
	iunlock(d);

	qlock(&d->portm);

	if(p->cmd&Ast && ahciswreset(&d->portc) == -1){
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
	char *name, *s;
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

	ilock(d);
	d->state = Dready;
	iunlock(d);

	qunlock(c->m);

	s = "";
	if(m->feat & Dllba)
		s = "L";
	idprint("%s: %sLBA %,lld sectors\n", d->unit->name, s, d->sectors);
	idprint("  %s %s %s %s\n", d->model, d->firmware, d->serial,
		d->mediachange?"[mediachange]":"");
	return 0;

lose:
//	qunlock(&d->portm);		/* shurely shome mishtake */
	qunlock(c->m);
	return -1;
}

enum{
	Nms		= 256,
	Mphywait	=  2*1024/Nms - 1,
	Midwait		= 16*1024/Nms - 1,
	Mcomrwait	= 64*1024/Nms - 1,
};

static void
westerndigitalhung(Drive *d)
{
	if((d->portm.feat&Datapi) == 0 && d->active &&
	    TK2MS(MACHP(0)->ticks-d->intick) > 5000){
		dprint("%s: drive hung; resetting [%ux] ci=%x\n",
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
		dprint("ahciportreset fails\n");
	else
		i = 0;
	qunlock(&d->portm);
	dprint("portreset → %s  [task %ux]\n",
		diskstates[d->state], d->port->task);
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
		case 0:
		case 1:
			break;
		default:
			dprint("%s: unknown status %04ux\n", name, s);
		case 0x100:
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
		case 0x103:
			if((++d->wait&Midwait) == 0){
				dprint("%s: slow reset %04ux task=%ux; %d\n",
					name, s, d->port->task, d->wait);
				goto reset;
			}
			s = d->port->task&0xff;
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
		dprint("%s: portreset [%s]: mode %d; status %04ux\n",
			name, diskstates[d->state], d->mode, s);
		d->portm.flag |= Ferror;
		clearci(d->port);
		wakeup(&d->portm);
		if((s & 7) == 0){
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
satakproc(void*)
{
	int i;

	memset(olds, 0xff, sizeof olds);
	for(;;){
		tsleep(&up->sleep, return0, 0, Nms);
		for(i = 0; i < niadrive; i++)
			checkdrive(iadrive[i], i);
	}
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
	for(i = 0; i < c->ndrive; i++){
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
	Ctlr *c;
	Drive *d;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	ilock(c);
	ilock(d);
	d->unit = u;
	iunlock(d);
	iunlock(c);
	return 1;
}

static int
iaenable(SDev *s)
{
	char name[32];
	static int once;
	Ctlr *c;

	c = s->ctlr;
	ilock(c);
	if(!c->enabled) {
		if(once++ == 0)
			kproc("iasata", satakproc, 0);
		pcisetbme(c->pci);
		snprint(name, sizeof name, "%s (%s)", s->name, s->ifc->name);
		intrenable(c->irq, iainterrupt, c, c->tbdf, name);
		/* supposed to squelch leftover interrupts here. */
		ahcienable(c->hba);
	}
	c->enabled = 1;
	iunlock(c);
	return 1;
}

static int
iadisable(SDev *s)
{
	char name[32];
	Ctlr *c;

	c = s->ctlr;
	ilock(c);
	ahcidisable(c->hba);
	snprint(name, sizeof name, "%s (%s)", s->name, s->ifc->name);
	intrdisable(c->irq, iainterrupt, c, c->tbdf, name);
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
	llba = m->feat&Dllba? 1: 0;
	acmd = tab[dir][llba];
	qlock(m);
	l = m->list;
	t = m->ctab;
	c = t->cfis;

	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = acmd;
	c[3] = 0;

	c[4] = lba;		/* sector		lba low	7:0 */
	c[5] = lba >> 8;	/* cylinder low		lba mid	15:8 */
	c[6] = lba >> 16;	/* cylinder hi		lba hi	23:16 */
	c[7] = 0xa0 | 0x40;	/* obsolete device bits + lba */
	if(llba == 0)
		c[7] |= (lba>>24) & 7;

	c[8] = lba >> 24;	/* sector (exp)		lba 	31:24 */
	c[9] = lba >> 32;	/* cylinder low (exp)	lba	39:32 */
	c[10] = lba >> 48;	/* cylinder hi (exp)	lba	48:40 */
	c[11] = 0;		/* features (exp); */

	c[12] = n;		/* sector count */
	c[13] = n >> 8;		/* sector count (exp) */
	c[14] = 0;		/* r */
	c[15] = 0;		/* control */

	*(ulong*)(c + 16) = 0;

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
	Alist *l;
	Actab *t;
	Aprdt *p;

	qlock(m);
	l = m->list;
	t = m->ctab;
	c = t->cfis;

	fill = m->feat&Datapi16? 16: 12;
	if((len = r->clen) > fill)
		len = fill;
	memmove(t->atapi, r->cmd, len);
	memset(t->atapi+len, 0, fill-len);

	c[0] = 0x27;
	c[1] = 0x80;
	c[2] = 0xa0;
	if(n != 0)
		c[3] = 1;	/* dma */
	else
		c[3] = 0;	/* features (exp); */

	c[4] = 0;		/* sector		lba low	7:0 */
	c[5] = n;		/* cylinder low		lba mid	15:8 */
	c[6] = n >> 8;		/* cylinder hi		lba hi	23:16 */
	c[7] = 0xa0;		/* obsolete device bits */

	*(ulong*)(c + 8) = 0;
	*(ulong*)(c + 12) = 0;
	*(ulong*)(c + 16) = 0;

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
	u32int s, t, i;

	for(i = 0; i < 120; i++){
		ilock(d);
		s = d->port->sstatus;
		t = d->port->task;
		iunlock(d);
		if((s & 0x100) == 0)
			return -1;
		if(d->state == Dready && (s & 7) == 3)
			return 0;
		if((i+1) % 30 == 0)
			print("%s: waitready: [%s] task=%ux sstat=%ux\n",
				d->unit->name, diskstates[d->state], t, s);
		esleep(1000);
	}
	print("%s: not responding; offline\n", d->unit->name);
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
	if(cmd[0] == 0x5a && (cmd[2] & 0x3f) == 0x3f)
		return sdmodesense(r, cmd, d->info, sizeof d->info);
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
	d->intick = MACHP(0)->ticks;

	while(waserror())
		;
	sleep(&d->portm, ahciclear, &as);
	poperror();

	ilock(d);
	flag = d->portm.flag;
	task = d->port->task;
	iunlock(d);

	if(task & (Efatal<<8) || task & (ASbsy|ASdrq) && d->state == Dready){
		d->port->ci = 0;		/* @? */
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
		iprint("%s: retry\n", name);
		esleep(1000);
		goto retry;
	}
	if(flag & Ferror){
		iprint("%s: i/o error %ux\n", name, task);
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
	int i, n, count, try, max, flag, task;
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

	if(r->cmd[0] == 0x35 || r->cmd[0] == 0x91){
		qlock(&d->portm);
		i = flushcache(&d->portc);
		qunlock(&d->portm);
		if(i == 0)
			return sdsetsense(r, SDok, 0, 0, 0);
		return sdsetsense(r, SDcheck, 3, 0xc, 2);
	}

	if((i = sdfakescsi(r, d->info, sizeof d->info)) != SDnostatus){
		r->status = i;
		return i;
	}

	if(*cmd != 0x28 && *cmd != 0x2a){
		print("%s: bad cmd 0x%.2ux\n", name, cmd[0]);
		r->status = SDcheck;
		return SDcheck;
	}

	lba   = cmd[2]<<24 | cmd[3]<<16 | cmd[4]<<8 | cmd[5];
	count = cmd[7]<<8 | cmd[8];
	if(r->data == nil)
		return SDok;
	if(r->dlen < count * unit->secsize)
		count = r->dlen / unit->secsize;
	max = 128;

	try = 0;
retry:
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
		d->intick = MACHP(0)->ticks;

		while(waserror())
			;
		sleep(&d->portm, ahciclear, &as);
		poperror();

		ilock(d);
		flag = d->portm.flag;
		task = d->port->task;
		iunlock(d);

		if(task & (Efatal<<8) ||
		    task & (ASbsy|ASdrq) && d->state == Dready){
			d->port->ci = 0;	/* @? */
			ahcirecover(&d->portc);
			task = d->port->task;
		}
		d->active--;
		qunlock(&d->portm);
		if(flag == 0){
			if(++try == 10){
				print("%s: bad disk\n", name);
				r->status = SDeio;
				return SDeio;
			}
			iprint("%s: retry %lld\n", name, lba);
			esleep(1000);
			goto retry;
		}
		if(flag & Ferror){
			iprint("%s: i/o error %ux @%,lld\n", name, task, lba);
			r->status = SDeio;
			return SDeio;
		}

		count -= n;
		lba   += n;
		data += n * unit->secsize;
	}
	r->rlen = data - (uchar*)r->data;
	r->status = SDok;
	return SDok;
}

/*
 * configure drives 0-5 as ahci sata  (c.f. errata)
 */
static int
iaahcimode(Pcidev *p)
{
	dprint("iaahcimode %ux %ux %ux\n", pcicfgr8(p, 0x91), pcicfgr8(p, 92),
		pcicfgr8(p, 93));
	pcicfgw16(p, 0x92, pcicfgr32(p, 0x92) | 0xf);	/* ports 0-3 */
//	pcicfgw8(p, 0x93, pcicfgr32(p, 9x93) | 3);	/* ports 4-5 */
	return 0;
}

static void
iasetupahci(Ctlr *c)
{
	/* disable cmd block decoding. */
	pcicfgw16(c->pci, 0x40, pcicfgr16(c->pci, 0x40) & ~(1<<15));
	pcicfgw16(c->pci, 0x42, pcicfgr16(c->pci, 0x42) & ~(1<<15));

	c->lmmio[0x4/4] |= 1<<31;	/* enable ahci mode (ghc register) */
	c->lmmio[0xc/4] = (1<<6) - 1;	/* 5 ports. (supposedly ro pi reg.) */

	/* enable ahci mode. */
//	pcicfgw8(c->pci, 0x90, 0x40);
//	pcicfgw16(c->pci, 0x90, 1<<6 | 1<<5); /* pedantically proper for ich9 */
	pcicfgw8(c->pci, 0x90, 1<<6 | 1<<5);  /* pedantically proper for ich9 */
}

static SDev*
iapnp(void)
{
	int i, n, nunit;
	ulong io;
	Ctlr *c;
	Drive *d;
	Pcidev *p;
	SDev *head, *tail, *s;
	static int done;

	if(done++)
		return nil;

	p = nil;
	head = tail = nil;
loop:
	while((p = pcimatch(p, 0x8086, 0)) != nil){
		if((p->did & 0xfffc) != 0x2680 && (p->did & 0xfffe) != 0x27c4)
			continue;		/* !esb && !82801g[bh]m */
		if(niactlr == NCtlr){
			print("iapnp: too many controllers\n");
			break;
		}
		c = iactlr + niactlr;
		s = sdevs  + niactlr;
		memset(c, 0, sizeof *c);
		memset(s, 0, sizeof *s);
		io = p->mem[Abar].bar & ~0xf;
		c->mmio = vmap(io, p->mem[0].size);
		if(c->mmio == 0){
			print("iapnp: address 0x%luX in use did=%x\n",
				io, p->did);
			continue;
		}
		c->lmmio = (ulong*)c->mmio;
		c->pci = p;
		if(p->did != 0x2681)
			iasetupahci(c);
		nunit = ahciconf(c);
		// ahcihbareset((Ahba*)c->mmio);
		if(iaahcimode(p) == -1)
			break;
		if(nunit < 1){
			vunmap(c->mmio, p->mem[0].size);
			continue;
		}

		i = (c->hba->cap>>21) & 1;
		print("intel 63[12]xesb: sata-%s ports with %d ports\n",
			"I\0II" + i*2, nunit);
		s->ifc = &sd63xxesbifc;
		s->ctlr = c;
		s->nunit = nunit;
		s->idno = 'E';
		c->sdev = s;
		c->irq = p->intl;
		c->tbdf = p->tbdf;
		c->ndrive = nunit;

		/* map the drives -- they don't all need to be enabled. */
		memset(c->rawdrive, 0, sizeof c->rawdrive);
		n = 0;
		for(i = 0; i < NCtlrdrv; i++) {
			d = c->rawdrive + i;
			d->portno = i;
			d->driveno = -1;
			d->sectors = 0;
			d->ctlr = c;
			if((c->hba->pi & (1<<i)) == 0)
				continue;
			d->port = (Aport*)(c->mmio + 0x80*i + 0x100);
			d->portc.p = d->port;
			d->portc.m = &d->portm;
			d->driveno = n++;
			c->drive[i] = d;
			iadrive[d->driveno] = d;
		}
		for(i = 0; i < n; i++)
			if(ahciidle(c->drive[i]->port) == -1){
				dprint("intel 63[12]xesb: port %d wedged; abort\n", i);
				goto loop;
			}
		for(i = 0; i < n; i++){
			c->drive[i]->mode = DMsatai;
			configdrive(c->drive[i]);
		}

		niadrive += nunit;
		if(head)
			tail->next = s;
		else
			head = s;
		tail = s;
	}
	return head;
}

static char* smarttab[] = {
	"unset",
	"error",
	"threshold exceeded",
	"normal"
};

static char *
pflag(char *s, char *e, uchar f)
{
	uchar i, m;

	for(i = 0; i < 8; i++){
		m = 1 << i;
		if(f&m)
			s = seprint(s, e, "%s ", flagname[i]);
	}
	return seprint(s, e, "\n");
}

static int
iarctl(SDunit *u, char *p, int l)
{
	char buf[32];
	char *e, *op;
	Aport *o;
	Ctlr *c;
	Drive *d;

	if((c = u->dev->ctlr) == nil)
		return 0;
	d = c->drive[u->subno];
	o = d->port;

	e = p+l;
	op = p;
	if(d->state == Dready){
		p = seprint(p, e, "model\t%s\n", d->model);
		p = seprint(p, e, "serial\t%s\n", d->serial);
		p = seprint(p, e, "firm\t%s\n", d->firmware);
		if(d->smartrs == 0xff)
			p = seprint(p, e, "smart\tenable error\n");
		else if(d->smartrs == 0)
			p = seprint(p, e, "smart\tdisabled\n");
		else
			p = seprint(p, e, "smart\t%s\n",
				smarttab[d->portm.smart]);
		p = seprint(p, e, "flag\t");
		p = pflag(p, e, d->portm.feat);
	}else
		p = seprint(p, e, "no disk present [%s]\n", diskstates[d->state]);
	serrstr(o->serror, buf, buf + sizeof buf - 1);
	p = seprint(p, e, "reg\ttask %ux cmd %ux serr %ux %s ci %ux is %ux; "
		"sig %ux sstatus %04x\n", o->task, o->cmd, o->serror, buf,
		o->ci, o->isr, o->sig, o->sstatus);
	p = seprint(p, e, "geometry %llud 512\n", d->sectors);
	return p - op;
}

static void
runflushcache(Drive *d)
{
	long t0;

	t0 = MACHP(0)->ticks;
	qlock(&d->portm);
	flushcache(&d->portc);
	qunlock(&d->portm);
	dprint("flush in %ldms\n", TK2MS(MACHP(0)->ticks-t0));
}

static void
forcemode(Drive *d, char *mode)
{
	int i;

	for(i = 0; i < nelem(modename); i++)
		if(strcmp(mode, modename[i]) == 0)
			break;
	if(i == nelem(modename))
		i = 0;
	ilock(d);
	d->mode = i;
	iunlock(d);
}

static void
runsmartable(Drive *d, int i)
{
	if(waserror()){
		qunlock(&d->portm);
		d->smartrs = 0;
		nexterror();
	}
	qlock(&d->portm);
	d->smartrs = smart(&d->portc, i);
	d->portm.smart = 0;
	qunlock(&d->portm);
	poperror();
}

static void
forcestate(Drive *d, char *state)
{
	int i;

	for(i = 0; i < nelem(diskstates); i++)
		if(strcmp(state, diskstates[i]) == 0)
			break;
	if(i == nelem(diskstates))
		i = 0;
	ilock(d);
	d->state = i;
	if(i == Dnull){
		d->mediachange = 1;
		d->unit->sectors = 0;		/* force disk to disappear. */
	}
	iunlock(d);
}


static int
iawctl(SDunit *u, Cmdbuf *cmd)
{
	char **f;
	Ctlr *c;
	Drive *d;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	f = cmd->f;

	if(strcmp(f[0], "flushcache") == 0)
		runflushcache(d);
	else if(strcmp(f[0], "identify") ==  0){
		uint i;

		i = strtoul(f[1]? f[1]: "0", 0, 0);
		if(i > 0xff)
			i = 0;
		dprint("%04d %ux\n", i, d->info[i]);
	}else if(strcmp(f[0], "mode") == 0)
		forcemode(d, f[1]? f[1]: "satai");
	else if(strcmp(f[0], "nop") == 0){
		if((d->portm.feat & Dnop) == 0){
			cmderror(cmd, "nop command not supported");
			return -1;
		}
		if(waserror()){
			qunlock(&d->portm);
			nexterror();
		}
		qlock(&d->portm);
		nop(&d->portc);
		qunlock(&d->portm);
		poperror();
	}else if(strcmp(f[0], "reset") == 0)
		forcestate(d, "reset");
	else if(strcmp(f[0], "smart") == 0){
		if(d->smartrs == 0){
			cmderror(cmd, "smart not enabled");
			return -1;
		}
		if(waserror()){
			qunlock(&d->portm);
			d->smartrs = 0;
			nexterror();
		}
		qlock(&d->portm);
		d->portm.smart = 2 + smartrs(&d->portc);
		qunlock(&d->portm);
		poperror();
	}else if(strcmp(f[0], "smartdisable") == 0)
		runsmartable(d, 1);
	else if(strcmp(f[0], "smartenable") == 0)
		runsmartable(d, 0);
	else if(strcmp(f[0], "state") == 0)
		forcestate(d, f[1]? f[1]: "null");
	else{
		cmderror(cmd, Ebadctl);
		return -1;
	}
	return 0;
}

static char *
portr(char *p, char *e, uint x)
{
	int i, a;

	p[0] = 0;
	a = -1;
	for(i = 0; i < 32; i++){
		if((x & (1<<i)) == 0){
			if(a != -1 && i - 1 != a)
				p = seprint(p, e, "-%d", i - 1);
			a = -1;
			continue;
		}
		if(a == -1){
			if(i > 0)
				p = seprint(p, e, ", ");
			p = seprint(p, e, "%d", a = i);
		}
	}
	if(a != -1 && i - 1 != a)
		p = seprint(p, e, "-%d", i - 1);
	return p;
}

#define has(x, s) if(u & (x)) p = seprint(p, e, (s))

static char*
iartopctl(SDev *s, char *p, char *e)
{
	u32int u;
	char name[10], pr[25];
	Ahba *h;
	Ctlr *c;

	c = s->ctlr;

	snprint(name, sizeof name, "sd%c", s->idno);
	p = seprint(p, e, "%s ahci ", name);
	u = c->hba->cap;
	has(Hs64a, "64a ");
	has(Hsncq, "ncq ");
	has(Hssntf, "ntf ");
	has(Hsmps, "mps ");
	has(Hsss, "ss ");
	has(Hsalp, "alp ");
	has(Hsal, "led ");
	has(Hsclo, "clo ");
	has(Hsam, "am ");
	has(Hspm, "pm ");
	has(Hssc, "slum ");
	has(Hpsc, "pslum ");
	has(Hcccs, "coal ");
	has(Hems, "ems ");
	has(Hsxs, "sxs ");
	p = seprint(p, e, "\n");

	p = seprint(p, e, "%s iss %d ncs %d np %d\n", name, (u>>20) & 0xf,
		(u>>8) & 0x1f, 1 + (u & 0x1f));
	h = c->hba;
	portr(pr, pr + sizeof pr, h->pi);
	p = seprint(p, e, "%s ghc %ux isr %ux pi %ux %s ver %ux\n",
		name, h->ghc, h->isr, h->pi, pr, h->ver);
	return p;
}

static int
iawtopctl(SDev *, Cmdbuf *cmd)
{
	int *v;
	char **f;

	f = cmd->f;
	v = 0;

	if(strcmp(f[0], "debug") == 0)
		v = &debug;
	else if(strcmp(f[0], "idprint") == 0)
		v = &prid;
	else if(strcmp(f[0], "aprint") == 0)
		v = &datapi;
	else
		cmderror(cmd, Ebadctl);

	switch(cmd->nf){
	default:
		cmderror(cmd, Ebadarg);
	case 1:
		*v ^= 1;
		break;
	case 2:
		if(strcmp(f[1], "on") == 0)
			*v = 1;
		else
			*v = 0;
		break;
	}
	return 0;
}

SDifc sd63xxesbifc = {
	"iahci",

	iapnp,
	nil,		/* legacy */
	iaenable,
	iadisable,

	iaverify,
	iaonline,
	iario,
	iarctl,
	iawctl,

	scsibio,
	nil,		/* probe */
	nil,		/* clear */
	iartopctl,
	iawtopctl,
};
