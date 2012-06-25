#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

int
ufstat(int fd, Ustat *ps)
{
	Ufile *f;
	int err;

	err = -EBADF;
	if((f = fdgetfile(fd)) == nil)
		goto out;
	err = -EPERM;
	if(devtab[f->dev]->fstat == nil)
		goto out;
	memset(ps, 0, sizeof(Ustat));
	err = devtab[f->dev]->fstat(f, ps);
out:
	putfile(f);
	return err;
}

struct linux_stat {
	ushort st_dev;
	ushort __pad1;
	ulong st_ino;
	ushort st_mode;
	ushort st_nlink;
	ushort st_uid;
	ushort st_gid;
	ushort st_rdev;
	ushort __pad2;
	ulong  st_size;
	ulong  st_blksize;
	ulong  st_blocks;
	ulong  st_atime;
	ulong  __unused1;
	ulong  st_mtime;
	ulong  __unused2;
	ulong  st_ctime;
	ulong  __unused3;
	ulong  __unused4;
	ulong  __unused5;
};

static void
ustat2linuxstat(Ustat *x, struct linux_stat *s)
{
	memset(s, 0, sizeof(*s));
	s->st_dev = x->dev;
	s->st_ino = x->ino;
	s->st_mode = x->mode;
	s->st_nlink = 1;
	s->st_uid = x->uid;
	s->st_gid = x->gid;
	s->st_size = x->size;
	s->st_rdev = x->rdev;
	s->st_blksize = 4096;
	s->st_blocks = (x->size+s->st_blksize-1) / s->st_blksize;
	s->st_atime = x->atime;
	s->st_mtime = x->mtime;
	s->st_ctime = x->ctime;
}


struct linux_stat64 {
	uvlong	lst_dev;
	uint		__pad1;
	uint		__lst_ino;
	uint		lst_mode;
	uint		lst_nlink;
	uint		lst_uid;
	uint		lst_gid;
	uvlong	lst_rdev;
	uint		__pad2;
	vlong	lst_size;
	uint		lst_blksize;
	uvlong	lst_blocks;
	uint		lst_atime;
	uint		lst_atime_nsec;
	uint		lst_mtime;
	uint		lst_mtime_nsec;
	uint		lst_ctime;
	uint		lst_ctime_nsec;
	uvlong	lst_ino;
};

static void
ustat2linuxstat64(Ustat *x, struct linux_stat64 *s)
{
	memset(s, 0, sizeof(*s));
	s->lst_dev = x->dev;
	s->lst_ino = x->ino;
	s->__lst_ino = x->ino & 0xFFFFFFFF;
	s->lst_mode = x->mode;
	s->lst_nlink = 1;
	s->lst_uid = x->uid;
	s->lst_gid = x->gid;
	s->lst_size = x->size;
	s->lst_rdev = x->rdev;
	s->lst_blksize = 4096; // good as any
	s->lst_blocks = (x->size+s->lst_blksize-1) / s->lst_blksize;
	s->lst_atime = x->atime;
	s->lst_mtime = x->mtime;
	s->lst_ctime = x->ctime;	
}

int sys_linux_stat(char *path, void *st)
{
	int err;
	Ustat x;

	trace("sys_linux_stat(%s, %p)", path, st);
	err = fsstat(path, 0, &x);
	if(err < 0)
		return err;
	ustat2linuxstat(&x, (struct linux_stat*)st);
	return err;
}

int sys_linux_lstat(char *path, void *st)
{
	int err;
	Ustat x;

	trace("sys_linux_lstat(%s, %p)", path, st);

	if((path = fsfullpath(path)) == nil)
		return -EFAULT;
	err = fsstat(path, 1, &x);
	free(path);

	if(err < 0)
		return err;
	ustat2linuxstat(&x, (struct linux_stat*)st);
	return err;
}

int sys_linux_stat64(char *path, void *st)
{
	int err;
	Ustat x;

	trace("sys_linux_stat64(%s, %p)", path, st);

	if((path = fsfullpath(path)) == nil)
		return -EFAULT;
	err = fsstat(path, 0, &x);
	free(path);

	if(err < 0)
		return err;
	ustat2linuxstat64(&x, (struct linux_stat64*)st);
	return err;
}

int sys_linux_lstat64(char *path, void *st)
{
	int err;
	Ustat x;

	trace("sys_linux_lstat64(%s, %p)", path, st);

	if((path = fsfullpath(path)) == nil)
		return -EFAULT;
	err = fsstat(path, 1, &x);
	free(path);

	if(err < 0)
		return err;
	ustat2linuxstat64(&x, (struct linux_stat64*)st);
	return err;
}

int sys_linux_fstat(int fd, void *st)
{
	int err;
	Ustat x;

	trace("sys_linux_fstat(%d, %p)", fd, st);

	err = ufstat(fd, &x);
	if(err < 0)
		return err;
	ustat2linuxstat(&x, (struct linux_stat*)st);
	return err;
}

int sys_linux_fstat64(int fd, void *st)
{
	int err;
	Ustat x;

	trace("sys_linux_fstat64(%d, %p)", fd, st);

	err = ufstat(fd, &x);
	if(err < 0)
		return err;
	ustat2linuxstat64(&x, (struct linux_stat64*)st);
	return err;
}

static int
getdents(int fd, void *buf, int len, int (*fconv)(Udirent *, void *, int, int))
{
	Ufile *f;
	Udirent *t, *x;
	uchar *p, *e;
	int o, r, err;

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	o = 0;
	p = buf;
	e = p + len;
	t = f->rdaux;
	if(t == nil || f->off == 0){
		f->rdaux = nil;
		while(x = t){
			t = t->next;
			free(x);
		}
		if((err = devtab[f->dev]->readdir(f, &t)) <= 0){
			putfile(f);
			return err;
		}
		f->rdaux = t;
	}
	for(; t; t=t->next){
		/* just calculate size */
		r = fconv(t, nil, 0, e - p);
		if(r <= 0)
			break;
		if(o >= f->off){
			/* convert */
			f->off = o + r;
			r = fconv(t, p, t->next ? f->off : 0, e - p);
			p += r;
		}
		o += r;
	}
	putfile(f);
	return p - (uchar*)buf;
}

Udirent*
newdirent(char *path, char *name, int mode)
{
	Udirent *d;
	int nlen;
	char *s;

	nlen = strlen(name);
	d = kmallocz(sizeof(*d) + nlen + 1, 1);
	d->mode = mode;
	strcpy(d->name, name);
	s = allocpath(path, nil, d->name);
	d->ino = hashpath(s);
	free(s);

	return d;
}

struct linux_dirent {
	long		d_ino;
	long		d_off;
	ushort	d_reclen;
	char		d_name[];
};

static int
udirent2linux(Udirent *u, void *d, int off, int left)
{
	int n;
	struct linux_dirent *e = d;

	n = sizeof(*e) + strlen(u->name) + 1;
	if(n > left)
		return 0;
	if(e){
		e->d_ino = u->ino & 0xFFFFFFFF;
		e->d_off = off;
		e->d_reclen = n;
		strcpy(e->d_name, u->name);
	}
	return n;
}

struct linux_dirent64 {
	uvlong	d_ino;
	vlong	d_off;
	ushort	d_reclen;
	uchar	d_type;
	char		d_name[];
};

static int
udirent2linux64(Udirent *u, void *d, int off, int left)
{
	int n;
	struct linux_dirent64 *e = d;

	n = sizeof(*e) + strlen(u->name) + 1;
	if(n > left)
		return 0;
	if(e){
		e->d_ino = u->ino;
		e->d_off = off;
		e->d_reclen = n;
		e->d_type = (u->mode>>12)&15;
		strcpy(e->d_name, u->name);
	}
	return n;
}

int sys_linux_getdents(int fd, void *buf, int nbuf)
{
	trace("sys_linux_getdents(%d, %p, %x)", fd, buf, nbuf);

	return getdents(fd, buf, nbuf, udirent2linux);
}

int sys_linux_getdents64(int fd, void *buf, int nbuf)
{
	trace("sys_linux_getdents64(%d, %p, %x)", fd, buf, nbuf);

	return getdents(fd, buf, nbuf, udirent2linux64);
}

struct  linux_statfs  { 
	long f_type; 
	long f_bsize; 
	long f_blocks; 
	long f_bfree; 
	long f_bavail; 
	long f_files; 
	long f_ffree;
	long f_fsid[2]; 
	long f_namelen; 
	long f_frsize; 
	long f_spare[5]; 
}; 

int sys_statfs(char *name, void *pstatfs)
{
	struct linux_statfs *s = pstatfs;

	trace("sys_statfs(%s, %p)", name, s);

	if((s == nil) || (name == nil))
		return -EINVAL;

	memset(s, 0, sizeof(*s));

	s->f_namelen = 256;
	s->f_bsize = 4096;
	s->f_blocks = 0x80000000;
	s->f_bavail = s->f_bfree = 0x80000000;
	s->f_files = s->f_ffree = 0x40000000;

	if(strncmp(name, "/dev/pts", 8) == 0){
		s->f_type = 0x1cd1;
		return 0;
	}

	memmove(&s->f_type, "PLN9", 4);
	memmove(s->f_fsid, "PLAN9_FS", 8);

	return 0;
}

int
sys_getxattr(char *path, char *name, void *value, int size)
{
	trace("sys_getxattr(%s, %s, %p, %x)", path, name, value, size);

	return -EOPNOTSUPP;
}

int
sys_lgetxattr(char *path, char *name, void *value, int size)
{
	trace("sys_lgetxattr(%s, %s, %p, %x)", path, name, value, size);

	return -EOPNOTSUPP;
}

int
sys_fgetxattr(int fd, char *name, void *value, int size)
{
	Ufile *f;
	int err;

	trace("sys_fgetxattr(%d, %s, %p, %x)", fd, name, value, size);

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	err = -EOPNOTSUPP;
	putfile(f);

	return err;
}

int
sys_setxattr(char *path, char *name, void *value, int flags, int size)
{
	trace("sys_setxattr(%s, %s, %p, %x, %x)", path, name, value, flags, size);

	return -EOPNOTSUPP;
}

int
sys_lsetxattr(char *path, char *name, void *value, int flags, int size)
{
	trace("sys_lsetxattr(%s, %s, %p, %x, %x)", path, name, value, flags, size);

	return -EOPNOTSUPP;
}

int
sys_fsetxattr(int fd, char *name, void *value, int size, int flags)
{
	Ufile *f;
	int err;

	trace("sys_fsetxattr(%d, %s, %p, %x, %x)", fd, name, value, flags, size);

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	err = -EOPNOTSUPP;
	putfile(f);
	return err;
}
