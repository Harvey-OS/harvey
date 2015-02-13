/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Disable Unicode until the calls to FindFirstFile etc
 * are changed to use wide character strings.
 */
#undef UNICODE
#include	<windows.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#ifndef NAME_MAX
#	define NAME_MAX 256
#endif
#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
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

struct DIR
{
	HANDLE	handle;
	int8_t*	path;
	int	index;
	WIN32_FIND_DATA	wfd;
};

struct Ufsinfo
{
	int	mode;
	int	fd;
	int	uid;
	int	gid;
	DIR*	dir;
	uint32_t	offset;
	QLock	oq;
	int8_t nextname[NAME_MAX];
};

DIR*	opendir(int8_t*);
int	readdir(int8_t*, DIR*);
void	closedir(DIR*);
void	rewinddir(DIR*);

int8_t	*base = "c:/.";

static	Qid	fsqid(int8_t*, struct stat *);
static	void	fspath(Chan*, int8_t*, int8_t*);
// static	void	fsperm(Chan*, int);
static	uint32_t	fsdirread(Chan*, uint8_t*, int, uint32_t);
static	int	fsomode(int);
static  int	chown(int8_t *path, int uid, int);

/* clumsy hack, but not worse than the Path stuff in the last one */
static int8_t*
uc2name(Chan *c)
{
	int8_t *s;

	if(c->name == nil)
		return "/";
	s = c2name(c);
	if(s[0]=='#' && s[1]=='U')
		return s+2;
	return s;
}

static int8_t*
lastelem(Chan *c)
{
	int8_t *s, *t;

	s = uc2name(c);
	if((t = strrchr(s, '/')) == nil)
		return s;
	if(t[1] == 0)
		return t;
	return t+1;
}
	
static Chan*
fsattach(int8_t *spec)
{
	Chan *c;
	struct stat stbuf;
	static int devno;
	Ufsinfo *uif;

	if(stat(base, &stbuf) < 0)
		error(strerror(errno));

	c = devattach('U', spec);

	uif = mallocz(sizeof(Ufsinfo), 1);
	uif->gid = stbuf.st_gid;
	uif->uid = stbuf.st_uid;
	uif->mode = stbuf.st_mode;

	c->aux = uif;
	c->dev = devno++;
	c->qid.type = QTDIR;
/*print("fsattach %s\n", c2name(c));*/

	return c;
}

static Chan*
fsclone(Chan *c, Chan *nc)
{
	Ufsinfo *uif;

	uif = mallocz(sizeof(Ufsinfo), 1);
	*uif = *(Ufsinfo*)c->aux;
	nc->aux = uif;

	return nc;
}

static int
fswalk1(Chan *c, int8_t *name)
{
	struct stat stbuf;
	int8_t path[MAXPATH];
	Ufsinfo *uif;

	fspath(c, name, path);

	/*	print("** fs walk '%s' -> %s\n", path, name); */

	if(stat(path, &stbuf) < 0)
		return 0;

	uif = c->aux;

	uif->gid = stbuf.st_gid;
	uif->uid = stbuf.st_uid;
	uif->mode = stbuf.st_mode;

	c->qid = fsqid(path, &stbuf);

	return 1;
}

extern Cname* addelem(Cname*, int8_t*);

static Walkqid*
fswalk(Chan *c, Chan *nc, int8_t **name, int nname)
{
	int i;
	Cname *cname;
	Walkqid *wq;

	if(nc != nil)
		panic("fswalk: nc != nil");
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	nc = devclone(c);
	cname = c->name;
	incref(&cname->ref);

	fsclone(c, nc);
	wq->clone = nc;
	for(i=0; i<nname; i++){
		nc->name = cname;
		if(fswalk1(nc, name[i]) == 0)
			break;
		cname = addelem(cname, name[i]);
		wq->qid[i] = nc->qid;
	}
	nc->name = cname;
	if(i != nname){
		cclose(nc);
		wq->clone = nil;
	}
	wq->nqid = i;
	return wq;
}
	
static int
fsstat(Chan *c, uint8_t *buf, int n)
{
	Dir d;
	struct stat stbuf;
	int8_t path[MAXPATH];

	if(n < BIT16SZ)
		error(Eshortstat);

	fspath(c, 0, path);
	if(stat(path, &stbuf) < 0)
		error(strerror(errno));

	d.name = lastelem(c);
	d.uid = "unknown";
	d.gid = "unknown";
	d.muid = "unknown";
	d.qid = c->qid;
	d.mode = (c->qid.type<<24)|(stbuf.st_mode&0777);
	d.atime = stbuf.st_atime;
	d.mtime = stbuf.st_mtime;
	d.length = stbuf.st_size;
	d.type = 'U';
	d.dev = c->dev;
	return convD2M(&d, buf, n);
}

static Chan*
fsopen(Chan *c, int mode)
{
	int8_t path[MAXPATH];
	int m, isdir;
	Ufsinfo *uif;

/*print("fsopen %s\n", c2name(c));*/
	m = mode & (OTRUNC|3);
	switch(m) {
	case 0:
		break;
	case 1:
	case 1|16:
		break;
	case 2:	
	case 0|16:
	case 2|16:
		break;
	case 3:
		break;
	default:
		error(Ebadarg);
	}

	isdir = c->qid.type & QTDIR;

	if(isdir && mode != OREAD)
		error(Eperm);

	m = fsomode(m & 3);
	c->mode = openmode(mode);

	uif = c->aux;

	fspath(c, 0, path);
	if(isdir) {
		uif->dir = opendir(path);
		if(uif->dir == 0)
			error(strerror(errno));
	}	
	else {
		if(mode & OTRUNC)
			m |= O_TRUNC;
		uif->fd = open(path, m|_O_BINARY, 0666);

		if(uif->fd < 0)
			error(strerror(errno));
	}
	uif->offset = 0;

	c->offset = 0;
	c->flag |= COPEN;
	return c;
}

static void
fscreate(Chan *c, int8_t *name, int mode, uint32_t perm)
{
	int fd, m;
	int8_t path[MAXPATH];
	struct stat stbuf;
	Ufsinfo *uif;

	m = fsomode(mode&3);

	fspath(c, name, path);

	uif = c->aux;

	if(perm & DMDIR) {
		if(m)
			error(Eperm);

		if(mkdir(path) < 0)
			error(strerror(errno));

		fd = open(path, 0);
		if(fd >= 0) {
			chmod(path, perm & 0777);
			chown(path, uif->uid, uif->uid);
		}
		close(fd);

		uif->dir = opendir(path);
		if(uif->dir == 0)
			error(strerror(errno));
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
			error(strerror(errno));
		uif->fd = fd;
	}

	if(stat(path, &stbuf) < 0)
		error(strerror(errno));
	c->qid = fsqid(path, &stbuf);
	c->offset = 0;
	c->flag |= COPEN;
	c->mode = openmode(mode);
}

static void
fsclose(Chan *c)
{
	Ufsinfo *uif;

	uif = c->aux;

	if(c->flag & COPEN) {
		if(c->qid.type & QTDIR)
			closedir(uif->dir);
		else
			close(uif->fd);
	}

	free(uif);
}

static int32_t
fsread(Chan *c, void *va, int32_t n, int64_t offset)
{
	int fd, r;
	Ufsinfo *uif;

/*print("fsread %s\n", c2name(c));*/
	if(c->qid.type & QTDIR)
		return fsdirread(c, va, n, offset);

	uif = c->aux;
	qlock(&uif->oq);
	if(waserror()) {
		qunlock(&uif->oq);
		nexterror();
	}
	fd = uif->fd;
	if(uif->offset != offset) {
		r = lseek(fd, offset, 0);
		if(r < 0)
			error(strerror(errno));
		uif->offset = offset;
	}

	n = read(fd, va, n);
	if(n < 0)
		error(strerror(errno));

	uif->offset += n;
	qunlock(&uif->oq);
	poperror();

	return n;
}

static int32_t
fswrite(Chan *c, void *va, int32_t n, int64_t offset)
{
	int fd, r;
	Ufsinfo *uif;

	uif = c->aux;

	qlock(&uif->oq);
	if(waserror()) {
		qunlock(&uif->oq);
		nexterror();
	}
	fd = uif->fd;
	if(uif->offset != offset) {
		r = lseek(fd, offset, 0);
		if(r < 0)
			error(strerror(errno));
		uif->offset = offset;
	}

	n = write(fd, va, n);
	if(n < 0)
		error(strerror(errno));

	uif->offset += n;
	qunlock(&uif->oq);
	poperror();

	return n;
}

static void
fsremove(Chan *c)
{
	int n;
	int8_t path[MAXPATH];

	fspath(c, 0, path);
	if(c->qid.type & QTDIR)
		n = rmdir(path);
	else
		n = remove(path);
	if(n < 0)
		error(strerror(errno));
}

static int
fswstat(Chan *c, uint8_t *buf, int n)
{
	Dir d;
	struct stat stbuf;
	int8_t old[MAXPATH], new[MAXPATH];
	int8_t strs[MAXPATH*3], *p;
	Ufsinfo *uif;

	if (convM2D(buf, n, &d, strs) != n)
		error(Ebadstat);
	
	fspath(c, 0, old);
	if(stat(old, &stbuf) < 0)
		error(strerror(errno));

	uif = c->aux;

//	if(uif->uid != stbuf.st_uid)
//		error(Eowner);

	if(d.name[0] && strcmp(d.name, lastelem(c)) != 0) {
		fspath(c, 0, old);
		strcpy(new, old);
		p = strrchr(new, '/');
		strcpy(p+1, d.name);
		if(rename(old, new) < 0)
			error(strerror(errno));
	}

	fspath(c, 0, old);
	if(~d.mode != 0 && (int)(d.mode&0777) != (int)(stbuf.st_mode&0777)) {
		if(chmod(old, d.mode&0777) < 0)
			error(strerror(errno));
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
	return n;
}

static Qid
fsqid(int8_t *p, struct stat *st)
{
	Qid q;
	int dev;
	uint32_t h;
	static int nqdev;
	static uint8_t *qdev;

	if(qdev == 0)
		qdev = mallocz(65536U, 1);

	q.type = 0;
	if((st->st_mode&S_IFMT) ==  S_IFDIR)
		q.type = QTDIR;

	dev = st->st_dev & 0xFFFFUL;
	if(qdev[dev] == 0)
		qdev[dev] = ++nqdev;

	h = 0;
	while(*p != '\0')
		h += *p++ * 13;
	
	q.path = (int64_t)qdev[dev]<<32;
	q.path |= h;
	q.vers = st->st_mtime;

	return q;
}

static void
fspath(Chan *c, int8_t *ext, int8_t *path)
{
	strcpy(path, base);
	strcat(path, "/");
	strcat(path, uc2name(c));
	if(ext) {
		strcat(path, "/");
		strcat(path, ext);
	}
	cleanname(path);
}

static int
isdots(int8_t *name)
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

static int
p9readdir(int8_t *name, Ufsinfo *uif)
{
	if(uif->nextname[0]){
		strcpy(name, uif->nextname);
		uif->nextname[0] = 0;
		return 1;
	}

	return readdir(name, uif->dir);
}

static uint32_t
fsdirread(Chan *c, uint8_t *va, int count, uint32_t offset)
{
	int i;
	Dir d;
	int32_t n;
	int8_t de[NAME_MAX];
	struct stat stbuf;
	int8_t path[MAXPATH], dirpath[MAXPATH];
	Ufsinfo *uif;

/*print("fsdirread %s\n", c2name(c));*/
	i = 0;
	uif = c->aux;

	errno = 0;
	if(uif->offset != offset) {
		if(offset != 0)
			error("bad offset in fsdirread");
		uif->offset = offset;  /* sync offset */
		uif->nextname[0] = 0;
		rewinddir(uif->dir);
	}

	fspath(c, 0, dirpath);

	while(i+BIT16SZ < count) {
		if(!p9readdir(de, uif))
			break;

		if(de[0]==0 || isdots(de))
			continue;

		d.name = de;
		sprint(path, "%s/%s", dirpath, de);
		memset(&stbuf, 0, sizeof stbuf);

		if(stat(path, &stbuf) < 0) {
			print("dir: bad path %s\n", path);
			/* but continue... probably a bad symlink */
		}

		d.uid = "unknown";
		d.gid = "unknown";
		d.muid = "unknown";
		d.qid = fsqid(path, &stbuf);
		d.mode = (d.qid.type<<24)|(stbuf.st_mode&0777);
		d.atime = stbuf.st_atime;
		d.mtime = stbuf.st_mtime;
		d.length = stbuf.st_size;
		d.type = 'U';
		d.dev = c->dev;
		n = convD2M(&d, (uint8_t*)va+i, count-i);
		if(n == BIT16SZ){
			strcpy(uif->nextname, de);
			break;
		}
		i += n;
	}
/*print("got %d\n", i);*/
	uif->offset += i;
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
readdir(int8_t *name, DIR *d)
{
	if(d->index != 0) {
		if(FindNextFile(d->handle, &d->wfd) == FALSE)
			return 0;
	}
	strcpy(name, (int8_t*)d->wfd.cFileName);
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
chown(int8_t *path, int uid, int perm)
{
/*	panic("chown"); */
	return 0;
}

DIR*
opendir(int8_t *p)
{
	DIR *d;
	int8_t path[MAX_PATH];

	
	snprint(path, sizeof(path), "%s/*.*", p);

	d = mallocz(sizeof(DIR), 1);
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

Dev fsdevtab = {
	'U',
	"fs",

	devreset,
	devinit,
	devshutdown,
	fsattach,
	fswalk,
	fsstat,
	fsopen,
	fscreate,
	fsclose,
	fsread,
	devbread,
	fswrite,
	devbwrite,
	fsremove,
	fswstat,
};
