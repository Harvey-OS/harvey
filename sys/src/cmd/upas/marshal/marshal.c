#include "common.h"
#include <ctype.h>

typedef struct Attach Attach;
typedef struct Alias Alias;
typedef struct Addr Addr;
typedef struct Ctype Ctype;

struct Attach {
	Attach	*next;
	char	*path;
	char	*type;
	int	inline;
	Ctype	*ctype;
};

struct Alias
{
	Alias	*next;
	int	n;
	char	**v;
};

struct Addr
{
	Addr	*next;
	char	*v;
};

enum {
	Hfrom,
	Hto,
	Hcc,
	Hbcc,
	Hsender,
	Hreplyto,
	Hdate,
	Hsubject,
	Hmime,
	Hpriority,
	Hmsgid,
	Hcontent,
	Hx,
	Nhdr,
};

char *hdrs[Nhdr] = {
[Hfrom]		"from:",
[Hto]		"to:",
[Hcc]		"cc:",
[Hbcc]		"bcc:",
[Hreplyto]	"reply-to:",
[Hsender]	"sender:",
[Hdate]		"date:",
[Hsubject]	"subject:",
[Hpriority]	"priority:",
[Hmsgid]	"message-id:",
[Hmime]		"mime-",
[Hcontent]	"content-",
[Hx]		"x-",
};

struct Ctype {
	char	*type;
	char 	*ext;
	int	display;
};

Ctype ctype[] = {
	{ "text/plain",			"txt",	1,	},
	{ "text/html",			"html",	1,	},
	{ "text/html",			"htm",	1,	},
	{ "text/tab-separated-values",	"tsv",	1,	},
	{ "text/richtext",		"rtx",	1,	},
	{ "message/rfc822",		"txt",	1,	},
	{ "image/jpeg",			"jpg",	0,	},
	{ "image/jpeg",			"jpeg",	0,	},
	{ "image/gif",			"gif",	0,	},
	{ "application/pdf",		"pdf",	0,	},
	{ "application/postscript",	"ps",	0,	},
	{ "text/plain",			"c",	1,	},	// c language
	{ "text/plain",			"s",	1,	},	// asm
	{ "text/plain",			"b",	1,	},	// limbo language
	{ "", 				0,	0,	},
};

int pid = -1;

Attach*	mkattach(char*, char*, int);
int	readheaders(Biobuf*, int*, String**, Addr**);
void	body(Biobuf*, Biobuf*, int);
char*	mkboundary(void);
int	printdate(Biobuf*);
int	printfrom(Biobuf*);
int	printto(Biobuf*, Addr*);
int	printsubject(Biobuf*, char*);
int	sendmail(Addr*, int*, char*);
void	attachment(Attach*, Biobuf*);
int	cistrncmp(char*, char*, int);
int	cistrcmp(char*, char*);
char*	waitforsendmail(int);
int	enc64(char*, int, uchar*, int);
Addr*	expand(int, char**);
Alias*	readaliases(void);
Addr*	expandline(String**, Addr*);
void	Bdrain(Biobuf*);
void	freeaddr(Addr *);

int rflag, lbflag, xflag, holding, nflag, Fflag, eightflag;
char *user;
char *login;
Alias *aliases;
int rfc822syntaxerror;
char lastchar;

enum
{
	Ok = 0,
	Nomessage = 1,
	Nobody = 2,
	Error = -1,
};

void
usage(void)
{
	fprint(2, "usage: %s [-Fr#xn] [-s subject] [-t type] [-aA attachment] -8 | recipient-list\n",
		argv0);
	exits("usage");
}

void
fatal(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	if(pid >= 0)
		postnote(PNPROC, pid, "die");

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", argv0, buf);
	holdoff(holding);
	exits(buf);
}

void
main(int argc, char **argv)
{
	Attach *first, **l, *a;
	char *subject, *type, *boundary;
	int flags, fd;
	Biobuf in, out, *b;
	Addr *to;
	String *file, *hdrstring;
	int noinput, headersrv;

	noinput = 0;
	subject = nil;
	first = nil;
	l = &first;
	type = nil;
	hdrstring = nil;
	ARGBEGIN{
	case 't':
		type = ARGF();
		if(type == nil)
			usage();
		break;
	case 'a':
		flags = 0;
		goto aflag;
	case 'A':
		flags = 1;
	aflag:
		a = mkattach(ARGF(), type, flags);
		if(a == nil)
			exits("bad args");
		type = nil;
		*l = a;
		l = &a->next;
		break;
	case 's':
		subject = ARGF();
		break;
	case 'F':
		Fflag = 1;		// file message
		break;
	case 'r':
		rflag = 1;		// for sendmail
		break;
	case '#':
		lbflag = 1;		// for sendmail
		break;
	case 'x':
		xflag = 1;		// for sendmail
		break;
	case 'n':			// no standard input
		nflag = 1;
		break;
	case '8':			// read recipients from rfc822 header
		eightflag = 1;
		break;
	default:
		usage();
		break;
	}ARGEND;

	login = getlog();
	user = getenv("upasname");
	if(user == nil || *user == 0)
		user = login;
	if(user == nil || *user == 0)
		sysfatal("can't read user name");

	if(Binit(&in, 0, OREAD) < 0)
		sysfatal("can't Binit 0: %r");

	if(nflag && eightflag)
		sysfatal("can't use both -n and -8");
	if(eightflag && argc >= 1)
		usage();
	else if(!eightflag && argc < 1)
		usage();

	aliases = readaliases();
	if(!eightflag)
		to = expand(argc, argv);
	else
		to = nil;

	flags = 0;
	headersrv = Nomessage;
	if(!nflag && !xflag && !lbflag) {
		// pass through headers, keeping track of which we've seen,
		// perhaps building to list.
		holding = holdon();
		headersrv = readheaders(&in, &flags, &hdrstring, eightflag ? &to : nil);
		if(rfc822syntaxerror){
			Bdrain(&in);
			fatal("rfc822 syntax error, message not sent");
		}
		if(to == nil){
			Bdrain(&in);
			fatal("no addresses found, message not sent");
		}

		switch(headersrv){
		case Error:		// error
			fatal("reading");
			break;
		case Nomessage:		// no message, just exit mimicking old behavior
			noinput = 1;
			if(first == nil)
				exits(0);
			break;
		}
	}

	fd = sendmail(to, &pid, Fflag ? argv[0] : nil);
	if(fd < 0)
		sysfatal("execing sendmail: %r\n:");
	if(xflag || lbflag){
		close(fd);
		exits(waitforsendmail(pid));
	}
	
	if(Binit(&out, fd, OWRITE) < 0)
		fatal("can't Binit 1: %r");

	if(!nflag){
		if(Bwrite(&out, s_to_c(hdrstring), s_len(hdrstring)) != s_len(hdrstring))
			fatal("write error");
		s_free(hdrstring);
		hdrstring = nil;

		// read user's standard headers
		file = s_new();
		mboxpath("headers", user, file, 0);
		b = Bopen(s_to_c(file), OREAD);
		if(b != nil){
			switch(readheaders(b, &flags, &hdrstring, nil)){
			case Error:	// error
				fatal("reading");
			}
			Bterm(b);
			if(Bwrite(&out, s_to_c(hdrstring), s_len(hdrstring)) != s_len(hdrstring))
				fatal("write error");
			s_free(hdrstring);
			hdrstring = nil;
		}
	}

	// add any headers we need
	if((flags & (1<<Hdate)) == 0)
		if(printdate(&out) < 0)
			fatal("writing");
	if((flags & (1<<Hfrom)) == 0)
		if(printfrom(&out) < 0)
			fatal("writing");
	if((flags & (1<<Hto)) == 0)
		if(printto(&out, to) < 0)
			fatal("writing");
	if((flags & (1<<Hsubject)) == 0 && subject != nil)
		if(printsubject(&out, subject) < 0)
			fatal("writing");
	Bprint(&out, "MIME-Version: 1.0\n");

	// if attachments, stick in multipart headers
	boundary = nil;
	if(first != nil){
		boundary = mkboundary();
		Bprint(&out, "Content-Type: multipart/mixed;\n");
		Bprint(&out, "\tboundary=\"%s\"\n\n", boundary);
		Bprint(&out, "This is a multi-part message in MIME format.\n");
		Bprint(&out, "--%s\n", boundary);
		Bprint(&out, "Content-Disposition: inline\n");
	}

	if(!nflag){
		if(!noinput && headersrv == Ok){
			body(&in, &out, 1);
		}
	} else
		Bprint(&out, "\n");
	holdoff(holding);

	Bflush(&out);
	for(a = first; a != nil; a = a->next){
		if(lastchar != '\n')
			Bprint(&out, "\n");
		Bprint(&out, "--%s\n", boundary);
		attachment(a, &out);
	}

	if(first != nil)
		Bprint(&out, "--%s--\n", boundary);

	Bterm(&out);
	close(fd);
	exits(waitforsendmail(pid));
}

// read headers from stdin into a String, expanding local aliases,
// keep track of which headers are there, which addresses we have
// remove Bcc: line.
int
readheaders(Biobuf *in, int *fp, String **sp, Addr **top)
{
	Addr *to;
	String *s, *sline;
	char *p;
	int i, seen, hdrtype;

	s = s_new();
	sline = nil;
	to = nil;
	hdrtype = -1;
	seen = 0;
	for(;;) {
		if((p = Brdline(in, '\n')) != nil) {
			seen = 1;
			p[Blinelen(in)-1] = 0;
			if((*p == ' ' || *p == '\t') && sline){
				s_append(sline, "\n");
				s_append(sline, p);
				p[Blinelen(in)-1] = '\n';
				continue;
			}
		}

		if(sline) {
			assert(hdrtype != -1);
			if(top){
				switch(hdrtype){
				case Hto:
				case Hcc:
				case Hbcc:
					to = expandline(&sline, to);
					break;
				}
			}
			if(top==nil || hdrtype!=Hbcc){
				s_append(s, s_to_c(sline));
				s_append(s, "\n");
			}
			s_free(sline);
			sline = nil;
		}

		if(p == nil)
			break;

		if(strchr(p, ':') == nil){
			p[Blinelen(in)-1] = '\n';
			Bseek(in, -Blinelen(in), 1);
			break;
		}

		sline = s_copy(p);

		hdrtype = -1;
		for(i = 0; i < nelem(hdrs); i++){
			if(cistrncmp(hdrs[i], p, strlen(hdrs[i])) == 0){
				*fp |= 1<<i;
				hdrtype = i;
				break;
			}
		}
		if(hdrtype == -1){
			p[Blinelen(in)-1] = '\n';
			Bseek(in, -Blinelen(in), 1);
			break;
		}
		p[Blinelen(in)-1] = '\n';
	}

	*sp = s;
	if(top)
		*top = to;

	if(seen == 0)
		return Nomessage;
	if(p == nil)
		return Nobody;
	return Ok;
}

// pass the body to sendmail, make sure body starts with a newline
void
body(Biobuf *in, Biobuf *out, int docontenttype)
{
	char *buf, *p;
	int i, n, len;

	n = 0;
	len = 16*1024;
	buf = malloc(len);
	if(buf == nil)
		fatal("%r");

	// first char must be newline
	i = Bgetc(in);
	if(i < 0)
		fatal("input error1");
	if(i != '\n')
		buf[n++] = '\n';
	buf[n++] = i;

	// read into memory
	if(docontenttype){
		while(docontenttype){
			if(n == len){
				len += len>>2;
				buf = realloc(buf, len);
				if(buf == nil)
					sysfatal("%r");
			}
			p = buf+n;
			i = Bread(in, p, len - n);
			if(i < 0)
				fatal("input error2");
			if(i == 0)
				break;
			n += i;
			for(; i > 0; i--)
				if(*p++ & 0x80 && docontenttype){
					Bprint(out, "Content-Type: text/plain; charset=\"UTF-8\"\n");
					Bprint(out, "Content-Transfer-Encoding: 8bit\n");
					docontenttype = 0;
					break;
				}
		}
		if(docontenttype){
			Bprint(out, "Content-Type: text/plain; charset=\"US-ASCII\"\n");
			Bprint(out, "Content-Transfer-Encoding: 7bit\n");
		}
	}

	// write what we already read
	if(Bwrite(out, buf, n) < 0)
		fatal("output error");
	if(n > 0)
		lastchar = buf[n-1];
	else
		lastchar = '\n';


	// pass the rest
	for(;;){
		n = Bread(in, buf, len);
		if(n < 0)
			fatal("input error2");
		if(n == 0)
			break;
		if(Bwrite(out, buf, n) < 0)
			fatal("output error");
		lastchar = buf[n-1];
	}
}

// pass the body to sendmail encoding with base64
//
//  the size of buf is very important to enc64.  Anything other than
//  a multiple of 3 will cause enc64 to output a termination sequence.
//  To ensure that a full buf corresponds to a multiple of complete lines,
//  we make buf a multiple of 3*18 since that's how many enc64 sticks on
//  a single line.  This avoids short lines in the output which is pleasing
//  but not necessary.
//
void
body64(Biobuf *in, Biobuf *out)
{
	uchar buf[3*18*54];
	char obuf[3*18*54*2];
	int m, n;

	Bprint(out, "\n");
	for(;;){
		n = Bread(in, buf, sizeof(buf));
		if(n < 0)
			fatal("input error");
		if(n == 0)
			break;
		m = enc64(obuf, sizeof(obuf), buf, n);
		if(Bwrite(out, obuf, m) < 0)
			fatal("output error");
	}
	lastchar = '\n';
}

// pass message to sendmail, make sure body starts with a newline
void
copy(Biobuf *in, Biobuf *out)
{
	char buf[4*1024];
	int n;

	for(;;){
		n = Bread(in, buf, sizeof(buf));
		if(n < 0)
			fatal("input error");
		if(n == 0)
			break;
		if(Bwrite(out, buf, n) < 0)
			fatal("output error");
	}
}

void
attachment(Attach *a, Biobuf *out)
{
	Biobuf *f;
	char *p;

	// if it's already mime encoded, just copy
	if(strcmp(a->type, "mime") == 0){
		f = Bopen(a->path, OREAD);
		if(f == nil){
			/* hack: give marshal time to stdin, before we kill it (for dead.letter) */
			sleep(500);
			postnote(PNPROC, pid, "interrupt");
			sysfatal("opening %s: %r", a->path);
		}
		copy(f, out);
		Bterm(f);
	}
	
	// if it's not already mime encoded ...
	if(strcmp(a->type, "text/plain") != 0)
		Bprint(out, "Content-Type: %s\n", a->type);

	if(a->inline){
		Bprint(out, "Content-Disposition: inline\n");
	} else {
		p = strrchr(a->path, '/');
		if(p == nil)
			p = a->path;
		else
			p++;
		Bprint(out, "Content-Disposition: attachment; filename=%s\n", p);
	}

	f = Bopen(a->path, OREAD);
	if(f == nil){
		/* hack: give marshal time to stdin, before we kill it (for dead.letter) */
		sleep(500);
		postnote(PNPROC, pid, "interrupt");
		sysfatal("opening %s: %r", a->path);
	}
	if(a->ctype->display){
		body(f, out, strcmp(a->type, "text/plain") == 0);
	} else {
		Bprint(out, "Content-Transfer-Encoding: base64\n");
		body64(f, out);
	}
	Bterm(f);
}

char *ascwday[] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *ascmon[] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int
printdate(Biobuf *b)
{
	Tm *tm;
	int tz;

	tm = localtime(time(0));
	tz = (tm->tzoff/3600)*100 + ((tm->tzoff/60)%60);

	return Bprint(b, "Date: %s, %d %s %d %2.2d:%2.2d:%2.2d %s%.4d\n",
		ascwday[tm->wday], tm->mday, ascmon[tm->mon], 1900+tm->year,
		tm->hour, tm->min, tm->sec, tz>0?"+":"", tz);
}

int
printfrom(Biobuf *b)
{
	return Bprint(b, "From: %s\n", user);
}

int
printto(Biobuf *b, Addr *to)
{
	if(Bprint(b, "To: %s", to->v) < 0)
		return -1;
	for(to = to->next; to != nil; to = to->next)
		if(Bprint(b, ", %s", to->v) < 0)
			return -1;
	if(Bprint(b, "\n") < 0)
		return -1;
}

int
printsubject(Biobuf *b, char *subject)
{
	return Bprint(b, "Subject: %s\n", subject);
}

Attach*
mkattach(char *file, char *type, int inline)
{
	Ctype *c;
	Attach *a;
	char ftype[64];
	char *p;
	int n, pfd[2];

	if(file == nil)
		return nil;
	a = malloc(sizeof(*a));
	if(a == nil)
		return nil;
	a->path = file;
	a->next = nil;
	a->type = type;
	a->inline = inline;
	a->ctype = nil;
	if(type != nil){
		for(c = ctype; ; c++)
			if(strncmp(type, c->type, strlen(c->type)) == 0){
				a->ctype = c;
				break;
			}
		return a;
	}

	// pick a type depending on extension
	p = strchr(file, '.');
	if(p != nil){
		p++;
		for(c = ctype; c->ext != nil; c++)
			if(strcmp(p, c->ext) == 0){
				a->type = c->type;
				a->ctype = c;
				return a;
			}
	}

	// run file to figure out the type
	a->type = "application/octet-stream";		// safest default
	if(pipe(pfd) < 0)
		return a;
	switch(fork()){
	case -1:
		break;
	case 0:
		close(pfd[1]);
		close(0);
		dup(pfd[0], 0);
		close(1);
		dup(pfd[0], 1);
		execl("/bin/file", "file", "-m", file, 0);
		exits(0);
	default:
		close(pfd[0]);
		n = read(pfd[1], ftype, sizeof(ftype));
		if(n > 0){
			ftype[n-1] = 0;
			a->type = strdup(ftype);
		}
		close(pfd[1]);
		waitpid();
		break;
	}

	for(c = ctype; ; c++)
		if(strncmp(a->type, c->type, strlen(c->type)) == 0){
			a->ctype = c;
			break;
		}

	return a;
}

char*
mkboundary(void)
{
	char buf[32];
	int i;

	srand((time(0)<<16)|getpid());
	strcpy(buf, "upas-");
	for(i = 5; i < sizeof(buf)-1; i++)
		buf[i] = 'a' + nrand(26);
	buf[i] = 0;
	return strdup(buf);
}

// copy types to two fd's
static void
tee(int in, int out1, int out2)
{
	char buf[8*1024];
	int n;

	for(;;){
		n = read(in, buf, sizeof(buf));
		if(n <= 0)
			break;
		if(write(out1, buf, n) < 0)
			break;
		if(write(out2, buf, n) < 0)
			break;
	}
}

// print the unix from line
int
printunixfrom(int fd)
{
	Tm *tm;
	int tz;

	tm = localtime(time(0));
	tz = (tm->tzoff/3600)*100 + ((tm->tzoff/60)%60);

	return fprint(fd, "From %s %s %s %d %2.2d:%2.2d:%2.2d %s%.4d %d\n",
		user,
		ascwday[tm->wday], ascmon[tm->mon], tm->mday,
		tm->hour, tm->min, tm->sec, tz>0?"+":"", tz, 1900+tm->year);
}

// find the recipient account name
static void
foldername(char *folder, char *rcvr)
{
	char *p;
	char *e = folder+Elemlen-1;

	p = strrchr(rcvr, '!');
	if(p != nil)
		rcvr = p+1;

	while(folder < e && *rcvr && *rcvr != '@')
		*folder++ = *rcvr++;
	*folder = 0;
}

// start up sendmail and return an fd to talk to it with
int
sendmail(Addr *to, int *pid, char *rcvr)
{
	char **av, **v;
	int ac, fd;
	int pfd[2];
	char folder[Elemlen];
	String *file;
	Addr *a;

	fd = -1;
	if(rcvr != nil){
		foldername(folder, rcvr);
		file = s_new();
		mboxpath(folder, user, file, 0);
		fd = open(s_to_c(file), OWRITE);
	}

	ac = 0;
	for(a = to; a != nil; a = a->next)
		ac++;
	v = av = malloc(sizeof(char*)*(ac+8));
	if(av == nil)
		fatal("%r");
	ac = 0;
	v[ac++] = "sendmail";
	if(xflag)
		v[ac++] = "-x";
	if(rflag)
		v[ac++] = "-r";
	if(lbflag)
		v[ac++] = "-#";
	for(a = to; a != nil; a = a->next)
		v[ac++] = a->v;
	v[ac] = 0;

	if(pipe(pfd) < 0)
		fatal("%r");
	switch(*pid = fork()){
	case -1:
		fatal("%r");
		break;
	case 0:
		if(holding)
			close(holding);
		close(pfd[1]);
		dup(pfd[0], 0);
		close(pfd[0]);

		if(rcvr != nil){
			if(pipe(pfd) < 0)
				fatal("%r");
			switch(fork()){
			case -1:
				fatal("%r");
				break;
			case 0:
				close(pfd[0]);
				seek(fd, 0, 2);
				printunixfrom(fd);
				tee(0, pfd[1], fd);
				write(fd, "\n", 1);
				exits(0);
			default:
				close(fd);
				close(pfd[1]);
				dup(pfd[0], 0);
				break;
			}
		}

		exec("/bin/myupassend", av);
		exec("/bin/upas/send", av);
		fatal("execing: %r");
		break;
	default:
		if(rcvr != nil)
			close(fd);
		close(pfd[0]);
		break;
	}
	return pfd[1];
}


// wait for sendmail to exit
char*
waitforsendmail(int pid)
{
	Waitmsg *w;

	while((w = wait()) != nil){
		if(w->pid == pid){
			if(w->msg[0] != 0)
				exits(w->msg);
			free(w);
			break;
		}
		free(w);
	}
	return nil;
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
	for(;;){
		if(tolower(*a) != tolower(*b++))
			return -1;
		if(*a++ == 0)
			break;
	}
	return 0;
}

static uchar t64d[256];
static char t64e[64];

static void
init64(void)
{
	int c, i;

	memset(t64d, 255, 256);
	memset(t64e, '=', 64);
	i = 0;
	for(c = 'A'; c <= 'Z'; c++){
		t64e[i] = c;
		t64d[c] = i++;
	}
	for(c = 'a'; c <= 'z'; c++){
		t64e[i] = c;
		t64d[c] = i++;
	}
	for(c = '0'; c <= '9'; c++){
		t64e[i] = c;
		t64d[c] = i++;
	}
	t64e[i] = '+';
	t64d['+'] = i++;
	t64e[i] = '/';
	t64d['/'] = i;
}

int
enc64(char *out, int lim, uchar *in, int n)
{
	int i;
	ulong b24;
	char *start = out;
	char *e = out + lim;

	if(t64e[0] == 0)
		init64();
	for(i = 0; i < n/3; i++){
		b24 = (*in++)<<16;
		b24 |= (*in++)<<8;
		b24 |= *in++;
		if(out + 5 >= e)
			goto exhausted;
		*out++ = t64e[(b24>>18)];
		*out++ = t64e[(b24>>12)&0x3f];
		*out++ = t64e[(b24>>6)&0x3f];
		*out++ = t64e[(b24)&0x3f];
		if((i%18) == 17)
			*out++ = '\n';
	}

	switch(n%3){
	case 2:
		b24 = (*in++)<<16;
		b24 |= (*in)<<8;
		if(out + 4 >= e)
			goto exhausted;
		*out++ = t64e[(b24>>18)];
		*out++ = t64e[(b24>>12)&0x3f];
		*out++ = t64e[(b24>>6)&0x3f];
		break;
	case 1:
		b24 = (*in)<<16;
		if(out + 4 >= e)
			goto exhausted;
		*out++ = t64e[(b24>>18)];
		*out++ = t64e[(b24>>12)&0x3f];
		*out++ = '=';
		break;
	case 0:
		if((i%18) != 0)
			*out++ = '\n';
		*out = 0;
		return out - start;
	}
exhausted:
	*out++ = '=';
	*out++ = '\n';
	*out = 0;
	return out - start;
}

//
//  read alias file
//
Alias*
readaliases(void)
{
	Alias *a, **l, *first;
	char *p, *e, *nl;
	char *token[1024];
	String *file;
	Dir *d;
	int fd, n, len;
	static int already;

	// open and get length
	file = s_new();
	mboxpath("names", login, file, 0);
	fd = open(s_to_c(file), OREAD);
	if(fd < 0)
		return nil;
	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		return nil;
	}
	len = d->length;
	free(d);

	// read in file in one go
	p = malloc(len+1);
	if(p == nil){
		close(fd);
		return nil;
	}
	n = read(fd, p, len);
	close(fd);
	if(n <= 0){
		free(p);
		return nil;
	}

	// parse alias file
	first = nil;
	l = &first;
	p[n] = '\0';
	for(e = p + n; p < e; p = nl){
		for(;;){
			nl = strchr(p, '\n');
			if(nl == nil){
				nl = e;
				break;
			}
			if(nl == p || *(nl-1) != '\\')
				break;
			*(nl-1) = ' ';
			*nl = ' ';
		}
		*nl++ = 0;
		n = tokenize(p, token, nelem(token));
		if(n < 2)
			continue;
		if(*token[0] == '#')
			continue;
		a = malloc(sizeof(*a) + n*sizeof(char*));
		if(a == nil)
			return nil;
		a->v = (char**)((int)a + sizeof(*a));
		memmove(a->v, token, n*sizeof(char*));
		a->n = n;
		a->next = nil;
		*l = a;
		l = &a->next;
	}

	return first;
}

Addr*
newaddr(char *name)
{
	Addr *a;

	a = malloc(sizeof(*a));
	if(a == nil)
		sysfatal("%r");
	a->next = nil;
	a->v = strdup(name);
	if(a->v == nil)
		sysfatal("%r");
	return a;
}

//
//  expand personal aliases since the names are meaningless in
//  other contexts
//
Addr*
_expand(Addr *old, int *changedp)
{
	Alias *al;
	Addr *first, *next, **l;
	int j;

	*changedp = 0;
	first = nil;
	l = &first;
	for(;old != nil; old = next){
		next = old->next;
		for(al = aliases; al != nil; al = al->next){
			if(strcmp(al->v[0], old->v) == 0){
				for(j = 1; j < al->n; j++){
					*l = newaddr(al->v[j]);
					if(*l == nil)
						sysfatal("%r");
					l = &(*l)->next;
					*changedp = 1;
				}
				break;
			}
		}
		if(al != nil){
			freeaddr(old);
			continue;
		}
		*l = old;
		old->next = nil;
		l = &(*l)->next;
	}
	return first;
}

Addr*
rexpand(Addr *old)
{
	int i, changed;

	changed = 0;
	for(i=0; i<32; i++){
		old = _expand(old, &changed);
		if(changed == 0)
			break;
	}
	return old;
}

Addr*
unique(Addr *first)
{
	Addr *a, **l, *x;

	for(a = first; a != nil; a = a->next){
		for(l = &a->next; *l != nil;){
			if(strcmp(a->v, (*l)->v) == 0){
				x = *l;
				*l = x->next;
				freeaddr(x);
			} else
				l = &(*l)->next;
		}
	}
	return first;
}

Addr*
expand(int ac, char **av)
{
	Addr *first, **l;
	int i;

	// make a list of the starting addresses
	l = &first;
	for(i = 0; i < ac; i++){
		*l = newaddr(av[i]);
		if(*l == nil)
			sysfatal("%r");
		l = &(*l)->next;
	}

	// recurse till we don't change any more
	return unique(rexpand(first));
}

Addr*
concataddr(Addr *a, Addr *b)
{
	Addr *oa;

	if(a == nil)
		return b;

	oa = a;
	for(; a->next; a=a->next)
		;
	a->next = b;
	return oa;
}

void
freeaddr(Addr *ap)
{
	free(ap->v);
	free(ap);
}

void
freeaddrs(Addr *ap)
{
	Addr *next;

	for(; ap; ap=next) {
		next = ap->next;
		freeaddr(ap);
	}
}

String*
s_copyn(char *s, int n)
{
	return s_nappend(s_reset(nil), s, n);
}

// fetch the next token from an RFC822 address string
// we assume the header is RFC822-conformant in that
// we recognize escaping anywhere even though it is only
// supposed to be in quoted-strings, domain-literals, and comments.
//
// i'd use yylex or yyparse here, but we need to preserve 
// things like comments, which i think it tosses away.
//
// we're not strictly RFC822 compliant.  we misparse such nonsense as
//
//	To: gre @ (Grace) plan9 . (Emlin) bell-labs.com
//
// make sure there's no whitespace in your addresses and 
// you'll be fine.
//
enum {
	Twhite,
	Tcomment,
	Twords,
	Tcomma,
	Tleftangle,
	Trightangle,
	Terror,
	Tend,
};
//char *ty82[] = {"white", "comment", "words", "comma", "<", ">", "err", "end"};
#define ISWHITE(p) ((p)==' ' || (p)=='\t' || (p)=='\n' || (p)=='\r')
int
get822token(String **tok, char *p, char **pp)
{
	char *op;
	int type;
	int quoting;

	op = p;
	switch(*p){
	case '\0':
		*tok = nil;
		*pp = nil;
		return Tend;

	case ' ':	// get whitespace
	case '\t':
	case '\n':
	case '\r':
		type = Twhite;
		while(ISWHITE(*p))
			p++;
		break;

	case '(':	// get comment
		type = Tcomment;
		for(p++; *p && *p != ')'; p++)
			if(*p == '\\') {
				if(*(p+1) == '\0') {
					*tok = nil;
					return Terror;
				}
				p++;
			}

		if(*p != ')') {
			*tok = nil;
			return Terror;
		}
		p++;
		break;
	case ',':
		type = Tcomma;
		p++;
		break;
	case '<':
		type = Tleftangle;
		p++;
		break;
	case '>':
		type = Trightangle;
		p++;
		break;
	default:	// bunch of letters, perhaps quoted strings tossed in
		type = Twords;
		quoting = 0;
		for(; *p && (quoting || (!ISWHITE(*p) && *p != '>' && *p != '<' && *p != ',')); p++) {
			if(*p == '"') 
				quoting = !quoting;
			if(*p == '\\') {
				if(*(p+1) == '\0') {
					*tok = nil;
					return Terror;
				}
				p++;
			}
		}
		break;
	}

	if(pp)
		*pp = p;
	*tok = s_copyn(op, p-op);
	return type;
}	

// expand local aliases in an RFC822 mail line
// add list of expanded addresses to to.
Addr*
expandline(String **s, Addr *to)
{
	Addr *na, *nto, *ap;
	char *p;
	int tok, inangle, hadangle, nword;
	String *os, *ns, *stok, *lastword, *sinceword;

	os = s_copy(s_to_c(*s));
	p = strchr(s_to_c(*s), ':');
	assert(p != nil);
	p++;

	ns = s_copyn(s_to_c(*s), p-s_to_c(*s));
	stok = nil;
	nto = nil;
	//
	// the only valid mailbox namings are word
	// and word* < addr >
	// without comments this would be simple.
	// we keep the following:
	//	lastword - current guess at the address
	//	sinceword - whitespace and comment seen since lastword
	//
	lastword = s_new();
	sinceword = s_new();
	inangle = 0;
	nword = 0;
	hadangle = 0;
	for(;;) {
		stok = nil;
		switch(tok = get822token(&stok, p, &p)){
		default:
			abort();
		case Tcomma:
		case Tend:
			if(inangle)
				goto Error;
			if(nword != 1)
				goto Error;
			na = rexpand(newaddr(s_to_c(lastword)));
			s_append(ns, na->v);
			s_append(ns, s_to_c(sinceword));
			for(ap=na->next; ap; ap=ap->next) {
				s_append(ns, ", ");
				s_append(ns, ap->v);
			}
			nto = concataddr(na, nto);
			if(tok == Tcomma){
				s_append(ns, ",");
				s_free(stok);
			}
			if(tok == Tend)
				goto Break2;
			inangle = 0;
			nword = 0;
			hadangle = 0;
			s_reset(sinceword);
			s_reset(lastword);
			break;
		case Twhite:
		case Tcomment:
			s_append(sinceword, s_to_c(stok));
			s_free(stok);
			break;
		case Trightangle:
			if(!inangle)
				goto Error;
			inangle = 0;
			hadangle = 1;
			s_append(sinceword, s_to_c(stok));
			s_free(stok);
			break;
		case Twords:
		case Tleftangle:
			if(hadangle)
				goto Error;
			if(tok != Tleftangle && inangle && s_len(lastword))
				goto Error;
			if(tok == Tleftangle) {
				inangle = 1;
				nword = 1;
			}
			s_append(ns, s_to_c(lastword));
			s_append(ns, s_to_c(sinceword));
			s_reset(sinceword);
			if(tok == Tleftangle) {
				s_append(ns, "<");
				s_reset(lastword);
			} else {
				s_free(lastword);
				lastword = stok;
			}
			if(!inangle)
				nword++;
			break;
		case Terror:	// give up, use old string, addrs
		Error:
			ns = os;
			os = nil;
			freeaddrs(nto);
			nto = nil;
			werrstr("rfc822 syntax error");
			rfc822syntaxerror = 1;
			goto Break2;			
		}
	}
Break2:
	s_free(*s);
	s_free(os);
	*s = ns;
	nto = concataddr(nto, to);
	return nto;
}

void
Bdrain(Biobuf *b)
{
	char buf[8192];

	while(Bread(b, buf, sizeof buf) > 0)
		;
}
