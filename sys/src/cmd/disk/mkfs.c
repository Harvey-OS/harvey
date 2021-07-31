#include <u.h>
#include <libc.h>
#include <auth.h>
#include <bio.h>

enum{
	LEN	= 8*1024,
	HUNKS	= 128,

	/*
	 * types of destination file sytems
	 */
	Kfs = 0,
	Fs,
	Archive,
};

typedef struct File	File;

struct File{
	char	*new;
	char	*elem;
	char	*old;
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	ulong	mode;
};

void	arch(Dir*);
void	copy(Dir*);
int	copyfile(File*, Dir*, int);
void*	emalloc(ulong);
void	error(char *, ...);
void	freefile(File*);
File*	getfile(File*);
char*	getmode(char*, ulong*);
char*	getname(char*, char*, int);
char*	getpath(char*);
void	kfscmd(char *);
void	mkdir(Dir*);
int	mkfile(File*);
void	mkfs(File*, int);
char*	mkpath(char*, char*);
void	mktree(File*, int);
void	mountkfs(char*);
void	printfile(File*);
void	setnames(File*);
void	setusers(void);
void	skipdir(void);
char*	strdup(char*);
int	uptodate(Dir*, char*);
void	usage(void);
void	warn(char *, ...);

Biobuf	*b;
Biobufhdr bout;			/* stdout when writing archive */
uchar	boutbuf[2*LEN];
char	newfile[LEN];
char	oldfile[LEN];
char	*proto;
char	*cputype;
char	*users;
char	*oldroot;
char	*newroot;
char	*prog = "mkfs";
int	lineno;
char	*buf;
char	*zbuf;
int	buflen = 1024-8;
int	indent;
int	verb;
int	modes;
int	ream;
int	debug;
int	xflag;
int	sfd;
int	fskind;			/* Kfs, Fs, Archive */
char	*user;

void
main(int argc, char **argv)
{
	File file;
	char name[NAMELEN];
	int i, errs;

	user = getuser();
	name[0] = '\0';
	memset(&file, 0, sizeof file);
	file.new = "";
	file.old = 0;
	oldroot = "";
	newroot = "/n/kfs";
	users = 0;
	fskind = Kfs;
	ARGBEGIN{
	case 'a':
		fskind = Archive;
		newroot = "";
		Binits(&bout, 1, OWRITE, boutbuf, sizeof boutbuf);
		break;
	case 'd':
		fskind = Fs;
		newroot = ARGF();
		break;
	case 'D':
		debug = 1;
		break;
	case 'n':
		strncpy(name, ARGF(), NAMELEN - 1);
		name[NAMELEN - 1] = '\0';
		break;
	case 'p':
		modes = 1;
		break;
	case 'r':
		ream = 1;
		break;
	case 's':
		oldroot = ARGF();
		break;
	case 'u':
		users = ARGF();
		break;
	case 'v':
		verb = 1;
		break;
	case 'x':
		xflag = 1;
		break;
	case 'z':
		buflen = atoi(ARGF())-8;
		break;
	default:
		usage();
	}ARGEND

	if(!argc)	
		usage();

	buf = emalloc(buflen);
	zbuf = emalloc(buflen);
	memset(zbuf, 0, buflen);

	mountkfs(name);
	kfscmd("allow");
	proto = "users";
	setusers();
	cputype = getenv("cputype");
	if(cputype == 0)
		cputype = "68020";

	errs = 0;
	for(i = 0; i < argc; i++){
		proto = argv[i];
		fprint(2, "processing %s\n", proto);

		b = Bopen(proto, OREAD);
		if(!b){
			fprint(2, "%s: can't open %s: skipping\n", prog, proto);
			errs++;
			continue;
		}

		lineno = 0;
		indent = 0;
		mkfs(&file, -1);
		Bterm(b);
	}
	fprint(2, "file system made\n");
	kfscmd("disallow");
	kfscmd("sync");
	if(errs)
		exits("skipped protos");
	if(fskind == Archive){
		Bprint(&bout, "end of archive\n");
		Bterm(&bout);
	}
	exits(0);
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
	while(child && indent > level){
		if(mkfile(child))
			mkfs(child, indent);
		freefile(child);
		child = getfile(me);
	}
	if(child){
		freefile(child);
		Bseek(b, -Blinelen(b), 1);
		lineno--;
	}
}

void
mktree(File *me, int rec)
{
	File child;
	Dir d[HUNKS];
	int i, n, fd;

	fd = open(oldfile, OREAD);
	if(fd < 0){
		warn("can't open %s: %r", oldfile);
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

	if(dirstat(oldfile, &dir) < 0){
		warn("can't stat file %s: %r", oldfile);
		skipdir();
		return 0;
	}
	return copyfile(f, &dir, 0);
}

int
copyfile(File *f, Dir *d, int permonly)
{
	ulong mode;

	if(xflag){
		Bprint(&bout, "%s\t%ld\t%d\n", f->new, d->mtime, d->length);
		return (d->mode & CHDIR) != 0;
	}
	if(verb)
		fprint(2, "%s\n", f->new);
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
	if(fskind == Fs){
		strncpy(d->uid, user, NAMELEN);
		strncpy(d->gid, user, NAMELEN);
	}
	if(f->mode != ~0){
		if(permonly)
			d->mode = (d->mode & ~0666) | (f->mode & 0666);
		else if((d->mode&CHDIR) != (f->mode&CHDIR))
			warn("inconsistent mode for %s", f->new);
		else
			d->mode = f->mode;
	}
	if(!uptodate(d, newfile)){
		if(d->mode & CHDIR)
			mkdir(d);
		else
			copy(d);
	}else if(modes && dirwstat(newfile, d) < 0)
		warn("can't set modes for %s: %r", f->new);
	return (d->mode & CHDIR) != 0;
}

/*
 * check if file to is up to date with
 * respect to the file represented by df
 */
int
uptodate(Dir *df, char *to)
{
	Dir dt;

	if(fskind == Archive || ream || dirstat(to, &dt) < 0)
		return 0;
	return dt.mtime >= df->mtime;
}

void
copy(Dir *d)
{
	char cptmp[LEN], *p;
	long tot;
	int f, t, n;

	f = open(oldfile, OREAD);
	if(f < 0){
		warn("can't open %s: %r", oldfile);
		return;
	}
	t = -1;
	if(fskind == Archive)
		arch(d);
	else{
		strcpy(cptmp, newfile);
		p = utfrrune(cptmp, L'/');
		if(!p)
			error("internal temporary file error");
		strcpy(p+1, "__mkfstmp");
		t = create(cptmp, OWRITE, 0666);
		if(t < 0){
			warn("can't create %s: %r", newfile);
			close(f);
			return;
		}
	}

	for(tot = 0;; tot += n){
		n = read(f, buf, buflen);
		if(n < 0){
			warn("can't read %s: %r", oldfile);
			break;
		}
		if(n == 0)
			break;
		if(fskind == Archive){
			if(Bwrite(&bout, buf, n) != n)
				error("write error: %r");
		}else if(memcmp(buf, zbuf, buflen) == 0){
			if(seek(t, buflen, 1) < 0)
				error("can't write zeros to %s: %r", newfile);
		}else if(write(t, buf, n) < n)
			error("can't write %s: %r", newfile);
	}
	close(f);
	if(tot != d->length){
		warn("wrong number bytes written to %s (was %d should be %d)\n",
			newfile, tot, d->length);
		if(fskind == Archive){
			warn("seeking to proper position\n");
			Bseek(&bout, d->length - tot, 1);
		}
	}
	if(fskind == Archive)
		return;
	remove(newfile);
	if(dirfwstat(t, d) < 0)
		error("can't move tmp file to %s: %r", newfile);
	close(t);
}

void
mkdir(Dir *d)
{
	Dir d1;
	int fd;

	if(fskind == Archive){
		arch(d);
		return;
	}
	fd = create(newfile, OREAD, d->mode);
	if(fd < 0){
		if(dirstat(newfile, &d1) < 0 || !(d1.mode & CHDIR))
			error("can't create %s", newfile);
		if(dirwstat(newfile, d) < 0)
			warn("can't set modes for %s: %r", newfile);
		return;
	}
	if(dirfwstat(fd, d) < 0)
		warn("can't set modes for %s: %r", newfile);
	close(fd);
}

void
arch(Dir *d)
{
	Bprint(&bout, "%s %luo %s %s %lud %d\n",
		newfile, d->mode, d->uid, d->gid, d->mtime, d->length);
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
	sprint(newfile, "%s%s", newroot, f->new);
	if(f->old){
		if(f->old[0] == '/')
			sprint(oldfile, "%s%s", oldroot, f->old);
		else
			strcpy(oldfile, f->old);
	}else
		sprint(oldfile, "%s%s", oldroot, f->new);
	if(strlen(newfile) >= sizeof newfile 
	|| strlen(oldfile) >= sizeof oldfile)
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

/*
 * skip all files in the proto that
 * could be in the current dir
 */
void
skipdir(void)
{
	char *p, c;
	int level;

	if(indent < 0)
		return;
	level = indent;
	for(;;){
		indent = 0;
		p = Brdline(b, '\n');
		lineno++;
		if(!p){
			indent = -1;
			return;
		}
		while((c = *p++) != '\n')
			if(c == ' ')
				indent++;
			else if(c == '\t')
				indent += 8;
			else
				break;
		if(indent <= level){
			Bseek(b, -Blinelen(b), 1);
			lineno--;
			return;
		}
	}
}

File*
getfile(File *old)
{
	File *f;
	char elem[NAMELEN];
	char *p;
	int c;

	if(indent < 0)
		return 0;
loop:
	indent = 0;
	p = Brdline(b, '\n');
	lineno++;
	if(!p){
		indent = -1;
		return 0;
	}
	while((c = *p++) != '\n')
		if(c == ' ')
			indent++;
		else if(c == '\t')
			indent += 8;
		else
			break;
	if(c == '\n' || c == '#')
		goto loop;
	p--;
	f = emalloc(sizeof *f);
	p = getname(p, elem, sizeof elem);
	if(debug)
		fprint(2, "getfile: %s root %s\n", elem, old->new);
	f->new = mkpath(old->new, elem);
	f->elem = utfrrune(f->new, L'/') + 1;
	p = getmode(p, &f->mode);
	p = getname(p, f->uid, sizeof f->uid);
	if(!*f->uid)
		strcpy(f->uid, "-");
	p = getname(p, f->gid, sizeof f->gid);
	if(!*f->gid)
		strcpy(f->gid, "-");
	f->old = getpath(p);
	if(f->old && strcmp(f->old, "-") == 0){
		free(f->old);
		f->old = 0;
	}
	setnames(f);

	if(debug)
		printfile(f);

	return f;
}

char*
getpath(char *p)
{
	char *q, *new;
	int c, n;

	while((c = *p) == ' ' || c == '\t')
		p++;
	q = p;
	while((c = *q) != '\n' && c != ' ' && c != '\t')
		q++;
	if(q == p)
		return 0;
	n = q - p;
	new = emalloc(n + 1);
	memcpy(new, p, n);
	new[n] = 0;
	return new;
}

char*
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
		strcpy(buf, cputype);
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

char*
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

void
setusers(void)
{
	File file;
	int m;

	if(fskind != Kfs)
		return;
	m = modes;
	modes = 1;
	strcpy(file.uid, "adm");
	strcpy(file.gid, "adm");
	file.mode = CHDIR|0775;
	file.new = "/adm";
	file.elem = "adm";
	file.old = 0;
	setnames(&file);
	mkfile(&file);
	file.new = "/adm/users";
	file.old = users;
	file.elem = "users";
	file.mode = 0664;
	setnames(&file);
	mkfile(&file);
	kfscmd("user");
	mkfile(&file);
	file.mode = CHDIR|0775;
	file.new = "/adm";
	file.old = "/adm";
	file.elem = "adm";
	setnames(&file);
	mkfile(&file);
	modes = m;
}

void
mountkfs(char *name)
{
	char kname[2*NAMELEN];

	if(fskind != Kfs)
		return;
	if(name[0])
		sprint(kname, "/srv/kfs.%s", name);
	else
		strcpy(kname, "/srv/kfs");
	sfd = open(kname, ORDWR);
	if(sfd < 0){
		fprint(2, "can't open %s\n", kname);
		exits("open /srv/kfs");
	}
	if(amount(sfd, "/n/kfs", MREPL|MCREATE, "") < 0){
		fprint(2, "can't mount kfs on /n/kfs\n");
		exits("mount kfs");
	}
	close(sfd);
	strcat(kname, ".cmd");
	sfd = open(kname, ORDWR);
	if(sfd < 0){
		fprint(2, "can't open %s\n", kname);
		exits("open /srv/kfs");
	}
}

void
kfscmd(char *cmd)
{
	char buf[4*1024];
	int n;

	if(fskind != Kfs)
		return;
	if(write(sfd, cmd, strlen(cmd)) != strlen(cmd)){
		fprint(2, "%s: error writing %s: %r", prog, cmd);
		return;
	}
	for(;;){
		n = read(sfd, buf, sizeof buf - 1);
		if(n <= 0)
			return;
		buf[n] = '\0';
		if(strcmp(buf, "done") == 0 || strcmp(buf, "success") == 0)
			return;
		if(strcmp(buf, "unknown command") == 0){
			fprint(2, "%s: command %s not recognized\n", prog, cmd);
			return;
		}
	}
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

	sprint(buf, "%s: %s: %d: ", prog, proto, lineno);
	doprint(buf+strlen(buf), buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s\n", buf);
	kfscmd("disallow");
	kfscmd("sync");
	exits(0);
}

void
warn(char *fmt, ...)
{
	char buf[1024];

	sprint(buf, "%s: %s: %d: ", prog, proto, lineno);
	doprint(buf+strlen(buf), buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "%s\n", buf);
}

void
printfile(File *f)
{
	if(f->old)
		fprint(2, "%s from %s %s %s %o\n", f->new, f->old, f->uid, f->gid, f->mode);
	else
		fprint(2, "%s %s %s %o\n", f->new, f->uid, f->gid, f->mode);
}

void
usage(void)
{
	fprint(2, "usage: %s [-aprv] [-z n] [-n kfsname] [-u userfile] [-s src-fs] proto ...\n", prog);
	exits("usage");
}
