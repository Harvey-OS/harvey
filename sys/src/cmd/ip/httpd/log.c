#include <u.h>
#include <libc.h>
#include "httpd.h"
#include "httpsrv.h"

int		logall[2];

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
	ulong now, today;
	int logfd;
	va_list arg;

	bufe = buf + sizeof(buf);
	now = time(nil);
	today = now / (24*60*60);
	logfd = logall[today & 1];
	if(c == nil || logfd <= 0)
		return;
	p = c->private;
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
