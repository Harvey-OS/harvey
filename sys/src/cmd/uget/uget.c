/*
 * Trivial web browser
 */
#include "mothra.h"

Url *selection=0;
Url *current=0;
char *ofile = 0;
char *argv0;
int server, debug;

Url defurl =
{
	"http://plan9.bell-labs.com/",
	0,
	"plan9.bell-labs.com",
	"/",
	"",
	80,
	HTTP,
	HTML
};

void*
emalloc(int n)
{
	void *v;

	v=malloc(n);
	if(v==0){
		fprint(2, "out of space\n");
		exits("no mem");
	}
	return v;
}

void
message(char *fmt, ...)
{
	va_list arg;
	static char buf[1024];
	char *out;

	va_start(arg, fmt);
	out = doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	write(2, buf, out-buf);
}

int
urlopen(Url *url, int method, char *body)
{
	int fd;
	Url prev;
	int nredir;

	nredir = 0;

Again:
	if(++nredir == NREDIR) {
		werrstr("redir loop");
		return -1;
	}

	switch(url->access) {
	default:
		werrstr("unknown access type");
		return -1;
	case FTP:
		url->type = suffix2type(url->reltext);
		if(server)
			print("%s\n", type2content(url->type));
		return ftp(url);

	case HTTP:
		fd = http(url, method, body);
		if(url->type == FORWARD){
			prev = *url;
			crackurl(url, prev.redirname, &prev);
			/*
			 * I'm not convinced that the following two lines are right,
			 * but once I got a redir loop because they were missing.
			 */
			method = GET;
			body = 0;
			goto Again;
		}
		if(server)
			print("%s\n", type2content(url->type));
		return fd;

	case GOPHER:
		if(server)
			print("%s\n", type2content(url->type));
		return gopher(url);
	}
}

void
selurl(char *urlname)
{
	Url *cur;
	static Url url;

	if(current)
		cur=current;
	else
		cur=&defurl;
	crackurl(&url, urlname, cur);
	selection=&url;
}

void
geturl(char *urlname, int method, char *body)
{
	int fd, nfd;
	char buf[4096];
	int n;
	Dir d;

	memset(&d, 0, sizeof(d));

	selurl(urlname);
	switch(selection->access) {
	default:
		fprint(2, "unknown access %d\n", selection->access);
		return;
	case FTP:
	case HTTP:
	case AFILE:
	case EXEC:
	case GOPHER:
		fd = urlopen(selection, method, body);
		break;
	}
	if(server)
		print("url %s\n", selection->fullname);

	if(fd < 0) {
		if(!server){
			fprint(2, "%r getting %s\n", urlname);
			exits("io");
		}
		snprint(buf, sizeof(buf),
			"<head><title>%r</title></head>\r\n"
			"<body><h1>%r</h1>\r\n"
			"Error accessing %s</body>\r\n",
				urlname);
		n = strlen(buf);
		fprint(1, "%d\n", n);
		write(1, buf, n);
		fprint(1, "0\n");
		return;
	}

	if(ofile) {
		if(debug)
			fprint(2, "modtime %lud\n", modtime);
		if(dirstat(ofile, &d) >= 0 && d.mtime >= modtime)
			exits("not changed");
		nfd = create(ofile, OWRITE, 0666);
		if(nfd < 0)
			exits("can't write");
	} else
		nfd = 1;

	while((n = read(fd, buf, sizeof buf)) > 0){
		if(server)
			fprint(nfd, "%d\n", n);
		write(nfd, buf, n);
	}
	if(server)
		fprint(nfd, "0\n");
	else
		close(nfd);
	close(fd);
}

int
dourl(Ibuf *b)
{
	char *bp, *up;
	static Url base;

	bp = rdline(b);
	if(bp == 0)
		return -1;
	bp[linelen(b)-1] = 0;
	up = strchr(bp, ' ');
	if(up == 0)
		return -1;
	*up++ = 0;
	if(*bp){
		crackurl(&base, bp, &defurl);
		current = &base;
	} else {
		current = &defurl;
	}

	geturl(up, GET, 0);
	return 0;
}

void
main(int argc, char **argv)
{
	int i;
	Ibuf *b;
	char body[4096];
	static Url base;

	body[0] = 0;
	ARGBEGIN {
	case 'o':
		ofile = ARGF();
		break;
	case 'b':
		crackurl(&base, ARGF(), &defurl);
		current = &base;
		break;
	case 'd':
		debug = 1;
		break;
	case 's':
		server = 1;
		break;
	case 'p':
		if(body[0] != 0)
			strcat(body, "&");
		strcat(body, ARGF());
		break;
	} ARGEND;

	if(server){
		b = ibufalloc(0);
		for(;;)
			if(dourl(b) < 0)
				break;
	} else {
		for(i = 0; i < argc; i++)
			geturl(argv[i], body[0]==0? GET: POST, body);
	}

	exits(0);
}
