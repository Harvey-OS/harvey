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
	Hsender,
	Hreplyto,
	Hdate,
	Hsubject,
	Hmime,
	Hpriority,
	Hmsgid,
	Hcontent,
	Hx,
};

char *hdrs[] = {
[Hfrom]		"from:",
[Hto]		"to:",
[Hcc]		"cc:",
[Hreplyto]	"reply-to:",
[Hsender]	"sender:",
[Hdate]		"date:",
[Hsubject]	"subject:",
[Hpriority]	"priority:",
[Hmsgid]	"message-id:",
[Hmime]		"mime-",
[Hcontent]	"content-",
[Hx]		"x-",
		0,
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
	{ "message/rfc822",		"rtx",	1,	},
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
int	headers(Biobuf*, Biobuf*, int*);
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

int rflag, lbflag, xflag, holding, nflag, Fflag;
char *user;
Alias *aliases;

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
	fprint(2, "usage: %s [-F] [-s subject] [-t type] [-aA attachment] recipient-list\n",
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
	doprint(buf, buf+sizeof(buf), fmt, arg);
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
	String *file;
	int noinput, headersrv;

	noinput = 0;
	subject = nil;
	first = nil;
	l = &first;
	type = nil;
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
	}ARGEND;

	user = getenv("upasname");
	if(user == nil || *user == 0)
		user = getlog();
	if(user == nil || *user == 0)
		sysfatal("can't read user name");

	if(Binit(&in, 0, OREAD) < 0)
		sysfatal("can't Binit 0: %r");

	if(argc < 1)
		usage();

	aliases = readaliases();
	to = expand(argc, argv);

	fd = sendmail(to, &pid, Fflag ? argv[0] : nil);
	if(fd < 0)
		sysfatal("execing sendmail: %r\n:");
	if(xflag || lbflag){
		close(fd);
		exits(waitforsendmail(pid));
	}
	
	if(Binit(&out, fd, OWRITE) < 0)
		fatal("can't Binit 1: %r");

	flags = 0;
	headersrv = Nomessage;
	if(!nflag){
		// pass through headers, keeping track of which we've seen
		holding = holdon();
		headersrv = headers(&in, &out, &flags);
		switch(headersrv){
		case Error:		// error
			fatal("reading");
			break;
		case Nomessage:		// no message, just exit mimicking old behavior
			noinput = 1;
			if(first == nil){
				postnote(PNPROC, pid, "die");
				exits(0);
			}
			break;
		}

		// read user's standard headers
		file = s_new();
		mboxpath("headers", user, file, 0);
		b = Bopen(s_to_c(file), OREAD);
		if(b != nil){
			switch(headers(b, &out, &flags)){
			case Error:	// error
				fatal("reading");
			}
			Bterm(b);
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
		if(!noinput && headersrv == Ok)
			body(&in, &out, 1);
	} else
		Bprint(&out, "\n");

	Bflush(&out);
	for(a = first; a != nil; a = a->next){
		Bprint(&out, "\n--%s\n", boundary);
		attachment(a, &out);
	}

	if(first != nil)
		Bprint(&out, "--%s--\n", boundary);

	Bterm(&out);
	close(fd);
	holdoff(holding);
	exits(waitforsendmail(pid));
}

// pass headers to sendmail and keep track of which ones are really there
int
headers(Biobuf *in, Biobuf *out, int *fp)
{
	int i, inheader, seen;
	char *p;

	inheader = 0;
	seen = 0;
	while((p = Brdline(in, '\n')) != nil){
		seen = 1;
		p[Blinelen(in)-1] = 0;
		if((*p == ' ' || *p == '\t') && inheader){
			if(Bprint(out, "%s\n", p) < 0)
				return Error;
			continue;
		}
		if(strchr(p, ':') == nil){
			p[Blinelen(in)-1] = '\n';
			Bseek(in, -Blinelen(in), 1);
			break;
		}
		inheader = 1;
		for(i = 0; hdrs[i]; i++){
			if(cistrncmp(hdrs[i], p, strlen(hdrs[i])) == 0){
				*fp |= 1<<i;
				break;
			}
		}
		if(hdrs[i] == 0){
			p[Blinelen(in)-1] = '\n';
			Bseek(in, -Blinelen(in), 1);
			break;
		}
		if(Bprint(out, "%s\n", p) < 0)
			return Error;
		p[Blinelen(in)-1] = '\n';
	}
	if(p == nil && seen)
		return Nobody;
	if(seen == 0)
		return Nomessage;
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
		for(;;){
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
				if(*p++ & 0x80){
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

	// pass the rest
	for(;;){
		n = Bread(in, buf, len);
		if(n < 0)
			fatal("input error2");
		if(n == 0)
			break;
		if(Bwrite(out, buf, n) < 0)
			fatal("output error");
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
			postnote(PNPROC, pid, "interrupt");
			sysfatal("opening %s: %r", a->path);
		}
		copy(f, out);
		Bterm(f);
	}
	

	// if it's not already mime encoded ...
	if(strcmp(a->type, "text/plain") != 0)
		Bprint(out, "Content-Type: %s\n", a->type);

	if(a->inline)
		Bprint(out, "Content-Disposition: inline\n");
	else {
		p = strrchr(a->path, '/');
		if(p == nil)
			p = a->path;
		else
			p++;
		Bprint(out, "Content-Disposition: attachment; filename=%s\n", p);
	}

	f = Bopen(a->path, OREAD);
	if(f == nil){
		postnote(PNPROC, pid, "interrupt");
sleep(1000);
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

	return Bprint(b, "Date: %s, %d %s %d %2.2d:%2.2d:%2.2d %.4d\n",
		ascwday[tm->wday], tm->mday, ascmon[tm->mon], 1900+tm->year,
		tm->hour, tm->min, tm->sec, tz);
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
		wait(nil);
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

	return fprint(fd, "From %s %s %s %d %2.2d:%2.2d:%2.2d %.4d %d\n",
		user,
		ascwday[tm->wday], ascmon[tm->mon], tm->mday,
		tm->hour, tm->min, tm->sec, tz, 1900+tm->year);
}

// find the recipient account name
static void
foldername(char *folder, char *rcvr)
{
	char *p;
	char *e = folder+NAMELEN-1;

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
	char folder[NAMELEN];
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

		exec("/bin/upas/send", av);
		fatal("/bin/upas/send: %r");
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
	Waitmsg w;
	int wpid;

	while((wpid = wait(&w)) >= 0){
		if(wpid == pid){
			if(w.msg[0] != 0)
				exits(w.msg);
			break;
		}
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
	Dir d;
	int fd, n;
	static int already;

	// open and get length
	file = s_new();
	mboxpath("names", user, file, 0);
	fd = open(s_to_c(file), OREAD);
	if(fd < 0)
		return nil;
	if(dirfstat(fd, &d) < 0){
		close(fd);
		return nil;
	}

	// read in file in one go
	p = malloc(d.length+1);
	if(p == nil){
		close(fd);
		return nil;
	}
	n = read(fd, p, d.length);
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
	a->v = name;
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
			free(old);
			continue;
		}
		*l = old;
		old->next = nil;
		l = &(*l)->next;
	}
	return first;
}

Addr*
expand(int ac, char **av)
{
	Addr *first, **l;
	int i, changed;

	// make a list of the starting addresses
	l = &first;
	for(i = 0; i < ac; i++){
		*l = newaddr(av[i]);
		if(*l == nil)
			sysfatal("%r");
		l = &(*l)->next;
	}

	// recurse till we don't change any more
	for(i = 0; i < 32; i++){
		first = _expand(first, &changed);
		if(changed == 0)
			break;
	}

	return first;
}
