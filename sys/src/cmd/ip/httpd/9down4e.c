#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include <libsec.h>
#include <auth.h>
#include "httpd.h"
#include "httpsrv.h"
#include "whois.h"

static	Hio	houtb;
static	Hio	*hout;
static	HConnect	*connect;

static	char	me[] = "/magic/9down4e/";

typedef struct Cap Cap;
struct Cap
{
	uchar	pad1[4];
	uchar	ip[IPaddrlen];
	uchar	exp[4];
	uchar	pad2[4];
};

enum
{
	MNAMELEN	= 64
};

char*	mkcap(long);
char*	getcap(HConnect*);
int	capok(char*);
char*	dnsreverse(char*);
void	filter(char*, char*, char*, char*, char*);
int	agreed(char*);
char*	getval(char*, char*, char*, int);
char*	mkdiskette(char*, char*);
HRange*	myfixrange(HConnect*, Dir*, int);

typedef struct Page Page;
struct Page
{
	char	*uri;
	void	(*fn)(HConnect*, char**);
	int	check;
};

void	doreg(HConnect*, char**);
void	doconf(HConnect*, char**);
void	dodisk(HConnect*, char**);
void	doconfdisk(HConnect*, char**);
void	docompressed(HConnect*, char**);
void	dosources(HConnect*, char**);

Page pages[] = {
	{ "reg",	doreg,	0	},
	{ "ureg",	doreg,	0	},
	{ "conf",	doconf,	1	},
	{ "disk",	dodisk,	1	},
	{ "confdisk",	doconfdisk,	1	},
	{ "compressed",	docompressed,	1	},
	{ "sources",	dosources,	0	},
	{ 0, 0 }
};

#define LOG "9down4e"

void
main(int argc, char **argv)
{
	int fd;
	HSPriv *hp;
	Page *pg;
	char *uri;
	char *uriparts[3];

	if((fd = open("#s/fossil.dist", ORDWR)) >= 0)
		mount(fd, -1, "/sys/lib/dist/web.protect", MBEFORE, "");

	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);

	connect = init(argc, argv);
	hout = &connect->hout;
	if(hparseheaders(connect, HSTIMEOUT) < 0)
		exits("failed");

	if(strcmp(connect->req.meth, "GET") != 0 && strcmp(connect->req.meth, "HEAD") != 0){
		hunallowed(connect, "GET, HEAD");
		exits("unallowed");
	}
	if(connect->head.expectother || connect->head.expectcont){
		hfail(connect, HExpectFail, nil);
		exits("failed");
	}

	hp = connect->private;
	syslog(0, LOG, "rs %s ver %d.%d uri %s search %s",
		hp->remotesys,
		connect->req.vermaj, connect->req.vermin,
		connect->req.uri, connect->req.search);

	/* find the current page */
	pg = nil;
	uri = strdup(connect->req.uri);
	memset(uriparts, 0, sizeof(uriparts));
	getfields(uri, uriparts, 3, 1, "/");
	if(uriparts[0] != nil)
		for(pg = pages; pg->uri != 0; pg++)
			if(strcmp(pg->uri, uriparts[0]) == 0)
				break;

//	syslog(0, LOG, "rs %s pg %s",
//		hp->remotesys, pg->uri);
//
	if(pg->uri == nil){
		filter("/sys/lib/dist/web/err/notfound.html", nil, nil, nil, nil);
		hflush(&connect->hout);
		exits(nil);
	}

	if(pg->check){
		if(uriparts[1] == nil || !capok(uriparts[1])){
			filter("/sys/lib/dist/web/err/badcap.html", nil, nil, nil, nil);
			hflush(&connect->hout);
			exits(nil);
		}
	}

	(*pg->fn)(connect, uriparts);
	hflush(&connect->hout);

	exits(nil);
}

char *class[] =
{
[Cok]		"ok",
[Cunknown]	"unknown",
[Cbadc]		"disallowed country",
[Cbadgov]	"disallowed government",
};

void
logreg(char *rv, char *ip, Ndbtuple *t)
{
	char buf[2048];
	char *p, *e;

	p = buf;
	e = p + sizeof(buf);

	p = seprint(p, e, "ip=%s status=\"%s\" ", ip, rv);
	for(;t != nil; t = t->entry){
		p = seprint(p, e, "%s=", t->attr);
		if(strchr(t->val, ' ') != nil)
			p = seprint(p, e, "\"%s\"", t->val);
		else
			p = seprint(p, e, "%s", t->val);
		if(t->entry != t->line){
			if(t->entry == nil)
				break;
			p = seprint(p, e, " | ");
		} else {
			p = seprint(p, e, " ");
		}
	}
	syslog(0, LOG, "%s", buf);
}

void
doreg(HConnect *c, char **uriparts)
{
	HSPriv *hp;
	char *cap, ncap[512];
	char b[128];
	Ndbtuple *t;

	hp = c->private;
	/* first page, check source */
	t = whois("/net.alt", hp->remotesys);
	getval(c->req.search, "cap", b, sizeof(b));
	if(capok(b))
		goto OK;
	switch(classify(hp->remotesys, t)){
	case Cok:
		break;
	case Cunknown:
		logreg("unknown", hp->remotesys, t);
		goto reject;
	case Cbadc:
		logreg("badcountry", hp->remotesys, t);
		goto reject;
	case Cbadgov:
		logreg("badgov", hp->remotesys, t);
		goto reject;
	reject:
		ndbfree(t);
		filter("/sys/lib/dist/web/err/prohibited.html", nil, nil, nil, nil);
		return;
	}
/*
	if(strcmp(getval(c->req.search, "haveread", a, sizeof(a)), "yes") != 0){
		logreg("not read license", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/didntagree.html", nil, nil, nil, nil);
		return;
	}
*/
	if(strcmp(getval(c->req.search, "exportok", b, sizeof(b)), "yes") != 0){
		logreg("export not ok", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badexport.html", nil, nil, nil, nil);
		return;
	}
	if(0 && strcmp(getval(c->req.search, "name", b, sizeof(b)), "liz.bimmler") != 0
	&& strcmp(b, "liz.bimmler") != 0){
		logreg("not bell labs", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/notlucent.html", nil, nil, nil, nil);
		return;
	}

OK:
	logreg("ok", hp->remotesys, t);
	ndbfree(t);
	cap = mkcap(time(0));
	if(strcmp(getval(c->req.search, "name", b, sizeof(b)), "rabbit!bimmler")==0
	|| strcmp(getval(c->req.search, "name", b, sizeof(b)), "rabbit%21bimmler")==0)
		snprint(ncap, sizeof ncap, "n%s", cap);
	else
		strcpy(ncap, cap);
	if(strcmp(uriparts[0], "ureg") == 0)
		filter("/sys/lib/dist/web/update.html", "XYZZY", ncap, nil, nil);
	else
		filter("/sys/lib/dist/web/conf.html", "XYZZY", ncap, nil, nil);
	free(cap);
}

void
dosources(HConnect *c, char **)
{
	HSPriv *hp;
	char a[10], b[128];
	Ndbtuple *t;
	char user[64];
	char pass[64];
	int fd, n;
	AuthInfo *ai;
	char *userp, *passp;

	hp = c->private;
	/* first page, check source */
	t = whois("/net.alt", hp->remotesys);
	getval(c->req.search, "cap", b, sizeof(b));
	if(capok(b))
		goto OK;
	switch(classify(hp->remotesys, t)){
	case Cok:
		break;
	case Cunknown:
		logreg("unknown", hp->remotesys, t);
		goto reject;
	case Cbadc:
		logreg("badcountry", hp->remotesys, t);
		goto reject;
	case Cbadgov:
		logreg("badgov", hp->remotesys, t);
		goto reject;
	reject:
		ndbfree(t);
		filter("/sys/lib/dist/web/err/prohibited.html", nil, nil, nil, nil);
		return;
	}
	if(strcmp(getval(c->req.search, "haveread", a, sizeof(a)), "yes") != 0){
		logreg("not read license", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/didntagree.html", nil, nil, nil, nil);
		return;
	}
	if(strcmp(getval(c->req.search, "exportok", b, sizeof(b)), "yes") != 0){
		logreg("export not ok", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badexport.html", nil, nil, nil, nil);
		return;
	}

	/* check user name and password for size */
	getval(connect->req.search, "user", user, sizeof(user));
	userp = hurlunesc(connect, user);
	n = strlen(user);
	if(n < 3 || n >= 287){
		logreg("bad user name", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badsources.html", "XYZZY",
			"user name must be 3 to 27 characters", nil, nil);
		return;
	}
	getval(connect->req.search, "pass", pass, sizeof(pass));
	passp = hurlunesc(connect, pass);
	n = strlen(passp);
	if(n < 8 || n >= 28){
		logreg("bad password", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badsources.html", "XYZZY",
			"password name must be 8 to 27 characters", nil, nil);
		return;
	}

syslog(0, "sources", "newaccount %s %s(%s)", user, passp, pass);

	/* call up martha to make the account */
	fd = dial("/net.alt/tcp!sources.cs.bell-labs.com!1111", 0, 0, 0);
	if(fd < 0)
		fd = dial("tcp!sources.cs.bell-labs.com!1111", 0, 0, 0);
	if(fd < 0){
		logreg("server down", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badsources.html", "XYZZY",
			"server down: try again later", nil, nil);
		return;
	}

	/* authenticate */
	ai = auth_proxy(fd, nil, "proto=p9any role=client");
	if(ai == nil){
		logreg("authentication failed", hp->remotesys, t);
		ndbfree(t);
		snprint(b, sizeof b, "authenticating to sources: %r");
		filter("/sys/lib/dist/web/err/badsources.html", "XYZZY",
			b, nil, nil);
		return;
	}

	/* rpc */
	fprint(fd, "%s\n%s\n", userp, passp);
	n = read(fd, user, sizeof(user)-1);
	if(n <= 0){
		logreg("server down", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badsources.html", "XYZZY",
			"server down: try again later", nil, nil);
		return;
	}
	user[n] = 0;
	if(strncmp(user, "OK", 2) != 0){
		logreg("bad password", hp->remotesys, t);
		ndbfree(t);
		filter("/sys/lib/dist/web/err/badsources.html", "XYZZY",
			user, nil, nil);
		return;
	}

OK:
	logreg("ok", hp->remotesys, t);
	ndbfree(t);
	filter("/sys/lib/dist/web/oksources.html", nil, nil, nil, nil);
}

void
doconf(HConnect *c, char **uriparts)
{
	char *disk;
	char buf[256];

	disk = mkdiskette(uriparts[1], c->req.search);
	if(disk == nil){
		filter("/sys/lib/dist/web/err/whoops.html", nil, nil, nil, nil);
		return;
	}
	snprint(buf, sizeof(buf), "%s/%s/9disk.flp", uriparts[1], disk);
	free(disk);
	filter("/sys/lib/dist/web/disk.html", "YZZYX", buf, "XYZZY", uriparts[1]);
}

char *magicstr = "THIS IS A 512 byte BLANK PLAN9.INI";

void
dodisk(HConnect *c, char **uriparts)
{
	char buf[512];
	char ini[512];
	int m, n, fd, len;
	char *p;
	Dir *d;
	HRange *r;
	ulong rem;
	char *reply;
	char *file;

	if(uriparts[2] == nil || strstr(uriparts[2], "..") != nil){
		syslog(0, LOG, "bad uriparts[2] %s", uriparts[2]);
		goto err;
	}

	file = "/sys/lib/dist/web.protect/disk";
	if(uriparts[1][0]=='n')
		file = "/sys/lib/dist/web.protect/ndisk";

	/* get rid of the /9disk.flp */
	p = strchr(uriparts[2], '/');
	if(p != nil){
		*p = 0;
	}

	/* stop weird loops */
	snprint(buf, sizeof(buf), "/usr/none/pcdist/tmp/count.%s", uriparts[2]);
	if((fd = create(buf, OWRITE, DMAPPEND|0666)) >= 0 && (d = dirfstat(fd)) != nil){
		if(d->length >= 30){
			remove(buf);
			snprint(buf, sizeof(buf), "/usr/none/pcdist/tmp/%s", uriparts[2]);
			remove(buf);
		}
		free(d);
	}
	if(fd >= 0)
		close(fd);

	/* read in the generated plan9.ini */
	snprint(buf, sizeof(buf), "/usr/none/pcdist/tmp/%s", uriparts[2]);
	fd = open(buf, OREAD);
	if(fd < 0){
		syslog(0, LOG, "dodisk: open %s: %r", buf);
		goto err;
	}
	n = read(fd, ini, sizeof(ini));
	close(fd);
	if(n <= 0){
		syslog(0, LOG, "dodisk: %r");
		goto err;
	}

	/* read out the disk replacing the fake plan9.ini */
	fd = open(file, OREAD);
	if(fd < 0){
		syslog(0, LOG, "dodisk: open %s: %r", file);
		goto err;
	}

	d = dirfstat(fd);
	if(d == nil){
		syslog(0, LOG, "dodisk: %r");
		goto err;
	}

	r = myfixrange(c, d, 1);

	hflush(hout);
	if(r){
		reply = "206";
		seek(fd, r->start, 0);
		rem = r->stop-r->start+1;
	} else {
		reply = "200";
		rem = d->length;
	}
	len = strlen(magicstr);
	for(; rem > 0; rem -= m){
		m = read(fd, buf, sizeof(buf));
		if(m <= 0)
			break;
		if(strncmp(magicstr, buf, len) == 0)
			memmove(buf, ini, n);
		if(m > rem)
			m = rem;
		if(hwrite(hout, buf, m) != m)
			break;
	}
	hflush(hout);
	close(fd);
	writelog(c, "Reply: %s\n", reply);
	return;

err:
	filter("/sys/lib/dist/web/err/whoops.html", nil, nil, nil, nil);
	return;
}

void
doconfdisk(HConnect *c, char **uriparts)
{
	char *disk;
	char *rest;
	char *p;

	if(rest = strchr(c->req.search, '/'))
		*rest++ = '\0';
	else
		rest = nil;

	disk = mkdiskette(uriparts[1], c->req.search);
	if(disk == nil){
		filter("/sys/lib/dist/web/err/whoops.html", nil, nil, nil, nil);
		return;
	}
	uriparts[2] = disk;
	if(rest && (p = malloc(strlen(disk)+1+strlen(rest)+1))){
		strcpy(p, disk);
		strcat(p, "/");
		strcat(p, rest);
		uriparts[2] = p;
	}
	dodisk(c, uriparts);
	free(disk);
}

void
docompressed(HConnect *c, char **uriparts)
{
	HSPriv *hp;
	char buf[512];
	int m, fd;
	Dir *d;
	HRange *r, rr;
	ulong rem;
	char *reply;

	if(uriparts[2] == nil || strstr(uriparts[2], "..") != nil || strchr(uriparts[2], '/') != 0)
		goto err;

	/* open the compressed file */
	snprint(buf, sizeof(buf), "/sys/lib/dist/web.protect/%s%s", uriparts[1][0]=='n' ? "n" : "",
		uriparts[2]);
	fd = open(buf, OREAD);
	if(fd < 0){
		hprint(hout, "%s 404 Not Found\r\n", hversion);
		writelog(c, "Reply: 404 Not Found\n");
		return;
	}

	d = dirfstat(fd);
	if(d == nil)
		goto err;

	r = myfixrange(c, d, 0);

	hflush(hout);
	if(r){
		reply = "206";
		if(seek(fd, r->start, 0) < 0)
			syslog(0, LOG, "seek: %r");
		rem = r->stop-r->start+1;
	} else {
		reply = "200";
		rr.start = 0;
		rr.stop = d->length-1;
		r = &rr;
		rem = d->length;
	}
	for(; rem > 0; rem -= m){
		m = read(fd, buf, sizeof(buf));
		if(m <= 0)
			break;
		if(m > rem)
			m = rem;
		if(hwrite(hout, buf, m) != m)
			break;
	}
	hp = c->private;
	syslog(0, LOG, "%s: rs %s %lud-%lud/%lud-%lud %s", uriparts[2], hp->remotesys,
		r->start, r->stop+1-rem,
		r->start, r->stop+1,
		c->head.client);
	writelog(c, "Reply: %s\n", reply);
	close(fd);
	return;
err:
	filter("/sys/lib/dist/web/err/whoops.html", nil, nil, nil, nil);
	return;
}

HRange*
myfixrange(HConnect *c, Dir *d, int align)
{
	HRange *r, *rv;

	if(!c->req.vermaj)
		return nil;

	rv = nil;
	r = c->head.range;

	if(r == nil)
		goto out;

	if(c->head.ifrangeetag != nil)
		goto out;

	if(c->head.ifrangedate != 0 && c->head.ifrangedate != d->mtime)
		goto out;

	if(d->length == 0)
		goto out;

	/* we only support a single range */
	if(r->next != nil)
		goto out;

	if(r->suffix){
		r->start = d->length - r->stop;
		if(r->start >= d->length)
			r->start = 0;
		r->stop = d->length - 1;
		r->suffix = 0;
	}
	if(r->stop >= d->length)
		r->stop = d->length - 1;
	if(r->start > r->stop)
		goto out;
	if(align && (r->start%512) != 0)
		goto out;

	rv = r;

out:
	if(rv != nil)
		hprint(hout, "%s 206 Partial Content\r\n", hversion);
	else
		hprint(hout, "%s 200 OK\r\n", hversion);
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "Connection: close\r\n");
	hprint(hout, "Content-type: application/octet-stream\r\n");
	if(rv != nil){
		hprint(hout, "Content-Range: bytes %uld-%uld/%lld\r\n", r->start, r->stop,
			d->length);
		hprint(hout, "Content-Length: %uld\r\n", r->stop-r->start+1);
	}else
		hprint(hout, "Content-Length: %lld\r\n", d->length);
	hprint(hout, "Last-Modified: %D\r\n", d->mtime);
	hprint(hout, "\r\n");
	return rv;
}

/*
 *  create and check capability
 */
char *vers = "plan9.2002";
char *secret = "vitanuova.com";

char*
mkcap(long when)
{
	char cap[256], *p;
	uchar digest[SHA1dlen];
	DigestState *s;

	p = cap + sprint(cap, "%ld.", when);
	s = hmac_sha1((uchar*)vers, strlen(vers), (uchar*)secret, strlen(secret), nil, nil);
	hmac_sha1((uchar*)cap, strlen(cap), (uchar*)secret, strlen(secret), digest, s);
	enc32(p, sizeof(cap)-strlen(cap)-1, digest, SHA1dlen);
	return strdup(cap);
}

char*
getcap(HConnect *c)
{
	char *p;

	p = strchr(c->req.uri, '/');
	if(p == nil)
		p = "";
	else
		p++;
	p = strchr(p, '/');
	if(p == nil)
		p = "";
	else
		p++;
	return strdup(p);
}

int
capok(char *cap)
{
	char *ncap;
	int rv;
	long captime;

	if(cap[0]=='n')
		cap++;

	captime = atol(cap);
	if(time(0) - captime > 7*24*60*60)
		return 0;

	ncap = mkcap(captime);
	rv = strcmp(cap, ncap) == 0;
	free(ncap);

	return rv;
}

/*
 *  get a field out of a search string
 */
char*
getval(char *search, char *attr, char *buf, int len)
{
	int n;
	char *a = buf;

	len--;
	n = strlen(attr);
	while(search != nil){
		if(strncmp(search, attr, n) == 0 && *(search+n) == '='){
			search += n+1;
			while(len > 0 && *search && *search != '&'){
				*buf++ = *search++;
				len--;
			}
			break;
		}
		search = strchr(search, '&');
		if(search)
			search++;
	}
	*buf = 0;

	if(strcmp(a, "none") == 0 || strcmp(a, "auto") == 0)
		*a = 0;

	return a;
}

/*
 *  reverse dns lookup
 */
char*
dnsreverse(char *addr)
{
	Ndbtuple *t, *nt;
	char *rv;

	rv = nil;
	t = dnsquery("/net.alt", addr, "ptr");
	for(nt = t; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "ptr")){
			rv = strdup(nt->val);
			break;
		}
	}
	ndbfree(t);
	return rv;		
}

/*
 *  filter the page substituting if required
 */
void
filter(char *page, char *m0, char *s0, char *m1, char *s1)
{
	Biobuf *b;
	char *p, *m, *s, buf[MNAMELEN];
	int len;

	if(connect->req.vermaj && strstr(page, ".htm")){
		hokheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	} else {	/* it's the boot disk */
		hokheaders(connect);
		hprint(hout, "Content-type: application/octet-stream\r\n");
		hprint(hout, "\r\n");
	}

	b = Bopen(page, OREAD);
	if(b == nil)
		return;
	while((p = Brdline(b, '\n')) != nil){
		p[Blinelen(b)-1] = 0;
		if(m0 == nil || (m = strstr(p, m0)) == nil){
			if(m1 == nil || (m = strstr(p, m1)) == nil){
				hprint(hout, "%s\n", p);
				continue;
			} else {
				s = s1;
				len = strlen(m1);
			}
		} else {
			s = s0;
			len = strlen(m0);
		}
		*m = 0;
		hprint(hout, "%s", p);
		hprint(hout, "%s", s);
		hprint(hout, "%s\n", m + len);
	}
	Bterm(b);
	hflush(hout);
	p = strrchr(page, '/');
	if(p == nil)
		p = page;
	else{
		strncpy(buf, p + 1, MNAMELEN);
		buf[MNAMELEN-1] = '\0';
		p = strchr(buf, '.');
		if(p != nil)
			*p = '\0';
		p = buf;
	}
	writelog(connect, "Reply: 200 9down-%s %ld %ld\n", p, hout->seek, hout->seek);
}

int
mkini(int fd, char *search, char *cap)
{
	char *file, x[32], y[32];
	char buf[512];
	Biobuf b;
	int n;

	Binit(&b, fd, OWRITE);
	file = "/sys/lib/dist/web.protect/proto.ini";
	fd = open(file, OREAD);
	if(fd < 0)
		syslog(0, LOG, "opening %s: %r", file);
	else{
		n = read(fd, buf, sizeof(buf));
		if(n > 0)
			Bwrite(&b, buf, n);
		close(fd);
	}

	getval(search, "scsi", x, sizeof(x));
	if(*x){
		Bprint(&b, "scsi0=type=%s", x);
		getval(search, "scsiport", x, sizeof(x));
		if(*x){
			if(strncmp(x, "0x", 2) == 0)
				Bprint(&b, " port=%s", x);
			else
				Bprint(&b, " port=0x%s", x);
		}
		getval(search, "scsiirq", x, sizeof(x));
		if(*x)
			Bprint(&b, " irq=%s", x);
		Bprint(&b, "\r\n");
	}
	getval(search, "ether0", x, sizeof(x));
	if(*x){
		Bprint(&b, "ether0=type=%s", x);
		getval(search, "ether0port", x, sizeof(x));
		if(*x){
			if(strncmp(x, "0x", 2) == 0)
				Bprint(&b, " port=%s", x);
			else
				Bprint(&b, " port=0x%s", x);
		}
		getval(search, "ether0irq", x, sizeof(x));
		if(*x)
			Bprint(&b, " irq=%s", x);
		getval(search, "ether0media", x, sizeof(x));
		if(*x)
			Bprint(&b, " media=%s", x);
		Bprint(&b, "\r\n");
	}
	getval(search, "ether1", x, sizeof(x));
	if(*x){
		Bprint(&b, "ether1=type=%s", x);
		getval(search, "ether1port", x, sizeof(x));
		if(*x){
			if(strncmp(x, "0x", 2) == 0)
				Bprint(&b, " port=%s", x);
			else
				Bprint(&b, " port=0x%s", x);
		}
		getval(search, "ether1irq", x, sizeof(x));
		if(*x)
			Bprint(&b, " irq=%s", x);
		getval(search, "ether1media", x, sizeof(x));
		if(*x)
			Bprint(&b, " media=%s", x);
		Bprint(&b, "\r\n");
	}
	getval(search, "monitor", x, sizeof(x));
	if(*x)
		Bprint(&b, "monitor=%s\r\n", x);
	getval(search, "vgasize", x, sizeof(x));
	getval(search, "vgadepth", y, sizeof(y));
	if(*x && *y)
		Bprint(&b, "vgasize=%sx%s\r\n", x, y);
	getval(search, "mouseport", x, sizeof(x));
	if(*x)
		Bprint(&b, "mouseport=%s\r\n", x);
	getval(search, "audio0", x, sizeof(x));
	if(*x){
		Bprint(&b, "audio0=type=%s", x);
		getval(search, "audio0port", x, sizeof(x));
		if(*x){
			if(strncmp(x, "0x", 2) == 0)
				Bprint(&b, " port=%s", x);
			else
				Bprint(&b, " port=0x%s", x);
		}
		getval(search, "audio0irq", x, sizeof(x));
		if(*x)
			Bprint(&b, " irq=%s", x);
		getval(search, "audio0dma", x, sizeof(x));
		if(*x)
			Bprint(&b, " dma=%s", x);
		Bprint(&b, "\r\n");
	}
	getval(search, "bootdev", x, sizeof(x));
	if(*x == 0)
		strcpy(x, "fd0");
	if(strncmp(x, "fd", 2) == 0){
		Bprint(&b, "bootfile=%s!dos!9pcflop.gz\r\n", x);
	}else{
		Bprint(&b, "bootfile=%s!data!9pcflop.gz\r\n", x);
	}

	Bprint(&b, "installurl=http://204.178.31.2/magic/9down4e/compressed/%s\r\n",
		cap);
	Bterm(&b);

	return 0;
}

char*
mkdiskette(char *cap, char *search)
{
	char ini[256];
	long t;
	int fd, pid;

	if(*cap == 'n')
		cap++;
	t = atol(cap);
	pid = getpid();
	sprint(ini, "/usr/none/pcdist/tmp/%ld.%d", t, pid);

	/* make the ini file */
	fd = create(ini, OWRITE, 0444);
	if(fd < 0){
		syslog(0, LOG, "creating %s: %r", ini);
		return nil;
	}
	if(mkini(fd, search, cap) < 0){
		syslog(0, LOG, "writing %s: %r", ini);
		close(fd);
		remove(ini);
		return nil;
	}
	close(fd);
	return strdup(strrchr(ini, '/')+1);
}
