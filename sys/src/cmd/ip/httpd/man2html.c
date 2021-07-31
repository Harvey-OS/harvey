#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"

enum
{
	SSIZE = 10,

	/* list types */
	Lordered = 1,
	Lunordered,
	Ldef,
	Lother,

};

static	Biobuf	bin;
static	Hio	houtb;
static	Hio	*hout;
static	Connect	*connect;
static	char	section[32];
static	char	onelinefont[32];

static	Biobuf in;
static	int sol;
static	char	me[] = "/magic/man2html/";

static	int list, listnum, indents, example, hangingdt;

typedef struct Goobie	Goobie;
struct Goobie 
{
	char *name;
	void (*f)(int, char**);
};

typedef void F(int, char**);
F g_1C, g_2C, g_B, g_BI, g_BR, g_DT, g_EE, g_EX, g_HP, g_I;
F g_IB, g_IP, g_IR, g_L, g_LP, g_LR, g_PD, g_PP, g_RB, g_RE, g_RI;
F g_RL, g_RS, g_SH, g_SM, g_SS, g_TF, g_TH, g_TP, g_br, g_ft, g_nf, g_fi;

F g_notyet, g_ignore; 

static Goobie gtab[] =
{
	{ "1C",	g_ignore, },
	{ "2C",	g_ignore, },
	{ "B",	g_B, },
	{ "BI",	g_BI, },
	{ "BR",	g_BR, },
	{ "DT",	g_ignore, },
	{ "EE",	g_EE, },
	{ "EX",	g_EX, },
	{ "HP",	g_HP, },
	{ "I",	g_I, },
	{ "IB",	g_IB, },
	{ "IP",	g_IP, },
	{ "IR",	g_IR, },
	{ "L",	g_L, },
	{ "LP",	g_LP, },
	{ "LR",	g_LR, },
	{ "PD",	g_ignore, },
	{ "PP",	g_PP, },
	{ "RB",	g_RB, },
	{ "RE",	g_RE, },
	{ "RI",	g_RI, },
	{ "RL",	g_RL, },
	{ "RS",	g_RS, },
	{ "SH",	g_SH, },
	{ "SM",	g_SM, },
	{ "SS",	g_SS, },
	{ "TF",	g_ignore, },
	{ "TH",	g_TH, },
	{ "TP",	g_TP, },

	{ "br",	g_br, },
	{ "ti",	g_br, },
	{ "nf",	g_nf, },
	{ "fi",	g_fi, },
	{ "ft",	g_ft, },
	{ nil, nil },
};


typedef struct Troffspec	Troffspec;
struct Troffspec 
{
	char *name;
	char *value;
};

static Troffspec tspec[] =
{
	{ "ff", "ff", },
	{ "fi", "fi", },
	{ "fl", "fl", },
	{ "Fi", "ffi", },
	{ "ru", "_", },
	{ "em", "&#173;", },
	{ "14", "&#188;", },
	{ "12", "&#189;", },
	{ "co", "&#169;", },
	{ "de", "&#176;", },
	{ "dg", "&#161;", },
	{ "fm", "&#180;", },
	{ "rg", "&#174;", },
	{ "bu", "*", },
	{ "sq", "&#164;", },
	{ "hy", "-", },
	{ "pl", "+", },
	{ "mi", "-", },
	{ "mu", "&#215;", },
	{ "di", "&#247;", },
	{ "eq", "=", },
	{ "==", "==", },
	{ ">=", ">=", },
	{ "<=", "<=", },
	{ "!=", "!=", },
	{ "+-", "&#177;", },
	{ "no", "&#172;", },
	{ "sl", "/", },
	{ "ap", "&", },
	{ "~=", "~=", },
	{ "pt", "oc", },
	{ "gr", "GRAD", },
	{ "->", "->", },
	{ "<-", "<-", },
	{ "ua", "^", },
	{ "da", "v", },
	{ "is", "Integral", },
	{ "pd", "DIV", },
	{ "if", "oo", },
	{ "sr", "-/", },
	{ "sb", "(~", },
	{ "sp", "~)", },
	{ "cu", "U", },
	{ "ca", "(^)", },
	{ "ib", "(=", },
	{ "ip", "=)", },
	{ "mo", "C", },
	{ "es", "&Oslash;", },
	{ "aa", "&#180;", },
	{ "ga", "`", },
	{ "ci", "O", },
	{ "L1", "DEATHSTAR", },
	{ "sc", "&#167;", },
	{ "dd", "++", },
	{ "lh", "<=", },
	{ "rh", "=>", },
	{ "lt", "(", },
	{ "rt", ")", },
	{ "lc", "|", },
	{ "rc", "|", },
	{ "lb", "(", },
	{ "rb", ")", },
	{ "lf", "|", },
	{ "rf", "|", },
	{ "lk", "|", },
	{ "rk", "|", },
	{ "bv", "|", },
	{ "ts", "s", },
	{ "br", "|", },
	{ "or", "|", },
	{ "ul", "_", },
	{ "rn", " ", },
	{ "**", "*", },
	{ nil, nil, },
};

static	char *curfont;
static	char token[128];
static	int lastc = '\n';

void	closel(void);
void	closeall(void);
void	doconvert(char*, int, int);

/* get next logical character.  expand it with escapes */
char*
getnext(void)
{
	int r, mult, size;
	Rune rr;
	Htmlesc *e;
	Troffspec *t;
	char buf[32];

	if(lastc == '\n')
		sol = 1;
	else
		sol = 0;
	r = Bgetrune(&in);
	if(r < 0)
		return nil;
	lastc = r;
	if(r > 128){
		for(e = htmlesc; e->name; e++)
			if(e->value == r)
				return e->name;
		rr = r;
		runetochar(buf, &rr);
		for(r = 0; r < runelen(rr); r++)
			snprint(token + 3*r, sizeof(token)-3*r, "%%%2.2ux", buf[r]);
		return token;
	}
	switch(r){
	case '\\':
		r = Bgetrune(&in);
		if(r < 0)
			return nil;
		lastc = r;
		switch(r){
		/* chars to ignore */
		case '|':
		case '&':
			return getnext();

		/* ignore arg */
		case 'k':
			Bgetrune(&in);
			return getnext();

		/* defined strings */
		case '*':
			switch(Bgetrune(&in)){
			case 'R':
				return "&#174;";
			}
			return getnext();

		/* special chars */
		case '(':
			token[0] = Bgetc(&in);
			token[1] = Bgetc(&in);
			token[2] = 0;
			for(t = tspec; t->name; t++)
				if(strcmp(token, t->name) == 0)
					return t->value;
			return "&#191;";
		case 'c':
			r = Bgetrune(&in);
			if(r == '\n'){
				lastc = r;
				return getnext();
			}
			break;
		case 'e':
			return "\\";
			break;
		case 'f':
			lastc = r = Bgetrune(&in);
			switch(r){
			case '2':
			case 'B':
				strcpy(token, "<TT>");
				if(curfont)
					snprint(token, sizeof(token), "%s<TT>", curfont);
				curfont = "</TT>";
				return token;
			case '3':
			case 'I':
				strcpy(token, "<I>");
				if(curfont)
					snprint(token, sizeof(token), "%s<I>", curfont);
				curfont = "</I>";
				return token;
			case 'L':
				strcpy(token, "<TT>");
				if(curfont)
					snprint(token, sizeof(token), "%s<TT>", curfont);
				curfont = "</TT>";
				return token;
			default:
				token[0] = 0;
				if(curfont)
					strcpy(token, curfont);
				curfont = nil;
				return token;
			}
		case 's':
			mult = 1;
			size = 0;
			for(;;){
				r = Bgetc(&in);
				if(r < 0)
					return nil;
				if(r == '+')
					;
				else if(r == '-')
					mult *= -1;
				else if(r >= '0' && r <= '9')
					size = size*10 + (r-'0');
				else{
					Bungetc(&in);
					break;
				}
				lastc = r;
			}
			break;
		case ' ':
			return "&#32;";
		}
		break;
	case '<':
		return example ? "<" : "&#60;";
		break;
	case '>':
		return example ? ">" : "&#62;";
		break;
	}
	token[0] = r;
	token[1] = 0;
	return token;
}

enum
{
	Narg = 32,
	Nline = 1024,
	Maxget = 10,
};

void
dogoobie(void)
{
	char *p, *np, *e;
	Goobie *g;
	char line[Nline];
	int argc;
	char *argv[Narg];

	/* read line, translate special chars */
	e = line + sizeof(line) - Maxget;
	for(p = line; p < e; ){
		np = getnext();
		if(np == nil)
			return;
		if(np[0] == '\n')
			break;
		if(np[1]) {
			strcpy(p, np);
			p += strlen(np);
		} else
			*p++ = np[0];
	}
	*p = 0;

	/* parse into arguments */
	p = line;
	for(argc = 0; argc < Narg; argc++){
		while(*p == ' ' || *p == '\t')
			*p++ = 0;
		if(*p == 0)
			break;
		if(*p == '"'){
			*p++ = 0;
			argv[argc] = p;
			while(*p && *p != '"')
				p++;
			if(*p == '"')
				*p++ = 0;
		} else {
			argv[argc] = p;
			while(*p && *p != ' ' && *p != '\t')
				p++;
		}
	}
	argv[argc] = nil;

	if(argc == 0)
		return;

	for(g = gtab; g->name; g++)
		if(strncmp(g->name, argv[0], 2) == 0){
			(*g->f)(argc, argv);
			return;
		}

	fprint(2, "unknown directive %s\n", line);
}

void
printargs(int argc, char **argv)
{
	argc--;
	argv++;
	while(--argc > 0)
		hprint(hout, "%s ", *(argv++));
	if(argc == 0)
		hprint(hout, "%s", *argv);
}

void
error(char *title, char *fmt, ...)
{
	va_list arg;
	char buf[1024], *out;

	va_start(arg, fmt);
	out = doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	*out = 0;

	hprint(hout, "%s 404 %s\n", version, title);
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
	char file[4*NAMELEN];
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
		okheaders(connect);
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
		doconvert(list->file, vermaj, 0);
		return;
	}

	if(vermaj){
		okheaders(connect);
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
redirectto(char *uri)
{
	if(connect)
		moved(connect, uri);
	else
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
		search = urlunesc(search+4);
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
		okheaders(connect);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}

	if(strncmp(search, "pat=", 4) == 0){
		search = urlunesc(search+4);
		search = lower(search);
		searchfor(search);
		return;
	}

	hprint(hout, "<head><title>illegal search</title></head>\n");
	hprint(hout, "<body><p>Illegally formatted Plan 9 man page search</p>\n");
	search = urlunesc(search);
	hprint(hout, "<body><p>%H</p>\n", search);
	hprint(hout, "</body>");
}

void
dohangingdt(void)
{
	switch(hangingdt){
	case 3:
		hangingdt--;
		break;
	case 2:
		hprint(hout, "<dd>");
		hangingdt = 0;
		break;
	}
}

/*
 *  finish any one line font changes
 */
void
dooneliner(void)
{
	if(onelinefont[0] != 0)
		hprint(hout, "%s", onelinefont);
	onelinefont[0] = 0;
}

/*
 *  convert a man page to html and output
 */
void
doconvert(char *uri, int vermaj, int stdin)
{
	int fd;
	char c, *p;
	char file[256];
	Dir d;

	if(stdin){
		fd = 0;
	} else {
		if(strstr(uri, ".."))
			error("bad URI", "man page URI cannot contain ..");
		p = strstr(uri, "/intro");
		if(p == nil){
			snprint(file, sizeof(file), "/sys/man/%s", uri);
			if(dirstat(file, &d) >= 0 && (d.qid.path & CHDIR)){
				if(*uri == 0 || strcmp(uri, "/") == 0)
					redirectto("/plan9/vol1.html");
				else {
					snprint(file, sizeof(file), "/plan9/man%s.html",
						uri+1);
					redirectto(file);
				}
				return;
			}
			if(*uri == '/')
				uri++;
			snprint(section, sizeof(section), uri);
			p = strstr(section, "/");
			if(p != nil)
				*p = 0;
		} else {
			*p = 0;
			snprint(file, sizeof(file), "/sys/man/%s/0intro", uri);
			if(*uri == '/')
				uri++;
			snprint(section, sizeof(section), uri);
		}
		fd = open(file, OREAD);
		if(fd < 0)
			error("non-existent man page", "cannot open %H: %r", file);
	
		if(vermaj){
			okheaders(connect);
			hprint(hout, "Content-type: text/html\r\n");
			hprint(hout, "\r\n");
		}
	
	}

	Binit(&in, fd, OREAD);
	hprint(hout, "<html>\n<head>\n");

	for(;;){
		p = getnext();
		if(p == nil)
			break;
		c = *p;
		if(c == '.' && sol){
			dogoobie();
			dohangingdt();
		} else if(c == '\n'){
			dooneliner();
			hprint(hout, "%s", p);
			dohangingdt();
		} else
			hprint(hout, "%s", p);
	}
	closeall();
	hprint(hout, "<br><font size=1><A href=http://www.lucent.com/copyright.html>\n");
	hprint(hout, "Copyright</A> &#169; 2000 Lucent Technologies.  All rights reserved.</font>\n");
	hprint(hout, "</body></html>\n");
}

void
main(int argc, char **argv)
{

	fmtinstall('H', httpconv);
	fmtinstall('U', urlconv);

	if(argc == 2 && strcmp(argv[1], "-") == 0){
		hinit(&houtb, 1, Hwrite);
		hout = &houtb;
		doconvert(nil, 0, 1);
		exits(nil);
	}
	close(2);

	connect = init(argc, argv);
	hout = &connect->hout;
	httpheaders(connect);

	if(strcmp(connect->req.meth, "GET") != 0 && strcmp(connect->req.meth, "HEAD") != 0)
		unallowed(connect, "GET, HEAD");
	if(connect->head.expectother || connect->head.expectcont)
		fail(connect, ExpectFail, nil);

	anonymous(connect);

	if(connect->req.search != nil)
		dosearch(connect->req.vermaj, connect->req.search);
	else
		doconvert(connect->req.uri, connect->req.vermaj, 0);
	hflush(hout);
	writelog(connect, "200 man2html %ld %ld\n", hout->seek, hout->seek);
	exits(nil);
}

void
g_notyet(int, char **argv)
{
	fprint(2, ".%s not yet supported\n", argv[0]);
}

void
g_ignore(int, char**)
{
}

void
g_PP(int, char**)
{
	closel();
	hprint(hout, "<P>\n");
}

/* close a list */
void
closel(void)
{
	switch(list){
	case Lordered:
		hprint(hout, "</ol>\n");
		break;
	case Lunordered:
		hprint(hout, "</ul>\n");
		break;
	case Lother:
	case Ldef:
		hprint(hout, "</dl>\n");
		break;
	}
	list = 0;
	
}

void
closeall(void)
{
	closel();
	while(indents > 0){
		indents--;
		hprint(hout, "</DL>\n");
	}
}

void
g_IP(int argc, char **argv)
{
	switch(list){
	default:
		closel();
		if(argc > 1){
			if(strcmp(argv[1], "1") == 0){
				list = Lordered;
				listnum = 1;
				hprint(hout, "<OL>\n");
			} else if(strcmp(argv[1], "*") == 0){
				list = Lunordered;
				hprint(hout, "<UL>\n");
			} else {
				list = Lother;
				hprint(hout, "<DL>\n");
			}
		} else {
			list = Lother;
			hprint(hout, "<DL>\n");
		}
		break;
	case Lother:
	case Lordered:
	case Lunordered:
		break;
	}

	switch(list){
	case Lother:
		hprint(hout, "<DT>");
		printargs(argc, argv);
		hprint(hout, "<DD>\n");
		break;
	case Lordered:
	case Lunordered:
		hprint(hout, "<LI>\n");
		break;
	}
}

void
g_TP(int, char**)
{
	switch(list){
	default:
		closel();
		list = Ldef;
		hangingdt = 3;
		hprint(hout, "<DL><DT>\n");
		break;
	case Ldef:
		if(hangingdt)
			hprint(hout, "<DD>");
		hprint(hout, "<DT>");
		hangingdt = 3;
		break;
	}
}

void
g_HP(int, char**)
{
	switch(list){
	default:
		closel();
		list = Ldef;
		hangingdt = 1;
		hprint(hout, "<DL><DT>\n");
		break;
	case Ldef:
		if(hangingdt)
			hprint(hout, "<DD>");
		hprint(hout, "<DT>");
		hangingdt = 1;
		break;
	}
}

void
g_LP(int argc, char **argv)
{
	g_PP(argc, argv);
}

void
g_TH(int argc, char **argv)
{
	if(argc > 1){
		hprint(hout, "<title>");
		if(argc > 2)
			hprint(hout, "Plan 9's %s(%s)\n", argv[1], argv[2]);
		else
			hprint(hout, "Plan 9's %s()\n", argv[1]);
		hprint(hout, "</title>\n");
		hprint(hout, "</head><body>\n");
		hprint(hout, "<B>[<A href=/sys/man/index.html>manual index</A>]</B>");
		hprint(hout, "<B>[<A href=/sys/man/%s/INDEX.html>section index</A>]</B>\n", section);
	} else
		hprint(hout, "</head><body>\n");
}

void
g_SH(int argc, char **argv)
{
	closeall();
	indents++;
	hprint(hout, "<H4>");
	printargs(argc, argv);
	hprint(hout, "</H4>\n");
	hprint(hout, "<DL><DT><DD>\n");
}

void
g_SS(int argc, char **argv)
{
	closeall();
	indents++;
	hprint(hout, "<H5>");
	printargs(argc, argv);
	hprint(hout, "</H5>\n");
	hprint(hout, "<DL><DT><DD>\n");
}

void
font(char *f, int argc, char **argv)
{
	if(argc < 2){
		hprint(hout, "<%s>", f);
		sprint(onelinefont, "</%s>", f);
	} else {
		hprint(hout, "<%s>", f);
		printargs(argc, argv);
		hprint(hout, "</%s>\n", f);
	}
}

void
altfont(char *f1, char *f2, int argc, char **argv)
{
	char *f;
	int i;

	argc--;
	argv++;
	for(i = 0; i < argc; i++){
		if(i & 1)
			f = f2;
		else
			f = f1;
		if(strcmp(f, "R") == 0)
			hprint(hout, "%s", argv[i]);
		else
			hprint(hout, "<%s>%s</%s>", f, argv[i], f);
	}
	hprint(hout, "\n");
}

void
g_B(int argc, char **argv)
{
	font("TT", argc, argv);
}

void
g_BI(int argc, char **argv)
{
	altfont("TT", "I", argc, argv);
}

void
g_BR(int argc, char **argv)
{
	altfont("TT", "R", argc, argv);
}

void
g_RB(int argc, char **argv)
{
	altfont("R", "TT", argc, argv);
}

void
g_I(int argc, char **argv)
{
	font("I", argc, argv);
}

void
g_IB(int argc, char **argv)
{
	altfont("I", "TT", argc, argv);
}

void
g_IR(int argc, char **argv)
{
	int anchor;
	char *p;

	anchor = 0;
	if(argc > 2){
		p = argv[2];
		if(p[0] == '(' && p[1] >= '1' && p[1] <= '9' && p[2] == ')')
			anchor = 1;
		if(anchor)
			hprint(hout, "<A href=\"/magic/man2html/%c/%U\">",
				p[1], httpunesc(lower(argv[1])));
	}
	altfont("I", "R", argc, argv);
	if(anchor)
		hprint(hout, "</A>");
}

void
g_RI(int argc, char **argv)
{
	altfont("R", "I", argc, argv);
}

void
g_br(int, char**)
{
	if(hangingdt){
		hprint(hout, "<dd>");
		hangingdt = 0;
	}else
		hprint(hout, "<br>\n");
}

void
g_nf(int, char**)
{
	example = 1;
	hprint(hout, "<PRE>\n");
}

void
g_fi(int, char**)
{
	example = 0;
	hprint(hout, "</PRE>\n");
}

void
g_EX(int, char**)
{
	example = 1;
	hprint(hout, "<TT><XMP>\n");
}

void
g_EE(int, char**)
{
	example = 0;
	hprint(hout, "</XMP></TT>\n");
}

void
g_L(int argc, char **argv)
{
	font("TT", argc, argv);
}

void
g_LR(int argc, char **argv)
{
	altfont("TT", "R", argc, argv);
}

void
g_RL(int argc, char **argv)
{
	altfont("R", "TT", argc, argv);
}

void
g_RS(int, char**)
{
	hprint(hout, "<DL><DT><DD>\n");
}

void
g_RE(int, char**)
{
	hprint(hout, "</DL>\n");
}

void
g_SM(int argc, char **argv)
{
	if(argc > 1)
		hprint(hout, "%s ", argv[1]);
}

void
g_ft(int argc, char **argv)
{
	char *font;

	if(argc < 2)
		return;

	if(curfont)
		hprint(hout, "%s", curfont);

	switch(*argv[1]){
	case '2':
	case 'B':
		font = "B";
		curfont = "</B>";
		break;
	case '3':
	case 'I':
		font = "I";
		curfont = "</I>";
		break;
	case 'L':
		font = "TT";
		curfont = "</TT>";
		break;
	default:
		curfont = nil;
		return;
	}
	hprint(hout, "<%s>", font);
}
