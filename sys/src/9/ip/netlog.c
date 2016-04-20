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
#include	"../ip/ip.h"

enum {
	Nlog		= 16*1024,
};

/*
 *  action log
 */
struct Netlog {
	Lock	_lock;
	int	opens;
	char*	buf;
	char	*end;
	char	*rptr;
	int	len;

	int	logmask;			/* mask of things to debug */
	uint8_t	iponly[IPaddrlen];		/* ip address to print debugging for */
	int	iponlyset;

	QLock ql;
	Rendez Rendez;
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
	Proc *up = externup();
	lock(&f->alog->_lock);
	if(waserror()){
		unlock(&f->alog->_lock);
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
	unlock(&f->alog->Rendez.l);
	poperror();
}

void
netlogclose(Fs *f)
{
	Proc *up = externup();
	lock(&f->alog->Rendez.l);
	if(waserror()){
		unlock(&f->alog->Rendez.l);
		nexterror();
	}
	f->alog->opens--;
	if(f->alog->opens == 0){
		free(f->alog->buf);
		f->alog->buf = nil;
	}
	unlock(&f->alog->Rendez.l);
	poperror();
}

static int
netlogready(void *a)
{
	Fs *f = a;

	return f->alog->len;
}

int32_t
netlogread(Fs *f, void *a, uint32_t u, int32_t n)
{
	Proc *up = externup();
	int i, d;
	char *p, *rptr;

	qlock(&f->alog->ql);
	if(waserror()){
		qunlock(&f->alog->ql);
		nexterror();
	}

	for(;;){
		lock(&f->alog->_lock);
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
			unlock(&f->alog->_lock);

			i = n-d;
			p = a;
			memmove(p, rptr, i);
			memmove(p+i, f->alog->buf, d);
			break;
		}
		else
			unlock(&f->alog->_lock);

		sleep(&f->alog->Rendez, netlogready, f);
	}

	qunlock(&f->alog->ql);
	poperror();

	return n;
}

void
netlogctl(Fs *f, char* s, int n)
{
	Proc *up = externup();
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
	va_list arg;

	if(!(f->alog->logmask & mask))
		return;

	if(f->alog->opens == 0)
		return;

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	lock(&f->alog->_lock);
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
	unlock(&f->alog->_lock);
	wakeup(&f->alog->Rendez);
}
