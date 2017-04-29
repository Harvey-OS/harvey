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
#include	"../port/error.h"

#include	<authsrv.h>

enum
{
	Nconsdevs	= 64,		/* max number of consoles */

	/* Consdev flags */
	Ciprint		= 2,		/* call this fn from iprint */
	Cntorn		= 4,		/* change \n to \r\n */
};

typedef struct Consdev Consdev;

struct Consdev
{
	Chan*	c;			/* external file */
	Queue*	q;			/* i/o queue, if any */
	void	(*fn)(char*, int);	/* i/o function when no queue */
	int	flags;
};

extern int printallsyscalls;

void	(*consdebug)(void) = nil;
void	(*consputs)(char*, int) = nil;
void	(*consuartputs)(char*, int) = nil;

static void kmesgputs(char *, int);

static	Lock	consdevslock;
static	int	nconsdevs = 1;
static	Consdev	consdevs[Nconsdevs] =			/* keep this order */
{
	{nil, nil,	kmesgputs,	0},			/* kmesg */
};

static	int	nkeybqs;
static	int	nkeybprocs;
static	Queue*	keybqs[Nconsdevs];
static	int	keybprocs[Nconsdevs];
static	Queue*	keybq;		/* unprocessed console input */
static	Queue*	lineq;		/* processed console input */

int	panicking;

static struct
{
	QLock QLock;

	int	raw;		/* true if we shouldn't process input */
	Ref	ctl;		/* number of opens to the control file */
	int	x;		/* index into line */
	char	line[1024];	/* current input line */

	int	count;
	int	ctlpoff;

	/* a place to save up characters at interrupt time before dumping them in the queue */
	Lock	lockputc;
	char	istage[1024];
	char	*iw;
	char	*ir;
	char	*ie;
} kbd = {
	.iw	= kbd.istage,
	.ir	= kbd.istage,
	.ie	= kbd.istage + sizeof(kbd.istage),
};

char	*sysname;
int64_t	fasthz;

static void	seedrand(void);
static int	readtime(uint32_t, char*, int);
static int	readbintime(char*, int);
static int	writetime(char*, int);
static int	writebintime(char*, int);

enum
{
	CMhalt,
	CMreboot,
	CMpanic,
};

Cmdtab rebootmsg[] =
{
	CMhalt,		"halt",		1,
	CMreboot,	"reboot",	0,
	CMpanic,	"panic",	0,
};

int
addconsdev(Queue *q, void (*fn)(char*,int), int i, int flags)
{
	Consdev *c;

	ilock(&consdevslock);
	if(i < 0)
		i = nconsdevs;
	else
		flags |= consdevs[i].flags;
	if(nconsdevs == Nconsdevs)
		panic("Nconsdevs too small");
	c = &consdevs[i];
	c->flags = flags;
	c->q = q;
	c->fn = fn;
	if(i == nconsdevs)
		nconsdevs++;
	iunlock(&consdevslock);
	return i;
}

void
delconsdevs(void)
{
	nconsdevs = 2;	/* throw away serial consoles and kprint */
	consdevs[1].q = nil;
}

static void
conskbdqproc(void *a)
{
	char buf[64];
	Queue *q;
	int nr;

	q = a;
	while((nr = qread(q, buf, sizeof(buf))) > 0)
		qwrite(keybq, buf, nr);
	pexit("hangup", 1);
}

static void
kickkbdq(void)
{
	Proc *up = externup();
	int i;

	if(up != nil && nkeybqs > 1 && nkeybprocs != nkeybqs){
		lock(&consdevslock);
		if(nkeybprocs == nkeybqs){
			unlock(&consdevslock);
			return;
		}
		for(i = 0; i < nkeybqs; i++)
			if(keybprocs[i] == 0){
				keybprocs[i] = 1;
				kproc("conskbdq", conskbdqproc, keybqs[i]);
			}
		unlock(&consdevslock);
	}
}

int
addkbdq(Queue *q, int i)
{
	int n;

	ilock(&consdevslock);
	if(i < 0)
		i = nkeybqs++;
	if(nkeybqs == Nconsdevs)
		panic("Nconsdevs too small");
	keybqs[i] = q;
	n = nkeybqs;
	iunlock(&consdevslock);
	switch(n){
	case 1:
		/* if there's just one, pull directly from it. */
		keybq = q;
		break;
	case 2:
		/* later we'll merge bytes from all keybqs into a single keybq */
		keybq = qopen(4*1024, 0, 0, 0);
		if(keybq == nil)
			panic("no keybq");
		/* fall */
	default:
		kickkbdq();
	}
	return i;
}

void
printinit(void)
{
	lineq = qopen(2*1024, 0, nil, nil);
	if(lineq == nil)
		panic("printinit");
	qnoblock(lineq, 1);
}

int
consactive(void)
{
	int i;
	Queue *q;

	for(i = 0; i < nconsdevs; i++)
		if((q = consdevs[i].q) != nil && qlen(q) > 0)
			return 1;
	return 0;
}

/*
 * Log console output so it can be retrieved via /dev/kmesg.
 * This is good for catching boot-time messages after the fact.
 */
struct {
	Lock lk;
	char buf[1048576];
	uint n;
} kmesg;

static void
kmesgputs(char *str, int n)
{
	uint nn, d;

	ilock(&kmesg.lk);
	/* take the tail of huge writes */
	if(n > sizeof kmesg.buf){
		d = n - sizeof kmesg.buf;
		str += d;
		n -= d;
	}

	/* slide the buffer down to make room */
	nn = kmesg.n;
	if(nn + n >= sizeof kmesg.buf){
		d = nn + n - sizeof kmesg.buf;
		if(d)
			memmove(kmesg.buf, kmesg.buf+d, sizeof kmesg.buf-d);
		nn -= d;
	}

	/* copy the data in */
	memmove(kmesg.buf+nn, str, n);
	nn += n;
	kmesg.n = nn;
	iunlock(&kmesg.lk);
}

static
void printcontrolt(int c)
{
	int x;
	/* ^T escapes */
	print("^T^T%c\n", c);
	switch(c){
	case 'S':
		x = splhi();
		dumpstack();
		procdump();
		splx(x);
		break;
	case 's':
		dumpstack();
		break;
	case 'x':
		//xsummary();
		ixsummary();
		mallocsummary();
		//	memorysummary();
		pagersummary();
		break;
	case 'p':
		x = spllo();
		procdump();
		splx(x);
		break;
	case 'q':
		scheddump();
		break;
	case 'k':
		killbig("^t ^t k");
		break;
	case 'r':
		exit(0);
		break;
	}
}

void catchcontrolt(int c)
{
	static int ctrlt;

	if(ctrlt == 2) {
		ctrlt = 0;
		printcontrolt(c);
	}

	switch(c){
	case 0x10:	/* ^P */
		active.exiting = 1;
		break;
	case 0x14:	/* ^T */
		ctrlt++;
		if(ctrlt > 2)
			ctrlt = 2;
		break;
	}
}

/*
 *   Print a string on the console.  Convert \n to \r\n for serial
 *   line consoles.  Locking of the queues is left up to the screen
 *   or uart code.  Multi-line messages to serial consoles may get
 *   interspersed with other messages.
 */

static void
putstrn0(char *str, int n, int usewrite)
{
	if(n == 0)
		return;

	kmesgputs(str, n);
	if(consputs != nil)
		consputs(str, n);
	if(consuartputs != nil)
		consuartputs(str, n);
}

void
putstrn(char *str, int n)
{
	putstrn0(str, n, 0);
}

int
print(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	putstrn(buf, n);

	return n;
}

int
kmprint(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	kmesgputs(buf, n);

	return n;
}

/*
 * Want to interlock iprints to avoid interlaced output on
 * multiprocessor, but don't want to deadlock if one processor
 * dies during print and another has something important to say.
 * Make a good faith effort.
 */
static Lock iprintlock;

static int
iprintcanlock(Lock *l)
{
	int i;

	for(i=0; i<1000; i++){
		if(canlock(l))
			return 1;
		if(l->m == machp())
			return 0;
		microdelay(100);
	}
	return 0;
}

int
iprint(char *fmt, ...)
{
	Mpl pl;
	int i, n, locked;
	va_list arg;
	char buf[PRINTSIZE];

	pl = splhi();
	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	locked = iprintcanlock(&iprintlock);
	for(i = 0; i < nconsdevs; i++)
		if((consdevs[i].flags&Ciprint) != 0){
			if(consdevs[i].q != nil)
				qiwrite(consdevs[i].q, buf, n);
			else
				consdevs[i].fn(buf, n);
		}
	if(locked)
		unlock(&iprintlock);
	splx(pl);

	return n;
}

#pragma profile 0
void
panic(char *fmt, ...)
{
	int n;
	Mpl pl;
	va_list arg;
	char buf[PRINTSIZE];

	consdevs[1].q = nil;	/* don't try to write to /dev/kprint */

	if(panicking)
		for(;;);
	panicking = 1;

	pl = splhi();
	seprint(buf, buf+sizeof buf, "panic: cpu%d: ", machp()->machno);
	va_start(arg, fmt);
	n = vseprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	iprint("%s\n", buf);
	if(consdebug)
		(*consdebug)();
	splx(pl);
	//prflush();
	buf[n] = '\n';
	putstrn(buf, n+1);
	//dumpstack();
	delay(1000);	/* give time to consoles */
	die("wait forever");
	exit(1);
}
#pragma profile 1
/* libmp at least contains a few calls to sysfatal; simulate with panic */
void
sysfatal(char *fmt, ...)
{
	char err[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(err, err + sizeof err, fmt, arg);
	va_end(arg);
	panic("sysfatal: %s", err);
}

void
_assert(char *fmt)
{
	panic("assert failed at %#p: %s", getcallerpc(), fmt);
}

int
pprint(char *fmt, ...)
{
	Proc *up = externup();
	int n;
	Chan *c;
	va_list arg;
	char buf[2*PRINTSIZE];

	if(up == nil || up->fgrp == nil)
		return 0;

	c = up->fgrp->fd[2];
	if(c==0 || (c->mode!=OWRITE && c->mode!=ORDWR))
		return 0;
	n = snprint(buf, sizeof buf, "%s %d: ", up->text, up->pid);
	va_start(arg, fmt);
	n = vseprint(buf+n, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(waserror())
		return 0;
	c->dev->write(c, buf, n, c->offset);
	poperror();

	lock(&c->r.l);
	c->offset += n;
	unlock(&c->r.l);

	return n;
}

static void
echo(char *buf, int n)
{
	Mpl pl;
	static int ctrlt;
	char *e, *p;

	if(n == 0)
		return;

	e = buf+n;
	for(p = buf; p < e; p++){
		switch(*p){
		case 0x10:	/* ^P */
			if(cpuserver && !kbd.ctlpoff){
				active.exiting = 1;
				return;
			}
			break;
		case 0x14:	/* ^T */
			ctrlt++;
			if(ctrlt > 2)
				ctrlt = 2;
			continue;
		}

		if(ctrlt != 2)
			continue;

		/* ^T escapes */
		ctrlt = 0;
		switch(*p){
		case 'S':
			pl = splhi();
			dumpstack();
			procdump();
			splx(pl);
			return;
		case 's':
			dumpstack();
			return;
		case 'x':
			ixsummary();
			mallocsummary();
//			memorysummary();
			pagersummary();
			return;
/*		case 'd':
			if(consdebug == nil)
				consdebug = rdb;
			else
				consdebug = nil;
			print("consdebug now %#p\n", consdebug);
			return;
		case 'D':
			if(consdebug == nil)
				consdebug = rdb;
			consdebug();
			return;*/
		case 'p':
			pl = spllo();
			procdump();
			splx(pl);
			return;
		case 'q':
			scheddump();
			return;
		case 'k':
			killbig("^t ^t k");
			return;
		case 'r':
			exit(0);
			return;
		}
	}

	if(keybq != nil)
		qproduce(keybq, buf, n);
	if(kbd.raw == 0)
		putstrn(buf, n);
}

/*
 *  Called by a uart interrupt for console input.
 *
 *  turn '\r' into '\n' before putting it into the queue.
 */
int
kbdcr2nl(Queue* q, int ch)
{
	char *next;

	ilock(&kbd.lockputc);		/* just a mutex */
	if(ch == '\r' && !kbd.raw)
		ch = '\n';
	next = kbd.iw+1;
	if(next >= kbd.ie)
		next = kbd.istage;
	if(next != kbd.ir){
		*kbd.iw = ch;
		kbd.iw = next;
	}
	iunlock(&kbd.lockputc);
	return 0;
}

/*
 *  Put character, possibly a rune, into read queue at interrupt time.
 *  Called at interrupt time to process a character.
 */
int
kbdputc(Queue *q, int ch)
{
	int i, n;
	char buf[3];
	Rune r;
	char *next;

	if(kbd.ir == nil)
		return 0;		/* in case we're not inited yet */

	ilock(&kbd.lockputc);		/* just a mutex */
	r = ch;
	n = runetochar(buf, &r);
	for(i = 0; i < n; i++){
		next = kbd.iw+1;
		if(next >= kbd.ie)
			next = kbd.istage;
		if(next == kbd.ir)
			break;
		*kbd.iw = buf[i];
		kbd.iw = next;
	}
	iunlock(&kbd.lockputc);
	return 0;
}

/*
 *  we save up input characters till clock time to reduce
 *  per character interrupt overhead.
 */
static void
kbdputcclock(void)
{
	char *iw;

	/* this amortizes cost of qproduce */
	if(kbd.iw != kbd.ir){
		iw = kbd.iw;
		if(iw < kbd.ir){
			echo(kbd.ir, kbd.ie-kbd.ir);
			kbd.ir = kbd.istage;
		}
		if(kbd.ir != iw){
			echo(kbd.ir, iw-kbd.ir);
			kbd.ir = iw;
		}
	}
}

enum{
	Qdir,
	Qbintime,
	Qcons,
	Qconsctl,
	Qcputime,
	Qdrivers,
	Qkmesg,
	Qkprint,
	Qhostdomain,
	Qhostowner,
	Qnull,
	Qosversion,
	Qpgrpid,
	Qpid,
	Qppid,
	Qrandom,
	Qurandom,
	Qreboot,
	Qswap,
	Qsysname,
	Qsysstat,
	Qtime,
	Quser,
	Qzero,
	Qsyscall,
	Qdebug,
};

enum
{
	VLNUMSIZE=	22,
};

static Dirtab consdir[]={
	".",	{Qdir, 0, QTDIR},	0,		DMDIR|0555,
	"bintime",	{Qbintime},	24,		0664,
	"cons",		{Qcons},	0,		0660,
	"consctl",	{Qconsctl},	0,		0220,
	"cputime",	{Qcputime},	6*NUMSIZE,	0444,
	"drivers",	{Qdrivers},	0,		0444,
	"hostdomain",	{Qhostdomain},	DOMLEN,		0664,
	"hostowner",	{Qhostowner},	0,		0664,
	"kmesg",	{Qkmesg},	0,		0440,
	"kprint",	{Qkprint, 0, QTEXCL},	0,	DMEXCL|0440,
	"null",		{Qnull},	0,		0666,
	"osversion",	{Qosversion},	0,		0444,
	"pgrpid",	{Qpgrpid},	NUMSIZE,	0444,
	"pid",		{Qpid},		NUMSIZE,	0444,
	"ppid",		{Qppid},	NUMSIZE,	0444,
	"random",	{Qrandom},	0,		0444,
	"urandom",  {Qurandom}, 0,      0444,
	"reboot",	{Qreboot},	0,		0664,
	"swap",		{Qswap},	0,		0664,
	"sysname",	{Qsysname},	0,		0664,
	"sysstat",	{Qsysstat},	0,		0666,
	"time",		{Qtime},	NUMSIZE+3*VLNUMSIZE,	0664,
	"user",		{Quser},	0,		0666,
	"zero",		{Qzero},	0,		0444,
	"syscall",	{Qsyscall},	0,		0666,
	"debug",	{Qdebug},	0,		0666,
};

int
readnum(uint32_t off, char *buf, uint32_t n, uint32_t val, int size)
{
	char tmp[64];

	snprint(tmp, sizeof(tmp), "%*lu", size-1, val);
	tmp[size-1] = ' ';
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, tmp+off, n);
	return n;
}

int32_t
readmem(int32_t offset, void *buf, int32_t n, void *v, int32_t size)
{
	if(offset >= size)
		return 0;
	if(offset+n > size)
		n = size-offset;
	memmove(buf, v+offset, n);
	return n;
}

int32_t
readstr(int32_t offset, char *buf, int32_t n, char *str)
{
	int32_t size;

	size = strlen(str);
	if(offset >= size)
		return 0;
	if(offset+n > size)
		n = size-offset;
	memmove(buf, str+offset, n);
	return n;
}

static void
consinit(void)
{
	todinit();
	/*
	 * at 115200 baud, the 1024 char buffer takes 56 ms to process,
	 * processing it every 22 ms should be fine
	 */
	addclock0link(kbdputcclock, 22);
	kickkbdq();
}

static Chan*
consattach(char *spec)
{
	return devattach('c', spec);
}

static Walkqid*
conswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name,nname, consdir, nelem(consdir), devgen);
}

static int32_t
consstat(Chan *c, uint8_t *dp, int32_t n)
{
	return devstat(c, dp, n, consdir, nelem(consdir), devgen);
}

static Chan*
consopen(Chan *c, int omode)
{
	c->aux = nil;
	c = devopen(c, omode, consdir, nelem(consdir), devgen);
	switch((uint32_t)c->qid.path){
	case Qconsctl:
		incref(&kbd.ctl);
		break;

	case Qkprint:
		error(Egreg);
	}
	return c;
}

static void
consclose(Chan *c)
{
	switch((uint32_t)c->qid.path){
	/* last close of control file turns off raw */
	case Qconsctl:
		if(c->flag&COPEN){
			if(decref(&kbd.ctl) == 0)
				kbd.raw = 0;
		}
		break;

	/* close of kprint allows other opens */
	case Qkprint:
		error(Egreg);
	}
}

static int32_t
consread(Chan *c, void *buf, int32_t n, int64_t off)
{
	Proc *up = externup();
	uint64_t l;
	Mach *mp;
	char *b, *bp, ch, *s, *e;
	char tmp[512];		/* Qswap is 381 bytes at clu */
	int i, k, id, send;
	int32_t offset, nread;


	if(n <= 0)
		return n;

	nread = n;
	offset = off;
	switch((uint32_t)c->qid.path){
	case Qdir:
		return devdirread(c, buf, n, consdir, nelem(consdir), devgen);

	case Qcons:
		qlock(&kbd.QLock);
		if(waserror()) {
			qunlock(&kbd.QLock);
			nexterror();
		}
		while(!qcanread(lineq)){
			if(qread(keybq, &ch, 1) == 0)
				continue;
			send = 0;
			if(ch == 0){
				/* flush output on rawoff -> rawon */
				if(kbd.x > 0)
					send = !qcanread(keybq);
			}else if(kbd.raw){
				kbd.line[kbd.x++] = ch;
				send = !qcanread(keybq);
			}else{
				switch(ch){
				case '\b':
					if(kbd.x > 0)
						kbd.x--;
					break;
				case 0x15:	/* ^U */
					kbd.x = 0;
					break;
				case '\n':
				case 0x04:	/* ^D */
					send = 1;
				default:
					if(ch != 0x04)
						kbd.line[kbd.x++] = ch;
					break;
				}
			}
			if(send || kbd.x == sizeof kbd.line){
				qwrite(lineq, kbd.line, kbd.x);
				kbd.x = 0;
			}
		}
		n = qread(lineq, buf, n);
		qunlock(&kbd.QLock);
		poperror();
		return n;

	case Qcputime:
		k = offset;
		if(k >= 6*NUMSIZE)
			return 0;
		if(k+n > 6*NUMSIZE)
			n = 6*NUMSIZE - k;
		/* easiest to format in a separate buffer and copy out */
		for(i=0; i<6 && NUMSIZE*i<k+n; i++){
			l = up->time[i];
			if(i == TReal)
				l = sys->ticks - l;
			l = TK2MS(l);
			readnum(0, tmp+NUMSIZE*i, NUMSIZE, l, NUMSIZE);
		}
		memmove(buf, tmp+k, n);
		return n;

	case Qkmesg:
		/*
		 * This is unlocked to avoid tying up a process
		 * that's writing to the buffer.  kmesg.n never
		 * gets smaller, so worst case the reader will
		 * see a slurred buffer.
		 */
		if(off >= kmesg.n)
			n = 0;
		else{
			if(off+n > kmesg.n)
				n = kmesg.n - off;
			memmove(buf, kmesg.buf+off, n);
		}
		return n;

	case Qkprint:
		error(Egreg);

	case Qpgrpid:
		return readnum(offset, buf, n, up->pgrp->pgrpid, NUMSIZE);

	case Qpid:
		return readnum(offset, buf, n, up->pid, NUMSIZE);

	case Qppid:
		return readnum(offset, buf, n, up->parentpid, NUMSIZE);

	case Qtime:
		return readtime(offset, buf, n);

	case Qbintime:
		return readbintime(buf, n);

	case Qhostowner:
		return readstr(offset, buf, n, eve);

	case Qhostdomain:
		return readstr(offset, buf, n, hostdomain);

	case Quser:
		return readstr(offset, buf, n, up->user);

	case Qnull:
		return 0;

	case Qsysstat:
		n = MACHMAX*(NUMSIZE*11+2+1);
		b = smalloc(n + 1);	/* +1 for NUL */
		bp = b;
		e = bp + n;
		for(id = 0; id < MACHMAX; id++)
			if((mp = sys->machptr[id]) != nil && mp->online){
				readnum(0, bp, NUMSIZE, mp->machno, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, mp->cs, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, mp->intr, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, mp->syscall, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, mp->pfault, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, mp->tlbfault, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, mp->tlbpurge, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, sys->load, NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE,
					(mp->perf.avg_inidle*100)/mp->perf.period,
					NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE,
					(mp->perf.avg_inintr*100)/mp->perf.period,
					NUMSIZE);
				bp += NUMSIZE;
				readnum(0, bp, NUMSIZE, 0, NUMSIZE); /* sched # */
				bp += NUMSIZE;
				bp = strecpy(bp, e, rolename[mp->NIX.nixtype]);
				*bp++ = '\n';
			}
		if(waserror()){
			free(b);
			nexterror();
		}
		n = readstr(offset, buf, nread, b);
		free(b);
		poperror();
		return n;

	case Qswap:
		tmp[0] = 0;
		s = seprintpagestats(tmp, tmp + sizeof tmp);
		s = seprintphysstats(s, tmp + sizeof tmp);
		b = buf;
		l = s - tmp;
		i = readstr(offset, b, l, tmp);
		b += i;
		n -= i;
		if(offset > l)
			offset -= l;
		else
			offset = 0;

		return i + mallocreadsummary(c, b, n, offset);

	case Qsysname:
		if(sysname == nil)
			return 0;
		return readstr(offset, buf, n, sysname);

	case Qrandom:
		return randomread(buf, n);

	case Qurandom:
		return urandomread(buf, n);

	case Qdrivers:
		return devtabread(c, buf, n, off);

	case Qzero:
		memset(buf, 0, n);
		return n;

	case Qosversion:
		snprint(tmp, sizeof tmp, "2000");
		n = readstr(offset, buf, n, tmp);
		return n;

	case Qdebug:
		s = seprint(tmp, tmp + sizeof tmp, "locks %lu\n", lockstats.locks);
		s = seprint(s, tmp + sizeof tmp, "glare %lu\n", lockstats.glare);
		s = seprint(s, tmp + sizeof tmp, "inglare %lu\n", lockstats.inglare);
		s = seprint(s, tmp + sizeof tmp, "qlock %lu\n", qlockstats.qlock);
		seprint(s, tmp + sizeof tmp, "qlockq %lu\n", qlockstats.qlockq);
		return readstr(offset, buf, n, tmp);
		break;

	case Qsyscall:
		snprint(tmp, sizeof tmp, "%s", printallsyscalls ? "on" : "off");
		return readstr(offset, buf, n, tmp);
		break;
	default:
		print("consread %#llx\n", c->qid.path);
		error(Egreg);
	}
	return -1;		/* never reached */
}

static int32_t
conswrite(Chan *c, void *va, int32_t n, int64_t off)
{
	Proc *up = externup();
	char buf[256], ch;
	int32_t l, bp;
	char *a;
	Mach *mp;
	int i;
	uint32_t offset;
	Cmdbuf *cb;
	Cmdtab *ct;
	a = va;
	offset = off;
	extern int printallsyscalls;

	switch((uint32_t)c->qid.path){
	case Qcons:
		/*
		 * Can't page fault in putstrn, so copy the data locally.
		 */
		l = n;
		while(l > 0){
			bp = l;
			if(bp > sizeof buf)
				bp = sizeof buf;
			memmove(buf, a, bp);
			putstrn0(buf, bp, 1);
			a += bp;
			l -= bp;
		}
		break;

	case Qconsctl:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		for(a = buf; a;){
			if(strncmp(a, "rawon", 5) == 0){
				kbd.raw = 1;
				/* clumsy hack - wake up reader */
				ch = 0;
				qwrite(keybq, &ch, 1);
			}
			else if(strncmp(a, "rawoff", 6) == 0)
				kbd.raw = 0;
			else if(strncmp(a, "ctlpon", 6) == 0)
				kbd.ctlpoff = 0;
			else if(strncmp(a, "ctlpoff", 7) == 0)
				kbd.ctlpoff = 1;
			else if(strncmp(a, "sys", 3) == 0) {
				printallsyscalls = ! printallsyscalls;
				print("%sracing syscalls\n", printallsyscalls ? "T" : "Not t");
			}
			if(a = strchr(a, ' '))
				a++;
		}
		break;

	case Qtime:
		if(!iseve())
			error(Eperm);
		return writetime(a, n);

	case Qbintime:
		if(!iseve())
			error(Eperm);
		return writebintime(a, n);

	case Qhostowner:
		return hostownerwrite(a, n);

	case Qhostdomain:
		return hostdomainwrite(a, n);

	case Quser:
		return userwrite(a, n);

	case Qnull:
		break;

	case Qreboot:
		if(!iseve())
			error(Eperm);
		cb = parsecmd(a, n);

		if(waserror()) {
			free(cb);
			nexterror();
		}
		ct = lookupcmd(cb, rebootmsg, nelem(rebootmsg));
		switch(ct->index) {
		case CMhalt:
			reboot(nil, 0, 0);
			break;
		case CMreboot:
			rebootcmd(cb->nf-1, cb->f+1);
			break;
		case CMpanic:
			*(volatile uint32_t*)0=0;
			panic("/dev/reboot");
		}
		poperror();
		free(cb);
		break;

	case Qsysstat:
		for(i = 0; i < MACHMAX; i++)
			if((mp = sys->machptr[i]) != nil && mp->online){
				mp = sys->machptr[i];
				mp->cs = 0;
				mp->intr = 0;
				mp->syscall = 0;
				mp->pfault = 0;
				mp->tlbfault = 0;	/* not updated */
				mp->tlbpurge = 0;	/* # mmuflushtlb */
			}
		break;

	case Qswap:
		if(n >= sizeof buf)
			error(Egreg);
		memmove(buf, va, n);	/* so we can NUL-terminate */
		buf[n] = 0;
		if(!iseve())
			error(Eperm);
		if(buf[0]<'0' || '9'<buf[0])
			error(Ebadarg);
		if(strncmp(buf, "start", 5) == 0){
			print("request to start pager ignored\n");
			break;
		}
		break;

	case Qsysname:
		if(offset != 0)
			error(Ebadarg);
		if(n <= 0 || n >= sizeof buf)
			error(Ebadarg);
		strncpy(buf, a, n);
		buf[n] = 0;
		if(buf[n-1] == '\n')
			buf[n-1] = 0;
		kstrdup(&sysname, buf);
		break;

	case Qdebug:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		if(n > 0 && buf[n-1] == '\n')
			buf[n-1] = 0;
		error(Ebadctl);
		break;

	case Qsyscall:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		if(n > 0 && buf[n-1] == '\n')
			buf[n-1] = 0;
		// Doing strncmp right is just painful and overkill here.
		if (buf[0] == 'o') {
			if (buf[1] == 'n' && buf[2] == 0) {
				printallsyscalls = 1;
				break;
			}
			if (buf[1] == 'f' && buf[2] == 'f' && buf[3] == 0) {
				printallsyscalls = 0;
				break;
			}
		}
		error("#c/syscall: can only write on or off");
		break;
	default:
		print("conswrite: %#llx\n", c->qid.path);
		error(Egreg);
	}
	return n;
}

Dev consdevtab = {
	.dc = 'c',
	.name = "cons",

	.reset = devreset,
	.init = consinit,
	.shutdown = devshutdown,
	.attach = consattach,
	.walk = conswalk,
	.stat = consstat,
	.open = consopen,
	.create = devcreate,
	.close = consclose,
	.read = consread,
	.bread = devbread,
	.write = conswrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};

static	uint32_t	randn;

static void
seedrand(void)
{
	Proc *up = externup();
	if(!waserror()){
		randomread((void*)&randn, sizeof(randn));
		poperror();
	}
}

int
nrand(int n)
{
	if(randn == 0)
		seedrand();
	randn = randn*1103515245 + 12345 + sys->ticks;
	return (randn>>16) % n;
}

int
rand(void)
{
	nrand(1);
	return randn;
}

static uint64_t uvorder = 0x0001020304050607ULL;

static uint8_t*
le2int64_t(int64_t *to, uint8_t *f)
{
	uint8_t *t, *o;
	int i;

	t = (uint8_t*)to;
	o = (uint8_t*)&uvorder;
	for(i = 0; i < sizeof(int64_t); i++)
		t[o[i]] = f[i];
	return f+sizeof(int64_t);
}

static uint8_t*
int64_t2le(uint8_t *t, int64_t from)
{
	uint8_t *f, *o;
	int i;

	f = (uint8_t*)&from;
	o = (uint8_t*)&uvorder;
	for(i = 0; i < sizeof(int64_t); i++)
		t[i] = f[o[i]];
	return t+sizeof(int64_t);
}

static int32_t order = 0x00010203;

static uint8_t*
le2long(int32_t *to, uint8_t *f)
{
	uint8_t *t, *o;
	int i;

	t = (uint8_t*)to;
	o = (uint8_t*)&order;
	for(i = 0; i < sizeof(int32_t); i++)
		t[o[i]] = f[i];
	return f+sizeof(int32_t);
}

#if 0
static uint8_t*
long2le(uint8_t *t, int32_t from)
{
	uint8_t *f, *o;
	int i;

	f = (uint8_t*)&from;
	o = (uint8_t*)&order;
	for(i = 0; i < sizeof(int32_t); i++)
		t[i] = f[o[i]];
	return t+sizeof(int32_t);
}
#endif

char *Ebadtimectl = "bad time control";

/*
 *  like the old #c/time but with added info.  Return
 *
 *	secs	nanosecs	fastticks	fasthz
 */
static int
readtime(uint32_t off, char *buf, int n)
{
	int64_t nsec, ticks;
	int32_t sec;
	char str[7*NUMSIZE];

	nsec = todget(&ticks);
	if(fasthz == 0LL)
		fastticks((uint64_t*)&fasthz);
	sec = nsec/1000000000ULL;
	snprint(str, sizeof(str), "%*lu %*llu %*llu %*llu ",
		NUMSIZE-1, sec,
		VLNUMSIZE-1, nsec,
		VLNUMSIZE-1, ticks,
		VLNUMSIZE-1, fasthz);
	return readstr(off, buf, n, str);
}

/*
 *  set the time in seconds
 */
static int
writetime(char *buf, int n)
{
	char b[13];
	int32_t i;
	int64_t now;

	if(n >= sizeof(b))
		error(Ebadtimectl);
	strncpy(b, buf, n);
	b[n] = 0;
	i = strtol(b, 0, 0);
	if(i <= 0)
		error(Ebadtimectl);
	now = i*1000000000LL;
	todset(now, 0, 0);
	return n;
}

/*
 *  read binary time info.  all numbers are little endian.
 *  ticks and nsec are syncronized.
 */
static int
readbintime(char *buf, int n)
{
	int i;
	int64_t nsec, ticks;
	uint8_t *b = (uint8_t*)buf;

	i = 0;
	if(fasthz == 0LL)
		fastticks((uint64_t*)&fasthz);
	nsec = todget(&ticks);
	if(n >= 3*sizeof(uint64_t)){
		int64_t2le(b+2*sizeof(uint64_t), fasthz);
		i += sizeof(uint64_t);
	}
	if(n >= 2*sizeof(uint64_t)){
		int64_t2le(b+sizeof(uint64_t), ticks);
		i += sizeof(uint64_t);
	}
	if(n >= 8){
		int64_t2le(b, nsec);
		i += sizeof(int64_t);
	}
	return i;
}

/*
 *  set any of the following
 *	- time in nsec
 *	- nsec trim applied over some seconds
 *	- clock frequency
 */
static int
writebintime(char *buf, int n)
{
	uint8_t *p;
	int64_t delta;
	int32_t period;

	n--;
	p = (uint8_t*)buf + 1;
	switch(*buf){
	case 'n':
		if(n < sizeof(int64_t))
			error(Ebadtimectl);
		le2int64_t(&delta, p);
		todset(delta, 0, 0);
		break;
	case 'd':
		if(n < sizeof(int64_t)+sizeof(int32_t))
			error(Ebadtimectl);
		p = le2int64_t(&delta, p);
		le2long(&period, p);
		todset(-1, delta, period);
		break;
	case 'f':
		if(n < sizeof(uint64_t))
			error(Ebadtimectl);
		le2int64_t(&fasthz, p);
		todsetfreq(fasthz);
		break;
	}
	return n;
}
