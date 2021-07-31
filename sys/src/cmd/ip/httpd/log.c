#include <u.h>
#include <libc.h>
#include "httpd.h"
#include "httpsrv.h"

int		logall[3];  /* logall[2] is in "Common Log Format" */

static char *
monname[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void
logit(HConnect *c, char *fmt, ...)
{
	char buf[4096];
	va_list arg;
	HSPriv *p;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	p = nil;
	if(c != nil)
		p = c->private;
	if(p != nil && p->remotesys != nil)
		syslog(0, HTTPLOG, "%s %s", p->remotesys, buf);
	else
		syslog(0, HTTPLOG, "%s", buf);
}

void
writelog(HConnect *c, char *fmt, ...)
{
	HSPriv *p;
	char buf[HBufSize+500], *bufp, *bufe;
	char statuscode[4];
	vlong objectsize;
	ulong now, today;
	int logfd;
	va_list arg;
	Tm *tm;

	if(c == nil)
		return;
	p = c->private;
	bufe = buf + sizeof(buf);
	now = time(nil);
	tm = gmtime(now);
	today = now / (24*60*60);

	/* verbose logfile, for research on web traffic */
	logfd = logall[today & 1];
	if(logfd > 0){
		if(c->hstop == c->header || c->hstop[-1] != '\n')
			*c->hstop = '\n';
		*c->hstop = '\0';
		bufp = seprint(buf, bufe, "==========\n");
		bufp = seprint(bufp, bufe, "LogTime:  %D\n", now);
		bufp = seprint(bufp, bufe, "ConnTime: %D\n", c->reqtime);
		bufp = seprint(bufp, bufe, "RemoteIP: %s\n", p->remotesys);
		bufp = seprint(bufp, bufe, "Port: %s\n", p->remoteserv);
		va_start(arg, fmt);
		bufp = vseprint(bufp, bufe, fmt, arg);
		va_end(arg);
		if(c->req.uri != nil && c->req.uri[0] != 0)
			bufp = seprint(bufp, bufe, "FinalURI: %s\n", c->req.uri);
		bufp = seprint(bufp, bufe, "----------\n%s\n", (char*)c->header);
		write(logfd, buf, bufp-buf);   /* append-only file */
	}

	/* another log, with less information but formatted for common analysis tools */
	if(logall[2] > 0 && strncmp(fmt, "Reply: ", 7) == 0){
		objectsize = 0;
		strecpy(statuscode, statuscode+4, fmt+7);
		if( fmt[7] == '%'){
			va_start(arg, fmt);
			vseprint(statuscode, statuscode+4, fmt+7, arg);
			va_end(arg);
		}else if(
			strcmp(fmt+7, "200 file %lld %lld\n") == 0 ||
			strcmp(fmt+7, "206 partial content %lld %lld\n") == 0 ||
			strcmp(fmt+7, "206 partial content, early termination %lld %lld\n") == 0){
			va_start(arg, fmt);
			objectsize = va_arg(arg, vlong); /* length in sendfd.c */
			objectsize = va_arg(arg, vlong); /* wrote in sendfd.c */
			va_end(arg);
		}
		bufp = seprint(buf, bufe, "%s - -", p->remotesys);
		bufp = seprint(bufp, bufe, " [%.2d/%s/%d:%.2d:%.2d:%.2d +0000]", tm->mday, monname[tm->mon], tm->year+1900, tm->hour, tm->min, tm->sec);
		if(c->req.uri == nil || c->req.uri[0] == 0){
			bufp = seprint(bufp, bufe, " \"%.*s\"",
				(int)utfnlen((char*)c->header, strcspn((char*)c->header, "\r\n")),
				(char*)c->header);
		}else{
			/* use more canonical form of URI, if available */
			bufp = seprint(bufp, bufe, " \"%s %s HTTP/%d.%d\"", c->req.meth, c->req.uri, c->req.vermaj,  c->req.vermin);
		}
		bufp = seprint(bufp, bufe, " %s %lld\n", statuscode, objectsize);
		write(logall[2], buf, bufp-buf);   /* append-only file */
	}
}
