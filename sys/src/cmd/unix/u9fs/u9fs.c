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
#	undef sendmsg
	char	bsdhost[256];
	void	remotehostname(void);
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
	int		gid;
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
int	devallowed;
int	connected;

void	io(void);
void	error(char*);
char	*mfmt(Fcall*);
void	sendmsg(char*);
int	okfid(int);
Rfile*	rfilefid(void);
File*	newfile(void);
void*	erealloc(void*, unsigned);
char*	estrdup(char*);
char*	dostat(File*, char*);
char*	bldpath(char*, char*, char*);
Ulong	qid(struct stat*);
Ulong	vers(struct stat*);
void	errjmp(char*);
int	omode(int);
char*	id2name(Pass**, int);
Pass*	name2pass(Pass**, char*);
void	getpwdf(void);
void	getgrpf(void);
void	perm(Rfile*, int, struct stat*);
void	parentwrperm(Rfile*);

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

int
main(int argc, char *argv[])
{
	freopen(LOG, "a", stderr);
	setbuf(stderr, (void*)0);
	DBG(fprintf(stderr, "u9fs\nkill %d\n", getpid()));
	if(argc > 1)
		if(chroot(argv[1]) == -1)
			error("chroot failed");

#	ifdef SOCKETS
	remotehostname();
#	endif

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
		if(m <= 0)
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
	char *err;
	Pass *p;

	err = 0;
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
	p = name2pass(uid, rhdr.uname);
	if(p == 0)
		errjmp(Eunknown);
	if(p->id == 0)
		errjmp(Eperm);
#	ifdef SOCKETS
	if(ruserok(bsdhost, 0, rhdr.uname, rhdr.uname) < 0)
		errjmp(Eperm);
#	endif
	/* mark busy & inc ref cnt only after committed to succeed */
	rf->busy = 1;
	rf->file = file0;
	file0->ref++;
	rf->rclose = 0;
	rf->uid = p->id;
	rf->gid = p->gid;
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
	nrf->gid = rf->gid;
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
	char *err;
	Rfile *rf;
	File *of, *f;

	rf = rfilefid();
	if(rf->fd)
		errjmp(Eopen);
	of = rf->file;
	perm(rf, 1, 0);
	f = newfile();
	f->path = estrdup(of->path);
	err = dostat(f, rhdr.name);
	if(err){
		f->ref = 0;
		free(f->path);
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
		perm(rf, 4, 0);
		break;
	case 1:
	case 1|16:
		perm(rf, 2, 0);
		break;
	case 2:	
	case 0|16:
	case 2|16:
		perm(rf, 4, 0);
		perm(rf, 2, 0);
		break;
	case 3:
		perm(rf, 1, 0);
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
	char *path, *err;
	int fd;
	int m;
	char name[NAMELEN];

	rf = rfilefid();
	if(rf->fd)
		errjmp(Eopen);
	perm(rf, 2, 0);
	path = bldpath(rf->file->path, rhdr.name, name);
	m = omode(rhdr.mode&3);
	errno = 0;
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
		if(fd >= 0){
			fchmod(fd, rhdr.perm&0777);
			fchown(fd, rf->uid, rf->gid);
		}
	}else{
		fd = creat(path, 0666);
		if(fd >= 0){
			if(m != 1){
				close(fd);
				fd = open(path, m);
			}
			fchmod(fd, rhdr.perm&0777);
			fchown(fd, rf->uid, rf->gid);
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
			d.len.l.hlength = 0;
			d.len.l.length = stbuf.st_size;
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
	d.len.l.hlength = 0;
	d.len.l.length = f->stbuf.st_size;
	convD2M(&d, thdr.stat);
}

void
rwstat(void)
{
	Rfile *rf;
	File *f;
	Dir d;
	Pass *p;
	char *path, *dir, name[NAMELEN];

	rf = rfilefid();
	f = rf->file;
	errjmp(dostat(f, 0));
	convM2D(rhdr.stat, &d);
	errno = 0;
	if(rf->uid != f->stbuf.st_uid)
		errjmp(Eowner);
	if(strcmp(d.name, f->name) != 0){
		parentwrperm(rf);
		dir = bldpath(f->path, "..", name);
		path = erealloc(0, strlen(dir)+1+strlen(d.name)+1);
		sprintf(path, "%s/%s", dir, d.name);
		if(link(f->path, path) < 0){
			free(path);
			errjmp(sys_errlist[errno]);
		}
		if(unlink(f->path) < 0){
			free(path);
			errjmp(sys_errlist[errno]);
		}
		free(f->path);
		free(f->name);
		f->path = path;
		f->name = estrdup(d.name);
	}
	if((d.mode&0777) != (f->stbuf.st_mode&0777)){
		if(chmod(f->path, d.mode&0777) < 0)
			errjmp(sys_errlist[errno]);
		f->stbuf.st_mode &= ~0777;
		f->stbuf.st_mode |= d.mode&0777;
	}
	p = name2pass(gid, d.gid);
	if(p == 0)
		errjmp(Eunknown);
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
	char *err;
	Rfile *rf;
	File *f;
	Fd *fd;

	err = 0;
	rf = rfilefid();
	f = rf->file;
	if(rm){
		parentwrperm(rf);
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
			strcpy(name, p);
		}
	}else{
		if(strcmp(a, "/") == 0)
			a = "";
		path = erealloc(0, strlen(a)+1+strlen(elem)+1);
		sprintf(path, "%s/%s", a, elem);
		strcpy(name, elem);
	}
	if(strlen(name) >= NAMELEN)
		error("bldpath: name too long");
	return path;
}

char*
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
sendmsg(char *err)
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
	return rf;
}

void
perm(Rfile *rf, int mask, struct stat *st)
{
	if(st == 0)
		st = &rf->file->stbuf;
	/* plan 9 access semantics; simpler and more sensible */
	if(rf->uid == st->st_uid)
		if((st->st_mode>>6) & mask)
			return;
	if(rf->gid == st->st_gid)
		if((st->st_mode>>3) & mask)
			return;
	if((st->st_mode>>0) & mask)
		return;
	errjmp(Eperm);
}

void
parentwrperm(Rfile *rf)
{
	Rfile trf;
	struct stat st;
	char *dirp, dir[512];

	dirp = bldpath(rf->file->path, "..", dir);
	if(strlen(dirp) < sizeof dir){	/* ugly: avoid leaving dirp allocated */
		strcpy(dir, dirp);
		free(dirp);
		dirp = dir;
	}
	if(stat(dirp, &st) < 0)
		errjmp(Eperm);
	trf.uid = rf->uid;
	trf.gid = rf->gid;
	perm(&trf, 2, &st);
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
	return st->st_mtime;
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
	int i;
	Pass *p;

	for(i=0; i<256; i++)
		for(p = pw[i]; p; p = p->next)
			if(strcmp(name, p->name) == 0)
				return p;
	return 0;
}

char*
id2name(Pass **pw, int id)
{
	int i;
	Pass *p;
	char *s;
	static char buf[8];

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
			free(p);
		}
		pass[i] = 0; 
	}
}

void
getpwdf(void)
{
	static mtime;
	struct stat stbuf;
	struct passwd *pw;
	int i;
	Pass *p;

	if(stat("/etc/passwd", &stbuf) < 0)
		error("can't read /etc/passwd");
	if(stbuf.st_mtime <= mtime)
		return;
	freepass(uid);
	while(pw = getpwent()){
		i = pw->pw_uid;
		i = (i&0xFF) ^ ((i>>8)&0xFF);
		p = erealloc(0, sizeof(Pass));
		p->next = uid[i];
		uid[i] = p;
		p->id = pw->pw_uid;
		p->gid = pw->pw_gid;
		p->name = estrdup(pw->pw_name);
	}
	setpwent();
	endpwent();
}

void
getgrpf(void)
{
	static mtime;
	struct stat stbuf;
	struct group *pw;
	int i;
	Pass *p;

	if(stat("/etc/group", &stbuf) < 0)
		error("can't read /etc/group");
	if(stbuf.st_mtime <= mtime)
		return;
	freepass(gid);
	while(pw = getgrent()){
		i = pw->gr_gid;
		i = (i&0xFF) ^ ((i>>8)&0xFF);
		p = erealloc(0, sizeof(Pass));
		p->next = gid[i];
		gid[i] = p;
		p->id = pw->gr_gid;
		p->gid = 0;
		p->name = estrdup(pw->gr_name);
	}
	setgrent();
	endgrent();
}

void
error(char *s)
{
	fprintf(stderr, "u9fs: %s\n", s);
	perror("unix error");
	exit(1);
}

void
errjmp(char *s)
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
	if(getpeername(0, &sock, &len) < 0)
		error("getpeername");
	hp = gethostbyaddr((char *)&sock.sin_addr, sizeof (struct in_addr),
		sock.sin_family);
	if(hp == 0)
		error("gethostbyaddr");
	strcpy(bsdhost, hp->h_name);
	fprintf(stderr, "bsdhost %s on %d\n", bsdhost, getpid());

	setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
}
#endif
