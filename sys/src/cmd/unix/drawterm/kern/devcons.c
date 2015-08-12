/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

#include "keyboard.h"

void (*consdebug)(void) = 0;
void (*screenputs)(char*, int) = 0;

Queue* kbdq;         /* unprocessed console input */
Queue* lineq;        /* processed console input */
Queue* serialoq;     /* serial console output */
Queue* kprintoq;     /* console output, for /dev/kprint */
int32_t kprintinuse; /* test and set whether /dev/kprint is open */
Lock kprintlock;
int iprintscreenputs = 0;

int panicking;

struct {
	int exiting;
	int machs;
} active;

static struct {
	QLock lk;

	int raw;         /* true if we shouldn't process input */
	int ctl;         /* number of opens to the control file */
	int x;           /* index into line */
	char line[1024]; /* current input line */

	int count;
	int ctlpoff;

	/* a place to save up characters at interrupt time before dumping them
	 * in the queue */
	Lock lockputc;
	char istage[1024];
	char* iw;
	char* ir;
	char* ie;
} kbd = {
    {0},
    0,
    0,
    0,
    {0},
    0,
    0,
    {0},
    {0},
    kbd.istage,
    kbd.istage,
    kbd.istage + sizeof(kbd.istage),
};

char* sysname;
int64_t fasthz;

static int readtime(uint32_t, char*, int);
static int readbintime(char*, int);
static int writetime(char*, int);
static int writebintime(char*, int);

enum { CMreboot,
       CMpanic,
};

Cmdtab rebootmsg[] = {
    CMreboot, "reboot", 0, CMpanic, "panic", 0,
};

int
return0(void* v)
{
	return 0;
}

void
printinit(void)
{
	lineq = qopen(2 * 1024, 0, 0, nil);
	if(lineq == nil)
		panic("printinit");
	qnoblock(lineq, 1);

	kbdq = qopen(4 * 1024, 0, 0, 0);
	if(kbdq == nil)
		panic("kbdinit");
	qnoblock(kbdq, 1);
}

int
consactive(void)
{
	if(serialoq)
		return qlen(serialoq) > 0;
	return 0;
}

void
prflush(void)
{
	/*
	        ulong now;

	        now = m->ticks;
	        while(consactive())
	                if(m->ticks - now >= HZ)
	                        break;
	*/
}

/*
 *   Print a string on the console.  Convert \n to \r\n for serial
 *   line consoles.  Locking of the queues is left up to the screen
 *   or uart code.  Multi-line messages to serial consoles may get
 *   interspersed with other messages.
 */
static void
putstrn0(char* str, int n, int usewrite)
{
	/*
	 *  if someone is reading /dev/kprint,
	 *  put the message there.
	 *  if not and there's an attached bit mapped display,
	 *  put the message there.
	 *
	 *  if there's a serial line being used as a console,
	 *  put the message there.
	 */
	if(kprintoq != nil && !qisclosed(kprintoq)) {
		if(usewrite)
			qwrite(kprintoq, str, n);
		else
			qiwrite(kprintoq, str, n);
	} else if(screenputs != 0)
		screenputs(str, n);
}

void
putstrn(char* str, int n)
{
	putstrn0(str, n, 0);
}

int noprint;

int
print(char* fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	if(noprint)
		return -1;

	va_start(arg, fmt);
	n = vseprint(buf, buf + sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	putstrn(buf, n);

	return n;
}

void
panic(char* fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	kprintoq = nil; /* don't try to write to /dev/kprint */

	if(panicking)
		for(;;)
			;
	panicking = 1;

	splhi();
	strcpy(buf, "panic: ");
	va_start(arg, fmt);
	n = vseprint(buf + strlen(buf), buf + sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\n';
	uartputs(buf, n + 1);
	if(consdebug)
		(*consdebug)();
	spllo();
	prflush();
	putstrn(buf, n + 1);
	dumpstack();

	exit(1);
}

int
pprint(char* fmt, ...)
{
	int n;
	Chan* c;
	va_list arg;
	char buf[2 * PRINTSIZE];

	if(up == nil || up->fgrp == nil)
		return 0;

	c = up->fgrp->fd[2];
	if(c == 0 || (c->mode != OWRITE && c->mode != ORDWR))
		return 0;
	n = sprint(buf, "%s %lud: ", up->text, up->pid);
	va_start(arg, fmt);
	n = vseprint(buf + n, buf + sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(waserror())
		return 0;
	devtab[c->type]->write(c, buf, n, c->offset);
	poperror();

	lock(&c->ref.lk);
	c->offset += n;
	unlock(&c->ref.lk);

	return n;
}

static void
echoscreen(char* buf, int n)
{
	char* e, *p;
	char ebuf[128];
	int x;

	p = ebuf;
	e = ebuf + sizeof(ebuf) - 4;
	while(n-- > 0) {
		if(p >= e) {
			screenputs(ebuf, p - ebuf);
			p = ebuf;
		}
		x = *buf++;
		if(x == 0x15) {
			*p++ = '^';
			*p++ = 'U';
			*p++ = '\n';
		} else
			*p++ = x;
	}
	if(p != ebuf)
		screenputs(ebuf, p - ebuf);
}

static void
echoserialoq(char* buf, int n)
{
	char* e, *p;
	char ebuf[128];
	int x;

	p = ebuf;
	e = ebuf + sizeof(ebuf) - 4;
	while(n-- > 0) {
		if(p >= e) {
			qiwrite(serialoq, ebuf, p - ebuf);
			p = ebuf;
		}
		x = *buf++;
		if(x == '\n') {
			*p++ = '\r';
			*p++ = '\n';
		} else if(x == 0x15) {
			*p++ = '^';
			*p++ = 'U';
			*p++ = '\n';
		} else
			*p++ = x;
	}
	if(p != ebuf)
		qiwrite(serialoq, ebuf, p - ebuf);
}

static void
echo(char* buf, int n)
{
	static int ctrlt;
	int x;
	char* e, *p;

	e = buf + n;
	for(p = buf; p < e; p++) {
		switch(*p) {
		case 0x10: /* ^P */
			if(cpuserver && !kbd.ctlpoff) {
				active.exiting = 1;
				return;
			}
			break;
		case 0x14: /* ^T */
			ctrlt++;
			if(ctrlt > 2)
				ctrlt = 2;
			continue;
		}

		if(ctrlt != 2)
			continue;

		/* ^T escapes */
		ctrlt = 0;
		switch(*p) {
		case 'S':
			x = splhi();
			dumpstack();
			procdump();
			splx(x);
			return;
		case 's':
			dumpstack();
			return;
		case 'x':
			xsummary();
			ixsummary();
			mallocsummary();
			pagersummary();
			return;
		case 'd':
			if(consdebug == 0)
				consdebug = rdb;
			else
				consdebug = 0;
			print("consdebug now 0x%p\n", consdebug);
			return;
		case 'D':
			if(consdebug == 0)
				consdebug = rdb;
			consdebug();
			return;
		case 'p':
			x = spllo();
			procdump();
			splx(x);
			return;
		case 'q':
			scheddump();
			return;
		case 'k':
			killbig();
			return;
		case 'r':
			exit(0);
			return;
		}
	}

	qproduce(kbdq, buf, n);
	if(kbd.raw)
		return;
	if(screenputs != 0)
		echoscreen(buf, n);
	if(serialoq)
		echoserialoq(buf, n);
}

/*
 *  Called by a uart interrupt for console input.
 *
 *  turn '\r' into '\n' before putting it into the queue.
 */
int
kbdcr2nl(Queue* q, int ch)
{
	char* next;

	USED(q);
	ilock(&kbd.lockputc); /* just a mutex */
	if(ch == '\r' && !kbd.raw)
		ch = '\n';
	next = kbd.iw + 1;
	if(next >= kbd.ie)
		next = kbd.istage;
	if(next != kbd.ir) {
		*kbd.iw = ch;
		kbd.iw = next;
	}
	iunlock(&kbd.lockputc);
	return 0;
}
static void
_kbdputc(int c)
{
	Rune r;
	char buf[UTFmax];
	int n;

	r = c;
	n = runetochar(buf, &r);
	if(n == 0)
		return;
	echo(buf, n);
	//	kbd.c = r;
	//	qproduce(kbdq, buf, n);
}

/* _kbdputc, but with compose translation */
int
kbdputc(Queue* q, int c)
{
	int i;
	static int collecting, nk;
	static Rune kc[5];

	if(c == Kalt) {
		collecting = 1;
		nk = 0;
		return 0;
	}

	if(!collecting) {
		_kbdputc(c);
		return 0;
	}

	kc[nk++] = c;
	c = latin1(kc, nk);
	if(c < -1) /* need more keystrokes */
		return 0;
	if(c != -1) /* valid sequence */
		_kbdputc(c);
	else
		for(i = 0; i < nk; i++)
			_kbdputc(kc[i]);
	nk = 0;
	collecting = 0;

	return 0;
}

enum { Qdir,
       Qbintime,
       Qcons,
       Qconsctl,
       Qcpunote,
       Qcputime,
       Qdrivers,
       Qkprint,
       Qhostdomain,
       Qhostowner,
       Qnull,
       Qosversion,
       Qpgrpid,
       Qpid,
       Qppid,
       Qrandom,
       Qreboot,
       Qsecstore,
       Qshowfile,
       Qsnarf,
       Qswap,
       Qsysname,
       Qsysstat,
       Qtime,
       Quser,
       Qzero,
};

enum { VLNUMSIZE = 22,
};

static Dirtab consdir[] = {
    ".",
    {Qdir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "bintime",
    {Qbintime},
    24,
    0664,
    "cons",
    {Qcons},
    0,
    0660,
    "consctl",
    {Qconsctl},
    0,
    0220,
    "cpunote",
    {Qcpunote},
    0,
    0444,
    "cputime",
    {Qcputime},
    6 * NUMSIZE,
    0444,
    "drivers",
    {Qdrivers},
    0,
    0444,
    "hostdomain",
    {Qhostdomain},
    DOMLEN,
    0664,
    "hostowner",
    {Qhostowner},
    0,
    0664,
    "kprint",
    {Qkprint, 0, QTEXCL},
    0,
    DMEXCL | 0440,
    "null",
    {Qnull},
    0,
    0666,
    "osversion",
    {Qosversion},
    0,
    0444,
    "pgrpid",
    {Qpgrpid},
    NUMSIZE,
    0444,
    "pid",
    {Qpid},
    NUMSIZE,
    0444,
    "ppid",
    {Qppid},
    NUMSIZE,
    0444,
    "random",
    {Qrandom},
    0,
    0444,
    "reboot",
    {Qreboot},
    0,
    0664,
    "secstore",
    {Qsecstore},
    0,
    0666,
    "showfile",
    {Qshowfile},
    0,
    0220,
    "snarf",
    {Qsnarf},
    0,
    0666,
    "swap",
    {Qswap},
    0,
    0664,
    "sysname",
    {Qsysname},
    0,
    0664,
    "sysstat",
    {Qsysstat},
    0,
    0666,
    "time",
    {Qtime},
    NUMSIZE + 3 * VLNUMSIZE,
    0664,
    "user",
    {Quser},
    0,
    0666,
    "zero",
    {Qzero},
    0,
    0444,
};

char secstorebuf[65536];
Dirtab* secstoretab = &consdir[Qsecstore];
Dirtab* snarftab = &consdir[Qsnarf];

int
readnum(uint32_t off, char* buf, uint32_t n, uint32_t val, int size)
{
	char tmp[64];

	snprint(tmp, sizeof(tmp), "%*.0lud", size - 1, val);
	tmp[size - 1] = ' ';
	if(off >= size)
		return 0;
	if(off + n > size)
		n = size - off;
	memmove(buf, tmp + off, n);
	return n;
}

int
readstr(uint32_t off, char* buf, uint32_t n, char* str)
{
	int size;

	size = strlen(str);
	if(off >= size)
		return 0;
	if(off + n > size)
		n = size - off;
	memmove(buf, str + off, n);
	return n;
}

static void
consinit(void)
{
	todinit();
	randominit();
	/*
	 * at 115200 baud, the 1024 char buffer takes 56 ms to process,
	 * processing it every 22 ms should be fine
	 */
	/*	addclock0link(kbdputcclock, 22); */
}

static Chan*
consattach(char* spec)
{
	return devattach('c', spec);
}

static Walkqid*
conswalk(Chan* c, Chan* nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, consdir, nelem(consdir), devgen);
}

static int
consstat(Chan* c, uint8_t* dp, int n)
{
	return devstat(c, dp, n, consdir, nelem(consdir), devgen);
}

static Chan*
consopen(Chan* c, int omode)
{
	c->aux = nil;
	c = devopen(c, omode, consdir, nelem(consdir), devgen);
	switch((uint32_t)c->qid.path) {
	case Qconsctl:
		qlock(&kbd.lk);
		kbd.ctl++;
		qunlock(&kbd.lk);
		break;

	case Qkprint:
		lock(&kprintlock);
		if(kprintinuse != 0) {
			c->flag &= ~COPEN;
			unlock(&kprintlock);
			error(Einuse);
		}
		kprintinuse = 1;
		unlock(&kprintlock);
		if(kprintoq == nil) {
			kprintoq = qopen(8 * 1024, Qcoalesce, 0, 0);
			if(kprintoq == nil) {
				c->flag &= ~COPEN;
				error(Enomem);
			}
			qnoblock(kprintoq, 1);
		} else
			qreopen(kprintoq);
		c->iounit = qiomaxatomic;
		break;

	case Qsecstore:
		if(omode == ORDWR)
			error(Eperm);
		if(omode != OREAD)
			memset(secstorebuf, 0, sizeof secstorebuf);
		break;

	case Qsnarf:
		if(omode == ORDWR)
			error(Eperm);
		if(omode == OREAD)
			c->aux = strdup("");
		else
			c->aux = mallocz(SnarfSize, 1);
		break;
	}
	return c;
}

static void
consclose(Chan* c)
{
	switch((uint32_t)c->qid.path) {
	/* last close of control file turns off raw */
	case Qconsctl:
		if(c->flag & COPEN) {
			qlock(&kbd.lk);
			if(--kbd.ctl == 0)
				kbd.raw = 0;
			qunlock(&kbd.lk);
		}
		break;

	/* close of kprint allows other opens */
	case Qkprint:
		if(c->flag & COPEN) {
			kprintinuse = 0;
			qhangup(kprintoq, nil);
		}
		break;

	case Qsnarf:
		if(c->mode == OWRITE)
			clipwrite(c->aux);
		free(c->aux);
		break;
	}
}

static int32_t
consread(Chan* c, void* buf, int32_t n, int64_t off)
{
	char* b;
	char tmp[128]; /* must be >= 6*NUMSIZE */
	char* cbuf = buf;
	int ch, i, eol;
	int64_t offset = off;

	if(n <= 0)
		return n;
	switch((uint32_t)c->qid.path) {
	case Qdir:
		return devdirread(c, buf, n, consdir, nelem(consdir), devgen);

	case Qcons:
		qlock(&kbd.lk);
		if(waserror()) {
			qunlock(&kbd.lk);
			nexterror();
		}
		if(kbd.raw) {
			if(qcanread(lineq))
				n = qread(lineq, buf, n);
			else {
				/* read as much as possible */
				do {
					i = qread(kbdq, cbuf, n);
					cbuf += i;
					n -= i;
				} while(n > 0 && qcanread(kbdq));
				n = cbuf - (char*)buf;
			}
		} else {
			while(!qcanread(lineq)) {
				qread(kbdq, &kbd.line[kbd.x], 1);
				ch = kbd.line[kbd.x];
				eol = 0;
				switch(ch) {
				case '\b':
					if(kbd.x)
						kbd.x--;
					break;
				case 0x15:
					kbd.x = 0;
					break;
				case '\n':
				case 0x04:
					eol = 1;
				default:
					kbd.line[kbd.x++] = ch;
					break;
				}
				if(kbd.x == sizeof(kbd.line) || eol) {
					if(ch == 0x04)
						kbd.x--;
					qwrite(lineq, kbd.line, kbd.x);
					kbd.x = 0;
				}
			}
			n = qread(lineq, buf, n);
		}
		qunlock(&kbd.lk);
		poperror();
		return n;

	case Qcpunote:
		sleep(&up->sleep, return0, nil);

	case Qcputime:
		return 0;

	case Qkprint:
		return qread(kprintoq, buf, n);

	case Qpgrpid:
		return readnum((uint32_t)offset, buf, n, up->pgrp->pgrpid,
		               NUMSIZE);

	case Qpid:
		return readnum((uint32_t)offset, buf, n, up->pid, NUMSIZE);

	case Qppid:
		return readnum((uint32_t)offset, buf, n, up->parentpid,
		               NUMSIZE);

	case Qtime:
		return readtime((uint32_t)offset, buf, n);

	case Qbintime:
		return readbintime(buf, n);

	case Qhostowner:
		return readstr((uint32_t)offset, buf, n, eve);

	case Qhostdomain:
		return readstr((uint32_t)offset, buf, n, hostdomain);

	case Quser:
		return readstr((uint32_t)offset, buf, n, up->user);

	case Qnull:
		return 0;

	case Qsnarf:
		if(offset == 0) {
			free(c->aux);
			c->aux = clipread();
		}
		if(c->aux == nil)
			return 0;
		return readstr(offset, buf, n, c->aux);

	case Qsecstore:
		return readstr(offset, buf, n, secstorebuf);

	case Qsysstat:
		return 0;

	case Qswap:
		return 0;

	case Qsysname:
		if(sysname == nil)
			return 0;
		return readstr((uint32_t)offset, buf, n, sysname);

	case Qrandom:
		return randomread(buf, n);

	case Qdrivers:
		b = malloc(READSTR);
		if(b == nil)
			error(Enomem);
		n = 0;
		for(i = 0; devtab[i] != nil; i++)
			n += snprint(b + n, READSTR - n, "#%C %s\n",
			             devtab[i]->dc, devtab[i]->name);
		if(waserror()) {
			free(b);
			nexterror();
		}
		n = readstr((uint32_t)offset, buf, n, b);
		free(b);
		poperror();
		return n;

	case Qzero:
		memset(buf, 0, n);
		return n;

	case Qosversion:
		snprint(tmp, sizeof tmp, "2000");
		n = readstr((uint32_t)offset, buf, n, tmp);
		return n;

	default:
		print("consread 0x%llux\n", c->qid.path);
		error(Egreg);
	}
	return -1; /* never reached */
}

static int32_t
conswrite(Chan* c, void* va, int32_t n, int64_t off)
{
	char buf[256];
	int32_t l, bp;
	char* a = va;
	int fd;
	Chan* swc;
	uint32_t offset = off;
	Cmdbuf* cb;
	Cmdtab* ct;

	switch((uint32_t)c->qid.path) {
	case Qcons:
		/*
		 * Can't page fault in putstrn, so copy the data locally.
		 */
		l = n;
		while(l > 0) {
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
			n = sizeof(buf) - 1;
		strncpy(buf, a, n);
		buf[n] = 0;
		for(a = buf; a;) {
			if(strncmp(a, "rawon", 5) == 0) {
				qlock(&kbd.lk);
				if(kbd.x) {
					qwrite(kbdq, kbd.line, kbd.x);
					kbd.x = 0;
				}
				kbd.raw = 1;
				qunlock(&kbd.lk);
			} else if(strncmp(a, "rawoff", 6) == 0) {
				qlock(&kbd.lk);
				kbd.raw = 0;
				kbd.x = 0;
				qunlock(&kbd.lk);
			} else if(strncmp(a, "ctlpon", 6) == 0) {
				kbd.ctlpoff = 0;
			} else if(strncmp(a, "ctlpoff", 7) == 0) {
				kbd.ctlpoff = 1;
			}
			if((a = strchr(a, ' ')))
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
		case CMreboot:
			rebootcmd(cb->nf - 1, cb->f + 1);
			break;
		case CMpanic:
			panic("/dev/reboot");
		}
		poperror();
		free(cb);
		break;

	case Qsecstore:
		if(offset >= sizeof secstorebuf ||
		   offset + n + 1 >= sizeof secstorebuf)
			error(Etoobig);
		secstoretab->qid.vers++;
		memmove(secstorebuf + offset, va, n);
		return n;

	case Qshowfile:
		return showfilewrite(a, n);

	case Qsnarf:
		if(offset >= SnarfSize || offset + n >= SnarfSize)
			error(Etoobig);
		snarftab->qid.vers++;
		memmove((uint8_t*)c->aux + offset, va, n);
		return n;

	case Qsysstat:
		n = 0;
		break;

	case Qswap:
		if(n >= sizeof buf)
			error(Egreg);
		memmove(buf, va, n); /* so we can NUL-terminate */
		buf[n] = 0;
		/* start a pager if not already started */
		if(strncmp(buf, "start", 5) == 0) {
			kickpager();
			break;
		}
		if(cpuserver && !iseve())
			error(Eperm);
		if(buf[0] < '0' || '9' < buf[0])
			error(Ebadarg);
		fd = strtoul(buf, 0, 0);
		swc = fdtochan(fd, -1, 1, 1);
		setswapchan(swc);
		break;

	case Qsysname:
		if(offset != 0)
			error(Ebadarg);
		if(n <= 0 || n >= sizeof buf)
			error(Ebadarg);
		strncpy(buf, a, n);
		buf[n] = 0;
		if(buf[n - 1] == '\n')
			buf[n - 1] = 0;
		kstrdup(&sysname, buf);
		break;

	default:
		print("conswrite: 0x%llux\n", c->qid.path);
		error(Egreg);
	}
	return n;
}

Dev consdevtab = {
    'c',      "cons",

    devreset, consinit,  devshutdown, consattach, conswalk,
    consstat, consopen,  devcreate,   consclose,  consread,
    devbread, conswrite, devbwrite,   devremove,  devwstat,
};

static uint64_t uvorder = (uint64_t)0x0001020304050607ULL;

static uint8_t*
le2vlong(int64_t* to, uint8_t* f)
{
	uint8_t* t, *o;
	int i;

	t = (uint8_t*)to;
	o = (uint8_t*)&uvorder;
	for(i = 0; i < sizeof(int64_t); i++)
		t[o[i]] = f[i];
	return f + sizeof(int64_t);
}

static uint8_t*
vlong2le(uint8_t* t, int64_t from)
{
	uint8_t* f, *o;
	int i;

	f = (uint8_t*)&from;
	o = (uint8_t*)&uvorder;
	for(i = 0; i < sizeof(int64_t); i++)
		t[i] = f[o[i]];
	return t + sizeof(int64_t);
}

static int32_t order = 0x00010203;

static uint8_t*
le2long(int32_t* to, uint8_t* f)
{
	uint8_t* t, *o;
	int i;

	t = (uint8_t*)to;
	o = (uint8_t*)&order;
	for(i = 0; i < sizeof(int32_t); i++)
		t[o[i]] = f[i];
	return f + sizeof(int32_t);
}

/*
static uchar*
long2le(uchar *t, long from)
{
        uchar *f, *o;
        int i;

        f = (uchar*)&from;
        o = (uchar*)&order;
        for(i = 0; i < sizeof(long); i++)
                t[i] = f[o[i]];
        return t+sizeof(long);
}
*/

char* Ebadtimectl = "bad time control";

/*
 *  like the old #c/time but with added info.  Return
 *
 *	secs	nanosecs	fastticks	fasthz
 */
static int
readtime(uint32_t off, char* buf, int n)
{
	int64_t nsec, ticks;
	int32_t sec;
	char str[7 * NUMSIZE];

	nsec = todget(&ticks);
	if(fasthz == (int64_t)0)
		fastticks((uint64_t*)&fasthz);
	sec = nsec / ((uint64_t)1000000000);
	snprint(str, sizeof(str), "%*.0lud %*.0llud %*.0llud %*.0llud ",
	        NUMSIZE - 1, sec, VLNUMSIZE - 1, nsec, VLNUMSIZE - 1, ticks,
	        VLNUMSIZE - 1, fasthz);
	return readstr(off, buf, n, str);
}

/*
 *  set the time in seconds
 */
static int
writetime(char* buf, int n)
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
	now = i * ((int64_t)1000000000);
	todset(now, 0, 0);
	return n;
}

/*
 *  read binary time info.  all numbers are little endian.
 *  ticks and nsec are syncronized.
 */
static int
readbintime(char* buf, int n)
{
	int i;
	int64_t nsec, ticks;
	uint8_t* b = (uint8_t*)buf;

	i = 0;
	if(fasthz == (int64_t)0)
		fastticks((uint64_t*)&fasthz);
	nsec = todget(&ticks);
	if(n >= 3 * sizeof(uint64_t)) {
		vlong2le(b + 2 * sizeof(uint64_t), fasthz);
		i += sizeof(uint64_t);
	}
	if(n >= 2 * sizeof(uint64_t)) {
		vlong2le(b + sizeof(uint64_t), ticks);
		i += sizeof(uint64_t);
	}
	if(n >= 8) {
		vlong2le(b, nsec);
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
writebintime(char* buf, int n)
{
	uint8_t* p;
	int64_t delta;
	int32_t period;

	n--;
	p = (uint8_t*)buf + 1;
	switch(*buf) {
	case 'n':
		if(n < sizeof(int64_t))
			error(Ebadtimectl);
		le2vlong(&delta, p);
		todset(delta, 0, 0);
		break;
	case 'd':
		if(n < sizeof(int64_t) + sizeof(int32_t))
			error(Ebadtimectl);
		p = le2vlong(&delta, p);
		le2long(&period, p);
		todset(-1, delta, period);
		break;
	case 'f':
		if(n < sizeof(uint64_t))
			error(Ebadtimectl);
		le2vlong(&fasthz, p);
		todsetfreq(fasthz);
		break;
	}
	return n;
}

int
iprint(char* fmt, ...)
{
	int n, s;
	va_list arg;
	char buf[PRINTSIZE];

	s = splhi();
	va_start(arg, fmt);
	n = vseprint(buf, buf + sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	if(screenputs != 0 && iprintscreenputs)
		screenputs(buf, n);
#undef write
	write(2, buf, n);
	splx(s);

	return n;
}
