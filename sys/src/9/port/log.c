#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

static char Ebadlogctl[] = "unknown log ctl message";

enum {
	Nlog		= 4*1024,
};

void
logopen(Log *alog)
{
	lock(alog);
	if(waserror()){
		unlock(alog);
		nexterror();
	}
	if(alog->opens == 0){
		if(alog->buf == nil)
			alog->buf = malloc(Nlog);
		alog->rptr = alog->buf;
		alog->end = alog->buf + Nlog;
	}
	alog->opens++;
	unlock(alog);
	poperror();
}

void
logclose(Log *alog)
{
	lock(alog);
	if(waserror()){
		unlock(alog);
		nexterror();
	}
	alog->opens--;
	if(alog->opens == 0){
		free(alog->buf);
		alog->buf = nil;
	}
	unlock(alog);
	poperror();
}

static int
logready(void *a)
{
	Log *alog = a;

	return alog->len;
}

long
logread(Log *alog, void *a, ulong, long n)
{
	int i, d;
	char *p, *rptr;

	qlock(&alog->readq);
	if(waserror()){
		qunlock(&alog->readq);
		nexterror();
	}

	for(;;){
		lock(alog);
		if(alog->len){
			if(n > alog->len)
				n = alog->len;
			d = 0;
			rptr = alog->rptr;
			alog->rptr += n;
			if(alog->rptr >= alog->end){
				d = alog->rptr - alog->end;
				alog->rptr = alog->buf + d;
			}
			alog->len -= n;
			unlock(alog);

			i = n-d;
			p = a;
			memmove(p, rptr, i);
			memmove(p+i, alog->buf, d);
			break;
		}
		else
			unlock(alog);

		sleep(&alog->readr, logready, alog);
	}

	qunlock(&alog->readq);
	poperror();

	return n;
}

char*
logctl(Log *alog, int argc, char *argv[], Logflag *flags)
{
	int i, set;
	Logflag *fp;

	if(argc < 2)
		return Ebadlogctl;

	if(strcmp("set", argv[0]) == 0)
		set = 1;
	else if(strcmp("clear", argv[0]) == 0)
		set = 0;
	else
		return Ebadlogctl;

	for(i = 1; i < argc; i++){
		for(fp = flags; fp->name; fp++)
			if(strcmp(fp->name, argv[i]) == 0)
				break;
		if(fp->name == nil)
			continue;
		if(set)
			alog->logmask |= fp->mask;
		else
			alog->logmask &= ~fp->mask;
	}

	return nil;
}

void
log(Log *alog, int mask, char *fmt, ...)
{
	char buf[128], *t, *fp;
	int i, n;
	va_list arg;

	if(!(alog->logmask & mask))
		return;

	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(alog->opens == 0)
		return;

	lock(alog);
	i = alog->len + n - Nlog;
	if(i > 0){
		alog->len -= i;
		alog->rptr += i;
		if(alog->rptr >= alog->end)
			alog->rptr = alog->buf + (alog->rptr - alog->end);
	}
	t = alog->rptr + alog->len;
	fp = buf;
	alog->len += n;
	while(n-- > 0){
		if(t >= alog->end)
			t = alog->buf + (t - alog->end);
		*t++ = *fp++;
	}
	unlock(alog);

	wakeup(&alog->readr);
}
