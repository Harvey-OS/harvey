#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "sac.h"
#include "sacfs.h"

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
	OffsetSize = 4,		/* size in bytes of an offset */
	CacheSize = 20,
};

typedef struct Fid Fid;
typedef struct Path Path;
typedef struct Sac Sac;
typedef struct Cache Cache;

struct Fid
{
	short busy;
	short open;
	int fid;
	char *user;
	Qid qid;
	Sac *sac;
	Fid	*next;
};

struct Sac
{
	SacDir;
	Path *path;
};

struct Path
{
	int ref;
	Path *up;
	long blocks;
	int entry;
	int nentry;
};

struct Cache
{
	long block;
	ulong age;
	uchar *data;
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
uchar *data;
int	mfd[2];
char	user[NAMELEN];
char	mdata[MAXMSG+MAXFDATA];
Fcall	rhdr;
Fcall	thdr;
int blocksize;
Sac root;
Cache cache[CacheSize];
ulong cacheage;

Fid *	newfid(int);
void	sacstat(SacDir*, char*);
void	error(char*);
void	io(void);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
void	usage(void);
int	perm(Fid*, Sac*, int);
ulong	getl(void *p);
void	init(char*);
Sac	*saccpy(Sac *s);
Sac *saclookup(Sac *s, char *name);
int sacdirread(Sac *s, char *p, long off, long cnt);
void loadblock(void *buf, uchar *offset, int blocksize);
void sacfree(Sac*);

char	*rflush(Fid*), *rnop(Fid*), *rsession(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

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
main(int argc, char *argv[])
{
	char *defmnt;
	int p[2];
	char buf[12];
	int fd;
	int stdio = 0;

	defmnt = "/n/c:";
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
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

	if(argc != 1)
		usage();

	init(argv[0]);
	
	if(pipe(p) < 0)
		error("pipe failed");
	if(!stdio){
		mfd[0] = p[0];
		mfd[1] = p[0];
		if(defmnt == 0){
			fd = create("#s/sacfs", OWRITE, 0666);
			if(fd < 0)
				error("create of /srv/sacfs failed");
			sprint(buf, "%d", p[1]);
			if(write(fd, buf, strlen(buf)) < 0)
				error("writing /srv/sacfs");
		}
	}

	if(debug)
		fmtinstall('F', fcallconv);
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		error("fork");
	case 0:
		close(p[1]);
		io();
		break;
	default:
		close(p[0]);	/* don't deadlock if child fails */
		if(defmnt && mount(p[1], defmnt, MREPL|MCREATE, "") < 0)
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
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	return 0;
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
	f->qid = (Qid){getl(root.qid), 0};
	f->sac = saccpy(&root);
	thdr.qid = f->qid;
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
	if(f->busy == 0)
		return Enotexist;
	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	nf->qid = f->qid;
	nf->sac = saccpy(f->sac);
	nf->user = strdup(f->user);
	return 0;
}

char*
rwalk(Fid *f)
{
	char *name;
	Sac *sac;

	if((f->qid.path & CHDIR) == 0)
		return Enotdir;
	if(f->busy == 0)
		return Enotexist;
	name = rhdr.name;
	sac = f->sac;
	if(strcmp(name, ".") == 0){
		thdr.qid = f->qid;
		return 0;
	}
	if(!perm(f, sac, Pexec))
		return Eperm;
	sac = saclookup(sac, name);
	if(sac == nil)
		return Enotexist;
	f->sac = sac;
	f->qid = (Qid){getl(sac->qid), 0};
	thdr.qid = f->qid;
	return 0;
}

char *
rclwalk(Fid *f)
{
	Fid *nf;
	char *err;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->sac = saccpy(f->sac);
	nf->user = strdup(f->user);
	if(err = rwalk(nf))
		rclunk(nf);
	return err;
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
	if(f->qid.path & CHDIR){
		if(mode != OREAD)
			return Eperm;
		thdr.qid = f->qid;
		return 0;
	}
	if(mode & ORCLOSE)
		return Erdonly;
	trunc = mode & OTRUNC;
	mode &= OPERM;
	if(mode==OWRITE || mode==ORDWR || trunc)
		return Erdonly;
	if(mode==OREAD)
		if(!perm(f, f->sac, Pread))
			return Eperm;
	if(mode==OEXEC)
		if(!perm(f, f->sac, Pexec))
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
	Sac *sac;
	char *buf, *buf2;
	long off;
	int n, cnt, i, j;
	uchar *blocks;
	long length;

	if(f->busy == 0)
		return Enotexist;
	sac = f->sac;
	thdr.count = 0;
	off = rhdr.offset;
	buf = thdr.data;
	cnt = rhdr.count;
	if(f->qid.path & CHDIR){
		cnt = (rhdr.count/DIRLEN)*DIRLEN;
		if(off%DIRLEN)
			return "i/o error";
		thdr.count = sacdirread(sac, buf, off, cnt);
		return 0;
	}
	length = getl(sac->length);
	if(off >= length) {
		rhdr.count = 0;
		return 0;
	}
	if(cnt > length-off)
		cnt = length-off;
	thdr.count = cnt;
	if(cnt == 0)
		return 0;
	blocks = data + getl(sac->blocks);
	buf2 = malloc(blocksize);
	if(buf2 == nil)
		sysfatal("malloc failed");
	while(cnt > 0) {
		i = off/blocksize;
		n = blocksize;
		if(n > length-i*blocksize)
			n = length-i*blocksize;
		loadblock(buf2, blocks+i*OffsetSize, n);
		j = off-i*blocksize;
		n = blocksize-j;
		if(n > cnt)
			n = cnt;
		memmove(buf, buf2+j, n);
		cnt -= n;
		off += n;
		buf += n;
	}
	free(buf2);
	return 0;
}

char*
rwrite(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

char *
rclunk(Fid *f)
{
	f->busy = 0;
	f->open = 0;
	free(f->user);
	sacfree(f->sac);
	return 0;
}

char *
rremove(Fid *f)
{
	f->busy = 0;
	f->open = 0;
	free(f->user);
	sacfree(f->sac);
	return Erdonly;
}

char *
rstat(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	sacstat(f->sac, thdr.stat);
	return 0;
}

char *
rwstat(Fid *f)
{
	if(f->busy == 0)
		return Enotexist;
	return Erdonly;
}

Sac*
saccpy(Sac *s)
{
	Sac *ss;
	
	ss = emalloc(sizeof(Sac));
	*ss = *s;
	if(ss->path)
		ss->path->ref++;
	return ss;
}

Path *
pathalloc(Path *p, long blocks, int entry, int nentry)
{
	Path *pp = emalloc(sizeof(Path));

	pp->ref = 1;
	pp->blocks = blocks;
	pp->entry = entry;
	pp->nentry = nentry;
	pp->up = p;
	return pp;
}

void
pathfree(Path *p)
{
	if(p == nil)
		return;
	p->ref--;
	if(p->ref > 0)
		return;

	pathfree(p->up);
	free(p);
}


void
sacfree(Sac *s)
{
	pathfree(s->path);
	free(s);
}

void
sacstat(SacDir *s, char *buf)
{
	Dir dir;

	memmove(dir.name, s->name, NAMELEN);
	dir.qid = (Qid){getl(s->qid), 0};
	dir.mode = getl(s->mode);
	dir.length = getl(s->length);
	if(dir.mode &CHDIR)
		dir.length *= DIRLEN;
	memmove(dir.uid, s->uid, NAMELEN);
	memmove(dir.gid, s->gid, NAMELEN);
	dir.atime = getl(s->atime);
	dir.mtime = getl(s->mtime);
	convD2M(&dir, buf);
}

void
loadblock(void *buf, uchar *offset, int blocksize)
{
	long block, n;
	ulong age;
	int i, j;

	block = getl(offset);
	if(block < 0) {
		block = -block;
		cacheage++;
		// age has wraped
		if(cacheage == 0) {
			for(i=0; i<CacheSize; i++)
				cache[i].age = 0;
		}
		j = 0;
		age = cache[0].age;
		for(i=0; i<CacheSize; i++) {
			if(cache[i].age < age) {
				age = cache[i].age;
				j = i;
			}
			if(cache[i].block != block)
				continue;
			memmove(buf, cache[i].data, blocksize);
			cache[i].age = cacheage;
			return;
		}

		n = getl(offset+OffsetSize);
		if(n < 0)
			n = -n;
		n -= block;
		if(unsac(buf, data+block, blocksize, n)<0)
			sysfatal("unsac failed!");
		memmove(cache[j].data, buf, blocksize);
		cache[j].age = cacheage;
		cache[j].block = block;
	} else {
		memmove(buf, data+block, blocksize);
	}
}

Sac*
sacparent(Sac *s)
{
	uchar *blocks;
	SacDir *buf;
	int per, i, n;
	Path *p;

	p = s->path;
	if(p == nil || p->up == nil) {
		pathfree(p);
		*s = root;
		return s;
	}
	p = p->up;

	blocks = data + p->blocks;
	per = blocksize/sizeof(SacDir);
	i = p->entry/per;
	n = per;
	if(n > p->nentry-i*per)
		n = p->nentry-i*per;
	buf = emalloc(per*sizeof(SacDir));
	loadblock(buf, blocks + i*OffsetSize, n*sizeof(SacDir));
	s->SacDir = buf[p->entry-i*per];
	free(buf);
	p->ref++;
	pathfree(s->path);
	s->path = p;
	return s;
}

int
sacdirread(Sac *s, char *p, long off, long cnt)
{
	uchar *blocks;
	SacDir *buf;
	int iblock, per, i, j, ndir, n;

	blocks = data + getl(s->blocks);
	per = blocksize/sizeof(SacDir);
	ndir = getl(s->length);
	off /= DIRLEN;
	cnt /= DIRLEN;
	if(off >= ndir)
		return 0;
	if(cnt > ndir-off)
		cnt = ndir-off;
	iblock = -1;
	buf = emalloc(per*sizeof(SacDir));
	for(i=off; i<off+cnt; i++) {
		j = i/per;
		if(j != iblock) {
			n = per;
			if(n > ndir-j*per)
				n = ndir-j*per;
			loadblock(buf, blocks + j*OffsetSize, n*sizeof(SacDir));
			iblock = j;
		}
		sacstat(buf+i-j*per, p);
		p += DIRLEN;
	}
	free(buf);
	return cnt*DIRLEN;
}

Sac*
saclookup(Sac *s, char *name)
{
	int ndir;
	int top, bot, i, j, k, n, per;
	uchar *blocks;
	SacDir *buf;
	int iblock;
	SacDir *sd;
	
	if(strcmp(name, "..") == 0)
		return sacparent(s);
	blocks = data + getl(s->blocks);
	per = blocksize/sizeof(SacDir);
	ndir = getl(s->length);
	buf = malloc(per*sizeof(SacDir));
	if(buf == nil)
		sysfatal("malloc failed");
	iblock = -1;

	if(1) {
		// linear search
		for(i=0; i<ndir; i++) {
			j = i/per;
			if(j != iblock) {
				n = per;
				if(n > ndir-j*per)
					n = ndir-j*per;
				loadblock(buf, blocks + j*OffsetSize, n*sizeof(SacDir));
				iblock = j;
			}
			sd = buf+i-j*per;
			k = strcmp(name, sd->name);
			if(k == 0) {
				s->path = pathalloc(s->path, getl(s->blocks), i, ndir);
				s->SacDir = *sd;
				free(buf);
				return s;
			}
		}
		free(buf);
		return 0;
	}

	// binary search
	top = ndir;
	bot = 0;
	while(bot != top){
		i = (bot+top)>>1;
		j = i/per;
		if(j != iblock) {
			n = per;
			if(n > ndir-j*per)
				n = ndir-j*per;
			loadblock(buf, blocks + j*OffsetSize, n*sizeof(SacDir));
			iblock = j;
		}
		j *= per;
		sd = buf+i-j;
		k = strcmp(name, sd->name);
		if(k == 0) {
			s->path = pathalloc(s->path, getl(s->blocks), i, ndir);
			s->SacDir = *sd;
			free(buf);
		}
		if(k < 0) {
			top = i;
			sd = buf;
			if(strcmp(name, sd->name) < 0)
				top = j;
		} else {
			bot = i+1;
			if(ndir-j < per)
				i = ndir-j;
			else
				i = per;
			sd = buf+i-1;
			if(strcmp(name, sd->name) > 0)
				bot = j+i;
		}
	}
	return 0;
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
	memset(f, 0 , sizeof(Fid));
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

		if(debug)
			fprint(2, "sacfs:<-%F\n", &rhdr);

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
		if(debug)
			fprint(2, "ramfs:->%F\n", &thdr);/**/
		n = convS2M(&thdr, mdata);
		if(write(mfd[1], mdata, n) != n)
			error("mount write");
	}
}

int
perm(Fid *f, Sac *s, int p)
{
	ulong perm = getl(s->mode);
	if((p*Pother) & perm)
		return 1;
	if(strcmp(f->user, s->gid)==0 && ((p*Pgroup) & perm))
		return 1;
	if(strcmp(f->user, s->uid)==0 && ((p*Powner) & perm))
		return 1;
	return 0;
}

void
init(char *file)
{
	SacHeader *hdr;
	Dir dir;
	int fd;
	int i;
	uchar *p;

	notify(notifyf);
	strcpy(user, getuser());


	if(dirstat(file, &dir) < 0)
		error("bad file");
	data = emalloc(dir.length);
	fd = open(file, OREAD);
	if(fd < 0)
		error("opening file");
	if(read(fd, data, dir.length) < dir.length)
		error("reading file");
	hdr = (SacHeader*)data;
	if(getl(hdr->magic) != Magic)
		error("bad magic");
	if(getl(hdr->length) != (ulong)(dir.length))
		error("bad length");
	blocksize = getl(hdr->blocksize);
	root.SacDir = *(SacDir*)(data + sizeof(SacHeader));
	p = malloc(CacheSize*blocksize);
	if(p == nil)
		error("allocating cache");
	for(i=0; i<CacheSize; i++) {
		cache[i].data = p;
		p += blocksize;
	}
}

void
error(char *s)
{
	fprint(2, "%s: %s: %r\n", argv0, s);
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
	fprint(2, "usage: %s [-i infd outfd] [-s] [-m mountpoint] sacfsfile\n", argv0);
	exits("usage");
}

ulong
getl(void *p)
{
	uchar *a = p;

	return (a[0]<<24) | (a[1]<<16) | (a[2]<<8) | a[3];
}

