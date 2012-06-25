#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Mount Mount;

struct Mount
{
	Mount	*next;
	Udev		*dev;
	int		npath;
	char		path[];
};

static Mount *mtab;

void
fsmount(Udev *dev, char *path)
{
	Mount *m, **p;
	int n;

	if(dev == nil)
		return;

	n = strlen(path);
	m = kmalloc(sizeof(*m) + n + 1);
	m->dev = dev;
	m->next = nil;
	m->npath = n;
	strcpy(m->path, path);

	for(p=&mtab;;p=&((*p)->next)){
		Mount *x;

		if(x = *p){
			if(m->npath < x->npath)
				continue;
			if(m->npath == x->npath){
				if(strcmp(m->path, x->path) < 0)
					continue;
			}
		}
		m->next = *p;
		*p = m;
		break;
	}
}

ulong
hashpath(char *s)
{
	ulong h;
	for(h=0; *s; s++)
		h = (h * 13) + (*s - 'a');
	return h;
}

char*
basepath(char *p, char **ps)
{
	char *x, *s;
	int n;

	if(s = strrchr(p, '/')){
		if(s[1] != 0){
			if(ps)
				*ps = kstrdup(s+1);
			if((n = s - p) == 0)
				n = 1;
			x = kmalloc(n+1);
			memmove(x, p, n);
			x[n] = 0;
			return x;
		}
	}
	if(ps)
		*ps = nil;
	return nil;
}

char*
allocpath(char *base, char *prefix, char *name)
{
	char *p, *s;
	int n, m, k;

	n = strlen(base);
	m = strlen(name);
	k = prefix ? strlen(prefix) : 0;
	p = s = kmalloc(n+m+k+2);
	memmove(p, base, n);
	p += n;
	if(m || k)
		*p++ = '/';
	if(k){
		memmove(p, prefix, k);
		p += k;
	}
	memmove(p, name, m+1);
	return s;
}

char*
fullpath(char *base, char *name)
{
	char *s;

	if(*name == '/' || *name == '#'){
		s = kstrdup(name);
	} else if(base) {
		s = allocpath(base, nil, name);
	} else {
		s = nil;
	}
	if(s != nil)
		cleanname(s);
	return s;
}

char*
shortpath(char *base, char *path)
{
	int n;

	n = strlen(base);
	if((n <= strlen(path)) && (strncmp(path, base, n)==0)){
		path += n;
		if(*path == '/')
			path++;
		if(*path == 0)
			path = ".";
	}
	return path;
}

char*
fsfullpath(char *path)
{
	char *root;

	path = fullpath(current->cwd, path);
	if(path && (root = current->root)){
		root = allocpath(root, nil, path+1);
		free(path);
		path = root;
	}
	return path;
}

char*
fsrootpath(char *path)
{
	char *root;

	if(root = current->root){
		root = shortpath(root, path);
		if(*root == '.'){
			path = "/";
		} else if(root > path){
			path = root-1;
		}
	}
	return path;
}

static Mount*
path2mount(char *path)
{
	Mount *m;

	for(m=mtab; m; m=m->next){
		if(strncmp(path, m->path, m->npath) == 0){
			switch(path[m->npath]){
			case '\0':
			case '/':
				return m;
			}
		}
	}
	return nil;
}

static Udev*
path2dev(char *path)
{
	Mount *m;

	if(m = path2mount(path))
		return m->dev;
	return nil;
}

static int
fsenter(int *perr)
{
	int err;

	if(perr == nil)
		perr = &err;
	if(current->linkloop > 8)
		return *perr = -ELOOP;
	current->linkloop++;
	return 0;
}

static void
fsleave(void)
{
	current->linkloop--;
}

int sys_getcwd(char *buf, int len)
{
	int n;
	char *cwd;

	trace("sys_getcwd(%p, %x)", buf, len);

	cwd = current->cwd;
	n = strlen(cwd)+1;
	if(n > len)
		return -ERANGE;
	memmove(buf, cwd, n);
	return n;
}

int
fsopen(char *path, int mode, int perm, Ufile **pf)
{
	int err;
	Udev *dev;

	trace("fsopen(%s, %#o, %#o)", path, mode, perm);

	*pf = nil;
	if(fsenter(&err) < 0)
		return err;
	err = -ENOENT;
	if((dev = path2dev(path)) && dev->open)
		err = dev->open(path, mode, perm, pf);
	fsleave();
	return err;
}

int
fsaccess(char *path, int mode)
{
	int err;
	Udev *dev;

	trace("fsaccess(%s, %#o)", path, mode);

	if(fsenter(&err) < 0)
		return err;
	err = -ENOENT;
	if(dev = path2dev(path)){
		err = 0;
		if(dev->access)
			err = dev->access(path, mode);
	}
	fsleave();

	return err;
}

int sys_access(char *name, int mode)
{
	int err;

	trace("sys_access(%s, %#o)", name, mode);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsaccess(name, mode);
	free(name);

	return err;
}

int sys_open(char *name, int mode, int perm)
{
	int err;
	Ufile *file;

	trace("sys_open(%s, %#o, %#o)", name, mode, perm);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsopen(name, mode, perm, &file);
	free(name);

	if(err == 0)
		err = newfd(file, FD_CLOEXEC);

	return err;
}

int sys_creat(char *name, int perm)
{
	trace("sys_create(%s, %#o)", name, perm);

	return sys_open(name, O_CREAT|O_TRUNC, perm);
}

int
fsstat(char *path, int link, Ustat *ps)
{
	int err;
	Udev *dev;

	trace("fsstat(%s, %d)", path, link);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->stat){
		memset(ps, 0, sizeof(Ustat));
		err = dev->stat(path, link, ps);
	}
	fsleave();
	return err;
}

int
sys_chdir(char *name)
{
	int err;
	Ufile *f;

	trace("sys_chdir(%s)", name);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsopen(name, O_RDONLY, 0, &f);
	free(name);
	if(err == 0){
		err = chdirfile(f);
		putfile(f);
	}
	return err;
}

int sys_chroot(char *name)
{
	Ufile *f;
	Ustat s;
	int err;

	trace("sys_chroot(%s)", name);

	f = nil;
	if((err = fsopen(name, O_RDONLY, 0, &f)) < 0)
		goto out;
	err = -ENOTDIR;
	if(f->path == nil)
		goto out;
	if(devtab[f->dev]->fstat == nil)
		goto out;
	if((err = devtab[f->dev]->fstat(f, &s)) < 0)
		goto out;
	err = -ENOTDIR;
	if((s.mode & ~0777) != S_IFDIR)
		goto out;
	err = 0;
	free(current->root);
	if(strcmp(f->path, "/") == 0){
		current->root = nil;
	} else {
		current->root = kstrdup(f->path);
	}
out:
	putfile(f);
	return err;
}

int
fschown(char *path, int uid, int gid, int link)
{
	int err;
	Udev *dev;

	trace("fschown(%s, %d, %d, %d)", path, uid, gid, link);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->chown)
		err = dev->chown(path, uid, gid, link);
	fsleave();
	return err;
}

int sys_chown(char *name, int uid, int gid)
{
	int err;

	trace("sys_chown(%s, %d, %d)", name, uid, gid);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err =  fschown(name, uid, gid, 0);
	free(name);

	return err;
}

int sys_lchown(char *name, int uid, int gid)
{
	int err;

	trace("sys_lchown(%s, %d, %d)", name, uid, gid);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fschown(name, uid, gid, 1);
	free(name);

	return err;
}

int
fsreadlink(char *path, char *buf, int len)
{
	int err;
	Udev *dev;

	trace("fsreadlink(%s)", path);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->readlink)
		err = dev->readlink(path, buf, len);
	fsleave();

	return err;
}

int sys_readlink(char *name, char *buf, int len)
{
	int err;

	trace("sys_readlink(%s, %p, %x)", name, buf, len);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsreadlink(name, buf, len);
	free(name);

	return err;
}

int
fsrename(char *old, char *new)
{
	int err;
	Udev *dev;

	trace("fsrename(%s, %s)", old, new);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(old)) && dev->rename){
		err = -EXDEV;
		if(dev == path2dev(new))
			err = dev->rename(old, new);
	}
	fsleave();

	return err;
}


int sys_rename(char *from, char *to)
{
	int err;

	trace("sys_rename(%s, %s)", from, to);

	if((from = fsfullpath(from)) == nil)
		return -EFAULT;
	if((to = fsfullpath(to)) == nil){
		free(from);
		return -EFAULT;
	}
	err = fsrename(from, to);
	free(from);
	free(to);

	return err;
}

int
fsmkdir(char *path, int mode)
{
	int err;
	Udev *dev;

	trace("fsmkdir(%s, %#o)", path, mode);

	if(fsenter(&err) < 0)
		return err;

	err = -EPERM;
	if((dev = path2dev(path)) && dev->mkdir)
		err = dev->mkdir(path, mode);
	fsleave();

	return err;
}

int sys_mkdir(char *name, int mode)
{
	int err;

	trace("sys_mkdir(%s, %#o)", name, mode);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsmkdir(name, mode);
	free(name);

	return err;
}

int
fsutime(char *path, int atime, int mtime)
{
	int err;
	Udev *dev;

	trace("fsutime(%s, %d, %d)", path, atime, mtime);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->utime)
		err = dev->utime(path, atime, mtime);
	fsleave();

	return err;
}

struct linux_utimbuf
{
	long	atime;
	long	mtime;
};

int sys_utime(char *name, void *times)
{
	int err;
	struct linux_utimbuf *t = times;

	trace("sys_utime(%s, %p)", name, times);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	if(t != nil){
		err = fsutime(name, t->atime, t->mtime);
	}else{
		long x = time(0);
		err = fsutime(name, x, x);
	}
	free(name);

	return err;
}

int sys_utimes(char *name, void *tvp)
{
	int err;
	struct linux_timeval *t = tvp;

	trace("sys_utimes(%s, %p)", name, tvp);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	if(t != nil){
		err = fsutime(name, t[0].tv_sec, t[1].tv_sec);
	}else{
		long x = time(0);
		err = fsutime(name, x, x);
	}
	free(name);

	return err;
}

int
fschmod(char *path, int mode)
{
	int err;
	Udev *dev;

	trace("fschmod(%s, %#o)", path, mode);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->chmod)
		err = dev->chmod(path, mode);
	fsleave();

	return err;
}

int sys_chmod(char *name, int mode)
{
	int err;

	trace("sys_chmod(%s, %#o)", name, mode);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fschmod(name, mode);
	free(name);

	return err;
}

int
fstruncate(char *path, vlong size)
{
	int err;
	Udev *dev;

	trace("fstruncate(%s, %llx)", path, size);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->truncate)
		err = dev->truncate(path, size);
	fsleave();

	return err;
}

int sys_truncate(char *name, ulong size)
{
	int err;

	trace("sys_truncate(%s, %lux)", name, size);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fstruncate(name, size);
	free(name);

	return err;
}

int
fsunlink(char *path, int rmdir)
{
	int err;
	Udev *dev;

	trace("fsunlink(%s, %d)", path, rmdir);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(path)) && dev->unlink)
		err = dev->unlink(path, rmdir);
	fsleave();

	return err;
}

int sys_unlink(char *name)
{
	int err;

	trace("sys_unlink(%s)", name);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsunlink(name, 0);
	free(name);

	return err;
}

int sys_rmdir(char *name)
{
	int err;

	trace("sys_rmdir(%s)", name);

	if((name = fsfullpath(name)) == nil)
		return -EFAULT;
	err = fsunlink(name, 1);
	free(name);

	return err;
}

int
fslink(char *old, char *new, int sym)
{
	int err;
	Udev *dev;

	trace("fslink(%s, %s, %d)", old, new, sym);

	if(fsenter(&err) < 0)
		return err;
	err = -EPERM;
	if((dev = path2dev(new)) && dev->link){
		err = -EXDEV;
		if(sym || dev == path2dev(old))
			err = dev->link(old, new, sym);
	}
	fsleave();

	return err;
}

int sys_link(char *old, char *new)
{
	int err;

	trace("sys_link(%s, %s)", old, new);

	if((old = fsfullpath(old)) == nil)
		return -EFAULT;
	if((new = fsfullpath(new)) == nil){
		free(old);
		return -EFAULT;
	}
	err = fslink(old, new, 0);
	free(old);
	free(new);

	return err;
}

int sys_symlink(char *old, char *new)
{
	int err;

	trace("sys_symlink(%s, %s)", old, new);

	if((new = fsfullpath(new)) == nil)
		return -EFAULT;
	err = fslink(old, new, 1);
	free(new);

	return err;
}

