#include	"all.h"

typedef	struct	Direc	Direc;
typedef struct	File	File;

enum
{
	RUNOUT	= 16,
	MAXSIZE	= 60*75*60-RUNOUT,
	HUNKS	= 128,
	FLEN	= 200,		/* length of a path name */
	Nblock	= 12,

	DTdot	= 0,
	DTdotdot,
	DTiden,
	DTroot,
};

struct	Direc
{
	char	name[30];	/* name after conversion */
	char	rname[30];	/* real name in unicode */
	char	uid[30];	/* uid - put in system dep part */
	char	gid[30];	/* gid - put in system dep part */
	long	mode;		/* has CHDIR bit */
	long	length;		/* length in bytes */
	long	date;		/* mod date of file -- not used */
	char	file[FLEN];	/* path name of sorce for copy */

	char	dup;		/* flag that this name has been seen before */
	char	level;		/* hier level */
	char	conf;		/* this is the map file */
	char	noncon;		/* this file real name doesnt conform */
	char	abst;		/* this is the abstract file */
	char	biblio;		/* this is the bibliographic file */
	char	notice;		/* this is the notice file */
	int	path;
	long	blockno;

	Direc*	first;
	Direc*	next;
	Direc*	parent;
	Direc*	chain;
};


struct	File
{
	char*	new;
	char*	elem;
	char*	old;
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	ulong	mode;
};

struct
{
	char*	afile;
	char	argline[FLEN];
	char*	bfile;
	Biobuf*	bproto;
	Direc*	chain;
	char*	confname;
	int	conform;
	char*	cputype;
	uchar*	dbuf;
	int	dt;
	int	extend;
	int	indent;
	long	iobufn;
	int	iobufp;
	uchar	iobuf[Bcdrom];
	int	lineno;
	long	lpathloc;
	int	maxlevel;
	long	mpathloc;
	char	newfile[FLEN];
	char*	newroot;
	char*	nfile;
	long	now;
	char	oldfile[FLEN];
	char*	oldroot;
	int	pass;
	long	pathsize;
	char*	proto;
	Direc*	root;
	int	verb;
	char	volid[FLEN];
} d9660;

void	arch(Dir*);
void	bputc(int);
void	bputdate1(long);
void	bputdate(long);
void	bputdir(Direc*, int);
void	bputnl(long, int);
void	bputn(long, int);
void	bputnm(long, int);
void	bputs(char*, int);
void	brepeat(int, int);
void	ckpath(Direc*, int);
int	consub(char*);
int	copyfile(File*, Dir*, int);
void*	emalloc(ulong);
void	error(char*, ...);
void	freefile(File*);
File*	getfile(File*);
int	gethdr(void);
char*	getmode(char*, ulong*);
char*	getname(char*, char*, int);
char*	getpath(char*);
Direc*	lookup(char*);
int	mkfile(File*);
void	mkfs(File*, int);
char*	mkpath(char*, char*);
void	mktree(File*, int);
void	padtoblock(void);
int	Pconv(void*, Fconv*);
void	putevol(void);
void	putprivol(int);
void	setnames(File*);
char*	strdup(char*);
void	usage(void);
void	warn(char*, ...);
void	wrpath(void);
int	xwrite(void);
int	xflush(void);

void
doargs(void)
{
	char *p;
	char *argvv[10];
	char **argv;
	int argc, c, f;

	p = Brdline(d9660.bproto, '\n');
	*strchr(p, '\n') = 0;

	strncpy(d9660.argline, p, sizeof(d9660.argline));
	argc = 0;
	argv = &argvv[0];
	argv[argc++] = "mkfs";
	f = 1;		/* waiting to see beginning of word */
	for(p=d9660.argline;; p++) {
		c = *p;
		if(c == 0)
			break;
		if(c == ' ' || c == '\t') {
			*p = 0;
			f = 1;
			continue;
		}
		if(f == 1) {
			f = 0;
			argv[argc++] = p;
		}
	}
	argv[argc] = 0;

	ARGBEGIN {
	default:
		error("unknown option: %c\n", ARGC());
		break;
	case 'c':
		d9660.conform = 1;
		break;
	case 'e':
		d9660.extend = 1;
		break;
	case 'a':
		d9660.afile = ARGF();
		break;
	case 'b':
		d9660.bfile = ARGF();
		break;
	case 'n':
		d9660.nfile = ARGF();
		break;
	case 'v':
		d9660.verb = 1;
		break;
	case 's':
		d9660.oldroot = ARGF();
		break;
	} ARGEND
}

void
mk9660(void)
{
	File file;
	int track;
	long n;
	char *p;

	memset(&d9660, 0, sizeof(d9660));
	if(eol())
		print("proto file: ");
	getword();
	d9660.bproto = Bopen(word, OREAD);
	if(!d9660.bproto) {
		print("cant open %s: %r\n", word);
		return;
	}
	p = utfrrune(word, '/');
	if(p == 0)
		p = word-1;
	strcpy(d9660.volid, p+1);
	track = gettrack("disk track [number]");

	d9660.dbuf = dbufalloc(Nblock, Bcdrom);
	if(d9660.dbuf == 0) {
		return;
	}

	memset(&file, 0, sizeof(file));
	file.new = "";
	file.old = 0;
	d9660.oldroot = "";
	d9660.newroot = "";
	d9660.cputype = getenv("cputype");
	if(d9660.cputype == 0)
		d9660.cputype = "68020";
	d9660.confname = "_conform.map";

	doargs();

	fmtinstall('P', Pconv);
	d9660.now = time(0);

	for(p=d9660.volid; *p; p++)
		if(*p >= 'a' && *p <= 'z')
			*p += 'A'-'a';

	d9660.root = malloc(sizeof(*d9660.root));
	memset(d9660.root, 0, sizeof(*d9660.root));
	d9660.root->mode = CHDIR | 0755;
	strcpy(d9660.root->uid, "sys");
	strcpy(d9660.root->gid, "sys");
	d9660.root->parent = d9660.root;
	if(d9660.conform) {
		d9660.root->first = malloc(sizeof(*d9660.root->first));
		memset(d9660.root->first, 0, sizeof(*d9660.root->first));
		d9660.root->first->mode = 0444;
		strcpy(d9660.root->first->name, d9660.confname);
		strcpy(d9660.root->first->uid, "sys");
		strcpy(d9660.root->first->gid, "sys");
		d9660.root->first->parent = d9660.root;
		d9660.root->first->conf = 1;
		d9660.chain = d9660.root->first;
	}

	/* mk fs */

	d9660.lineno = 0;
	d9660.indent = 0;
	mkfs(&file, -1);
	Bterm(d9660.bproto);

	/* mk 9660 */
	d9660.pass = 1;	/* allocate the addresses */
	d9660.iobufp = 0;
	d9660.iobufn = 0;
	brepeat(0, 18*Bcdrom);
	ckpath(d9660.root, 0);
	wrpath();
	padtoblock();
	n = d9660.iobufn;
	if(d9660.maxlevel > 8)
		fprint(2, "too many directory levels: %d\n", d9660.maxlevel);
	if(d9660.afile)
		fprint(2, "abstract file not found: %s\n", d9660.afile);
	if(d9660.bfile)
		fprint(2, "bibliographic file not found: %s\n", d9660.bfile);
	if(d9660.nfile)
		fprint(2, "notice file not found: %s\n", d9660.nfile);
	print("size = %ld blocks (%ld Mbyte)\n", n, n*Bcdda/1000000);
	if(n > MAXSIZE)
		print("thats too big!!\n");

	d9660.pass = 2;	/* do the writing */
	d9660.iobufp = 0;
	d9660.iobufn = 0;
	brepeat(0, 16*Bcdrom);
	putprivol(n);
	padtoblock();
	putevol();
	padtoblock();
	ckpath(d9660.root, 0);
	wrpath();
	padtoblock();
	brepeat(0, RUNOUT*Bcdrom);
	padtoblock();

	xflush();
	dcommit(track, Tcdrom);
}

void
mkfs(File *me, int level)
{
	File *child;
	int rec;

	child = getfile(me);
	if(!child)
		return;
	if((child->elem[0] == '+' || child->elem[0] == '*') && child->elem[1] == '\0'){
		rec = child->elem[0] == '+';
		free(child->new);
		child->new = strdup(me->new);
		setnames(child);
		mktree(child, rec);
		freefile(child);
		child = getfile(me);
	}
	while(child && d9660.indent > level){
		if(mkfile(child))
			mkfs(child, d9660.indent);
		freefile(child);
		child = getfile(me);
	}
	if(child){
		freefile(child);
		Bseek(d9660.bproto, -Blinelen(d9660.bproto), 1);
		d9660.lineno--;
	}
}

void
mktree(File *me, int rec)
{
	File child;
	Dir d[HUNKS];
	int i, n, fd;

	fd = open(d9660.oldfile, OREAD);
	if(fd < 0){
		warn("can't open %s: %r", d9660.oldfile);
		return;
	}

	child = *me;
	while((n = dirread(fd, d, sizeof d)) > 0){
		n /= DIRLEN;
		for(i = 0; i < n; i++){
			child.new = mkpath(me->new, d[i].name);
			if(me->old)
				child.old = mkpath(me->old, d[i].name);
			child.elem = d[i].name;
			setnames(&child);
			if(copyfile(&child, &d[i], 1) && rec)
				mktree(&child, rec);
			free(child.new);
			if(child.old)
				free(child.old);
		}
	}
	close(fd);
}

int
mkfile(File *f)
{
	Dir dir;

	if(dirstat(d9660.oldfile, &dir) < 0){
		error("can't stat file %s: %r", d9660.oldfile);
		return 0;
	}
	return copyfile(f, &dir, 0);
}

int
copyfile(File *f, Dir *d, int permonly)
{
	ulong mode;

	if(d9660.verb)
		print("%s\n", f->new);
	memmove(d->name, f->elem, NAMELEN);
	if(d->type != 'M'){
		strncpy(d->uid, "sys", NAMELEN);
		strncpy(d->gid, "sys", NAMELEN);
		mode = (d->mode >> 6) & 7;
		d->mode |= mode | (mode << 3);
	}
	if(strcmp(f->uid, "-") != 0)
		strncpy(d->uid, f->uid, NAMELEN);
	if(strcmp(f->gid, "-") != 0)
		strncpy(d->gid, f->gid, NAMELEN);
	if(f->mode != ~0){
		if(permonly)
			d->mode = (d->mode & ~0666) | (f->mode & 0666);
		else if((d->mode&CHDIR) != (f->mode&CHDIR))
			warn("inconsistent mode for %s", f->new);
		else
			d->mode = f->mode;
	}
	arch(d);
	return (d->mode & CHDIR) != 0;
}

void
arch(Dir *dr)
{
	Direc *d;

	d = lookup(d9660.newfile);
	if(d->dup)
		fprint(2, "%s not unique\n", d9660.newfile);
	d->dup = 1;

	strncpy(d->uid, dr->uid, sizeof(d->uid));
	strncpy(d->gid, dr->gid, sizeof(d->gid));
	d->length = dr->length;
	d->mode = dr->mode;
	d->date = dr->mtime;
	strncpy(d->file, d9660.oldfile, sizeof(d->file));
}

char *
mkpath(char *prefix, char *elem)
{
	char *p;
	int n;

	n = strlen(prefix) + strlen(elem) + 2;
	p = emalloc(n);
	sprint(p, "%s/%s", prefix, elem);
	return p;
}

char *
strdup(char *s)
{
	char *t;

	t = emalloc(strlen(s) + 1);
	return strcpy(t, s);
}

void
setnames(File *f)
{
	sprint(d9660.newfile, "%s%s", d9660.newroot, f->new);
	if(f->old) {
		if(f->old[0] == '/')
			sprint(d9660.oldfile, "%s%s", d9660.oldroot, f->old);
		else
			strcpy(d9660.oldfile, f->old);
	} else
		sprint(d9660.oldfile, "%s%s", d9660.oldroot, f->new);
	if(strlen(d9660.newfile) >= sizeof(d9660.newfile) ||
	   strlen(d9660.oldfile) >= sizeof(d9660.oldfile))
		error("name overfile");
}

void
freefile(File *f)
{
	if(f->old)
		free(f->old);
	if(f->new)
		free(f->new);
	free(f);
}

File *
getfile(File *old)
{
	File *f;
	char elem[NAMELEN];
	char *p;
	int c;

	if(d9660.indent < 0)
		return 0;
loop:
	d9660.indent = 0;
	p = Brdline(d9660.bproto, '\n');
	d9660.lineno++;
	if(!p){
		d9660.indent = -1;
		return 0;
	}
	while((c = *p++) != '\n')
		if(c == ' ')
			d9660.indent++;
		else
		if(c == '\t')
			d9660.indent += 8;
		else
			break;
	if(c == '\n' || c == '#')
		goto loop;
	p--;
	f = emalloc(sizeof(*f));
	p = getname(p, elem, sizeof(elem));
	f->new = mkpath(old->new, elem);
	f->elem = utfrrune(f->new, L'/') + 1;
	p = getmode(p, &f->mode);
	p = getname(p, f->uid, sizeof(f->uid));
	if(!*f->uid)
		strcpy(f->uid, "-");
	p = getname(p, f->gid, sizeof(f->gid));
	if(!*f->gid)
		strcpy(f->gid, "-");
	f->old = getpath(p);
	if(f->old && strcmp(f->old, "-") == 0){
		free(f->old);
		f->old = 0;
	}
	setnames(f);
	return f;
}

char *
getpath(char *p)
{
	char *q;
	int c, n;

	while((c = *p) == ' ' || c == '\t')
		p++;
	q = p;
	while((c = *q) != '\n' && c != ' ' && c != '\t')
		q++;
	n = q - p;
	if(n == 0)
		return 0;
	q = emalloc(n + 1);
	memcpy(q, p, n);
	q[n] = 0;
	return q;
}

char *
getname(char *p, char *buf, int len)
{
	char *s;
	int i, c;

	while((c = *p) == ' ' || c == '\t')
		p++;
	i = 0;
	while((c = *p) != '\n' && c != ' ' && c != '\t'){
		if(i < len)
			buf[i++] = c;
		p++;
	}
	if(i == len){
		buf[len-1] = '\0';
		warn("name %s too long; truncated", buf);
	}else
		buf[i] = '\0';

	if(strcmp(buf, "$cputype") == 0)
		strcpy(buf, d9660.cputype);
	else if(buf[0] == '$'){
		s = getenv(buf+1);
		if(s == 0)
			error("can't read environment variable %s", buf+1);
		strncpy(buf, s, NAMELEN-1);
		buf[NAMELEN-1] = '\0';
		free(s);
	}
	return p;
}

char *
getmode(char *p, ulong *mode)
{
	char buf[7], *s;
	ulong m;

	*mode = ~0;
	p = getname(p, buf, sizeof buf);
	s = buf;
	if(!*s || strcmp(s, "-") == 0)
		return p;
	m = 0;
	if(*s == 'd'){
		m |= CHDIR;
		s++;
	}
	if(*s == 'a'){
		m |= CHAPPEND;
		s++;
	}
	if(*s == 'l'){
		m |= CHEXCL;
		s++;
	}
	if(s[0] < '0' || s[0] > '7'
	|| s[1] < '0' || s[1] > '7'
	|| s[2] < '0' || s[2] > '7'
	|| s[3]){
		warn("bad mode specification %s", buf);
		return p;
	}
	*mode = m | strtoul(s, 0, 8);
	return p;
}

void *
emalloc(ulong n)
{
	void *p;

	if((p = malloc(n)) == 0)
		error("out of memory");
	return p;
}

void
error(char *fmt, ...)
{
	char buf[1024];

	sprint(buf, "mkfs: %s: %d: ", d9660.proto, d9660.lineno);
	doprint(buf+strlen(buf), buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s\n", buf);
	exits(0);
}

void
warn(char *fmt, ...)
{
	char buf[1024];

	sprint(buf, "mkfs: %s: %d: ", d9660.proto, d9660.lineno);
	doprint(buf+strlen(buf), buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s\n", buf);
}

void
usage(void)
{
	fprint(2, "usage: mkfs [-aprv] [-z n] [-n kfsname] [-s src-fs] proto ...\n");
	exits("usage");
}

int
Pconv(void *o, Fconv *f)
{
	char path[FLEN];
	Direc *d;

	d = *(Direc**)o;
	if(d->parent == d)
		strcpy(path, "");
	else
		sprint(path, "%P/%s", d->parent, d->name);
	strconv(path, f);
	return sizeof(d);
}

void
bputn(long val, int size)
{

	bputnl(val, size);
	bputnm(val, size);
}

void
bputs(char *s, int size)
{
	int n;

	n = size - strlen(s);
	if(n < 0)
		s -= n;
	while(*s)
		bputc(*s++);
	if(n > 0)
		brepeat(' ', n);
}

void
bputc(int c)
{

	d9660.iobuf[d9660.iobufp++] = c;
	if(d9660.iobufp >= Bcdrom) {
		d9660.iobufn++;
		d9660.iobufp = 0;
		if(d9660.pass == 2) {
			if(xwrite())
				return;
		}
	}
}

int
xwrite(void)
{
	memmove(d9660.dbuf + d9660.dt*Bcdrom, d9660.iobuf, Bcdrom);
	d9660.dt++;
	if(d9660.dt >= Nblock) {
		d9660.dt = 0;
		if(dwrite(d9660.dbuf, Nblock))
			return 1;
	}
	return 0;
}

int
xflush(void)
{
	if(d9660.dt > 0)
		if(dwrite(d9660.dbuf, d9660.dt))
			return 1;
	return 0;
}

void
bputnl(long val, int size)
{
	switch(size) {
	default:
		fprint(2, "bad size in bputnl: %d\n", size);
		exits("bad size");
	case 2:
		bputc(val);
		bputc(val>>8);
		break;
	case 4:
		bputc(val);
		bputc(val>>8);
		bputc(val>>16);
		bputc(val>>24);
		break;
	}
}

void
bputnm(long val, int size)
{
	switch(size) {
	default:
		fprint(2, "bad size in bputnm: %d\n", size);
		exits("bad size");
	case 2:
		bputc(val>>8);
		bputc(val);
		break;
	case 4:
		bputc(val>>24);
		bputc(val>>16);
		bputc(val>>8);
		bputc(val);
		break;
	}
}

Direc*
lookup(char *path)
{
	char *p, *q;
	Direc *c, *d;
	char path1[FLEN];

	strcpy(path1, path);
	p = path1;
	strcat(p, "/");
	c = d9660.root;

loop:
	if(*p == 0)
		return c;
	q = utfrune(p, '/');
	if(q == p) {
		p++;
		goto loop;
	}

	*q++ = 0;
	for(d=c->first; d; d=d->next)
		if(strcmp(d->name, p) == 0) {
			c = d;
			p = q;
			goto loop;
		}
	d = malloc(sizeof(*d));
	memset(d, 0, sizeof(*d));
	strcpy(d->name, p);
	d->chain = d9660.chain;
	d9660.chain = d;
	d->parent = c;
	d->next = c->first;
	c->first = d;
	c = d;
	p = q;
	goto loop;
}

int
dcmp(Direc *a, Direc *b)
{

	return strcmp(a->name, b->name);
}

void
dirsort(Direc *c)
{
	Direc l[2000], *d, *e;
	int n;

	n = 0;
	for(d=c->first; d; d=d->next) {
		if(n >= nelem(l)) {
			fprint(2, "too many entries in a directory: %s\n", c->name);
			exits("2big");
		}
		l[n++] = *d;
	}
	if(n == 0)
		return;
	qsort(l, n, sizeof(l[0]), dcmp);
	n = 0;
	for(d=c->first; d; d=d->next) {
		l[n].chain = d->chain;
		l[n].next = d->next;
		*d = l[n++];
		for(e=d->first; e; e=e->next)
			e->parent = d;
	}
}

int
consub(char *p)
{
	int c;

	c = *p;
	if(c >= '0' && c <= '9')
		return 1;
	if(c >= 'a' && c <= 'z') {
		*p = c + ('A'-'a');
		return 1;
	}
	if(c == '_')
		return 1;
	return 0;
}

void
doconf(Direc *c)
{
	int n, i;
	char *s;

	strcpy(c->rname, c->name);	/* real name before conversion */
	s = strchr(c->name, '.');

	/*
	 * names without '.'
	 */
	if(s == 0) {
		s = c->name;
		n = strlen(s);

		/*
		 * if length is > 8
		 */
		if(n > 8)
			goto redo;

		/*
		 * if contains any nasty characters
		 */
		for(i=0; i<n; i++) {
			if(consub(s+i))
				continue;
			goto redo;
		}

		/*
		 * if format is [FD]dddddd
		 *	(after conversion)
		 */
		if(n == 7)
		if(s[0] == 'D' || s[0] == 'F') {
			for(i=1; i<n; i++)
				if(s[i] < '0' || s[i] > '9')
					break;
			if(i >= n)
				goto redo;
		}
		return;
	}

	/*
	 * names with '.'
	 * if prefix length is > 8
	 */
	n = s - c->name;
	if(n > 8)
		goto redo;

	/*
	 * if prefix contains any nasty characters
	 */
	s = c->name;
	for(i=0; i<n; i++) {
		if(consub(s+i))
			continue;
		goto redo;
	}

	/*
	 * if suffix length is > 3
	 */
	s += n+1;
	n = strlen(s);
	if(n > 3)
		goto redo;

	/*
	 * if suffix contains any nasty characters
	 */
	for(i=0; i<n; i++) {
		if(consub(s+i))
			continue;
		goto redo;
	}
	return;


	/*
	 * convert to [FD]dddddd
	 */
redo:
	sprint(c->name, "F%.6d", d9660.conform);
	if(c->mode & CHDIR)
		c->name[0] = 'D';
	d9660.conform++;
	c->noncon = 1;
}

void
ckpath(Direc *c, int lev)
{
	Direc *d;
	int f;
	long n, l, i;

	if(d9660.pass == 1) {
		if(c->length && (c->mode & CHDIR)) {
			fprint(2, "%P:both directory and file\n", c);
			if(c->first)
				c->length = 0;
			else
				c->mode &= ~CHDIR;
		}
		if(c->first && !(c->mode & CHDIR)) {
			fprint(2, "%P: mod not set to DIR\n", c);
			c->mode |= CHDIR;
		}
		c->level = lev;
		if(d9660.conform)
			for(d=c->first; d; d=d->next) {
				if(lev == 0 && d->first == 0) {
					if(d9660.afile && strcmp(d->name, d9660.afile) == 0) {
						d->abst = 1;
						d9660.afile = 0;
					}
					if(d9660.bfile && strcmp(d->name, d9660.bfile) == 0) {
						d->biblio = 1;
						d9660.bfile = 0;
					}
					if(d9660.nfile && strcmp(d->name, d9660.nfile) == 0) {
						d->notice = 1;
						d9660.nfile = 0;
					}
				}
				doconf(d);
			}
		dirsort(c);
		if(lev > d9660.maxlevel)
			d9660.maxlevel = lev;
	}

	padtoblock();
	if(c->conf)
		return;
	n = d9660.iobufn;

	if(d9660.pass == 2) {
		if(c->blockno != n)
			fprint(2, "%s: phase error 1\n", c->file);
		d9660.iobufn = c->blockno;
	}

	c->blockno = n;
	if(c->mode & CHDIR) {
		bputdir(c, DTdot);
		bputdir(c->parent, DTdotdot);
		for(d=c->first; d; d=d->next) {
			if(d->parent != c)
				print("not parent\n");
			bputdir(d, DTiden);
		}
		padtoblock();
		if(d9660.pass == 2)
			if(c->length != (d9660.iobufn-n)*Bcdrom + d9660.iobufp)
				fprint(2, "%s: phase error 2\n", c->file);
		c->length = (d9660.iobufn-n)*Bcdrom + d9660.iobufp;

		for(d=c->first; d; d=d->next)
			ckpath(d, lev+1);
		return;
	}

	n = (c->length + Bcdrom - 1) / Bcdrom;	/* number of blocks */
	d9660.iobufn += n;
	if(d9660.pass != 2)
		return;

	if(d9660.verb)
		print("copy %s %ld\n", c->file, c->length);

	f = open(c->file, OREAD);
	l = 0;

	if(f >= 0) {
		for(;;) {
			i = read(f, d9660.iobuf, Bcdrom);
			if(i <= 0) {
				if(i < 0)
					fprint(2, "%s: read error: %r\n", c->file);
				break;
			}
			l += i;
			if(i < Bcdrom)
				memset(d9660.iobuf+i, 0, Bcdrom-i);
			if(n > 0) {
				n--;
				if(xwrite())
					n = 0;
			}
		}
		close(f);
	} else
		fprint(2, "%s: cannot open: %r\n", c->file);

	while(n > 0) {
		memset(d9660.iobuf, 0, Bcdrom);
		if(xwrite())
			break;
		n--;
	}
	if(c->length != l)
		fprint(2, "%s: phase error file length %ld was %ld\n",
			c->file, l, c->length);
}

void
padtoblock(void)
{
	int n;

	n = Bcdrom - d9660.iobufp;
	if(n != Bcdrom)
		brepeat(0, n);
}

void
brepeat(int c, int n)
{
	int i;

	for(i=0; i<n; i++)
		bputc(c);
}

void
putprivol(int size)
{
	Direc *af, *bf, *nf, *d;

	bputc(1);				/* primary volume descriptor */
	bputs("CD001", 5);			/* standard identifier */
	bputc(1);				/* volume descriptor version */
	bputc(0);				/* unused */
	bputs("PLAN 9", 32);			/* system identifier */
	bputs(d9660.volid, 32);			/* volume identifier */
	brepeat(0, 8);				/* unused */
	bputn(size, 4);				/* volume space size */
	brepeat(0, 32);				/* unused */
	bputn(1, 2);				/* volume set size */
	bputn(1, 2);				/* volume sequence number */
	bputn(Bcdrom, 2);			/* logical block size */
	bputn(d9660.pathsize, 4);			/* path table size */
	bputnl(d9660.lpathloc, 4);			/* location of Lpath */
	bputnl(0, 4);				/* location of optional Lpath */
	bputnm(d9660.mpathloc, 4);			/* location of Mpath */
	bputnm(0, 4);				/* location of optional Mpath */
	bputdir(d9660.root, DTroot);			/* root directory */
	bputs("ANDREW_bob", 128);		/* volume set identifier */
	bputs("ROB_bob", 128);			/* publisher identifier */
	bputs("KEN_bob", 128);			/* data preparer identifier */
	bputs("PHILW_bob", 128);		/* application identifier */

	af = 0;
	bf = 0;
	nf = 0;
	for(d=d9660.chain; d; d=d->chain) {
		if(d->abst)
			af = d;
		if(d->biblio)
			bf = d;
		if(d->notice)
			nf = d;
	}
	if(nf)					/* copyright file */
		bputs(nf->name, 37);
	else
		bputs("", 37);
	if(af)					/* abstract file */
		bputs(af->name, 37);
	else
		bputs("", 37);
	if(bf)					/* biblographic file */
		bputs(bf->name, 37);
	else
		bputs("", 37);

	bputdate1(d9660.now);			/* volume creation date */
	bputdate1(d9660.now);			/* volume modification date */
	bputdate1(0);				/* volume expiration date */
	bputdate1(0);				/* volume effective date */
	bputc(1);				/* file structure version */
}

void
putevol(void)
{

	bputc(255);				/* volume descriptor set terminator */
	bputs("CD001", 5);			/* standard identifier */
	bputc(1);				/* volume descriptor version */
}

void
bputdir(Direc *d, int dot)
{
	int f, n, l;
	long o;

	f = 0;
	if(d->mode & CHDIR)
		f |= 2;

	n = 1;
	if(dot == DTiden)
		n = strlen(d->name);

	l = 33+n;
	if(l & 1)
		l++;

	if(d9660.extend && dot == DTiden) {
		if(d->noncon)
			l += strlen(d->rname);
		l += strlen(d->uid);
		l += strlen(d->gid);
		l += 3+8;
		if(l & 1)
			l++;
	}

	o = Bcdrom - d9660.iobufp;
	if(o < l)
		padtoblock();

	bputc(l);			/* length of directory record */
	bputc(0);			/* extended attribute record length */
	bputn(d->blockno, 4);		/* location of extent */
	bputn(d->length, 4);		/* data length */
	bputdate(d9660.now);		/* recorded date */
	bputc(f);			/* file flags */
	bputc(0);			/* file unit size */
	bputc(0);			/* interleave gap size */
	bputn(1, 2);			/* volume sequence number */
	bputc(n);			/* length of file identifier */

	if(dot == DTiden)		/* identifier */
		bputs(d->name, n);
	else
	if(dot == DTdotdot)
		bputc(1);
	else
		bputc(0);

	if(d9660.iobufp & 1)			/* pad */
		bputc(0);

	/*
	 * plan 9 extension in system use space on end
	 * of each dirsctory. contains the real file
	 * name if the file was not conformal. also
	 * contains string gid and uid and 32-bit mode.
	 */
	if(d9660.extend && dot == DTiden) {	/* system use */
		if(d->noncon) {
			n = strlen(d->rname);
			bputc(n);
			bputs(d->rname, n);
		} else
			bputc(0);

		n = strlen(d->uid);
		bputc(n);
		bputs(d->uid, n);

		n = strlen(d->gid);
		bputc(n);
		bputs(d->gid, n);

		if(d9660.iobufp & 1)
			bputc(0);
		bputn(d->mode, 4);
	}
}

void
bputdate(long ust)
{
	Tm *tm;

	if(ust == 0) {
		brepeat(0, 7);
		return;
	}
	tm = gmtime(ust);
	bputc(tm->year);
	bputc(tm->mon+1);
	bputc(tm->mday);
	bputc(tm->hour);
	bputc(tm->min);
	bputc(tm->sec);
	bputc(0);
}

void
bputdate1(long ust)
{
	Tm *tm;
	char str[20];

	if(ust == 0) {
		brepeat('0', 16);
		bputc(0);
		return;
	}
	tm = gmtime(ust);
	sprint(str, "%.4d%.2d%.2d%.2d%.2d%.4d",
		tm->year+1900,
		tm->mon+1,
		tm->mday,
		tm->hour,
		tm->min,
		tm->sec*100);
	bputs(str, 16);
	bputc(0);
}

int
pathcmp(void *o1, void *o2)
{
	Direc *a, *b;
	char pn1[FLEN], pn2[FLEN];
	int n;

	a = *(Direc**)o1;
	b = *(Direc**)o2;
	n = a->level - b->level;
	if(n)
		return n;
	sprint(pn1, "%P", a);
	sprint(pn2, "%P", b);
	return strcmp(pn1, pn2);
}

void
dopath(Direc *c, int endian)
{
	int n, i;

	n = strlen(c->name);
	if(n == 0)
		n = 1;
	bputc(n);
	bputc(0);
	if(endian) {
		bputnm(c->blockno, 4);
		bputnm(c->parent->path, 2);
	} else {
		bputnl(c->blockno, 4);
		bputnl(c->parent->path, 2);
	}
	for(i=0; i<n; i++)
		bputc(c->name[i]);
	if(n & 1)
		bputc(0);
}

void
wrpath(void)
{
	Direc *d, *c;
	char str[100];
	Direc *path[1000];
	int i, np;
	long n;

	/*
	 * add file of non-conforming names
	 */
	c = 0;
	if(d9660.conform)
		for(d=d9660.chain; d; d=d->chain)
			if(d->conf) {
				c = d;
				break;
			}
	if(c) {
		padtoblock();
		n = d9660.iobufn;
		if(d9660.pass == 2)
			if(c->blockno != d9660.iobufn)
				fprint(2, "phase error 4\n");
		c->blockno = n;
		for(d=d9660.chain; d; d=d->chain)
			if(d->noncon) {
				sprint(str, "%s %s\n", d->name, d->rname);
				bputs(str, strlen(str));
			}
		if(d9660.pass == 2)
			if(c->length != (d9660.iobufn-n)*Bcdrom + d9660.iobufp)
				fprint(2, "phase error 5\n");
		c->length = (d9660.iobufn-n)*Bcdrom + d9660.iobufp;
	}

	/*
	 * do the path crap
	 */
	np = 0;
	for(d=d9660.chain; d; d=d->chain)
		if(d->mode & CHDIR)
			path[np++] = d;
	path[np++] = d9660.root;

	qsort(path, np, sizeof(path[0]), pathcmp);
	for(i=0; i<np; i++)
		path[i]->path = i+1;

	padtoblock();
	n = d9660.iobufn;
	d9660.lpathloc = n;
	for(i=0; i<np; i++)
		dopath(path[i], 0);
	d9660.pathsize = (d9660.iobufn-n)*Bcdrom + d9660.iobufp;

	padtoblock();
	n = d9660.iobufn;
	d9660.mpathloc = n;
	for(i=0; i<np; i++)
		dopath(path[i], 1);
}
