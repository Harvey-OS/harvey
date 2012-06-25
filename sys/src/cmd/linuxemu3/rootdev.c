#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Rfile Rfile;
typedef struct Rpath Rpath;

struct Rfile
{
	Ufile;
	struct
	{
		Dir	*d;
		int	i;
		int	n;
	}		diraux;
};

struct Rpath
{
	Ref;

	Rpath	*hash;

	int		deleted;
	char		str[];
};

static Rpath	*rpathtab[64];
static QLock	rpathtablk;

static Rpath **
rpathent(char *path)
{
	Rpath **prp;

	prp = &rpathtab[hashpath(path) % nelem(rpathtab)];
	while(*prp){
		if(strcmp(path, (*prp)->str) == 0)
			break;
		prp = &((*prp)->hash);
	}
	return prp;
}

static char*
linkname(char *name)
{
	if(strncmp(name, ".udir.L.", 8) == 0)
		name += 8;
	return name;
}

static char*
udirpath(char *base, char *name, char type)
{
	char buf[9];

	strcpy(buf, ".udir.T.");
	buf[6] = type;
	return allocpath(base, buf, name);
}

static int
udirget(char *base, char *name, char type, char **val)
{
	char *f, *b;
	int n, r, s;
	int fd;

	r = -1;
	f = udirpath(base, name, type);
	if((fd = open(shortpath(current->kcwd, f), OREAD)) < 0)
		goto out;
	if(val == nil){
		r = 0;
		goto out;
	}
	if((s = seek(fd, 0, 2)) < 0)
		goto out;
	b = kmalloc(s+1);
	n = 0;
	if(s > 0){
		seek(fd, 0, 0);
		if((n = read(fd, b, s)) < 0){
			free(b);
			goto out;
		}
	}
	b[n] = 0;

	r = 0;
	*val = b;
out:
	free(f);
	close(fd);
	return r;
}

static char*
resolvepath1(char *path, int link)
{
	char *r, *b, *p, *o, *e;
	char **a;

	int n;
	int i;

	r = nil;
	a = nil;
	n = 0;

	b = kstrdup(path);
	for(p=b; *p; p++){
		if(*p == '/'){
			if((n % 16) == 0)
				a = krealloc(a, sizeof(a[0]) * (n+16));
			a[n++] = p;
		}
	}

	e = nil;
	for(i=n-1; i>=0; i--){
		char *t;
		char *f;

		o = e;
		e = a[i];
		*e++ = 0;

		f = linkname(e);
		t = nil;

		if(!udirget(b, f, 'L', &t)){
			if(t == nil)
				break;
			if(link && o==nil){
				free(t);
				if(f != e)
					break;
				t = udirpath(b, e, 'L');
			}
			r = fullpath(b, t);
			free(t);
			if(o && o[1]){
				t = r;
				r = fullpath(t, &o[1]);
				free(t);
			}
			break;
		}

		--e;
		if(o) *o = '/';
	}
	free(b);
	free(a);

	return r;
}

static char *
resolvepath(char *path, int link)
{
	char *t;
	int x;

	x = 0;
	path = kstrdup(path);
	while(t = resolvepath1(path, link)){
		if(++x > 8){
			free(t);
			free(path);
			return nil;
		}
		free(path); path = t;
	}
	return path;
}

static int
ropen(char *path, int mode, int perm, Ufile **pf)
{
	Ufile *f;
	int err;
	char *s, *t;
	int mode9, perm9;
	int fd;
	char *base;
	char *name;
	Rpath **prp;

	trace("ropen(%s, %#o, %#o, ...)", path, mode, perm);

	base = nil;
	name = nil;
	mode9 = mode & 3;
	perm9 = (perm & ~current->umask) & 0777;

	s = shortpath(current->kcwd, path);

	if(mode & O_CREAT) {
		Dir *d;

		err = -EINVAL;
		if((base = basepath(path, &name)) == nil)
			goto out;

		/* resolve base directory */
		if((d = dirstat(shortpath(current->kcwd, base))) == nil){
			err = mkerror();
			if(t = resolvepath1(base, 0)){
				free(base); base = t;
				t = allocpath(t, nil, name);
				err = fsopen(t, mode, perm, pf);
				free(t);
			}
			goto out;
		}
		err = -ENOTDIR;
		if((d->mode & DMDIR) == 0){
			free(d);
			goto out;
		}
		free(d);

		/* check if here is a symlink in the way */
		t = udirpath(base, name, 'L');
		if((fd = open(shortpath(current->kcwd, t), OREAD)) >= 0){
			free(t);
			close(fd);

			if(mode & O_EXCL){
				err = -EEXIST;
				goto out;
			}

			if((t = resolvepath1(path, 0)) == nil)
				goto out;
			err = fsopen(t, mode, perm, pf);
			free(t);
			goto out;
		}
		free(t);

		if(mode & (O_EXCL | O_TRUNC)){
			if(mode & O_EXCL)
				mode9 |= OEXCL;
			fd = create(s, mode9, perm9);
		} else {
			/* try open first to avoid truncating existing the file */
			if((fd = open(s, mode9)) < 0)
				fd = create(s, mode9, perm9);
		}
		if(fd < 0){
			err = mkerror();
			goto out;
		}
	} else {
		if(((mode & 3) == O_RDWR) || ((mode & 3) == O_WRONLY))
			if(mode & O_TRUNC)
				mode9 |= OTRUNC;

		if((fd = open(s, mode9)) < 0){
			err = mkerror();
			if(t = resolvepath1(path, 0)){
				err = fsopen(t, mode, perm, pf);
				free(t);
			}
			goto out;
		}
	}

	qlock(&rpathtablk);
	prp = rpathent(path);
	if(*prp != nil){
		incref(*prp);
	} else {
		Rpath *rp;

		rp = kmalloc(sizeof(*rp) + strlen(path) + 1);
		rp->ref = 1;
		rp->hash = nil;
		rp->deleted = 0;
		strcpy(rp->str, path);
		*prp = rp;
	}
	qunlock(&rpathtablk);

	f = kmallocz(sizeof(Rfile), 1);
	f->ref = 1;
	f->path = kstrdup(path);
	f->dev = ROOTDEV;
	f->mode = mode;
	f->fd = fd;
	f->off = 0;
	*pf = f;

	err = 0;

out:
	free(base);
	free(name);

	return err;
}

static int
rclose(Ufile *f)
{
	Rpath **prp;
	Rfile *file = (Rfile*)f;
	static char path[1024];	/* protected by rpathtablk */

	qlock(&rpathtablk);
	prp = rpathent(file->path);
	if(!decref(*prp)){
		Rpath *rp = *prp;
		*prp = rp->hash;
		if(rp->deleted){
			if(fd2path(file->fd, path, sizeof(path)) == 0)
				remove(shortpath(current->kcwd, path));
		}
		free(rp);
	}
	qunlock(&rpathtablk);

	close(file->fd);
	return 0;
}

static int
rread(Ufile *f, void *buf, int len, vlong off)
{
	Rfile *file = (Rfile*)f;
	int ret, n;

	n = ret = 0;
	if(notifyme(1))
		return -ERESTART;
	while((n < len) && ((ret = pread(file->fd, (uchar*)buf + n, len - n, off + n)) > 0))
		n += ret;
	notifyme(0);
	if(ret < 0)
		return mkerror();
	return n;
}

static int
rwrite(Ufile *f, void *buf, int len, vlong off)
{
	Rfile *file = (Rfile*)f;
	int ret;

	if(notifyme(1))
		return -ERESTART;
	ret = pwrite(file->fd, buf, len, off);
	notifyme(0);
	if(ret < 0)
		ret = mkerror();
	return ret;
}

static vlong
rsize(Ufile *f)
{
	Rfile *file = (Rfile*)f;

	return seek(file->fd, 0, 2);
}

static int
raccess(char *path, int mode)
{
	static char omode[] = {
		0,			// ---
		OEXEC,		// --x
		OWRITE,		// -w- 
		ORDWR,		// -wx
		OREAD,		// r--
		OEXEC,		// r-x
		ORDWR,		// rw-
		ORDWR		// rwx
	};

	int err;
	int fd;
	Dir *d;
	char *s;

	err = -EINVAL;
	if(mode & ~07)
		return err;

	s = shortpath(current->kcwd, path);
	if((d = dirstat(s)) == nil){
		err = mkerror();
		if(path = resolvepath1(path, 0)){
			err = fsaccess(path, mode);
			free(path);
		}
		goto out;
	}

	/* ignore the exec bit... firefox gets confused */
	mode &= ~01;
	if((mode == 0) || (d->mode & DMDIR)){
		err = 0;
	} else {
		err = -EACCES;
		if((mode & 01) && ((d->mode & 0111) == 0))
			goto out;
		if((mode & 02) && ((d->mode & 0222) == 0))
			goto out;
		if((mode & 04) && ((d->mode & 0444) == 0))
			goto out;
		if((fd = open(s, omode[mode])) >= 0){
			close(fd);
			err = 0;
		}
	}
out:
	free(d);
	return err;
}

static ulong
dir2statmode(Dir *d)
{
	ulong mode;

	mode = d->mode & 0777;
	if(d->mode & DMDIR)
		mode |= S_IFDIR;
	else if(strcmp(d->name, "cons") == 0)
		mode |= S_IFCHR;
	else if(strncmp(d->name, "PTS.", 4) == 0)
		mode |= S_IFCHR;
	else if(strcmp(d->name, "zero") == 0)
		mode |= S_IFCHR | 0222;
	else if(strcmp(d->name, "null") == 0)
		mode |= S_IFCHR | 0222;
	else if(strncmp(d->name, ".udir.", 6) == 0){
		switch(d->name[6]){
		case 'L':
			mode |= S_IFLNK;
			break;
		case 'S':
			mode |= S_IFSOCK;
			break;
		case 'F':
			mode |= S_IFIFO;
			break;
		case 'C':
			mode |= S_IFCHR;
			break;
		case 'B':
			mode |= S_IFBLK;
			break;
		}
	} else if(d->type == '|') 
		mode |= S_IFIFO;
	else if(d->type == 'H')
		mode |= S_IFBLK;
	else
		mode |= S_IFREG;

	return mode;
}

static void
dir2ustat(Dir *d, Ustat *s)
{
	s->mode = dir2statmode(d);
	s->uid = current->uid;
	s->gid = current->gid;
	s->size = d->length;
	s->atime = d->atime;
	s->mtime = d->mtime;
	s->ctime = d->mtime;
	s->ino = 0;	// use d->qid?
	s->dev = 0;
	s->rdev = 0;
}

static int
rstat(char *path, int link, Ustat *s)
{
	Dir *d;
	int err;
	char *t;

	if((d = dirstat(shortpath(current->kcwd, path))) == nil){
		if(link){
			char *base;
			char *name;
			if(base = basepath(path, &name)){
				t = udirpath(base, name, 'L');
				free(name);
				free(base);
				d = dirstat(shortpath(current->kcwd, t));
				free(t);
			}
				
		}
	}
	if(d == nil){
		err = mkerror();
		if(t = resolvepath1(path, 0)){
			err = fsstat(t, link, s);
			free(t);
		}
		return err;
	}

	dir2ustat(d, s);
	s->ino = hashpath(path);

	free(d);
	return 0;
}

static int
rfstat(Ufile *f, Ustat *s)
{
	Dir *d;

	if((d = dirfstat(f->fd)) == nil)
		return mkerror();

	dir2ustat(d, s);
	s->ino = hashpath(f->path);

	free(d);
	return 0;
}

static char*
fixname(char *name)
{
	if(name == nil)
		return nil;
	if(strncmp(name, ".udir.", 6) == 0){
		if(name[6] && name[7]=='.')
			name += 8;
	}
	return name;
}

static int
rreaddir(Ufile *f, Udirent **pd)
{
	Dir *d;
	int i, n;

	seek(f->fd, 0, 0);
	n = dirreadall(f->fd, &d);
	if(n < 0)
		return mkerror();
	for(i=0; i<n; i++){
		if((*pd = newdirent(f->path, fixname(d[i].name), dir2statmode(&d[i]))) == nil)
			break;
		pd = &((*pd)->next);
	}
	free(d);
	return i;
}

static int
rreadlink(char *path, char *buf, int len)
{
	int err;
	int fd;

	char *t;
	char *name;
	char *base;

	trace("rreadlink(%s)", path);

	if((base = basepath(path, &name)) == nil)
		return -EINVAL;

	/* resolve base path */
	if((fd = open(shortpath(current->kcwd, base), OREAD)) < 0){
		err = mkerror();
		if(t = resolvepath1(base, 0)){
			free(base); base = t;
			t = allocpath(base, nil, name);
			err = fsreadlink(t, buf, len);
			free(t);
		}
		goto out;
	}
	close(fd);

	/* check if path is regular file */
	if((fd = open(shortpath(current->kcwd, path), OREAD)) >= 0){
		close(fd);
		err = -EINVAL;
		goto out;
	}

	t = udirpath(base, name, 'L');
	if((fd = open(shortpath(current->kcwd, t), OREAD)) < 0){
		err = mkerror();
		free(t);
		goto out;
	}
	free(t);
	if((err = read(fd, buf, len)) < 0)
		err = mkerror();
	close(fd);
out:
	free(base);
	free(name);
	return err;
}

enum {
	COPYSIZE = 8*1024,
};

static int
copyfile(char *from, char *to)
{
	int err, fromfd, tofd;
	char *buf, *s;
	Dir *ent;
	Dir *dir;

	dir = nil;
	buf = nil;
	ent = nil;

	tofd = -1;

	trace("copyfile(%s, %s)", from, to);

	if((fromfd = open(shortpath(current->kcwd, from), OREAD)) < 0){
		err = mkerror();
		goto out;
	}
	if((dir = dirfstat(fromfd)) == nil){
		err = mkerror();
		goto out;
	}
	s = shortpath(current->kcwd, to);
	if((err = open(s, OREAD)) >= 0){
		close(err);
		err = -EEXIST;
		goto out;
	}
	if(dir->mode & DMDIR){
		int n;
		if((tofd = create(s, OREAD, dir->mode)) < 0){
			err = mkerror();
			goto out;
		}
		close(tofd);
		tofd = -1;
		while((n = dirread(fromfd, &ent)) > 0){
			int i;

			for(i=0; i<n; i++){
				char *froment, *toent;

				froment = allocpath(from, nil, ent[i].name);
				toent = allocpath(to, nil, ent[i].name);
				err = copyfile(froment, toent);
				free(froment);
				free(toent);

				if(err < 0)
					goto out;
			}
			free(ent); ent = nil;
		}
	} else {
		if((tofd = create(s, OWRITE, dir->mode)) < 0){
			err = mkerror();
			goto out;
		}
		buf = kmalloc(COPYSIZE);
		for(;;){
			err = read(fromfd, buf, COPYSIZE);
			if(err == 0)
				break;
			if(err < 0){
				err = mkerror();
				goto out;
			}
			if(write(tofd, buf, err) != err){
				err = mkerror();
				goto out;
			}
		}
	}

	err = 0;
out:
	free(ent);
	free(dir);
	free(buf);
	close(fromfd);
	close(tofd);
	return err;
}

static int
removefile(char *path)
{
	int err;
	int n;
	Dir *d;
	int fd;
	char *s;

	trace("removefile(%s)", path);

	s = shortpath(current->kcwd, path);

	if((d = dirstat(s)) == nil)
		return mkerror();
	if(remove(s) == 0){
		free(d);
		return 0;
	}
	if((d->mode & DMDIR) == 0){
		free(d);
		return mkerror();
	}
	free(d);
	if((fd = open(s, OREAD)) < 0)
		return mkerror();
	err = 0;
	d = nil;
	while((n = dirread(fd, &d)) > 0){
		char *t;
		int i;

		for(i=0; i<n; i++){
			t = allocpath(path, nil, d[i].name);
			err = removefile(t);
			free(t);

			if(err < 0)
				break;
		}
		free(d); d = nil;

		if(err < 0)
			break;
	}
	close(fd);
	if(err < 0)
		return err;
	if(n < 0)
		return mkerror();
	if(remove(s) < 0)
		return mkerror();
	return 0;
}

static int
resolvefromtopath(char **from, char **to)
{
	char *t;

	trace("resolvefromtopath(%s, %s)", *from, *to);

	if((*from = resolvepath(*from, 1)) == nil){
		*to = nil;
		return -ELOOP;
	}
	if((*to = resolvepath(*to, 1)) == nil){
		free(*from);
		*from = nil;
		return -ELOOP;
	}
	if(strstr(*from, ".udir.L")){
		char *x;

		x = nil;
		for(t=*to; *t; t++){
			if(*t == '/')
				x = t;
		}

		if(strncmp(x+1, ".udir.", 6)){
			*x = 0;
			t = udirpath(*to, x+1, 'L');
			free(*to); *to = t;
		}
	}

	return 0;
}

static int
rrename(char *from, char *to)
{
	int err;
	char *x, *y, *t;

	trace("rrename(%s, %s)", from, to);

	if((err = resolvefromtopath(&from, &to)) < 0)
		goto out;
	if(strcmp(from, to) == 0)
		goto out;
	x = nil;
	for(t=from; *t; t++){
		if(*t == '/')
			x = t;
	}
	y = nil;
	for(t=to; *t; t++){
		if(*t == '/')
			y = t;
	}
	if(x && y){
		char *e;

		e = nil;
		*x = 0; *y = 0;
		if(strcmp(from, to) == 0)
			e = &y[1];
		*x = '/'; *y = '/';

		if(e != nil){
			Dir d;

			nulldir(&d);
			d.name = e;

			remove(to);
			if(dirwstat(shortpath(current->kcwd, from), &d) < 0)
				err = mkerror();
			goto out;
		}
	}
	t = ksmprint("%s%d%d.tmp", to, current->pid, current->tid);
	if((err = copyfile(from, t)) == 0){
		Dir d;

		nulldir(&d);
		d.name = &y[1];

		remove(shortpath(current->kcwd, to));
		if(dirwstat(shortpath(current->kcwd, t), &d) < 0) {
			err = mkerror();
		} else {
			removefile(from);
		}
	}
	if(err != 0)
		removefile(t);
	free(t);
out:
	free(from);
	free(to);

	return err;
}

static int
rmkdir(char *path, int mode)
{
	int err;
	Dir *d;
	int fd;
	int mode9;

	char *base;
	char *name;
	char *t;

	trace("rmkdir(%s, %#o)", path, mode);

	if((base = basepath(path, &name)) == nil)
		return -EINVAL;

	if((d = dirstat(shortpath(current->kcwd, base))) == nil){
		err = mkerror();
		if(t = resolvepath1(base, 0)){
			free(base); base = t;
			t = allocpath(base, nil, name);
			err = fsmkdir(t, mode);
			free(t);
		}
		goto out;
	}
	err = -ENOTDIR;
	if((d->mode & DMDIR) == 0){
		free(d);
		goto out;
	}
	free(d);

	err = -EEXIST;
	t = udirpath(base, name, 'L');
	if(d = dirstat(shortpath(current->kcwd, t))){
		free(d);
		free(t);
		goto out;
	}
	free(t);

	mode9 = DMDIR | ((mode & ~current->umask) & 0777);
	if((fd = create(shortpath(current->kcwd, path), OREAD|OEXCL, mode9)) < 0){
		err = mkerror();
		goto out;
	}
	close(fd);
	err = 0;

out:
	free(name);
	free(base);
	return err;
}

static void
combinedir(Dir *ndir, Dir *odir)
{
	if(ndir->mode != ~0)
		ndir->mode = (odir->mode & ~0777) | (ndir->mode & 0777);
}

static int
uwstat(char *path, Dir *ndir, int link)
{
	int err;
	Dir *dir;
	char *s;

	trace("uwstat(%s, ..., %d)", path, link);

	s = shortpath(current->kcwd, path);
	if((dir = dirstat(s)) == nil){
		err = mkerror();
		if(link){
			char *base;
			char *name;

			if(base = basepath(path, &name)){
				char *t;

				t = udirpath(base, name, 'L');
				free(base);
				free(name);

				err = uwstat(t, ndir, 0);
				free(t);
			}
		}
		return err;
	}
	combinedir(ndir, dir);
	err = 0;
	if(dirwstat(s, ndir) < 0)
		err = mkerror();
	free(dir);
	return err;	
}

static int
uwfstat(Ufile *f, Dir *ndir)
{
	int err;
	Dir *dir;

	if((dir = dirfstat(f->fd)) == nil){
		err = mkerror();
		goto out;
	}
	combinedir(ndir, dir);
	err = 0;
	if(dirfwstat(f->fd, ndir) < 0)
		err = mkerror();
out:
	free(dir);
	return err;
}

static int
rutime(char *path, long atime, long mtime)
{
	Dir ndir;
	int err;

	trace("rutime(%s, %ld, %ld)", path, atime, mtime);

	nulldir(&ndir);
	ndir.atime = atime;
	ndir.mtime = mtime;

	if((err = uwstat(path, &ndir, 1)) < 0){
		char *t;

		if(t = resolvepath1(path, 0)){
			err = fsutime(t, atime, mtime);
			free(t);
		}
	}
	return err;
}

static int
rchmod(char *path, int mode)
{
	Dir ndir;
	int err;

	trace("rchmod(%s, %#o)", path, mode);

	nulldir(&ndir);
	ndir.mode = mode;

	if((err = uwstat(path, &ndir, 1)) < 0){
		char *t;

		if(t = resolvepath1(path, 0)){
			err = fschmod(t, mode);
			free(t);
		}
	}
	return err;
}

static int
rchown(char *path, int uid, int gid, int link)
{
	Ustat s;

	USED(uid);
	USED(gid);

	/* FIXME, just return the right errorcode for now */
	return fsstat(path, link, &s);
}

static int
rtruncate(char *path, vlong size)
{
	Dir ndir;
	int err;

	trace("rtruncate(%s, %lld)", path, size);

	nulldir(&ndir);
	ndir.length = size;

	if((err = uwstat(path, &ndir, 0)) < 0){
		char *t;

		if(t = resolvepath1(path, 0)){
			err = fstruncate(t, size);
			free(t);
		}
	}
	return err;
}

static int
rfchmod(Ufile *f, int mode)
{
	Dir ndir;

	nulldir(&ndir);
	ndir.mode = mode;
	return uwfstat(f, &ndir);
}

static int
rfchown(Ufile *f, int uid, int gid)
{
	USED(f);
	USED(uid);
	USED(gid);

	return 0;
}

static int
rftruncate(Ufile *f, vlong size)
{
	Dir ndir;

	nulldir(&ndir);
	ndir.length = size;
	return uwfstat(f, &ndir);
}

static int
runlink(char *path, int rmdir)
{
	int err;
	Dir *dir;
	char *t, *s;
	char *base;
	char *name;
	char *rpath;
	Rpath **prp;

	trace("runlink(%s, %d)", path, rmdir);

	rpath = nil;
	dir = nil;
	err = -EINVAL;
	if((base = basepath(path, &name)) == nil)
		goto out;
	if(dir = dirstat(shortpath(current->kcwd, path))){
		rpath = kstrdup(path);
	} else {
		rpath = udirpath(base, name, 'L');
		dir = dirstat(shortpath(current->kcwd, rpath));
	}
 	if(dir == nil){
		err = mkerror();
		if(t = resolvepath1(path, 0)){
			err = fsunlink(t, rmdir);
			free(t);
		}		
		goto out;
	}
	if(rmdir){
		if((dir->mode & DMDIR) == 0){
			err = -ENOTDIR;
			goto out;
		}
	} else {
		if(dir->mode & DMDIR){
			err = -EISDIR;
			goto out;
		}
	}

	s = shortpath(current->kcwd, rpath);

	qlock(&rpathtablk);
	prp = rpathent(path);
	if(*prp){
		Dir ndir;

		t = ksmprint(".%s.%d.deleted", name, current->kpid);
		nulldir(&ndir);
		ndir.name = t;
		trace("runlink: file %s still in use renaming to -> %s", path, t);
		if(dirwstat(s, &ndir) < 0){
			qunlock(&rpathtablk);
			err = mkerror();
			free(t);
			goto out;
		}
		free(t);
		(*prp)->deleted = 1;
		qunlock(&rpathtablk);

	} else {
		int x;
		qunlock(&rpathtablk);

		x = 0;
		while(remove(s) < 0){
			err = mkerror();
			if(++x > 8){
				/* old debian bug clashes with mntgen */
				if(strcmp(base, "/")==0 && strstr(path, ".dpkg-"))
					err = -ENOENT;
				goto out;
			}
		}
	}
	err = 0;
out:
	free(dir);
	free(name);
	free(base);
	free(rpath);

	return err;
}

static int
rlink(char *old, char *new, int sym)
{
	int err;
	int fd;
	char *base;
	char *name;
	char *t;

	trace("rlink(%s, %s, %d)", old, new, sym);

	if((base = basepath(new, &name)) == nil)
		return -EINVAL;

	/* resolve base directory */
	if((fd = open(shortpath(current->kcwd, base), OREAD)) < 0){
		err = mkerror();
		if(t = resolvepath1(base, 0)){
			free(base); base = t;
			t = allocpath(base, nil, name);
			err = fslink(old, t, sym);
			free(t);
		}
		goto out;
	}
	close(fd);

	if(sym == 0){
		if((err = resolvefromtopath(&old, &new)) == 0)
			err = copyfile(old, new);
		free(old);
		free(new);
		goto out;
	}

	/* check if regular file is in the way */
	err = -EEXIST;
	if((fd = open(shortpath(current->kcwd, new), OREAD)) >= 0){
		close(fd);
		goto out;
	}

	/* try to create the link, will fail if alreadt exists */
	t = udirpath(base, name, 'L');
	if((fd = create(shortpath(current->kcwd, t), OWRITE|OEXCL, 0777)) < 0){
		err = mkerror();
		free(t);
		goto out;
	}
	free(t);

	if(write(fd, old, strlen(old)) < 0){
		err = mkerror();
		close(fd);
		goto out;
	}
	close(fd);
	err = 0;
out:
	free(base);
	free(name);
	return err;
}

static Udev rootdev = 
{
	.open = ropen,
	.access = raccess,
	.stat = rstat,
	.link = rlink,
	.unlink = runlink,
	.rename = rrename,
	.mkdir = rmkdir,
	.utime = rutime,
	.chmod = rchmod,
	.chown = rchown,
	.truncate = rtruncate,

	.read = rread,
	.write = rwrite,
	.size = rsize,
	.close = rclose,

	.fstat = rfstat,
	.readdir = rreaddir,
	.readlink = rreadlink,

	.fchmod = rfchmod,
	.fchown = rfchown,
	.ftruncate = rftruncate,
};

void rootdevinit(void)
{
	devtab[ROOTDEV] = &rootdev;

	fsmount(&rootdev, "");
}
