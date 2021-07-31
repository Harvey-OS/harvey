#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct URL URL;
struct URL
{
	int	method;
	char	*host;
	char	*port;
	char	*page;
	char	*etag;
	char	*redirect;
	long	mtime;
};

typedef struct Range Range;
struct Range
{
	long	start;	/* only 2 gig supported, tdb */
	long	end;
};

enum
{
	Http,
	Ftp,
	Other
};

enum
{
	Eof = 0,
	Error = -1,
	Server = -2,
	Changed = -3,
};

int debug;
char *ofile;

int	doftp(URL*, Range*, int, long);
int	dohttp(URL*, Range*, int, long);
int	crackurl(URL*, char*);
Range*	crackrange(char*);
int	getheader(int, char*, int);
int	httpheaders(int, URL*, Range*);
int	httprcode(int);
int	cistrncmp(char*, char*, int);
int	cistrcmp(char*, char*);
void	initibuf(void);
int	readline(int, char*, int);
int	readibuf(int, char*, int);
int	dfprint(int, char*, ...);
void	unreadline(char*);
int	verbose;

struct {
	char	*name;
	int	(*f)(URL*, Range*, int, long);
} method[] = {
	[Http]	{ "http",	dohttp },
	[Ftp]	{ "ftp",	doftp },
	[Other]	{ "_______",	nil },
};

void
usage(void)
{
	fprint(2, "usage: %s [-v] [-o outfile] url\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	URL u;
	Range r;
	int fd, errs, n;
	Dir d;

	ofile = nil;
	r.start = 0;
	r.end = -1;
	d.mtime = 0;

	ARGBEGIN {
	case 'o':
		ofile = ARGF();
		break;
	case 'd':
		debug = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	} ARGEND;

	if(argc != 1)
		usage();

	fd = 1;
	if(ofile != nil){
		if(dirstat(ofile, &d) < 0){
			fd = create(ofile, OWRITE, 0664);
			if(fd < 0)
				sysfatal("creating %s: %r", ofile);
		} else {
			fd = open(ofile, OWRITE);
			if(fd < 0)
				sysfatal("can't open %s: %r", ofile);
			r.start = d.length;

			/* always move back to a previous 512 byte bound */
			if(r.start)
				r.start = ((r.start-1)/512)*512;
		}
	}

	errs = 0;

	if(crackurl(&u, argv[0]) < 0)
		sysfatal("%r");

	for(;;){
		/* transfer data */
		werrstr("");
		n = (*method[u.method].f)(&u, &r, fd, d.mtime);

		switch(n){
		case Eof:
			exits(0);
			break;
		case Error:
			if(errs++ < 10)
				continue;
			sysfatal("too many errors with no progress %r");
			break;
		case Server:
			sysfatal("server returned: %r");
			break;
		}

		/* forward progress */
		errs = 0;
		r.start += n;
		if(r.start >= r.end)
			break;
	}

	exits(0);
}

int
crackurl(URL *u, char *s)
{
	char *p;
	int i;

	if(u->host != nil){
		free(u->host);
		u->host = nil;
	}
	if(u->page != nil){
		free(u->page);
		u->page = nil;
	}

	/* get type */
	u->method = Other;
	for(p = s; *p; p++){
		if(*p == '/'){
			u->method = Http;
			p = s;
			break;
		}
		if(*p == ':' && *(p+1)=='/' && *(p+2)=='/'){
			*p = 0;
			p += 3;
			for(i = 0; i < nelem(method); i++){
				if(cistrcmp(s, method[i].name) == 0){
					u->method = i;
					break;
				}
			}
			break;
		}
	}

	if(u->method == Other){
		werrstr("unsupported URL type %s", s);
		return -1;
	}

	/* get system */
	s = p;
	p = strchr(s, '/');
	if(p == nil){
		u->host = strdup(s);
		u->page = strdup("/");
	} else {
		u->page = strdup(p);
		*p = 0;
		u->host = strdup(s);
		*p = '/';
	}

	if(p = strchr(u->host, ':')) {
		*p++ = 0;
		u->port = p;
	} else 
		u->port = "80";

	if(*(u->host) == 0){
		werrstr("bad url, null host");
		return -1;
	}

	return 0;
}

int
doftp(URL*, Range*, int, long)
{
	sysfatal("ftp not yet implemented");
	return -1;
}

char *day[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

struct
{
	int	fd;
	long	mtime;
} note;

void
catch(void*, char*)
{
	Dir d;

	if(dirfstat(note.fd, &d) < 0)
		sysfatal("catch: can't dirfstat: %r");
	d.mtime = note.mtime;
	if(dirfwstat(note.fd, &d) < 0)
		sysfatal("catch: can't dirfwstat: %r");
	noted(NDFLT);
}

int
dohttp(URL *u, Range *r, int out, long mtime)
{
	int fd;
	int redirect, loop;
	int n, rv, code;
	long tot, vtime;
	Tm *tm;
	char buf[1024];
	char err[ERRLEN];

	/* loop for redirects, requires reading both response code and headers */
	fd = -1;
	for(loop = 0; loop < 32; loop++){
		fd = dial(netmkaddr(u->host, "tcp", u->port), 0, 0, 0);
		if(fd < 0)
			return Error;

		/* write request, use range if not start of file */
		dfprint(fd, "GET %s HTTP/1.0\n", u->page);
		dfprint(fd, "Host: %s\n", u->host);
		if(r->start != 0){
			dfprint(fd, "Range: bytes=%d-\n", r->start);
			if(u->etag != nil){
				dfprint(fd, "If-range: %s\n", u->etag);
			} else {
				tm = gmtime(mtime);
				dfprint(fd, "If-range: %s, %d %s %d %2d:%2.2d:%2.2d GMT\n",
					day[tm->wday], tm->mday, month[tm->mon],
					tm->year+1900, tm->hour, tm->min, tm->sec);
			}
		}
		dfprint(fd, "\r\n", u->host);

		redirect = 0;
		initibuf();
		code = httprcode(fd);
		switch(code){
		case Error:	/* connection timed out */
		case Eof:
			return code;

		case 200:	/* OK */
		case 201:	/* Created */
		case 202:	/* Accepted */
			if(ofile == nil && r->start != 0)
				sysfatal("page changed underfoot");
			break;

		case 204:	/* No Content */
			sysfatal("No Content");

		case 206:	/* Partial Content */
			seek(out, r->start, 0);
			break;

		case 301:	/* Moved Permanently */
		case 302:	/* Moved Temporarily */
			redirect = 1;
			break;

		case 304:	/* Not Modified */
			break;

		case 400:	/* Bad Request */
			sysfatal("Bad Request");

		case 401:	/* Unauthorized */
		case 402:	/* ??? */
			sysfatal("Unauthorized");

		case 403:	/* Forbidden */
			sysfatal("Forbidden by server");

		case 404:	/* Not Found */
			sysfatal("Not found on server");

		case 500:	/* Internal server error */
			sysfatal("Server choked");

		case 501:	/* Not implemented */
			sysfatal("Server can't do it!");

		case 502:	/* Bad gateway */
			sysfatal("Bad gateway");

		case 503:	/* Service unavailable */
			sysfatal("Service unavailable");
		
		default:
			sysfatal("Unknown response code");
		}

		if(u->redirect != nil){
			free(u->redirect);
			u->redirect = nil;
		}

		rv = httpheaders(fd, u, r);
		if(rv != 0)
			return rv;

		if(!redirect)
			break;

		if(u->redirect == nil)
			sysfatal("redirect: no URL");
		if(crackurl(u, u->redirect) < 0)
			sysfatal("redirect: %r");
	}

	/* transfer whatever you get */
	if(ofile != nil && u->mtime != 0){
		note.fd = out;
		note.mtime = u->mtime;
		notify(catch);
	}

	tot = 0;
	vtime = 0;
	for(;;){
		n = readibuf(fd, buf, sizeof(buf));
		if(n <= 0)
			break;
		if(write(out, buf, n) != n)
			break;
		tot += n;
		if(verbose && vtime != time(0)) {
			vtime = time(0);
			fprint(2, "%ld %ld\n", r->start+tot, r->end);
		}
	}
	notify(nil);

	errstr(err);
	if(ofile != nil && u->mtime != 0){
		Dir d;

		dirfstat(out, &d);
		d.mtime = u->mtime;
		if(dirfwstat(out, &d) < 0)
			fprint(2, "couldn't set mtime: %r\n");
	}
	errstr(err);

	return tot;
}

/* get the http response code */
int
httprcode(int fd)
{
	int n;
	char *p;
	char buf[256];

	n = readline(fd, buf, sizeof(buf)-1);
	if(n <= 0)
		return n;
	if(debug)
		fprint(2, "%d <- %s\n", fd, buf);
	p = strchr(buf, ' ');
	if(strncmp(buf, "HTTP/", 5) != 0 || p == nil){
		werrstr("bad response from server");
		return -1;
	}
	buf[n] = 0;
	return atoi(p+1);
}

/* read in and crack the http headers, update u and r */
void	hhetag(char*, URL*, Range*);
void	hhmtime(char*, URL*, Range*);
void	hhclen(char*, URL*, Range*);
void	hhcrange(char*, URL*, Range*);
void	hhuri(char*, URL*, Range*);
void	hhlocation(char*, URL*, Range*);

struct {
	char *name;
	void (*f)(char*, URL*, Range*);
} headers[] = {
	{ "etag:", hhetag },
	{ "last-modified:", hhmtime },
	{ "content-length:", hhclen },
	{ "content-range:", hhcrange },
	{ "uri:", hhuri },
	{ "location:", hhlocation },
};
int
httpheaders(int fd, URL *u, Range *r)
{
	char buf[2048];
	char *p;
	int i, n;

	for(;;){
		n = getheader(fd, buf, sizeof(buf));
		if(n <= 0)
			break;
		for(i = 0; i < nelem(headers); i++){
			n = strlen(headers[i].name);
			if(cistrncmp(buf, headers[i].name, n) == 0){
				/* skip field name and leading white */
				p = buf + n;
				while(*p == ' ' || *p == '\t')
					p++;

				(*headers[i].f)(p, u, r);
				break;
			}
		}
	}
	return n;
}

/*
 *  read a single mime header, collect continuations.
 *
 *  this routine assumes that there is a blank line twixt
 *  the header and the message body, otherwise bytes will
 *  be lost.
 */
int
getheader(int fd, char *buf, int n)
{
	char *p, *e;
	int i;

	n--;
	p = buf;
	for(e = p + n; ; p += i){
		i = readline(fd, p, e-p);
		if(i < 0)
			return i;

		if(p == buf){
			/* first line */
			if(strchr(buf, ':') == nil)
				break;		/* end of headers */
		} else {
			/* continuation line */
			if(*p != ' ' && *p != '\t'){
				unreadline(p);
				*p = 0;
				break;		/* end of this header */
			}
		}
	}

	if(debug)
		fprint(2, "%d <- %s\n", fd, buf);
	return p-buf;
}

void
hhetag(char *p, URL *u, Range*)
{
	if(u->etag != nil){
		if(strcmp(u->etag, p) != 0)
			sysfatal("file changed underfoot");
	} else
		u->etag = strdup(p);
}

char*	monthchars = "janfebmaraprmayjunjulaugsepoctnovdec";

void
hhmtime(char *p, URL *u, Range*)
{
	char *month, *day, *yr, *hms;
	char *fields[6];
	Tm tm, now;
	int i;

	i = getfields(p, fields, 6, 1, " \t");
	if(i < 5)
		return;

	day = fields[1];
	month = fields[2];
	yr = fields[3];
	hms = fields[4];

	/* default time */
	now = *gmtime(time(0));
	tm = now;

	/* convert ascii month to a number twixt 1 and 12 */
	if(*month >= '0' && *month <= '9'){
		tm.mon = atoi(month) - 1;
		if(tm.mon < 0 || tm.mon > 11)
			tm.mon = 5;
	} else {
		for(p = month; *p; p++)
			*p = tolower(*p);
		for(i = 0; i < 12; i++)
			if(strncmp(&monthchars[i*3], month, 3) == 0){
				tm.mon = i;
				break;
			}
	}

	tm.mday = atoi(day);

	if(hms) {
		tm.hour = strtoul(hms, &p, 10);
		if(*p == ':') {
			p++;
			tm.min = strtoul(p, &p, 10);
			if(*p == ':') {
				p++;
				tm.sec = strtoul(p, &p, 10);
			}
		}
		if(tolower(*p) == 'p')
			tm.hour += 12;
	}

	if(yr) {
		tm.year = atoi(yr);
		if(tm.year >= 1900)
			tm.year -= 1900;
	} else {
		if(tm.mon > now.mon || (tm.mon == now.mon && tm.mday > now.mday+1))
			tm.year--;
	}

	strcpy(tm.zone, "GMT");
	/* convert to epoch seconds */
	u->mtime = tm2sec(&tm);
}

void
hhclen(char *p, URL*, Range *r)
{
	r->end = atoi(p);
}

void
hhcrange(char *p, URL*, Range *r)
{
	char *x;
	vlong l;

	l = 0;
	x = strchr(p, '/');
	if(x)
		l = atoll(x+1);
	if(l == 0)
	x = strchr(p, '-');
	if(x)
		l = atoll(x+1);
	if(l)
		r->end = l;
}

void
hhuri(char *p, URL *u, Range*)
{
	if(*p != '<')
		return;
	u->redirect = strdup(p+1);
	p = strchr(u->redirect, '>');
	if(p != nil)
		*p = 0;
}

void
hhlocation(char *p, URL *u, Range*)
{
	u->redirect = strdup(p);
}

int
cistrncmp(char *a, char *b, int n)
{
	while(n-- > 0){
		if(tolower(*a++) != tolower(*b++))
			return -1;
	}
	return 0;
}

int
cistrcmp(char *a, char *b)
{
	while(*a || *b)
		if(tolower(*a++) != tolower(*b++))
			return -1;

	return 0;
}

/*
 *  buffered io
 */
struct
{
	char *rp;
	char *wp;
	char buf[4*1024];
} b;

void
initibuf(void)
{
	b.rp = b.wp = b.buf;
}

int
readline(int fd, char *buf, int len)
{
	int n;
	char *p;

	len--;

	for(p = buf;;){
		if(b.rp >= b.wp){
			n = read(fd, b.wp, sizeof(b.buf)/2);
			if(n < 0)
				return -1;
			if(n == 0)
				break;
			b.wp += n;
		}
		n = *b.rp++;
		if(len > 0){
			*p++ = n;
			len--;
		}
		if(n == '\n')
			break;
	}

	/* drop trailing white */
	for(;;){
		if(p <= buf)
			break;
		n = *(p-1);
		if(n != ' ' && n != '\t' && n != '\r' && n != '\n')
			break;
		p--;
	}

	*p = 0;
	return p-buf;
}

void
unreadline(char *line)
{
	int i, n;

	i = strlen(line);
	n = b.wp-b.rp;
	memmove(&b.buf[i+1], b.rp, n);
	memmove(b.buf, line, i);
	b.buf[i] = '\n';
	b.rp = b.buf;
	b.wp = b.rp + i + 1 + n;
}

int
readibuf(int fd, char *buf, int len)
{
	int n;

	n = b.wp-b.rp;
	if(n > 0){
		if(n > len)
			n = len;
		memmove(buf, b.rp, n);
		b.rp += n;
		return n;
	}
	return read(fd, buf, len);
}

int
dfprint(int fd, char *fmt, ...)
{
	char buf[4*1024];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(debug)
		fprint(2, "%d -> %s", fd, buf);
	return fprint(fd, "%s", buf);
}
