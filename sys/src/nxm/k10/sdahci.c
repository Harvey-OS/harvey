/*
 * intel/amd ahci sata controller
 * copyright © 2007-12 coraid, inc.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/sd.h"
#include <fis.h>
#include "../port/sdfis.h"
#include "ahci.h"
#include "../port/led.h"

#pragma	varargck	type	"T"	int
#define	dprint(...)	if(debug)	iprint(__VA_ARGS__); else USED(debug)
#define	idprint(...)	if(prid)		print(__VA_ARGS__); else USED(prid)
#define	aprint(...)	if(datapi)	print(__VA_ARGS__); else USED(datapi)
#define	ledprint(...)	if(dled)		print(__VA_ARGS__); else USED(dled)
#define	Ticks		sys->ticks
#define Pciwaddrh(va)	((u32int)(PCIWADDR(va)>>32))

enum {
	NCtlr		= 4,
	NCtlrdrv		= 32,
	NDrive		= NCtlr*NCtlrdrv,

	Fahdrs		= 4,

	Read		= 0,
	Write,

	Eesb		= 1<<0,	/* must have (Eesb & Emtype) == 0 */

	/* pci space configuration */
	Pmap		= 0x90,
	Ppcs		= 0x91,

	Nms		= 256,
	Mphywait	=  2*1024/Nms - 1,
	Midwait		= 16*1024/Nms - 1,
	Mcomrwait	= 64*1024/Nms - 1,
};

enum {
	Tesb,
	Tsb600,
	Tjmicron,
	Tahci,
	Tlast,
};

typedef	struct	Ctlrtype	Ctlrtype;
typedef	struct	Ctlr	Ctlr;
typedef	struct	Drive	Drive;

struct Ctlrtype {
	uint	type;
	uint	maxdmaxfr;
	uint	flags;
	char	*name;
};

Ctlrtype cttab[Tlast] = {
[Tesb]		Tesb,		8192,	0,	"63xxesb",
[Tsb600]		Tsb600,		256,	0,	"sb600",
[Tjmicron]	Tjmicron,	8192,	0,	"jmicron",
[Tahci]		Tahci,		8192,	0,	"ahci",
};

enum {
	Dnull		= 0,
	Dmissing		= 1<<0,
	Dnew		= 1<<1,
	Dready		= 1<<2,
	Derror		= 1<<3,
	Dreset		= 1<<4,
	Doffline		= 1<<5,
	Dportreset	= 1<<6,
	Dlast		= 8,
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

extern SDifc sdahciifc;

enum {
	DMautoneg,
	DMsatai,
	DMsataii,
	DMsataiii,
	DMlast,
};

static char *modes[DMlast] = {
	"auto",
	"satai",
	"sataii",
	"sataiii",
};

typedef struct Htab Htab;
struct Htab {
	uint	bit;
	char	*name;
};

struct Drive {
	Lock;

	Ctlr	*ctlr;
	SDunit	*unit;
	char	name[10];
	Aport	*port;
	Aportm	portm;
	Aportc	portc;	/* redundant ptr to port and portm. */
	Ledport;

	ulong	totick;
	ulong	lastseen;
	uint	wait;
	uchar	mode;
	uchar	state;

	/*
	 * ahci allows non-sequential ports.
	 * to avoid this hassle, we let
	 * driveno	ctlr*NCtlrdrv + unit
	 * portno	nth available port
	 */
	uint	driveno;
	uint	portno;
};

struct Ctlr {
	Lock;

	Ctlrtype	*type;
	int	enabled;
	SDev	*sdev;
	Pcidev	*pci;

	uchar	*mmio;
	u32int	*lmmio;
	Ahba	*hba;
	Aenc;
	uint	enctype;

	Drive	rawdrive[NCtlrdrv];
	Drive*	drive[NCtlrdrv];
	int	ndrive;
	uint	pi;
};

static	Ctlr	iactlr[NCtlr];
static	SDev	sdevs[NCtlr];
static	int	niactlr;
static	ushort	olds[NCtlr*NCtlrdrv];

static	Drive	*iadrive[NDrive];
static	int	niadrive;

static	int	debug;
static	int	prid = 1;
static	int	datapi;
static	int	dled;

static char stab[] = {
[0]	'i', 'm',
[8]	't', 'c', 'p', 'e',
[16]	'N', 'I', 'W', 'B', 'D', 'C', 'H', 'S', 'T', 'F', 'X'
};

static void
serrstr(u32int r, char *s, char *e)
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

static void
dreg(char *s, Aport *p)
{
	dprint("%stask=%ux; cmd=%ux; ci=%ux; is=%ux\n",
		s, p->task, p->cmd, p->ci, p->isr);
}

static void
esleep(int ms)
{
	if(waserror())
		return;
	tsleep(&up->sleep, return0, 0, ms);
	poperror();
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
	Aport *p;
	Asleep as;

	p = c->p;
	p->ci = 1;
	as.p = p;
	as.i = 1;
	aesleep(c->m, &as, ms);
	if((p->task & 1) == 0 && p->ci == 0)
		return 0;
	dreg("ahciwait fail/timeout ", c->p);
	return -1;
}

static void
mkalist(Aportm *m, uint flags, uchar *data, int len)
{
	Actab *t;
	Alist *l;
	Aprdt *p;

	t = m->ctab;
	l = m->list;
	l->flags = flags | 0x5;
	l->len = 0;
	l->ctab = PCIWADDR(t);
	l->ctabhi = Pciwaddrh(t);
	if(data){
		l->flags |= 1<<16;
		p = &t->prdt;
		p->dba = PCIWADDR(data);
		p->dbahi = Pciwaddrh(data);
		p->count = 1<<31 | len - 2 | 1;
	}
}

static int
settxmode(Aportc *pc, uchar f)
{
	uchar *c;

	c = pc->m->ctab->cfis;
	if(txmodefis(pc->m, c, f) == -1)
		return 0;
	mkalist(pc->m, Lwrite, 0, 0);
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
ahciportreset(Aportc *c, uint mode)
{
	int i;
	u32int *cmd;
	Aport *p;

	p = c->p;
	cmd = &p->cmd;
	*cmd &= ~(Afre|Ast);
	for(i = 0; i < 500; i += 25){
		if((*cmd & Acr) == 0)
			break;
		asleep(25);
	}
	p->sctl = 3*Aipm | 0*Aspd | Adet;
	delay(1);
	p->sctl = 3*Aipm | mode*Aspd;
	return 0;
}

static int
ahciquiet(Aport *a)
{
	int i;
	u32int *p;

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
	dprint("ahci: clo clear %ux\n", a->task);
	if(a->task & ASbsy)
		return -1;
	*p |= Afre | Ast;
	return 0;
}

static int
ahcicomreset(Aportc *pc)
{
	uchar *c;

	dreg("comreset ", pc->p);
	if(ahciquiet(pc->p) == -1){
		dprint("ahci: ahciquiet failed\n");
		return -1;
	}
	dreg("comreset ", pc->p);

	c = pc->m->ctab->cfis;
	nopfis(pc->m, c, 1);
	mkalist(pc->m, Lclear | Lreset, 0, 0);
	if(ahciwait(pc, 500) == -1){
		dprint("ahci: comreset1 failed\n");
		return -1;
	}
	microdelay(250);
	dreg("comreset ", pc->p);

	nopfis(pc->m, c, 0);
	mkalist(pc->m, Lwrite, 0, 0);
	if(ahciwait(pc, 150) == -1){
		dprint("ahci: comreset2 failed\n");
		return -1;
	}
	dreg("comreset ", pc->p);
	return 0;
}

static int
ahciidle(Aport *port)
{
	int i, r;
	u32int *p;

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
static int
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

static int
ahcirecover(Aportc *pc)
{
	ahciswreset(pc);
	pc->p->cmd |= Ast;
	if(settxmode(pc, pc->m->udma) == -1)
		return -1;
	return 0;
}

static void
setupfis(Afis *f)
{
	f->base = mallocalign(0x100, 0x100, 0, 0);
	f->d = f->base + 0;
	f->p = f->base + 0x20;
	f->r = f->base + 0x40;
	f->u = f->base + 0x60;
	f->devicebits = (u32int*)(f->base + 0x58);
}

static void
ahciwakeup(Aportc *c, uint mode)
{
	ushort s;

	s = c->p->sstatus;
	if((s & Isleepy) == 0)
		return;
	if((s & Smask) != Spresent){
		print("ahci: slumbering drive missing %.3ux\n", s);
		return;
	}
	ahciportreset(c, mode);
//	iprint("ahci: wake %.3ux -> %.3lux\n", s, c->p->sstatus);
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
		m->list = mallocalign(sizeof *m->list, 1024, 0, 0);
		m->ctab = mallocalign(sizeof *m->ctab, 128, 0, 0);
	}

	p->list = PCIWADDR(m->list);
	p->listhi = Pciwaddrh(m->list);
	p->fis = PCIWADDR(m->fis.base);
	p->fishi = Pciwaddrh(m->fis.base);

	p->cmd |= Afre;

	if((p->sstatus & Sbist) == 0 && (p->cmd & Apwr) != Apwr)
	if((p->sstatus & Sphylink) == 0 && h->cap & Hss){
		dprint("ahci:  spin up ... [%.3ux]\n", p->sstatus);
		p->cmd |= Apwr;
		for(int i = 0; i < 1400; i += 50){
			if(p->sstatus & (Sphylink | Sbist))
				break;
			asleep(50);
		}
	}

	p->serror = SerrAll;

	if((p->sstatus & SSmask) == (Isleepy | Spresent))
		ahciwakeup(c, mode);
	/* disable power managment sequence from book. */
	p->sctl = 3*Aipm | mode*Aspd | 0*Adet;
	p->cmd &= ~Aalpe;

	p->cmd |= Ast;
	p->ie = IEM;

	return 0;
}

static void
setstate(Drive *d, int state)
{
	ilock(d);
	d->state = state;
	iunlock(d);
}

static void
ahcienable(Ahba *h)
{
	h->ghc |= Hie;
}

static void
ahcidisable(Ahba *h)
{
	h->ghc &= ~Hie;
}

static int
countbits(u32int u)
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
	u32int u;
	Ahba *h;

	h = c->hba = (Ahba*)c->mmio;
	u = h->cap;
	if((u & Ham) == 0)
		h->ghc |= Hae;
	return countbits(h->pi);
}

static int
ahcihbareset(Ahba *h)
{
	int wait;

	h->ghc |= Hhr;
	for(wait = 0; wait < 1000; wait += 100){
		if(h->ghc == 0)
			return 0;
		delay(100);
	}
	return -1;
}

static char*
dstate(uint s)
{
	int i;

	for(i = 0; s; i++)
		s >>= 1;
	return diskstates[i];
}

static char*
tnam(Ctlr *c)
{
	return c->type->name;
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

static void
clearci(Aport *p)
{
	if(p->cmd & Ast){
		p->cmd &= ~Ast;
		p->cmd |=  Ast;
	}
}

static int
intel(Ctlr *c)
{
	return c->pci->vid == 0x8086;
}

static int
ignoreahdrs(Drive *d)
{
	return d->portm.feat & Datapi && d->ctlr->type->type == Tsb600;
}

static void
updatedrive(Drive *d)
{
	u32int f, cause, serr, s0, pr, ewake;
	Aport *p;
	static u32int last;

	pr = 1;
	ewake = 0;
	f = 0;
	p = d->port;
	cause = p->isr;
	if(d->ctlr->type->type == Tjmicron)
		cause &= ~Aifs;
	serr = p->serror;
	p->isr = cause;

	if(p->ci == 0){
		f |= Fdone;
		pr = 0;
	}else if(cause & Adps)
		pr = 0;
	if(cause & Ifatal){
		ewake = 1;
		dprint("%s: fatal\n", dnam(d));
	}
	if(cause & Adhrs){
		if(p->task & 33){
			if(ignoreahdrs(d) && serr & ErrE)
				f |= Fahdrs;
			dprint("%s: Adhrs cause %ux serr %ux task %ux\n",
				dnam(d), cause, serr, p->task);
			f |= Ferror;
			ewake = 1;
		}
		pr = 0;
	}
	if(p->task & 1 && last != cause)
		dprint("%s: err ca %ux serr %ux task %ux sstat %.3ux\n",
			dnam(d), cause, serr, p->task, p->sstatus);
	if(pr)
		dprint("%s: upd %ux ta %ux\n", dnam(d), cause, p->task);

	if(cause & (Aprcs|Aifs)){
		s0 = d->state;
		switch(p->sstatus & Smask){
		case Smissing:
			d->state = Dmissing;
			break;
		case Spresent:
			if((p->sstatus & Imask) == Islumber)
				d->state = Dnew;
			else
				d->state = Derror;
			break;
		case Sphylink:
			/* power mgnt crap for suprise removal */
			p->ie |= Aprcs|Apcs;	/* is this required? */
			d->state = Dreset;
			break;
		case Sbist:
			d->state = Doffline;
			break;
		}
		dprint("%s: %s → %s [Apcrs] %.3ux\n", dnam(d), dstate(s0),
			dstate(d->state), p->sstatus);
		if(s0 == Dready && d->state != Dready)
			idprint("%s: pulled\n", dnam(d));
		if(d->state != Dready)
			f |= Ferror;
		if(d->state != Dready || p->ci)
			ewake = 1;
	}
	p->serror = serr;
	if(ewake)
		clearci(p);
	if(f){
		d->portm.flag = f;
		wakeup(&d->portm);
	}
	last = cause;
}

static void
pstatus(Drive *d, u32int s)
{
	/*
	 * bogus code because the first interrupt is currently dropped.
	 * likely my fault.  serror is maybe cleared at the wrong time.
	 */
	if(s)
		d->lastseen = Ticks;
	switch(s){
	default:
		print("%s: pstatus: bad status %.3ux\n", dnam(d), s);
	case Smissing:
		d->state = Dmissing;
		break;
	case Spresent:
		break;
	case Sphylink:
		d->wait = 0;
		d->state = Dnew;
		break;
	case Sbist:
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
	pstatus(d, d->port->sstatus & Smask);
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
	stat = p->sstatus & Smask;
	state = (p->cmd>>28) & 0xf;
	dprint("%s: resetdisk: icc %ux  det %.3ux sdet %.3ux\n", dnam(d), state, det, stat);

	ilock(d);
	state = d->state;
	if(d->state != Dready || d->state != Dnew)
		d->portm.flag |= Ferror;
	clearci(p);			/* satisfy sleep condition. */
	wakeup(&d->portm);
	d->state = Derror;
	iunlock(d);

	if(stat != Sphylink){
		setstate(d, Dportreset);
		return;
	}

	qlock(&d->portm);
	if(p->cmd&Ast && ahciswreset(&d->portc) == -1)
		setstate(d, Dportreset);	/* get a bigger stick. */
	else{
		setstate(d, Dmissing);
		configdrive(d);
	}
	dprint("%s: resetdisk: %s → %s\n", dnam(d), dstate(state), dstate(d->state));
	qunlock(&d->portm);
}

static int
newdrive(Drive *d)
{
	Aportc *c;
	Aportm *m;

	c = &d->portc;
	m = &d->portm;

	qlock(c->m);
	setfissig(m, c->p->sig);
	qunlock(c->m);

	if(ataonline(d->unit, m) != 0)
		goto lose;
	m->atamaxxfr = 128;
	if(d->portm.feat & Dllba)
		m->atamaxxfr = d->ctlr->type->maxdmaxfr;

	setstate(d, Dready);
	pronline(d->unit, m);
	return 0;

lose:
	qlock(c->m);
	idprint("%s: can't be initialized\n", dnam(d));
	setstate(d, Dnull);
	qunlock(c->m);
	return -1;
}

static int
doportreset(Drive *d)
{
	int i;

	i = -1;
	qlock(&d->portm);
	if(ahciportreset(&d->portc, d->mode) == -1)
		dprint("ahci: ahciportreset fails\n");
	else
		i = 0;
	qunlock(&d->portm);
	dprint("ahci: portreset → %s  [task %.4ux ss %.3ux]\n",
		dstate(d->state), d->port->task, d->port->sstatus);
	return i;
}

static void
statechange(Drive *d)
{
	Aportm *m;

	m = &d->portm;
	switch(d->state){
	case Dnull:
	case Doffline:
		if(d->unit)
		if(d->unit->sectors != 0){
			m->sectors = 0;
			m->drivechange = 1;
		}
	case Dready:
		d->wait = 0;
	}
}

static uint
maxmode(Ctlr *c)
{
	return (c->hba->cap & 0xf*Hiss)/Hiss;
}

static void
checkdrive(Drive *d, int i)
{
	ushort s, sig;

	ilock(d);
	s = d->port->sstatus;
	if(s)
		d->lastseen = Ticks;
	if(s != olds[i]){
		dprint("%s: status: %.3ux -> %.3ux: %s\n",
			dnam(d), olds[i], s, dstate(d->state));
		olds[i] = s;
		d->wait = 0;
	}
	switch(d->state){
	case Dnull:
	case Dready:
		break;
	case Dmissing:
	case Dnew:
		switch(s & (Iactive|Smask)){
		case Spresent:
			ahciwakeup(&d->portc, d->mode);
		case Smissing:
			break;
		default:
			dprint("%s: unknown status %.3ux\n", dnam(d), s);
			/* fall through */
		case Iactive:		/* active, no device */
			if(++d->wait&Mphywait)
				break;
reset:
			if(d->mode == 0)
				d->mode = maxmode(d->ctlr);
			else
				d->mode--;
			if(d->mode == DMautoneg){
				d->state = Dportreset;
				goto portreset;
			}
			dprint("%s: reset; new mode %s\n", dnam(d),
				modes[d->mode]);
			iunlock(d);
			resetdisk(d);
			ilock(d);
			break;
		case Iactive | Sphylink:
			if(d->unit == nil)
				break;
			if((++d->wait&Midwait) == 0){
				dprint("%s: slow reset %.3ux task=%ux; %d\n",
					dnam(d), s, d->port->task, d->wait);
				goto reset;
			}
			s = (uchar)d->port->task;
			sig = d->port->sig >> 16;
			if(s == 0x7f || s&ASbsy ||
			    (sig != 0xeb14 && (s & ASdrdy) == 0))
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
		dprint("%s: reset [%s]: mode %d; status %.3ux\n",
			dnam(d), dstate(d->state), d->mode, s);
		iunlock(d);
		resetdisk(d);
		ilock(d);
		break;
	case Dportreset:
portreset:
		if(d->wait++ & 0xff && (s & Iactive) == 0)
			break;
		dprint("%s: portreset [%s]: mode %d; status %.3ux\n",
			dnam(d), dstate(d->state), d->mode, s);
		d->portm.flag |= Ferror;
		clearci(d->port);
		wakeup(&d->portm);
		if((s & Smask) == 0){
			d->state = Dmissing;
			break;
		}
		iunlock(d);
		doportreset(d);
		ilock(d);
		break;
	}
	statechange(d);
	iunlock(d);
}

static void
satakproc(void*)
{
	int i;

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
	u32int cause, m;
	Ctlr *c;
	Drive *d;

	c = a;
	ilock(c);
	cause = c->hba->isr;
	for(i = 0; cause; i++){
		m = 1 << i;
		if((cause & m) == 0)
			continue;
		cause &= ~m;
		d = c->rawdrive + i;
		ilock(d);
		if(d->port->isr && c->pi & m)
			updatedrive(d);
		c->hba->isr = m;
		iunlock(d);
	}
	iunlock(c);
}

static int
ahciencreset(Ctlr *c)
{
	Ahba *h;

	if(c->enctype == Eesb)
		return 0;
	h = c->hba;
	h->emctl |= Emrst;
	while(h->emctl & Emrst)
		delay(1);
	return 0;
}

/*
 * from the standard: (http://en.wikipedia.org/wiki/IBPI)
 * rebuild is preferred as locate+fail; alternate 1hz fail
 * we're going to assume no locate led.
 */
enum {
	Ledsleep	= 125,		/* 8hz */

	N0	= Ledon*Aled,
	L0	= Ledon*Aled | Ledon*Locled,
	L1	= Ledon*Aled | Ledoff*Locled,
	R0	= Ledon*Aled | Ledon*Locled |	Ledon*Errled,
	R1	= Ledon*Aled | 			Ledoff*Errled,
	S0	= Ledon*Aled |  Ledon*Locled /*|	Ledon*Errled*/,	/* botch */
	S1	= Ledon*Aled | 			Ledoff*Errled,
	P0	= Ledon*Aled | 			Ledon*Errled,
	P1	= Ledon*Aled | 			Ledoff*Errled,
	F0	= Ledon*Aled | 			Ledon*Errled,
	C0	= Ledon*Aled | Ledon*Locled,
	C1	= Ledon*Aled | Ledoff*Locled,

};

//static ushort led3[Ibpilast*8] = {
//[Ibpinone*8]	0,	0,	0,	0,	0,	0,	0,	0,
//[Ibpinormal*8]	N0,	N0,	N0,	N0,	N0,	N0,	N0,	N0,
//[Ibpirebuild*8]	R0,	R0,	R0,	R0,	R1,	R1,	R1,	R1,
//[Ibpilocate*8]	L0,	L1,	L0,	L1,	L0,	L1,	L0,	L1,
//[Ibpispare*8]	S0,	S1,	S0,	S1,	S1,	S1,	S1,	S1,
//[Ibpipfa*8]	P0,	P1,	P0,	P1,	P1,	P1,	P1,	P1,	/* first 1 sec */
//[Ibpifail*8]	F0,	F0,	F0,	F0,	F0,	F0,	F0,	F0,
//[Ibpicritarray*8]	C0,	C0,	C0,	C0,	C1,	C1,	C1,	C1,
//[Ibpifailarray*8]	C0,	C1,	C0,	C1,	C0,	C1,	C0,	C1,
//};

static ushort led2[Ibpilast*8] = {
[Ibpinone*8]	0,	0,	0,	0,	0,	0,	0,	0,
[Ibpinormal*8]	N0,	N0,	N0,	N0,	N0,	N0,	N0,	N0,
[Ibpirebuild*8]	R0,	R0,	R0,	R0,	R1,	R1,	R1,	R1,
[Ibpilocate*8]	L0,	L0,	L0,	L0,	L0,	L0,	L0,	L0,
[Ibpispare*8]	S0,	S0,	S0,	S0,	S1,	S1,	S1,	S1,
[Ibpipfa*8]	P0,	P1,	P0,	P1,	P1,	P1,	P1,	P1,	/* first 1 sec */
[Ibpifail*8]	F0,	F0,	F0,	F0,	F0,	F0,	F0,	F0,
[Ibpicritarray*8]	C0,	C0,	C0,	C0,	C1,	C1,	C1,	C1,
[Ibpifailarray*8]	C0,	C1,	C0,	C1,	C0,	C1,	C0,	C1,
};

static int
ledstate(Ledport *p, uint seq)
{
	ushort i;

	if(p->led == Ibpipfa && seq%32 >= 8)
		i = P1;
	else
		i = led2[8*p->led + seq%8];
	if(i != p->ledbits){
		p->ledbits = i;
		ledprint("ledstate %,.011ub %ud\n", p->ledbits, seq);
		return 1;
	}
	return 0;
}

static int
blink(Drive *d, uint t)
{
	Ahba *h;
	Ctlr *c;
	Aledmsg msg;

	if(ledstate(d, t) == 0)
		return 0;
	c = d->ctlr;
	h = c->hba;
	/* ensure last message has been transmitted */
	while(h->emctl & Tmsg)
		microdelay(1);
	switch(c->enctype){
	default:
		panic("%s: bad led type %d", dnam(d), c->enctype);
	case Elmt:
		memset(&msg, 0, sizeof msg);
		msg.type = Mled;
		msg.dsize = 0;
		msg.msize = sizeof msg - 4;
		msg.led[0] = d->ledbits;
		msg.led[1] = d->ledbits>>8;
		msg.pm = 0;
		msg.hba = d->driveno;
		memmove(c->enctx, &msg, sizeof msg);
		break;
	}
	h->emctl |= Tmsg;
	return 1;
}

enum {
	Esbdrv0	= 4,		/* start pos in bits */
	Esbiota	= 3,		/* shift in bits */
	Esbact	= 1,
	Esbloc	= 2,
	Esberr	= 4,
};

uint
esbbits(uint s)
{
	uint i, e;			/* except after c */

	e = 0;
	for(i = 0; i < 3; i++)
		e |= ((s>>3*i & 7) != 0)<<i;
	return e;
}

static int
blinkesb(Ctlr *c, uint t)
{
	uint i, s, u[32/4];
	uvlong v;
	Drive *d;

	s = 0;
	for(i = 0; i < c->ndrive; i++){
		d = c->drive[i];
		s |= ledstate(d, t);		/* no port mapping */
	}
	if(s == 0)
		return 0;
	memset(u, 0, sizeof u);
	for(i = 0; i < c->ndrive; i++){
		d = c->drive[i];
		s = Esbdrv0 + Esbiota*i;
		v = esbbits(d->ledbits) * (1ull << s%32);
		u[s/32 + 0] |= v;
		u[s/32 + 1] |= v>>32;
	}
	for(i = 0; i < c->encsz; i++)
		c->enctx[i] = u[i];
	return 1;
}

static long
ahciledr(SDunit *u, Chan *ch, void *a, long n, vlong off)
{
	Ctlr *c;
	Drive *d;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	return ledr(d, ch, a, n, off);
}

static long
ahciledw(SDunit *u, Chan *ch, void *a, long n, vlong off)
{
	Ctlr *c;
	Drive *d;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	return ledw(d, ch, a, n, off);
}

static void
ledkproc(void*)
{
	uchar map[NCtlr];
	uint i, j, t0, t1;
	Ctlr *c;
	Drive *d;

	j = 0;
	memset(map, 0, sizeof map);
	for(i = 0; i < niactlr; i++)
		if(iactlr[i].enctype != 0){
			ahciencreset(iactlr + i);
			map[i] = 1;
			j++;
		}
	if(j == 0)
		pexit("no work", 1);
	for(i = 0; i < niadrive; i++){
		iadrive[i]->nled = 3;		/* hardcoded */
		if(iadrive[i]->ctlr->enctype == Eesb)
			iadrive[i]->nled = 3;
		iadrive[i]->ledbits = -1;
	}
	for(i = 0; ; i++){
		t0 = Ticks;
		for(j = 0; j < niadrive; ){
			c = iadrive[j]->ctlr;
			if(map[j] == 0)
				j += c->enctype;
			else if(c->enctype == Eesb){
				blinkesb(c, i);
				j += c->ndrive;
			}else{
				d = iadrive[j++];
				blink(d, i);
			}
		}
		t1 = Ticks;
		esleep(Ledsleep - TK2MS(t1 - t0));
	}
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
	if(d->unit == nil){
		d->unit = u;
		if(c->enctype != 0)
			sdaddfile(u, "led", 0644, eve, ahciledr, ahciledw);
	}
	iunlock(d);
	iunlock(c);
	checkdrive(d, d->driveno);		/* c->d0 + d->driveno */
	return 1;
}

static int
iaenable(SDev *s)
{
	char name[32];
	Ctlr *c;
	static int once;

	c = s->ctlr;
	ilock(c);
	if(!c->enabled){
		if(once == 0)
			kproc("iasata", satakproc, 0);
		if(c->ndrive == 0)
			panic("iaenable: zero s->ctlr->ndrive");
		pcisetbme(c->pci);
		snprint(name, sizeof name, "%s (%s)", s->name, s->ifc->name);
		intrenable(c->pci->intl, iainterrupt, c, c->pci->tbdf, name);
		/* supposed to squelch leftover interrupts here. */
		ahcienable(c->hba);
		c->enabled = 1;
		if(++once == niactlr)
			kproc("ialed", ledkproc, 0);
	}
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
	print("missing the intrdisable because intrdisable is wierd\n");
//	intrdisable(c->pci->intl, iainterrupt, c, c->pci->tbdf, name);
	c->enabled = 0;
	iunlock(c);
	return 1;
}

static int
iaonline(SDunit *u)
{
	int r;
	Ctlr *c;
	Drive *d;
	Aportm *m;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	m = &d->portm;
	r = 0;

	if(m->feat & Datapi && m->drivechange){
		r = scsionlinex(u, m) == SDok;
		if(r > 0)
			m->drivechange = 0;
		return r;
	}

	ilock(d);
	if(m->drivechange){
		r = 2;
		m->drivechange = 0;
		/* devsd resets this after online is called; why? */
		u->sectors = m->sectors;
		u->secsize = m->secsize;
	}else if(d->state == Dready)
		r = 1;
	iunlock(d);
	return r;
}

static Alist*
ahcibuildpkt(Aportm *m, SDreq *r, void *data, int n)
{
	uint flags;
	uchar *c;
	Actab *t;
	Alist *l;

	l = m->list;
	t = m->ctab;
	c = t->cfis;
	atapirwfis(m, c, r->cmd, r->clen, n);
	flags = 1<<16 | Lpref | Latapi;
	if(r->write != 0 && data)
		flags |= Lwrite;
	mkalist(m, flags, data, n);
	return l;
}

static Alist*
ahcibuildfis(Aportm *m, SDreq *r, void *data, uint n)
{
	uchar *c;
	uint flags, dir;
	Alist *l;

	l = m->list;
	c = m->ctab->cfis;
	if((r->ataproto & Pprotom) != Ppkt){
		memmove(c, r->cmd, r->clen);
		flags = Lpref;
		if(r->ataproto&Pout && n > 0)
			flags |= Lwrite;
		dir = r->ataproto&Pdatam;
		if(dir == Pnd && n == 0)
			flags |= Lwrite;
		mkalist(m, flags, data, n);
	}else{
		atapirwfis(m, c, r->cmd, r->clen, n);
		flags = 1<<16 | Lpref | Latapi;
		if(r->write && data)
			flags |= Lwrite;
		mkalist(m, flags, data, n);
	}
	return l;
}

static int
isready(Drive *d)
{
	u32int s;
	ulong δ;

	if(d->state & (Dreset | Dportreset /*| Dnew*/))
		return 1;
	δ = TK2MS(Ticks - d->lastseen);
	if(d->state == Dnull || δ > 10*1000){
		dprint("%s: last seen too long ago: %ld\n", dnam(d), δ);
		return -1;
	}
	ilock(d);
	s = d->port->sstatus;
	iunlock(d);
	if((s & Imask) == 0 && δ > 1500){
		dprint("%s: phy off %ldms\n", dnam(d), δ);
		return -1;
	}
	if(d->state & (Dready | Dnew) && (s & Smask) == Sphylink)
		return 0;
	return 1;
}

static int
waitready(Drive *d, int tk)
{
	int r;

	for(;;){
		r = isready(d);
		if(r <= 0)
			return r;
		if(tk - Ticks - 10 < 1ul<<31)
			return -1;
		esleep(10);
	}
}

static int
io(Drive *d, uint proto, int totk, int interrupt)
{
	uint task, flag, rv;
	Aport *p;
	Asleep as;

	switch(waitready(d, totk)){
	case -1:
		return SDeio;
	case 1:
		return SDretry;
	}

	ilock(d);
	d->portm.flag = 0;
	iunlock(d);
	p = d->port;
	p->ci = 1;

	as.p = p;
	as.i = 1;
	d->totick = 0;
	if(totk > 0)
		d->totick = totk | 1;	/* fix fencepost */

	while(waserror())
		if(interrupt){
			d->port->ci = 0;
			if(ahcicomreset(&d->portc) == -1)
				setstate(d, Dreset);
			return SDtimeout;
		}
	sleep(&d->portm, ahciclear, &as);
	poperror();

	ilock(d);
	flag = d->portm.flag;
	task = p->task;
	iunlock(d);

	rv = SDok;
	if(proto & Ppkt){
		rv = task >> 8 + 4 & 0xf;
		flag &= ~Fahdrs;
		flag |= Fdone;
	}else if(task & (Efatal<<8) || task & (ASbsy|ASdrq) && d->state == Dready){
		p->ci = 0;
		ahcirecover(&d->portc);
		task = p->task;
		flag &= ~Fdone;		/* either an error or do-over */
	}
	if(flag == 0){
		print("%s: retry\n", dnam(d));
		return SDretry;
	}
	if(flag & (Fahdrs | Ferror)){
		if((task & Eidnf) == 0)
			print("%s: i/o error %ux\n", dnam(d), task);
		return SDcheck;
	}
	return rv;
}

static int
iariopkt(SDreq *r, Drive *d)
{
	int n, count, t, max, δ;
	uchar *cmd;

	cmd = r->cmd;
	aprint("%s: %.2ux %.2ux %c %d %p\n", dnam(d), cmd[0], cmd[2],
		"rw"[r->write], r->dlen, r->data);
	r->rlen = 0;
	count = r->dlen;
	max = 65536;
	δ = r->timeout - Ticks;

	for(t = r->timeout; setreqto(r, t) != -1;){
		n = count;
		if(n > max)
			n = max;
		qlock(&d->portm);
		ahcibuildpkt(&d->portm, r, r->data, n);
		r->status = io(d, Ppkt, r->timeout, 0);
		qunlock(&d->portm);
		switch(r->status){
		case SDeio:
			return r->status = SDcheck;
		case SDretry:
			continue;
		}
//		aprint("%s: OK %.2ux :: %d :: %.4lux\n", dnam(d), r->cmd[0], r->status, d->port->task);
		r->rlen = d->portm.list->len;
		return SDok;
	}
	print("%s: atapi timeout %dms\n", dnam(d), TK2MS(δ));
	return r->status = SDcheck;
}

static long
ahcibio(SDunit *u, int lun, int write, void *a, long count0, uvlong lba)
{
	Ctlr *c;
	Drive *d;

	c = u->dev->ctlr;
	d = c->drive[u->subno];
	if(d->portm.feat & Datapi)
		return scsibiox(u, &d->portm, lun, write, a, count0, lba);
	return atabio(u, &d->portm, lun, write, a, count0, lba);
}

static int
iario(SDreq *r)
{
	Ctlr *c;
	Drive *d;
	SDunit *u;

	u = r->unit;
	c = u->dev->ctlr;
	d = c->drive[u->subno];
	if((d->state & (Dnew | Dready)) == 0)
		return sdsetsense(r, SDcheck, 3, 0x04, 0x24);
	if(r->timeout == 0)
		r->timeout = totk(Ms2tk(600*1000));
	if(d->portm.feat & Datapi)
		return iariopkt(r, d);
	return atariosata(u, &d->portm, r);
}

static uchar bogusrfis[16] = {
[Ftype]		0x34,
[Fioport]	0x40,
[Fstatus]		0x50,
[Fdev]		0xa0,
};

static void
sdr0(Drive *d)
{
	uchar *c;

	c = d->portm.fis.r;
	memmove(c, bogusrfis, sizeof bogusrfis);
	coherence();
}

static int
sdr(SDreq *r, Drive *d, int st)
{
	uchar *c;
	uint t;

	if((r->ataproto & Pprotom) == Ppkt){
		t = d->port->task;
		if(t & ASerr)
			st = t >> 8 + 4 & 0xf;
	}
	c = d->portm.fis.r;
	memmove(r->cmd, c, 16);
	r->status = st;
	if(st == SDcheck)
		st = SDok;
	return st;
}

static int
fisreqchk(Sfis *f, SDreq *r)
{
	if((r->ataproto & Pprotom) == Ppkt)
		return SDnostatus;
	/*
	 * handle oob requests;
	 *    restrict & sanitize commands
	 */
	if(r->clen != 16)
		error(Eio);
	if(r->cmd[0] == 0xf0){
		sigtofis(f, r->cmd);
		r->status = SDok;
		return SDok;
	}
	r->cmd[0] = 0x27;
	r->cmd[1] = 0x80;
	r->cmd[7] |= 0xa0;
	return SDnostatus;
}

static int
iaataio(SDreq *r)
{
	Ctlr *c;
	Drive *d;
	SDunit *u;

	u = r->unit;
	c = u->dev->ctlr;
	d = c->drive[u->subno];

	if(r->timeout == 0)
		r->timeout = totk(Ms2tk(600*1000));
	if((r->status = fisreqchk(&d->portm, r)) != SDnostatus)
		return r->status;
	r->rlen = 0;
	sdr0(d);

	qlock(&d->portm);
	ahcibuildfis(&d->portm, r, r->data, r->dlen);
	r->status = io(d, r->ataproto & Pprotom, -1, 1);
	qunlock(&d->portm);
	if(r->status != SDok)
		return r->status;
	r->rlen = r->dlen;
	if((r->ataproto & Pprotom) == Ppkt)
		r->rlen = d->portm.list->len;
	return sdr(r, d, r->status);
}

/* configure drives 0-5 as ahci sata  (c.f. errata)  */
static int
iaahcimode(Pcidev *p)
{
	uint u;

	u = pcicfgr16(p, 0x92);
	dprint("ahci: %T: iaahcimode %.2ux %.4ux\n", p->tbdf, pcicfgr8(p, 0x91), u);
	pcicfgw16(p, 0x92, u | 0xf);		/* ports 0-15 (sic) */
	return 0;
}

enum{
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

static void
sbsetupahci(Pcidev *p)
{
	print("sbsetupahci: tweaking %.4ux ccru %.2ux ccrp %.2ux\n",
		p->did, p->ccru, p->ccrp);
	pcicfgw8(p, 0x40, pcicfgr8(p, 0x40) | 1);
	pcicfgw8(p, PciCCRu, 6);
	pcicfgw8(p, PciCCRp, 1);
	p->ccru = 6;
	p->ccrp = 1;
}

static int
esbenc(Ctlr *c)
{
	c->encsz = 1;
	c->enctx = (u32int*)(c->mmio + 0xa0);
	c->enctype = Eesb;
	c->enctx[0] = 0;
	return 0;
}

static int
ahciencinit(Ctlr *c)
{
	uint type, sz, o;
	u32int *bar;
	Ahba *h;

	h = c->hba;
	if(c->type == Tesb)
		return esbenc(c);
	if((h->cap & Hems) == 0)
		return -1;
	type = h->emctl & Emtype;
	switch(type){
	case Esgpio:
	case Eses2:
	case Esafte:
		return -1;
	case Elmt:
		break;
	default:
		return -1;
	}

	sz = h->emloc & 0xffff;
	o = h->emloc>>16;
	if(sz == 0 || o == 0)
		return -1;
	bar = c->lmmio;
	ledprint("size = %#.4ux; loc = %#.4ux*4\n", sz, o);

	c->encsz = sz;
	c->enctx = bar + o;
	if((h->emctl & Xonly) == 0){
		if(h->emctl & Smb)
			c->encrx = bar + o;
		else
			c->encrx = bar + o*2;
	}
	c->enctype = type;
	return 0;
}

static ushort itab[] = {
	0xfffc,	0x2680,	Tesb,
	0xfffb,	0x27c1,	Tahci,		/* 82801g[bh]m */
	0xffff,	0x2821,	Tahci,		/* 82801h[roh] */
	0xfffe,	0x2824,	Tahci,		/* 82801h[b] */
	0xfeff,	0x2829,	Tahci,		/* ich8 */
	0xfffe,	0x2922,	Tahci,		/* ich9 */
	0xffff,	0x3a02,	Tahci,		/* 82801jd/do */
	0xfefe,	0x3a22,	Tahci,		/* ich10, pch */
	0xfff7,	0x3b28,	Tahci,		/* pchm */
	0xfffe,	0x3b22,	Tahci,		/* pch */
};

static int
didtype(Pcidev *p)
{
	int type, i;

	type = Tahci;
	switch(p->vid){
	default:
		return -1;
	case 0x8086:
		for(i = 0; i < nelem(itab); i += 3)
			if((p->did & itab[i]) == itab[i+1])
				return itab[i+2];
		break;
	case 0x1002:
		if(p->ccru == 1 || p->ccrp != 1)
		if(p->did == 0x4380 || p->did == 0x4390)
			sbsetupahci(p);
		type = Tsb600;
		break;
	case 0x1106:
		/*
		 * unconfirmed report that the programming
		 * interface is set incorrectly.
		 */
		if(p->did == 0x3349)
			return Tahci;
		break;
	case 0x10de:
	case 0x1039:
	case 0x1b4b:
	case 0x11ab:
		break;
	case 0x197b:
	case 0x10b9:
		type = Tjmicron;
		break;
	}
	if(p->ccrb == 1 && p->ccru == 6 && p->ccrp == 1)
		return type;
	return -1;
}

static SDev*
iapnp(void)
{
	int i, n, nunit, type;
	uintptr io;
	Ctlr *c;
	Drive *d;
	Pcidev *p;
	SDev *s;
	static int done;

	if(done)
		return nil;
	done = 1;
	memset(olds, 0xff, sizeof olds);
	p = nil;
loop:
	while((p = pcimatch(p, 0, 0)) != nil){
		if((type = didtype(p)) == -1)
			continue;
		if(p->mem[Abar].bar == 0)
			continue;
		if(niactlr == NCtlr){
			print("iapnp: %s: too many controllers\n", cttab[type].name);
			break;
		}
		c = iactlr + niactlr;
		s = sdevs + niactlr;
		memset(c, 0, sizeof *c);
		memset(s, 0, sizeof *s);
		io = p->mem[Abar].bar & ~0xf;
		c->mmio = vmap(io, p->mem[Abar].size);
		if(c->mmio == 0){
			print("%s: address %#p in use did %.4ux\n",
				tnam(c), io, p->did);
			continue;
		}
		c->lmmio = (u32int*)c->mmio;
		c->pci = p;
		c->type = cttab + type;

		s->ifc = &sdahciifc;
		s->idno = 'E';
		s->ctlr = c;
		c->sdev = s;

		if(intel(c) && p->did != 0x2681)
			iasetupahci(c);
//		ahcihbareset((Ahba*)c->mmio);
		nunit = ahciconf(c);
		c->pi = c->hba->pi;
		if(0 && p->vid == 0x1002 && p->did == 0x4391){
			c->pi = 0x3f;		/* noah's opteron */
			nunit = 6;
		}
		if(intel(c) && iaahcimode(p) == -1 || nunit < 1){
			vunmap(c->mmio, p->mem[Abar].size);
			continue;
		}
		c->ndrive = s->nunit = nunit;

		/* map the drives -- they don't all need to be enabled. */
		memset(c->rawdrive, 0, sizeof c->rawdrive);
		n = 0;
		for(i = 0; i < NCtlrdrv; i++){
			d = c->rawdrive + i;
			d->portno = i;
			d->driveno = -1;
			d->portm.tler = 5000;
			d->portm.sectors = 0;
			d->portm.serial[0] = ' ';
			d->led = Ibpinormal;
			d->ctlr = c;
			if((c->pi & 1<<i) == 0)
				continue;
			snprint(d->name, sizeof d->name, "iahci%d.%d", niactlr, i);
			d->port = (Aport*)(c->mmio + 0x80*i + 0x100);
			d->portc.p = d->port;
			d->portc.m = &d->portm;
			d->driveno = n++;
			c->drive[d->driveno] = d;
			iadrive[niadrive + d->driveno] = d;
		}
		for(i = 0; i < n; i++)
			if(ahciidle(c->drive[i]->port) == -1){
				print("%s: port %d wedged; abort\n",
					tnam(c), i);
				goto loop;
			}
		for(i = 0; i < n; i++){
			c->drive[i]->mode = DMautoneg;
			configdrive(c->drive[i]);
		}
		ahciencinit(c);

		niadrive += n;
		niactlr++;
		sdadddevs(s);
		i = (c->hba->cap >> 21) & 1;
		print("#S/%s: %s: sata-%s with %d ports\n", s->name,
			tnam(c), "I\0II" + i*2, nunit);
	}
	return nil;
}

static Htab ctab[] = {
	Aasp,	"asp",
	Aalpe ,	"alpe ",
	Adlae,	"dlae",
	Aatapi,	"atapi",
	Apste,	"pste",
	Afbsc,	"fbsc",
	Aesp,	"esp",
	Acpd,	"cpd",
	Ampsp,	"mpsp",
	Ahpcp,	"hpcp",
	Apma,	"pma",
	Acps,	"cps",
	Acr,	"cr",
	Afr,	"fr",
	Ampss,	"mpss",
	Apod,	"pod",
	Asud,	"sud",
	Ast,	"st",
};

static char*
capfmt(char *p, char *e, Htab *t, int n, u32int cap)
{
	uint i;

	*p = 0;
	for(i = 0; i < n; i++)
		if(cap & t[i].bit)
			p = seprint(p, e, "%s ", t[i].name);
	return p;
}

static int
iarctl(SDunit *u, char *p, int l)
{
	char buf[32], *e, *op;
	Aport *o;
	Ctlr *c;
	Drive *d;

	if((c = u->dev->ctlr) == nil)
		return 0;
	d = c->drive[u->subno];
	o = d->port;

	e = p+l;
	op = p;
	if(d->state == Dready)
		p = sfisxrdctl(&d->portm, p, e);
	else
		p = seprint(p, e, "no disk present [%s]\n", dstate(d->state));
	serrstr(o->serror, buf, buf + sizeof buf - 1);
	p = seprint(p, e, "reg\ttask %ux cmd %ux serr %ux %s ci %ux is %ux "
		"sig %ux sstatus %.3ux\n", o->task, o->cmd, o->serror, buf,
		o->ci, o->isr, o->sig, o->sstatus);
	p = seprint(p, e, "cmd\t");
	p = capfmt(p, e, ctab, nelem(ctab), o->cmd);
	p = seprint(p, e, "\n");
	p = seprint(p, e, "mode\t%s %s\n", modes[d->mode], modes[maxmode(c)]);
	p = seprint(p, e, "geometry %llud %lud\n", u->sectors, u->secsize);
	return p - op;
}

static void
forcemode(Drive *d, char *mode)
{
	int i;

	for(i = 0; i < nelem(modes); i++)
		if(strcmp(mode, modes[i]) == 0)
			break;
	if(i == nelem(modes))
		i = 0;
	ilock(d);
	d->mode = i;
	iunlock(d);
}

static void
forcestate(Drive *d, char *state)
{
	int i;

	for(i = 1; i < nelem(diskstates); i++)
		if(strcmp(state, diskstates[i]) == 0)
			break;
	if(i == nelem(diskstates))
		error(Ebadctl);
	setstate(d, 1 << i-1);
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

	if(strcmp(f[0], "mode") == 0)
		forcemode(d, f[1]? f[1]: "satai");
	else if(strcmp(f[0], "state") == 0)
		forcestate(d, f[1]? f[1]: "null");
	else
		cmderror(cmd, Ebadctl);
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

static Htab htab[] = {
	H64a,	"64a",
	Hncq,	"ncq",
	Hsntf,	"ntf",
	Hmps,	"mps",
	Hss,	"ss",
	Halp,	"alp",
	Hal,	"led",
	Hclo,	"clo",
	Ham,	"am",
	Hpm,	"pm",
	Hfbs,	"fbs",
	Hpmb,	"pmb",
	Hssc,	"slum",
	Hpsc,	"pslum",
	Hcccs,	"coal",
	Hems,	"ems",
	Hxs,	"xs",
};

static Htab htab2[] = {
	Apts,	"apts",
	Nvmp,	"nvmp",
	Boh,	"boh",
};

static Htab emtab[] = {
	Pm,	"pm",
	Alhd,	"alhd",
	Xonly,	"xonly",
	Smb,	"smb",
	Esgpio,	"esgpio",
	Eses2,	"eses2",
	Esafte,	"esafte",
	Elmt,	"elmt",
};

static char*
iartopctl(SDev *s, char *p, char *e)
{
	char pr[25];
	u32int cap;
	Ahba *h;
	Ctlr *c;

	c = s->ctlr;
	h = c->hba;
	cap = h->cap;
	p = seprint(p, e, "sd%c ahci %s port %#p: ", s->idno, tnam(c), h);
	p = capfmt(p, e, htab, nelem(htab), cap);
	p = capfmt(p, e, htab2, nelem(htab2), h->cap2);
	p = capfmt(p, e, emtab, nelem(emtab), h->emctl);
	portr(pr, pr + sizeof pr, h->pi);
	return seprint(p, e,
		"iss %d ncs %d np %d ghc %ux isr %ux pi %ux %s ver %ux\n",
		(cap>>20) & 0xf, (cap>>8) & 0x1f, 1 + (cap & 0x1f),
		h->ghc, h->isr, h->pi, pr, h->ver);
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
	else if(strcmp(f[0], "ledprint") == 0)
		v = &dled;
	else
		cmderror(cmd, Ebadctl);

	switch(cmd->nf){
	default:
		cmderror(cmd, Ebadarg);
	case 1:
		*v ^= 1;
		return 0;
	case 2:
		*v = strcmp(f[1], "on") == 0;
		return 0;
	}
}

SDifc sdahciifc = {
	"ahci",

	iapnp,
	nil,		/* legacy */
	iaenable,
	iadisable,

	iaverify,
	iaonline,
	iario,
	iarctl,
	iawctl,

	ahcibio,
	nil,		/* probe */
	nil,		/* clear */
	iartopctl,
	iawtopctl,
	iaataio,
};
