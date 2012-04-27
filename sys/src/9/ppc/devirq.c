#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"m8260.h"
#include	"../port/error.h"

enum{
	IRQ0 = 18,
	Level = 0,
	Edge = 1,
};

enum{
	Qdir,
	Qirq1,
	Qirq2,
	Qirq3,
	Qirq4,
	Qirq5,
	Qirq6,
	Qirq7,
	Qmstimer,
	Qfpgareset,
	NIRQ,
};

static Dirtab irqdir[]={
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"irq1",		{Qirq1},		0,	0666,
	"irq2",		{Qirq2},		0,	0666,
	"irq3",		{Qirq1},		0,	0666,
	"irq4",		{Qirq1},		0,	0666,
	"irq5",		{Qirq1},		0,	0666,
	"irq6",		{Qirq1},		0,	0666,
	"irq7",		{Qirq1},		0,	0666,
	"mstimer",	{Qmstimer},		0,	0666,
	"fpgareset",	{Qfpgareset},		0,	0222,
};

enum
{
	CMinterrupt,
	CMmode,
	CMreset,
	CMwait,
	CMdebug,
};

Cmdtab irqmsg[] =
{
	CMinterrupt,	"interrupt",	2,
	CMmode,		"mode",		2,
	CMreset,	"reset",	1,
	CMwait,		"wait",		1,
	CMdebug,	"debug",	1,
};

typedef struct Irqconfig Irqconfig;
struct Irqconfig {
	int		intenable;	/* Interrupts are enabled */
	int		mode;		/* level == 0; edge == 1 */
	ulong		interrupts;	/* Count interrupts */
	ulong		sleepints;	/* interrupt count when waiting */
	Rendez		r;		/* Rendez-vous point for interrupt waiting */
	Irqconfig	*next;
	Timer;
};

Irqconfig *irqconfig[NIRQ];	/* irqconfig[0] is not used */
Lock irqlock;

static void interrupt(Ureg*, void*);
void dumpvno(void);

static void
ticmstimer(Ureg*, Timer *t)
{
	Irqconfig *ic;

	ic = t->ta;
 	ic->interrupts++;
	wakeup(&ic->r);
}

void
irqenable(Irqconfig *ic, int irq)
{
	/* call with ilock(&irqlock) held */

	if (ic->intenable)
		return;
	if (irq == Qmstimer){
		if (ic->tnext == nil)
			ic->tns = MS2NS(ic->mode);
		ic->tmode = Tperiodic;
		timeradd(&ic->Timer);
	}else{
		if (irqconfig[irq]){
			ic->next = irqconfig[irq];
			irqconfig[irq] = ic;
		}else{
			ic->next = nil;
			irqconfig[irq] = ic;
			intrenable(IRQ0 + irq, interrupt, &irqconfig[irq], irqdir[irq].name);
		}
	}
	ic->intenable = 1;
}

void
irqdisable(Irqconfig *ic, int irq)
{
	Irqconfig **pic;

	/* call with ilock(&irqlock) held */

	if (ic->intenable == 0)
		return;
	if (irq == Qmstimer){
		timerdel(&ic->Timer);
	}else{
		for(pic = &irqconfig[irq]; *pic != ic; pic = &(*pic)->next)
			assert(*pic);
		*pic = (*pic)->next;
		if (irqconfig[irq] == nil)
			intrdisable(IRQ0 + irq, interrupt, &irqconfig[irq], irqdir[irq].name);
	}
	ic->intenable = 0;
}

static Chan*
irqattach(char *spec)
{
	return devattach('b', spec);
}

static Walkqid*
irqwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name,nname, irqdir, nelem(irqdir), devgen);
}

static int
irqstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, irqdir, nelem(irqdir), devgen);
}

static Chan*
irqopen(Chan *c, int omode)
{
	Irqconfig *ic;
	int irq;

	irq = (ulong)c->qid.path;
	if(irq != Qdir){
		ic = mallocz(sizeof(Irqconfig), 1);
		ic->tf = ticmstimer;
		ic->ta = ic;
		if (irq == Qmstimer)
			ic->mode = 1000;
		c->aux = ic;
	}
	return devopen(c, omode, irqdir, nelem(irqdir), devgen);
}

static void
irqclose(Chan *c)
{
	int irq;
	Irqconfig *ic;

	irq = (ulong)c->qid.path;
	if(irq == Qdir)
		return;
	ic = c->aux;
	if (irq > Qmstimer)
		return;
	ilock(&irqlock);
	irqdisable(ic, irq);
	iunlock(&irqlock);
	free(ic);
}

static int
irqtfn(void *arg)
{
	Irqconfig *ic;

	ic = arg;
	return ic->sleepints != ic->interrupts;
}

static long
irqread(Chan *c, void *buf, long n, vlong)
{
	int irq;
	Irqconfig *ic;
	char tmp[24];

	if(n <= 0)
		return n;
	irq = (ulong)c->qid.path;
	if(irq == Qdir)
		return devdirread(c, buf, n, irqdir, nelem(irqdir), devgen);
	if(irq > Qmstimer){
		print("irqread 0x%llux\n", c->qid.path);
		error(Egreg);
	}
	ic = c->aux;
	if (ic->intenable == 0)
		error("disabled");
	ic->sleepints = ic->interrupts;
	sleep(&ic->r, irqtfn, ic);
	if (irq == Qmstimer)
		snprint(tmp, sizeof tmp, "%11lud %d", ic->interrupts, ic->mode);
	else
		snprint(tmp, sizeof tmp, "%11lud %s", ic->interrupts, ic->mode ?"edge":"level");
	n = readstr(0, buf, n, tmp);
	return n;
}

static long
irqwrite(Chan *c, void *a, long n, vlong)
{
	int irq;
	Irqconfig *ic;
	Cmdbuf *cb;
	Cmdtab *ct;

	if(n <= 0)
		return n;

	irq = (ulong)c->qid.path;
	if(irq <= 0 || irq >= nelem(irqdir)){
		print("irqwrite 0x%llux\n", c->qid.path);
		error(Egreg);
	}
	if (irq == Qfpgareset){
		if (strncmp(a, "reset", 5) == 0)
			fpgareset();
		else
			error(Egreg);
		return n;
	}
	ic = c->aux;

	cb = parsecmd(a, n);

	if(waserror()) {
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, irqmsg, nelem(irqmsg));
	switch(ct->index) {
	case 	CMinterrupt:
		/* Turn interrupts on or off */
		if (strcmp(cb->f[1], "on") == 0){
			ilock(&irqlock);
			irqenable(ic, irq);
			iomem->siprr = 0x65009770;
			iunlock(&irqlock);
		}else if (strcmp(cb->f[1], "off") == 0){
			ilock(&irqlock);
			irqdisable(ic, irq);
			iunlock(&irqlock);
		}else
			error(Ebadarg);
		break;
	case CMmode:
		/* Set mode */
		if (irq == Qmstimer){
			ic->mode = strtol(cb->f[1], nil, 0);
			if (ic->mode <= 0){
				ic->tns = MS2NS(1000);
				ic->mode = 1000;
				error(Ebadarg);
			}
			ic->tns = MS2NS(ic->mode);
		}else if (strcmp(cb->f[1], "level") == 0){
			ic->mode = Level;
			iomem->siexr &= ~(0x8000 >> irq);
		}else if (strcmp(cb->f[1], "edge") == 0){
			ic->mode = Edge;
			iomem->siexr |= 0x8000 >> irq;
		}else
			error(Ebadarg);
		break;
	case CMreset:
		ic->interrupts = 0;
		break;
	case CMwait:
		if (ic->intenable == 0)
			error("interrupts are off");
		ic->sleepints = ic->interrupts;
		sleep(&ic->r, irqtfn, ic);
		break;
	case CMdebug:
		print("simr h/l 0x%lux/0x%lux, sipnr h/l 0x%lux/0x%lux, siexr 0x%lux, siprr 0x%lux\n",
			iomem->simr_h, iomem->simr_l,
			iomem->sipnr_h, iomem->sipnr_l,
			iomem->siexr, iomem->siprr);
		dumpvno();
	}
	poperror();
	free(cb);

	/* Irqi */
	return n;
}

static void
interrupt(Ureg*, void *arg)
{
	Irqconfig **pic, *ic;
	int irq;

	pic = arg;
	irq = pic - irqconfig;
	if (irq <= 0 || irq > nelem(irqdir)){
		print("Unexpected interrupt: %d\n", irq);
		return;
	}
	ilock(&irqlock);
	if (irq <= Qirq7)
		iomem->sipnr_h |= 0x8000 >> irq;	/* Clear the interrupt */
	for(ic = *pic; ic; ic = ic->next){
		ic->interrupts++;
		wakeup(&ic->r);
	}
	iunlock(&irqlock);
}

Dev irqdevtab = {
	'b',
	"irq",

	devreset,
	devinit,
	devshutdown,
	irqattach,
	irqwalk,
	irqstat,
	irqopen,
	devcreate,
	irqclose,
	irqread,
	devbread,
	irqwrite,
	devbwrite,
	devremove,
	devwstat,
};
