#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "bzfs.h"

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
	Maxsize	= 512*1024*1024,
	Maxfdata	= 8192,
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
	char	*name;
	ulong	atime;
	ulong	mtime;
	char	*user;
	char	*group;
	char	*muid;
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
char	*user;
uchar	mdata[IOHDRSZ+Maxfdata];
uchar	rdata[Maxfdata];	/* buffer for data in reply */
uchar statbuf[STATMAX];
Fcall thdr;
Fcall	rhdr;
int	messagesize = sizeof mdata;

Fid *	newfid(int);
uint	ramstat(Ram*, uchar*, uint);
void	io(void);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
char	*estrdup(char*);
void	ramfsusage(void);
int	perm(Fid*, Ram*, int);
char *atom(char*);

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
char	Enoauth[] =	"no authentication in ramfs";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Eisdir[] =	"file is a directory";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Eversion[] =	"unknown 9P version";

int debug;

void
notifyf(void *a, char *s)
{
	USED(a);
	if(strncmp(s, "interrupt", 9) == 0)
		noted(NCONT);
	noted(NDFLT);
}

void
ramfsmain(int argc, char *argv[])
{
	Ram *r;
	char *defmnt;
	int p[2];
	char buf[32];
	int fd, srvfd;
	int stdio = 0;

	srvfd = -1;
	defmnt = "/tmp";
	ARGBEGIN{
	case 'D':
		debug = 1;
		break;
	case 'i':		/* this is DIFFERENT from normal ramfs; use 1 for both for kernel */
		defmnt = 0;
		stdio = 1;
		srvfd = 0;
		mfd[0] = 1;
		mfd[1] = 1;
		break;
	case 's':
		defmnt = 0;
		break;
	case 'm':
		defmnt = ARGF();
		break;
	default:
		ramfsusage();
	}ARGEND

	if(!stdio){
		if(pipe(p) < 0)
			error("pipe failed");
		srvfd = p[1];
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

	user = atom(getuser());
	notify(notifyf);
	nram = 1;
	r = &ram[0];
	r->busy = 1;
	r->data = 0;
	r->ndata = 0;
	r->perm = DMDIR | 0775;
	r->qid.type = QTDIR;
	r->qid.path = 0LL;
	r->qid.vers = 0;
	r->parent = 0;
	r->user = user;
	r->group = user;
	r->muid = user;
	r->atime = time(0);
	r->mtime = r->atime;
	r->name = estrdup(".");

	if(debug)
		fmtinstall('F', fcallfmt);
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		error("fork");
	case 0:
		close(srvfd);
		io();
		break;
	default:
		close(mfd[0]);	/* don't deadlock if child fails */
		if(defmnt && mount(srvfd, -1, defmnt, MREPL|MCREATE, "") < 0)
			error("mount failed: %r");
	}
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
	return 0;
}

char*
rauth(Fid*)
{
	return "ramfs: no authentication required";
}

char*
rflush(Fid *f)
{
	USED(f);
	return 0;
}

char*
rattach(Fid *f)
{
	/* no authentication! */
	f->busy = 1;
	f->rclose = 0;
	f->ram = &ram[0];
	rhdr.qid = f->ram->qid;
	if(thdr.uname[0])
		f->user = atom(thdr.uname);
	else
		f->user = atom("none");
	if(strcmp(user, "none") == 0)
		user = f->user;
	return 0;
}

char*
clone(Fid *f, Fid **nf)
{
	if(f->open)
		return Eisopen;
	if(f->ram->busy == 0)
		return Enotexist;
	*nf = newfid(thdr.newfid);
	(*nf)->busy = 1;
	(*nf)->open = 0;
	(*nf)->rclose = 0;
	(*nf)->ram = f->ram;
	(*nf)->user = f->user;	/* no ref count; the leakage is minor */
	return 0;
}

char*
rwalk(Fid *f)
{
	Ram *r, *fram;
	char *name;
	Ram *parent;
	Fid *nf;
	char *err;
	ulong t;
	int i;

	err = nil;
	nf = nil;
	rhdr.nwqid = 0;
	if(rhdr.newfid != rhdr.fid){
		err = clone(f, &nf);
		if(err)
			return err;
		f = nf;	/* walk the new fid */
	}
	fram = f->ram;
	if(thdr.nwname > 0){
		t = time(0);
		for(i=0; i<thdr.nwname && i<MAXWELEM; i++){
			if((fram->qid.type & QTDIR) == 0){
				err = Enotdir;
 				break;
			}
			if(fram->busy == 0){
				err = Enotexist;
				break;
			}
			fram->atime = t;
			name = thdr.wname[i];
			if(strcmp(name, ".") == 0){
    Found:
				rhdr.nwqid++;
				rhdr.wqid[i] = fram->qid;
				continue;
			}
			parent = &ram[fram->parent];
#ifdef CHECKS
			if(!perm(f, parent, Pexec)){
				err = Eperm;
				break;
			}
#endif
			if(strcmp(name, "..") == 0){
				fram = parent;
				goto Found;
			}
			for(r=ram; r < &ram[nram]; r++)
				if(r->busy && r->parent==fram-ram && strcmp(name, r->name)==0){
					fram = r;
					goto Found;
				}
			break;
		}
		if(i==0 && err == nil)
			err = Enotexist;
	}
	if(nf != nil && (err!=nil || rhdr.nwqid<thdr.nwname)){
		/* clunk the new fid, which is the one we walked */
		f->busy = 0;
		f->ram = nil;
	}
	if(rhdr.nwqid == thdr.nwname)	/* update the fid after a successful walk */
		f->ram = fram;
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
	if(r->perm & DMEXCL)
		if(r->open)
			return Excl;
	mode = thdr.mode;
	if(r->qid.type & QTDIR){
		if(mode != OREAD)
			return Eperm;
		rhdr.qid = r->qid;
		return 0;
	}
	if(mode & ORCLOSE){
		/* can't remove root; must be able to write parent */
		if(r->qid.path==0 || !perm(f, &ram[r->parent], Pwrite))
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
	if(trunc && (r->perm&DMAPPEND)==0){
		r->ndata = 0;
		if(r->data)
			free(r->data);
		r->data = 0;
		r->qid.vers++;
	}
	rhdr.qid = r->qid;
	rhdr.iounit = messagesize-IOHDRSZ;
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
	if((f->ram->qid.type&QTDIR) == 0)
		return Enotdir;
	/* must be able to write parent */
#ifdef CHECKS
	if(!perm(f, f->ram, Pwrite))
		return Eperm;
#endif
	prm = thdr.perm;
	name = thdr.name;
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
	if(prm & DMDIR)
		r->qid.type |= QTDIR;
	r->parent = parent;
	free(r->name);
	r->name = estrdup(name);
	r->user = f->user;
	r->group = f->ram->group;
	r->muid = f->ram->muid;
	if(prm & DMDIR)
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
	rhdr.qid = r->qid;
	rhdr.iounit = messagesize-IOHDRSZ;
	f->open = 1;
	if(thdr.mode & ORCLOSE)
		f->rclose = 1;
	r->open++;
	return 0;
}

char*
rread(Fid *f)
{
	Ram *r;
	uchar *buf;
	long off;
	int n, m, cnt;

	if(f->ram->busy == 0)
		return Enotexist;
	n = 0;
	rhdr.count = 0;
	off = thdr.offset;
	buf = rdata;
	cnt = thdr.count;
	if(cnt > messagesize)	/* shouldn't happen, anyway */
		cnt = messagesize;
	if(f->ram->qid.type & QTDIR){
		for(r=ram+1; off > 0; r++){
			if(r->busy && r->parent==f->ram-ram)
				off -= ramstat(r, statbuf, sizeof statbuf);
			if(r == &ram[nram-1])
				return 0;
		}
		for(; r<&ram[nram] && n < cnt; r++){
			if(!r->busy || r->parent!=f->ram-ram)
				continue;
			m = ramstat(r, buf+n, cnt-n);
			if(m == 0)
				break;
			n += m;
		}
		rhdr.data = (char*)rdata;
		rhdr.count = n;
		return 0;
	}
	r = f->ram;
	if(off >= r->ndata)
		return 0;
	r->atime = time(0);
	n = cnt;
	if(off+n > r->ndata)
		n = r->ndata - off;
	rhdr.data = r->data+off;
	rhdr.count = n;
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
	off = thdr.offset;
	if(r->perm & DMAPPEND)
		off = r->ndata;
	cnt = thdr.count;
	if(r->qid.type & QTDIR)
		return Eisdir;
	if(off+cnt >= Maxsize)		/* sanity check */
		return "write too big";
	if(off+cnt > r->ndata)
		r->data = erealloc(r->data, off+cnt);
	if(off > r->ndata)
		memset(r->data+r->ndata, 0, off-r->ndata);
	if(off+cnt > r->ndata)
		r->ndata = off+cnt;
	memmove(r->data+off, thdr.data, cnt);
	r->qid.vers++;
	r->mtime = time(0);
	rhdr.count = cnt;
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
	memset(&r->qid, 0, sizeof r->qid);
	free(r->name);
	r->name = nil;
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
#ifdef CHECKS
	if(r->qid.path == 0 || !perm(f, &ram[r->parent], Pwrite))
		return Eperm;
#endif
	ram[r->parent].mtime = time(0);
	realremove(r);
	return 0;
}

char *
rstat(Fid *f)
{
	if(f->ram->busy == 0)
		return Enotexist;
	rhdr.nstat = ramstat(f->ram, statbuf, sizeof statbuf);
	rhdr.stat = statbuf;
	return 0;
}

char *
rwstat(Fid *f)
{
	Ram *r, *s;
	Dir dir;

	if(f->ram->busy == 0)
		return Enotexist;
	convM2D(thdr.stat, thdr.nstat, &dir, (char*)statbuf);
	r = f->ram;

	/*
	 * To change length, must have write permission on file.
	 */
#ifdef CHECKS
	if(dir.length!=~0 && dir.length!=r->ndata){
	 	if(!perm(f, r, Pwrite))
			return Eperm;
	}
#endif

	/*
	 * To change name, must have write permission in parent
	 * and name must be unique.
	 */
	if(dir.name[0]!='\0' && strcmp(dir.name, r->name)!=0){
#ifdef CHECKS
	 	if(!perm(f, &ram[r->parent], Pwrite))
			return Eperm;
#endif
		for(s=ram; s<&ram[nram]; s++)
			if(s->busy && s->parent==r->parent)
			if(strcmp(dir.name, s->name)==0)
				return Eexist;
	}

#ifdef OWNERS
	/*
	 * To change mode, must be owner or group leader.
	 * Because of lack of users file, leader=>group itself.
	 */
	if(dir.mode!=~0 && r->perm!=dir.mode){
		if(strcmp(f->user, r->user) != 0)
		if(strcmp(f->user, r->group) != 0)
			return Enotowner;
	}

	/*
	 * To change group, must be owner and member of new group,
	 * or leader of current group and leader of new group.
	 * Second case cannot happen, but we check anyway.
	 */
	if(dir.gid[0]!='\0' && strcmp(r->group, dir.gid)!=0){
		if(strcmp(f->user, r->user) == 0)
		if(strcmp(f->user, dir.gid) == 0)
			goto ok;
		if(strcmp(f->user, r->group) == 0)
		if(strcmp(f->user, dir.gid) == 0)
			goto ok;
		return Enotowner;
		ok:;
	}
#endif

	/* all ok; do it */
	if(dir.mode != ~0){
		dir.mode &= ~DMDIR;	/* cannot change dir bit */
		dir.mode |= r->perm&DMDIR;
		r->perm = dir.mode;
	}
	if(dir.name[0] != '\0'){
		free(r->name);
		r->name = estrdup(dir.name);
	}
	if(dir.gid[0] != '\0')
		r->group = atom(dir.gid);

	if(dir.uid[0] != '\0')
		r->user = atom(dir.uid);

	if(dir.length!=~0 && dir.length!=r->ndata){
		r->data = erealloc(r->data, dir.length);
		if(r->ndata < dir.length)
			memset(r->data+r->ndata, 0, dir.length-r->ndata);
		r->ndata = dir.length;
	}

	if(dir.mtime != ~0)
		r->mtime = dir.mtime;

	ram[r->parent].mtime = time(0);
	return 0;
}

uint
ramstat(Ram *r, uchar *buf, uint nbuf)
{
	Dir dir;

	dir.name = r->name;
	dir.qid = r->qid;
	dir.mode = r->perm;
	dir.length = r->ndata;
	dir.uid = r->user;
	dir.gid = r->group;
	dir.muid = r->muid;
	dir.atime = r->atime;
	dir.mtime = r->mtime;
	return convD2M(&dir, buf, nbuf);
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
	f->ram = nil;
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
			error("mount read: %r");
		if(n == 0)
			continue;
		if(convM2S(mdata, n, &thdr) == 0)
			continue;

		if(debug)
			fprint(2, "ramfs %d:<-%F\n", pid, &thdr);

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
			fprint(2, "ramfs %d:->%F\n", pid, &rhdr);/**/
		n = convS2M(&rhdr, mdata, messagesize);
		if(n == 0)
			error("convS2M error on write");
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

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(!p)
		error("out of memory");
	memset(p, 0, n);
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

char *
estrdup(char *q)
{
	char *p;
	int n;

	n = strlen(q)+1;
	p = malloc(n);
	if(!p)
		error("out of memory");
	memmove(p, q, n);
	return p;
}

void
ramfsusage(void)
{
	fprint(2, "usage: %s [-is] [-m mountpoint]\n", argv0);
	exits("usage");
}

/*
 *	Custom allocators to avoid malloc overheads on small objects.
 * 	We never free these.  (See below.)
 */
typedef struct Stringtab	Stringtab;
struct Stringtab {
	Stringtab *link;
	char *str;
};
static Stringtab*
taballoc(void)
{
	static Stringtab *t;
	static uint nt;

	if(nt == 0){
		t = malloc(64*sizeof(Stringtab));
		if(t == 0)
			sysfatal("out of memory");
		nt = 64;
	}
	nt--;
	return t++;
}

static char*
xstrdup(char *s)
{
	char *r;
	int len;
	static char *t;
	static int nt;

	len = strlen(s)+1;
	if(len >= 8192)
		sysfatal("strdup big string");

	if(nt < len){
		t = malloc(8192);
		if(t == 0)
			sysfatal("out of memory");
		nt = 8192;
	}
	r = t;
	t += len;
	nt -= len;
	strcpy(r, s);
	return r;
}

/*
 *	Return a uniquely allocated copy of a string.
 *	Don't free these -- they stay in the table for the 
 *	next caller who wants that particular string.
 *	String comparison can be done with pointer comparison 
 *	if you know both strings are atoms.
 */
static Stringtab *stab[1024];

static uint
hash(char *s)
{
	uint h;
	uchar *p;

	h = 0;
	for(p=(uchar*)s; *p; p++)
		h = h*37 + *p;
	return h;
}

char*
atom(char *str)
{
	uint h;
	Stringtab *tab;
	
	h = hash(str) % nelem(stab);
	for(tab=stab[h]; tab; tab=tab->link)
		if(strcmp(str, tab->str) == 0)
			return tab->str;

	tab = taballoc();
	tab->str = xstrdup(str);
	tab->link = stab[h];
	stab[h] = tab;
	return tab->str;
}
