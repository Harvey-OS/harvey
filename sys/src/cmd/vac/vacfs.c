#include "stdinc.h"
#include <auth.h>
#include <fcall.h>
#include "vac.h"

typedef struct Fid Fid;
typedef struct DirBuf DirBuf;

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
};

enum
{
	DirBufSize = 20,
};

struct Fid
{
	short busy;
	short open;
	int fid;
	char *user;
	Qid qid;
	VacFile *file;

	DirBuf *db;

	Fid	*next;
};

struct DirBuf
{
	VacDirEnum *vde;
	VacDir buf[DirBufSize];
	int i, n;
	int eof;
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

Fid	*fids;
uchar	*data;
int	mfd[2];
char	*user;
uchar	mdata[8192+IOHDRSZ];
int messagesize = sizeof mdata;
Fcall	rhdr;
Fcall	thdr;
VacFS	*fs;
VtSession *session;

Fid *	newfid(int);
void	error(char*);
void	io(void);
void	shutdown(void);
void	usage(void);
int	perm(Fid*, int);
int	permf(VacFile*, char*, int);
ulong	getl(void *p);
void	init(char*, char*, long, int);
static void warn(char *fmt, ...);
static void debug(char *fmt, ...);
DirBuf	*dirBufAlloc(VacFile*);
VacDir	*dirBufGet(DirBuf*);
int	dirBufUnget(DirBuf*);
void	dirBufFree(DirBuf*);
int	vacdirread(Fid *f, char *p, long off, long cnt);
int	vacDirStat(VacDir *vd, uchar *p, int np);

char	*rflush(Fid*), *rversion(Fid*),
	*rauth(Fid*), *rattach(Fid*), *rwalk(Fid*),
	*ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tversion]	rversion,
	[Tattach]	rattach,
	[Tauth]		rauth,
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
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Erdonly[] = 	"read only file system";
char	Eio[] = 	"i/o error";

int dflag;

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
	char buf[12];
	int fd;
	int stdio = 0;
	char *host = nil;
	long ncache = 1000;
	int readOnly = 1;

	defmnt = "/n/vac";
	ARGBEGIN{
	case 'd':
		fmtinstall('F', fcallfmt);
		dflag = 1;
		break;
	case 'c':
		ncache = atoi(ARGF());
		break;
	case 'i':
		defmnt = 0;
		stdio = 1;
		mfd[0] = 0;
		mfd[1] = 1;
		break;
	case 'h':
		host = ARGF();
		break;
	case 'r':
		readOnly = 0;
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

	if(argc != 1)
		usage();

	vtAttach();

	init(argv[0], host, ncache, readOnly);
	
	if(pipe(p) < 0)
		sysfatal("pipe failed: %r");
	if(!stdio){
		mfd[0] = p[0];
		mfd[1] = p[0];
		if(defmnt == 0){
			fd = create("#s/vacfs", OWRITE, 0666);
			if(fd < 0)
				sysfatal("create of /srv/vacfs failed: %r");
			sprint(buf, "%d", p[1]);
			if(write(fd, buf, strlen(buf)) < 0)
				sysfatal("writing /srv/vacfs: %r");
		}
	}

	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		vtAttach();
		close(p[1]);
		io();
		shutdown();
		break;
	default:
		close(p[0]);	/* don't deadlock if child fails */
		if(defmnt && mount(p[1], -1, defmnt, MREPL|MCREATE, "") < 0)
			sysfatal("mount failed: %r");
	}
	vtDetach();
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: %s [-sd] [-h host] [-c nchance] [-m mountpoint] vacfile\n", argv0);
	exits("usage");
}

char*
rversion(Fid *unused)
{
	Fid *f;

	USED(unused);

	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);

	if(rhdr.msize < 256)
		return "version: message size too small";
	messagesize = rhdr.msize;
	if(messagesize > sizeof mdata)
		messagesize = sizeof mdata;
	thdr.msize = messagesize;
	if(strncmp(rhdr.version, "9P2000", 6) != 0)
		return "unrecognized 9P version";
	thdr.version = "9P2000";
	return nil;
}

char*
rflush(Fid *f)
{
	USED(f);
	return 0;
}

char*
rauth(Fid *f)
{
	USED(f);
	return "vacfs: authentication not required";
}

char*
rattach(Fid *f)
{
	/* no authentication for the momment */
	VacFile *file;

	file = vacFileOpen(fs, "/");
	if(file == nil)
		return vtGetError();
	f->busy = 1;
	f->file = file;
	f->qid = (Qid){vacFileGetId(f->file), 0, QTDIR};
	thdr.qid = f->qid;
	if(rhdr.uname[0])
		f->user = vtStrDup(rhdr.uname);
	else
		f->user = "none";
	return 0;
}

char*
rwalk(Fid *f)
{
	VacFile *file, *nfile;
	Fid *nf;
	int nqid, nwname;
	Qid qid;

	if(f->busy == 0)
		return Enotexist;
	nf = nil;
	if(rhdr.fid != rhdr.newfid){
		if(f->open)
			return Eisopen;
		if(f->busy == 0)
			return Enotexist;
		nf = newfid(rhdr.newfid);
		if(nf->busy)
			return Eisopen;
		nf->busy = 1;
		nf->open = 0;
		nf->qid = f->qid;
		nf->file = vacFileIncRef(f->file);
		nf->user = vtStrDup(f->user);
		f = nf;
	}

	nwname = rhdr.nwname;

	/* easy case */
	if(nwname == 0) {
		thdr.nwqid = 0;
		return 0;
	}

	file = f->file;
	vacFileIncRef(file);
	qid = f->qid;

	for(nqid = 0; nqid < nwname; nqid++){
		if((qid.type & QTDIR) == 0){
			vtSetError(Enotdir);
			break;
		}
		if(!permf(file, f->user, Pexec)) {
			vtSetError(Eperm);
			break;
		}
		nfile = vacFileWalk(file, rhdr.wname[nqid]);
		if(nfile == nil)
			break;
		vacFileDecRef(file);
		file = nfile;
		qid.type = QTFILE;
		if(vacFileIsDir(file))
			qid.type = QTDIR;
		qid.vers = 0;
		qid.path = vacFileGetId(file);
		thdr.wqid[nqid] = qid;
	}

	thdr.nwqid = nqid;

	if(nqid == nwname){
		/* success */
		f->qid = thdr.wqid[nqid-1];
		vacFileDecRef(f->file);
		f->file = file;
		return 0;
	}

	vacFileDecRef(file);
	if(nf != nil)
		rclunk(nf);

	/* only error on the first element */
	if(nqid == 0)
		return vtGetError();

	return 0;
}

char *
ropen(Fid *f)
{
	int mode, trunc;

	if(f->open)
		return Eisopen;
	if(f->busy == 0)
		return Enotexist;
	mode = rhdr.mode;
	thdr.iounit = messagesize - IOHDRSZ;
	if(f->qid.type & QTDIR){
		if(mode != OREAD)
			return Eperm;
		if(!perm(f, Pread))
			return Eperm;
		thdr.qid = f->qid;
		f->db = nil;
		f->open = 1;
		return 0;
	}
	if(mode & ORCLOSE)
		return Erdonly;
	trunc = mode & OTRUNC;
	mode &= OPERM;
	if(mode==OWRITE || mode==ORDWR || trunc)
		if(!perm(f, Pwrite))
			return Eperm;
	if(mode==OREAD || mode==ORDWR)
		if(!perm(f, Pread))
			return Eperm;
	if(mode==OEXEC)
		if(!perm(f, Pexec))
			return Eperm;
	thdr.qid = f->qid;
	f->open = 1;
	return 0;
}

char *
rcreate(Fid *f)
{
	if(f->open)
		return Eisopen;
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

char*
rread(Fid *f)
{
	char *buf;
	vlong off;
	int cnt;
	VacFile *file;

	if(f->busy == 0)
		return Enotexist;
	file = f->file;
	thdr.count = 0;
	off = rhdr.offset;
	buf = thdr.data;
	cnt = rhdr.count;
	if(f->qid.type & QTDIR)
		thdr.count = vacdirread(f, buf, off, cnt);
	else
		thdr.count = vacFileRead(file, buf, cnt, off);
	if(thdr.count < 0)
		return vtGetError();
	return 0;
}

char*
rwrite(Fid *f)
{
	char *buf;
	vlong off;
	int cnt;
	VacFile *file;

	if(f->busy == 0)
		return Enotexist;
	file = f->file;
	thdr.count = 0;
	off = rhdr.offset;
	buf = rhdr.data;
	cnt = rhdr.count;
	if(f->qid.type & QTDIR)
		return "file is a directory";
	thdr.count = vacFileWrite(file, buf, cnt, off, nil);
	if(thdr.count < 0) {
fprint(2, "write failed: %s\n", vtGetError());
		return vtGetError();
	}
	return 0;
}

char *
rclunk(Fid *f)
{
	f->busy = 0;
	f->open = 0;
	vtMemFree(f->user);
	f->user = nil;
	vacFileDecRef(f->file);
	f->file = nil;
	dirBufFree(f->db);
	f->db = nil;
	return 0;
}

char *
rremove(Fid *f)
{
	rclunk(f);
	return Erdonly;
}

char *
rstat(Fid *f)
{
	VacDir dir;
	static uchar statbuf[1024];

	if(f->busy == 0)
		return Enotexist;
	vacFileGetDir(f->file, &dir);
	thdr.stat = statbuf;
	thdr.nstat = vacDirStat(&dir, thdr.stat, sizeof statbuf);
	vacDirCleanup(&dir);
	return 0;
}

char *
rwstat(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

int
vacDirStat(VacDir *vd, uchar *p, int np)
{
	Dir dir;

	memset(&dir, 0, sizeof(dir));
	/* fill in plan 9 dir structure */
	dir.name = vd->elem;
	dir.uid = vd->uid;
	dir.gid = vd->gid;
	dir.muid = vd->mid;
	dir.qid.path = vd->qid;
	dir.mode = vd->mode & 0777;
	dir.qid.type = QTFILE;
	if(vd->mode & ModeDir){
		dir.mode |= DMDIR;
		dir.qid.type = QTDIR;
	}
	dir.length = vd->size;
	dir.mtime = vd->mtime;
	dir.atime = vd->atime;
	return convD2M(&dir, p, np);
}

DirBuf*
dirBufAlloc(VacFile *vf)
{
	DirBuf *db;

	db = vtMemAllocZ(sizeof(DirBuf));
	db->vde = vacFileDirEnum(vf);
	return db;
}

VacDir *
dirBufGet(DirBuf *db)
{
	VacDir *vd;
	int n;

	if(db->eof)
		return nil;

	if(db->i >= db->n) {
		n = vacDirEnumRead(db->vde, db->buf, DirBufSize);
		if(n < 0)
			return nil;
		db->i = 0;
		db->n = n;
		if(n == 0) {
			db->eof = 1;
			return nil;
		}
	}

	vd = db->buf + db->i;
	db->i++;

	return vd;
}

int
dirBufUnget(DirBuf *db)
{
	assert(db->i > 0);
	db->i--;
	return 1;
}

void
dirBufFree(DirBuf *db)
{
	int i;

	if(db == nil)
		return;

	for(i=db->i; i<db->n; i++)
		vacDirCleanup(db->buf + i);
	vacDirEnumFree(db->vde);
	vtMemFree(db);
}

int
vacdirread(Fid *f, char *p, long off, long cnt)
{
	int n, nb;
	VacDir *vd;

	/*
	 * special case of rewinding a directory
	 * otherwise ignore the offset
	 */
	if(off == 0 && f->db) {
		dirBufFree(f->db);
		f->db = nil;
	}

	if(f->db == nil)
		f->db = dirBufAlloc(f->file);

	for(nb = 0; nb < cnt; nb += n) {
		vd = dirBufGet(f->db);
		if(vd == nil) {
			if(!f->db->eof)
				return -1;
			break;
		}
		n = vacDirStat(vd, (uchar*)p, cnt-nb);
		if(n <= BIT16SZ) {
			dirBufUnget(f->db);
			break;
		}
		vacDirCleanup(vd);
		p += n;
	}
	return nb;
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
	f = vtMemAllocZ(sizeof *f);
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
		n = read9pmsg(mfd[0], mdata, sizeof mdata);
		if(n == 0)
			continue;
		if(n < 0)
			break;
		if(convM2S(mdata, n, &rhdr) != n)
			sysfatal("convM2S conversion error");

		if(dflag)
			fprint(2, "vacfs:<-%F\n", &rhdr);

		thdr.data = (char*)mdata + IOHDRSZ;
		if(!fcalls[rhdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rhdr.type])(newfid(rhdr.fid));
		if(err){
			thdr.type = Rerror;
			thdr.ename = err;
		}else{
			thdr.type = rhdr.type + 1;
			thdr.fid = rhdr.fid;
		}
		thdr.tag = rhdr.tag;
		if(dflag)
			fprint(2, "vacfs:->%F\n", &thdr);
		n = convS2M(&thdr, mdata, messagesize);
		if(write(mfd[1], mdata, n) != n)
			sysfatal("mount write: %r");
	}
}

int
permf(VacFile *vf, char *user, int p)
{
	VacDir dir;
	ulong perm;

	vacFileGetDir(vf, &dir);
	perm = dir.mode & 0777;
	if((p*Pother) & perm)
		goto Good;
	if(strcmp(user, dir.gid)==0 && ((p*Pgroup) & perm))
		goto Good;
	if(strcmp(user, dir.uid)==0 && ((p*Powner) & perm))
		goto Good;
	vacDirCleanup(&dir);
	return 0;
Good:
	vacDirCleanup(&dir);
	return 1;
}

int
perm(Fid *f, int p)
{
	return permf(f->file, f->user, p);
}

void
init(char *file, char *host, long ncache, int readOnly)
{
	notify(notifyf);
	user = getuser();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	session = vtDial(host);
	if(session == nil)
		vtFatal("could not connect to server: %s", vtGetError());

	if(!vtConnect(session, 0))
		vtFatal("vtConnect: %s", vtGetError());

	fs = vacFSOpen(session, file, readOnly, ncache);
	if(fs == nil)
		vtFatal("vacFSOpen: %s", vtGetError());
}

void
shutdown(void)
{
	Fid *f;

	for(f = fids; f; f = f->next) {
		if(!f->busy)
			continue;
fprint(2, "open fid: %d\n", f->fid);
		rclunk(f);
	}

	vacFSClose(fs);
	vtClose(session);
}


static void
warn(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, arg);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(arg);
}

static void
debug(char *fmt, ...)
{
	va_list arg;
	
	if(!dflag)
		return;

	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	fflush(stderr);
	va_end(arg);
}

