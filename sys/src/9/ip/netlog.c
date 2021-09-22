#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../ip/ip.h"

enum {
	Nlog		= 16*1024,
};

/*
 *  action log
 */
struct Netlog {
	Lock;
	int	opens;
	char*	buf;
	char	*end;
	char	*rptr;
	int	len;

	int	logmask;			/* mask of things to debug */
	uchar	iponly[IPaddrlen];		/* ip address to print debugging for */
	int	iponlyset;

	QLock;
	Rendez;
};

typedef struct Netlogflag {
	char*	name;
	int	mask;
} Netlogflag;

static Netlogflag flags[] =
{
	{ "ppp",	Logppp, },
	{ "ip",		Logip, },
	{ "fs",		Logfs, },
	{ "tcp",	Logtcp, },
	{ "icmp",	Logicmp, },
	{ "udp",	Logudp, },
	{ "compress",	Logcompress, },
	{ "gre",	Loggre, },
	{ "tcpwin",	Logtcp|Logtcpwin, },
	{ "tcprxmt",	Logtcp|Logtcprxmt, },
	{ "udpmsg",	Logudp|Logudpmsg, },
	{ "ipmsg",	Logip|Logipmsg, },
	{ "esp",	Logesp, },
	{ nil,		0, },
};

char Ebadnetctl[] = "too few arguments for netlog control message";

enum
{
	CMset,
	CMclear,
	CMonly,
};

static
Cmdtab routecmd[] = {
	CMset,		"set",		0,
	CMclear,	"clear",	0,
	CMonly,		"only",		0,
};

void
netloginit(Fs *f)
{
	f->alog = smalloc(sizeof(Netlog));
}

void
netlogopen(Fs *f)
{
	lock(f->alog);
	if(waserror()){
		unlock(f->alog);
		nexterror();
	}
	if(f->alog->opens == 0){
		if(f->alog->buf == nil)
			f->alog->buf = malloc(Nlog);
		if(f->alog->buf == nil)
			error(Enomem);
		f->alog->rptr = f->alog->buf;
		f->alog->end = f->alog->buf + Nlog;
	}
	f->alog->opens++;
	unlock(f->alog);
	poperror();
}

void
netlogclose(Fs *f)
{
	lock(f->alog);
	if(waserror()){
		unlock(f->alog);
		nexterror();
	}
	f->alog->opens--;
	if(f->alog->opens == 0){
		free(f->alog->buf);
		f->alog->buf = nil;
	}
	unlock(f->alog);
	poperror();
}

static int
netlogready(void *a)
{
	Fs *f = a;

	return f->alog->len;
}

long
netlogread(Fs *f, void *a, ulong, long n)
{
	int i, d;
	char *p, *rptr;
	Netlog *log;

	log = f->alog;
	qlock(log);
	if(waserror()){
		qunlock(log);
		nexterror();
	}

	for(;;){
		lock(log);
		if(log->len){
			if(n > log->len)
				n = log->len;
			d = 0;
			rptr = log->rptr;
			log->rptr += n;
			if(log->rptr >= log->end){
				d = log->rptr - log->end;
				log->rptr = log->buf + d;
			}
			log->len -= n;
			unlock(log);

			i = n-d;
			p = a;
			memmove(p, rptr, i);
			memmove(p+i, log->buf, d);
			break;
		}
		else
			unlock(log);

		sleep(log, netlogready, f);
	}

	qunlock(log);
	poperror();

	return n;
}

void
netlogctl(Fs *f, char* s, int n)
{
	int i, set;
	Netlogflag *fp;
	Cmdbuf *cb;
	Cmdtab *ct;

	cb = parsecmd(s, n);
	if(waserror()){
		free(cb);
		nexterror();
	}

	if(cb->nf < 2)
		error(Ebadnetctl);

	ct = lookupcmd(cb, routecmd, nelem(routecmd));

	SET(set);

	switch(ct->index){
	case CMset:
		set = 1;
		break;

	case CMclear:
		set = 0;
		break;

	case CMonly:
		parseip(f->alog->iponly, cb->f[1]);
		if(ipcmp(f->alog->iponly, IPnoaddr) == 0)
			f->alog->iponlyset = 0;
		else
			f->alog->iponlyset = 1;
		free(cb);
		poperror();
		return;

	default:
		cmderror(cb, "unknown netlog control message");
	}

	for(i = 1; i < cb->nf; i++){
		for(fp = flags; fp->name; fp++)
			if(strcmp(fp->name, cb->f[i]) == 0)
				break;
		if(fp->name == nil)
			continue;
		if(set)
			f->alog->logmask |= fp->mask;
		else
			f->alog->logmask &= ~fp->mask;
	}

	free(cb);
	poperror();
}

void
netlog(Fs *f, int mask, char *fmt, ...)
{
	char buf[256], *t, *fp;
	int i, n;
	Netlog *log;
	va_list arg;

	log = f->alog;
	if(!(log->logmask & mask))
		return;

	if(log->opens == 0)
		return;

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	lock(log);
	i = log->len + n - Nlog;
	if(i > 0){
		log->len -= i;
		log->rptr += i;
		if(log->rptr >= log->end)
			log->rptr = log->buf + (log->rptr - log->end);
	}
	t = log->rptr + log->len;
	fp = buf;
	log->len += n;
	while(n-- > 0){
		if(t >= log->end)
			t = log->buf + (t - log->end);
		*t++ = *fp++;
	}
	unlock(log);

	wakeup(log);
}
