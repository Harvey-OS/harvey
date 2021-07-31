#include <u.h>
#include <libc.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>
#include "httpd.h"
#include "httpsrv.h"

typedef struct Strings		Strings;

struct Strings
{
	char	*s1;
	char	*s2;
};

char	*netdir;
char	*HTTPLOG = "httpd/log";

static	char		netdirb[256];
static	char		*namespace;

static	void		becomenone(char*);
static	char		*csquery(char*, char*, char*);
static	void		dolisten(char*);
static	int		doreq(HConnect*);
static	int		send(HConnect*);
static	Strings		stripmagic(HConnect*, char*);
static	char*		stripprefix(char*, char*);
static	char*		sysdom(void);
static	int		notfound(HConnect *c, char *url);

uchar *certificate;
int certlen;
PEMChain *certchain;	

void
usage(void)
{
	fprint(2, "usage: httpd [-c certificate] [-C CAchain] [-a srvaddress] "
		"[-d domain] [-n namespace] [-w webroot]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *address;

	namespace = nil;
	address = nil;
	hmydomain = nil;
	netdir = "/net";
	fmtinstall('D', hdatefmt);
	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);
	ARGBEGIN{
	case 'c':
		certificate = readcert(EARGF(usage()), &certlen);
		if(certificate == nil)
			sysfatal("reading certificate: %r");
		break;
	case 'C':
		certchain = readcertchain(EARGF(usage()));
		if (certchain == nil)
			sysfatal("reading certificate chain: %r");
		break;
	case 'n':
		namespace = EARGF(usage());
		break;
	case 'a':
		address = EARGF(usage());
		break;
	case 'd':
		hmydomain = EARGF(usage());
		break;
	case 'w':
		webroot = EARGF(usage());
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
		address = "*";
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
	if(hmydomain == nil)
		hmydomain = sysdom();
	syslog(0, HTTPLOG, nil);
	logall[0] = open("/sys/log/httpd/0", OWRITE);
	logall[1] = open("/sys/log/httpd/1", OWRITE);
	logall[2] = open("/sys/log/httpd/clf", OWRITE);
	redirectinit();
	contentinit();
	urlinit();
	statsinit();

	becomenone(namespace);
	dolisten(netmkaddr(address, "tcp", certificate == nil ? "http" : "https"));
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

static HConnect*
mkconnect(char *scheme, char *port)
{
	HConnect *c;

	c = ezalloc(sizeof(HConnect));
	c->hpos = c->header;
	c->hstop = c->header;
	c->replog = writelog;
	c->scheme = scheme;
	c->port = port;
	return c;
}

static HSPriv*
mkhspriv(void)
{
	HSPriv *p;

	p = ezalloc(sizeof(HSPriv));
	return p;
}

static void
dolisten(char *address)
{
	HSPriv *hp;
	HConnect *c;
	NetConnInfo *nci;
	char ndir[NETPATHLEN], dir[NETPATHLEN], *p, *scheme;
	int ctl, nctl, data, t, ok, spotchk;
	TLSconn conn;

	spotchk = 0;
	syslog(0, HTTPLOG, "httpd starting");
	ctl = announce(address, dir);
	if(ctl < 0){
		syslog(0, HTTPLOG, "can't announce on %s: %r", address);
		return;
	}
	strcpy(netdirb, dir);
	p = nil;
	if(netdir[0] == '/'){
		p = strchr(netdirb+1, '/');
		if(p != nil)
			*p = '\0';
	}
	if(p == nil)
		strcpy(netdirb, "/net");
	netdir = netdirb;

	for(;;){

		/*
		 *  wait for a call (or an error)
		 */
		nctl = listen(dir, ndir);
		if(nctl < 0){
			syslog(0, HTTPLOG, "can't listen on %s: %r", address);
			syslog(0, HTTPLOG, "ctls = %d", ctl);
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
			if(data >= 0 && certificate != nil){
				memset(&conn, 0, sizeof(conn));
				conn.cert = certificate;
				conn.certlen = certlen;
				if (certchain != nil)
					conn.chain = certchain;
				data = tlsServer(data, &conn);
				scheme = "https";
			}else
				scheme = "http";
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

			nci = getnetconninfo(ndir, -1);
			c = mkconnect(scheme, nci->lserv);
			hp = mkhspriv();
			hp->remotesys = nci->rsys;
			hp->remoteserv = nci->rserv;
			c->private = hp;

			hinit(&c->hin, 0, Hread);
			hinit(&c->hout, 1, Hwrite);

			/*
			 * serve requests until a magic request.
			 * later requests have to come quickly.
			 * only works for http/1.1 or later.
			 */
			for(t = 15*60*1000; ; t = 15*1000){
				if(hparsereq(c, t) <= 0)
					exits(nil);
				ok = doreq(c);

				hflush(&c->hout);

				if(c->head.closeit || ok < 0)
					exits(nil);

				hreqcleanup(c);
			}
			/* not reached */

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
doreq(HConnect *c)
{
	HSPriv *hp;
	Strings ss;
	char *magic, *uri, *newuri, *origuri, *newpath, *hb;
	char virtualhost[100], logfd0[10], logfd1[10], vers[16];
	int n, nredirect;
	uint flags;

	/*
	 * munge uri for magic
	 */
	uri = c->req.uri;
	nredirect = 0;
	werrstr("");
top:
	if(++nredirect > 10){
		if(hparseheaders(c, 15*60*1000) < 0)
			exits("failed");
		werrstr("redirection loop");
		return hfail(c, HNotFound, uri);
	}
	ss = stripmagic(c, uri);
	uri = ss.s1;
	origuri = uri;
	magic = ss.s2;
	if(magic)
		goto magic;

	/*
	 * Apply redirects.  Do this before reading headers
	 * (if possible) so that we can redirect to magic invisibly.
	 */
	flags = 0;
	if(origuri[0]=='/' && origuri[1]=='~'){
		n = strlen(origuri) + 4 + UTFmax;
		newpath = halloc(c, n);
		snprint(newpath, n, "/who/%s", origuri+2);
		c->req.uri = newpath;
		newuri = newpath;
	}else if(origuri[0]=='/' && origuri[1]==0){
		/* can't redirect / until we read the headers below */
		newuri = nil;
	}else
		newuri = redirect(c, origuri, &flags);

	if(newuri != nil){
		if(flags & Redirsilent) {
			c->req.uri = uri = newuri;
			logit(c, "%s: silent replacement %s", origuri, uri);
			goto top;
		}
		if(hparseheaders(c, 15*60*1000) < 0)
			exits("failed");
		if(flags & Redirperm) {
			logit(c, "%s: permanently moved to %s", origuri, newuri);
			return hmoved(c, newuri);
		} else if (flags & (Redironly | Redirsubord))
			logit(c, "%s: top-level or many-to-one replacement %s",
				origuri, uri);

		/*
		 * try temporary redirect instead of permanent
		 */
		if (http11(c))
			return hredirected(c, "307 Temporary Redirect", newuri);
		else
			return hredirected(c, "302 Temporary Redirect", newuri);
	}

	/*
	 * for magic we exec a new program and serve no more requests
	 */
magic:
	if(magic != nil && strcmp(magic, "httpd") != 0){
		snprint(c->xferbuf, HBufSize, "/bin/ip/httpd/%s", magic);
		snprint(logfd0, sizeof(logfd0), "%d", logall[0]);
		snprint(logfd1, sizeof(logfd1), "%d", logall[1]);
		snprint(vers, sizeof(vers), "HTTP/%d.%d", c->req.vermaj, c->req.vermin);
		hb = hunload(&c->hin);
		if(hb == nil){
			hfail(c, HInternal);
			return -1;
		}
		hp = c->private;
		execl(c->xferbuf, magic, "-d", hmydomain, "-w", webroot,
			"-s", c->scheme, "-p", c->port,
			"-r", hp->remotesys, "-N", netdir, "-b", hb,
			"-L", logfd0, logfd1, "-R", c->header,
			c->req.meth, vers, uri, c->req.search, nil);
		logit(c, "no magic %s uri %s", magic, uri);
		hfail(c, HNotFound, uri);
		return -1;
	}

	/*
	 * normal case is just file transfer
	 */
	if(hparseheaders(c, 15*60*1000) < 0)
		exits("failed");
	if(origuri[0] == '/' && origuri[1] == 0){	
		snprint(virtualhost, sizeof virtualhost, "http://%s/", c->head.host);
		newuri = redirect(c, virtualhost, nil);
		if(newuri == nil)
			newuri = redirect(c, origuri, nil);
		if(newuri)
			return hmoved(c, newuri);
	}
	if(!http11(c) && !c->head.persist)
		c->head.closeit = 1;
	return send(c);
}

static int
send(HConnect *c)
{
	Dir *dir;
	char *w, *w2, *p, *masque;
	int fd, fd1, n, force301, ok;

/*
	if(c->req.search)
		return hfail(c, HNoSearch, c->req.uri);
 */
	if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0)
		return hunallowed(c, "GET, HEAD");
	if(c->head.expectother || c->head.expectcont)
		return hfail(c, HExpectFail);

	masque = masquerade(c->head.host);

	/*
	 * check for directory/file mismatch with trailing /,
	 * and send any redirections.
	 */
	n = strlen(webroot) + strlen(masque) + strlen(c->req.uri) +
		STRLEN("/index.html") + STRLEN("/.httplogin") + 1;
	w = halloc(c, n);
	strcpy(w, webroot);
	strcat(w, masque);
	strcat(w, c->req.uri);

	/*
	 *  favicon can be overridden by hostname.ico
	 */
	if(strcmp(c->req.uri, "/favicon.ico") == 0){
		w2 = halloc(c, n+strlen(c->head.host)+2);
		strcpy(w2, webroot);
		strcat(w2, masque);
		strcat(w2, "/");
		strcat(w2, c->head.host);
		strcat(w2, ".ico");
		if(access(w2, AREAD)==0)
			w = w2;
	}

	/*
	 * don't show the contents of .httplogin
	 */
	n = strlen(w);
	if(strcmp(w+n-STRLEN(".httplogin"), ".httplogin") == 0)
		return notfound(c, c->req.uri);

	fd = open(w, OREAD);
	if(fd < 0 && strlen(masque)>0 && strncmp(c->req.uri, masque, strlen(masque)) == 0){
		// may be a URI from before virtual hosts;  try again without masque
		strcpy(w, webroot);
		strcat(w, c->req.uri);
		fd = open(w, OREAD);
	}
	if(fd < 0)
		return notfound(c, c->req.uri);
	dir = dirfstat(fd);
	if(dir == nil){
		close(fd);
		return hfail(c, HInternal);
	}
	p = strchr(w, '\0');
	if(dir->mode & DMDIR){
		free(dir);
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
		c->req.uri = w + strlen(webroot) + strlen(masque);
		if(force301 && c->req.vermaj){
			close(fd);
			close(fd1);
			return hmoved(c, c->req.uri);
		}
		close(fd);
		fd = fd1;
		dir = dirfstat(fd);
		if(dir == nil){
			close(fd);
			return hfail(c, HInternal);
		}
	}else if(p > w && p[-1] == '/'){
		free(dir);
		close(fd);
		*strrchr(c->req.uri, '/') = '\0';
		return hmoved(c, c->req.uri);
	}

	ok = authorize(c, w);
	if(ok <= 0){
		free(dir);
		close(fd);
		return ok;
	}

	return sendfd(c, fd, dir, nil, nil);
}

static Strings
stripmagic(HConnect *hc, char *uri)
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
		newuri = hstrdup(hc, s);
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

/*
 * couldn't open a file
 * figure out why and return and error message
 */
static int
notfound(HConnect *c, char *url)
{
	c->xferbuf[0] = 0;
	rerrstr(c->xferbuf, sizeof c->xferbuf);
	if(strstr(c->xferbuf, "file does not exist") != nil)
		return hfail(c, HNotFound, url);
	if(strstr(c->xferbuf, "permission denied") != nil)
		return hfail(c, HUnauth, url);
	return hfail(c, HNotFound, url);
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
