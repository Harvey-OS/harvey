#include "dat.h"

void
logbufproc(Logbuf *lb)
{
	char *s;
	int n;
	Req *r;

	while(lb->wait && lb->rp != lb->wp){
		r = lb->wait;
		lb->wait = r->aux;
		if(lb->wait == nil)
			lb->waitlast = &lb->wait;
		r->aux = nil;
		if(r->ifcall.count < 5){
			respond(r, "factotum: read request count too short");
			continue;
		}
		s = lb->msg[lb->rp];
		lb->msg[lb->rp] = nil;
		if(++lb->rp == nelem(lb->msg))
			lb->rp = 0;
		n = r->ifcall.count;
		if(n < strlen(s)+1+1){
			memmove(r->ofcall.data, s, n-5);
			n -= 5;
			r->ofcall.data[n] = '\0';
			/* look for first byte of UTF-8 sequence by skipping continuation bytes */
			while(n>0 && (r->ofcall.data[--n]&0xC0)==0x80)
				;
			strcpy(r->ofcall.data+n, "...\n");
		}else{
			strcpy(r->ofcall.data, s);
			strcat(r->ofcall.data, "\n");
		}
		r->ofcall.count = strlen(r->ofcall.data);
		free(s);
		respond(r, nil);
	}
}

void
logbufread(Logbuf *lb, Req *r)
{
	if(lb->waitlast == nil)
		lb->waitlast = &lb->wait;
	*(lb->waitlast) = r;
	lb->waitlast = &r->aux;
	r->aux = nil;
	logbufproc(lb);
}

void
logbufflush(Logbuf *lb, Req *r)
{
	Req **l;

	for(l=&lb->wait; *l; l=&(*l)->aux){
		if(*l == r){
			*l = r->aux;
			r->aux = nil;
			if(*l == nil)
				lb->waitlast = l;
			respond(r, "interrupted");
			break;
		}
	}
}

void
logbufappend(Logbuf *lb, char *buf)
{
	if(debug)
		fprint(2, "%s\n", buf);

	if(lb->msg[lb->wp])
		free(lb->msg[lb->wp]);
	lb->msg[lb->wp] = estrdup9p(buf);
	if(++lb->wp == nelem(lb->msg))
		lb->wp = 0;
	logbufproc(lb);
}

Logbuf logbuf;

void
logread(Req *r)
{
	logbufread(&logbuf, r);
}

void
logflush(Req *r)
{
	logbufflush(&logbuf, r);
}

void
flog(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);
	logbufappend(&logbuf, buf);
}

