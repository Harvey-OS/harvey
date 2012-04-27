#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

static	Hio		*hout;
static	Hio		houtb;
static	HConnect	*connect;

void	doconvert(char*, int);

void
error(char *title, char *fmt, ...)
{
	va_list arg;
	char buf[1024], *out;

	va_start(arg, fmt);
	out = vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	*out = 0;

	hprint(hout, "%s 404 %s\n", hversion, title);
	hprint(hout, "Date: %D\n", time(nil));
	hprint(hout, "Server: Plan9\n");
	hprint(hout, "Content-type: text/html\n");
	hprint(hout, "\n");
	hprint(hout, "<head><title>%s</title></head>\n", title);
	hprint(hout, "<body><h1>%s</h1></body>\n", title);
	hprint(hout, "%s\n", buf);
	hflush(hout);
	writelog(connect, "Reply: 404\nReason: %s\n", title);
	exits(nil);
}

typedef struct Hit	Hit;
struct Hit 
{
	Hit *next;
	char *file;
};

void
lookup(char *object, int section, Hit **list)
{
	int fd;
	char *p, *f;
	Biobuf b;
	char file[256];
	Hit *h;

	while(*list != nil)
		list = &(*list)->next;

	snprint(file, sizeof(file), "/sys/man/%d/INDEX", section);
	fd = open(file, OREAD);
	if(fd > 0){
		Binit(&b, fd, OREAD);
		for(;;){
			p = Brdline(&b, '\n');
			if(p == nil)
				break;
			p[Blinelen(&b)-1] = 0;
			f = strchr(p, ' ');
			if(f == nil)
				continue;
			*f++ = 0;
			if(strcmp(p, object) == 0){
				h = ezalloc(sizeof *h);
				*list = h;
				h->next = nil;
				snprint(file, sizeof(file), "/%d/%s", section, f);
				h->file = estrdup(file);
				close(fd);
				return;
			}
		}
		close(fd);
	}
	snprint(file, sizeof(file), "/sys/man/%d/%s", section, object);
	if(access(file, 0) == 0){
		h = ezalloc(sizeof *h);
		*list = h;
		h->next = nil;
		h->file = estrdup(file+8);
	}
}

void
manindex(int sect, int vermaj)
{
	int i;

	if(vermaj){
		hokheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}

	hprint(hout, "<head><title>plan 9 section index");
	if(sect)
		hprint(hout, "(%d)\n", sect);
	hprint(hout, "</title></head><body>\n");
	hprint(hout, "<H6>Section Index");
	if(sect)
		hprint(hout, "(%d)\n", sect);
	hprint(hout, "</H6>\n");

	if(sect)
		hprint(hout, "<p><a href=\"/plan9/man%d.html\">/plan9/man%d.html</a>\n",
			sect, sect);
	else for(i = 1; i < 10; i++)
		hprint(hout, "<p><a href=\"/plan9/man%d.html\">/plan9/man%d.html</a>\n",
			i, i);
	hprint(hout, "</body>\n");
}

void
man(char *o, int sect, int vermaj)
{
	int i;
	Hit *list;

	list = nil;

	if(*o == 0){
		manindex(sect, vermaj);
		return;
	}

	if(sect > 0 && sect < 10)
		lookup(o, sect, &list);
	else
		for(i = 1; i < 9; i++)
			lookup(o, i, &list);

	if(list != nil && list->next == nil){
		doconvert(list->file, vermaj);
		return;
	}

	if(vermaj){
		hokheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}

	hprint(hout, "<head><title>plan 9 man %H", o);
	if(sect)
		hprint(hout, "(%d)\n", sect);
	hprint(hout, "</title></head><body>\n");
	hprint(hout, "<H6>Search for %H", o);
	if(sect)
		hprint(hout, "(%d)\n", sect);
	hprint(hout, "</H6>\n");

	for(; list; list = list->next)
		hprint(hout, "<p><a href=\"/magic/man2html%U\">/magic/man2html%H</a>\n",
			list->file, list->file);
	hprint(hout, "</body>\n");
}

void
strlwr(char *p)
{
	for(; *p; p++)
		if('A' <= *p && *p <= 'Z')
			*p += 'a'-'A';
}

void
redirectto(char *uri)
{
	if(connect){
		hmoved(connect, uri);
		exits(0);
	}else
		hprint(hout, "Your selection moved to <a href=\"%U\"> here</a>.<p></body>\r\n", uri);
}

void
searchfor(char *search)
{
	int i, j, n, fd;
	char *p, *sp;
	Biobufhdr *b;
	char *arg[32];

	hprint(hout, "<head><title>plan 9 search for %H</title></head>\n", search);
	hprint(hout, "<body>\n");

	hprint(hout, "<p>This is a keyword search through Plan 9 man pages.\n");
	hprint(hout, "The search is case insensitive; blanks denote \"boolean and\".\n");
	hprint(hout, "<FORM METHOD=\"GET\" ACTION=\"/magic/man2html\">\n");
	hprint(hout, "<INPUT NAME=\"pat\" TYPE=\"text\" SIZE=\"60\">\n");
	hprint(hout, "<INPUT TYPE=\"submit\" VALUE=\"Submit\">\n");
	hprint(hout, "</FORM>\n");

	hprint(hout, "<hr><H6>Search for %H</H6>\n", search);
	n = getfields(search, arg, 32, 1, "+");
	for(i = 0; i < n; i++){
		for(j = i+1; j < n; j++){
			if(strcmp(arg[i], arg[j]) > 0){
				sp = arg[j];
				arg[j] = arg[i];
				arg[i] = sp;
			}
		}
		sp = malloc(strlen(arg[i]) + 2);
		if(sp != nil){
			strcpy(sp+1, arg[i]);
			sp[0] = ' ';
			arg[i] = sp;
		}
	}

	/*
	 *  search index till line starts alphabetically < first token
	 */
	fd = open("/sys/man/searchindex", OREAD);
	if(fd < 0){
		hprint(hout, "<body>error: No Plan 9 search index\n");
		hprint(hout, "</body>");
		return;
	}
	p = malloc(32*1024);
	if(p == nil){
		close(fd);
		return;
	}
	b = ezalloc(sizeof *b);
	Binits(b, fd, OREAD, (uchar*)p, 32*1024);
	for(;;){
		p = Brdline(b, '\n');
		if(p == nil)
			break;
		p[Blinelen(b)-1] = 0;
		for(i = 0; i < n; i++){
			sp = strstr(p, arg[i]);
			if(sp == nil)
				break;
			p = sp;
		}
		if(i < n)
			continue;
		sp = strrchr(p, '\t');
		if(sp == nil)
			continue;
		sp++;
		hprint(hout, "<p><a href=\"/magic/man2html/%U\">/magic/man2html/%H</a>\n",
			sp, sp);
	}
	hprint(hout, "</body>");

	Bterm(b);
	free(b);
	free(p);
	close(fd);
}

/*
 *  find man pages mentioning the search string
 */
void
dosearch(int vermaj, char *search)
{
	int sect;
	char *p;

	if(strncmp(search, "man=", 4) == 0){
		sect = 0;
		search = hurlunesc(connect, search+4);
		p = strchr(search, '&');
		if(p != nil){
			*p++ = 0;
			if(strncmp(p, "sect=", 5) == 0)
				sect = atoi(p+5);
		}
		man(search, sect, vermaj);
		return;
	}

	if(vermaj){
		hokheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}

	if(strncmp(search, "pat=", 4) == 0){
		search = hurlunesc(connect, search+4);
		search = hlower(search);
		searchfor(search);
		return;
	}

	hprint(hout, "<head><title>illegal search</title></head>\n");
	hprint(hout, "<body><p>Illegally formatted Plan 9 man page search</p>\n");
	search = hurlunesc(connect, search);
	hprint(hout, "<body><p>%H</p>\n", search);
	hprint(hout, "</body>");
}

/*
 *  convert a man page to html and output
 */
void
doconvert(char *uri, int vermaj)
{
	char *p;
	char file[256];
	char title[256];
	char err[ERRMAX];
	int pfd[2];
	Dir *d;
	Waitmsg *w;
	int x;

	if(strstr(uri, ".."))
		error("bad URI", "man page URI cannot contain ..");
	p = strstr(uri, "/intro");

	if(p == nil){
		while(*uri == '/')
			uri++;
		/* redirect section requests */
		snprint(file, sizeof(file), "/sys/man/%s", uri);
		d = dirstat(file);
		if(d == nil){
			strlwr(file);
			if(dirstat(file) != nil){
				snprint(file, sizeof(file), "/magic/man2html/%s", uri);
				strlwr(file);
				redirectto(file);
			}
			error(uri, "man page not found");
		}
		x = d->qid.type;
		free(d);
		if(x & QTDIR){
			if(*uri == 0 || strcmp(uri, "/") == 0)
				redirectto("/sys/man/index.html");
			else {
				snprint(file, sizeof(file), "/sys/man/%s/INDEX.html",
					uri+1);
				redirectto(file);
			}
			return;
		}
	} else {
		/* rewrite the name intro */
		*p = 0;
		snprint(file, sizeof(file), "/sys/man/%s/0intro", uri);
		d = dirstat(file);
		free(d);
		if(d == nil)
			error(uri, "man page not found");
	}

	if(vermaj){
		hokheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}
	hflush(hout);

	if(pipe(pfd) < 0)
		error("out of resources", "pipe failed");

	/* troff -manhtml <file> | troff2html -t '' */
	switch(fork()){
	case -1:
		error("out of resources", "fork failed");
	case 0:
		snprint(title, sizeof(title), "Plan 9 %s", file);
		close(0);
		dup(pfd[0], 0);
		close(pfd[0]);
		close(pfd[1]);
		execl("/bin/troff2html", "troff2html", "-t", title, nil);
		errstr(err, sizeof err);
		exits(err);
	}
	switch(fork()){
	case -1:
		error("out of resources", "fork failed");
	case 0:
		snprint(title, sizeof(title), "Plan 9 %s", file);
		close(0);
		close(1);
		dup(pfd[1], 1);
		close(pfd[0]);
		close(pfd[1]);
		execl("/bin/troff", "troff", "-manhtml", file, nil);
		errstr(err, sizeof err);
		exits(err);
	}
	close(pfd[0]);
	close(pfd[1]);

	/* wait for completion */
	for(;;){
		w = wait();
		if(w == nil)
			break;
		if(w->msg[0] != 0)
			print("whoops %s\n", w->msg);
		free(w);
	}
}

void
main(int argc, char **argv)
{
	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);

	if(argc == 2){
		hinit(&houtb, 1, Hwrite);
		hout = &houtb;
		doconvert(argv[1], 0);
		exits(nil);
	}
	close(2);

	connect = init(argc, argv);
	hout = &connect->hout;
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

	bind("/usr/web/sys/man", "/sys/man", MREPL);

	if(connect->req.search != nil)
		dosearch(connect->req.vermaj, connect->req.search);
	else
		doconvert(connect->req.uri, connect->req.vermaj);
	hflush(hout);
	writelog(connect, "200 man2html %ld %ld\n", hout->seek, hout->seek);
	exits(nil);
}
