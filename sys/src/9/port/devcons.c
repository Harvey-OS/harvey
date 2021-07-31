#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

struct {
	IOQ;			/* lock to klogputs */
	QLock;			/* qlock to getc */
}klogq;

IOQ	mouseq;
IOQ	lineq;			/* lock to getc; interrupt putc's */
IOQ	printq;
KIOQ	kbdq;

static Ref	ctl;		/* number of opens to the control file */
static int	raw;		/* true if raw has been requested on ctl file */

char	sysname[NAMELEN];

/*
 *  init the queues and set the output routine
 */
void
printinit(void)
{
	initq(&printq);
	printq.puts = 0;
	initq(&lineq);
	initq(&kbdq);
	kbdq.putc = kbdputc;
	initq(&klogq);
	initq(&mouseq);
	mouseq.putc = mouseputc;
}

/*
 *   Print a string on the console.  Convert \n to \r\n for serial
 *   line consoles.  Locking of the queues is left up to the screen
 *   or uart code.  Multi-line messages to serial consoles may get
 *   interspersed with other messages.
 */
void
putstrn(char *str, int n)
{
	char buf[PRINTSIZE+2];
	int m;
	char *t;

	/*
	 *  if there's an attached bit mapped display,
	 *  put the message there.  screenputs is defined
	 *  as a null macro for systems that have no such
	 *  display.
	 */
	screenputs(str, n);

	/*
	 *  if there's a serial line being used as a console,
	 *  put the message there.  Tack a carriage return
	 *  before new lines.
	 */
	if(printq.puts == 0)
		return;

	while(n > 0){
		t = memchr(str, '\n', n);
		if(t){
			m = t - str;
			memmove(buf, str, m);
			buf[m] = '\r';
			buf[m+1] = '\n';
			(*printq.puts)(&printq, buf, m+2);
			str = t + 1;
			n -= m + 1;
		} else {
			(*printq.puts)(&printq, str, n);
			break;
		}
	}
}

/*
 *   Print a string in the kernel log.  Ignore overflow.
 */
void
klogputs(char *str, long n)
{
	int s, m;
	uchar *nextin;

	s = splhi();
	lock(&klogq);
	while(n){
		m = &klogq.buf[NQ] - klogq.in;
		if(m > n)
			m = n;
		memmove(klogq.in, str, m);
		n -= m;
		str += m;
		nextin = klogq.in + m;
		if(nextin >= &klogq.buf[NQ])
			klogq.in = klogq.buf;
		else
			klogq.in = nextin;
	}
	unlock(&klogq);
	splx(s);
	wakeup(&klogq.r);
}

int
isbrkc(KIOQ *q)
{
	uchar *p;

	for(p=q->out; p!=q->in; ){
		if(raw)
			return 1;
		if(*p==0x04 || *p=='\n')
			return 1;
		p++;
		if(p >= q->buf+sizeof(q->buf))
			p = q->buf;
	}
	return 0;
}

int
sprint(char *s, char *fmt, ...)
{
	return doprint(s, s+PRINTSIZE, fmt, (&fmt+1)) - s;
}

int
snprint(char *s, int n, char *fmt, ...)
{
	return doprint(s, s+n, fmt, (&fmt+1)) - s;
}

int
print(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	putstrn(buf, n);
	return n;
}

int
kprint(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	klogputs(buf, n);
	return n;
}

void
panic(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	strcpy(buf, "panic: ");
	n = doprint(buf+strlen(buf), buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	buf[n] = '\n';
	putstrn(buf, n+1);
	dumpstack();
	exit(1);
}
int
pprint(char *fmt, ...)
{
	char buf[2*PRINTSIZE];
	Chan *c;
	int n;

	if(u->p->fgrp == 0)
		return 0;

	c = u->p->fgrp->fd[2];
	if(c==0 || (c->mode!=OWRITE && c->mode!=ORDWR))
		return 0;
	n = sprint(buf, "%s %d: ", u->p->text, u->p->pid);
	n = doprint(buf+n, buf+sizeof(buf), fmt, (&fmt+1)) - buf;

	if(waserror())
		return 0;
	(*devtab[c->type].write)(c, buf, n, c->offset);
	poperror();

	lock(c);
	c->offset += n;
	unlock(c);

	return n;
}

void
prflush(void)
{
	while(printq.in != printq.out) ;
}
#include <ureg.h>
void
echo(Rune r, char *buf, int n)
{
	static int ctrlt;

	/*
	 * ^p hack
	 */
	if(r==0x10 && cpuserver)
		exit(0);

	/*
	 * ^t hack BUG
	 */
	if(ctrlt == 2){
		ctrlt = 0;
		switch(r){
		case 0x14:
			break;	/* pass it on */
		case 'x':
			xsummary();
			break;
		case 'b':
			bitdebug();
			break;
		case 'd':
			consdebug();
			return;
		case 'p':
			procdump();
			return;
		case 'r':
			exit(0);
			break;
		}
	}else if(r == 0x14){
		ctrlt++;
		return;
	}
	ctrlt = 0;
	if(raw)
		return;
	if(r == 0x15)
		putstrn("^U\n", 3);
	else
		putstrn(buf, n);
}

/*
 *  turn '\r' into '\n' before putting it into the queue
 */
int
kbdcr2nl(IOQ *q, int ch)
{
	if(ch == '\r')
		ch = '\n';
	return kbdputc(q, ch);
}

/*
 *  Put character, possibly a rune, into read queue at interrupt time.
 *  Always called splhi from processor 0.
 */
int
kbdputc(IOQ *q, int ch)
{
	int i, n;
	char buf[3];
	Rune r;

	USED(q);
	r = ch;
	n = runetochar(buf, &r);
	if(n == 0)
		return 0;
	echo(r, buf, n);
	kbdq.c = r;
	for(i=0; i<n; i++){
		*kbdq.in++ = buf[i];
		if(kbdq.in == kbdq.buf+sizeof(kbdq.buf))
			kbdq.in = kbdq.buf;
	}
	if(raw || r=='\n' || r==0x04)
		wakeup(&kbdq.r);
	return 0;
}

void
kbdrepeat(int rep)
{
	kbdq.repeat = rep;
	kbdq.count = 0;
}

void
kbdclock(void)
{
	if(kbdq.repeat == 0)
		return;
	if(kbdq.repeat==1 && ++kbdq.count>HZ){
		kbdq.repeat = 2;
		kbdq.count = 0;
		return;
	}
	if(++kbdq.count&1)
		kbdputc(&kbdq, kbdq.c);
}

int
consactive(void)
{
	return printq.in != printq.out;
}

enum{
	Qdir,
	Qauth,
	Qauthcheck,
	Qauthent,
	Qclock,
	Qcons,
	Qconsctl,
	Qcputime,
	Qhz,
	Qkey,
	Qhostdomain,
	Qhostowner,
	Qklog,
	Qlights,
	Qmsec,
	Qnoise,
	Qnull,
	Qpgrpid,
	Qpid,
	Qppid,
	Qswap,
	Qsysname,
	Qsysstat,
	Qtime,
	Quser,
};

Dirtab consdir[]={
	"authenticate",	{Qauth},	0,		0666,
	"authcheck",	{Qauthcheck},	0,		0666,
	"authenticator", {Qauthent},	0,		0666,
	"clock",	{Qclock},	2*NUMSIZE,	0444,
	"cons",		{Qcons},	0,		0660,
	"consctl",	{Qconsctl},	0,		0220,
	"cputime",	{Qcputime},	6*NUMSIZE,	0444,
	"hostdomain",	{Qhostdomain},	DOMLEN,		0664,
	"hostowner",	{Qhostowner},	NAMELEN,	0664,
	"hz",		{Qhz},		NUMSIZE,	0666,
	"key",		{Qkey},		DESKEYLEN,	0622,
	"klog",		{Qklog},	0,		0444,
	"lights",	{Qlights},	0,		0220,
	"msec",		{Qmsec},	NUMSIZE,	0444,
	"noise",	{Qnoise},	0,		0220,
	"null",		{Qnull},	0,		0666,
	"pgrpid",	{Qpgrpid},	NUMSIZE,	0444,
	"pid",		{Qpid},		NUMSIZE,	0444,
	"ppid",		{Qppid},	NUMSIZE,	0444,
	"swap",		{Qswap},	0,		0664,
	"sysname",	{Qsysname},	0,		0664,
	"sysstat",	{Qsysstat},	0,		0666,
	"time",		{Qtime},	NUMSIZE,	0664,
 	"user",		{Quser},	NAMELEN,	0666,
};

#define	NCONS	(sizeof consdir/sizeof(Dirtab))

ulong	boottime;		/* seconds since epoch at boot */

long
seconds(void)
{
	return boottime + TK2SEC(MACHP(0)->ticks);
}

int
readnum(ulong off, char *buf, ulong n, ulong val, int size)
{
	char tmp[64];
	Fconv fconv = (Fconv){ tmp, tmp+sizeof(tmp), size-1, 0, 0, 'u' };

	numbconv(&val, &fconv);
	tmp[size-1] = ' ';
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, tmp+off, n);
	return n;
}

int
readstr(ulong off, char *buf, ulong n, char *str)
{
	int size;

	size = strlen(str);
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, str+off, n);
	return n;
}

void
consreset(void)
{
}

void
consinit(void)
{
}

Chan*
consattach(char *spec)
{
	return devattach('c', spec);
}

Chan*
consclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
conswalk(Chan *c, char *name)
{
	return devwalk(c, name, consdir, NCONS, devgen);
}

void
consstat(Chan *c, char *dp)
{
	devstat(c, dp, consdir, NCONS, devgen);
}

Chan*
consopen(Chan *c, int omode)
{
	c->aux = 0;
	switch(c->qid.path){
	case Qconsctl:
		if(!iseve())
			error(Eperm);
		incref(&ctl);
		break;
	}
	return devopen(c, omode, consdir, NCONS, devgen);
}

void
conscreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
consclose(Chan *c)
{
	/* last close of control file turns off raw */
	switch(c->qid.path){
	case Qconsctl:
		if(c->flag&COPEN){
			lock(&ctl);
			if(--ctl.ref == 0)
				raw = 0;
			unlock(&ctl);
		}
		break;
	case Qauth:
	case Qauthcheck:
	case Qauthent:
		authclose(c);
		break;
	}
}

long
consread(Chan *c, void *buf, long n, ulong offset)
{
	int ch, i, k, id;
	ulong l;
	char *cbuf = buf;
	char *b, *bp;
	char tmp[128];	/* must be >= 6*NUMSIZE */
	Mach *mp;

	if(n <= 0)
		return n;
	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, consdir, NCONS, devgen);

	case Qcons:
		qlock(&kbdq);
		if(waserror()){
			qunlock(&kbdq);
			nexterror();
		}
		while(!cangetc(&lineq)){
			sleep(&kbdq.r, isbrkc, &kbdq);
			do{
				lock(&lineq);
				ch = getc(&kbdq);
				if(raw)
					goto Default;
				switch(ch){
				case '\b':
					if(lineq.in != lineq.out){
						if(lineq.in == lineq.buf)
							lineq.in = lineq.buf+sizeof(lineq.buf);
						lineq.in--;
					}
					break;
				case 0x15:
					lineq.in = lineq.out;
					break;
				Default:
				default:
					*lineq.in = ch;
					if(lineq.in >= lineq.buf+sizeof(lineq.buf)-1)
						lineq.in = lineq.buf;
					else
						lineq.in++;
				}
				unlock(&lineq);
			}while(raw==0 && ch!='\n' && ch!=0x04);
		}
		i = 0;
		while(n > 0){
			ch = getc(&lineq);
			if(ch==-1 || (raw==0 && ch==0x04))
				break;
			i++;
			*cbuf++ = ch;
			--n;
		}
		poperror();
		qunlock(&kbdq);
		return i;

	case Qcputime:
		k = offset;
		if(k >= 6*NUMSIZE)
			return 0;
		if(k+n > 6*NUMSIZE)
			n = 6*NUMSIZE - k;
		/* easiest to format in a separate buffer and copy out */
		for(i=0; i<6 && NUMSIZE*i<k+n; i++){
			l = u->p->time[i];
			if(i == TReal)
				l = MACHP(0)->ticks - l;
			l = TK2MS(l);
			readnum(0, tmp+NUMSIZE*i, NUMSIZE, l, NUMSIZE);
		}
		memmove(buf, tmp+k, n);
		return n;

	case Qpgrpid:
		return readnum(offset, buf, n, u->p->pgrp->pgrpid, NUMSIZE);

	case Qpid:
		return readnum(offset, buf, n, u->p->pid, NUMSIZE);

	case Qppid:
		return readnum(offset, buf, n, u->p->parentpid, NUMSIZE);

	case Qtime:
		return readnum(offset, buf, n, boottime+TK2SEC(MACHP(0)->ticks), 12);

	case Qclock:
		k = offset;
		if(k >= 2*NUMSIZE)
			return 0;
		if(k+n > 2*NUMSIZE)
			n = 2*NUMSIZE - k;
		readnum(0, tmp, NUMSIZE, MACHP(0)->ticks, NUMSIZE);
		readnum(0, tmp+NUMSIZE, NUMSIZE, HZ, NUMSIZE);
		memmove(buf, tmp+k, n);
		return n;

	case Qkey:
		return keyread(buf, n, offset);

	case Qauth:
		return authread(c, cbuf, n);

	case Qauthent:
		return authentread(c, cbuf, n);

	case Qhostowner:
		return readstr(offset, buf, n, eve);

	case Qhostdomain:
		return readstr(offset, buf, n, hostdomain);

	case Quser:
		return readstr(offset, buf, n, u->p->user);

	case Qnull:
		return 0;

	case Qklog:
		qlock(&klogq);
		if(waserror()){
			qunlock(&klogq);
			nexterror();
		}
		while(!cangetc(&klogq))
			sleep(&klogq.r, cangetc, &klogq);
		for(i=0; i<n; i++){
			if((ch=getc(&klogq)) == -1)
				break;
			*cbuf++ = ch;
		}
		poperror();
		qunlock(&klogq);
		return i;

	case Qmsec:
		return readnum(offset, buf, n, TK2MS(MACHP(0)->ticks), NUMSIZE);

	case Qhz:
		return readnum(offset, buf, n, HZ, NUMSIZE);

	case Qsysstat:
		b = smalloc(conf.nmach*(NUMSIZE*8+1) + 1);	/* +1 for NUL */
		bp = b;
		for(id = 0; id < 32; id++) {
			if(active.machs & (1<<id)) {
				mp = MACHP(id);
				readnum(0, bp, NUMSIZE, id, NUMSIZE);
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
				readnum(0, bp, NUMSIZE, mp->load, NUMSIZE);
				bp += NUMSIZE;
				*bp++ = '\n';
			}
		}
		n = readstr(offset, buf, n, b);
		free(b);
		return n;

	case Qswap:
		sprint(tmp, "%d/%d memory %d/%d swap\n",
				palloc.user-palloc.freecount, palloc.user, 
				conf.nswap-swapalloc.free, conf.nswap);

		return readstr(offset, buf, n, tmp);

	case Qsysname:
		return readstr(offset, buf, n, sysname);

	default:
		print("consread %lux\n", c->qid);
		error(Egreg);
	}
	return -1;		/* never reached */
}

void
conslights(char *a, int n)
{
	char line[128];
	char *lp;
	int c;

	lp = line;
	while(n--){
		*lp++ = c = *a++;
		if(c=='\n' || n==0 || lp==&line[sizeof(line)-1])
			break;
	}
	*lp = 0;
	lights(strtoul(line, 0, 0));
}

void
consnoise(char *a, int n)
{
	int freq;
	int duration;
	char line[128];
	char *lp;
	int c;

	lp = line;
	while(n--){
		*lp++ = c = *a++;
		if(c=='\n' || n==0 || lp==&line[sizeof(line)-1]){
			*lp = 0;
			freq = strtoul(line, &lp, 0);
			while(*lp==' ' || *lp=='\t')
				lp++;
			duration = strtoul(lp, &lp, 0);
			buzz(freq, duration);
			lp = line;
		}
	}
}

long
conswrite(Chan *c, void *va, long n, ulong offset)
{
	char cbuf[64];
	char buf[256];
	long l, bp;
	char *a = va;
	Mach *mp;
	int id, fd, ch;
	Chan *swc;

	switch(c->qid.path){
	case Qcons:
		l = n;
		while(l > 0){
			bp = l;
			if(bp > sizeof buf)
				bp = sizeof buf;
			memmove(buf, a, bp);
			putstrn(a, bp);
			a += bp;
			l -= bp;
		}
		break;

	case Qconsctl:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		if(strncmp(a, "rawon", 5) == 0){
			lock(&lineq);
			while((ch=getc(&kbdq)) != -1){
				*lineq.in++ = ch;
				if(lineq.in == lineq.buf+sizeof(lineq.buf))
					lineq.in = lineq.buf;
			}
			unlock(&lineq);
			lock(&ctl);
			raw = 1;
			unlock(&ctl);
		} 
		else
		if(strncmp(a, "rawoff", 6) == 0){
			lock(&ctl);
			raw = 0;
			unlock(&ctl);
		}
		else
			error(Ebadctl);
		break;

	case Qtime:
		if(n<=0 || boottime!=0)	/* write once file */
			return 0;
		if(n >= sizeof cbuf)
			n = sizeof cbuf - 1;
		memmove(cbuf, a, n);
		cbuf[n-1] = 0;
		boottime = strtoul(a, 0, 0)-TK2SEC(MACHP(0)->ticks);
		break;

	case Qkey:
		return keywrite(a, n);

	case Qhostowner:
		return hostownerwrite(a, n);

	case Qhostdomain:
		return hostdomainwrite(a, n);

	case Quser:
		return userwrite(a, n);

	case Qauth:
		return authwrite(c, a, n);

	case Qauthcheck:
		return authcheck(c, a, n);

	case Qauthent:
		return authentwrite(c, a, n);

	case Qnull:
		break;

	case Qnoise:
		consnoise(a, n);
		break;

	case Qlights:
		conslights(a, n);
		break;

	case Qsysstat:
		for(id = 0; id < 32; id++) {
			if(active.machs & (1<<id)) {
				mp = MACHP(id);
				mp->cs = 0;
				mp->intr = 0;
				mp->syscall = 0;
				mp->pfault = 0;
				mp->tlbfault = 0;
				mp->tlbpurge = 0;
			}
		}
		break;

	case Qswap:
		if(n >= sizeof buf)
			error(Egreg);
		memmove(buf, va, n);	/* so we can NUL-terminate */
		buf[n] = 0;
		/* start a pager if not already started */
		if(strncmp(buf, "start", 5) == 0){
			kickpager();
			break;
		}
		if(cpuserver && strcmp(u->p->user, eve) != 0)
			error(Eperm);
		if(buf[0]<'0' || '9'<buf[0])
			error(Ebadusefd);
		fd = strtoul(buf, 0, 0);
		swc = fdtochan(fd, -1, 1, 0);
		setswapchan(swc);
		break;

	case Qsysname:
		if(offset != 0)
			error(Ebadarg);
		if(n <= 0 || n >= NAMELEN)
			error(Ebadarg);
		strncpy(sysname, a, n);
		sysname[n] = 0;
		if(sysname[n-1] == '\n')
			sysname[n-1] = 0;
		break;

	default:
		print("conswrite: %d\n", c->qid.path);
		error(Egreg);
	}
	return n;
}

void
consremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
conswstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

int
nrand(int n)
{
	static ulong randn;

	randn = randn*1103515245 + 12345 + MACHP(0)->ticks;
	return (randn>>16) % n;
}

void
setterm(char *f)
{
	char buf[2*NAMELEN];

	sprint(buf, f, conffile);
	ksetenv("terminal", buf);
}
