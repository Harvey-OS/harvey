/*
 * Serve files from the Plan 9 distribution.
 * Check that we're not giving out files to bad countries
 * and then just handle the request normally.
 * Beware: behaviour changes based on argv[0].
 * Modified to serve /sys/src, not /n/sources.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include <libsec.h>
#include <auth.h>
#include "httpd.h"
#include "httpsrv.h"

#define LOG "9down"

#ifndef OUTSIDE
#define OUTSIDE 0
#endif

static	Hio	houtb;
static	Hio	*hout;
static	HConnect*connect;

enum
{
	MNAMELEN	= 64,
};

void	cat(char*, int);
void	filter(char*, char*, char*, char*, char*);
HRange*	myfixrange(HConnect*, Dir*, int);
void	error(char*, ...);
void	interr(char*, ...);
void	system(char*, ...);

int sources;

void
main(int argc, char **argv)
{
	int trailingslash;
	HSPriv *hp;
	char *file, *dir;
	char ok[256];

	quotefmtinstall();
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
	/* rename 9down to sources to display sources tree! */
	if(strstr(argv[0], "sources"))
		sources = 1;
	syslog(0, LOG, "%s %s ver %d.%d uri %s search %s",
		sources ? "sources" : "9down",
		hp->remotesys,
		connect->req.vermaj, connect->req.vermin,
		connect->req.uri, connect->req.search);

	file = strdup(connect->req.uri);

	dir = "/usr/web";
	if(sources)
		dir = "/usr/web/sources";
	if(chdir(dir) < 0)
		interr("cd %s: %r", dir);

	trailingslash = (file[0] && file[strlen(file)-1] == '/');
	if(file[0] == 0)
		file = strdup("/");
	cleanname(file);
	if(file[0] == '/'){
		if(file[1])
			file++;
		else
			file = ".";
	}
	/* now file is not rooted, no dot dots */

	if(file[0] == '#')
		interr("bad file %s", file);
	if(access(file, AEXIST) < 0)
		error("404 %s not found", file);
	if (OUTSIDE) {
		if(access("/mnt/ipok/ok", AEXIST) < 0){
			system("mount /srv/ipok /mnt/ipok >[2]/dev/null");
			if(access("/mnt/ipok/ok", AEXIST) < 0)
				interr("no /mnt/ipok/ok");
		}
		snprint(ok, sizeof ok, "/mnt/ipok/ok/%s", hp->remotesys);
		if(access(ok, AEXIST) < 0){
			syslog(0, LOG, "reject %s %s",
				hp->remotesys, connect->req.uri);
			file = "/sys/lib/dist/web/err/prohibited.html";
		}
	}
	cat(file, trailingslash);
	hflush(hout);
	exits(nil);
}

static void
doerr(char *msg)
{
	hprint(hout, "%s %s\r\n", hversion, msg);
	writelog(connect, "Reply: %s\n", msg);
	hflush(hout);
	exits(nil);
}

/* must pass an http error code then a message, e.g., 404 not found */
void
error(char *fmt, ...)
{
	char *buf;
	va_list arg;

	va_start(arg, fmt);
	buf = vsmprint(fmt, arg);
	va_end(arg);

	syslog(0, LOG, "error: %s", buf);
	doerr(buf);
}

/* must not pass an http error code, just a message */
void
interr(char *fmt, ...)
{
	char *buf;
	va_list arg;

	va_start(arg, fmt);
	buf = vsmprint(fmt, arg);
	va_end(arg);

	syslog(0, LOG, "internal error: %s", buf);
	doerr("500 Internal Error");
}

void
system(char *fmt, ...)
{
	char buf[512];
	va_list arg;

	va_start(arg, fmt);
	vsnprint(buf, sizeof buf, fmt, arg);
	va_end(arg);

	if(fork() == 0){
		execl("/bin/rc", "rc", "-c", buf, nil);
		_exits("execl failed");
	}
	waitpid();
}

void
cat(char *file, int trailingslash)
{
	int fd, m;
	char *reply;
	long rem;
	HRange rr;
	Dir *d;
	HRange *r;
	HConnect *c;
	HSPriv *hp;
	char buf[8192];

	c = connect;
	if((fd = open(file, OREAD)) < 0){
	notfound:
		hprint(hout, "%s 404 Not Found\r\n", hversion);
		writelog(c, "Reply: 404 Not Found\n");
		return;
	}

	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		goto notfound;
	}

	if(sources){
		if((d->mode&DMDIR) && !trailingslash){
			hprint(hout, "%s 301 Moved Permanently\r\n", hversion);
			hprint(hout, "Date: %D\r\n", time(nil));
			hprint(hout, "Connection: close\r\n");
			if(strcmp(file, ".") == 0)
				hprint(hout, "Location: /sources/\r\n");
			else
				hprint(hout, "Location: /sources/%s/\r\n", file);
			hprint(hout, "\r\n");
			hflush(hout);
			exits(0);
		}
		hprint(hout, "%s 200 OK\r\n", hversion);
		hprint(hout, "Server: Plan9\r\n");
		hprint(hout, "Date: %D\r\n", time(nil));
		hprint(hout, "Connection: close\r\n");
		hprint(hout, "Last-Modified: %D\r\n", d->mtime);
		hflush(hout);
		dup(hout->fd, 1);
		dup(hout->fd, 2);
		system("/sys/lib/dist/sources2web %q", file);
		exits(0);
	}

	if(d->mode&DMDIR)
		goto notfound;

	r = myfixrange(connect, d, 0);
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
	syslog(0, LOG, "%s: rs %s %lud-%lud/%lud-%lud %s", file, hp->remotesys,
		r->start, r->stop+1-rem,
		r->start, r->stop+1,
		c->head.client);
	writelog(c, "Reply: %s\n", reply);
	close(fd);
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


