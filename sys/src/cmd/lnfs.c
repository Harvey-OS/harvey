#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <String.h>
#include <libsec.h>

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Maxsize	= 512*1024*1024,
	Maxfdata	= 8192,
	NAMELEN = 28,
};

typedef struct Fid Fid;
struct Fid
{
	short	busy;
	int	fid;
	Fid	*next;
	char	*user;
	String	*path;		/* complete path */

	int	fd;		/* set on open or create */
	Qid	qid;		/* set on open or create */
	int	attach;		/* this is an attach fd */

	ulong	diroff;		/* directory offset */
	Dir	*dir;		/* directory entries */
	int	ndir;		/* number of directory entries */
};

Fid	*fids;
int	mfd[2];
char	*user;
uchar	mdata[IOHDRSZ+Maxfdata];
uchar	rdata[Maxfdata];	/* buffer for data in reply */
uchar	statbuf[STATMAX];
Fcall	thdr;
Fcall	rhdr;
int	messagesize = sizeof mdata;
int	readonly;
char	*srvname;
int	debug;

Fid *	newfid(int);
void	io(void);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
char	*estrdup(char*);
void	usage(void);
void	fidqid(Fid*, Qid*);
char*	short2long(char*);
char*	long2short(char*, int);
void	readnames(void);
void	post(char*, int);

char	*rflush(Fid*), *rversion(Fid*), *rauth(Fid*),
	*rattach(Fid*), *rwalk(Fid*),
	*ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tversion]	rversion,
	[Tflush]	rflush,
	[Tauth]	rauth,
	[Tattach]	rattach,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enoauth[] =	"lnfs: authentication not required";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Eisdir[] =	"file is a directory";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Eversion[] =	"unknown 9P version";

void
usage(void)
{
	fprint(2, "usage: %s [-r] [-s srvname] mountpoint\n", argv0);
	exits("usage");
}

void
notifyf(void *a, char *s)
{
	USED(a);
	if(strncmp(s, "interrupt", 9) == 0)
		noted(NCONT);
	noted(NDFLT);
}

void
main(int argc, char *argv[])
{
	char *defmnt;
	int p[2];
	Dir *d;

	ARGBEGIN{
	case 'r':
		readonly = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 's':
		srvname = ARGF();
		if(srvname == nil)
			usage();
		break;
	default:
		usage();
	}ARGEND

	if(argc < 1)
		usage();
	defmnt = argv[0];
	d = dirstat(defmnt);
	if(d == nil || !(d->qid.type & QTDIR))
		sysfatal("mountpoint must be an accessible directory");
	free(d);

	if(pipe(p) < 0)
		sysfatal("pipe failed");
	mfd[0] = p[0];
	mfd[1] = p[0];

	user = getuser();
	notify(notifyf);

	if(srvname != nil)
		post(srvname, p[1]);

	if(debug)
		fmtinstall('F', fcallfmt);
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(p[1]);
		chdir(defmnt);
		io();
		break;
	default:
		close(p[0]);	/* don't deadlock if child fails */
		if(mount(p[1], -1, defmnt, MREPL|MCREATE, "") < 0)
			sysfatal("mount failed: %r");
	}
	exits(0);
}

void
post(char *srvname, int pfd)
{
	char name[128];
	int fd;

	snprint(name, sizeof name, "#s/%s", srvname);
	fd = create(name, OWRITE, 0666);
	if(fd < 0)
		sysfatal("create of %s failed: %r", srvname);
	sprint(name, "%d", pfd);
	if(write(fd, name, strlen(name)) < 0)
		sysfatal("writing %s: %r", srvname);
}
char*
rversion(Fid*)
{
	Fid *f;

	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);
	if(thdr.msize > sizeof mdata)
		rhdr.msize = sizeof mdata;
	else
		rhdr.msize = thdr.msize;
	messagesize = rhdr.msize;
	if(strncmp(thdr.version, "9P2000", 6) != 0)
		return Eversion;
	rhdr.version = "9P2000";
	return nil;
}

char*
rauth(Fid*)
{
	return Enoauth;
}

char*
rflush(Fid *f)
{
	USED(f);
	return nil;
}

char*
rattach(Fid *f)
{
	/* no authentication! */
	f->busy = 1;
	if(thdr.uname[0])
		f->user = estrdup(thdr.uname);
	else
		f->user = "none";
	if(strcmp(user, "none") == 0)
		user = f->user;
	if(f->path)
		s_free(f->path);
	f->path = s_copy(".");
	fidqid(f, &rhdr.qid);
	f->attach = 1;
	return nil;
}

char*
clone(Fid *f, Fid **nf)
{
	if(f->fd >= 0)
		return Eisopen;
	*nf = newfid(thdr.newfid);
	(*nf)->busy = 1;
	if((*nf)->path)
		s_free((*nf)->path);
	(*nf)->path = s_clone(f->path);
	(*nf)->fd = -1;
	(*nf)->user = f->user;
	(*nf)->attach = 0;
	return nil;
}

char*
rwalk(Fid *f)
{
	char *name;
	Fid *nf;
	char *err;
	int i;
	String *npath;
	Dir *d;
	char *cp;
	Qid qid;

	err = nil;
	nf = nil;
	rhdr.nwqid = 0;
	if(rhdr.newfid != rhdr.fid){
		err = clone(f, &nf);
		if(err)
			return err;
		f = nf;	/* walk the new fid */
	}
	readnames();
	npath = s_clone(f->path);
	if(thdr.nwname > 0){
		for(i=0; i<thdr.nwname && i<MAXWELEM; i++){
			name = long2short(thdr.wname[i], 0);
			if(strcmp(name, ".") == 0){
				;
			} else if(strcmp(name, "..") == 0){
				cp = strrchr(s_to_c(npath), '/');
				if(cp != nil){
					*cp = 0;
					npath->ptr = cp;
				}
			} else {
				s_append(npath, "/");
				s_append(npath, name);
			}
			d = dirstat(s_to_c(npath));
			if(d == nil)
				break;
			rhdr.nwqid++;
			qid = d->qid;
			rhdr.wqid[i] = qid;
			free(d);
		}
		if(i==0 && err == nil)
			err = Enotexist;
	}

	/* if there was an error and we cloned, get rid of the new fid */
	if(nf != nil && (err!=nil || rhdr.nwqid<thdr.nwname)){
		f->busy = 0;
		s_free(npath);
	}

	/* update the fid after a successful walk */
	if(rhdr.nwqid == thdr.nwname){
		s_free(f->path);
		f->path = npath;
	}
	return err;
}

static char*
passerror(void)
{
	static char err[256];

	rerrstr(err, sizeof err);
	return err;
}

char*
ropen(Fid *f)
{
	if(readonly && (thdr.mode & 3))
		return Eperm;
	if(f->fd >= 0)
		return Eisopen;
	f->fd = open(s_to_c(f->path), thdr.mode);
	if(f->fd < 0)
		return passerror();
	fidqid(f, &rhdr.qid);
	f->qid = rhdr.qid;
	rhdr.iounit = messagesize-IOHDRSZ;
	return nil;
}

char*
rcreate(Fid *f)
{
	char *name;

	if(readonly)
		return Eperm;
	readnames();
	name = long2short(thdr.name, 1);
	if(f->fd >= 0)
		return Eisopen;
	s_append(f->path, "/");
	s_append(f->path, name);
	f->fd = create(s_to_c(f->path), thdr.mode, thdr.perm);
	if(f->fd < 0)
		return passerror();
	fidqid(f, &rhdr.qid);
	f->qid = rhdr.qid;
	rhdr.iounit = messagesize-IOHDRSZ;
	return nil;
}

char*
rreaddir(Fid *f)
{
	int i;
	int n;

	/* reread the directory */
	if(thdr.offset == 0){
		if(f->dir)
			free(f->dir);
		f->dir = nil;
		if(f->diroff != 0)
			seek(f->fd, 0, 0);
		f->ndir = dirreadall(f->fd, &f->dir);
		f->diroff = 0;
		if(f->ndir < 0)
			return passerror();
		readnames();
		for(i = 0; i < f->ndir; i++)
			f->dir[i].name = short2long(f->dir[i].name);
	}

	/* copy in as many directory entries as possible */
	for(n = 0; f->diroff < f->ndir; n += i){
		i = convD2M(&f->dir[f->diroff], rdata+n, thdr.count - n);
		if(i <= BIT16SZ)
			break;
		f->diroff++;
	}
	rhdr.data = (char*)rdata;
	rhdr.count = n;
	return nil;
}

char*
rread(Fid *f)
{
	long n;

	if(f->fd < 0)
		return Enotexist;
	if(thdr.count > messagesize-IOHDRSZ)
		thdr.count = messagesize-IOHDRSZ;
	if(f->qid.type & QTDIR)
		return rreaddir(f);
	n = pread(f->fd, rdata, thdr.count, thdr.offset);
	if(n < 0)
		return passerror();
	rhdr.data = (char*)rdata;
	rhdr.count = n;
	return nil;
}

char*
rwrite(Fid *f)
{
	long n;

	if(readonly || (f->qid.type & QTDIR))
		return Eperm;
	if(f->fd < 0)
		return Enotexist;
	if(thdr.count > messagesize-IOHDRSZ)	/* shouldn't happen, anyway */
		thdr.count = messagesize-IOHDRSZ;
	n = pwrite(f->fd, thdr.data, thdr.count, thdr.offset);
	if(n < 0)
		return passerror();
	rhdr.count = n;
	return nil;
}

char*
rclunk(Fid *f)
{
	f->busy = 0;
	close(f->fd);
	f->fd = -1;
	f->path = s_reset(f->path);
	if(f->attach){
		free(f->user);
		f->user = nil;
	}
	f->attach = 0;
	if(f->dir != nil){
		free(f->dir);
		f->dir = nil;
	}
	f->diroff = f->ndir = 0;
	return nil;
}

char*
rremove(Fid *f)
{
	if(remove(s_to_c(f->path)) < 0)
		return passerror();
	return nil;
}

char*
rstat(Fid *f)
{
	int n;
	Dir *d;

	d = dirstat(s_to_c(f->path));
	if(d == nil)
		return passerror();
	d->name = short2long(d->name);
	n = convD2M(d, statbuf, sizeof statbuf);
	free(d);
	if(n <= BIT16SZ)
		return passerror();
	rhdr.nstat = n;
	rhdr.stat = statbuf;
	return nil;
}

char*
rwstat(Fid *f)
{
	int n;
	Dir d;

	if(readonly)
		return Eperm;
	convM2D(thdr.stat, thdr.nstat, &d, (char*)rdata);
	d.name = long2short(d.name, 1);
	n = dirwstat(s_to_c(f->path), &d);
	if(n < 0)
		return passerror();
	return nil;
}

Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fids; f; f = f->next)
		if(f->fid == fid)
			return f;
		else if(!ff && !f->busy)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = emalloc(sizeof *f);
	f->path = s_reset(f->path);
	f->fd = -1;
	f->fid = fid;
	f->next = fids;
	fids = f;
	return f;
}

void
io(void)
{
	char *err;
	int n, pid;

	pid = getpid();

	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads.
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error.
		 */
		n = read9pmsg(mfd[0], mdata, messagesize);
		if(n < 0)
			sysfatal("mount read");
		if(n == 0)
			continue;
		if(convM2S(mdata, n, &thdr) == 0)
			continue;

		if(debug)
			fprint(2, "%s %d:<-%F\n", argv0, pid, &thdr);

		if(!fcalls[thdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[thdr.type])(newfid(thdr.fid));
		if(err){
			rhdr.type = Rerror;
			rhdr.ename = err;
		}else{
			rhdr.type = thdr.type + 1;
			rhdr.fid = thdr.fid;
		}
		rhdr.tag = thdr.tag;
		if(debug)
			fprint(2, "%s %d:->%F\n", argv0, pid, &rhdr);/**/
		n = convS2M(&rhdr, mdata, messagesize);
		if(n == 0)
			sysfatal("convS2M error on write");
		if(write(mfd[1], mdata, n) != n)
			sysfatal("mount write");
	}
}

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(!p)
		sysfatal("out of memory");
	memset(p, 0, n);
	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(!p)
		sysfatal("out of memory");
	return p;
}

char *
estrdup(char *q)
{
	char *p;
	int n;

	n = strlen(q)+1;
	p = malloc(n);
	if(!p)
		sysfatal("out of memory");
	memmove(p, q, n);
	return p;
}

void
fidqid(Fid *f, Qid *q)
{
	Dir *d;

	d = dirstat(s_to_c(f->path));
	if(d == nil)
		*q = (Qid){0, 0, QTFILE};
	else {
		*q = d->qid;
		free(d);
	}
}

/*
 *  table of name translations
 *
 *  the file ./.longnames contains all the known long names.
 *  the short name is the first NAMELEN-1 bytes of the base64
 *  encoding of the MD5 hash of the longname.
 */

typedef struct Name Name;
struct Name
{
	Name	*next;
	char	shortname[NAMELEN];
	char	*longname;
};

Dir *dbstat;	/* last stat of the name file */
char *namefile = "./.longnames";
char *namebuf;
Name *names;

Name*
newname(char *longname, int writeflag)
{
	Name *np;
	int n;
	uchar digest[MD5dlen];
	int fd;

	/* chain in new name */
	n = strlen(longname);
	np = emalloc(sizeof(*np)+n+1);
	np->longname = (char*)&np[1];
	strcpy(np->longname, longname);
	md5((uchar*)longname, n, digest, nil);
	enc32(np->shortname, sizeof(np->shortname), digest, MD5dlen);
	np->next = names;
	names = np;

	/* don't change namefile if we're read only */
	if(!writeflag)
		return np;

	/* add to namefile */
	fd = open(namefile, OWRITE);
	if(fd >= 0){
		seek(fd, 0, 2);
		fprint(fd, "%s\n", longname);
		close(fd);
	}
	return np;
}

void
freenames(void)
{
	Name *np, *next;

	for(np = names; np != nil; np = next){
		next = np->next;
		free(np);
	}
	names = nil;
}

/*
 *  reread the file if the qid.path has changed.
 *
 *  read any new entries if length has changed.
 */
void
readnames(void)
{
	Dir *d;
	int fd;
	vlong offset;
	Biobuf *b;
	char *p;

	d = dirstat(namefile);
	if(d == nil){
		if(readonly)
			return;

		/* create file if it doesn't exist */
		fd = create(namefile, OREAD, DMAPPEND|0666);
		if(fd < 0)
			return;
		if(dbstat != nil)
			free(dbstat);
		dbstat = nil;
		close(fd);
		return;
	}

	/* up to date? */
	offset = 0;
	if(dbstat != nil){
		if(d->qid.path == dbstat->qid.path){
			if(d->length <= dbstat->length){
				free(d);
				return;
			}
			offset = dbstat->length;
		} else {
			freenames();
		}
		free(dbstat);
		dbstat = nil;
	}

	/* read file */
	b = Bopen(namefile, OREAD);
	if(b == nil){
		free(d);
		return;
	}
	Bseek(b, offset, 0);
	while((p = Brdline(b, '\n')) != nil){
		p[Blinelen(b)-1] = 0;
		newname(p, 0);
	}
	Bterm(b);
	dbstat = d;
}

/*
 *  look up a long name,  if it doesn't exist in the
 *  file, add an entry to the file if the writeflag is
 *  non-zero.  Return a pointer to the short name.
 */
char*
long2short(char *longname, int writeflag)
{
	Name *np;

	if(strlen(longname) < NAMELEN-1 && strpbrk(longname, " ")==nil)
		return longname;

	for(np = names; np != nil; np = np->next)
		if(strcmp(longname, np->longname) == 0)
			return np->shortname;
	if(!writeflag)
		return longname;
	np = newname(longname, !readonly);
	return np->shortname;
}

/*
 *  look up a short name, if it doesn't exist, return the
 *  longname.
 */
char*
short2long(char *shortname)
{
	Name *np;

	for(np = names; np != nil; np = np->next)
		if(strcmp(shortname, np->shortname) == 0)
			return np->longname;
	return shortname;
}
