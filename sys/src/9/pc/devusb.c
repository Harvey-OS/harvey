#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"usb.h"

static int debug = 0;

#define Chatty	1
#define DPRINT if(Chatty)print
#define XPRINT if(debug)iprint

Usbhost*	usbhost[MaxUsb];

static char *devstates[] = {
	[Disabled]		"Disabled",
	[Attached]	"Attached",
	[Enabled]		"Enabled",
	[Assigned]	"Assigned",
	[Configured]	"Configured",
};

static	char	Ebadusbmsg[] = "invalid parameters to USB ctl message";

enum
{
	Qtopdir = 0,
	Q2nd,
	Qnew,
	Qport,
	Q3rd,
	Qctl,
	Qstatus,
	Qep0,
	/* other endpoint files */
};

/*
 * Qid path is:
 *	8 bits of file type (qids above)
 *	8 bits of slot number; default address 0 used for per-controller files
 *	4 bits of controller number
 */
enum {
	TYPEBITS	= 8,
	SLOTBITS	= 8,
	CTLRBITS	= 4,

	SLOTSHIFT	= TYPEBITS,
	CTLRSHIFT	= SLOTSHIFT+SLOTBITS,

	TYPEMASK	= (1<<TYPEBITS)-1,
	SLOTMASK	= (1<<SLOTBITS)-1,
	CTLRMASK	= (1<<CTLRBITS)-1,
};

#define	TYPE(q)		(((ulong)(q).path)&TYPEMASK)
#define	SLOT(q)		((((ulong)(q).path)>>SLOTSHIFT)&SLOTMASK)
#define	CTLR(q)		((((ulong)(q).path)>>CTLRSHIFT)&CTLRMASK)
#define	PATH(t, s, c)	((t)|((s)<<SLOTSHIFT)|((c)<<CTLRSHIFT))

static Dirtab usbdir2[] = {
	"new",	{Qnew},			0,	0666,
	"port",	{Qport},			0,	0666,
};

static Dirtab usbdir3[]={
	"ctl",		{Qctl},			0,	0666,
	"status",	{Qstatus},			0,	0444,
	"setup",	{Qep0},			0,	0666,
	/* epNdata names are generated on demand */
};

enum
{
	PMdisable,
	PMenable,
	PMreset,
};

enum
{
	CMclass,
	CMdata,
	CMdebug,
	CMep,
	CMmaxpkt,
	CMadjust,
	CMspeed,
	CMunstall,
};

static Cmdtab usbportmsg[] =
{
	PMdisable,	"disable",	2,
	PMenable,		"enable",	2,
	PMreset,		"reset",	2,
};

static Cmdtab usbctlmsg[] =
{
	CMclass,		"class",	0,
	CMdata,		"data",	3,
	CMdebug,		"debug",	3,
	CMep,		"ep",		6,
	CMmaxpkt,	"maxpkt",	3,
	CMadjust,		"adjust",	3,
	CMspeed,		"speed",	2,
	CMunstall,	"unstall",	2,
};

static struct
{
	char*	type;
	int	(*reset)(Usbhost*);
} usbtypes[MaxUsb+1];

void
addusbtype(char* t, int (*r)(Usbhost*))
{
	static int ntype;

	if(ntype == MaxUsb)
		panic("too many USB host interface types");
	usbtypes[ntype].type = t;
	usbtypes[ntype].reset = r;
	ntype++;
}

static Udev*
usbdeviceofslot(Usbhost *uh, int s)
{
	if(s < 0 || s > nelem(uh->dev))
		return nil;
	return uh->dev[s];
}

static Udev*
usbdevice(Chan *c)
{
	int bus;
	Udev *d;
	Usbhost *uh;

	bus = CTLR(c->qid);
	if(bus > nelem(usbhost) || (uh = usbhost[bus]) == nil) {
		error(Egreg);
		return nil;		/* for compiler */
	}
	d = usbdeviceofslot(uh, SLOT(c->qid));
	if(d == nil || d->id != c->qid.vers || d->state == Disabled)
		error(Ehungup);
	return d;
}

static Endpt *
devendpt(Udev *d, int id, int add)
{
	Usbhost *uh;
	Endpt *e, **p;

	p = &d->ep[id&0xF];
	lock(d);
	e = *p;
	if(e != nil){
		incref(e);
		XPRINT("incref(0x%p) in devendpt, new value %ld\n", e, e->ref);
		unlock(d);
		return e;
	}
	unlock(d);
	if(!add)
		return nil;

	e = mallocz(sizeof(*e), 1);
	e->ref = 1;
	e->x = id&0xF;
	e->id = id;
	e->sched = -1;
	e->maxpkt = 8;
	e->nbuf = 1;
	e->dev = d;
	e->active = 0;

	uh = d->uh;
	uh->epalloc(uh, e);

	lock(d);
	if(*p != nil){
		incref(*p);
		XPRINT("incref(0x%p) in devendpt, new value %ld\n", *p, (*p)->ref);
		unlock(d);
		uh->epfree(uh, e);
		free(e);
		return *p;
	}
	*p = e;
	unlock(d);
	e->rq = qopen(8*1024, 0, nil, e);
	e->wq = qopen(8*1024, 0, nil, e);
	return e;
}

static void
freept(Endpt *e)
{
	Usbhost *uh;

	if(e != nil && decref(e) == 0){
		XPRINT("freept(%d,%d)\n", e->dev->x, e->x);
		uh = e->dev->uh;
		uh->epclose(uh, e);
		e->dev->ep[e->x] = nil;
		uh->epfree(uh, e);
		free(e);
	}
}

static Udev*
usbnewdevice(Usbhost *uh)
{
	int i;
	Udev *d;
	Endpt *e;

	d = nil;
	qlock(uh);
	if(waserror()){
		qunlock(uh);
		nexterror();
	}
	for(i=0; i<nelem(uh->dev); i++)
		if(uh->dev[i] == nil){
			uh->idgen++;
			d = mallocz(sizeof(*d), 1);
			d->uh = uh;
			d->ref = 1;
			d->x = i;
			d->id = (uh->idgen << 8) | i;
			d->state = Enabled;
			XPRINT("calling devendpt in usbnewdevice\n");
			e = devendpt(d, 0, 1);	/* always provide control endpoint 0 */
			e->mode = ORDWR;
			e->iso = 0;
			e->sched = -1;
			uh->dev[i] = d;
			break;
		}
	poperror();
	qunlock(uh);
	return d;
}

static void
freedev(Udev *d, int ept)
{
	int i;
	Endpt *e;
	Usbhost *uh;

	uh = d->uh;
	if(decref(d) == 0){
		XPRINT("freedev 0x%p, 0\n", d);
		for(i=0; i<nelem(d->ep); i++)
			freept(d->ep[i]);
		if(d->x >= 0)
			uh->dev[d->x] = nil;
		free(d);
	} else {
		if(ept >= 0 && ept < nelem(d->ep)){
			e = d->ep[ept];
			XPRINT("freedev, freept 0x%p\n", e);
			if(e != nil)
				uh->epclose(uh, e);
		}
	}	
}

static int
usbgen(Chan *c, char *, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	Udev *d;
	Endpt *e;
	Dirtab *tab;
	Usbhost *uh;
	int t, bus, slot, perm;

	/*
	 * Top level directory contains the controller names.
	 */
	if(c->qid.path == Qtopdir){
		if(s == DEVDOTDOT){
			mkqid(&q, Qtopdir, 0, QTDIR);
			devdir(c, q, "#U", 0, eve, 0555, dp);
			return 1;
		}
		if(s >= nelem(usbhost) || usbhost[s] == nil)
			return -1;
		mkqid(&q, PATH(Q2nd, 0, s), 0, QTDIR);
		snprint(up->genbuf, sizeof up->genbuf, "usb%d", s);
		devdir(c, q, up->genbuf, 0, eve, 0555, dp);
		return 1;
	}
	bus = CTLR(c->qid);
	if(bus >= nelem(usbhost) || (uh = usbhost[bus]) == nil)
			return -1;

	/*
	 * Second level contains "new", "port", and a numbered
	 * directory for each enumerated device on the bus.
	 */
	t = TYPE(c->qid);
	if(t < Q3rd){
		if(s == DEVDOTDOT){
			mkqid(&q, Qtopdir, 0, QTDIR);
			devdir(c, q, "#U", 0, eve, 0555, dp);
			return 1;
		}
		if(s < nelem(usbdir2)){
			d = uh->dev[0];
			if(d == nil)
				return -1;
			tab = &usbdir2[s];
			mkqid(&q, PATH(tab->qid.path, 0, bus), d->id, QTFILE);
			devdir(c, q, tab->name, tab->length, eve, tab->perm, dp);
			return 1;
		}
		s -= nelem(usbdir2);
		if(s >= 0 && s < nelem(uh->dev)) {
			d = uh->dev[s];
			if(d == nil)
				return 0;
			sprint(up->genbuf, "%d", s);
			mkqid(&q, PATH(Q3rd, s, bus), d->id, QTDIR);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		return -1;
	}

	/*
	 * Third level.
	 */
	slot = SLOT(c->qid);
	if(s == DEVDOTDOT) {
		mkqid(&q, PATH(Q2nd, 0, bus), c->qid.vers, QTDIR);
		snprint(up->genbuf, sizeof up->genbuf, "usb%d", bus);
		devdir(c, q, up->genbuf, 0, eve, 0555, dp);
		return 1;
	}
	if(s < nelem(usbdir3)) {
		tab = &usbdir3[s];
		mkqid(&q, PATH(tab->qid.path, slot, bus), c->qid.vers, QTFILE);
		devdir(c, q, tab->name, tab->length, eve, tab->perm, dp);
		return 1;
	}
	s -= nelem(usbdir3);

	/* active endpoints */
	d = usbdeviceofslot(uh, slot);
	if(d == nil || s >= nelem(d->ep))
		return -1;
	if(s == 0 || (e = d->ep[s]) == nil)	/* ep0data is called "setup" */
		return 0;
	sprint(up->genbuf, "ep%ddata", s);
	mkqid(&q, PATH(Qep0+s, slot, bus), c->qid.vers, QTFILE);
	switch(e->mode) {
	case OREAD:
		perm = 0444;
		break;
	case OWRITE:
		perm = 0222;
		break;
	default:
		perm = 0666;
		break;
	}
	devdir(c, q, up->genbuf, e->buffered, eve, perm, dp);
	return 1;
}

static Usbhost*
usbprobe(int cardno, int ctlrno)
{
	Usbhost *uh;
	char buf[128], *ebuf, name[64], *p, *type;

	uh = malloc(sizeof(Usbhost));
	memset(uh, 0, sizeof(Usbhost));
	uh->tbdf = BUSUNKNOWN;

	if(cardno < 0){
		if(isaconfig("usb", ctlrno, uh) == 0){
			free(uh);
			return nil;
		}
		for(cardno = 0; usbtypes[cardno].type; cardno++){
			type = uh->type;
			if(type==nil || *type==0)
				type = "uhci";
			if(cistrcmp(usbtypes[cardno].type, type))
				continue;
			break;
		}
	}

	if(cardno >= MaxUsb || usbtypes[cardno].type == nil){
		free(uh);
		return nil;
	}
	if(usbtypes[cardno].reset(uh) < 0){
		free(uh);
		return nil;
	}

	/*
	 * IRQ2 doesn't really exist, it's used to gang the interrupt
	 * controllers together. A device set to IRQ2 will appear on
	 * the second interrupt controller as IRQ9.
	 */
	if(uh->irq == 2)
		uh->irq = 9;
	snprint(name, sizeof(name), "usb%d", ctlrno);
	intrenable(uh->irq, uh->interrupt, uh, uh->tbdf, name);

	ebuf = buf + sizeof buf;
	p = seprint(buf, ebuf, "#U/usb%d: %s: port 0x%luX irq %d", ctlrno, usbtypes[cardno].type, uh->port, uh->irq);
	if(uh->mem)
		p = seprint(p, ebuf, " addr 0x%luX", PADDR(uh->mem));
	if(uh->size)
		seprint(p, ebuf, " size 0x%luX", uh->size);
	print("%s\n", buf);

	return uh;
}

static void
usbreset(void)
{
	int cardno, ctlrno;
	Usbhost *uh;

	for(ctlrno = 0; ctlrno < MaxUsb; ctlrno++){
		if((uh = usbprobe(-1, ctlrno)) == nil)
			continue;
		usbhost[ctlrno] = uh;
	}

	if(getconf("*nousbprobe"))
		return;

	cardno = ctlrno = 0;
	while(usbtypes[cardno].type != nil && ctlrno < MaxUsb){
		if(usbhost[ctlrno] != nil){
			ctlrno++;
			continue;
		}
		if((uh = usbprobe(cardno, ctlrno)) == nil){
			cardno++;
			continue;
		}
		usbhost[ctlrno] = uh;
		ctlrno++;
	}
}

void
usbinit(void)
{
	Udev *d;
	int ctlrno;
	Usbhost *uh;

	for(ctlrno = 0; ctlrno < MaxUsb; ctlrno++){
		uh = usbhost[ctlrno];
		if(uh == nil)
			continue;
		if(uh->init != 0)
			uh->init(uh);

		/* reserve device for configuration */
		d = usbnewdevice(uh);
		incref(d);
		d->state = Attached;
	}
}

Chan *
usbattach(char *spec)
{
	return devattach('U', spec);
}

static Walkqid*
usbwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, usbgen);
}

static int
usbstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, nil, 0, usbgen);
}

Chan*
usbopen(Chan *c, int omode)
{
	Udev *d;
	Endpt *e;
	int f, s, type;
	Usbhost *uh;

	if(c->qid.type == QTDIR)
		return devopen(c, omode, nil, 0, usbgen);

	f = 0;
	type = TYPE(c->qid);
	if(type == Qnew){
		d = usbdevice(c);
		d = usbnewdevice(d->uh);
		XPRINT("usbopen, new dev 0x%p\n", d);
		if(d == nil) {
			XPRINT("usbopen failed (usbnewdevice)\n");
			error(Enodev);
		}
		type = Qctl;
		mkqid(&c->qid, PATH(type, d->x, CTLR(c->qid)), d->id, QTFILE);
		f = 1;
	}

	if(type < Q3rd){
		XPRINT("usbopen, devopen < Q3rd\n");
		return devopen(c, omode, nil, 0, usbgen);
	}

	d = usbdevice(c);
	uh = d->uh;
	qlock(uh);
	if(waserror()){
		qunlock(uh);
		nexterror();
	}

	switch(type){
	case Qctl:
		if(0&&d->busy)
			error(Einuse);
		d->busy = 1;
		if(!f)
			incref(d);
		XPRINT("usbopen, Qctl 0x%p\n", d);
		break;

	default:
		s = type - Qep0;
		XPRINT("usbopen, default 0x%p, %d\n", d, s);
		if(s >= 0 && s < nelem(d->ep)){
			if((e = d->ep[s]) == nil) {
				XPRINT("usbopen failed (endpoint)\n");
				error(Enodev);
			}
			XPRINT("usbopen: dev 0x%p, ept 0x%p\n", d, e);
			uh->epopen(uh, e);
			e->foffset = 0;
			e->toffset = 0;
			e->poffset = 0;
			e->buffered = 0;
		}
		incref(d);
		break;
	}
	poperror();
	qunlock(uh);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
usbclose(Chan *c)
{
	Udev *d;
	int ept, type;
	Usbhost *uh;

	type = TYPE(c->qid);
	if(c->qid.type == QTDIR || type < Q3rd)
		return;
	d = usbdevice(c);
	uh = d->uh;
	qlock(uh);
	if(waserror()){
		qunlock(uh);
		nexterror();
	}
	if(type == Qctl)
		d->busy = 0;
	XPRINT("usbclose: dev 0x%p\n", d);
	if(c->flag & COPEN){
		ept = (type != Qctl) ? type - Qep0 : -1;
		XPRINT("usbclose: freedev 0x%p\n", d);
		freedev(d, ept);
	}
	poperror();
	qunlock(uh);
}

static char *
epstatus(char *s, char *se, Endpt *e, int i)
{
	char *p;

	p = seprint(s, se, "%2d %#6.6lux %10lud bytes %10lud blocks\n", i, e->csp, e->nbytes, e->nblocks);
	if(e->iso){
		p = seprint(p, se, "bufsize %6d buffered %6d", e->maxpkt, e->buffered);
		if(e->toffset)
			p = seprint(p, se, " offset  %10lud time %19lld\n", e->toffset, e->time);
		p = seprint(p, se, "\n");
	}
	return p;
}

long
usbread(Chan *c, void *a, long n, vlong offset)
{
	int t, i;
	Udev *d;
	Endpt *e;
	Usbhost *uh;
	char *s, *se, *p;

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, nil, 0, usbgen);

	d = usbdevice(c);
	uh = d->uh;
	t = TYPE(c->qid);

	if(t >= Qep0) {
		t -= Qep0;
		if(t >= nelem(d->ep))
			error(Eio);
		e = d->ep[t];
		if(e == nil || e->mode == OWRITE)
			error(Egreg);
		if(t == 0) {
			if(e->iso)
				error(Egreg);
			e->data01 = 1;
			n = uh->read(uh, e, a, n, 0LL);
			if(e->setin){
				e->setin = 0;
				e->data01 = 1;
				uh->write(uh, e, "", 0, 0LL, TokOUT);
			}
			return n;
		}
		return uh->read(uh, e, a, n, offset);
	}

	s = smalloc(READSTR);
	se = s+READSTR;
	if(waserror()){
		free(s);
		nexterror();
	}
	switch(t){
	case Qport:
		uh->portinfo(uh, s, se);
		break;

	case Qctl:
		seprint(s, se, "%11d %11d\n", d->x, d->id);
		break;

	case Qstatus:
		if (d->did || d->vid)
			p = seprint(s, se, "%s %#6.6lux %#4.4ux %#4.4ux\n", devstates[d->state], d->csp, d->vid, d->did);
		else
			p = seprint(s, se, "%s %#6.6lux\n", devstates[d->state], d->csp);
		for(i=0; i<nelem(d->ep); i++) {
			e = d->ep[i];
			if(e == nil)
				continue;
			/* TO DO: freeze e */
			p = epstatus(p, se, e, i);
		}
	}
	n = readstr(offset, a, n, s);
	poperror();
	free(s);
	return n;
}

long
usbwrite(Chan *c, void *a, long n, vlong offset)
{
	Udev *d;
	Endpt *e;
	Cmdtab *ct;
	Cmdbuf *cb;
	Usbhost *uh;
	int id, nw, t, i;
	char cmd[50];

	if(c->qid.type == QTDIR)
		error(Egreg);
	d = usbdevice(c);
	uh = d->uh;
	t = TYPE(c->qid);
	switch(t){
	case Qport:
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}

		ct = lookupcmd(cb, usbportmsg, nelem(usbportmsg));
		id = strtol(cb->f[1], nil, 0);
		switch(ct->index){
		case PMdisable:
			uh->portenable(uh, id, 0);
			break;
		case PMenable:
			uh->portenable(uh, id, 1);
			break;
		case PMreset:
			uh->portreset(uh, id);
			break;
		}
	
		poperror();
		free(cb);
		return n;
	case Qctl:
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}

		ct = lookupcmd(cb, usbctlmsg, nelem(usbctlmsg));
		switch(ct->index){
		case CMspeed:
			d->ls = strtoul(cb->f[1], nil, 0) == 0;
			break;
		case CMclass:
			if (cb->nf != 4 && cb->nf != 6)
				cmderror(cb, Ebadusbmsg);
			/* class #ifc ept csp ( == class subclass proto) [vendor product] */
			d->npt = strtoul(cb->f[1], nil, 0);	/* # of interfaces */
			i = strtoul(cb->f[2], nil, 0);		/* endpoint */
			if (i < 0 || i >= nelem(d->ep)
			 || d->npt > nelem(d->ep) || i >= d->npt)
				cmderror(cb, Ebadusbmsg);
			if (cb->nf == 6) {
				d->vid = strtoul(cb->f[4], nil, 0);
				d->did = strtoul(cb->f[5], nil, 0);
			}
			if (i == 0)
				d->csp = strtoul(cb->f[3], nil, 0);
			if(d->ep[i] == nil){
				XPRINT("calling devendpt in usbwrite (CMclass)\n");
				d->ep[i] = devendpt(d, i, 1);
			}
			d->ep[i]->csp = strtoul(cb->f[3], nil, 0);
			break;
		case CMdata:
			i = strtoul(cb->f[1], nil, 0);
			if(i < 0 || i >= nelem(d->ep) || d->ep[i] == nil)
				error(Ebadusbmsg);
			e = d->ep[i];
			e->data01 = strtoul(cb->f[2], nil, 0) != 0;
			break;
		case CMmaxpkt:
			i = strtoul(cb->f[1], nil, 0);
			if(i < 0 || i >= nelem(d->ep) || d->ep[i] == nil)
				error(Ebadusbmsg);
			e = d->ep[i];
			e->maxpkt = strtoul(cb->f[2], nil, 0);
			if(e->maxpkt > 1500)
				e->maxpkt = 1500;
			break;
		case CMadjust:
			i = strtoul(cb->f[1], nil, 0);
			if(i < 0 || i >= nelem(d->ep) || d->ep[i] == nil)
				error(Ebadusbmsg);
			e = d->ep[i];
			if (e->iso == 0)
				error(Eperm);
			i = strtoul(cb->f[2], nil, 0);
			/* speed may not result in change of maxpkt */
			if (i < (e->maxpkt-1)/e->samplesz * 1000/e->pollms
			  || i > e->maxpkt/e->samplesz * 1000/e->pollms){
				snprint(cmd, sizeof(cmd), "%d < %d < %d?",
					(e->maxpkt-1)/e->samplesz * 1000/e->pollms,
					i,
					e->maxpkt/e->samplesz * 1000/e->pollms);
				error(cmd);
			}
			e->hz = i;
			break;
		case CMdebug:
			i = strtoul(cb->f[1], nil, 0);
			if(i < -1 || i >= nelem(d->ep) || d->ep[i] == nil)
				error(Ebadusbmsg);
			if (i == -1)
				debug = 0;
			else {
				debug = 1;
				e = d->ep[i];
				e->debug = strtoul(cb->f[2], nil, 0);
			}
			break;
		case CMunstall:
			i = strtoul(cb->f[1], nil, 0);
			if(i < 0 || i >= nelem(d->ep) || d->ep[i] == nil)
				error(Ebadusbmsg);
			e = d->ep[i];
			e->err = nil;
			break;
		case CMep:
			/* ep n `bulk' mode maxpkt nbuf     OR
			 * ep n period mode samplesize Hz
			 */
			i = strtoul(cb->f[1], nil, 0);
			if(i < 0 || i >= nelem(d->ep)) {
				XPRINT("field 1: 0 <= %d < %d\n", i, nelem(d->ep));
				error(Ebadarg);
			}
			if((e = d->ep[i]) == nil){
				XPRINT("calling devendpt in usbwrite (CMep)\n");
				e = devendpt(d, i, 1);
			}
			qlock(uh);
			if(waserror()){
				freept(e);
				qunlock(uh);
				nexterror();
			}
			if(e->active)
				error(Eperm);
			if(strcmp(cb->f[2], "bulk") == 0){
				/* ep n `bulk' mode maxpkt nbuf */
				e->iso = 0;
				i = strtoul(cb->f[4], nil, 0);
				if(i < 8 || i > 1023)
					i = 8;
				e->maxpkt = i;
				i = strtoul(cb->f[5], nil, 0);
				if(i >= 1 && i <= 32)
					e->nbuf = i;
			} else {
				/* ep n period mode samplesize Hz */
				i = strtoul(cb->f[2], nil, 0);
				if(i > 0 && i <= 1000){
					e->pollms = i;
				}else {
					XPRINT("field 4: 0 <= %d <= 1000\n", i);
					error(Ebadarg);
				}
				i = strtoul(cb->f[4], nil, 0);
				if(i >= 1 && i <= 8){
					e->samplesz = i;
				}else {
					XPRINT("field 4: 0 < %d <= 8\n", i);
					error(Ebadarg);
				}
				i = strtoul(cb->f[5], nil, 0);
				if(i >= 1 && i*e->samplesz <= 12*1000*1000){
					/* Hz */
					e->hz = i;
					e->remain = 0;
				}else {
					XPRINT("field 5: 1 < %d <= 100000 Hz\n", i);
					error(Ebadarg);
				}
				e->maxpkt = (e->hz * e->pollms + 999)/1000 * e->samplesz;
				e->iso = 1;
			}
			e->mode = strcmp(cb->f[3],"r") == 0? OREAD :
				  	strcmp(cb->f[3],"w") == 0? OWRITE : ORDWR;
			uh->epmode(uh, e);
			poperror();
			qunlock(uh);
		}
	
		poperror();
		free(cb);
		return n;

	case Qep0:	/* SETUP endpoint 0 */
		/* should canqlock etc */
		e = d->ep[0];
		if(e == nil || e->iso)
			error(Egreg);
		if(n < 8)
			error(Eio);
		nw = *(uchar*)a & RD2H;
		e->data01 = 0;
		n = uh->write(uh, e, a, n, 0LL, TokSETUP);
		if(nw == 0) {	/* host to device: use IN[DATA1] to ack */
			e->data01 = 1;
			nw = uh->read(uh, e, cmd, 0LL, 8);
			if(nw != 0)
				error(Eio);	/* could provide more status */
		}else
			e->setin = 1;	/* two-phase */
		break;

	default:	/* sends DATA[01] */
		t -= Qep0;
		if(t < 0 || t >= nelem(d->ep))
			error(Egreg);
		e = d->ep[t];
		if(e == nil || e->mode == OREAD)
			error(Egreg);
		n = uh->write(uh, e, a, n, offset, TokOUT);
		break;
	}
	return n;
}

Dev usbdevtab = {
	'U',
	"usb",

	usbreset,
	usbinit,
	devshutdown,
	usbattach,
	usbwalk,
	usbstat,
	usbopen,
	devcreate,
	usbclose,
	usbread,
	devbread,
	usbwrite,
	devbwrite,
	devremove,
	devwstat,
};
