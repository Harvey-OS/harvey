#include "u.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "libc.h"
#include "9p.h"
#include "stdio.h"
#include "setjmp.h"
#include "pwd.h"
#include "grp.h"

#define	DBG(f)

struct group *getgrent(void);
struct passwd *getpwent(void);

#ifdef SYSV
#	include <dirent.h>
#	define	DTYPE	struct dirent
#	define SOCKETS
#endif
#ifdef V10
#	include <ndir.h>
#	define	DTYPE	struct direct
#endif
#ifdef BSD
#	include <sys/dir.h>
#	define	DTYPE	struct direct
#	define	SOCKETS
#endif
#ifdef SOCKETS
#	define sendmsg	__sendmsg
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	undef sendmsg
	char	bsdhost[256];
	void	remotehostname(void);
	char	ipaddr[20];
#endif

typedef struct File	File;
typedef struct Rfile	Rfile;
typedef struct Fd	Fd;
typedef struct Pass	Pass;

struct Fd{
	int		ref;
	Ulong		offset;
	int		fd;
	DIR		*dir;
};

struct Rfile{
	int		busy;
	int		uid;
	int		rclose;
	File		*file;
	Fd		*fd;
};

struct File{
	int		ref;
	char		*path;
	char		*name;
	Qid		qid;
	struct stat	stbuf;
};

struct Pass{
	int		id;
	int		gid;
	char		*name;
	Pass		*next;
	Pass		*nnext;
	int		nmem;
	gid_t		*mem;
};

char	data[2][MAXMSG+MAXFDATA];
char	tdata[MAXMSG+MAXFDATA];
char	rdata[MAXFDATA];
Fcall	rhdr;
Fcall	thdr;
Rfile	*rfile;
File	*file0;
int	nrfilealloc;
jmp_buf	loopjmp;
Pass*	uid[256];
Pass*	gid[256];
Pass*	uidname[256];
Pass*	gidname[256];
int	curuid;
int	devallowed;
int	connected;
int	readonly;
int	myuid;
char	*onlyuser;

void	io(void);
void	error(char*);
char	*mfmt(Fcall*);
void	sendmsg(const char*);
int	okfid(int);
Rfile*	rfilefid(void);
File*	newfile(void);
void*	erealloc(void*, unsigned);
char*	estrdup(char*);
const char*	dostat(File*, char*);
char*	bldpath(char*, char*, char*);
Ulong	qid(struct stat*);
Ulong	vers(struct stat*);
void	errjmp(const char*);
int	omode(int);
char*	id2name(Pass**, int);
Pass*	id2pass(Pass**, int);
Pass*	name2pass(Pass**, char*);
void	getpwdf(void);
void	getgrpf(void);
int	hash(char*);
void	setupuser(int);

void	rsession(void);
void	rattach(void);
void	rflush(void);
void	rclone(void);
void	rwalk(void);
void	ropen(void);
void	rcreate(void);
void	rread(void);
void	rwrite(void);
void	rclunk(int);
void	rstat(void);
void	rwstat(void);
void	rclwalk(void);

char	Eauth[] =	"authentication failed";
char	Eperm[] =	"permission denied";
char	Ebadfid[] =	"fid unknown or out of range";
char	Efidactive[] =	"fid already in use";
char	Eopen[] =	"file is open";
char	Emode[] =	"invalid open mode";
char	Especial[] =	"no access to special file";
char	Especial0[] =	"already attached without access to special files";
char	Especial1[] =	"already attached with access to special files";
char	Enotopen[] =	"file is not open";
char	Etoolarge[] =	"i/o count too large";
char	Ebaddir[] =	"i/o error on directory";
char	Eunknown[] =	"unknown user or group";
char	Euid[] =	"can't set uid";
char	Egid[] =	"can't set gid";
char	Eowner[] =	"not owner";

void
usage(void)
{
	fprintf(stderr, "u9fs [-r] [-u user uid]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *s;
	int onlyuid;

#	ifdef LOG
	freopen(LOG, "a", stderr);
#	endif

	setbuf(stderr, (void*)0);
	DBG(fprintf(stderr, "u9fs\nkill %d\n", getpid()));

	onlyuid = -1;
	ARGBEGIN{
	case 'r':
		readonly++;
		break;
	case 'u':
		onlyuser = ARGF();
		if(onlyuser == 0)
			usage();
		s = ARGF();
		if(s == 0)
			usage();
		onlyuid = atoi(s);
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc)
		usage();

#	ifdef SOCKETS
	if(!onlyuser)
		remotehostname();
#	endif

	setreuid(0, 0);
	myuid = geteuid();
	if(onlyuser && myuid != onlyuid)
		error("invalid uid");

	io();
	return 0;
}

void
io(void)
{
	int m;
	static int toggle, ndata;
	char *datap;

	/*
	 * TCP does not preserve record boundaries; this dance works around
	 * the problem.
	 */
	setjmp(loopjmp);

	/*
	 * Invariant: data[toggle] has ndata bytes already
	 */
    loop:
	datap = data[toggle];
	toggle ^= 1;
	for(;;){
		if(ndata){
			m = convM2S(datap, &rhdr, ndata);
			/* m is number of bytes more than a full message */
			if(m >= 0){
				memmove(data[toggle], datap+(ndata-m), m);
				ndata = m;
				break;
			}
		}
		m = read(0, datap+ndata, (MAXMSG+MAXFDATA)-ndata);
		if(m == 0){
			fprintf(stderr, "u9fs: eof\n");
			exit(1);
		}
		if(m < 0)
			error("read");
		ndata += m;
	}

	thdr.type = rhdr.type+1;
	thdr.tag = rhdr.tag;
	thdr.fid = rhdr.fid;
	DBG(fprintf(stderr, ">> %s\n", mfmt(&rhdr)));
	switch(rhdr.type){
	case Tnop:
	case Tflush:	/* this is a synchronous fs; easy */
		break;
	case Tsession:
		rsession();
		break;
	case Tattach:
		rattach();
		break;
	case Tclone:
		rclone();
		break;
	case Twalk:
		rwalk();
		break;
	case Tstat:
		rstat();
		break;
	case Twstat:
		rwstat();
		break;
	case Topen:
		ropen();
		break;
	case Tcreate:
		rcreate();
		break;
	case Tread:
		rread();
		break;
	case Twrite:
		rwrite();
		break;
	case Tclunk:
		rclunk(0);
		break;
	case Tremove:
		rclunk(1);
		break;
	default:
		fprintf(stderr, "unknown message %s\n", mfmt(&rhdr));
		error("bad message");
	}
	sendmsg(0);
	goto loop;
}

void
rsession(void)
{
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
}

void
rattach(void)
{
	Rfile *rf;
	Pass *p;

	if(file0 == 0){
		file0 = newfile();
		file0->ref++;		/* one extra to hold it up */	
		file0->path = estrdup("/");
		file0->name = estrdup("/");
		errjmp(dostat(file0, 0));
	}
	if(!okfid(rhdr.fid))
		errjmp(Ebadfid);
	if(strncmp(rhdr.aname, "device", 6) == 0){
		if(connected && !devallowed)
			errjmp(Especial0);
		devallowed = 1;
	}else{
		if(connected && devallowed)
			errjmp(Especial1);
	}
	getpwdf();
	getgrpf();
	rf = &rfile[rhdr.fid];
	if(rf->busy)
		errjmp(Efidactive);
	p = name2pass(uidname, rhdr.uname);
	if(p == 0)
		errjmp(Eunknown);
	if(p->id == 0 || (uid_t)p->id == (uid_t)-1
	|| myuid && p->id != myuid)
		errjmp(Eperm);
#	ifdef SOCKETS
	if(!myuid && ruserok(bsdhost, 0, rhdr.uname, rhdr.uname) < 0)
		errjmp(Eperm);
#	endif
	/* mark busy & inc ref cnt only after committed to succeed */
	rf->busy = 1;
	rf->file = file0;
	file0->ref++;
	rf->rclose = 0;
	rf->uid = p->id;
	thdr.qid = file0->qid;
	connected = 1;
}

void
rclone(void)
{
	Rfile *rf, *nrf;
	File *f;

	rfilefid();
	if(!okfid(rhdr.newfid))
		errjmp(Ebadfid);
	rf = &rfile[rhdr.fid];
	nrf = &rfile[rhdr.newfid];
	f = rf->file;
	if(nrf->busy)
		errjmp(Efidactive);
	nrf->busy = 1;
	nrf->file = f;
	f->ref++;
	nrf->fd = rf->fd;
	nrf->uid = rf->uid;
	nrf->rclose = rf->rclose;
	if(nrf->fd){
		if(nrf->fd->ref == 0)
			error("clone fd count");
		nrf->fd->ref++;
	}
}

void
rwalk(void)
{
	const char *err;
	Rfile *rf;
	File *of, *f;

	rf = rfilefid();
	if(rf->fd)
		errjmp(Eopen);
	of = rf->file;
	f = newfile();
	f->path = estrdup(of->path);
	err = dostat(f, rhdr.name);
	if(err){
		free(f->path);
		free(f->name);
		free(f);
		errjmp(err);
	}
	if(of->ref <= 0)
		error("walk ref count");
	if(--of->ref == 0){
		free(of->path);
		free(of->name);
		free(of);
	}
	rf->file = f;
	thdr.qid = f->qid;
}

void
ropen(void)
{
	Rfile *rf;
	File *f;
	int fd;
	DIR *dir;
	int m, trunc;

	rf = rfilefid();
	f = rf->file;
	if(rf->fd)
		error("open already open");
	if(!devallowed && (f->stbuf.st_mode & S_IFCHR))
		errjmp(Especial);
	m = rhdr.mode & (16|3);
	trunc = m & 16;	/* OTRUNC */
	switch(m){
	case 0:
	case 1:
	case 1|16:
	case 2:	
	case 0|16:
	case 2|16:
	case 3:
		break;
	default:
		errjmp(Emode);
	}
	
	m = omode(m & 3);
	errno = 0;
	if(f->qid.path & CHDIR){
		if(rhdr.mode != 0)		/* OREAD */
			errjmp(Eperm);
		dir = opendir(f->path);
		if(dir == 0)
			errjmp(sys_errlist[errno]);
		fd = 0;
	}else{
		if(trunc){
			if(readonly) errjmp("trunc disallowed");
			fd = creat(f->path, 0666);
			if(fd >= 0)
				if(m != 1){
					close(fd);
					fd = open(f->path, m);
				}
		}else
			fd = open(f->path, m);
		if(fd < 0)
			errjmp(sys_errlist[errno]);
		dir = 0;
	}
	rf->rclose = rhdr.mode & 64;	/* ORCLOSE */
	rf->fd = erealloc(0, sizeof(Fd));
	rf->fd->ref = 1;
	rf->fd->fd = fd;
	rf->fd->dir = dir;
	rf->fd->offset = 0;
	thdr.qid = f->qid;
}

void
rcreate(void)
{
	Rfile *rf;
	File *f, *of;
	char *path;
	const char *err;
	int fd, m;
	char name[NAMELEN];

	if(readonly) errjmp("write disallowed");
	rf = rfilefid();
	if(rf->fd)
		errjmp(Eopen);
	path = bldpath(rf->file->path, rhdr.name, name);
	m = omode(rhdr.mode&3);
	errno = 0;
	/* plan 9 group sematics: always inherit from directory */
	if(rhdr.perm & CHDIR){
		if(m){
			free(path);
			errjmp(Eperm);
		}
		fd = mkdir(path, 0777);
		if(fd < 0){
			free(path);
			errjmp(sys_errlist[errno]);
		}
		fd = open(path, 0);
		free(path);
		if(fd >= 0)
			fchmod(fd, S_ISGID|(rhdr.perm&rf->file->stbuf.st_mode&0777));
	}else{
		fd = creat(path, 0666);
		if(fd >= 0){
			if(m != 1){
				close(fd);
				fd = open(path, m);
			}
			fchmod(fd, rhdr.perm&rf->file->stbuf.st_mode&0777);
		}
		free(path);
		if(fd < 0)
			errjmp(sys_errlist[errno]);
	}
	f = newfile();
	of = rf->file;	
	f->path = estrdup(of->path);
	err = dostat(f, rhdr.name);
	if(err){
		free(f->path);
		free(f->name);
		free(f);
		errjmp(err);
	}
	if(!devallowed && (f->stbuf.st_mode & S_IFCHR)){
		free(f->path);
		free(f->name);
		free(f);
		errjmp(Especial);
	}
	if(--of->ref == 0){
		free(of->path);
		free(of->name);
		free(of);
	}
	rf->file = f;
	rf->rclose = rhdr.mode & 64;	/* ORCLOSE */
	rf->fd = erealloc(0, sizeof(Fd));
	rf->fd->ref = 1;
	rf->fd->fd = fd;
	rf->fd->dir = 0;
	rf->fd->offset = 0;
	thdr.qid = f->qid;
}

void
rread(void)
{
	Rfile *rf;
	File *f;
	long n;
	DTYPE *de;
	Dir d;
	struct stat stbuf;
	char *path;

	rf = rfilefid();
	if(rf->fd == 0)
		errjmp(Enotopen);
	if(rhdr.count > sizeof rdata)
		errjmp(Etoolarge);
	f = rf->file;
	if(rf->fd->dir){
		errno = 0;
		rhdr.count = (rhdr.count/DIRLEN)*DIRLEN;
		if(rf->fd->offset != rhdr.offset){
			rf->fd->offset = rhdr.offset;  /* sync offset */
			seekdir(rf->fd->dir, 0);
			for(n=0; n<rhdr.offset; ){
				de = readdir(rf->fd->dir);
				if(de == 0)
					break;
				if(de->d_ino==0 || de->d_name[0]==0)
					continue;
				if(strcmp(de->d_name, ".")==0 || strcmp(de->d_name, "..")==0)
					continue;
				n += DIRLEN;
			}
		}
		for(n=0; n<rhdr.count; ){
			de = readdir(rf->fd->dir);
			if(de == 0)
				break;
			if(de->d_ino==0 || de->d_name[0]==0)
				continue;
			if(strcmp(de->d_name, ".")==0 || strcmp(de->d_name, "..")==0)
				continue;
			strncpy(d.name, de->d_name, NAMELEN-1);
			d.name[NAMELEN-1] = 0;
			path = erealloc(0, strlen(f->path)+1+strlen(de->d_name)+1);
			sprintf(path, "%s/%s", f->path, de->d_name);
			memset(&stbuf, 0, sizeof stbuf);
			if(stat(path, &stbuf) < 0){
				fprintf(stderr, "dir: bad path %s\n", path);
				/* but continue... probably a bad symlink */
			}
			free(path);
			strncpy(d.uid, id2name(uid, stbuf.st_uid), NAMELEN);
			strncpy(d.gid, id2name(gid, stbuf.st_gid), NAMELEN);
			d.qid.path = qid(&stbuf);
			d.qid.vers = vers(&stbuf);
			d.mode = (d.qid.path&CHDIR)|(stbuf.st_mode&0777);
			d.atime = stbuf.st_atime;
			d.mtime = stbuf.st_mtime;
			d.len.hlength = 0;
			d.len.length = stbuf.st_size;
			convD2M(&d, rdata+n);
			n += DIRLEN;
		}
	}else{
		errno = 0;
		if(rf->fd->offset != rhdr.offset){
			rf->fd->offset = rhdr.offset;
			if(lseek(rf->fd->fd, rhdr.offset, 0) < 0)
				errjmp(sys_errlist[errno]);
		}
		n = read(rf->fd->fd, rdata, rhdr.count);
		if(n < 0)
			errjmp(sys_errlist[errno]);
	}
	rf->fd->offset += n;
	thdr.count = n;
	thdr.data = rdata;
}

void
rwrite(void)
{
	Rfile *rf;
	int n;

	if(readonly) errjmp("write disallowed");
	rf = rfilefid();
	if(rf->fd == 0)
		errjmp(Enotopen);
	if(rhdr.count > sizeof rdata)
		errjmp(Etoolarge);
	errno = 0;
	if(rf->fd->offset != rhdr.offset){
		rf->fd->offset = rhdr.offset;
		if(lseek(rf->fd->fd, rhdr.offset, 0) < 0)
			errjmp(sys_errlist[errno]);
	}
	n = write(rf->fd->fd, rhdr.data, rhdr.count);
	if(n < 0)
		errjmp(sys_errlist[errno]);
	rf->fd->offset += n;
	thdr.count = n;
}

void
rstat(void)
{
	Rfile *rf;
	File *f;
	Dir d;

	rf = rfilefid();
	f = rf->file;
	errjmp(dostat(f, 0));
	strncpy(d.name, f->name, NAMELEN);
	strncpy(d.uid, id2name(uid, f->stbuf.st_uid), NAMELEN);
	strncpy(d.gid, id2name(gid, f->stbuf.st_gid), NAMELEN);
	d.qid = f->qid;
	d.mode = (f->qid.path&CHDIR)|(f->stbuf.st_mode&0777);
	d.atime = f->stbuf.st_atime;
	d.mtime = f->stbuf.st_mtime;
	d.len.hlength = 0;
	d.len.length = f->stbuf.st_size;
	convD2M(&d, thdr.stat);
}

void
rwstat(void)
{
	Rfile *rf;
	File *f;
	Dir d;
	Pass *p;
	char *path, *dir, name[NAMELEN], *e;

	if(readonly) errjmp("write disallowed");
	rf = rfilefid();
	f = rf->file;
	errjmp(dostat(f, 0));
	convM2D(rhdr.stat, &d);
	errno = 0;
	if(strcmp(d.name, f->name) != 0){
		dir = bldpath(f->path, "..", name);
		path = erealloc(0, strlen(dir)+1+strlen(d.name)+1);
		sprintf(path, "%s/%s", dir, d.name);
		if(rename(f->path, path) < 0){
			free(path);
			errjmp(sys_errlist[errno]);
		}
		free(f->path);
		free(f->name);
		f->path = path;
		f->name = estrdup(d.name);
	}
	if((d.mode&0777) != (f->stbuf.st_mode&0777)){
		f->stbuf.st_mode = (f->stbuf.st_mode&~0777) | (d.mode&0777);
		if(chmod(f->path, f->stbuf.st_mode) < 0)
			errjmp(sys_errlist[errno]);
	}
	p = name2pass(gidname, d.gid);
	if(p == 0){
		if(strtol(d.gid, &e, 10) == f->stbuf.st_gid && *e == 0)
			return;
		errjmp(Eunknown);
	}
	if(p->id != f->stbuf.st_gid){
		if(chown(f->path, f->stbuf.st_uid, p->id) < 0)
			errjmp(sys_errlist[errno]);
		f->stbuf.st_gid = p->id;
	}
}

void
rclunk(int rm)
{
	int ret;
	const char *err;
	Rfile *rf;
	File *f;
	Fd *fd;

	err = 0;
	rf = rfilefid();
	f = rf->file;
	if(rm){
		if(f->qid.path & CHDIR)
			ret = rmdir(f->path);
		else
			ret = unlink(f->path);
		if(ret)
			err = sys_errlist[errno];
	}else if(rf->rclose){	/* ignore errors */
		if(f->qid.path & CHDIR)
			rmdir(f->path);
		else
			unlink(f->path);
	}
		
	rf->busy = 0;
	if(--f->ref == 0){
		free(f->path);
		free(f->name);
		free(f);
	}
	fd = rf->fd;
	if(fd){
		if(fd->ref <= 0)
			error("clunk fd count");
		if(--fd->ref == 0){
			if(fd->fd)
				close(fd->fd);
			if(fd->dir)
				closedir(fd->dir);
			free(fd);
		}
		rf->fd = 0;
	}
	if(err)
		errjmp(err);
}

char*
bldpath(char *a, char *elem, char *name)
{
	char *path, *p;

	if(strcmp(elem, "..") == 0){
		if(strcmp(a, "/") == 0){
			path = estrdup(a);
			strcpy(name, a);
		}else{
			p = strrchr(a, '/');
			if(p == 0){
				fprintf(stderr, "path: '%s'\n", path);
				error("malformed path 1");
			}
			if(p == a)	/* reduced to "/" */
				p++;
			path = erealloc(0, (p-a)+1);
			memmove(path, a, (p-a));
			path[(p-a)] = 0;
			if(strcmp(path, "/") == 0)
				p = path;
			else{
				p = strrchr(path, '/');
				if(p == 0){
					fprintf(stderr, "path: '%s'\n", path);
					error("malformed path 2");
				}
				p++;
			}
			if(strlen(p) >= NAMELEN)
				error("bldpath: name too long");
			strcpy(name, p);
		}
	}else{
		if(strcmp(a, "/") == 0)
			a = "";
		path = erealloc(0, strlen(a)+1+strlen(elem)+1);
		sprintf(path, "%s/%s", a, elem);
		if(strlen(elem) >= NAMELEN)
			error("bldpath: name too long");
		strcpy(name, elem);
	}
	return path;
}

const char*
dostat(File *f, char *elem)
{
	char *path;
	struct stat stbuf;
	char name[NAMELEN];

	if(elem)
		path = bldpath(f->path, elem, name);
	else
		path = f->path;
	errno = 0;
	if(stat(path, &stbuf) < 0)
		return sys_errlist[errno];
	if(elem){
		free(f->path);
		f->path = path;
		f->name = estrdup(name);
	}
	f->qid.path = qid(&stbuf);
	f->qid.vers = vers(&stbuf);
	f->stbuf = stbuf;
	return 0;
}

int
omode(int m)
{
	switch(m){
	case 0:		/* OREAD */
	case 3:		/* OEXEC */
		return 0;
	case 1:		/* OWRITE */
		return 1;
	case 2:		/* ORDWR */
		return 2;
	}
	errjmp(Emode);
	return 0;
}

void
sendmsg(const char *err)
{
	int n;

	if(err){
		thdr.type = Rerror;
		strncpy(thdr.ename, err, ERRLEN);
	}
	DBG(fprintf(stderr, "<< %s\n", mfmt(&thdr)));
	n = convS2M(&thdr, tdata);
	if(n == 0)
		error("bad sendmsg format");
	if(write(1, tdata, n) != n)
		error("write error");
}

int
okfid(int fid)
{
	enum{ Delta=10 };

	if(fid < 0){
		fprintf(stderr, "u9fs: negative fid %d\n", fid);
		return 0;
	}
	if(fid >= nrfilealloc){
		fid += Delta;
		rfile = erealloc(rfile, fid*sizeof(Rfile));
		memset(rfile+nrfilealloc, 0, (fid-nrfilealloc)*sizeof(Rfile));
		nrfilealloc = fid;
	}
	return 1;
}

Rfile*
rfilefid(void)
{
	Rfile *rf;

	if(!okfid(rhdr.fid))
		errjmp(Ebadfid);
	rf = &rfile[rhdr.fid];
	if(rf->busy == 0)
		errjmp(Ebadfid);
	if(rf->file->ref <= 0)
		error("ref count");
	setupuser(rf->uid);
	return rf;
}

File*
newfile(void)
{
	File *f;

	f = erealloc(0, sizeof(File));
	memset(f, 0, sizeof(File));
	f->ref = 1;
	return f;
}

/*
 * qids: directory bit, seven bits of device, 24 bits of inode
 */
Ulong
vers(struct stat *st)
{
	return st->st_mtime ^ (st->st_size<<8);
}

Ulong
qid(struct stat *st)
{
	static int nqdev;
	static Uchar *qdev;
	Ulong q;
	int dev;

	if(qdev == 0){
		qdev = erealloc(0, 65536U);
		memset(qdev, 0, 65536U);
	}
	q = 0;
	if((st->st_mode&S_IFMT) ==  S_IFDIR)
		q = CHDIR;
	dev = st->st_dev & 0xFFFFUL;
	if(qdev[dev] == 0){
		if(++nqdev >= 128)
			error("too many devices");
		qdev[dev] = nqdev;
	}
	q |= qdev[dev]<<24;
	q |= st->st_ino & 0x00FFFFFFUL;
	return q;
}

Pass*
name2pass(Pass **pw, char *name)
{
	Pass *p;

	for(p = pw[hash(name)]; p; p = p->nnext)
		if(strcmp(name, p->name) == 0)
			return p;
	return 0;
}

Pass*
id2pass(Pass **pw, int id)
{
	int i;
	Pass *p, *pp;

	pp = 0;
	/* use last on list == first in file */
	i = (id&0xFF) ^ ((id>>8)&0xFF);
	for(p = pw[i]; p; p = p->next)
		if(p->id == id)
			pp = p;
	return pp;
}

char*
id2name(Pass **pw, int id)
{
	int i;
	Pass *p;
	char *s;
	static char buf[40];

	s = 0;
	/* use last on list == first in file */
	i = (id&0xFF) ^ ((id>>8)&0xFF);
	for(p = pw[i]; p; p = p->next)
		if(p->id == id)
			s = p->name;
	if(s)
		return s;
	sprintf(buf, "%d", id);
	return buf;
}

void
freepass(Pass **pass)
{
	int i;
	Pass *p, *np;

	for(i=0; i<256; i++){
		for(p = pass[i]; p; p = np){
			np = p->next;
			free(p->mem);
			free(p);
		}
		pass[i] = 0; 
	}
}

void
setupuser(int u)
{
	Pass *p;

	if(curuid == u || myuid != 0)
		return;

	p = id2pass(uid, u);
	if(p == 0 || p->id == 0 || (uid_t)p->id == (uid_t)-1)
		errjmp(Eunknown);

	if(setreuid(0, 0) < 0)
		error("can't revert to root");

	/*
	 * this error message is just a likely guess
	 */
	if(setgroups(p->nmem, p->mem) < 0)
		errjmp("member of too many groups");

	if(setgid(p->gid) < 0
	|| setreuid(0, p->id) < 0)
		errjmp(Eunknown);
}

void
getpwdf(void)
{
	static int mtime;
	struct stat stbuf;
	struct passwd *pw;
	int i;
	Pass *p;

	if(onlyuser){
		i = (myuid&0xFF) ^ ((myuid>>8)&0xFF);
		if(uid[i])
			return;
		p = erealloc(0, sizeof(Pass));
		uid[i] = p;
		p->name = strdup(onlyuser);
		uidname[hash(p->name)] = p;
		p->id = myuid;
		p->gid = 60001;
		p->next = 0;
		p->nnext = 0;
		p->nmem = 0;
		p->mem = 0;
		curuid = p->id;
		return;
	}

	curuid = -1;
	if(myuid == 0 && setreuid(0, 0) < 0)
		error("can't revert to root");

	if(stat("/etc/passwd", &stbuf) < 0)
		error("can't read /etc/passwd");
	if(stbuf.st_mtime <= mtime)
		return;
	freepass(uid);
	for(i=0; i<256; i++)
		uidname[i] = 0;
	for(;;) {
		pw = getpwent();
		if(!pw)
			break;
		i = pw->pw_uid;
		i = (i&0xFF) ^ ((i>>8)&0xFF);
		p = erealloc(0, sizeof(Pass));
		p->next = uid[i];
		uid[i] = p;
		p->id = pw->pw_uid;
		p->gid = pw->pw_gid;
		p->name = estrdup(pw->pw_name);
		p->nmem = 1;
		p->mem = erealloc(0, sizeof(p->mem[0]));
		p->mem[0] = p->gid;
		i = hash(p->name);
		p->nnext = uidname[i];
		uidname[i] = p;
	}
	setpwent();
	endpwent();
}

int
ismem(Pass *u, int gid)
{
	int i;

	if(u->gid == gid)
		return 1;
	for(i = 0; i < u->nmem; i++)
		if(u->mem[i] == gid)
			return 1;
	return 0;
}

void
getgrpf(void)
{
	static int mtime;
	struct stat stbuf;
	struct group *pw;
	int i, m;
	Pass *p, *u;

	if(onlyuser)
		return;

	if(stat("/etc/group", &stbuf) < 0
	|| stbuf.st_mtime <= mtime)
		return;
	freepass(gid);
	for(i=0; i<256; i++)
		gidname[i] = 0;
	for(;;) {
		pw = getgrent();
		if(!pw)
			break;
		i = pw->gr_gid;
		i = (i&0xFF) ^ ((i>>8)&0xFF);
		p = erealloc(0, sizeof(Pass));
		p->next = gid[i];
		gid[i] = p;
		p->id = pw->gr_gid;
		p->gid = 0;
		p->name = estrdup(pw->gr_name);
		i = hash(p->name);
		p->nnext = gidname[i];
		gidname[i] = p;

		for(m=0; pw->gr_mem[m]; m++)
			;
		p->nmem = m;
		p->mem = 0;
		if(m != 0)
			p->mem = erealloc(0, m*sizeof(p->mem[0]));
		for(m = 0; m<p->nmem; m++){
			u = name2pass(uidname, pw->gr_mem[m]);
			p->mem[m] = -1;
			if(u){
				p->mem[m] = u->id;
				if(!ismem(u, p->id)){
					u->mem = erealloc(u->mem, (u->nmem+1)*sizeof(u->mem[0]));
					u->mem[u->nmem++] = p->id;
				}
			}
		}
	}
	setgrent();
	endgrent();
}

int
hash(char *s)
{
	int h;

	h = 0;
	for(; *s; s++)
		h = (h << 1) ^ *s;
	return h & 255;
}

void
error(char *s)
{
	fprintf(stderr, "u9fs: %s\n", s);
	perror("unix error");
	exit(1);
}

void
errjmp(const char *s)
{
	if(s == 0)
		return;
	sendmsg(s);
	longjmp(loopjmp, 1);
}

void*
erealloc(void *p, unsigned n)
{
	if(p == 0)
		p = malloc(n);
	else
		p = realloc(p, n);
	if(p == 0)
		error("realloc fail");
	return p;
}

char*
estrdup(char *p)
{
	p = strdup(p);
	if(p == 0)
		error("strdup fail");
	return p;
}

#ifdef SOCKETS
void
remotehostname(void)
{
	struct sockaddr_in sock;
	struct hostent *hp;
	int len;
	int on = 1;

	len = sizeof sock;
	if(getpeername(0, (struct sockaddr*)&sock, &len) < 0)
		error("getpeername");
	hp = gethostbyaddr((char *)&sock.sin_addr, sizeof (struct in_addr),
		sock.sin_family);
	if(hp == 0)
		error("gethostbyaddr");
	strncpy(bsdhost, hp->h_name, sizeof(bsdhost)-1);
	bsdhost[sizeof(bsdhost)-1] = '\0';
	fprintf(stderr, "bsdhost %s on %d\n", bsdhost, getpid());
	strcpy(ipaddr, inet_ntoa(sock.sin_addr));

	setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
}
#endif
