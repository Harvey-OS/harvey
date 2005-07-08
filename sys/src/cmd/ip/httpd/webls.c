#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <regexp.h>
#include <fcall.h>
#include "httpd.h"
#include "httpsrv.h"

static	Hio		*hout;
static	Hio		houtb;
static	HConnect	*connect;
static	int		vermaj, gidwidth, uidwidth, lenwidth, devwidth;
static	Biobuf		*aio, *dio;

static void
doctype(void)
{
	hprint(hout, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n");
	hprint(hout, "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
}

void
error(char *title, char *fmt, ...)
{
	va_list arg;
	char buf[1024], *out;

	va_start(arg, fmt);
	out = vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	*out = 0;

	hprint(hout, "%s 404 %s\r\n", hversion, title);
	hprint(hout, "Date: %D\r\n", time(nil));
	hprint(hout, "Server: Plan9\r\n");
	hprint(hout, "Content-type: text/html\r\n");
	hprint(hout, "\r\n");
	doctype();
	hprint(hout, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	hprint(hout, "<head><title>%s</title></head>\n", title);
	hprint(hout, "<body>\n");
	hprint(hout, "<h1>%s</h1>\n", title);
	hprint(hout, "%s\n", buf);
	hprint(hout, "</body>\n");
	hprint(hout, "</html>\n");
	hflush(hout);
	writelog(connect, "Reply: 404\nReason: %s\n", title);
	exits(nil);
}

/*
 * Are we actually allowed to look in here?
 *
 * Rules:
 *	1) If neither allowed nor denied files exist, access is granted.
 *	2) If allowed exists and denied does not, dir *must* be in allowed
 *	   for access to be granted, otherwise, access is denied.
 *	3) If denied exists and allowed does not, dir *must not* be in
 *	   denied for access to be granted, otherwise, access is enied.
 *	4) If both exist, okay if either (a) file is not in denied, or
 *	   (b) in denied and in allowed.  Otherwise, access is denied.
 */
static Reprog *
getre(Biobuf *buf)
{
	Reprog	*re;
	char	*p, *t;
	char	*bbuf;
	int	n;

	if (buf == nil)
		return(nil);
	for ( ; ; free(p)) {
		p = Brdstr(buf, '\n', 0);
		if (p == nil)
			return(nil);
		t = strchr(p, '#');
		if (t != nil)
			*t = '\0';
		t = p + strlen(p);
		while (--t > p && isspace(*t))
			*t = '\0';
		n = strlen(p);
		if (n == 0)
			continue;

		/* root the regular expresssion */
		bbuf = malloc(n+2);
		if(bbuf == nil)
			sysfatal("out of memory");
		bbuf[0] = '^';
		strcpy(bbuf+1, p);
		re = regcomp(bbuf);
		free(bbuf);

		if (re == nil)
			continue;
		free(p);
		return(re);
	}
}

static int
allowed(char *dir)
{
	Reprog	*re;
	int	okay;
	Resub	match;

	if (strcmp(dir, "..") == 0 || strncmp(dir, "../", 3) == 0)
		return(0);
	if (aio == nil)
		return(0);

	if (aio != nil)
		Bseek(aio, 0, 0);
	if (dio != nil)
		Bseek(dio, 0, 0);

	/* if no deny list, assume everything is denied */
	okay = (dio != nil);

	/* go through denials till we find a match */
	while (okay && (re = getre(dio)) != nil) {
		memset(&match, 0, sizeof(match));
		okay = (regexec(re, dir, &match, 1) != 1);
		free(re);
	}

	/* go through accepts till we have a match */
	if (aio == nil)
		return(okay);
	while (!okay && (re = getre(aio)) != nil) {
		memset(&match, 0, sizeof(match));
		okay = (regexec(re, dir, &match, 1) == 1);
		free(re);
	}
	return(okay);
}

/*
 * Comparison routine for sorting the directory.
 */
static int
compar(Dir *a, Dir *b)
{
	return(strcmp(a->name, b->name));
}

/*
 * These is for formating; how wide are variable-length
 * fields?
 */
static void
maxwidths(Dir *dp, long n)
{
	long	i;
	char	scratch[64];

	for (i = 0; i < n; i++) {
		if (snprint(scratch, sizeof scratch, "%ud", dp[i].dev) > devwidth)
			devwidth = strlen(scratch);
		if (strlen(dp[i].uid) > uidwidth)
			uidwidth = strlen(dp[i].uid);
		if (strlen(dp[i].gid) > gidwidth)
			gidwidth = strlen(dp[i].gid);
		if (snprint(scratch, sizeof scratch, "%lld", dp[i].length) > lenwidth)
			lenwidth = strlen(scratch);
	}
}

/*
 * Do an actual directory listing.
 * asciitime is lifted directly out of ls.
 */
char *
asciitime(long l)
{
	ulong clk;
	static char buf[32];
	char *t;

	clk = time(nil);
	t = ctime(l);
	/* 6 months in the past or a day in the future */
	if(l<clk-180L*24*60*60 || clk+24L*60*60<l){
		memmove(buf, t+4, 7);		/* month and day */
		memmove(buf+7, t+23, 5);		/* year */
	}else
		memmove(buf, t+4, 12);		/* skip day of week */
	buf[12] = 0;
	return buf;
}

static void
dols(char *dir)
{
	Dir	*d;
	char	*f, *p,*nm;
	long	i, n;
	int	fd;

	cleanname(dir); //  expands "" to "."; ``dir+1'' access below depends on that
	if (!allowed(dir)) {
		error("Permission denied", "<p>Cannot list directory %s: Access prohibited</p>", dir);
		return;
	}
	fd = open(dir, OREAD);
	if (fd < 0) {
		error("Cannot read directory", "<p>Cannot read directory %s: %r</p>", dir);
		return;
	}
	if (vermaj) {
		hokheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}
	doctype();
	hprint(hout, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	hprint(hout, "<head><title>Index of %s</title></head>\n", dir);
	hprint(hout, "<body>\n");
	hprint(hout, "<h1>Index of ");
	nm = dir;
	while((p = strchr(nm, '/')) != nil){
		*p = '\0';
		f = (*dir == '\0') ? "/" : dir;
		if (!(*dir == '\0' && *(dir+1) == '\0') && allowed(f))
			hprint(hout, "<a href=\"/magic/webls?dir=%H\">%s/</a>", f, nm);
		else
			hprint(hout, "%s/", nm);
		*p = '/';
		nm = p+1;
	}
	hprint(hout, "%s</h1>\n", nm);
	n = dirreadall(fd, &d);
	close(fd);
	maxwidths(d, n);
	qsort(d, n, sizeof(Dir), (int (*)(void *, void *))compar);
	hprint(hout, "<pre>\n");
	for (i = 0; i < n; i++) {
		f = smprint("%s/%s", dir, d[i].name);
		cleanname(f);
		if (d[i].mode & DMDIR) {
			p = smprint("/magic/webls?dir=%H", f);
			free(f);
			f = p;
		}
		hprint(hout, "%M %C %*ud %-*s %-*s %*lld %s <a href=\"%s\">%s</a>\n",
		    d[i].mode, d[i].type,
		    devwidth, d[i].dev,
		    uidwidth, d[i].uid,
		    gidwidth, d[i].gid,
		    lenwidth, d[i].length,
		    asciitime(d[i].mtime), f, d[i].name);
		free(f);
	}
	f = smprint("%s/..", dir);
	cleanname(f);
	if (strcmp(f, dir) != 0 && allowed(f))
		hprint(hout, "\nGo to <a href=\"/magic/webls?dir=%H\">parent</a> directory\n", f);
	else
		hprint(hout, "\nEnd of directory listing\n");
	free(f);
	hprint(hout, "</pre>\n</body>\n</html>\n");
	hflush(hout);
	free(d);
}

/*
 * Handle unpacking the request in the URI and
 * invoking the actual handler.
 */
static void
dosearch(char *search)
{
	if (strncmp(search, "dir=", 4) == 0){
		search = hurlunesc(connect, search+4);
		dols(search);
		return;
	}

	/*
	 * Otherwise, we've gotten an illegal request.
	 * spit out a non-apologetic error.
	 */
	search = hurlunesc(connect, search);
	error("Bad directory listing request",
	    "<p>Illegal formatted directory listing request:</p>\n"
	    "<p>%H</p>", search);
}

void
main(int argc, char **argv)
{
	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);
	fmtinstall('M', dirmodefmt);

	aio = Bopen("/sys/lib/webls.allowed", OREAD);
	dio = Bopen("/sys/lib/webls.denied", OREAD);

	if(argc == 2){
		hinit(&houtb, 1, Hwrite);
		hout = &houtb;
		dols(argv[1]);
		exits(nil);
	}
	close(2);

	connect = init(argc, argv);
	hout = &connect->hout;
	vermaj = connect->req.vermaj;
	if(hparseheaders(connect, HSTIMEOUT) < 0)
		exits("failed");

	if(strcmp(connect->req.meth, "GET") != 0 && strcmp(connect->req.meth, "HEAD") != 0){
		hunallowed(connect, "GET, HEAD");
		exits("not allowed");
	}
	if(connect->head.expectother || connect->head.expectcont){
		hfail(connect, HExpectFail, nil);
		exits("failed");
	}

	bind("/usr/web", "/", MREPL);

	if(connect->req.search != nil)
		dosearch(connect->req.search);
	else
		error("Bad argument", "<p>Need a search argument</p>");
	hflush(hout);
	writelog(connect, "200 webls %ld %ld\n", hout->seek, hout->seek);
	exits(nil);
}
