#include <u.h>
#include <libc.h>
#include <fcall.h>

/*
 * Rather than reading /adm/users, which is a lot of work for
 * a toy program, we assume all groups have the form
 *	NNN:user:user:
 * meaning that each user is the leader of his own group.
 */

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nram	= 512,
};

typedef struct Fid Fid;
typedef struct Ram Ram;

struct Fid
{
	short	busy;
	short	open;
	short	rclose;
	int	fid;
	Fid	*next;
	char	*user;
	Ram	*ram;
};

struct Ram
{
	short	busy;
	short	open;
	long	parent;		/* index in Ram array */
	Qid	qid;
	long	perm;
	char	name[NAMELEN];
	ulong	atime;
	ulong	mtime;
	char	*user;
	char	*group;
	char	*data;
	long	ndata;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

ulong	path;		/* incremented for each new file */
Fid	*fids;
Ram	ram[Nram];
int	nram;
int	mfd[2];
char	user[NAMELEN];
char	mdata[MAXMSG+MAXFDATA];
Fcall	rhdr;
Fcall	thdr;

Fid *	newfid(int);
void	ramstat(Ram*, char*);
void	error(char*);
void	io(void);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
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
	char buf[12];
	int fd;
	int stdio = 0;

	defmnt = "/tmp";
	ARGBEGIN{
	case 'i':
		defmnt = 0;
		stdio = 1;
		mfd[0] = 0;
		mfd[1] = 1;
		break;
	case 's':
		defmnt = 0;
		break;
	case 'm':
		defmnt = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(pipe(p) < 0)
		error("pipe failed");
	if(!stdio){
		mfd[0] = p[0];
		mfd[1] = p[0];
		if(defmnt == 0){
			fd = create("#s/ramfs", OWRITE, 0666);
			if(fd < 0)
				error("create of /srv/ramfs failed");
			sprint(buf, "%d", p[1]);
			if(write(fd, buf, strlen(buf)) < 0)
				error("writing /srv/ramfs");
		}
	}

	notify(notifyf);
	nram = 1;
	r = &ram[0];
	r->busy = 1;
	r->data = 0;
	r->ndata = 0;
	r->perm = CHDIR | 0775;
	r->qid.path = CHDIR;
	r->qid.vers = 0;
	r->parent = 0;
	r->user = user;
	r->group = user;
	r->atime = time(0);
	r->mtime = r->atime;
	strcpy(r->name, ".");
	strcpy(user, getuser());

/*	fmtinstall('F', fcallconv);/**/
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		error("fork");
	case 0:
		close(p[1]);
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
	f->ram = &ram[0];
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
	Ram *parent;

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
	parent = &ram[f->ram->parent];
	if(!perm(f, parent, Pexec))
		return Eperm;
	if(strcmp(name, "..") == 0){
		f->ram = parent;
		thdr.qid = f->ram->qid;
		return 0;
	}
	for(r=ram; r < &ram[nram]; r++)
		if(r->busy && r->parent==f->ram-ram && strcmp(name, r->name)==0){
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
	if(mode & ORCLOSE){
		/* must be able to write parent */
		if(!perm(f, &ram[r->parent], Pwrite))
			return Eperm;
		f->rclose = 1;
	}
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
		if(r->data)
			free(r->data);
		r->data = 0;
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
	Ram *r;
	char *name;
	long parent, prm;

	if(f->open)
		return Eisopen;
	if(f->ram->busy == 0)
		return Enotexist;
	parent = f->ram - ram;
	if((f->ram->qid.path&CHDIR) == 0)
		return Enotdir;
	/* must be able to write parent */
	if(!perm(f, f->ram, Pwrite))
		return Eperm;
	prm = rhdr.perm;
	name = rhdr.name;
	if(strcmp(name, ".")==0 || strcmp(name, "..")==0)
		return Ename;
	for(r=ram; r<&ram[nram]; r++)
		if(r->busy && parent==r->parent)
		if(strcmp((char*)name, r->name)==0)
			return Einuse;
	for(r=ram; r->busy; r++)
		if(r == &ram[Nram-1])
			return "no free ram resources";
	r->busy = 1;
	r->qid.path = ++path;
	r->qid.vers = 0;
	if(prm & CHDIR)
		r->qid.path |= CHDIR;
	r->parent = parent;
	strcpy(r->name, (char*)name);
	r->user = f->user;
	r->group = f->ram->group;
	if(prm & CHDIR)
		prm = (prm&~0777) | (f->ram->perm&prm&0777);
	else
		prm = (prm&(~0777|0111)) | (f->ram->perm&prm&0666);
	r->perm = prm;
	r->ndata = 0;
	if(r-ram >= nram)
		nram = r - ram + 1;
	r->atime = time(0);
	r->mtime = r->atime;
	f->ram->mtime = r->atime;
	f->ram = r;
	thdr.qid = r->qid;
	f->open = 1;
	r->open++;
	return 0;
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
		if(off%DIRLEN || cnt%DIRLEN)
			return "i/o error";
		for(r=ram+1; off; r++){
			if(r->busy && r->parent==f->ram-ram)
				off -= DIRLEN;
			if(r == &ram[nram-1])
				return 0;
		}
		for(; r<&ram[nram] && n < cnt; r++){
			if(!r->busy || r->parent!=f->ram-ram)
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
	thdr.data = r->data+off;
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
	if(off+cnt > r->ndata)
		r->data = erealloc(r->data, off+cnt);
	if(off > r->ndata)
		memset(r->data+r->ndata, 0, off-r->ndata);
	if(off+cnt > r->ndata)
		r->ndata = off+cnt;
	memmove(r->data+off, rhdr.data, cnt);
	r->qid.vers++;
	r->mtime = time(0);
	thdr.count = cnt;
	return 0;
}

void
realremove(Ram *r)
{
	r->ndata = 0;
	if(r->data)
		free(r->data);
	r->data = 0;
	r->parent = 0;
	r->qid = (Qid){0, 0};
	r->name[0] = 0;
	r->busy = 0;
}

char *
rclunk(Fid *f)
{
	if(f->open)
		f->ram->open--;
	if(f->rclose)
		realremove(f->ram);
	f->busy = 0;
	f->open = 0;
	f->ram = 0;
	return 0;
}

char *
rremove(Fid *f)
{
	Ram *r;

	if(f->open)
		f->ram->open--;
	f->busy = 0;
	f->open = 0;
	r = f->ram;
	f->ram = 0;
	if(!perm(f, &ram[r->parent], Pwrite))
		return Eperm;
	ram[r->parent].mtime = time(0);
	realremove(r);
	return 0;
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
	Ram *r, *s;
	Dir dir;

	if(f->ram->busy == 0)
		return Enotexist;
	convM2D(rhdr.stat, &dir);
	r = f->ram;

	/*
	 * To change name, must have write permission in parent
	 * and name must be unique.
	 */
	if(strcmp(dir.name, r->name) != 0){
	 	if(!perm(f, &ram[r->parent], Pwrite))
			return Eperm;
		for(s=ram; s<&ram[nram]; s++)
			if(s->busy && s->parent==r->parent)
			if(strcmp(dir.name, s->name)==0)
				return Eexist;
	}

	/*
	 * To change mode, must be owner or group leader to change mode.
	 * Because of lack of users file, leader=>group itself.
	 */
	if(r->perm != dir.mode){
		if(strcmp(f->user, r->user) != 0)
		if(strcmp(f->user, r->group) != 0)
			return Enotowner;
	}

	/*
	 * To change group, must be owner and member of new group,
	 * or leader of current group and leader of new group.
	 * Second case cannot happen, but we check anyway.
	 */
	if(strcmp(r->group, dir.gid) != 0){
		if(strcmp(f->user, r->user) == 0)
		if(strcmp(f->user, dir.gid) == 0)
			goto ok;
		if(strcmp(f->user, r->group) == 0)
		if(strcmp(f->user, dir.gid) == 0)
			goto ok;
		return Enotowner;
		ok:;
	}

	/* all ok; do it */
	dir.mode &= ~CHDIR;	/* cannot change dir bit */
	dir.mode |= r->perm&CHDIR;
	r->perm = dir.mode;
	memmove(r->name, dir.name, NAMELEN);
	r->group = strdup(dir.gid);
	ram[r->parent].mtime = time(0);
	return 0;
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
		return ff;
	}
	f = emalloc(sizeof *f);
	f->ram = 0;
	f->fid = fid;
	f->next = fids;
	fids = f;
	return f;
}

void
io(void)
{
	char *err;
	int n;

	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read(mfd[0], mdata, sizeof mdata);
		if(n == 0)
			continue;
		if(n < 0)
			error("mount read");
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
}

int
perm(Fid *f, Ram *r, int p)
{
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
