#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

/*
 * BUG: insertion events are detected by polling.
 *      Should look into the compaq docs to see if
 *      there's an interrupt for card insertion
 *      there's probably one.
 */

static PCMslot	slot[2];
int nslot = 2;

struct {
	Ref;
	Rendez	event;		// where to wait for card events
	int	evreader;	// there's a reader for events
} pcmcia;

enum
{
	Qdir,
	Qmem,
	Qattr,
	Qctl,
	Qevs,

	Nents = 3,
};

enum
{
	/*
	 *  configuration registers - they start at an offset in attribute
	 *  memory found in the CIS.
	 */
	Rconfig=	0,
	 Creset=	 (1<<7),	/*  reset device */
	 Clevel=	 (1<<6),	/*  level sensitive interrupt line */
};

static void increfp(PCMslot*);
static void decrefp(PCMslot*);
static void slotmap(int, ulong, ulong, ulong);
static void slottiming(int, int, int, int, int);
static void slotinfo(Ureg*, void*);

#define TYPE(c)		(((ulong)c->qid.path)&0xff)
#define PATH(s,t)	(((s)<<8)|(t))

static PCMslot*
slotof(Chan *c)
{
	ulong x;

	x = c->qid.path;
	return slot + ((x>>8)&0xff);
}

static int
pcmgen(Chan *c, char *, Dirtab * , int, int i, Dir *dp)
{
	int slotno;
	Qid qid;
	long len;
	PCMslot *sp;

	if(i == DEVDOTDOT){
		mkqid(&qid, Qdir, 0, QTDIR);
		devdir(c, qid, "#y", 0, eve, 0555, dp);
		return 1;
	}

	if(i >= Nents*nslot + 1)
		return -1;
	if(i == Nents*nslot){
		len = 0;
		qid.path = PATH(0, Qevs);
		snprint(up->genbuf, sizeof up->genbuf, "pcmevs");
		goto found;
	}

	slotno = i/Nents;
	sp = slot + slotno;
	len = 0;
	switch(i%Nents){
	case 0:
		qid.path = PATH(slotno, Qmem);
		snprint(up->genbuf, sizeof up->genbuf, "pcm%dmem", slotno);
		len = sp->memlen;
		break;
	case 1:
		qid.path = PATH(slotno, Qattr);
		snprint(up->genbuf, sizeof up->genbuf, "pcm%dattr", slotno);
		len = sp->memlen;
		break;
	case 2:
		qid.path = PATH(slotno, Qctl);
		snprint(up->genbuf, sizeof up->genbuf, "pcm%dctl", slotno);
		break;
	}
found:
	qid.vers = 0;
	qid.type = QTFILE;
	devdir(c, qid, up->genbuf, len, eve, 0660, dp);
	return 1;
}

static int
bitno(ulong x)
{
	int i;

	for(i = 0; i < 8*sizeof(x); i++)
		if((1<<i) & x)
			break;
	return i;
}

/*
 *  set up the cards, default timing is 300 ns
 */
static void
pcmciareset(void)
{
	/* staticly map the whole area */
	slotmap(0, PHYSPCM0REGS, PYHSPCM0ATTR, PYHSPCM0MEM);
	slotmap(1, PHYSPCM1REGS, PYHSPCM1ATTR, PYHSPCM1MEM);

	/* set timing to the default, 300 */
	slottiming(0, 300, 300, 300, 0);
	slottiming(1, 300, 300, 300, 0);

	/* if there's no pcmcia sleave, no interrupts */
	if(gpioregs->level & GPIO_OPT_IND_i)
		return;

	/* sleave there, interrupt on card removal */
	intrenable(GPIOrising, bitno(GPIO_CARD_IND1_i), slotinfo, nil, "pcmcia slot1 status");
	intrenable(GPIOrising, bitno(GPIO_CARD_IND0_i), slotinfo, nil, "pcmcia slot0 status");
}

static Chan*
pcmciaattach(char *spec)
{
	return devattach('y', spec);
}

static Walkqid*
pcmciawalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, pcmgen);
}

static int
pcmciastat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, 0, 0, pcmgen);
}

static Chan*
pcmciaopen(Chan *c, int omode)
{
	PCMslot *slotp;

	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Eperm);
	} else {
		slotp = slotof(c);
		increfp(slotp);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
pcmciaclose(Chan *c)
{
	if(c->flag & COPEN)
		if((c->qid.type & QTDIR) == 0)
			decrefp(slotof(c));
}

/* a memmove using only bytes */
static void
memmoveb(uchar *to, uchar *from, int n)
{
	while(n-- > 0)
		*to++ = *from++;
}

/* a memmove using only shorts & bytes */
static void
memmoves(uchar *to, uchar *from, int n)
{
	ushort *t, *f;

	if((((ulong)to) & 1) || (((ulong)from) & 1) || (n & 1)){
		while(n-- > 0)
			*to++ = *from++;
	} else {
		n = n/2;
		t = (ushort*)to;
		f = (ushort*)from;
		while(n-- > 0)
			*t++ = *f++;
	}
}

static long
pcmread(void *a, long n, ulong off, PCMslot *sp, uchar *start, ulong len)
{
	rlock(sp);
	if(waserror()){
		runlock(sp);
		nexterror();
	}
	if(off > len)
		return 0;
	if(off + n > len)
		n = len - off;
	memmoveb(a, start+off, n);
	runlock(sp);
	poperror();
	return n;
}

static long
pcmctlread(void *a, long n, ulong off, PCMslot *sp)
{
	char *p, *buf, *e;

	buf = p = malloc(READSTR);
	if(waserror()){
		free(buf);
		nexterror();
	}
	e = p + READSTR;

	buf[0] = 0;
	if(sp->occupied){
		p = seprint(p, e, "occupied\n");
		if(sp->verstr[0])
			p = seprint(p, e, "version %s\n", sp->verstr);
	}
	USED(p);

	n = readstr(off, a, n, buf);
	free(buf);
	poperror();
	return n;
}

static int
inserted(void *)
{
	if (slot[0].inserted)
		return 1;
	if (slot[1].inserted)
		return 2;
	return 0;
}

static long
pcmevsread(void *a, long n, ulong off)
{
	int i;
	char *buf = nil;
	char *e;

	if (pcmcia.evreader)
		error("At most one reader");
	off = 0;
	pcmcia.evreader++;
	if (waserror()){
		free(buf);
		pcmcia.evreader--;
		nexterror();
	}
	while((i = inserted(nil)) == 0){
		slotinfo(nil, nil);
		tsleep(&pcmcia.event, inserted, nil, 500);
	}
	pcmcia.evreader--;
	slot[i-1].inserted = 0;
	buf = malloc(READSTR);
	e = buf + READSTR;
	buf[0] = 0;
	seprint(buf, e, "#y/pcm%dctl\n", i-1);
	n = readstr(off, a, n, buf);
	free(buf);
	poperror();
	return n;
}

static long
pcmciaread(Chan *c, void *a, long n, vlong off)
{
	PCMslot *sp;
	ulong offset = off;

	sp = slotof(c);

	switch(TYPE(c)){
	case Qdir:
		return devdirread(c, a, n, 0, 0, pcmgen);
	case Qmem:
		if(!sp->occupied)
			error(Eio);
		return pcmread(a, n, offset, sp, sp->mem, 64*OneMeg);
	case Qattr:
		if(!sp->occupied)
			error(Eio);
		return pcmread(a, n, offset, sp, sp->attr, OneMeg);
	case Qevs:
		return pcmevsread(a, n, offset);
	case Qctl:
		return pcmctlread(a, n, offset, sp);
	}
	error(Ebadarg);
	return -1;	/* not reached */
}

static long
pcmwrite(void *a, long n, ulong off, PCMslot *sp, uchar *start, ulong len)
{
	rlock(sp);
	if(waserror()){
		runlock(sp);
		nexterror();
	}
	if(off > len)
		error(Eio);
	if(off + n > len)
		error(Eio);
	memmoveb(start+off, a, n);
	poperror();
	runlock(sp);
	return n;
}

static long
pcmctlwrite(char *p, long n, ulong, PCMslot *sp)
{
	Cmdbuf *cmd;
	uchar *cp;
	int index, i, dtx;
	Rune r;
	DevConf cf;
	Devport port;

	cmd = parsecmd(p, n);
	if(strcmp(cmd->f[0], "configure") == 0){
		wlock(sp);
		if(waserror()){
			wunlock(sp);
			nexterror();
		}

		/* see if driver exists and is configurable */
		if(cmd->nf < 3)
			error(Ebadarg);
		p = cmd->f[1];
		if(*p++ != '#')
			error(Ebadarg);
		p += chartorune(&r, p);
		dtx = devno(r, 1);
		if(dtx < 0)
			error("no such device type");
		if(devtab[dtx]->config == nil)
			error("not a dynamicly configurable device");

		/* set pcmcia card configuration */
		index = 0;
		if(sp->def != nil)
			index = sp->def->index;
		if(cmd->nf > 3){
			i = atoi(cmd->f[3]);
			if(i < 0 || i >= sp->nctab)
				error("bad configuration index");
			index = i;
		}
		if(sp->cfg[0].cpresent & (1<<Rconfig)){
			cp = sp->attr;
			cp += sp->cfg[0].caddr + Rconfig;
			*cp = index;
		}

		/* configure device */
		memset(&cf, 0, sizeof cf);
		kstrdup(&cf.type, cmd->f[2]);
		cf.mem = (ulong)sp->mem;
		cf.ports = &port;
		cf.ports[0].port = (ulong)sp->regs;
		cf.ports[0].size = 0;
		cf.nports = 1;
		cf.itype = GPIOfalling;
		cf.intnum = bitno(sp == slot ? GPIO_CARD_IRQ0_i : GPIO_CARD_IRQ1_i);
		if(devtab[dtx]->config(1, p, &cf) < 0)
			error("couldn't configure device");
		sp->dev = devtab[dtx];
		free(cf.type);
		wunlock(sp);
		poperror();

		/* don't let the power turn off */
		increfp(sp);
	}else if(strcmp(cmd->f[0], "remove") == 0){
		/* see if driver exists and is configurable */
		if(cmd->nf != 2)
			error(Ebadarg);
		p = cmd->f[1];
		if(*p++ != '#')
			error(Ebadarg);
		p += chartorune(&r, p);
		dtx = devno(r, 1);
		if(dtx < 0)
			error("no such device type");
		if(devtab[dtx]->config == nil)
			error("not a dynamicly configurable device");
		if(devtab[dtx]->config(0, p, nil) < 0)
			error("couldn't unconfigure device");

		/* let the power turn off */
		decrefp(sp);
	}
	free(cmd);

	return 0;
}

static long
pcmciawrite(Chan *c, void *a, long n, vlong off)
{
	PCMslot *sp;
	ulong offset = off;

	sp = slotof(c);

	switch(TYPE(c)){
	case Qmem:
		if(!sp->occupied)
			error(Eio);
		return pcmwrite(a, n, offset, sp, sp->mem, 64*OneMeg);
	case Qattr:
		if(!sp->occupied)
			error(Eio);
		return pcmwrite(a, n, offset, sp, sp->attr, OneMeg);
	case Qevs:
		break;
	case Qctl:
		if(!sp->occupied)
			error(Eio);
		return pcmctlwrite(a, n, offset, sp);
	}
	error(Ebadarg);
	return -1;	/* not reached */
}

/*
 * power up/down pcmcia
 */
void
pcmciapower(int on)
{
	PCMslot *sp;

	/* if there's no pcmcia sleave, no interrupts */
iprint("pcmciapower %d\n", on);

	if (on){
		/* set timing to the default, 300 */
		slottiming(0, 300, 300, 300, 0);
		slottiming(1, 300, 300, 300, 0);

		/* if there's no pcmcia sleave, no interrupts */
		if(gpioregs->level & GPIO_OPT_IND_i){
			iprint("pcmciapower: no sleeve\n");
			return;
		}

		for (sp = slot; sp < slot + nslot; sp++){
			if (sp->dev){
				increfp(sp);
				iprint("pcmciapower: %s\n", sp->verstr);
				delay(10000);
				if (sp->dev->power)
					sp->dev->power(on);
			}
		}
	}else{
		if(gpioregs->level & GPIO_OPT_IND_i){
			iprint("pcmciapower: no sleeve\n");
			return;
		}

		for (sp = slot; sp < slot + nslot; sp++){
			if (sp->dev){
				if (sp->dev->power)
					sp->dev->power(on);
				decrefp(sp);
			}
			sp->occupied = 0;
			sp->cisread = 0;
		}
		egpiobits(EGPIO_exp_nvram_power|EGPIO_exp_full_power, 0);
	}
}

Dev pcmciadevtab = {
	'y',
	"pcmcia",

	pcmciareset,
	devinit,
	devshutdown,
	pcmciaattach,
	pcmciawalk,
	pcmciastat,
	pcmciaopen,
	devcreate,
	pcmciaclose,
	pcmciaread,
	devbread,
	pcmciawrite,
	devbwrite,
	devremove,
	devwstat,
	pcmciapower,
};

/* see what's there */
static void
slotinfo(Ureg*, void*)
{
	ulong x = gpioregs->level;

	if(x & GPIO_OPT_IND_i){
		/* no expansion pack */
		slot[0].occupied = slot[0].inserted = 0;
		slot[1].occupied = slot[1].inserted = 0;
	} else {
		if(x & GPIO_CARD_IND0_i){
			slot[0].occupied = slot[0].inserted = 0;
			slot[0].cisread = 0;
		} else {
			if(slot[0].occupied == 0){
				slot[0].inserted = 1;
				slot[0].cisread = 0;
			}
			slot[0].occupied = 1;
		}
		if(x & GPIO_CARD_IND1_i){
			slot[1].occupied = slot[1].inserted = 0;
			slot[1].cisread = 0;
		} else {
			if(slot[1].occupied == 0){
				slot[1].inserted = 1;
				slot[1].cisread = 0;
			}
			slot[1].occupied = 1;
		}
		if (inserted(nil))
			wakeup(&pcmcia.event);
	}
}

/* use reference card to turn cards on and off */
static void
increfp(PCMslot *sp)
{
	wlock(sp);
	if(waserror()){
		wunlock(sp);
		nexterror();
	}

iprint("increfp %ld\n", sp - slot);
	if(incref(&pcmcia) == 1){
iprint("increfp full power\n");
		egpiobits(EGPIO_exp_nvram_power|EGPIO_exp_full_power, 1);
		delay(200);
		egpiobits(EGPIO_pcmcia_reset, 1);
		delay(100);
		egpiobits(EGPIO_pcmcia_reset, 0);
		delay(500);
	}
	incref(&sp->ref);
	slotinfo(nil, nil);
	if(sp->occupied && sp->cisread == 0) {
		pcmcisread(sp);
	}

	wunlock(sp);
	poperror();
}

static void
decrefp(PCMslot *sp)
{
iprint("decrefp %ld\n", sp - slot);
	decref(&sp->ref);
	if(decref(&pcmcia) == 0){
iprint("increfp power down\n");
		egpiobits(EGPIO_exp_nvram_power|EGPIO_exp_full_power, 0);
	}
}

/*
 *  the regions are staticly mapped
 */
static void
slotmap(int slotno, ulong regs, ulong attr, ulong mem)
{
	PCMslot *sp;

	sp = &slot[slotno];
	sp->slotno = slotno;
	sp->memlen = 64*OneMeg;
	sp->verstr[0] = 0;

	sp->mem = mapmem(mem, 64*OneMeg, 0);
	sp->memmap.ca = 0;
	sp->memmap.cea = 64*MB;
	sp->memmap.isa = (ulong)mem;
	sp->memmap.len = 64*OneMeg;
	sp->memmap.attr = 0;

	sp->attr = mapmem(attr, OneMeg, 0);
	sp->attrmap.ca = 0;
	sp->attrmap.cea = MB;
	sp->attrmap.isa = (ulong)attr;
	sp->attrmap.len = OneMeg;
	sp->attrmap.attr = 1;

	sp->regs = mapspecial(regs, 32*1024);
}

PCMmap*
pcmmap(int slotno, ulong, int, int attr)
{
	if(slotno > nslot)
		panic("pcmmap");
	if(attr)
		return &slot[slotno].attrmap;
	else
		return &slot[slotno].memmap;
}

void
pcmunmap(int, PCMmap*)
{
}

/*
 *  setup card timings
 *    times are in ns
 *    count = ceiling[access-time/(2*3*T)] - 1, where T is a processor cycle
 *
 */
static int
ns2count(int ns)
{
	ulong y;

	/* get 100 times cycle time */
	y = 100000000/(conf.hz/1000);

	/* get 10 times ns/(cycle*6) */
	y = (1000*ns)/(6*y);

	/* round up */
	y += 9;
	y /= 10;

	/* subtract 1 */
	return y-1;
}
static void
slottiming(int slotno, int tio, int tattr, int tmem, int fast)
{
	ulong x;

	x = 0;
	if(fast)
		x |= 1<<MECR_fast0;
	x |= ns2count(tio) << MECR_io0;
	x |= ns2count(tattr) << MECR_attr0;
	x |= ns2count(tmem) << MECR_mem0;
	if(slotno == 0){
		x |= memconfregs->mecr & 0xffff0000;
	} else {
		x <<= 16;
		x |= memconfregs->mecr & 0xffff;
	}
	memconfregs->mecr = x;
}

/* For compat with ../pc devices. Don't use it for the bitsy 
 */
int
pcmspecial(char*, ISAConf*)
{
	return -1;
}
