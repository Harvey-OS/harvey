#include	<windows.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	"lib9.h"
#include	"sys.h"
#include	"error.h"


typedef struct DIR	DIR;
typedef	struct Ufsinfo	Ufsinfo;

enum
{
	NUID	= 256,
	NGID	= 256,
	MAXPATH	= 1024,
	MAXCOMP	= 128
};

extern	char*	sys_errlist[];

struct DIR
{
	HANDLE	handle;
	char*	path;
	int	index;
	WIN32_FIND_DATA	wfd;
};

struct Ufsinfo
{
	int	uid;
	int	gid;
	int	mode;
	int	fd;
	DIR*	dir;
	ulong	offset;
	Qlock	oq;
};

DIR*	opendir(char*);
int	readdir(char*, DIR*);
void	closedir(DIR*);
void	rewinddir(DIR*);

char	*base = "c:/.";

static	Qid	fsqid(char*, struct stat *);
static	void	fspath(Path*, char*, char*);
static	void	fsperm(Chan*, int);
static	ulong	fsdirread(Chan*, uchar*, int, ulong);
static	int	fsomode(int);
static  int	chown(char *path, int uid, int);
static	int	link(char *path, char *next);

void
fsinit(void)
{
}

Chan*
fsattach(void *spec)
{
	Chan *c;
	struct stat stbuf;
	static int devno;
	Ufsinfo *uif;

	if(stat(base, &stbuf) < 0)
		error(sys_errlist[errno]);

	c = devattach('U', spec);

	uif = mallocz(sizeof(Ufsinfo));
	uif->gid = stbuf.st_gid;
	uif->uid = stbuf.st_uid;
	uif->mode = stbuf.st_mode;

	c->u.aux = uif;
	c->dev = devno++;

	return c;
}

Chan*
fsclone(Chan *c, Chan *nc)
{
	Ufsinfo *uif;

	nc = devclone(c, nc);
	uif = mallocz(sizeof(Ufsinfo));
	*uif = *(Ufsinfo*)c->u.aux;
	nc->u.aux = uif;

	return nc;
}

int
fswalk(Chan *c, char *name)
{
	Path *op;
	struct stat stbuf;
	char path[MAXPATH];
	Ufsinfo *uif;

	fspath(c->path, name, path);

/*	print("** fs walk '%s' -> %s\n", path, name); /**/

	if(stat(path, &stbuf) < 0)
		return 0;

	uif = c->u.aux;

	uif->gid = stbuf.st_gid;
	uif->uid = stbuf.st_uid;
	uif->mode = stbuf.st_mode;

	c->qid = fsqid(path, &stbuf);

	op = c->path;
	c->path = ptenter(&syspt, op, name);
	refdec(&op->r);

	return 1;
}

void
fsstat(Chan *c, char *buf)
{
	Dir d;
	struct stat stbuf;
	char path[MAXPATH];

	fspath(c->path, 0, path);
	if(stat(path, &stbuf) < 0)
		error(sys_errlist[errno]);

	strncpy(d.name, c->path->elem, NAMELEN);
	strncpy(d.uid, "unknown", NAMELEN);
	strncpy(d.gid, "unknown", NAMELEN);
	d.qid = c->qid;
	d.mode = (c->qid.path&CHDIR)|(stbuf.st_mode&0777);
	d.atime = stbuf.st_atime;
	d.mtime = stbuf.st_mtime;
	d.length = stbuf.st_size;
	d.type = 'U';
	d.dev = c->dev;
	convD2M(&d, buf);
}

Chan*
fsopen(Chan *c, int mode)
{
	char path[MAXPATH];
	int m, isdir;
	Ufsinfo *uif;

	m = mode & (OTRUNC|3);
	switch(m) {
	case 0:
		fsperm(c, 4);
		break;
	case 1:
	case 1|16:
		fsperm(c, 2);
		break;
	case 2:	
	case 0|16:
	case 2|16:
		fsperm(c, 4);
		fsperm(c, 2);
		break;
	case 3:
		fsperm(c, 1);
		break;
	default:
		error(Ebadarg);
	}

	isdir = c->qid.path & CHDIR;

	if(isdir && mode != OREAD)
		error(Eperm);

	m = fsomode(m & 3);
	c->mode = openmode(mode);

	uif = c->u.aux;

	fspath(c->path, 0, path);
	if(isdir) {
		uif->dir = opendir(path);
		if(uif->dir == 0)
			error(sys_errlist[errno]);
	}	
	else {
		if(mode & OTRUNC)
			m |= O_TRUNC;
		uif->fd = open(path, m|_O_BINARY, 0666);

		if(uif->fd < 0)
			error(sys_errlist[errno]);
	}
	uif->offset = 0;

	c->offset = 0;
	c->flag |= COPEN;
	return c;
}

void
fscreate(Chan *c, char *name, int mode, ulong perm)
{
	int fd, m;
	char path[MAXPATH];
	struct stat stbuf;
	Path *op;
	Ufsinfo *uif;

	fsperm(c, 2);

	m = fsomode(mode&3);

	fspath(c->path, name, path);

	uif = c->u.aux;

	if(perm & CHDIR) {
		if(m)
			error(Eperm);

		if(mkdir(path) < 0)
			error(sys_errlist[errno]);

		fd = open(path, 0);
		if(fd >= 0) {
			chmod(path, perm & 0777);
			chown(path, uif->uid, uif->uid);
		}
		close(fd);

		uif->dir = opendir(path);
		if(uif->dir == 0)
			error(sys_errlist[errno]);
	}
	else {
		fd = open(path, _O_WRONLY|_O_BINARY|_O_CREAT|_O_TRUNC, 0666);
		if(fd >= 0) {
			if(m != 1) {
				close(fd);
				fd = open(path, m|_O_BINARY);
			}
			chmod(path, perm & 0777);
			chown(path, uif->uid, uif->gid);
		}
		if(fd < 0)
			error(sys_errlist[errno]);
		uif->fd = fd;
	}

	if(stat(path, &stbuf) < 0)
		error(sys_errlist[errno]);
	c->qid = fsqid(path, &stbuf);
	c->offset = 0;
	c->flag |= COPEN;
	c->mode = openmode(mode);

	op = c->path;
	c->path = ptenter(&syspt, op, name);
	refdec(&op->r);
}

void
fsclose(Chan *c)
{
	Ufsinfo *uif;

	uif = c->u.aux;

	if(c->flag & COPEN) {
		if(c->qid.path & CHDIR)
			closedir(uif->dir);
		else
			close(uif->fd);
	}

	free(uif);
}

long
fsread(Chan *c, void *va, long n, ulong offset)
{
	int fd, r;
	Ufsinfo *uif;

	if(c->qid.path & CHDIR)
		return fsdirread(c, va, n, offset);

	uif = c->u.aux;
	qlock(&uif->oq);
	if(waserror()) {
		qunlock(&uif->oq);
		nexterror();
	}
	fd = uif->fd;
	if(uif->offset != offset) {
		r = lseek(fd, offset, 0);
		if(r < 0)
			error(sys_errlist[errno]);
		uif->offset = offset;
	}

	n = read(fd, va, n);
	if(n < 0)
		error(sys_errlist[errno]);

	uif->offset += n;
	qunlock(&uif->oq);
	poperror();

	return n;
}

long
fswrite(Chan *c, void *va, long n, ulong offset)
{
	int fd, r;
	Ufsinfo *uif;

	uif = c->u.aux;

	qlock(&uif->oq);
	if(waserror()) {
		qunlock(&uif->oq);
		nexterror();
	}
	fd = uif->fd;
	if(uif->offset != offset) {
		r = lseek(fd, offset, 0);
		if(r < 0)
			error(sys_errlist[errno]);
		uif->offset = offset;
	}

	n = write(fd, va, n);
	if(n < 0)
		error(sys_errlist[errno]);

	uif->offset += n;
	qunlock(&uif->oq);
	poperror();

	return n;
}

void
fsremove(Chan *c)
{
	int n;
	char path[MAXPATH];

	fspath(c->path, 0, path);
	if(c->qid.path & CHDIR)
		n = rmdir(path);
	else
		n = remove(path);
	if(n < 0)
		error(sys_errlist[errno]);
}

void
fswchk(char *path)
{
	struct stat stbuf;

	if(stat(path, &stbuf) < 0)
		error(sys_errlist[errno]);

	if(stbuf.st_uid == up->uid)
		stbuf.st_mode >>= 6;
	else
	if(stbuf.st_gid == up->gid)
		stbuf.st_mode >>= 3;

	if(stbuf.st_mode & S_IWRITE)
		return;

	error(Eperm);
}

void
fswstat(Chan *c, char *buf)
{
	Dir d;
	Path *ph;
	struct stat stbuf;
	char old[MAXPATH], new[MAXPATH], dir[MAXPATH];
	Ufsinfo *uif;

	convM2D(buf, &d);
	
	fspath(c->path, 0, old);
	if(stat(old, &stbuf) < 0)
		error(sys_errlist[errno]);

	uif = c->u.aux;

	if(uif->uid != stbuf.st_uid)
		error(Eowner);

	if(strcmp(d.name, c->path->elem) != 0) {
		fspath(c->path->parent, 0, dir);
		fswchk(dir);		
		fspath(c->path, 0, old);
		ph = ptenter(&syspt, c->path->parent, d.name);
		fspath(ph, 0, new);
		if(link(old, new) < 0)
			error(sys_errlist[errno]);
		if(unlink(old) < 0)
			error(sys_errlist[errno]);

		refdec(&c->path->r);
		c->path = ph;
	}

	fspath(c->path, 0, old);
	if((int)(d.mode&0777) != (int)(stbuf.st_mode&0777)) {
		if(chmod(old, d.mode&0777) < 0)
			error(sys_errlist[errno]);
		uif->mode &= ~0777;
		uif->mode |= d.mode&0777;
	}
/*
	p = name2pass(gid, d.gid);
	if(p == 0)
		error(Eunknown);

	if(p->id != stbuf.st_gid) {
		if(chown(old, stbuf.st_uid, p->id) < 0)
			error(sys_errlist[errno]);

		uif->gid = p->id;
	}
*/
}

static Qid
fsqid(char *p, struct stat *st)
{
	Qid q;
	int dev;
	ulong h;
	static int nqdev;
	static uchar *qdev;
	uchar *name;

	name = p;

	if(qdev == 0)
		qdev = mallocz(65536U);

	q.path = 0;
	if((st->st_mode&S_IFMT) ==  S_IFDIR)
		q.path = CHDIR;

	dev = st->st_dev & 0xFFFFUL;
	if(qdev[dev] == 0) {
		if(++nqdev >= 128)
			error("too many devices");
		qdev[dev] = nqdev;
	}
	h = 0;
	while(*p != '\0')
		h += *p++ * 13;
	
	q.path |= qdev[dev]<<24;
	q.path |= h & 0x00FFFFFFUL;
	q.vers = st->st_mtime;

	return q;
}

static void
fspath(Path *p, char *ext, char *path)
{
	int i, n;
	char *comp[MAXCOMP];

	strcpy(path, base);
	i = strlen(base);

	if(p == 0) {
		if(ext) {
			path[i++] = '/';
			strcpy(path+i, ext);
		}
		return;
	}

	n = 0;
	if(ext)
		comp[n++] = ext;
	while(p->parent) {
		comp[n++] = p->elem;
		p = p->parent;
	}

	while(n) {
		path[i++] = '/';
		strcpy(path+i, comp[--n]);
		i += strlen(comp[n]);
	}
	path[i] = '\0';
}

static void
fsperm(Chan *c, int mask)
{
	int m;
	Ufsinfo *uif;

	uif = c->u.aux;

	m = uif->mode;
/*
	print("fsperm: %o %o uuid %d ugid %d cuid %d cgid %d\n",
		m, mask, up->uid, up->gid, uif->uid, uif->gid);
*/
	if(uif->uid == up->uid)
		m >>= 6;
	else
	if(uif->gid == up->gid)
		m >>= 3;

	m &= mask;
	if(m == 0)
		error(Eperm);
}

static int
isdots(char *name)
{
	if(name[0] != '.')
		return 0;
	if(name[1] == '\0')
		return 1;
	if(name[1] != '.')
		return 0;
	if(name[2] == '\0')
		return 1;
	return 0;
}

static ulong
fsdirread(Chan *c, uchar *va, int count, ulong offset)
{
	int i;
	Dir d;
	long n;
	char de[256];
	struct stat stbuf;
	char path[MAXPATH], dirpath[MAXPATH];
	Ufsinfo *uif;

	count = (count/DIRLEN)*DIRLEN;

	i = 0;
	uif = c->u.aux;

	errno = 0;
	if(uif->offset != offset) {
		uif->offset = offset;  /* sync offset */
		rewinddir(uif->dir);
		for(n=0; n<(int)offset; ) {
			if(!readdir(de, uif->dir))
				break;
			if(de[0]==0 || isdots(de))
				continue;
			n += DIRLEN;
		}
	}

	fspath(c->path, 0, dirpath);

	while(i < count) {
		if(!readdir(de, uif->dir))
			break;

		if(de[0]==0 || isdots(de))
			continue;

		strncpy(d.name, de, NAMELEN-1);
		d.name[NAMELEN-1] = 0;
		sprint(path, "%s/%s", dirpath, de);
		memset(&stbuf, 0, sizeof stbuf);

		if(stat(path, &stbuf) < 0) {
			iprint("dir: bad path %s\n", path);
			/* but continue... probably a bad symlink */
		}

		strncpy(d.uid, "unknown", NAMELEN);
		strncpy(d.gid, "unknown", NAMELEN);
		d.qid = fsqid(path, &stbuf);
		d.mode = (d.qid.path&CHDIR)|(stbuf.st_mode&0777);
		d.atime = stbuf.st_atime;
		d.mtime = stbuf.st_mtime;
		d.length = stbuf.st_size;
		d.type = 'U';
		d.dev = c->dev;
		convD2M(&d, va+i);
		i += DIRLEN;
	}
	return i;
}

static int
fsomode(int m)
{
	switch(m) {
	case 0:			/* OREAD */
	case 3:			/* OEXEC */
		return 0;
	case 1:			/* OWRITE */
		return 1;
	case 2:			/* ORDWR */
		return 2;
	}
	error(Ebadarg);
	return 0;
}
void
closedir(DIR *d)
{
	FindClose(d->handle);
	free(d->path);
}

int
readdir(char *name, DIR *d)
{
	if(d->index != 0) {
		if(FindNextFile(d->handle, &d->wfd) == FALSE)
			return 0;
	}
	strcpy(name, d->wfd.cFileName);
	d->index++;

	return 1;
}

void
rewinddir(DIR *d)
{
	FindClose(d->handle);
	d->handle = FindFirstFile(d->path, &d->wfd);
	d->index = 0;
}

static int
chown(char *path, int uid, int perm)
{
/*	panic("chown"); */
	return 0;
}

static int
link(char *path, char *next)
{
	panic("link");
	return 0;
}

DIR*
opendir(char *p)
{
	DIR *d;
	char path[MAX_PATH];

	
	snprint(path, sizeof(path), "%s/*.*", p);

	d = mallocz(sizeof(DIR));
	if(d == 0)
		return 0;

	d->index = 0;

	d->handle = FindFirstFile(path, &d->wfd);
	if(d->handle == INVALID_HANDLE_VALUE) {
		free(d);
		return 0;
	}

	d->path = strdup(path);
	return d;
}

