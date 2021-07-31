#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <tapefs.h>

Fid	*fids;
Ram	*ram;
int	mfd[2];
char	user[NAMELEN];
char	mdata[MAXMSG+MAXFDATA];
Fcall	rhdr;
Fcall	thdr;
ulong	path;
Idmap	*uidmap;
Idmap	*gidmap;
int	replete;

Fid *	newfid(int);
void	ramstat(Ram*, char*);
void	io(void);
void	usage(void);
int	perm(Fid*, Ram*, int);

char	*rflush(Fid*), *rnop(Fid*), *rsession(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*),
	*rauth(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tsession]	rsession,
	[Tnop]		rnop,
	[Tattach]	rattach,
	[Tclone]	rclone,
	[Twalk]		rwalk,
	[Tclwalk]	rclwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
	[Tauth]		rauth,
};

char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enoauth[] =	"no authentication in ramfs";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";

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
	Ram *r;
	char *defmnt;
	int p[2];

	defmnt = 0;
	ARGBEGIN{
	case 'm':
		defmnt = ARGF();
		break;
	case 'p':			/* password file */
		uidmap = getpass(ARGF());
		break;
	case 'g':			/* group file */
		gidmap = getpass(ARGF());
		break;
	default:
		usage();
	}ARGEND

	if (argc==0)
		error("no file to mount");
	else if (defmnt==0)
		defmnt = argv[0];
	ram = r = (Ram *)emalloc(sizeof(Ram));
	r->busy = 1;
	r->data = 0;
	r->ndata = 0;
	r->perm = CHDIR | 0775;
	r->qid.path = CHDIR;
	r->qid.vers = 0;
	r->parent = 0;
	r->child = 0;
	r->next = 0;
	r->user = user;
	r->group = user;
	r->atime = time(0);
	r->mtime = r->atime;
	r->replete = 0;
	strcpy(r->name, ".");
	strcpy(user, getuser());
	populate(argv[0]);
	r->replete |= replete;
	if(pipe(p) < 0)
		error("pipe failed");
	mfd[0] = mfd[1] = p[0];
	notify(notifyf);

	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		error("fork");
	case 0:
		close(p[1]);
		notify(notifyf);
		io();
		break;
	default:
		close(p[0]);	/* don't deadlock if child fails */
		if(defmnt && mount(p[1], defmnt, MREPL|MCREATE, "", "") < 0)
			error("mount failed");
	}
	exits(0);
}

char*
rnop(Fid *f)
{
	USED(f);
	return 0;
}

char*
rsession(Fid *unused)
{
	Fid *f;

	USED(unused);

	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);
	return 0;
}

char*
rflush(Fid *f)
{
	USED(f);
	return 0;
}

char *
rauth(Fid *f)
{
	USED(f);
	return Enoauth;
}

char*
rattach(Fid *f)
{
	/* no authentication! */
	f->busy = 1;
	f->rclose = 0;
	f->ram = ram;
	thdr.qid = f->ram->qid;
	if(rhdr.uname[0])
		f->user = strdup(rhdr.uname);
	else
		f->user = "none";
	return 0;
}

char*
rclone(Fid *f)
{
	Fid *nf;

	if(f->open)
		return Eisopen;
	if(f->ram->busy == 0)
		return Enotexist;
	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	nf->rclose = 0;
	nf->ram = f->ram;
	nf->user = f->user;	/* no ref count; the leakage is minor */
	return 0;
}

char*
rwalk(Fid *f)
{
	Ram *r;
	char *name;
	Ram *dir;

	if((f->ram->qid.path & CHDIR) == 0)
		return Enotdir;
	if(f->ram->busy == 0)
		return Enotexist;
	f->ram->atime = time(0);
	name = rhdr.name;
	if(strcmp(name, ".") == 0){
		thdr.qid = f->ram->qid;
		return 0;
	}
	dir = f->ram;
	if(!dir || !perm(f, dir, Pexec))
		return Eperm;
	if(strcmp(name, "..") == 0){
		f->ram = dir->parent;
		thdr.qid = f->ram->qid;
		return 0;
	}
	if (!dir->replete)
		popdir(dir);
	for(r=dir->child; r; r=r->next)
		if(r->busy && strcmp(name, r->name)==0){
			thdr.qid = r->qid;
			f->ram = r;
			return 0;
		}
	return Enotexist;
}

char *
rclwalk(Fid *f)
{
	Fid *nf;
	char *err;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->rclose = 0;
	nf->ram = f->ram;
	nf->user = f->user;
	if(err = rwalk(nf))
		rclunk(nf);
	return err;
}

char *
ropen(Fid *f)
{
	Ram *r;
	int mode, trunc;

	if(f->open)
		return Eisopen;
	r = f->ram;
	if(r->busy == 0)
		return Enotexist;
	if(r->perm & CHEXCL)
		if(r->open)
			return Excl;
	mode = rhdr.mode;
	if(r->qid.path & CHDIR){
		if(mode != OREAD)
			return Eperm;
		thdr.qid = r->qid;
		return 0;
	}
	if(mode & ORCLOSE)
		return Eperm;
	trunc = mode & OTRUNC;
	mode &= OPERM;
	if(mode==OWRITE || mode==ORDWR || trunc)
		if(!perm(f, r, Pwrite))
			return Eperm;
	if(mode==OREAD || mode==ORDWR)
		if(!perm(f, r, Pread))
			return Eperm;
	if(mode==OEXEC)
		if(!perm(f, r, Pexec))
			return Eperm;
	if(trunc && (r->perm&CHAPPEND)==0){
		r->ndata = 0;
		dotrunc(r);
		r->qid.vers++;
	}
	thdr.qid = r->qid;
	f->open = 1;
	r->open++;
	return 0;
}

char *
rcreate(Fid *f)
{
	USED(f);

	return Eperm;
}

char*
rread(Fid *f)
{
	Ram *r;
	char *buf;
	long off;
	int n, cnt;

	if(f->ram->busy == 0)
		return Enotexist;
	n = 0;
	thdr.count = 0;
	off = rhdr.offset;
	cnt = rhdr.count;
	buf = thdr.data;
	if(f->ram->qid.path & CHDIR){
		if (!f->ram->replete)
			popdir(f->ram);
		if(off%DIRLEN || cnt%DIRLEN)
			return "i/o error";
		for(r=f->ram->child; off; r=r->next){
			if (r==0)
				return 0;
			if(r->busy)
				off -= DIRLEN;
		}
		for(; r && n < cnt; r=r->next){
			if(!r->busy)
				continue;
			ramstat(r, buf+n);
			n += DIRLEN;
		}
		thdr.count = n;
		return 0;
	}
	r = f->ram;
	if(off >= r->ndata)
		return 0;
	r->atime = time(0);
	n = cnt;
	if(off+n > r->ndata)
		n = r->ndata - off;
	thdr.data = doread(r, off, n);
	thdr.count = n;
	return 0;
}

char*
rwrite(Fid *f)
{
	Ram *r;
	ulong off;
	int cnt;

	r = f->ram;
	if (dopermw(f->ram)==0)
		return Eperm;
	if(r->busy == 0)
		return Enotexist;
	off = rhdr.offset;
	if(r->perm & CHAPPEND)
		off = r->ndata;
	cnt = rhdr.count;
	if(r->qid.path & CHDIR)
		return "file is a directory";
	if(off > 100*1024*1024)		/* sanity check */
		return "write too big";
	dowrite(r, rhdr.data, off, cnt);
	r->qid.vers++;
	r->mtime = time(0);
	thdr.count = cnt;
	return 0;
}

char *
rclunk(Fid *f)
{
	if(f->open)
		f->ram->open--;
	f->busy = 0;
	f->open = 0;
	f->ram = 0;
	return 0;
}

char *
rremove(Fid *f)
{
	USED(f);
	return Eperm;
}

char *
rstat(Fid *f)
{
	if(f->ram->busy == 0)
		return Enotexist;
	ramstat(f->ram, thdr.stat);
	return 0;
}

char *
rwstat(Fid *f)
{
	if(f->ram->busy == 0)
		return Enotexist;
	return Eperm;
}

void
ramstat(Ram *r, char *buf)
{
	Dir dir;

	memmove(dir.name, r->name, NAMELEN);
	dir.qid = r->qid;
	dir.mode = r->perm;
	dir.length = r->ndata;
	dir.hlength = 0;
	strcpy(dir.uid, r->user);
	strcpy(dir.gid, r->group);
	dir.atime = r->atime;
	dir.mtime = r->mtime;
	convD2M(&dir, buf);
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
		ff->open = 0;
		ff->busy = 1;
	}
	f = emalloc(sizeof *f);
	f->ram = 0;
	f->fid = fid;
	f->busy = 1;
	f->open = 0;
	f->next = fids;
	fids = f;
	return f;
}

void
io(void)
{
	char *err;
	int n, nerr;
	char buf[ERRLEN];

	errstr(buf);
	for(nerr=0, buf[0]='\0'; nerr<100; nerr++){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read(mfd[0], mdata, sizeof mdata);
		if (n==0)
			continue;
		if(n < 0){
			if (buf[0]=='\0')
				errstr(buf);
			continue;
		}
		nerr = 0;
		buf[0] = '\0';
		if(convM2S(mdata, &rhdr, n) == 0)
			continue;

/*		fprint(2, "ramfs:%F\n", &rhdr);/**/

		thdr.data = mdata + MAXMSG;
		if(!fcalls[rhdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rhdr.type])(newfid(rhdr.fid));
		if(err){
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		}else{
			thdr.type = rhdr.type + 1;
			thdr.fid = rhdr.fid;
		}
		thdr.tag = rhdr.tag;
		n = convS2M(&thdr, mdata);
		if(write(mfd[1], mdata, n) != n)
			error("mount write");
	}
	if (buf[0]=='\0' || strncmp(buf, "write to hung", 13)==0)
		exits("");
	fprint(2, "%s: mount read: %s\n", argv0, buf);
	exits(buf);
}

int
perm(Fid *f, Ram *r, int p)
{
	if (p==Pwrite && dopermw(r)==0)
		return 0;
	if((p*Pother) & r->perm)
		return 1;
	if(strcmp(f->user, r->group)==0 && ((p*Pgroup) & r->perm))
		return 1;
	if(strcmp(f->user, r->user)==0 && ((p*Powner) & r->perm))
		return 1;
	return 0;
}

void
error(char *s)
{
	fprint(2, "%s: %s: ", argv0, s);
	perror("");
	exits(s);
}

void *
emalloc(ulong n)
{
	void *p;
	p = malloc(n);
	if(!p)
		error("out of memory");
	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(!p)
		error("out of memory");
	return p;
}

void
usage(void)
{
	fprint(2, "usage: %s [-s] [-m mountpoint]\n", argv0);
	exits("usage");
}
