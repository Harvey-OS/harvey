/*
 * Invoke the named program using the conventional CGI interface.
 *
 * hget http://cgi-spec.golux.com/draft-coar-cgi-v11-03.txt
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "httpd.h"
#include "httpsrv.h"

typedef struct Hline Hline;
struct Hline 
{
	Hline *next;
	char *name;
	char *value;
};

typedef struct Header Header;
struct Header
{
	Hline *first;
	Hline *last;
};

typedef struct Hcgi Hcgi;
struct Hcgi
{
	HConnect *c;
	int argc;
	char **argv;
	char *execname;
	char *pathinfo;
	char *pathtranslated;
	char *scriptname;
	Header hdrin;
	int outputparsing;
};

char Eexec[] = "magic/cgi: exec failed";

void
die(HConnect *c, char *fmt)
{
	if(fmt)
		hfail(c, HInternal);
	postnote(PNGROUP, getpid(), "kill");
	_exits("die");
}

void
addheader(HConnect *c, Header *h, char *n, char *v)
{
	Hline *l;

	l = halloc(c, sizeof(Hline));
	l->name = n;
	l->value = v;
	l->next = nil;
	if(h->last)
		h->last->next = l;
	else
		h->first = l;
	h->last = l;
}

void
parseheaders(HConnect *c, Header *h, char *os, int len)
{
	char *s, *f, *v, *p, *e;

	s = halloc(c, len+1);
	memmove(s, os, len);
	s[len] = '\0';
	for(p=s; p && *p; ){
		f = p;
		while((p = strchr(p, '\n')) != nil){
			if(p > s && p[-1] == '\r')
				p[-1] = ' ';
			if(p[1] != ' ' && p[1] != '\t'){
				*p++ = '\0';
				break;
			}
			*p++ = ' ';
		}
		v = strchr(f, ':');
		if(v == nil)
			continue;
		*v++ = '\0';
		while(*v == ' ' || *v == '\t')
			v++;
		e = v+strlen(v);
		while(e > v && (e[-1]==' ' || e[-1]=='\t'))
			*--e = '\0';
		addheader(c, h, f, v);
	}
}

char*
findheader(Header *h, char *n, char *def)
{
	Hline *l;

	for(l=h->first; l; l=l->next)
		if(cistrcmp(n, l->name) == 0)
			return l->value;
	return def;
}

void
mkargv(Hcgi *hcgi)
{
	int n, nn;
	char *s, *t, **argv, *meth;

	nn = 5;
	if(hcgi->c->req.search != nil)
		nn += strlen(hcgi->c->req.search);

	/* can't possibly have more args than chars */
	argv = halloc(hcgi->c, nn*sizeof(char*));

	n = 0;
	argv[n++] = hcgi->scriptname;

	/*
	 * only pass on cmd line if GET or HEAD and no = in line
	 */
	meth = hcgi->c->req.meth;
	if((strcmp(meth, "GET")==0 || strcmp(meth, "HEAD") == 0)
	&& hcgi->c->req.search
	&& strchr(hcgi->c->req.search, '=') == nil){
		s = hstrdup(hcgi->c, hcgi->c->req.search);
		for(t=s; *t; t++)
			if(*t == '+')
				*t = ' ';
		s = hurlunesc(hcgi->c, s);
		n += getfields(s, argv+n, nn-n, 1, " \t\r\n\v");
	}
	hcgi->argc = n;
	hcgi->argv = argv;
}

void
mkenv(Hcgi *hcgi)
{
	char *s, *t;
	Hline *l;
	Header *h;
	HSPriv *hp;

	rfork(RFCENVG);
	h = &hcgi->hdrin;

	putenv("AUTH_TYPE", "");	/* BUG */
	putenv("CONTENT_LENGTH", findheader(h, "content-length", ""));
	putenv("CONTENT_TYPE", findheader(h, "content-type", ""));
	putenv("GATEWAY_INTERFACE", "CGI/1.1");

	/* http header Foo: Bar becomes HTTP_FOO=Bar */
	for(l=hcgi->hdrin.first; l; l=l->next){
		s = halloc(hcgi->c, 5+strlen(l->name)+1);
		strcpy(s, "HTTP_");
		strcat(s, l->name);
		for(t=s; *t; t++)
			if(islower(*t))
				*t += 'A' - 'a';
			else if(*t == '-')
				*t = '_';
		putenv(s, l->value);
	}

	putenv("PATH_INFO", hcgi->pathinfo);
	putenv("PATH_TRANSLATED", hcgi->pathtranslated);
	if(hcgi->c->req.search)
		putenv("QUERY_STRING", hcgi->c->req.search);
	hp = hcgi->c->private;
	putenv("REMOTE_ADDR", hp->remotesys);
	putenv("REQUEST_METHOD", hcgi->c->req.meth);
	putenv("SCRIPT_NAME", hcgi->scriptname);

	/*
	 * not clear which is right:
	 * use urihost, or Host: header, or domain
	 */
	if(hcgi->c->req.urihost)
		putenv("SERVER_NAME", hcgi->c->req.urihost);
	else
		putenv("SERVER_NAME", findheader(h, "host", hmydomain));

	putenv("SERVER_PORT", "80");		/* BUG */
	putenv("SERVER_PROTOCOL", hversion);
	putenv("SERVER_SOFTWARE", "plan9httpd");
}

void
mkpath(Hcgi *hcgi)
{
	char *p;

	hcgi->scriptname = hstrdup(hcgi->c, hcgi->c->req.uri);	/* actually just the path */
	if(hcgi->scriptname[0] != '/')
		die(hcgi->c, "internal error");
	p = strchr(hcgi->scriptname+1, '/');
	if(p == nil)
		hcgi->pathinfo = "";
	else{
		hcgi->pathinfo = hstrdup(hcgi->c, p);
		*p = '\0';
	}

	/*
	 * since the script name contains no slashes except the beginning one,
	 * it's safe.  at worst it's "/.." or "/.", which will fail when we exec.
	 */
	hcgi->execname = smprint("/bin/ip/httpd/cgi-bin%s", hcgi->scriptname);
	if(hcgi->execname == nil)
		die(hcgi->c, "out of memory");

	/* BUG: should deal with pathtranslated */
	hcgi->pathtranslated = "";
}

void
printheader(Hio *h, Header *hdr)
{
	Hline *l;

	for(l=hdr->first; l; l=l->next)
		hprint(h, "%s: %s\r\n", l->name, l->value);
}

void
parseoutput(Hcgi *hcgi, int fd)
{
	char *s;
	HConnect tmp;
	Header h;
	Hio *hout;
	int redirect;

	/* fake a connection using fd and read headers into buffer */
	memset(&tmp, 0, sizeof tmp);
	tmp.hstop = tmp.header;
	hinit(&tmp.hin, fd, Hread);
	if(hgethead(&tmp, 1) < 0)
		die(hcgi->c, nil);
	parseheaders(hcgi->c, &h, (char*)tmp.header, tmp.hstop - tmp.header);

	hout = &hcgi->c->hout;
	/* must have location or status */
	redirect = 0;
	if(findheader(&h, "location", nil)){
		redirect = 1;
		hprint(hout, "%s 302 Redirect\r\n", hversion);
	}else if(s = findheader(&h, "status", nil))
		hprint(hout, "%s %s\r\n", hversion, s);
	else
		hprint(hout, "%s 200 OK\r\n", hversion);

	printheader(hout, &h);
	/*
	 * maybe there are other fields to add if we're not redirecting?
	 */
	USED(redirect);

	hprint(hout, "\r\n");

	for(; s = hreadbuf(&tmp.hin, tmp.hin.pos); tmp.hin.pos = tmp.hin.stop)
		hwrite(hout, s, hbuflen(&tmp.hin, s));

	hflush(hout);
}

void
main(int argc, char **argv)
{
	char *s;
	int i, infd, outfd, logfd, len, p[2], pid;
	Hcgi hcgi;
	HConnect *c;
	Hio *hin, *hout;
	Waitmsg *w;

	rfork(RFNOTEG);

	c = init(argc, argv);
	hout = &c->hout;

	if(hparseheaders(c, 15*60*1000) < 0)
		exits("failed");

	/* what do these do? */
	if(c->head.expectother){
		hfail(c, HExpectFail, nil);
		exits("failed");
	}
	if(c->head.expectcont){
		hprint(hout, "100 Continue\r\n");
		hprint(hout, "\r\n");
		hflush(hout);
	}

	memset(&hcgi, 0, sizeof hcgi);
	hcgi.c = c;
	parseheaders(c, &hcgi.hdrin, (char*)c->header, c->hstop-c->header);
	mkpath(&hcgi);
	hcgi.outputparsing = 1;
	if(strncmp(hcgi.scriptname, "/nph-", 5) == 0)
		hcgi.outputparsing = 0;
	mkargv(&hcgi);

	/*
	 * Set up input.  Pipe posted data, or just /dev/null.
	 */
	if(strcmp(c->req.meth, "POST") == 0){
		if(pipe(p) < 0)
			die(c, "pipe: %r");
		hin = hbodypush(&c->hin, c->head.contlen, c->head.transenc);
		if(hin == nil)
			die(c, "bad transfer encoding");

		switch(rfork(RFPROC|RFFDG|RFNOWAIT)){
		case -1:
			die(c, "fork: %r");

		case 0:
			close(p[0]);
			for(; s = hreadbuf(hin, hin->pos); hin->pos = hin->stop){
				len = hbuflen(hin, s);
				if(write(p[1], s, len) != len)
					die(c, nil);
			}
			_exits(nil);

		default:
			close(p[1]);
			close(hin->fd);
			infd = p[0];
			break;
		}
	}else
		infd = open("/dev/null", OREAD);

	/*
	 * Set up stderr.
	 */
	logfd = open("/sys/log/httpd/cgi", OWRITE);
	if(logfd < 0)
		logfd = open("/dev/null", OWRITE);
	seek(logfd, 0, 2);

	/*
	 * Set up output pipe, if any.
	 */
	if(hcgi.outputparsing){
		if(pipe(p) < 0)
			die(c, "pipe: %r");
		outfd = p[1];
	}else
		outfd = c->hout.fd;

	/*
	 * Fork the process
	 */
	switch(pid = fork()){
	case -1:
		die(c, "fork: %r");

	case 0:
		if(hcgi.outputparsing)
			close(p[0]);
		if(infd != 0){
			dup(infd, 0);
			close(infd);
		}
		if(outfd != 1){
			dup(outfd, 1);
			close(outfd);
		}
		dup(logfd, 2);
		close(logfd);
		/*
		 * httpd is a little sloppy.
		 */
		for(i=3; i<20; i++)
			close(i);
		mkenv(&hcgi);

		/*
		 * Not in the spec, but everyone expects the chdir to the cgi-bin dir.
		 */
		chdir("/bin/ip/httpd/cgi-bin");

		exec(hcgi.execname, hcgi.argv);
		_exits(Eexec);
	}

	/*
	 * If we're parsing the output, do that.
	 * Otherwise just wait to see if the exec fails.
	 */	
	if(hcgi.outputparsing){
		close(p[1]);
		parseoutput(&hcgi, p[0]);
	}else{
		if((w = wait()) == nil || w->pid != pid)
			die(c, "wait failed");
		if(strstr(w->msg, Eexec))
			die(c, "exec failed");
	}
}
