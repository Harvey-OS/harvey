#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../ip/ip.h"

enum {
	Nlog		= 4*1024,
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
	{ "il",		Logil, },
	{ "icmp",	Logicmp, },
	{ "udp",	Logudp, },
	{ "compress",	Logcompress, },
	{ "ilmsg",	Logil|Logilmsg, },
	{ "gre",	Loggre, },
	{ "tcpmsg",	Logtcp|Logtcpmsg, },
	{ "udpmsg",	Logudp|Logudpmsg, },
	{ "ipmsg",	Logip|Logipmsg, },
	{ "esp",	Logesp, },
	{ nil,		0, },
};

static char Ebadnetctl[] = "unknown netlog ctl message";

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

	qlock(f->alog);
	if(waserror()){
		qunlock(f->alog);
		nexterror();
	}

	for(;;){
		lock(f->alog);
		if(f->alog->len){
			if(n > f->alog->len)
				n = f->alog->len;
			d = 0;
			rptr = f->alog->rptr;
			f->alog->rptr += n;
			if(f->alog->rptr >= f->alog->end){
				d = f->alog->rptr - f->alog->end;
				f->alog->rptr = f->alog->buf + d;
			}
			f->alog->len -= n;
			unlock(f->alog);

			i = n-d;
			p = a;
			memmove(p, rptr, i);
			memmove(p+i, f->alog->buf, d);
			break;
		}
		else
			unlock(f->alog);

		sleep(f->alog, netlogready, f);
	}

	qunlock(f->alog);
	poperror();

	return n;
}

char*
netlogctl(Fs *f, char* s, int len)
{
	int i, n, set;
	Netlogflag *fp;
	char *fields[10], *p, buf[256];

	if(len == 0)
		return Ebadnetctl;

	if(len >= sizeof(buf))
		len = sizeof(buf)-1;
	strncpy(buf, s, len);
	buf[len] = 0;
	if(len > 0 && buf[len-1] == '\n')
		buf[len-1] = 0;

	n = getfields(buf, fields, 10, 1, " ");
	if(n < 2)
		return Ebadnetctl;

	if(strcmp("set", fields[0]) == 0)
		set = 1;
	else if(strcmp("clear", fields[0]) == 0)
		set = 0;
	else if(strcmp("only", fields[0]) == 0){
		parseip(f->alog->iponly, fields[1]);
		if(ipcmp(f->alog->iponly, IPnoaddr) == 0)
			f->alog->iponlyset = 0;
		else
			f->alog->iponlyset = 1;
		return nil;
	} else
		return Ebadnetctl;

	p = strchr(fields[n-1], '\n');
	if(p)
		*p = 0;

	for(i = 1; i < n; i++){
		for(fp = flags; fp->name; fp++)
			if(strcmp(fp->name, fields[i]) == 0)
				break;
		if(fp->name == nil)
			continue;
		if(set)
			f->alog->logmask |= fp->mask;
		else
			f->alog->logmask &= ~fp->mask;
	}

	return nil;
}

void
netlog(Fs *f, int mask, char *fmt, ...)
{
	char buf[128], *t, *fp;
	int i, n;
	va_list arg;

	if(!(f->alog->logmask & mask))
		return;

	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(f->alog->opens == 0)
		return;

	lock(f->alog);
	i = f->alog->len + n - Nlog;
	if(i > 0){
		f->alog->len -= i;
		f->alog->rptr += i;
		if(f->alog->rptr >= f->alog->end)
			f->alog->rptr = f->alog->buf + (f->alog->rptr - f->alog->end);
	}
	t = f->alog->rptr + f->alog->len;
	fp = buf;
	f->alog->len += n;
	while(n-- > 0){
		if(t >= f->alog->end)
			t = f->alog->buf + (t - f->alog->end);
		*t++ = *fp++;
	}
	unlock(f->alog);

	wakeup(f->alog);
}
