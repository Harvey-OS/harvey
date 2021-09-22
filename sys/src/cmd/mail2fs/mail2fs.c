/*
 * Use upas/fs to convert a plan 9 mail box into
 * a file tree with a plan b mail box (similar to Mh).
 *
 * Implementation is not as clean as it should be,
 * A large part of the code is taken as-is from nedmail, trying to
 * keep it the same, so that bugs are easy to fix both
 * here and in nedmail (should they show up).
 * We should get rid of Strings, for example.
 *
 */

#include "common.h"
#include <ctype.h>
#include <libsec.h>
#include "util.h"

enum {
	Defperm = 0666, /* none needs access for delivery, else could be 0620 */
};

typedef struct Message Message;
typedef struct Ctype Ctype;
typedef struct Cmd Cmd;

struct Message {
	Message	*next;
	Message	*prev;
	Message	*cmd;
	Message	*child;
	Message	*parent;
	String	*path;
	int	id;
	int	len;
	int	fileno;		/* number of directory */
	String	*info;
	char	*from;
	char	*to;
	char	*cc;
	char	*replyto;
	char	*date;
	char	*subject;
	char	*type;
	char	*disposition;
	char	*filename;
	char	*sdigest;
	char	deleted;
	char	stored;
	int	saveit;
};

struct Ctype {
	char	*type;
	char 	*ext;
	int	display;
	char	*plumbdest;
	Ctype	*next;
};

Ctype ctype[] = {
	{ "text/plain",			"txt",	1,	0	},
	{ "text/html",			"htm",	1,	0	},
	{ "text/html",			"html",	1,	0	},
	{ "text/tab-separated-values",	"tsv",	1,	0	},
	{ "text/richtext",		"rtx",	1,	0	},
	{ "text/rtf",			"rtf",	1,	0	},
	{ "text",			"txt",	1,	0	},
	{ "message/rfc822",		"msg",	0,	0	},
	{ "image/bmp",			"bmp",	0,	"image"	},
	{ "image/jpeg",			"jpg",	0,	"image"	},
	{ "image/gif",			"gif",	0,	"image"	},
	{ "image/png",			"png",	0,	"image"	},
	{ "application/pdf",		"pdf",	0,	"postscript"	},
	{ "application/postscript",	"ps",	0,	"postscript"	},
	{ "application/",		0,	0,	0	},
	{ "image/",			0,	0,	0	},
	{ "multipart/",			"mul",	0,	0	},

};

Rune	altspc = L'·';
Rune	altlparen = L'«';
Rune	altrparen = L'»';
Rune	altquote = L'´';
Rune	altamp = L'­';
Rune	altslash = L'.';

enum
{
	NARG=	32,
};

struct Cmd {
	Message	*msgs;
	Message	*(*f)(Cmd*, Message*);
	int	an;
	char	*av[NARG];
	int	delete;
};

Message	top;		/* top-level of upas mailbox */

int	mboxfd;		/* lock/seq fd for plan b mail box */
int	debug;
int	dry;
int	cflag;		/* create */
char	aflag;		/* archive (value is character) */
int	oflag;		/* old mail */
int	rflag;		/* save complete raw message */
int	mboxisfile;

char*	digests;		/* in-memory copy of digests */
char*	mboxnm;		/* name for mbox in upas */

void
tabs(int n)
{
	static char t[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

	if(n >= sizeof t)
		n = sizeof t - 1;	/* don't run off the end, truncate */
	write(2, t, n);
}

void
printmessage(Message* m, int t)
{
	Message* l;

	tabs(t);
	fprint(2, "message %d file %d %d bytes\n", m->id, m->fileno, m->len);
	if(m->info != nil){
		tabs(t);
		fprint(2, "info %s\n", s_to_c(m->info));
	}
	if(m->from != nil){
		tabs(t);
		fprint(2, "from %s\n", m->from);
	}
	if(m->subject != nil){
		tabs(t);
		fprint(2, "subject %s\n", m->subject);
	}
	if(m->filename != nil){
		tabs(t);
		fprint(2, "file %s\n", m->filename);
	}
	for(l = m->child; l != nil ; l = l->next)
		printmessage(l, t+1);
	fprint(2, "\n");
}

String*
extendpath(String *dir, char *name)
{
	String *path;

	if(strcmp(s_to_c(dir), ".") == 0)
		path = s_new();
	else {
		path = s_copy(s_to_c(dir));
		s_append(path, "/");
	}
	s_append(path, name);
	return path;
}

String*
file2string(String *dir, char *file)
{
	String *s;
	int fd, n, m;

	s = extendpath(dir, file);
	fd = open(s_to_c(s), OREAD);
	s_grow(s, 512);			/* avoid multiple reads on info files */
	s_reset(s);
	if(fd < 0)
		return s;

	for(;;){
		n = s->end - s->ptr;
		if(n == 0){
			s_grow(s, 128);
			continue;
		}
		m = read(fd, s->ptr, n);
		if(m <= 0)
			break;
		s->ptr += m;
		if(m < n)
			break;
	}
	s_terminate(s);
	close(fd);

	return s;
}

int
lineize(char *s, char **f, int n)
{
	int i;

	for(i = 0; *s && i < n; i++){
		f[i] = s;
		s = strchr(s, '\n');
		if(s == nil)
			break;
		*s++ = 0;
	}
	return i;
}

int
filelen(String *dir, char *file)
{
	String *path;
	Dir *d;
	int rv;

	path = extendpath(dir, file);
	d = dirstat(s_to_c(path));
	if(d == nil){
		s_free(path);
		return -1;
	}
	s_free(path);
	rv = d->length;
	free(d);
	return rv;
}


int dir2message(Message *parent, int reverse);

Message*
file2message(Message *parent, char *name)
{
	Message *m;
	String *path;
	char *f[10];

	m = mallocz(sizeof(Message), 1);
	if(m == nil)
		return nil;
	m->path = path = extendpath(parent->path, name);
	m->fileno = atoi(name);
	m->info = file2string(path, "info");
	lineize(s_to_c(m->info), f, nelem(f));
	m->from = f[0];
	m->to = f[1];
	m->cc = f[2];
	m->replyto = f[3];
	m->date = f[4];
	m->subject = f[5];
	m->type = f[6];
	m->disposition = f[7];
	m->filename = f[8];
	m->len = filelen(path, "raw");
	if(strstr(m->type, "multipart") != nil ||
	 strcmp(m->type, "message/rfc822") == 0)
		dir2message(m, 0);
	m->parent = parent;

	return m;
}

int
dir2message(Message *parent, int reverse)
{
	int i, n, fd, highest, newmsgs;
	Dir *d;
	Message *first, *last, *m;

	fd = open(s_to_c(parent->path), OREAD);
	if(fd < 0)
		return -1;

	/* count current entries */
	first = parent->child;
	highest = newmsgs = 0;
	for(last = parent->child; last != nil && last->next != nil;
	 last = last->next)
		if(last->fileno > highest)
			highest = last->fileno;
	if(last != nil)
		if(last->fileno > highest)
			highest = last->fileno;

	n = dirreadall(fd, &d);
	for(i = 0; i < n; i++){
		if((d[i].qid.type & QTDIR) == 0)
			continue;
		if(atoi(d[i].name) <= highest)
			continue;
		m = file2message(parent, d[i].name);
		if(m == nil)
			break;
		newmsgs++;
		if(reverse){
			m->next = first;
			if(first != nil)
				first->prev = m;
			first = m;
		} else {
			if(first == nil)
				first = m;
			else
				last->next = m;
			m->prev = last;
			last = m;
		}
	}
	free(d);
	close(fd);
	parent->child = first;

	/* renumber */
	for(m = first; m != nil; m = m->next)
		m->id = m->fileno;

	return newmsgs;
}

void
compress(char *p)
{
	char *np;
	int last;

	last = ' ';
	for(np = p; *p; p++)
		if(*p != '' && (*p != ' ' || last != ' ')){
			last = *p;
			*np++ = last;
		}
	*np = 0;
}

int
printheader(Biobuf* bout, Message* m)
{
	if(m->from == nil || *m->from == 0)
		return 0;
	Bprint(bout, "From: %s\n", m->from);
	if(m->to != nil && *m->to)
		Bprint(bout, "To: %s\n", m->to);
	if(m->cc != nil && *m->cc)
		Bprint(bout, "Cc: %s\n", m->cc);
	if(m->replyto != nil && *m->replyto && m->from != nil && strcmp(m->replyto, m->from))
		Bprint(bout, "Reply-To: %s\n", m->replyto);
	if(m->date != nil && *m->date)
		Bprint(bout, "Date: %s\n", m->date);
	if(m->subject != nil && *m->subject)
		Bprint(bout, "Subject: %s\n", m->subject);
	return 1;
}

int
printpart(Biobuf* bout, String *s, char *part)
{
	char buf[4096];
	int n, fd, tot;
	String *path;

	path = extendpath(s, part);
	fd = open(s_to_c(path), OREAD);
	s_free(path);
	if(fd < 0){
		fprint(2, "!message disappeared\n");
		return 0;
	}
	tot = 0;
	while((n = read(fd, buf, sizeof(buf)-1)) > 0){
		buf[n] = 0;
		compress(buf);
		n = strlen(buf);
		if(Bwrite(bout, buf, n) <= 0)
			break;
		tot += n;
	}
	close(fd);
	return tot;
}

/*
 * Mail box format:
 * dir/			(mail box)
 *	seq		(file with last msg nb, +l)
 *	digest		(file with mail digests)
 *	yyyymm/		(per month folder)
 *		raw	(raw message; just headers)
 *		text	(text as in reader)
 *		1.x	(first attach)
 *		2.x	(second attach)
 *		3/	(third attach, a mail)
 */

int
mkmdir(char* mdir)
{
	int	fd, seqfd, nattempts;
	char	buf[200];
	char*	s;
	Dir*	d;

	digests = strdup("");
	d = dirstat(mdir);
	s = smprint("%s/seq", mdir);
	if(d == nil){
		if(!cflag)
			sysfatal("%s: not a mail dir", mdir);
		if(debug)
			fprint(2, "create mdir %s\n", mdir);

		fd = create(mdir, OREAD, DMDIR|Defperm|0111);
		if(fd < 0)
			sysfatal("%s: %r", mdir);
		close(fd);

		fd = seqfd = create(s, ORDWR, Defperm|DMEXCL);
		if(fd < 0)
			sysfatal("%s: %r", s);
		fprint(fd, "%11.11d", 0);
		seek(fd, 0, 0);
		free(s);

		s = smprint("%s/digest", mdir);
		fd = create(s, OREAD, Defperm|DMAPPEND|DMEXCL);
		if(fd < 0)
			sysfatal("%s: %r", s);
		close(fd);
	} else {
		if((d->qid.type&QTDIR) == 0)
			sysfatal("%s: not a directory", mdir);
		free(d);
		nattempts = 0;
		while ((seqfd = open(s, ORDWR)) < 0){
			rerrstr(buf, sizeof buf);
			if(strstr(buf, "exclusive") == nil &&
			 strstr(buf, "already open") == nil)
				break;
			if(nattempts++ >= 10)
				sysfatal("%s: can't get lock: %r", mdir);
			sleep(500);
		}
		if (seqfd < 0)
			seqfd = create(s, ORDWR, Defperm|DMEXCL);
		if (seqfd < 0)
			sysfatal("can't create %s: %r", s);
	}
	free(s);
	return seqfd;
}

void
closemdir(int fd)
{
	close(fd);
}

int
newseq(int startover)
{
	int	i, nr;
	char	buf[128];

	if(mboxfd < 0)
		return -1;
	seek(mboxfd, 0, 0);
	nr = read(mboxfd, buf, sizeof(buf)-1);
	if(nr < 0)
		return -1;
	if(nr == 0)
		i = 0;
	else {
		buf[nr] = 0;
		i = atoi(buf);
	}
	i++;

	if(startover && !oflag)		/* start sequencing each month */
		i = 1;			/* unless we are adding old msgs */
	seek(mboxfd, 0, 0);
	fprint(mboxfd, "%11d ", i);
	if(debug)
		fprint(2, "newmsg %d\n", i);
	return i;
}

char*
mksubmsg(int id, char* mdir)
{
	char*	fname;
	int	fd;

	if(aflag)
		fname = smprint("%s/%c.%d", mdir, aflag, id);
	else
		fname = smprint("%s/%d", mdir, id);
	if(debug)
		fprint(2, "create %s\n", fname);
	fd = create(fname, OREAD, Defperm|0111|DMDIR);
	if(fd < 0){
		fprint(2, "%s: %r\n", fname);
		free(fname);
		fname = nil;
	} else
		close(fd);
	return fname;
}

static char*
datedir(void)
{
	Tm *tm;
	static char datebuf[30];

	if(datebuf[0] == 0){
		tm = localtime(time(0));
		seprint(datebuf, datebuf+30, "%.4d%.2d",
			tm->year+1900, tm->mon+1);
	}
	return datebuf;
}

static struct{
	char *m;
	char *n;
} mnames[] = {
	{ "jan", "01"}, {"feb", "02"}, {"mar", "03"}, {"apr", "04"}, {"may", "05"}, {"jun", "06"},
	{ "jul", "07"}, {"aug", "08"}, {"sep", "09"}, {"oct", "10"}, {"nov", "11"}, {"dec", "12"}
};

static char*
mname(char *m)
{
	int i;

	for(i = 0; i < nelem(mnames); i++)
		if(cistrcmp(m, mnames[i].m))
			return mnames[i].n;
	return "00";
}

int
isnumeric(char *s)
{
	while(*s){
		if(!isdigit(*s))
			return 0;
		s++;
	}
	return 1;
}

static char*
mdatedir(Message *m)
{
	int n;
	char *s, *fld[10];
	static char datebuf[30];
	char *month, *year;

	n = 0;
	s= nil;
	if(m->date != nil){
		s = strdup(m->date);
		n = tokenize(s, fld, 10);
	}
	if(n >= 5){
		/* some dates have 19 Apr, some Apr 19 */
		if(strlen(fld[1])<4 && isnumeric(fld[1]))
			month = fld[2];
		else
			month = fld[1];
		month = mname(month);
		if(n > 5)
			year = fld[5];
		else
			year = fld[4];
		seprint(datebuf, datebuf+30, "%.4s%.2s", year, month);
	}else
		fprint(2, "%d fields [%s]\n", n, m->date);

	free(s);
	if(datebuf[0] != 0){
		if(debug)
			fprint(2, "olddir: %s\n", datebuf);
		return datebuf;	/* may be an old one if we failed. that's ok. */
	}else
		return datedir();
}

/*
 * Create directory for the message, including the per-month
 * directory.
 * Date is today or that from the mail if oflag (old mail convert)
 */
char*
mkmsg(Message *m, char* mdir)
{
	int	fd;
	char *d, *fname;
	int newmonth;

	d = smprint("%s/%s", mdir, oflag ? mdatedir(m) : datedir());
	newmonth = 0;
	if(access(d, AEXIST) < 0){
		fd = create(d, OREAD, Defperm|0111|DMDIR);
		if(fd < 0){
			fprint(2, "%s: %s: %r\n", argv0, d);
			free(d);
			return nil;
		}
		close(fd);
		newmonth=1;
	}
	fname = mksubmsg(newseq(newmonth), d);
	free(d);
	return fname;
}


Ctype*
findctype(Message *m)
{
	int n, kid, pid, pfd[2];
	char *p;
	char ftype[128];
	Ctype *a, *cp;
	static Ctype nulltype	= { "", 0, 0, 0 };
	static Ctype bintype 	= { "application/octet-stream", "bin", 0, 0 };

	for(cp = ctype; cp; cp = cp->next)
		if(strncmp(cp->type, m->type, strlen(cp->type)) == 0)
			return cp;

	/*
	 * use file(1) for any unknown mime types
	 */
	if(pipe(pfd) < 0) {
		fprint(2, "%s: out of pipes: %r\n", argv0);
		return &bintype;
	}

	*ftype = 0;
	switch(kid = fork()){
	case -1:
		break;
	case 0:
		close(pfd[1]);
		close(0);
		dup(pfd[0], 0);
		close(1);
		dup(pfd[0], 1);
		execl("/bin/file", "file", "-m",
			s_to_c(extendpath(m->path, "body")), nil);
		sysfatal("no /bin/file: %r");
	default:
		close(pfd[0]);
		n = read(pfd[1], ftype, sizeof(ftype));
		if(n > 0)
			ftype[n] = 0;
		close(pfd[1]);
		while ((pid = waitpid()) != -1 && pid != kid)
			;
		break;
	}

	if(*ftype=='\0' || (p = strchr(ftype, '/')) == nil)
		return &bintype;
	*p++ = 0;

	a = mallocz(sizeof(Ctype), 1);
	a->type = strdup(ftype);
	a->ext = strdup(p);
	a->display = 0;
	a->plumbdest = strdup(ftype);
	for(cp = ctype; cp->next; cp = cp->next)
		continue;
	cp->next = a;
	a->next = nil;
	return a;
}

void
pipecmd(Cmd *c, Biobuf* bout, Message *m, char* part)
{
	int i, fd, nr, kid, pid;
	int pfd[2];
	char *p, *e;
	char cmd[128], buf[512];
	char *av[4];
	String *path;

	path = extendpath(m->path, part);
	fd = open(s_to_c(path), OREAD);
	s_free(path);
	if(fd < 0){	/* compatibility with older upas/fs */
		path = extendpath(m->path, "raw");
		fd = open(s_to_c(path), OREAD);
		s_free(path);
	}
	if(fd < 0){
		fprint(2, "!message disappeared\n");
		return;
	}

	p = cmd;
	e = cmd + sizeof cmd;
	cmd[0] = 0;
	for(i = 1; i < c->an; i++)
		p = seprint(p, e, "%s ", c->av[i]);
	av[0] = "rc";
	av[1] = "-c";
	av[2] = cmd;
	av[3] = 0;
	if(pipe(pfd)<0){
		fprint(2, "%s: pipe: %r\n", argv0);
		return;
	}
	kid = fork();
	switch(kid){
	case -1:
		fprint(2, "%s: fork: %r\n", argv0);
		break;
	case 0:
		dup(fd, 0);
		close(fd);
		close(pfd[0]);
		dup(pfd[1], 1);
		close(pfd[1]);
		exec("/bin/rc", av);
		sysfatal("no /bin/rc: %r");
	default:
		close(fd);
		close(pfd[1]);
		while ((nr = read(pfd[0], buf, sizeof buf)) > 0)
			Bwrite(bout, buf, nr);
		close(pfd[0]);
		while ((pid = waitpid()) != -1 && pid != kid)
			;
		break;
	}
}

int
printhtml(Biobuf* bout, Message *m)
{
	Cmd c;

	c.an = 3;
	c.av[1] = "/bin/htmlfmt";
	c.av[2] = "-a -l 80 -cutf-8";
	pipecmd(&c, bout, m, "body");
	return 0;
}

void
fcopy(char* d, char* s, int append)
{
	int dfd, sfd, nr;
	char buf[1024];

	if(debug)
		fprint(2, "create %s\n", d);
	if(append){
		dfd = open(d, OWRITE);
		if(dfd >= 0)
			seek(dfd, 0, 2);
	}else
		dfd = create(d, OWRITE, 0660 /* TODO 0640 */);
	sfd = open(s, OREAD);
	if(dfd < 0 || sfd < 0){
		fprint(2, "%s: copying %s to %s: %r\n", argv0, s, d);
		goto fail;
	}
	while ((nr = read(sfd, buf, sizeof buf)) > 0)
		if(write(dfd, buf, nr) != nr){
			fprint(2, "%s: copying %s to %s: %r\n", argv0, s, d);
			break;
		}
fail:
	close(dfd);
	close(sfd);
}

char*
importname(char* name)
{
	int nr;
	Rune r;
	char *up, *p9name, *uname;

	uname = name;
	if(name == 0 ||
	 (strchr(name, ' ') == 0 && strchr(name, '(') == 0 &&
	 strchr(name, ')') == 0 && strchr(name, '&') == 0 &&
	 strchr(name, '\'') == 0 && strchr(name, '/') == 0))
		return name;
	p9name = malloc(strlen(name) * 3 + 1);	/* worst case: all blanks + 0 */
	up = p9name;
	while(*name != 0){
		nr = chartorune(&r, name);
		if(r == ' ' || r == '\n' || r == '\r' || r == '\t')
			r = altspc;
		else if(r == '(')
			r = altlparen;
		else if(r == ')')
			r = altrparen;
		else if(r == '&')
			r = altamp;
		else if(r == '\'')
			r = altquote;
		else if(r == '/')
			r = altslash;
		else if(!isalpharune(r)){
			free(p9name);
			free(uname);
			return nil;
		}
		up += runetochar(up, &r);
		name += nr;
	}
	*up = 0;
	free(uname);
	return p9name;
}

char*
mfname(Message* m, Ctype* cp)
{
	char *s, *r;

	if(m->filename == nil || *m->filename == 0)
		s = smprint("%d", m->id);
	else
		s = strdup(m->filename);
	s = importname(s);
	if(s == nil)
		s = smprint("%d", m->id);
	if(cp->ext != nil)
		if(strstr(s, cp->ext) != s + strlen(s) - strlen(cp->ext)){
			r = smprint("%s.%s",s, cp->ext);
			free(s);
			return r;
		}

	return s;
}

static void
addnl(char *f)
{
	int fd;

	fd = open(f, OWRITE);
	if(fd < 0)
		return;
	seek(fd, 0, 2);
	write(fd, "\n\n", 2);
	close(fd);
}

static char*
xtabs(int n)
{
	static char t[20];

	memset(t, '\t', sizeof(t));
	t[n] = 0;
	return t;
}
/*
 * Main conversion routine.
 * Convert message m into a directory.
 * It may recur for compound msgs, bout
 * is the message text, nil in the first call.
 */
int
msg2fs(Biobuf* bout, Message* m, char* mdir)
{
	char *dfname, *f, *fname, *msgdir;
	Ctype *cp;
	Message *nm;
	String *s;
	static int nest = 0;

	nest++;
	msgdir = nil;
	if(m->parent == &top || strcmp(m->type, "message/rfc822") == 0){
		if(m->parent == &top)
			msgdir = mkmsg(m, mdir);
		else
			msgdir = mksubmsg(m->id, mdir);
		if(msgdir == nil){
			nest--;
			return -1;
		}
		mdir = msgdir;
		fname = smprint("%s/text", mdir);
		if(debug){
			if(m->parent == &top)
				fprint(2, "%stoplevel\n", xtabs(nest));
			fprint(2, "%screate %s\n", xtabs(nest), fname);
		}
		if(oflag)
			print("%s\n", fname);
		close(create(fname, OWRITE, 0666 /* TODO 0640 */));
		bout = Bopen(fname, OWRITE);
		if(bout == nil){
			fprint(2, "%s: %s: %r\n", argv0, fname);
			free(fname);
			free(msgdir);
			nest--;
			return -1;
		}
		free(fname);

		dfname = smprint("%s/raw", mdir);
		fname = smprint("%s/unixheader", s_to_c(m->path));
		fcopy(dfname, fname, 0);
		free(fname);
		/* save entire original in raw or save just headers? */
		if(rflag){
			fname = smprint("%s/raw", s_to_c(m->path));
			fcopy(dfname, fname, 1);
			free(fname);
		}else{
			fname = smprint("%s/rawheader", s_to_c(m->path));
			fcopy(dfname, fname, 1);
			addnl(dfname);
			free(fname);
		}
		free(dfname);
	}
	if(bout != nil && printheader(bout, m) > 0)
		Bprint(bout, "\n");
	cp = findctype(m);
	if(cp == nil)
		fprint(2, "nil ctype\n");
	else if(cp->display){
		if(debug)
			fprint(2, "%sbody\n", xtabs(nest));
		if(strcmp(m->type, "text/html") == 0)
			printhtml(bout, m);
		else
			printpart(bout, m->path, "body");
		/* experiment: save also inline attachments */
		if(m->saveit)
			goto Save;
	} else if(strcmp(m->type, "multipart/alternative") == 0){
		if(debug){
			fprint(2, "%smultipart/alternative\n", xtabs(nest));
			for(nm = m->child; nm != nil; nm = nm->next)
				fprint(2, "%s->%s\n", xtabs(nest+1), nm->type);
		}
		/* all parts should be equivalent, according to mime. however,
		 * Apple mail dares to place attachs into a part that is multipart,
		 * which is not equivalent to the first text part we find. So, we must
		 * prefer a multipart subpart to any other plain text or displayed part.
		 */
		for(nm = m->child; nm != nil; nm = nm->next)
			if(strncmp(nm->type, "multipart/", 10) == 0)
				break;
		if(nm == nil)
			for(nm = m->child; nm != nil; nm = nm->next){
				cp = findctype(nm);
				if(cp->ext != nil && strncmp(cp->ext, "txt", 3) == 0)
					break;
			}
		if(nm == nil)
			for(nm = m->child; nm != nil; nm = nm->next){
				cp = findctype(nm);
				if(cp->display)
					break;
			}
		if(nm != nil)
			msg2fs(bout, nm, mdir);
	} else if(strncmp(m->type, "multipart/", 10) == 0){
		if(debug)
			fprint(2, "%smultipart\n", xtabs(nest));
		nm = m->child;
		if(nm != nil){
			/* always print first part */
			msg2fs(bout, nm, mdir);

			for(nm = nm->next; nm != nil; nm = nm->next){
				fname = smprint("%s", s_to_c(nm->path));
				s = s_copy(fname);
				free(fname);
				cp = findctype(nm);
				if(nm->type != nil &&
				 strcmp(nm->type, "message/rfc822") == 0){
					Bprint(bout, "\n— %d/\n", nm->id);
					msg2fs(bout, nm, mdir);
				} else {
					f = mfname(nm, cp);
					Bprint(bout,"\n!— %s\n\n",f);
					free(f);
					/* experiment: save also inline attachments */
					nm->saveit = 1;
					msg2fs(bout, nm, mdir);
				}
				s_free(s);
			}
		}
	} else if(strcmp(m->type, "message/rfc822") == 0){
		if(debug)
			fprint(2, "%smessage/rfc822\n", xtabs(nest));
		msg2fs(bout, m->child, mdir);
	} else {
	Save:
		if(debug)
			fprint(2, "%sattach\n", xtabs(nest));
		f = mfname(m, cp);
		dfname = smprint("%s/%s", mdir, f);
		free(f);
		if(cp->ext)
			fname = smprint("%s/body.%s", s_to_c(m->path), cp->ext);
		else
			fname = smprint("%s/body", s_to_c(m->path));
		fcopy(dfname, fname, 0);
		free(dfname);
		free(fname);
	}
	free(msgdir);
	if(m->parent == &top || strcmp(m->type, "message/rfc822") == 0)
		Bterm(bout);
	nest--;
	return 0;
}

/*
 * Compute signature for mail.
 * Msgs with same signature are considered dups and not
 * added to the mail box.
 * Returns 0 if message is already there.
 */
static char sdigest[MD5dlen*2+1];

char*
lastsigned(void)
{
	return sdigest;
}

int
sign(Message* m)
{
	int fd, i, n;
	char *edigest, *odigests, *p;
	uchar buf[256], digest[MD5dlen];
	DigestState *s;
	String *path;

	s = md5((uchar*)"mail2fs", 7, nil, nil);

	path = extendpath(m->path, "subject");
	fd = open(s_to_c(path), OREAD);
	s_free(path);
	if(fd < 0)
		goto fail;
	n = read(fd, buf, sizeof(buf));
	if(n < 0)
		goto fail;
	if(n > 0)
		md5(buf, n, nil, s);
	close(fd);
	path = extendpath(m->path, "body");
	fd = open(s_to_c(path), OREAD);
	s_free(path);
	if(fd < 0)
		goto fail;
	while((n = read(fd, buf, 256)) > 0)
		md5(buf, n, nil, s);
	md5((uchar*)"mail2fs", 7, digest, s);
	close(fd);
	p = sdigest;
	edigest = sdigest + sizeof(sdigest);
	for(i = 0; i < MD5dlen; i++)
		p = seprint(p, edigest, "%02x", digest[i]);
	*p = 0;
	if(strstr(digests, sdigest)){
		if(debug)
			fprint(2, "%s: %s: message already in digests\n",
				argv0, s_to_c(m->path));
		return 0;
	}
	odigests = digests;
	digests = smprint("%s%s\n", odigests, sdigest);
	if(digests == nil)
		sysfatal("not enough memory");
	if(debug)
		fprint(2, "%s: %s: digest %s\n",
			argv0, s_to_c(m->path), sdigest);
	free(odigests);
	return 1;
fail:
	close(fd);
	fprint(2, "!message disappeared\n");
	md5((uchar*)"mail2fs", 7, digest, s);
	return 1;
}

void
upasfs(char* mbox)
{
	int kid, pid;

	rfork(RFNAMEG);
	unmount(nil, "/mail/fs");
	kid = fork();
	switch(kid){
	case -1:
		break;
	case 0:
		execl("/bin/upas/fs", "upas/fs", "-p", "-f", mbox, nil);
		sysfatal("no upas/fs: %r");
	default:
		while ((pid = waitpid()) != -1 && pid != kid)
			;
		break;
	}
}


char*
openmbox(char* mbox)
{
	char*	d;
	int	fd;

	mboxnm = strrchr(mbox, '/');
	if (mboxnm != nil)
		mboxnm++;
	else
		mboxnm = mbox;
	d = smprint("/mail/fs/%s", mboxnm);
	upasfs(mbox); /* run upas/fs on mbox file to produce a tree */

	fd = open("/mail/fs/ctl", OWRITE);
	if(fd < 0)
		sysfatal("/mail/fs/ctl: %r");
	if(fprint(fd, "open %s\n", mbox) < 0){
		close(fd);
		sysfatal("%s: open: %r", mbox);
	}
	close(fd);
	return d;
}

void
closembox(void)
{
	int	fd, some;
	char *s, *e;
	char	buf[1024];
	Message *l;

	fd = open("/mail/fs/ctl", OWRITE);
	if(fd < 0)
		sysfatal("/mail/fs/ctl: %r");
	e = buf + sizeof buf;
	s = seprint(buf, e, "delete %s", mboxnm);
	some = 0;
	for(l = top.child; l != nil; l = l->next)
		if(l->deleted){
			if(e - s < 20){
				if(write(fd, buf, s-buf) != s-buf)
					fprint(2, "%s: delete: %r\n", mboxnm);
				s = seprint(buf, e, "delete %s", mboxnm);
				some = 0;
			}
			some++;
			s = seprint(s, e, " %d", l->id);
		}
	if(some && write(fd, buf, s-buf) != s-buf)
		fprint(2, "%s: delete: %r\n", mboxnm);
	close(fd);
}

int
mbox2fs(char* mbox, char* mdir)
{
	int	n, sfd;
	char*	fname;
	Message *l;
	String *ddir, *s;

	if(chdir(mbox) < 0){
		fprint(2, "%s: %s: not a directory: %r\n", argv0, mbox);
		return -1;
	}
	top.path = s_copy(mbox);
	ddir = s_copy(mdir);
	s = file2string(ddir, "digest");
	digests = strdup(s_to_c(s));
	s_free(s);
	s_free(ddir);
	n = dir2message(&top, 0);
	if(n < 0) {
		fprint(2, "%s: dirmessage failed\n", argv0);
		return -1;
	}
	fname = smprint("%s/digest", mdir);
	sfd = open(fname, OWRITE);
	free(fname);
	if(sfd >= 0)
		seek(sfd, 0, 2);
	for(l = top.child; l != nil; l = l->next){
		if(debug)
			fprint(2, "converting %s/%d\n", mbox, l->id);
		if(sign(l))
			if(msg2fs(nil, l, mdir) < 0){
				fprint(2, "mbox2fs: msg2fs errors\n");
				return -1;
			} else {
				if(sfd >= 0)
					fprint(sfd, "%s\n", lastsigned());
			}
		if(!dry)
			l->deleted = 1;
	}
	if(sfd >= 0)
		close(sfd);
	return 0;
}

static void
usage(void)
{
	fprint(2, "usage: %s [-acDnor] [-d mdir] [mbox]\n", argv0);
	exits("usage");
}

void
main(int argc, char*argv[])
{
	char *mbox = "mbox";
	char *mdir, *top;
	Ctype *cp;

	top = smprint("/mail/box/%s", getuser());
	mdir = smprint("%s/msgs", top);
	ARGBEGIN{
	case 'a':
		aflag = 'a';
		break;
	case 's':
		aflag = 's';
		break;
	case 'c':
		cflag++;
		break;
	case 'd':
		free(mdir);
		mdir = EARGF(usage());
		mdir = cleanpath(mdir, top);
		break;
	case 'D':
		debug++;
		break;
	case 'n':
		dry++;
		break;
	case 'o':
		oflag++;
		break;
	case 'r':
		rflag++;
		break;
	default:
		usage();
	}ARGEND;
	switch(argc){
	case 1:
		mbox = cleanpath(argv[0], top);
		break;
	case 0:
		mbox = cleanpath(mbox, top);
		break;
	default:
		usage();
	}

	for(cp = ctype; cp < ctype + nelem(ctype) - 1; cp++)
		cp->next = cp + 1;
	mbox = openmbox(mbox);
	mboxfd = mkmdir(mdir);
	mbox2fs(mbox, mdir);
	closembox();
	closemdir(mboxfd);
	exits(nil);
}
