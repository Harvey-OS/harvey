#include <u.h>
#include <libc.h>
#include <auth.h>
#include "httpd.h"
#include "can.h"

typedef struct Endpoints	Endpoints;
typedef struct Strings		Strings;

struct Endpoints
{
	char	*lsys;
	char	*lserv;
	char	*rsys;
	char	*rserv;
};

struct Strings
{
	char	*s1;
	char	*s2;
};

/*
 * import from parse.c
 */
extern	Can		*httpcan;

static	char*		abspath(char*, char*);
static	void		becomenone(char*);
static	char		*csquery(char*, char*, char*);
static	void		dolisten(char*);
static	int		getc(Connect*);
static	char*		getword(Connect*);
static	Strings		parseuri(char*);
static	int		send(Connect*);
static	Strings		stripmagic(char*);
static	char*		stripprefix(char*, char*);
static	Strings		stripsearch(char*);
static	char*		sysdom(void);
static	Endpoints*	getendpoints(char*);
static	int		parsereq(Connect *c);

/*
 * these should be done better; see the reponse codes in /lib/rfc/rfc2616 for
 * more info on what should be included.
 */
#define UNAUTHED	"You are not authorized to see this area.\n"
#define NOCONTENT	"No acceptable type of data is available.\n"
#define NOENCODE	"No acceptable encoding of the contents is available.\n"
#define UNMATCHED	"The entity requested does not match the existing entity.\n"
#define BADRANGE	"No bytes are avaible for the range you requested.\n"

void
usage(void)
{
	fprint(2, "usage: httpd [-a srvaddress] [-d domain] [-n namespace] [-w webroot]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *address;

	namespace = nil;
	address = nil;
	mydomain = nil;
	strcpy(netdir, "/net");
	fmtinstall('D', dateconv);
	fmtinstall('H', httpconv);
	fmtinstall('U', urlconv);
	ARGBEGIN{
	case 'n':
		namespace = ARGF();
		break;
	case 'a':
		address = ARGF();
		break;
	case 'd':
		mydomain = ARGF();
		break;
	case 'w':
		webroot = ARGF();
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc)
		usage();

	if(namespace == nil)
		namespace = "/lib/namespace.httpd";
	if(address == nil)
		address = "tcp!*!http";
	if(webroot == nil)
		webroot = "/usr/web";
	else{
		cleanname(webroot);
		if(webroot[0] != '/')
			webroot = "/usr/web";
	}

	switch(rfork(RFNOTEG|RFPROC|RFFDG|RFNAMEG)) {
	case -1:
		sysfatal("fork");
	case 0:
		break;
	default:
		exits(nil);
	}

	/*
	 * open all files we might need before castrating namespace
	 */
	time(nil);
	if(mydomain == nil)
		mydomain = sysdom();
	syslog(0, HTTPLOG, nil);
	logall[0] = open("/sys/log/httpd/0",OWRITE);
	logall[1] = open("/sys/log/httpd/1",OWRITE);
	redirectinit();
	contentinit();
	urlinit();
	statsinit();

	becomenone(namespace);
	dolisten(address);
	exits(nil);
}

static void
becomenone(char *namespace)
{
	int fd;

	fd = open("#c/user", OWRITE);
	if(fd < 0 || write(fd, "none", strlen("none")) < 0)
		sysfatal("can't become none");
	close(fd);
	if(newns("none", nil) < 0)
		sysfatal("can't build normal namespace");
	if(addns("none", namespace) < 0)
		sysfatal("can't build httpd namespace");
}

static Connect*
mkconnect(void)
{
	Connect *c;

	c = ezalloc(sizeof(Connect));
	c->hpos = c->header;
	c->hstop = c->header;
	return c;
}

static void
dolisten(char *address)
{
	Connect *c;
	Endpoints *ends;
	char ndir[NETPATHLEN], dir[NETPATHLEN], *p;
	int ctl, nctl, data;
	int spotchk = 0;

	syslog(0, HTTPLOG, "httpd starting");
	ctl = announce(address, dir);
	if(ctl < 0){
		syslog(0, HTTPLOG, "can't announce on %s: %r", address);
		return;
	}
	strcpy(netdir, dir);
	p = nil;
	if(netdir[0] == '/'){
		p = strchr(netdir+1, '/');
		if(p != nil)
			*p = '\0';
	}
	if(p == nil)
		strcpy(netdir, "/net");

	for(;;){

		/*
		 *  wait for a call (or an error)
		 */
		nctl = listen(dir, ndir);
		if(nctl < 0){
			syslog(0, HTTPLOG, "can't listen on %s: %r", address);
			syslog(0, HTTPLOG, "ctls = %d", ctl);
			for(;;)sleep(1000);
			return;
		}

		/*
		 *  start a process for the service
		 */
		switch(rfork(RFFDG|RFPROC|RFNOWAIT|RFNAMEG)){
		case -1:
			close(nctl);
			continue;
		case 0:
			/*
			 *  see if we know the service requested
			 */
			data = accept(ctl, ndir);
			if(data < 0){
				syslog(0, HTTPLOG, "can't open %s/data: %r", ndir);
				exits(nil);
			}
			dup(data, 0);
			dup(data, 1);
			dup(data, 2);
			close(data);
			close(ctl);
			close(nctl);

			ends = getendpoints(ndir);
			c = mkconnect();
			c->remotesys = ends->rsys;
			c->begin_time = time(nil);

			hinit(&c->hin, 0, Hread);
			hinit(&c->hout, 1, Hwrite);

			parsereq(c);

			exits(nil);
		default:
			close(nctl);
			break;
		}

		if(++spotchk > 50){
			spotchk = 0;
			redirectinit();
			contentinit();
			urlinit();
			statsinit();
		}
	}
}

static int
parsereq(Connect *c)
{
	Strings ss;
	char *vs, *v, *magic, *search, *uri, *origuri, *extra, *newpath;
	char virtualhost[100];
	int t, n, ok;

	if(httpcan != nil){
		logit(c, "starting request with request data allocated");
		fail(c, Internal);
	}

	/*
	 * serve requests until a magic request.
	 * later requests have to come quickly.
	 * only works for http/1.1 or later.
	 */
	for(t = 15*60*1000; ; t = 15*1000){
		alarm(t);
		if(!gethead(c, 0))
			return 0;
		alarm(0);
		c->req.meth = getword(c);
		if(c->req.meth == nil)
			return fail(c, Syntax);
		uri = getword(c);
		if(uri == nil || strlen(uri) == 0)
			return fail(c, Syntax);
		v = getword(c);
		if(v == nil){
			if(strcmp(c->req.meth, "GET") != 0)
				return fail(c, Unimp, c->req.meth);
			c->req.vermaj = 0;
			c->req.vermin = 9;
		}else{
			vs = v;
			if(strncmp(vs, "HTTP/", 5) != 0)
				return fail(c, UnkVers, vs);
			vs += 5;
			c->req.vermaj = strtoul(vs, &vs, 10);
			if(*vs != '.' || c->req.vermaj != 1)
				return fail(c, UnkVers, vs);
			vs++;
			c->req.vermin = strtoul(vs, &vs, 10);
			if(*vs != '\0')
				return fail(c, UnkVers, vs);

			extra = getword(c);
			if(extra != nil)
				return fail(c, Syntax);
		}

		/*
		 * the fragment is not supposed to be sent
		 * strip it 'cause some clients send it
		 */
		origuri = uri;
		uri = strchr(origuri, '#');
		if(uri != nil)
			*uri = 0;

		/*
		 * http/1.1 requires the server to accept absolute
		 * or relative uri's.  convert to relative with an absolute path
		 */
		if(http11(c)){
			ss = parseuri(origuri);
			uri = ss.s1;
			c->req.urihost = ss.s2;
			if(uri == nil)
				return fail(c, BadReq, uri);
			origuri = uri;
		}

		/*
		 * munge uri for search, protection, and magic
		 */
		ss = stripsearch(origuri);
		origuri = ss.s1;
		search = ss.s2;
		uri = urlunesc(origuri);
		uri = abspath(uri, "/");
		if(uri == nil || uri[0] == 0)
			return fail(c, NotFound, "no object specified");
		ss = stripmagic(uri);
		uri = ss.s1;
		magic = ss.s2;

		c->req.uri = uri;
		c->req.search = search;
		if(magic != nil && strcmp(magic, "httpd") != 0)
			break;

		/*
		 * normal case is just file transfer
		 */
		origuri = uri;
		httpheaders(c);
		if(!http11(c) && !c->head.persist)
			c->head.closeit = 1;

		if(origuri[0]=='/' && origuri[1]=='~'){
			n = strlen(origuri) + 4 + UTFmax;
			newpath = halloc(n);
			snprint(newpath, n, "/who/%s", origuri+2);
			c->req.uri = newpath;
			uri = newpath;
		}else if(origuri[0]=='/' && origuri[1]==0){
			snprint(virtualhost, sizeof virtualhost, "http://%s/", c->head.host);
			uri = redirect(virtualhost);
			if(uri == nil)
				uri = redirect(origuri);
		}else
			uri = redirect(origuri);
		if(uri == nil)
			ok = send(c);
		else
			ok = moved(c, uri);

		if(c->head.closeit || ok < 0)
			exits(nil);

		/*
		 * clean up all of the junk from that request
		 */
		hxferenc(&c->hout, 0);
		hflush(&c->hout);
		reqcleanup(c);
	}

	/*
	 * for magic we exec a new program and serve no more requests
	 */
	snprint(c->xferbuf, BufSize, "/bin/ip/httpd/%s", magic);
	writelog(c, "Magic: %s\n", magic);
	execl(c->xferbuf, magic, "-d", mydomain, "-w", webroot, "-r", c->remotesys, "-N", netdir, "-b", hunload(&c->hin),
		c->req.meth, v, uri, search, nil);
	logit(c, "no magic %s uri %s", magic, uri);
	return fail(c, NotFound, origuri);
}

static Strings
parseuri(char *uri)
{
	Strings ss;
	char *urihost, *p;

	urihost = nil;
	if(uri[0] != '/'){
		if(cistrncmp(uri, "http://", 7) != 0){
			ss.s1 = nil;
			ss.s2 = nil;
			return ss;
		}
		uri += 5;	/* skip http: */
	}

	/*
	 * anything starting with // is a host name or number
	 * hostnames constists of letters, digits, - and .
	 * for now, just ignore any port given
	 */
	if(uri[0] == '/' && uri[1] == '/'){
		urihost = uri + 2;
		p = strchr(urihost, '/');
		if(p == nil)
			uri = hstrdup("/");
		else{
			uri = hstrdup(p);
			*p = '\0';
		}
		p = strchr(urihost, ':');
		if(p != nil)
			*p = '\0';
	}

	if(uri[0] != '/' || uri[1] == '/'){
		ss.s1 = nil;
		ss.s2 = nil;
		return ss;
	}

	ss.s1 = uri;
	ss.s2 = lower(urihost);
	return ss;
}

static int
send(Connect *c)
{
	Dir dir;
	char *w, *p;
	int fd, fd1, n, force301, ok;

	if(c->req.search)
		return fail(c, NoSearch, c->req.uri);
	if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0)
		return unallowed(c, "GET, HEAD");
	if(c->head.expectother || c->head.expectcont)
		return fail(c, ExpectFail);

	ok = authcheck(c);
	if(ok <= 0)
		return ok;

	/*
	 * check for directory/file mismatch with trailing /,
	 * and send any redirections.
	 */
	n = strlen(webroot) + strlen(c->req.uri) + STRLEN("/index.html") + 1;
	w = halloc(n);
	strcpy(w, webroot);
	strcat(w, c->req.uri);
	fd = open(w, OREAD);
	if(fd < 0)
		return notfound(c, c->req.uri);
	if(dirfstat(fd, &dir) < 0){
		close(fd);
		return fail(c, Internal);
	}
	p = strchr(w, '\0');
	if(dir.mode & CHDIR){
		if(p > w && p[-1] == '/'){
			strcat(w, "index.html");
			force301 = 0;
		}else{
			strcat(w, "/index.html");
			force301 = 1;
		}
		fd1 = open(w, OREAD);
		if(fd1 < 0){
			close(fd);
			return notfound(c, c->req.uri);
		}
		c->req.uri = w + strlen(webroot);
		if(force301 && c->req.vermaj){
			close(fd);
			close(fd1);
			return moved(c, c->req.uri);
		}
		close(fd);
		fd = fd1;
		if(dirfstat(fd, &dir) < 0){
			close(fd);
			return fail(c, Internal);
		}
	}else if(p > w && p[-1] == '/'){
		close(fd);
		*strrchr(c->req.uri, '/') = '\0';
		return moved(c, c->req.uri);
	}

	if(verbose)
		logit(c, "%s %s %lld", c->req.meth, c->req.uri, dir.length);

	return sendfd(c, fd, &dir);
}

static Strings
stripmagic(char *uri)
{
	Strings ss;
	char *newuri, *prog, *s;

	prog = stripprefix("/magic/", uri);
	if(prog == nil){
		ss.s1 = uri;
		ss.s2 = nil;
		return ss;
	}

	s = strchr(prog, '/');
	if(s == nil)
		newuri = "";
	else{
		newuri = hstrdup(s);
		*s = 0;
		s = strrchr(s, '/');
		if(s != nil && s[1] == 0)
			*s = 0;
	}
	ss.s1 = newuri;
	ss.s2 = prog;
	return ss;
}

static char*
stripprefix(char *pre, char *str)
{
	while(*pre)
		if(*str++ != *pre++)
			return nil;
	return str;
}

static Strings
stripsearch(char *uri)
{
	Strings ss;
	char *search;

	search = strchr(uri, '?');
	if(search != nil)
		*search++ = 0;
	ss.s1 = uri;
	ss.s2 = search;
	return ss;
}

/*
 *  to circumscribe the accessible files we have to eliminate ..'s
 *  and resolve all names from the root.
 */
static char*
abspath(char *origpath, char *curdir)
{
	char *p, *sp, *path, *work, *rpath;
	int len, n, c;

	if(curdir == nil)
		curdir = "/";
	if(origpath == nil)
		origpath = "";
	work = hstrdup(origpath);
	path = work;

	/*
	 * remove any really special characters
	 */
	for(sp = "`;| "; *sp; sp++){
		p = strchr(path, *sp);
		if(p)
			*p = 0;
	}

	len = strlen(curdir) + strlen(path) + 2 + UTFmax;
	if(len < 10)
		len = 10;
	rpath = halloc(len);
	if(*path == '/')
		rpath[0] = 0;
	else
		strcpy(rpath, curdir);
	n = strlen(rpath);

	while(path){
		p = strchr(path, '/');
		if(p)
			*p++ = 0;
		if(strcmp(path, "..") == 0){
			while(n > 1){
				n--;
				c = rpath[n];
				rpath[n] = 0;
				if(c == '/')
					break;
			}
		} else if(strcmp(path, ".") == 0)
			;
		else if(n == 1)
			n += snprint(rpath+n, len-n, "%s", path);
		else
			n += snprint(rpath+n, len-n, "/%s", path);
		path = p;
	}

	if(strncmp(rpath, "/bin/", 5) == 0)
		strcpy(rpath, "/");
	return rpath;
}

static char*
getword(Connect *c)
{
	char buf[MaxWord];
	int ch, n;

	while((ch = getc(c)) == ' ' || ch == '\t' || ch == '\r')
		;
	buf[0] = '\0';
	if(ch == '\n')
		return nil;
	n = 0;
	for(;;){
		switch(ch){
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			buf[n] = '\0';
			return hstrdup(buf);
		}

		if(n < MaxWord-1)
			buf[n++] = ch;
		ch = getc(c);
	}
	return nil;
}

static int
getc(Connect *c)
{
	if(c->hpos < c->hstop)
		return *c->hpos++;
	return '\n';
}

static char*
sysdom(void)
{
	char *dn;

	dn = csquery("sys" , sysname(), "dom");
	if(dn == nil)
		dn = "who cares";
	return dn;
}

/*
 *  query the connection server
 */
static char*
csquery(char *attr, char *val, char *rattr)
{
	char token[64+4];
	char buf[256], *p, *sp;
	int fd, n;

	if(val == nil || val[0] == 0)
		return nil;
	snprint(buf, sizeof(buf), "%s/cs", netdir);
	fd = open(buf, ORDWR);
	if(fd < 0)
		return nil;
	fprint(fd, "!%s=%s", attr, val);
	seek(fd, 0, 0);
	snprint(token, sizeof(token), "%s=", rattr);
	for(;;){
		n = read(fd, buf, sizeof(buf)-1);
		if(n <= 0)
			break;
		buf[n] = 0;
		p = strstr(buf, token);
		if(p != nil && (p == buf || *(p-1) == 0)){
			close(fd);
			sp = strchr(p, ' ');
			if(sp)
				*sp = 0;
			p = strchr(p, '=');
			if(p == nil)
				return nil;
			return estrdup(p+1);
		}
	}
	close(fd);
	return nil;
}

static void
getendpoint(char *dir, char *file, char **sysp, char **servp)
{
	int fd, n;
	char buf[128];
	char *sys, *serv;

	sys = serv = nil;

	snprint(buf, sizeof buf, "%s/%s", dir, file);
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv){
				*serv++ = 0;
				serv = estrdup(serv);
			}
			sys = estrdup(buf);
		}
		close(fd);
	}
	if(serv == nil)
		serv = estrdup("unknown");
	if(sys == nil)
		sys = estrdup("unknown");
	*servp = serv;
	*sysp = sys;
}

static Endpoints*
getendpoints(char *dir)
{
	Endpoints *ep;

	ep = ezalloc(sizeof(*ep));
	getendpoint(dir, "local", &ep->lsys, &ep->lserv);
	getendpoint(dir, "remote", &ep->rsys, &ep->rserv);
	return ep;
}
