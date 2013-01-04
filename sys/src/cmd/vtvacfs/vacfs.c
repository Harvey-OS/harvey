#include "/sys/src/cmd/vac/stdinc.h"
#include <fcall.h>
#include "/sys/src/cmd/vac/vac.h"

#define convM2Su(a, b, c, d) convM2S(a, b, c)
#define convS2Mu(a, b, c, d) convS2M(a, b, c)
#define convM2Du(a, b, c, d) convM2D(a, b, c)
#define convD2Mu(a, b, c, d) convD2M(a, b, c)

typedef struct Fid Fid;

enum
{
	Stacksize = 320 * 1024,	/* was 32K */
	OPERM	= 0x3		/* mask of all permission types in open mode */
};

struct Fid
{
	short busy;
	short open;
	int fid;
	char *user;
	Qid qid;
	VacFile *file;
	VacDirEnum *vde;
	Fid	*next;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64
};

static Fid	*fids;
static uchar	*data;
static int	mfd[2];
static int	srvfd = -1;
static char	*user;
static uchar	mdata[8192+IOHDRSZ];
static int messagesize = sizeof mdata;
static Fcall	rhdr;
static Fcall	thdr;
static VacFs	*fs;
static VtConn  *conn;
static int	noperm;
static int	dotu;
static char *defmnt;

static Fid *	newfid(int);
static void	error(char*);
static void	io(void);
static void	vacshutdown(void);
static int	perm(Fid*, int);
static int	permf(VacFile*, char*, int);
static ulong	getl(void *p);
static void	init(char*, char*, long, int);
static int	vacdirread(Fid *f, char *p, long off, long cnt);
static int	vacstat(VacFile *parent, VacDir *vd, uchar *p, int np);
static void 	srv(void* a);


static char	*rflush(Fid*), *rversion(Fid*),
	*rauth(Fid*), *rattach(Fid*), *rwalk(Fid*),
	*ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

static char 	*(*fcalls[Tmax])(Fid*);

static void
initfcalls(void)
{
	fcalls[Tflush]=	rflush;
	fcalls[Tversion]=	rversion;
	fcalls[Tattach]=	rattach;
	fcalls[Tauth]=		rauth;
	fcalls[Twalk]=		rwalk;
	fcalls[Topen]=		ropen;
	fcalls[Tcreate]=	rcreate;
	fcalls[Tread]=		rread;
	fcalls[Twrite]=	rwrite;
	fcalls[Tclunk]=	rclunk;
	fcalls[Tremove]=	rremove;
	fcalls[Tstat]=		rstat;
	fcalls[Twstat]=	rwstat;
}

static char	Eperm[] =	"permission denied";
static char	Enotdir[] =	"not a directory";
static char	Enotexist[] =	"file does not exist";
static char	Einuse[] =	"file in use";
static char	Eexist[] =	"file exists";
static char	Enotowner[] =	"not owner";
static char	Eisopen[] = 	"file already open for I/O";
static char	Excl[] = 	"exclusive use file already open";
static char	Ename[] = 	"illegal name";
static char	Erdonly[] = 	"read only file system";
static char	Eio[] = 	"i/o error";
static char	Eempty[] = 	"directory is not empty";
static char	Emode[] =	"illegal mode";

static int dflag;

static void
notifyf(void *a, char *s)
{
	USED(a);
	if(strncmp(s, "interrupt", 9) == 0)
		noted(NCONT);
	noted(NDFLT);
}

void
vacfs(void)
{
	char *defsrv, *srvname;
	int p[2], fd;
	
	defsrv = "mmventi";

	initfcalls();

	notify(notifyf);
	user = getuser();

	if(pipe(p) < 0)
		sysfatal("pipe failed: %r");
	mfd[0] = p[0];
	mfd[1] = p[0];
	srvfd = p[1];

	procrfork(srv, 0, Stacksize, RFFDG|RFNAMEG|RFNOTEG);

	close(p[0]);
	srvname = smprint("/srv/%s", defsrv);
	fd = create(srvname, OWRITE|ORCLOSE, 0666);
	if(fd < 0)
		sysfatal("create %s: %r", srvname);
	if(fprint(fd, "%d", srvfd) < 0)
		sysfatal("write %s: %r", srvname);
	free(srvname);

	threadexits(0);
}

static void
srv(void *a)
{
	USED(a);
	io();
	vacshutdown();
}

static char*
rversion(Fid *unused)
{
	Fid *f;

	USED(unused);

	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);

	if(rhdr.msize < 256)
		return vtstrdup("version: message size too small");
	messagesize = rhdr.msize;
	if(messagesize > sizeof mdata)
		messagesize = sizeof mdata;
	thdr.msize = messagesize;
	if(strncmp(rhdr.version, "9P2000", 6) != 0)
		return vtstrdup("unrecognized 9P version");
	thdr.version = "9P2000";
	if(strncmp(rhdr.version, "9P2000.u", 8) == 0){
		dotu = 1;
		thdr.version = "9P2000.u";
	}
	return nil;
}

static char*
rflush(Fid *f)
{
	USED(f);
	return 0;
}

static char*
rauth(Fid *f)
{
	USED(f);
	return vtstrdup("vacfs: authentication not required");
}

static char*
rattach(Fid *f)
{
	/* no authentication for the moment */
	/* since the score is in the aname :-) */
	VacFile *file;
	char err[80];
	char *score = rhdr.aname;
	fs = vacfsopen(nil, score, VtORDWR, 1024);
	if (fs == nil) {
		rerrstr(err, sizeof err);
		return vtstrdup(err);
	}

	file = vacfsgetroot(fs);
	if(file == nil) {
		rerrstr(err, sizeof err);
		return vtstrdup(err);
	}

	f->busy = 1;
	f->file = file;
	f->qid.path = vacfilegetid(f->file);
	f->qid.vers = 0;
	f->qid.type = QTDIR;
	thdr.qid = f->qid;
	if(rhdr.uname[0])
		f->user = vtstrdup(rhdr.uname);
	else
		f->user = "none";
	return 0;
}

static char*
rwalk(Fid *f)
{
	VacFile *file, *nfile;
	Fid *nf;
	int nqid, nwname;
	Qid qid;
	char *err = nil;

	if(f->busy == 0)
		return Enotexist;
	nf = nil;
	if(rhdr.fid != rhdr.newfid){
		if(f->open)
			return vtstrdup(Eisopen);
		if(f->busy == 0)
			return vtstrdup(Enotexist);
		nf = newfid(rhdr.newfid);
		if(nf->busy)
			return vtstrdup(Eisopen);
		nf->busy = 1;
		nf->open = 0;
		nf->qid = f->qid;
		nf->file = vacfileincref(f->file);
		nf->user = vtstrdup(f->user);
		f = nf;
	}

	nwname = rhdr.nwname;

	/* easy case */
	if(nwname == 0) {
		thdr.nwqid = 0;
		return 0;
	}

	file = f->file;
	vacfileincref(file);
	qid = f->qid;

	for(nqid = 0; nqid < nwname; nqid++){
		if((qid.type & QTDIR) == 0){
			err = Enotdir;
			break;
		}
		if(!permf(file, f->user, Pexec)) {
			err = Eperm;
			break;
		}
		nfile = vacfilewalk(file, rhdr.wname[nqid]);
		if(nfile == nil)
			break;
		vacfiledecref(file);
		file = nfile;
		qid.type = QTFILE;
		if(vacfileisdir(file))
			qid.type = QTDIR;
		qid.vers = vacfilegetmcount(file);
		qid.path = vacfilegetid(file);
		thdr.wqid[nqid] = qid;
	}

	thdr.nwqid = nqid;

	if(nqid == nwname){
		/* success */
		f->qid = thdr.wqid[nqid-1];
		vacfiledecref(f->file);
		f->file = file;
		return 0;
	}

	vacfiledecref(file);
	if(nf != nil)
		rclunk(nf);

	/* only error on the first element */
	if(nqid == 0)
		return vtstrdup(err);

	return 0;
}

static char *
ropen(Fid *f)
{
	int mode, trunc;

	if(f->open)
		return vtstrdup(Eisopen);
	if(!f->busy)
		return vtstrdup(Enotexist);

	mode = rhdr.mode;
	thdr.iounit = messagesize - IOHDRSZ;
	if(f->qid.type & QTDIR){
		if(mode != OREAD)
			return vtstrdup(Eperm);
		if(!perm(f, Pread))
			return vtstrdup(Eperm);
		thdr.qid = f->qid;
		f->vde = nil;
		f->open = 1;
		return 0;
	}
	if(mode & ORCLOSE)
		return vtstrdup(Erdonly);
	trunc = mode & OTRUNC;
	mode &= OPERM;
	if(mode==OWRITE || mode==ORDWR || trunc)
		if(!perm(f, Pwrite))
			return vtstrdup(Eperm);
	if(mode==OREAD || mode==ORDWR)
		if(!perm(f, Pread))
			return vtstrdup(Eperm);
	if(mode==OEXEC)
		if(!perm(f, Pexec))
			return vtstrdup(Eperm);
	thdr.qid = f->qid;
	thdr.iounit = messagesize - IOHDRSZ;
	f->open = 1;
	return 0;
}

static char*
rcreate(Fid* fid)
{
	VacFile *vf;
	ulong mode;

	if(fid->open)
		return vtstrdup(Eisopen);
	if(!fid->busy)
		return vtstrdup(Enotexist);
	if(fs->mode & ModeSnapshot)
		return vtstrdup(Erdonly);
	vf = fid->file;
	if(!vacfileisdir(vf))
		return vtstrdup(Enotdir);
	if(!permf(vf, fid->user, Pwrite)){
		return smprint("%s:'%s' not allowed", Eperm, fid->user);
	}

	mode = rhdr.perm & 0777;

	if(rhdr.perm & DMDIR){
		if((rhdr.mode & OTRUNC) || (rhdr.perm & DMAPPEND))
			return vtstrdup(Emode);
		switch(rhdr.mode & OPERM){
		default:
			return vtstrdup(Emode);
		case OEXEC:
		case OREAD:
			break;
		case OWRITE:
		case ORDWR:
			return vtstrdup(Eperm);
		}
		mode |= ModeDir;
	}
	vf = vacfilecreate(vf, rhdr.name, mode);
	if(vf == nil) {
		char err[80];
		rerrstr(err, sizeof err);

		return vtstrdup(err);
	}

	vacfiledecref(fid->file);

	fid->file = vf;
	fid->qid.type = QTFILE;
	if(vacfileisdir(vf))
		fid->qid.type = QTDIR;
	fid->qid.vers = vacfilegetmcount(vf);
	fid->qid.path = vacfilegetid(vf);

	thdr.qid = fid->qid;
	thdr.iounit = messagesize - IOHDRSZ;

	return 0;
}

static char*
rread(Fid *f)
{
	char *buf;
	vlong off;
	int cnt;
	VacFile *vf;
	char err[80];
	int n;

	if(!f->busy)
		return vtstrdup(Enotexist);
	vf = f->file;
	thdr.count = 0;
	off = rhdr.offset;
	buf = thdr.data;
	cnt = rhdr.count;
	if(f->qid.type & QTDIR)
		n = vacdirread(f, buf, off, cnt);
	else if(vacfilegetmode(f->file)&ModeDevice)
		return vtstrdup("device");
	else if(vacfilegetmode(f->file)&ModeLink)
		return vtstrdup("symbolic link");
	else if(vacfilegetmode(f->file)&ModeNamedPipe)
		return vtstrdup("named pipe");
	else
		n = vacfileread(vf, buf, cnt, off);
	if(n < 0) {
		rerrstr(err, sizeof err);
		return vtstrdup(err);
	}
	thdr.count = n;
	return 0;
}

static char*
rwrite(Fid *f)
{
	char *buf;
	vlong off;
	int cnt;
	VacFile *vf;
	char err[80];
	int n;
	extern int debug;

		fprint(2, "rwrite busy %d\n", f->busy);
	if(!f->busy)
		return vtstrdup(Enotexist);
	vf = f->file;
	thdr.count = 0;
	off = rhdr.offset;
	buf = thdr.data;
	cnt = rhdr.count;
	n = vacfilewrite(vf, buf, cnt, off);
	if(n < 0) {
		rerrstr(err, sizeof err);
		return vtstrdup(err);
	}
	thdr.count = n;
	return 0;
}

static char *
rclunk(Fid *f)
{
	f->busy = 0;
	f->open = 0;
	vtfree(f->user);
	f->user = nil;
	if(f->file)
		vacfiledecref(f->file);
	f->file = nil;
	vdeclose(f->vde);
	f->vde = nil;
	return 0;
}

static char *
rremove(Fid *f)
{
	VacFile *vf, *vfp;
	char errbuf[80];
	char *err = nil;

	if(!f->busy)
		return vtstrdup(Enotexist);
	vf = f->file;
	vfp = vacfilegetparent(vf);

	if(!permf(vfp, f->user, Pwrite)) {
		err = Eperm;
		goto Exit;
	}

	if(!vacfileremove(vf)) {
		rerrstr(errbuf, sizeof errbuf);
		err = errbuf;
	}

Exit:
	vacfiledecref(vfp);
	rclunk(f);
	return vtstrdup(err);
}

static char *
rstat(Fid *f)
{
	VacDir dir;
	static uchar statbuf[1024];
	VacFile *parent;
	
	if(!f->busy)
		return vtstrdup(Enotexist);
	parent = vacfilegetparent(f->file);
	vacfilegetdir(f->file, &dir);
	thdr.stat = statbuf;
	thdr.nstat = vacstat(parent, &dir, thdr.stat, sizeof statbuf);
	vdcleanup(&dir);
	vacfiledecref(parent);
	return 0;
}

static char *
rwstat(Fid *f)
{
	if(!f->busy)
		return vtstrdup(Enotexist);
	return vtstrdup(Erdonly);
}

static int
vacstat(VacFile *parent, VacDir *vd, uchar *p, int np)
{
	int ret;
	Dir dir;

	memset(&dir, 0, sizeof(dir));

	dir.qid.path = vd->qid + vacfilegetqidoffset(parent);
	if(vd->qidspace)
		dir.qid.path += vd->qidoffset;
	dir.qid.vers = vd->mcount;
	dir.mode = vd->mode & 0777;
	if(vd->mode & ModeAppend){
		dir.qid.type |= QTAPPEND;
		dir.mode |= DMAPPEND;
	}
	if(vd->mode & ModeExclusive){
		dir.qid.type |= QTEXCL;
		dir.mode |= DMEXCL;
	}
	if(vd->mode & ModeDir){
		dir.qid.type |= QTDIR;
		dir.mode |= DMDIR;
	}
	
	
	dir.atime = vd->atime;
	dir.mtime = vd->mtime;
	dir.length = vd->size;

	dir.name = vd->elem;
	dir.uid = vd->uid;
	dir.gid = vd->gid;
	dir.muid = vd->mid;

	ret = convD2Mu(&dir, p, np, dotu);
	return ret;
}

static int
vacdirread(Fid *f, char *p, long off, long cnt)
{
	int i, n, nb;
	VacDir vd;

	/*
	 * special case of rewinding a directory
	 * otherwise ignore the offset
	 */
	if(off == 0 && f->vde){
		vdeclose(f->vde);
		f->vde = nil;
	}

	if(f->vde == nil){
		f->vde = vdeopen(f->file);
		if(f->vde == nil)
			return -1;
	}

	for(nb = 0; nb < cnt; nb += n) {
		i = vderead(f->vde, &vd);
		if(i < 0)
			return -1;
		if(i == 0)
			break;
		n = vacstat(f->file, &vd, (uchar*)p, cnt-nb);
		if(n <= BIT16SZ) {
			vdeunread(f->vde);
			break;
		}
		vdcleanup(&vd);
		p += n;
	}
	return nb;
}

static Fid *
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
	f = vtmallocz(sizeof *f);
	f->fid = fid;
	f->next = fids;
	fids = f;
	return f;
}

static void
io(void)
{
	char *err;
	int n;
	dflag=2;
	fmtinstall('Q', fcallfmt);

	for(;;){
		n = read9pmsg(mfd[0], mdata, sizeof mdata);
		if(n <= 0)
			break;
		if(convM2Su(mdata, n, &rhdr, dotu) != n)
			sysfatal("convM2S conversion error");

		if(dflag)
			fprint(2, "vacfs:<-%Q\n", &rhdr);

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
			fprint(2, "vacfs:->%Q\n", &thdr);
		n = convS2Mu(&thdr, mdata, messagesize, dotu);
		if(n <= BIT16SZ)
			sysfatal("convS2Mu conversion error");
		if(err)
			vtfree(err);

		if(write(mfd[1], mdata, n) != n)
			sysfatal("mount write: %r");
	}
}

static int
permf(VacFile *vf, char *user, int p)
{
	VacDir dir;
	ulong perm;

	if(vacfilegetdir(vf, &dir))
		return 0;
	perm = dir.mode & 0777;

	if(noperm)
		goto Good;
	if((p*Pother) & perm)
		goto Good;
	if(strcmp(user, dir.gid)==0 && ((p*Pgroup) & perm))
		goto Good;
	if(strcmp(user, dir.uid)==0 && ((p*Powner) & perm))
		goto Good;
	vdcleanup(&dir);
	return 0;
Good:
	vdcleanup(&dir);
	return 1;
}

int
static perm(Fid *f, int p)
{
	return permf(f->file, f->user, p);
}

static void
vacshutdown(void)
{
	Fid *f;

	for(f = fids; f; f = f->next) {
		if(!f->busy)
			continue;
		rclunk(f);
	}

	vacfsclose(fs);
	vthangup(conn);
}

