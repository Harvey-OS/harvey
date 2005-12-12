#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <bio.h>

typedef struct Args Args;

struct Args {
	int	argc;
	char	**argv;
};

typedef struct Dfile Dfile;
typedef struct Fid Fid;
typedef struct File File;
typedef struct Fs Fs;
typedef struct Request Request;
typedef struct Symbol Symbol;
typedef struct Tardir Tardir;

extern int threadrforkflag = RFNAMEG;

enum{
	Nstat = 1024,	/* plenty for this application */
	MAXSIZE = 8192+IOHDRSZ,
};

int messagesize = MAXSIZE;

void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[1024];

	write(2, "depend: ", 8);
	va_start(arg, fmt);
	vseprint(buf, buf+1024, fmt, arg);
	va_end(arg);
	write(2, buf, strlen(buf));
	write(2, "\n", 1);
	threadexitsall(fmt);
}

enum
{
	Nfidhash=	64,
	Ndfhash=	128,
};

struct Symbol
{
	Symbol	*next;		/* hash list chaining */
	char	*sym;
	int	fno;		/* file symbol is defined in */
};

/* source file */
struct File
{
	QLock;

	char	*name;
	Symbol	*ref;
	uchar	*refvec;	/* files resolving the references */
	uint	len;		/* length of file */
	uint	tarlen;		/* length of tar file */
	uint	mode;
	uint	mtime;

	int	use;
	int	fd;
};

/* .depend file */
struct Dfile
{
	Lock;
	int	use;		/* use count */
	int	old;		/* true if this is an superceded dfile */

	File	*file;		/* files */
	int	nfile;		/* number of files */
	int	flen;		/* length of file table */

	Symbol	**dhash;	/* hash table of symbols */
	int	hlen;		/* length of hash table */

	Dfile	*next;		/* hash chain */
	char	*path;		/* path name of dependency file */
	Qid	qid;		/* qid of the dependency file */
};

struct Fid
{
	Fid	*next;
	int	fid;
	int	ref;

	int	attached;
	int	open;
	Qid	qid;
	char	*path;
	Dfile	*df;
	Symbol	*dp;
	int	fd;
	Dir	*dir;
	int	ndir;
	int	dirindex;
};

struct Request
{
	Request	*next;
	Fid	*fid;
	Fcall	f;
	uchar	buf[1];
};

enum
{
	Tblocksize=	512,	/* tar block size */
	Tnamesize=	100,	/* tar name size */
};

struct Tardir
{
	char	name[Tnamesize];
	char	mode[8];
	char	uid[8];
	char	gid[8];
	char	size[12];
	char	mtime[12];
	char	chksum[8];
	char	linkflag;
	char	linkname[Tnamesize];
};

struct Fs
{
	Lock;

	int	fd;		/* to kernel mount point */
	Fid	*hash[Nfidhash];
	char	*root;
	Qid	rootqid;

};

struct Fsarg
{
	Fs	*fs;
	int	fd;
	char *root;
};


extern	void	fsrun(void*);
extern	Fid*	fsgetfid(Fs*, int);
extern	void	fsputfid(Fs*, Fid*);
extern	void	fsreply(Fs*, Request*, char*);
extern	void	fsversion(Fs*, Request*, Fid*);
extern	void	fsauth(Fs*, Request*, Fid*);
extern	void	fsflush(Fs*, Request*, Fid*);
extern	void	fsattach(Fs*, Request*, Fid*);
extern	void	fswalk(Fs*, Request*, Fid*);
extern	void	fsopen(Fs*, Request*, Fid*);
extern	void	fscreate(Fs*, Request*, Fid*);
extern	void	fsread(Fs*, Request*, Fid*);
extern	void	fswrite(Fs*, Request*, Fid*);
extern	void	fsclunk(Fs*, Request*, Fid*);
extern	void	fsremove(Fs*, Request*, Fid*);
extern	void	fsstat(Fs*, Request*, Fid*);
extern	void	fswstat(Fs*, Request*, Fid*);

void 	(*fcall[])(Fs*, Request*, Fid*) =
{
	[Tflush]	fsflush,
	[Tversion]	fsversion,
	[Tauth]	fsauth,
	[Tattach]	fsattach,
	[Twalk]		fswalk,
	[Topen]		fsopen,
	[Tcreate]	fscreate,
	[Tread]		fsread,
	[Twrite]	fswrite,
	[Tclunk]	fsclunk,
	[Tremove]	fsremove,
	[Tstat]		fsstat,
	[Twstat]	fswstat
};

char Eperm[]   = "permission denied";
char Eexist[]  = "file does not exist";
char Enotdir[] = "not a directory";
char Eisopen[] = "file already open";
char Enofid[] = "no such fid";
char mallocerr[]	= "malloc: %r";
char Etoolong[]	= "name too long";

char *dependlog = "depend";

int debug;
Dfile *dfhash[Ndfhash];		/* dependency file hash */
QLock dfhlock[Ndfhash];
QLock iolock;

Request*	allocreq(int);
Dfile*	getdf(char*);
void	releasedf(Dfile*);
Symbol*	dfsearch(Dfile*, char*);
void	dfresolve(Dfile*, int);
char*	mkpath(char*, char*);
int	mktar(Dfile*, Symbol*, uchar*, uint, int);
void	closetar(Dfile*, Symbol*);

void*
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		fatal(mallocerr);
	memset(p, 0, n);
	return p;
}

void *
erealloc(void *ReallocP, int ReallocN)
{
	if(ReallocN == 0)
		ReallocN = 1;
	if(!ReallocP)
		ReallocP = emalloc(ReallocN);
	else if(!(ReallocP = realloc(ReallocP, ReallocN)))
		fatal("unable to allocate %d bytes",ReallocN);
	return(ReallocP);
}

char*
estrdup(char *s)
{
	char *d, *d0;

	if(!s)
		return 0;
	d = d0 = emalloc(strlen(s)+1);
	while(*d++ = *s++)
		;
	return d0;
}

/*
 *  mount the user interface and start one request processor
 *  per CPU
 */
void
realmain(void *a)
{
	Fs *fs;
	int pfd[2];
	int srv;
	char service[128];
	struct Fsarg fsarg;
	Args *args;
	int argc;
	char **argv;

	args = a;
	argc = args->argc;
	argv = args->argv;

	fmtinstall('F', fcallfmt);

	ARGBEGIN{
		case 'd':
			debug++;
			break;
	}ARGEND
	if(argc != 2){
		fprint(2, "usage: %s [-d] svc-name directory\n", argv0);
		exits("usage");
	}
	snprint(service, sizeof service, "#s/%s", argv[0]);
	if(argv[1][0] != '/')
		fatal("directory must be rooted");

	if(pipe(pfd) < 0)
		fatal("opening pipe: %r");

	/* Typically mounted before /srv exists */
	srv = create(service, OWRITE, 0666);
	if(srv < 0)
		fatal("post: %r");
	fprint(srv, "%d", pfd[1]);
	close(srv);
	close(pfd[1]);

	time(nil);	/* open fd for time before losing / */
	if(bind(argv[1], "/", MREPL) == 0)
		fatal("can't bind %s to /", argv[1]);

	fs = emalloc(sizeof(Fs));
	fsarg.fs = fs;
	fsarg.fd = pfd[0];
	fsarg.root = argv[1];
	proccreate(fsrun, &fsarg, 16*1024);
	proccreate(fsrun, &fsarg, 16*1024);
	fsrun(&fsarg);
	exits(nil);
}

void
threadmain(int argc, char *argv[])
{
	static Args args;

	args.argc = argc;
	args.argv = argv;
	rfork(RFNAMEG);
	proccreate(realmain, &args, 16*1024);
}

char*
mkpath(char *dir, char *file)
{
	int len;
	char *path;

	len = strlen(dir) + 1;
	if(file != nil)
		len += strlen(file) + 1;
	path = emalloc(len);
	if(file != nil)
		sprint(path, "%s/%s", dir, file);
	else
		sprint(path, "%s", dir);
	return path;
}

void
fsrun(void *a)
{
	struct Fsarg *fsarg;
	Fs* fs;
	char *root;
	int n, t;
	Request *r;
	Fid *f;
	Dir *d;

	fsarg = a;
	fs = fsarg->fs;
	fs->fd = fsarg->fd;
	root = fsarg->root;
	d = dirstat("/");
	if(d == nil)
		fatal("root %s inaccessible: %r", root);
	fs->rootqid = d->qid;
	free(d);

	for(;;){
		r = allocreq(messagesize);
		qlock(&iolock);
		n = read9pmsg(fs->fd, r->buf, messagesize);
		qunlock(&iolock);
		if(n <= 0)
			fatal("read9pmsg error: %r");

		if(convM2S(r->buf, n, &r->f) == 0){
			fprint(2, "can't convert %ux %ux %ux\n", r->buf[0],
				r->buf[1], r->buf[2]);
			free(r);
			continue;
		}

		f = fsgetfid(fs, r->f.fid);
		r->fid = f;
		if(debug)
			fprint(2, "%F path %llux\n", &r->f, f->qid.path);

		t = r->f.type;
		r->f.type++;
		(*fcall[t])(fs, r, f);
		fsputfid(fs, f);
	}

}

/*
 *  any request that can get queued for a delayed reply
 */
Request*
allocreq(int bufsize)
{
	Request *r;

	r = emalloc(sizeof(Request)+bufsize);
	r->next = nil;
	return r;
}

Fid*
fsgetfid(Fs *fs, int fid)
{
	Fid *f, *nf;

	lock(fs);
	for(f = fs->hash[fid%Nfidhash]; f; f = f->next){
		if(f->fid == fid){
			f->ref++;
			unlock(fs);
			return f;
		}
	}

	nf = emalloc(sizeof(Fid));
	nf->next = fs->hash[fid%Nfidhash];
	fs->hash[fid%Nfidhash] = nf;
	nf->fid = fid;
	nf->ref = 1;
	nf->fd = -1;
	unlock(fs);
	return nf;
}

void
fsputfid(Fs *fs, Fid *f)
{
	Fid **l, *nf;

	lock(fs);
	if(--f->ref > 0){
		unlock(fs);
		return;
	}
	for(l = &fs->hash[f->fid%Nfidhash]; nf = *l; l = &nf->next)
		if(nf == f){
			*l = f->next;
			break;
		}
	unlock(fs);
	free(f);
}

void
fsreply(Fs *fs, Request *r, char *err)
{
	int n;
	uchar buf[MAXSIZE];

	if(err){
		r->f.type = Rerror;
		r->f.ename = err;
	}
	if(debug)
		fprint(2, "%F path %llux\n", &r->f, r->fid->qid.path);
	n = convS2M(&r->f, buf, messagesize);
	if(n == 0)
		fatal("bad convS2M");
	if(write(fs->fd, buf, n) != n)
		fatal("unmounted");
	free(r);
}

void
fsversion(Fs *fs, Request *r, Fid*)
{
	if(r->f.msize < 256){
		fsreply(fs, r, "version: bad message size");
		return;
	}
	if(messagesize > r->f.msize)
		messagesize = r->f.msize;
	r->f.msize = messagesize;
	r->f.version = "9P2000";
	fsreply(fs, r, nil);
}

void
fsauth(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, "depend: authentication not required");
}

void
fsflush(Fs*, Request*, Fid*)
{
}

void
fsattach(Fs *fs, Request *r, Fid *f)
{
	f->qid = fs->rootqid;
	f->path = strdup("/");
	f->df = getdf(mkpath(f->path, ".depend"));

	/* hold down the fid till the clunk */
	f->attached = 1;
	lock(fs);
	f->ref++;
	unlock(fs);

	r->f.qid = f->qid;
	fsreply(fs, r, nil);
}

void
fswalk(Fs *fs, Request *r, Fid *f)
{
	Fid *nf;
	char *name, *tmp;
	int i, nqid, nwname;
	char errbuf[ERRMAX], *err;
	Qid qid[MAXWELEM];
	Dfile *lastdf;
	char *path, *npath;
	Dir *d;
	Symbol *dp;

	if(f->attached == 0){
		fsreply(fs, r, Eexist);
		return;
	}

	if(f->fd >= 0 || f->open)
		fatal("walk of an open file");

	nf = nil;
	if(r->f.newfid != r->f.fid){
		nf = fsgetfid(fs, r->f.newfid);
		nf->attached = 1;
		nf->open = f->open;
		nf->path = strdup(f->path);
		nf->qid = f->qid;
		nf->dp = f->dp;
		nf->fd = f->fd;
		nf->df = f->df;
		if(nf->df){
			lock(nf->df);
			nf->df->use++;
			unlock(nf->df);
		}
		if(r->f.nwname == 0){
			r->f.nwqid = 0;
			fsreply(fs, r, nil);
			return;
		}
		f = nf;
	}

	err = nil;
	path = strdup(f->path);
	if(path == nil)
		fatal(mallocerr);
	nqid = 0;
	nwname = r->f.nwname;
	lastdf = f->df;

	if(nwname > 0){
		for(; nqid<nwname; nqid++){
			name = r->f.wname[nqid];

			if(strcmp(name, ".") == 0){
	Noop:
				if(nqid == 0)
					qid[nqid] = f->qid;
				else
					qid[nqid] = qid[nqid-1];
				continue;
			}

			if(strcmp(name, "..") == 0){
				name = strrchr(path, '/');
				if(name){
					if(name == path)	/* at root */
						goto Noop;
					*name = '\0';
				}
				d = dirstat(path);
				if(d == nil){
					*name = '/';
					errstr(errbuf, sizeof errbuf);
					err = errbuf;
					break;
				}
	Directory:
				qid[nqid] = d->qid;
				free(d);
				releasedf(lastdf);
				lastdf = getdf(mkpath(path, ".depend"));
				continue;
			}

			npath = mkpath(path, name);
			free(path);
			path = npath;
			d = dirstat(path);

			if(d !=nil && (d->qid.type & QTDIR))
				goto Directory;
			free(d);

			qid[nqid].type = QTFILE;
			qid[nqid].path = 0;
			qid[nqid].vers = 0;

			dp = dfsearch(lastdf, name);
			if(dp == nil){
				tmp = strdup(name);
				if(tmp == nil)
					fatal("mallocerr");
				i = strlen(tmp);
				if(i > 4 && strcmp(&tmp[i-4], ".tar") == 0){
					tmp[i-4] = 0;
					dp = dfsearch(lastdf, tmp);
				}
				free(tmp);
			}

			if(dp == nil){
				err = Eexist;
				break;
			}
			qid[nqid].path = (uvlong)dp;
			qid[nqid].vers = 0;
		}
		if(nqid == 0 && err == nil)
			err = "file does not exist";
	}

	/* for error or partial success, put the cloned fid back*/
	if(nf!=nil && (err != nil || nqid < nwname)){
		releasedf(nf->df);
		nf->df = nil;
		fsputfid(fs, nf);
	}

	if(err == nil){
		/* return (possibly short) list of qids */
		for(i=0; i<nqid; i++)
			r->f.wqid[i] = qid[i];
		r->f.nwqid = nqid;

		/* for full success, advance f */
		if(nqid > 0 && nqid == nwname){
			free(f->path);
			f->path = path;
			path = nil;

			f->qid = qid[nqid-1];
			f->dp = (Symbol*)f->qid.path;

			if(f->df != lastdf){
				releasedf(f->df);
				f->df = lastdf;
				lastdf = nil;
			}

		}
	}

	releasedf(lastdf);
	free(path);

	fsreply(fs, r, err);
}

#ifdef adf
void
fsclone(Fs *fs, Request *r, Fid *f)
{
	Fid *nf;

	if(f->attached == 0){
		fsreply(fs, r, Eexist);
		return;
	}
	nf = fsgetfid(fs, r->f.newfid);

	nf->attached = 1;
	nf->open = f->open;
	nf->path = strdup(f->path);
	nf->qid = f->qid;
	nf->dp = f->dp;
	nf->fd = f->fd;
	nf->df = f->df;
	if(nf->df){
		lock(nf->df);
		nf->df->use++;
		unlock(nf->df);
	}
	fsreply(fs, r, nil);
}

void
fswalk(Fs *fs, Request *r, Fid *f)
{
	char *name;
	int i;
	Dir d;
	char errbuf[ERRLEN];
	char *path;
	Symbol *dp;

	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}

	if(f->fd >= 0 || f->open)
		fatal("walk of an open file");

	name = r->f.name;
	if(strcmp(name, ".") == 0){
		fsreply(fs, r, nil);
		return;
	}
	if(strcmp(name, "..") == 0){
		name = strrchr(f->path, '/');
		if(name){
			if(name == f->path){
				fsreply(fs, r, nil);
				return;
			}
			*name = 0;
		}
		if(dirstat(f->path, &d) < 0){
			*name = '/';
			errstr(errbuf);
			fsreply(fs, r, errbuf);
			return;
		}
		r->f.qid = f->qid = d.qid;

		releasedf(f->df);
		f->df = getdf(mkpath(f->path, ".depend"));

		fsreply(fs, r, nil);
		return;
	}

	path = mkpath(f->path, name);
	if(dirstat(path, &d) < 0 || (d.qid.path & CHDIR) == 0){
		dp = dfsearch(f->df, name);
		if(dp == nil){
			i = strlen(name);
			if(i > 4 && strcmp(&name[i-4], ".tar") == 0){
				name[i-4] = 0;
				dp = dfsearch(f->df, name);
			}
		}
		if(dp == nil){
			fsreply(fs, r, Eexist);
			free(path);
			return;
		}
		f->dp = dp;
		d.qid.path = (uint)dp;
		d.qid.vers = 0;
	}

	free(f->path);
	f->path = path;

	if(d.qid.path & CHDIR){
		releasedf(f->df);
		f->df = getdf(mkpath(f->path, ".depend"));
	}

	r->f.qid = f->qid = d.qid;
	fsreply(fs, r, nil);
}
#endif
void
fsopen(Fs *fs, Request *r, Fid *f)
{
	int mode;
	char errbuf[ERRMAX];
	
	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}
	if(f->open){
		fsreply(fs, r, Eisopen);
		return;
	}

	mode = r->f.mode & 3;
	if(mode != OREAD){
		fsreply(fs, r, Eperm);
		return;
	}

	if(f->qid.type & QTDIR){
		f->fd = open(f->path, OREAD);
		if(f->fd < 0){
			errstr(errbuf, sizeof errbuf);
			fsreply(fs, r, errbuf);
			return;
		}
	}

	f->open = 1;
	r->f.qid = f->qid;
	fsreply(fs, r, nil);
}

void
fscreate(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

void
fsread(Fs *fs, Request *r, Fid *f)
{
	int i, n, len,skip;
	Dir d;
	Symbol *dp;
	char buf[512];

	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}
	if((int)r->f.count < 0){
		fsreply(fs, r, "bad read count");
		return;
	}

	if(f->qid.type & QTDIR){
		n = 0;
		if(f->dir == nil){
			f->ndir = dirreadall(f->fd, &f->dir);
			f->dirindex = 0;
		}
		if(f->dir == nil)
			goto Return;
		if(r->f.offset == 0)	/* seeking to zero is permitted */
			f->dirindex = 0;
		for(; f->dirindex < f->ndir; f->dirindex++){
			if((f->dir[f->dirindex].qid.type & QTDIR) == 0)
				continue;
			len = convD2M(&f->dir[f->dirindex], r->buf+n, r->f.count-n);
			if(len <= BIT16SZ)
				goto Return;
			n += len;
		}

		skip = f->dirindex - f->ndir;	/* # depend records already read */

		if(f->df){
			for(i = 0; i < f->df->hlen; i++)
				for(dp = f->df->dhash[i]; dp; dp = dp->next){
					if(skip-- > 0)
						continue;
					snprint(buf, sizeof buf, "%s.tar", dp->sym);
					d.name = buf;
					d.uid = "none";
					d.gid = "none";
					d.muid = "none";
					d.qid.type = QTFILE;
					d.qid.path = (uvlong)dp;
					d.qid.vers = 0;
					d.length = f->df->file[dp->fno].tarlen;
					d.mode = 0444;
					d.mtime = time(nil);
					d.atime = time(nil);
					len = convD2M(&d, r->buf + n, r->f.count - n);
					if(len <= BIT16SZ)
						goto Return;
					n += len;
					f->dirindex++;
				}
		}
	} else
		n = mktar(f->df, f->dp, r->buf, r->f.offset, r->f.count);

    Return:
	r->f.data = (char*)r->buf;
	r->f.count = n;
	fsreply(fs, r, nil);
}

void
fswrite(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

void
fsclunk(Fs *fs, Request *r, Fid *f)
{
	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}
	if(f->fd >= 0){
		close(f->fd);
		f->fd = -1;
	}

	if((f->qid.type & QTDIR) == 0)
		closetar(f->df, f->dp);

	releasedf(f->df);
	f->df = nil;
	free(f->dir);
	f->dir = nil;

	fsreply(fs, r, nil);
	fsputfid(fs, f);
}

void
fsremove(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

void
fsstat(Fs *fs, Request *r, Fid *f)
{
	char err[ERRMAX];
	Dir d;
	Symbol *dp;
	int n;
	uchar statbuf[Nstat];

	if(f->qid.type & QTDIR)
		n = stat(f->path, statbuf, sizeof statbuf);
	else {
		dp = f->dp;
		d.name = dp->sym;
		d.uid = "none";
		d.gid = "none";
		d.muid = "none";
		d.qid.type = QTFILE;
		d.qid.path = (uvlong)dp;
		d.qid.vers = 0;
		d.length = f->df->file[dp->fno].tarlen;
		d.mode = 0444;
		d.mtime = time(nil);
		d.atime = time(nil);
		n = convD2M(&d, statbuf, sizeof statbuf);
	}
	if(n <= BIT16SZ){
		errstr(err, sizeof err);
		fsreply(fs, r, err);
	} else {
		r->f.stat = statbuf;
		r->f.nstat = n;
		fsreply(fs, r, nil);
	}
}

void
fswstat(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

/*
 *  string hash
 */
uint
shash(char *str, int len)
{
	uint	hash;
	char	*val; 

	hash = 0;
	for(val = str; *val; val++)
		hash = (hash*13) + *val-'a';
	return hash % len;
}

/*
 *  free info about a dependency file
 */
void
freedf(Dfile *df)
{
	int i;
	Symbol *dp, *next;

	lock(df);
	df->old = 1;
	if(df->use){
		unlock(df);
		return;
	}

	unlock(df);	/* we're no longer referenced */
	for(i = 0; i < df->nfile; i++)
		free(df->file[i].name);
	free(df->file[i].refvec);
	free(df->file);
	df->file = nil;

	for(i = 0; i < df->hlen; i++)
		for(dp = df->dhash[i]; dp != nil; dp = next){
			next = dp->next;
			free(dp);
		}

	free(df->path);
	free(df);
	free(df->dhash);
	df->dhash = nil;
}

/*
 *  crack a dependency file
 */
void
newsym(char *name, int fno, Symbol **l)
{
	Symbol *dp;

	dp = emalloc(sizeof(Symbol));
	dp->sym = strdup(name);
	if(dp->sym == nil)
		fatal("mallocerr");
	dp->next = *l;
	dp->fno = fno;
	*l = dp;
}
int
awk(Biobuf *b, char **field, int n)
{
	char *line;
	int i;

	while(line = Brdline(b, '\n')){
		line[Blinelen(b)-1] = 0;
		while(*line == ' ' || *line == '\t')
			*line++ = 0;
		for(i = 0; i < n; i++){
			if(*line == 0 || *line == '#')
				break;
			field[i] = line;
			while(*line && *line != ' ' && *line != '\t')
				line++;
			while(*line == ' ' || *line == '\t')
				*line++ = 0;
		}
		if(i)
			return i;
	}

	return 0;
}

void
crackdf(Dfile *df, Biobuf *b, uint len, char *dpath)
{
	char *name;
	char *field[3];
	char path[512];
	int n, inc, ok, npath;
	Symbol **l, *dp, *next;
	File *f, *ef;
	Dir *d;

	inc = 32;
	df->flen = inc;
	df->file = emalloc(df->flen*sizeof(File));
	df->nfile = 0;

	df->hlen = 1 + len/8;
	df->dhash = emalloc(df->hlen*sizeof(Symbol*));

	l = nil;
	while((n = awk(b, field, 3)) > 0){
		if(n != 2)
			continue;

		name = field[1];
		switch(*field[0]){
		case 'F':
			if(df->flen == df->nfile){
				df->flen += inc;
				df->file = realloc(df->file, df->flen*sizeof(File));
				if(df->file == nil)
					fatal(mallocerr);
				memset(&df->file[df->nfile], 0, inc*sizeof(File));
			}
			f = &df->file[df->nfile++];
			f->name = strdup(name);
			l = &f->ref;
			/* fall through and define as a symbol */
		case 'D':
			if(l == nil)
				continue;
			newsym(name, df->nfile-1, &(df->dhash[shash(name, df->hlen)]));
			break;
		case 'R':
			if(l == nil)
				continue;
			newsym(name, 0, l);
			break;
		}
	}

	ef = &df->file[df->nfile];

	/* stat the files to get sizes */
	npath = strlen(dpath);
	if(npath+1+1 >= sizeof path)
		fatal(Etoolong);
	memmove(path, dpath, npath+1);	/* include NUL */
	name = strrchr(path, '/') + 1;
	for(f = df->file; f < ef; f++){
		n = strlen(f->name);
		if(npath+1+n+3+1 > sizeof path)
			fatal(Etoolong);
		memmove(name, f->name, n+1);	/* include NUL */
		ok = access(path, AEXIST);
		if(ok < 0){
			strcpy(name+n, ".Z");
			ok = access(path, AEXIST);
			if(ok < 0){
				strcpy(name+n, ".gz");
				ok = access(path, AEXIST);
			}
		}
		if(ok >= 0){
			free(f->name);
			f->name = strdup(name);
			if(f->name == nil)
				fatal(mallocerr);
		}
		d = dirstat(path);
		if(d){
			f->len = d->length;
			f->mode = d->mode;
			f->mtime = d->mtime;
			free(d);
		}else{
			f->len = 0;
			f->mode = 0;
			f->mtime = 0;
		}
		f->fd = -1;
	}

	/* resolve all file references */
	for(f = df->file; f < ef; f++)
		dfresolve(df, f-df->file);

	/* free the referenced symbols, don't need them anymore */
	for(f = df->file; f < ef; f++){
		f->tarlen += 2*Tblocksize;	/* tars trailing 0 blocks */
		for(dp = f->ref; dp != nil; dp = next){
			next = dp->next;
			free(dp);
		}
		f->ref = nil;
	}
}

/*
 *  get a cracked dependency file
 */
Dfile*
getdf(char *path)
{
	Dfile *df, **l;
	QLock *lk;
	Dir *d;
	int i, fd;
	Biobuf *b;

	i = shash(path, Ndfhash);
	l = &dfhash[i];
	lk = &dfhlock[i];
	qlock(lk);
	for(df = *l; df; df = *l){
		if(strcmp(path, df->path) == 0)
			break;
		l = &df->next;
	}
	d = dirstat(path);

	if(df){
		if(d!=nil && d->qid.type == df->qid.type && d->qid.vers == df->qid.vers && d->qid.vers == df->qid.vers){
			free(path);
			lock(df);
			df->use++;
			unlock(df);
			goto Return;
		}
		*l = df->next;
		freedf(df);
	}

	fd = open(path, OREAD);
	if(d == nil || fd < 0){
		close(fd);
		goto Return;
	}

	df = emalloc(sizeof(*df));
	b = emalloc(sizeof(Biobuf));

	Binit(b, fd, OREAD);
	df->qid = d->qid;
	df->path = path;
	crackdf(df, b, d->length, path);
	Bterm(b);

	free(b);

	df->next = *l;
	*l = df;
	df->use = 1;
    Return:
	qunlock(lk);
	free(d);
	return df;
}

/*
 *  stop using a dependency file.  Free it if it is no longer linked in.
 */
void
releasedf(Dfile *df)
{
	Dfile **l, *d;
	QLock *lk;
	int i;

	if(df == nil)
		return;

	/* remove from hash chain */
	i = shash(df->path, Ndfhash);
	l = &dfhash[i];
	lk = &dfhlock[i];
	qlock(lk);
	lock(df);
	df->use--;
	if(df->old == 0 || df->use > 0){
		unlock(df);
		qunlock(lk);
		return;
	}
	for(d = *l; d; d = *l){
		if(d == df){
			*l = d->next;
			break;
		}
		l = &d->next;
	}
	unlock(df);
	qunlock(lk);

	/* now we know it is unreferenced, remove it */
	freedf(df);
}

/*
 *  search a dependency file for a symbol
 */
Symbol*
dfsearch(Dfile *df, char *name)
{
	Symbol *dp;

	if(df == nil)
		return nil;
	for(dp = df->dhash[shash(name, df->hlen)]; dp; dp = dp->next)
		if(strcmp(dp->sym, name) == 0)
			return dp;
	return nil;
}

/*
 *  resolve a single file into a vector of referenced files and the sum of their
 *  lengths
 */
/* set a bit in the referenced file vector */
int
set(uchar *vec, int fno)
{
	if(vec[fno/8] & (1<<(fno&7)))
		return 1;
	vec[fno/8] |= 1<<(fno&7);
	return 0;
}
/* merge two referenced file vectors */
void
merge(uchar *vec, uchar *ovec, int nfile)
{
	nfile = (nfile+7)/8;
	while(nfile-- > 0)
		*vec++ |= *ovec++;
}
uint
res(Dfile *df, uchar *vec, int fno)
{
	File *f;
	Symbol *rp, *dp;
	int len;

	f = &df->file[fno];
	if(set(vec, fno))
		return 0;				/* already set */
	if(f->refvec != nil){
		merge(vec, f->refvec, df->nfile);	/* we've descended here before */
		return f->tarlen;
	}

	len = 0;
	for(rp = f->ref; rp; rp = rp->next){
		dp = dfsearch(df, rp->sym);
		if(dp == nil)
			continue;
		len += res(df, vec, dp->fno);
	}
	return len + Tblocksize + ((f->len + Tblocksize - 1)/Tblocksize)*Tblocksize;
}
void
dfresolve(Dfile *df, int fno)
{
	uchar *vec;
	File *f;

	f = &df->file[fno];
	vec = emalloc((df->nfile+7)/8);
	f->tarlen = res(df, vec, fno);
	f->refvec = vec;
}

/*
 *  make the tar directory block for a file
 */
uchar*
mktardir(File *f)
{
	uchar *ep;
	Tardir *tp;
	uint sum;
	uchar *p, *cp;

	p = emalloc(Tblocksize);
	tp = (Tardir*)p;

	strcpy(tp->name, f->name);
	sprint(tp->mode, "%6o ", f->mode & 0777);
	sprint(tp->uid, "%6o ", 0);
	sprint(tp->gid, "%6o ", 0);
	sprint(tp->size, "%11o ", f->len);
	sprint(tp->mtime, "%11o ", f->mtime);

	/* calculate checksum */
	memset(tp->chksum, ' ', sizeof(tp->chksum));
	sum = 0;
	ep = p + Tblocksize;
	for (cp = p; cp < ep; cp++)
		sum += *cp;
	sprint(tp->chksum, "%6o", sum);

	return p;
}

/*
 *  manage open files
 */
int
getfile(Dfile *df, File *f)
{
	int n;
	char path[512], *name;

	qlock(f);
	f->use++;
	if(f->fd < 0){
		name = strrchr(df->path, '/') + 1;
		n = snprint(path, sizeof path, "%.*s/%s", (int)(name-df->path), df->path, f->name);
		if(n >= sizeof path - UTFmax){
			syslog(0, dependlog, "path name too long: %.20s.../%.20s...", df->path, f->name);
			return -1;
		}
		f->fd = open(path, OREAD);
		if(f->fd < 0)
			syslog(0, dependlog, "can't open %s: %r", path);
	}

	return f->fd;
}
void
releasefile(File *f)
{
	--f->use;
	qunlock(f);
}
void
closefile(File *f)
{
	qlock(f);
	if(f->use == 0){
		close(f->fd);
		f->fd = -1;
	}
	qunlock(f);
}

/*
 *  return a block of a tar file
 */
int
mktar(Dfile *df, Symbol *dp, uchar *area, uint offset, int len)
{
	int fd, i, j, n, off;
	uchar *p, *buf;
	uchar *vec;
	File *f;

	f = &df->file[dp->fno];
	vec = f->refvec;
	p = area;

	/* find file */
	for(i = 0; i < df->nfile && len > 0; i++){
		if((vec[i/8] & (1<<(i&7))) == 0)
			continue;

		f = &df->file[i];
		n = Tblocksize + ((f->len + Tblocksize - 1)/Tblocksize)*Tblocksize;
		if(offset >= n){
			offset -= n;
			continue;
		}

		if(offset < Tblocksize){
			buf = mktardir(f);
			if(offset + len > Tblocksize)
				j = Tblocksize - offset;
			else
				j = len;
//if(debug)fprint(2, "reading %d bytes dir of %s\n", j, f->name);
			memmove(p, buf+offset, j);
			p += j;
			len -= j;
			offset += j;
			free(buf);
		}
		if(len <= 0)
			break;
		off = offset - Tblocksize;
		if(off >= 0 && off < f->len){
			if(off + len > f->len)
				j = f->len - off;
			else
				j = len;
			fd = getfile(df, f);
			if(fd >= 0){
//if(debug)fprint(2, "reading %d bytes from offset %d of %s\n", j, off, f->name);
				if(pread(fd, p, j, off) != j)
					syslog(0, dependlog, "%r reading %d bytes from offset %d of %s", j, off, f->name);
			}
			releasefile(f);
			p += j;
			len -= j;
			offset += j;
		}
		if(len <= 0)
			break;
		if(offset < n){
			if(offset + len > n)
				j = n - offset;
			else
				j = len;
//if(debug)fprint(2, "filling %d bytes after %s\n", j, f->name);
			memset(p, 0, j);
			p += j;
			len -= j;
		}
		offset = 0;
	}

	/* null blocks at end of tar file */
	if(offset < 2*Tblocksize && len > 0){
		if(offset + len > 2*Tblocksize)
			j = 2*Tblocksize - offset;
		else
			j = len;
//if(debug)fprint(2, "filling %d bytes at end\n", j);
		memset(p, 0, j);
		p += j;
	}

	return p - area;
}

/*
 *  close the files making up  a tar file
 */
void
closetar(Dfile *df, Symbol *dp)
{
	int i;
	uchar *vec;
	File *f;

	f = &df->file[dp->fno];
	vec = f->refvec;

	/* find file */
	for(i = 0; i < df->nfile; i++){
		if((vec[i/8] & (1<<(i&7))) == 0)
			continue;
		closefile(&df->file[i]);
	}
}

