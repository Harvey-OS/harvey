/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

enum {
	Qdir		= 0,
	Qdata,
	Qctl,
	Qstat,
};

#define UARTTYPE(x)	(((unsigned)x)&0x1f)
#define UARTID(x)	((((unsigned)x))>>5)
#define UARTQID(i, t)	((((unsigned)i)<<5)|(t))

enum
{
	/* soft flow control chars */
	CTLS= 023,
	CTLQ= 021,
};

extern Dev uartdevtab;
extern PhysUart* physuart[];

static Uart* uartlist;
static Uart** uart;
static int uartnuart;
static Dirtab *uartdir;
static int uartndir;
static Timer *uarttimer;

struct Uartalloc {
	Lock Lock;
	Uart *elist;	/* list of enabled interfaces */
} uartalloc;

static void	uartclock(void);
static void	uartflow(void*);

/*
 *  enable/disable uart and add/remove to list of enabled uarts
 */
static Uart*
uartenable(Uart *p)
{
	Uart **l;

	if(p->iq == nil){
		if((p->iq = qopen(8*1024, 0, uartflow, p)) == nil)
			return nil;
	}
	else
		qreopen(p->iq);
	if(p->oq == nil){
		if((p->oq = qopen(8*1024, 0, uartkick, p)) == nil){
			qfree(p->iq);
			p->iq = nil;
			return nil;
		}
	}
	else
		qreopen(p->oq);

	p->ir = p->istage;
	p->iw = p->istage;
	p->ie = &p->istage[Stagesize];
	p->op = p->ostage;
	p->oe = p->ostage;

	p->hup_dsr = p->hup_dcd = 0;
	p->dsr = p->dcd = 0;

	/* assume we can send */
	p->cts = 1;
	p->ctsbackoff = 0;

	if(p->bits == 0)
		uartctl(p, "l8");
	if(p->stop == 0)
		uartctl(p, "s1");
	if(p->parity == 0)
		uartctl(p, "pn");
	if(p->baud == 0)
		uartctl(p, "b9600");
	(*p->phys->enable)(p, 1);

	lock(&uartalloc.Lock);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p)
			break;
	}
	if(*l == 0){
		p->elist = uartalloc.elist;
		uartalloc.elist = p;
	}
	p->enabled = 1;
	unlock(&uartalloc.Lock);

	return p;
}

static void
uartdisable(Uart *p)
{
	Uart **l;

	(*p->phys->disable)(p);

	lock(&uartalloc.Lock);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p){
			*l = p->elist;
			break;
		}
	}
	p->enabled = 0;
	unlock(&uartalloc.Lock);
}

static void
uartsetlength(int i)
{
	Uart *p;

	if(i > 0){
		p = uart[i];
		if(p && p->opens && p->iq)
			uartdir[1+3*i].length = qlen(p->iq);
	} else for(i = 0; i < uartnuart; i++){
		p = uart[i];
		if(p && p->opens && p->iq)
			uartdir[1+3*i].length = qlen(p->iq);
	}
}

/*
 *  set up the '#t' directory
 */
static void
uartreset(void)
{
	int i;
	Dirtab *dp;
	Uart *p, *tail;

	tail = nil;
	for(i = 0; physuart[i] != nil; i++){
		if(physuart[i]->pnp == nil)
			continue;
		if((p = physuart[i]->pnp()) == nil)
			continue;
		if(uartlist != nil)
			tail->next = p;
		else
			uartlist = p;
		for(tail = p; tail->next != nil; tail = tail->next)
			uartnuart++;
		uartnuart++;
	}

	if(uartnuart)
		uart = malloc(uartnuart*sizeof(Uart*));

	uartndir = 1 + 3*uartnuart;
	uartdir = malloc(uartndir * sizeof(Dirtab));
	if(uartnuart > 0 && uart == nil || uartdir == nil)
		panic("uartreset: no memory");
	dp = uartdir;
	strcpy(dp->name, ".");
	mkqid(&dp->qid, 0, 0, QTDIR);
	dp->length = 0;
	dp->perm = DMDIR|0555;
	dp++;
	p = uartlist;
	for(i = 0; i < uartnuart; i++){
		/* 3 directory entries per port */
		sprint(dp->name, "eia%d", i);
		dp->qid.path = UARTQID(i, Qdata);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "eia%dctl", i);
		dp->qid.path = UARTQID(i, Qctl);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "eia%dstatus", i);
		dp->qid.path = UARTQID(i, Qstat);
		dp->perm = 0444;
		dp++;

		uart[i] = p;
		p->dev = i;
		p = p->next;
	}

	if(uartnuart){
		/*
		 * at 115200 baud, the 1024 char buffer takes 56 ms to process,
		 * processing it every 22 ms should be fine.
		 */
		uarttimer = addclock0link(uartclock, 22);
	}
}


static Chan*
uartattach(char *spec)
{
	return devattach('t', spec);
}

static Walkqid*
uartwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, uartdir, uartndir, devgen);
}

static int32_t
uartstat(Chan *c, uint8_t *dp, int32_t n)
{
	if(UARTTYPE(c->qid.path) == Qdata)
		uartsetlength(UARTID(c->qid.path));
	return devstat(c, dp, n, uartdir, uartndir, devgen);
}

static Chan*
uartopen(Chan *c, int omode)
{
	Uart *p;

	c = devopen(c, omode, uartdir, uartndir, devgen);

	switch(UARTTYPE(c->qid.path)){
	case Qctl:
	case Qdata:
		p = uart[UARTID(c->qid.path)];
		qlock(&p->ql);
		if(p->opens == 0 && uartenable(p) == nil){
			qunlock(&p->ql);
			c->flag &= ~COPEN;
			error(Enodev);
		}
		p->opens++;
		qunlock(&p->ql);
		break;
	}

	c->iounit = qiomaxatomic;
	return c;
}

static int
uartdrained(void* arg)
{
	Uart *p;

	p = arg;
	return qlen(p->oq) == 0 && p->op == p->oe;
}

static void
uartdrainoutput(Uart *p)
{
	Proc *up = externup();
	if(!p->enabled)
		return;

	p->drain = 1;
	if(waserror()){
		p->drain = 0;
		nexterror();
	}
	sleep(&p->rend, uartdrained, p);
	poperror();
}

static void
uartclose(Chan *c)
{
	Proc *up = externup();
	Uart *p;

	if(c->qid.type & QTDIR)
		return;
	if((c->flag & COPEN) == 0)
		return;
	switch(UARTTYPE(c->qid.path)){
	case Qdata:
	case Qctl:
		p = uart[UARTID(c->qid.path)];
		qlock(&p->ql);
		if(--(p->opens) == 0){
			qclose(p->iq);
			ilock(&p->rlock);
			p->ir = p->iw = p->istage;
			iunlock(&p->rlock);

			/*
			 */
			qhangup(p->oq, nil);
			if(!waserror()){
				uartdrainoutput(p);
				poperror();
			}
			qclose(p->oq);
			uartdisable(p);
			p->dcd = p->dsr = p->dohup = 0;
		}
		qunlock(&p->ql);
		break;
	}
}

static int32_t
uartread(Chan *c, void *buf, int32_t n, int64_t off)
{
	Uart *p;
	uint32_t offset = off;

	if(c->qid.type & QTDIR){
		uartsetlength(-1);
		return devdirread(c, buf, n, uartdir, uartndir, devgen);
	}

	p = uart[UARTID(c->qid.path)];
	switch(UARTTYPE(c->qid.path)){
	case Qdata:
		return qread(p->iq, buf, n);
	case Qctl:
		return readnum(offset, buf, n, UARTID(c->qid.path), NUMSIZE);
	case Qstat:
		return (*p->phys->status)(p, buf, n, offset);
	}

	return 0;
}

int
uartctl(Uart *p, char *cmd)
{
	char *f[16];
	int i, n, nf;

	nf = tokenize(cmd, f, nelem(f));
	for(i = 0; i < nf; i++){
		if(strncmp(f[i], "break", 5) == 0){
			(*p->phys->dobreak)(p, 0);
			continue;
		}

		n = atoi(f[i]+1);
		switch(*f[i]){
		case 'B':
		case 'b':
			uartdrainoutput(p);
			if((*p->phys->baud)(p, n) < 0)
				return -1;
			break;
		case 'C':
		case 'c':
			p->hup_dcd = n;
			break;
		case 'D':
		case 'd':
			uartdrainoutput(p);
			(*p->phys->dtr)(p, n);
			break;
		case 'E':
		case 'e':
			p->hup_dsr = n;
			break;
		case 'F':
		case 'f':
			if(p->oq != nil)
				qflush(p->oq);
			break;
		case 'H':
		case 'h':
			if(p->iq != nil)
				qhangup(p->iq, 0);
			if(p->oq != nil)
				qhangup(p->oq, 0);
			break;
		case 'I':
		case 'i':
			uartdrainoutput(p);
			(*p->phys->fifo)(p, n);
			break;
		case 'K':
		case 'k':
			uartdrainoutput(p);
			(*p->phys->dobreak)(p, n);
			break;
		case 'L':
		case 'l':
			uartdrainoutput(p);
			if((*p->phys->bits)(p, n) < 0)
				return -1;
			break;
		case 'M':
		case 'm':
			uartdrainoutput(p);
			(*p->phys->modemctl)(p, n);
			break;
		case 'N':
		case 'n':
			if(p->oq != nil)
				qnoblock(p->oq, n);
			break;
		case 'P':
		case 'p':
			uartdrainoutput(p);
			if((*p->phys->parity)(p, *(f[i]+1)) < 0)
				return -1;
			break;
		case 'Q':
		case 'q':
			if(p->iq != nil)
				qsetlimit(p->iq, n);
			if(p->oq != nil)
				qsetlimit(p->oq, n);
			break;
		case 'R':
		case 'r':
			uartdrainoutput(p);
			(*p->phys->rts)(p, n);
			break;
		case 'S':
		case 's':
			uartdrainoutput(p);
			if((*p->phys->stop)(p, n) < 0)
				return -1;
			break;
		case 'W':
		case 'w':
			if(uarttimer == nil || n < 1)
				return -1;
			uarttimer->tns = (int64_t)n * 100000LL;
			break;
		case 'X':
		case 'x':
			if(p->enabled){
				ilock(&p->tlock);
				p->xonoff = n;
				iunlock(&p->tlock);
			}
			break;
		}
	}
	return 0;
}

static int32_t
uartwrite(Chan *c, void *buf, int32_t n, int64_t mm)
{
	Proc *up = externup();
	Uart *p;
	char *cmd;

	if(c->qid.type & QTDIR)
		error(Eperm);

	p = uart[UARTID(c->qid.path)];

	switch(UARTTYPE(c->qid.path)){
	case Qdata:
		qlock(&p->ql);
		if(waserror()){
			qunlock(&p->ql);
			nexterror();
		}

		n = qwrite(p->oq, buf, n);

		qunlock(&p->ql);
		poperror();
		break;
	case Qctl:
		cmd = malloc(n+1);
		memmove(cmd, buf, n);
		cmd[n] = 0;
		qlock(&p->ql);
		if(waserror()){
			qunlock(&p->ql);
			free(cmd);
			nexterror();
		}

		/* let output drain */
		if(uartctl(p, cmd) < 0)
			error(Ebadarg);

		qunlock(&p->ql);
		poperror();
		free(cmd);
		break;
	}

	return n;
}

static int32_t
uartwstat(Chan *c, uint8_t *dp, int32_t n)
{
	Dir d;
	Dirtab *dt;

	if(!iseve())
		error(Eperm);
	if(QTDIR & c->qid.type)
		error(Eperm);
	if(UARTTYPE(c->qid.path) == Qstat)
		error(Eperm);

	dt = &uartdir[1 + 3 * UARTID(c->qid.path)];
	n = convM2D(dp, n, &d, nil);
	if(n == 0)
		error(Eshortstat);
	if(d.mode != (uint32_t)~0UL)
		dt[0].perm = dt[1].perm = d.mode;
	return n;
}

void
uartpower(int on)
{
	Uart *p;

	for(p = uartlist; p != nil; p = p->next) {
		if(p->phys->power)
			(*p->phys->power)(p, on);
	}
}

Dev uartdevtab = {
	.dc = 't',
	.name = "uart",

	.reset = uartreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = uartattach,
	.walk = uartwalk,
	.stat = uartstat,
	.open = uartopen,
	.create = devcreate,
	.close = uartclose,
	.read = uartread,
	.bread = devbread,
	.write = uartwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = uartwstat,
	.power = uartpower,
};

/*
 *  restart input if it's off
 */
static void
uartflow(void *v)
{
	Uart *p;

	p = v;
	if(p->modem)
		(*p->phys->rts)(p, 1);
}

/*
 *  put some bytes into the local queue to avoid calling
 *  qconsume for every character
 */
int
uartstageoutput(Uart *p)
{
	int n;

	n = qconsume(p->oq, p->ostage, Stagesize);
	if(n <= 0)
		return 0;
	p->op = p->ostage;
	p->oe = p->ostage + n;
	return n;
}

/*
 *  restart output
 */
void
uartkick(void *v)
{
	Uart *p = v;

	if(p->blocked)
		return;

	ilock(&p->tlock);
	(*p->phys->kick)(p);
	iunlock(&p->tlock);

	if(p->drain && uartdrained(p)){
		p->drain = 0;
		wakeup(&p->rend);
	}
}

/*
 * Move data from the interrupt staging area to
 * the input Queue.
 */
static void
uartstageinput(Uart *p)
{
	int n;
	uint8_t *ir, *iw;

	while(p->ir != p->iw){
		ir = p->ir;
		if(p->ir > p->iw){
			iw = p->ie;
			p->ir = p->istage;
		}
		else{
			iw = p->iw;
			p->ir = p->iw;
		}
		if((n = qproduce(p->iq, ir, iw - ir)) < 0){
			p->serr++;
			(*p->phys->rts)(p, 0);
		}
		else if(n == 0)
			p->berr++;
	}
}

/*
 *  receive a character at interrupt time
 */
void
uartrecv(Uart *p,  char ch)
{
	uint8_t *next;

	/* software flow control */
	if(p->xonoff){
		if(ch == CTLS){
			p->blocked = 1;
		}else if(ch == CTLQ){
			p->blocked = 0;
			p->ctsbackoff = 2; /* clock gets output going again */
		}
	}

	/* receive the character */
	if(p->putc)
		p->putc(p->iq, ch);
	else{
		ilock(&p->rlock);
		next = p->iw + 1;
		if(next == p->ie)
			next = p->istage;
		if(next == p->ir)
			uartstageinput(p);
		if(next != p->ir){
			*p->iw = ch;
			p->iw = next;
		}
		iunlock(&p->rlock);
	}
}

/*
 *  we save up input characters till clock time to reduce
 *  per character interrupt overhead.
 */
static void
uartclock(void)
{
	Uart *p;

	lock(&uartalloc.Lock);
	for(p = uartalloc.elist; p; p = p->elist){

		if(p->phys->poll != nil)
			(*p->phys->poll)(p);

		/* this hopefully amortizes cost of qproduce to many chars */
		if(p->iw != p->ir){
			ilock(&p->rlock);
			uartstageinput(p);
			iunlock(&p->rlock);
		}

		/* hang up if requested */
		if(p->dohup){
			qhangup(p->iq, 0);
			qhangup(p->oq, 0);
			p->dohup = 0;
		}

		/* this adds hysteresis to hardware/software flow control */
		if(p->ctsbackoff){
			ilock(&p->tlock);
			if(p->ctsbackoff){
				if(--(p->ctsbackoff) == 0)
					(*p->phys->kick)(p);
			}
			iunlock(&p->tlock);
		}
	}
	unlock(&uartalloc.Lock);
}

/*
 * polling console input, output
 */

Uart* consuart;

int
uartgetc(void)
{
	if(consuart == nil || consuart->phys->getc == nil)
		return -1;
	return consuart->phys->getc(consuart);
}

void
uartputc(int c)
{
	if(consuart == nil || consuart->phys->putc == nil)
		return;
	consuart->phys->putc(consuart, c);
}

void
uartputs(char *s, int n)
{
	char *e;

	if(consuart == nil || consuart->phys->putc == nil)
		return;

	e = s+n;
	for(; s<e; s++){
		if(*s == '\n')
			consuart->phys->putc(consuart, '\r');
		consuart->phys->putc(consuart, *s);
	}
}
