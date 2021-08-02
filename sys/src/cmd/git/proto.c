#include <u.h>
#include <libc.h>
#include <ctype.h>

#include "git.h"

#define Useragent	"useragent git/2.24.1"
#define Contenthdr	"headers Content-Type: application/x-git-%s-pack-request"
#define Accepthdr	"headers Accept: application/x-git-%s-pack-result"

enum {
	Nproto	= 16,
	Nport	= 16,
	Nhost	= 256,
	Npath	= 128,
	Nrepo	= 64,
	Nbranch	= 32,
};

void
tracepkt(int v, char *pfx, char *b, int n)
{
	char *f;
	int o, i;

	if(chattygit < v)
		return;
	o = 0;
	f = emalloc(n*4 + 1);
	for(i = 0; i < n; i++){
		if(isprint(b[i])){
			f[o++] = b[i];
			continue;
		}
		f[o++] = '\\';
		switch(b[i]){
		case '\\':	f[o++] = '\\';	break;
		case '\n':	f[o++] = 'n';	break;
		case '\r':	f[o++] = 'r';	break;
		case '\v':	f[o++] = 'v';	break;
		case '\0':	f[o++] = '0';	break;
		default:
			f[o++] = 'x';
			f[o++] = "0123456789abcdef"[(b[i]>>4)&0xf];
			f[o++] = "0123456789abcdef"[(b[i]>>0)&0xf];
			break;
		}
	}
	f[o] = '\0';
	fprint(2, "%s %04x:\t%s\n", pfx, n, f);
	free(f);
}

int
readpkt(Conn *c, char *buf, int nbuf)
{
	char len[5];
	char *e;
	int n;

	if(readn(c->rfd, len, 4) == -1)
		return -1;
	len[4] = 0;
	n = strtol(len, &e, 16);
	if(n == 0){
		dprint(1, "=r=> 0000\n");
		return 0;
	}
	if(e != len + 4 || n <= 4)
		sysfatal("pktline: bad length '%s'", len);
	n  -= 4;
	if(n >= nbuf)
		sysfatal("pktline: undersize buffer");
	if(readn(c->rfd, buf, n) != n)
		return -1;
	buf[n] = 0;
	tracepkt(1, "=r=>", buf, n);
	return n;
}

int
writepkt(Conn *c, char *buf, int nbuf)
{
	char len[5];


	snprint(len, sizeof(len), "%04x", nbuf + 4);
	if(write(c->wfd, len, 4) != 4)
		return -1;
	if(write(c->wfd, buf, nbuf) != nbuf)
		return -1;
	tracepkt(1, "<=w=", buf, nbuf);
	return 0;
}

int
flushpkt(Conn *c)
{
	dprint(1, "<=w= 0000\n");
	return write(c->wfd, "0000", 4);
}

static void
grab(char *dst, int n, char *p, char *e)
{
	int l;

	l = e - p;
	if(l >= n)
		sysfatal("overlong component");
	memcpy(dst, p, l);
	dst[l] = 0;
}

static int
parseuri(char *uri, char *proto, char *host, char *port, char *path, char *repo)
{
	char *s, *p, *q;
	int n, hasport;
	print("uri: \"%s\"\n", uri);

	p = strstr(uri, "://");
	if(p == nil)
		snprint(proto, Nproto, "ssh");
	else if(strncmp(uri, "git+", 4) == 0)
		grab(proto, Nproto, uri + 4, p);
	else
		grab(proto, Nproto, uri, p);
	*port = 0;
	hasport = 1;
	if(strcmp(proto, "git") == 0)
		snprint(port, Nport, "9418");
	else if(strncmp(proto, "https", 5) == 0)
		snprint(port, Nport, "443");
	else if(strncmp(proto, "http", 4) == 0)
		snprint(port, Nport, "80");
	else if(strncmp(proto, "hjgit", 5) == 0)
		snprint(port, Nport, "17021");
	else if(strncmp(proto, "gits", 5) == 0)
		snprint(port, Nport, "9419");
	else
		hasport = 0;
	s = (p != nil) ? p + 3 : uri;
	p = nil;
	if(!hasport){
		p = strstr(s, ":");
		if(p != nil)
			p++;
	}
	if(p == nil)
		p = strstr(s, "/");
	if(p == nil || strlen(p) == 1){
		werrstr("missing path");
		return -1;
	}

	q = memchr(s, ':', p - s);
	if(q){
		grab(host, Nhost, s, q);
		grab(port, Nport, q + 1, p);
	}else{
		grab(host, Nhost, s, p);
	}
	
	snprint(path, Npath, "%s", p);
	if((q = strrchr(p, '/')) != nil)
		p = q + 1;
	if(strlen(p) == 0){
		werrstr("missing repository in uri");
		return -1;
	}
	n = strlen(p);
	if(hassuffix(p, ".git"))
		n -= 4;
	grab(repo, Nrepo, p, p + n);
	return 0;
}

static int
webclone(Conn *c, char *url)
{
	char buf[16];
	int n, conn;

	if((c->cfd = open("/mnt/web/clone", ORDWR)) < 0)
		goto err;
	if((n = read(c->cfd, buf, sizeof(buf)-1)) == -1)
		goto err;
	buf[n] = 0;
	conn = atoi(buf);

	/* github will behave differently based on useragent */
	if(write(c->cfd, Useragent, sizeof(Useragent)) == -1)
		return -1;
	dprint(1, "open url %s\n", url);
	if(fprint(c->cfd, "url %s", url) == -1)
		goto err;
	free(c->dir);
	c->dir = smprint("/mnt/web/%d", conn);
	return 0;
err:
	if(c->cfd != -1)
		close(c->cfd);
	return -1;
}

static int
webopen(Conn *c, char *file, int mode)
{
	char path[128];
	int fd;

	snprint(path, sizeof(path), "%s/%s", c->dir, file);
	if((fd = open(path, mode)) == -1)
		return -1;
	return fd;
}

static int
issmarthttp(Conn *c, char *direction)
{
	char buf[Pktmax+1], svc[128];
	int n;

	if((n = readpkt(c, buf, sizeof(buf))) == -1)
		sysfatal("http read: %r");
	buf[n] = 0;
	snprint(svc, sizeof(svc), "# service=git-%s-pack\n", direction);
	if(strncmp(svc, buf, n) != 0){
		werrstr("dumb http protocol not supported");
		return -1;
	}
	if(readpkt(c, buf, sizeof(buf)) != 0){
		werrstr("protocol garble: expected flushpkt");
		return -1;
	}
	return 0;
}

static int
dialhttp(Conn *c, char *host, char *port, char *path, char *direction)
{
	char *geturl, *suff, *hsep, *psep;

	suff = "";
	hsep = "";
	psep = "";
	if(port && strlen(port) != 0)
		hsep = ":";
	if(path && path[0] != '/')
		psep = "/";
	memset(c, 0, sizeof(*c));
	geturl = smprint("https://%s%s%s%s%s%s/info/refs?service=git-%s-pack", host, hsep, port, psep, path, suff, direction);
	c->type = ConnHttp;
	c->url = smprint("https://%s%s%s%s%s%s/git-%s-pack", host, hsep, port, psep, path, suff, direction);
	c->cfd = webclone(c, geturl);
	free(geturl);
	if(c->cfd == -1)
		return -1;
	c->rfd = webopen(c, "body", OREAD);
	c->wfd = -1;
	if(c->rfd == -1)
		return -1;
	if(issmarthttp(c, direction) == -1)
		return -1;
	c->direction = estrdup(direction);
	return 0;
}

static int
dialssh(Conn *c, char *host, char *, char *path, char *direction)
{
	int pid, pfd[2];
	char cmd[64];

	if(pipe(pfd) == -1)
		sysfatal("unable to open pipe: %r");
	pid = fork();
	if(pid == -1)
		sysfatal("unable to fork");
	if(pid == 0){
		close(pfd[1]);
		dup(pfd[0], 0);
		dup(pfd[0], 1);
		snprint(cmd, sizeof(cmd), "git-%s-pack", direction);
		dprint(1, "exec ssh '%s' '%s' %s\n", host, cmd, path);
		execl("/bin/ssh", "ssh", host, cmd, path, nil);
		sysfatal("exec: %r");
	}
	close(pfd[0]);
	c->type = ConnSsh;
	c->rfd = pfd[1];
	c->wfd = dup(pfd[1], -1);
	return 0;
}

static int
githandshake(Conn *c, char *host, char *path, char *direction)
{
	char *p, *e, cmd[512];

	p = cmd;
	e = cmd + sizeof(cmd);
	p = seprint(p, e - 1, "git-%s-pack %s", direction, path);
	if(host != nil)
		p = seprint(p + 1, e, "host=%s", host);
	if(writepkt(c, cmd, p - cmd + 1) == -1){
		fprint(2, "failed to write message\n");
		closeconn(c);
		return -1;
	}
	return 0;
}

static int
dialhjgit(Conn *c, char *host, char *port, char *path, char *direction, int auth)
{
	char *ds;
	int pid, pfd[2];

	if((ds = netmkaddr(host, "tcp", port)) == nil)
		return -1;
	if(pipe(pfd) == -1)
		sysfatal("unable to open pipe: %r");
	pid = fork();
	if(pid == -1)
		sysfatal("unable to fork");
	if(pid == 0){
		close(pfd[1]);
		dup(pfd[0], 0);
		dup(pfd[0], 1);
		dprint(1, "exec tlsclient -a %s\n", ds);
		if(auth)
			execl("/bin/tlsclient", "tlsclient", "-a", ds, nil);
		else
			execl("/bin/tlsclient", "tlsclient", ds, nil);
		sysfatal("exec: %r");
	}
	close(pfd[0]);
	c->type = ConnGit9;
	c->rfd = pfd[1];
	c->wfd = dup(pfd[1], -1);
	return githandshake(c, host, path, direction);
}

void
initconn(Conn *c, int rd, int wr)
{
	c->type = ConnGit;
	c->rfd = rd;
	c->wfd = wr;
}

static int
dialgit(Conn *c, char *host, char *port, char *path, char *direction)
{
	char *ds;
	int fd;

	if((ds = netmkaddr(host, "tcp", port)) == nil)
		return -1;
	dprint(1, "dial %s git-%s-pack %s\n", ds, direction, path);
	fd = dial(ds, nil, nil, nil);
	if(fd == -1)
		return -1;
	c->type = ConnGit;
	c->rfd = fd;
	c->wfd = dup(fd, -1);
	return githandshake(c, host, path, direction);
}

static int
servelocal(Conn *c, char *path, char *direction)
{
	int pid, pfd[2];

	if(pipe(pfd) == -1)
		sysfatal("unable to open pipe: %r");
	pid = fork();
	if(pid == -1)
		sysfatal("unable to fork");
	if(pid == 0){
		close(pfd[1]);
		dup(pfd[0], 0);
		dup(pfd[0], 1);
		execl("/bin/git/serve", "serve", "-w", nil);
		sysfatal("exec: %r");
	}
	close(pfd[0]);
	c->type = ConnGit;
	c->rfd = pfd[1];
	c->wfd = dup(pfd[1], -1);
	return githandshake(c, nil, path, direction);
}

static int
localrepo(char *uri, char *path, int npath)
{
	int fd;

	snprint(path, npath, "%s/.git/../", uri);
	fd = open(path, OREAD);
	if(fd < 0)
		return -1;
	if(fd2path(fd, path, npath) != 0){
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int
gitconnect(Conn *c, char *uri, char *direction)
{
	char proto[Nproto], host[Nhost], port[Nport];
	char repo[Nrepo], path[Npath];

	memset(c, 0, sizeof(Conn));
	c->rfd = c->wfd = c->cfd = -1;

	if(localrepo(uri, path, sizeof(path)) == 0)
		return servelocal(c, path, direction);

	if(parseuri(uri, proto, host, port, path, repo) == -1){
		werrstr("bad uri %s", uri);
		return -1;
	}
	if(strcmp(proto, "ssh") == 0)
		return dialssh(c, host, port, path, direction);
	else if(strcmp(proto, "git") == 0)
		return dialgit(c, host, port, path, direction);
	else if(strcmp(proto, "hjgit") == 0)
		return dialhjgit(c, host, port, path, direction, 1);
	else if(strcmp(proto, "gits") == 0)
		return dialhjgit(c, host, port, path, direction, 0);
	else if(strcmp(proto, "http") == 0 || strcmp(proto, "https") == 0)
		return dialhttp(c, host, port, path, direction);
	werrstr("unknown protocol %s", proto);
	return -1;
}

int
writephase(Conn *c)
{
	char hdr[128];
	int n;

	dprint(1, "start write phase\n");
	if(c->type != ConnHttp)
		return 0;

	if(c->wfd != -1)
		close(c->wfd);
	if(c->cfd != -1)
		close(c->cfd);
	if((c->cfd = webclone(c, c->url)) == -1)
		return -1;
	n = snprint(hdr, sizeof(hdr), Contenthdr, c->direction);
	if(write(c->cfd, hdr, n) == -1)
		return -1;
	n = snprint(hdr, sizeof(hdr), Accepthdr, c->direction);
	if(write(c->cfd, hdr, n) == -1)
		return -1;
	if((c->wfd = webopen(c, "postbody", OWRITE)) == -1)
		return -1;
	c->rfd = -1;
	return 0;
}

int
readphase(Conn *c)
{
	dprint(1, "start read phase\n");
	if(c->type != ConnHttp)
		return 0;
	if(close(c->wfd) == -1)
		return -1;
	if((c->rfd = webopen(c, "body", OREAD)) == -1)
		return -1;
	c->wfd = -1;
	return 0;
}

void
closeconn(Conn *c)
{
	close(c->rfd);
	close(c->wfd);
	switch(c->type){
	case ConnGit:
		break;
	case ConnGit9:
	case ConnSsh:
		free(wait());
		break;
	case ConnHttp:
		close(c->cfd);
		break;
	}
}
