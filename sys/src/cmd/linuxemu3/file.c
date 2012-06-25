#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Fd Fd;
typedef struct Fdtab Fdtab;

struct Fd
{
	int		flags;
	Ufile		*file;
};

struct Fdtab
{
	Ref;
	QLock;
	int		lastfd;
	int		nfd;
	Fd		*fd;
};

Ufile*
getfile(Ufile *file)
{
	if(file)
		incref(file);
	return file;
}

void
putfile(Ufile *file)
{
	Udirent *d;

	if(file == nil)
		return;
	if(decref(file))
		return;
	trace("putfile(): closing %p %s", file, file->path);
	if(devtab[file->dev]->close)
		devtab[file->dev]->close(file);
	free(file->path);
	while(d = file->rdaux){
		file->rdaux = d->next;
		free(d);
	}
	free(file);
}

static Fdtab*
newfdtab(void)
{
	Fdtab *tab;

	tab = kmallocz(sizeof(*tab), 1);
	tab->ref = 1;
	tab->lastfd = -1;
	tab->nfd = 0;
	tab->fd = nil;

	return tab;
}

enum {
	CHUNK	= 64,
};

/* assumes tab->lock aquired */
static int
grow1(Fdtab *tab)
{
	if(tab->nfd >= MAXFD)
		return -EMFILE;
	if((tab->nfd % CHUNK) == 0)
		tab->fd = krealloc(tab->fd, sizeof(tab->fd[0]) * (tab->nfd + CHUNK));
	memset(&tab->fd[tab->nfd], 0, sizeof(tab->fd[0]));
	return tab->nfd++;
}

Ufile *procfdgetfile(Uproc *proc, int fd)
{
	Fdtab *tab;
	Ufile *file;

	file = nil;
	if(tab = proc->fdtab){
		qlock(tab);
		if(fd >= 0 && fd < tab->nfd)
			file = getfile(tab->fd[fd].file);
		qunlock(tab);
	}
	return file;
}

Ufile*
fdgetfile(int fd)
{
	return procfdgetfile(current, fd);
}

int
newfd(Ufile *file, int flags)
{
	int fd;
	Fdtab *tab;

	tab = current->fdtab;
	qlock(tab);
	fd = tab->lastfd;
	if((fd >= 0) && (fd < tab->nfd) && (tab->fd[fd].file == nil))
		goto found;
	for(fd=0; fd<tab->nfd; fd++)
		if(tab->fd[fd].file == nil)
			goto found;
	fd = grow1(tab);
found:
	if(fd >= 0){
		tab->fd[fd].file = file;
		tab->fd[fd].flags = flags;
		file = nil;
	}
	qunlock(tab);
	putfile(file);

	return fd;
}

static Fdtab*
getfdtab(Fdtab *tab, int copy)
{
	Fdtab *new;
	int i;

	if(!copy){
		incref(tab);
		return tab;
	}
	qlock(tab);
	new = newfdtab();
	new->lastfd = tab->lastfd;
	new->nfd = tab->nfd;
	new->fd = kmallocz(sizeof(new->fd[0]) * (((tab->nfd+CHUNK-1)/CHUNK)*CHUNK), 1);
	for(i=0; i<new->nfd; i++){
		Ufile *file;

		if((file = tab->fd[i].file) == nil)
			continue;
		incref(file);
		new->fd[i].file = file;
		new->fd[i].flags = tab->fd[i].flags;
	}
	qunlock(tab);
	return new;
}

static void
putfdtab(Fdtab *tab)
{
	int i;

	if(decref(tab))
		return;
	for(i=0; i<tab->nfd; i++){
		Ufile *file;
		if((file = tab->fd[i].file) == nil)
			continue;
		tab->fd[i].file = nil;
		putfile(file);
	}
	free(tab->fd);
	free(tab);
}

int sys_dup2(int old, int new)
{
	Ufile *file;
	Fdtab *tab;
	int err;

	trace("sys_dup2(%d, %d)", old, new);

	tab = current->fdtab;

	if((file = fdgetfile(old)) == nil)
		return -EBADF;
	if(new < 0)
		return newfd(file, 0);
	if(new >= MAXFD)
		return -EBADF;
	qlock(tab);
	while(new >= tab->nfd){
		err = grow1(tab);
		if(err < 0){
			qunlock(tab);
			putfile(file);
			return err;
		}
	}
	if(tab->fd[new].file != nil)
		putfile(tab->fd[new].file);
	tab->fd[new].file = file;
	tab->fd[new].flags &= ~FD_CLOEXEC;
	qunlock(tab);

	return new;
}

int sys_dup(int fd)
{
	return sys_dup2(fd, -1);
}

struct linux_flock
{
	short	l_type;
	short	l_whence;
	ulong	l_start;
	ulong	l_len;
	int		l_pid;
}; 

struct linux_flock64
{
	short	l_type;
	short	l_whence;
	uvlong	l_start;
	uvlong	l_len;
	int		l_pid;
};

enum {
	F_RDLCK,
	F_WRLCK,
	F_UNLCK,
};

int sys_fcntl(int fd, int cmd, int arg)
{
	int ret;
	Ufile *file;
	Fdtab *tab;

	trace("sys_fcntl(%d, %lux, %lux)", fd, (ulong)cmd, (ulong)arg);

	tab = current->fdtab;

	ret = -EBADF;
	if((file = fdgetfile(fd)) == nil)
		goto out;
	ret = -EINVAL;
	switch(cmd){
	default:
		trace("sys_fcntl() cmd %lux not implemented", (ulong)cmd);
		break;

	case F_DUPFD:
		if(arg < 0 || arg >= MAXFD)
			break;
		qlock(tab);
		for(ret=arg; ret<tab->nfd; ret++)
			if(tab->fd[ret].file == nil)
				goto found;
		do {
			if((ret = grow1(tab)) < 0)
				break;
		} while(ret < arg);
found:
		if(ret >= 0){
			tab->fd[ret].file = file;
			tab->fd[ret].flags = tab->fd[fd].flags & ~FD_CLOEXEC;
			file = nil;
		}
		qunlock(tab);
		break;

	case F_GETFD:
	case F_SETFD:
		qlock(tab);
		if(cmd == F_GETFD){
			ret = tab->fd[fd].flags & FD_CLOEXEC;
		} else {
			tab->fd[fd].flags = (arg & FD_CLOEXEC);
			ret = 0;
		}
		qunlock(tab);
		break;

	case F_GETFL:
		ret = file->mode;
		break;
	case F_SETFL:
		trace("sys_fcntl() changing mode from %o to %o", file->mode, arg);
		file->mode = arg;
		ret = 0;
		break;

	case F_GETLK:
		((struct linux_flock*)arg)->l_type = F_UNLCK;
	case F_SETLK:
	case F_SETLKW:
		ret = 0;
		break;

	case F_GETLK64:
		((struct linux_flock64*)arg)->l_type = F_UNLCK;
	case F_SETLK64:
		ret = 0;
		break;
	}
out:
	putfile(file);
	return ret;
}

int sys_close(int fd)
{
	Fdtab *tab;
	Ufile *file;

	trace("sys_close(%d)", fd);

	tab = current->fdtab;
	qlock(tab);
	if(fd >= 0 && fd < tab->nfd){
		if(file = tab->fd[fd].file){
			tab->fd[fd].file = nil;
			tab->lastfd = fd;
			qunlock(tab);

			putfile(file);
			return 0;
		}
	}
	qunlock(tab);
	return -EBADF;
}

int sys_ioctl(int fd, int cmd, void *arg)
{
	Ufile *file;
	int ret;

	trace("sys_ioctl(%d, %lux, %p)", fd, (ulong)cmd, arg);

	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = -ENOTTY;
	if(devtab[file->dev]->ioctl)
		ret = devtab[file->dev]->ioctl(file, cmd, arg);
	putfile(file);
	return ret;
}

int preadfile(Ufile *file, void *buf, int len, vlong off)
{
	if(file->mode & O_NONBLOCK){
		if(devtab[file->dev]->poll != nil){
			if((devtab[file->dev]->poll(file, nil) & POLLIN) == 0){
				trace("readfile(): nonblocking read blocked");

				return -EAGAIN;
			}
		}
	}
	if(devtab[file->dev]->read == nil)
		return 0;
	return devtab[file->dev]->read(file, buf, len, off);
}

int readfile(Ufile *file, void *buf, int len)
{
	int err;

	if((err = preadfile(file, buf, len, file->off)) > 0)
		file->off += err;
	return err;
}

int pwritefile(Ufile *file, void *buf, int len, vlong off)
{
	if(devtab[file->dev]->write == nil)
		return 0;
	if(file->mode & O_APPEND){
		if(devtab[file->dev]->size){
			off = devtab[file->dev]->size(file);
			if(off < 0)
				return (int)off;
		}
	}
	return devtab[file->dev]->write(file, buf, len, off);
}

int writefile(Ufile *file, void *buf, int len)
{
	int err;
	vlong end;

	if(devtab[file->dev]->write == nil)
		return 0;
	if(file->mode & O_APPEND){
		if(devtab[file->dev]->size){
			end = devtab[file->dev]->size(file);
			if(end < 0)
				return (int)end;
			file->off = end;
		}
	}
	if((err = devtab[file->dev]->write(file, buf, len, file->off)) > 0)
		file->off += err;
	return err;
}

int sys_read(int fd, void *buf, int len)
{
	int ret;
	Ufile *file;

	trace("sys_read(%d, %p, %x)", fd, buf, len);
	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = readfile(file, buf, len);
	putfile(file);
	return ret;
}

int sys_write(int fd, void *buf, int len)
{
	Ufile *file;
	int ret;

	trace("sys_write(%d, %p, %x)", fd, buf, len);
	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = writefile(file, buf, len);
	putfile(file);

	return ret;
}

int sys_pread64(int fd, void *buf, int len, ulong off)
{
	Ufile *file;
	int ret;

	trace("sys_pread(%d, %p, %x, %lux)", fd, buf, len, off);
	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = preadfile(file, buf, len, off);
	putfile(file);
	return ret;
}

int sys_pwrite64(int fd, void *buf, int len, ulong off)
{
	Ufile *file;
	int ret;

	trace("sys_pwrite(%d, %p, %x, %lux)", fd, buf, len, off);
	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = pwritefile(file, buf, len, off);
	putfile(file);
	return ret;
}

struct linux_iovec
{
	void		*base;
	ulong	len;
};

int sys_writev(int fd, void *vec, int n)
{
	struct linux_iovec *v = vec;
	int ret, i, w;
	Ufile *file;

	trace("sys_writev(%d, %p, %d)", fd, vec, n);

	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = 0;
	for(i=0; i<n; i++){
		w = writefile(file, v[i].base, v[i].len);
		if(w < 0){
			if(ret == 0)
				ret = w;
			break;
		}
		ret += w;
		if(w < v[i].len)
			break;
	}
	putfile(file);

	return ret;
}

int sys_readv(int fd, void *vec, int n)
{
	struct linux_iovec *v = vec;
	int ret, i, r;
	Ufile *file;

	trace("sys_readv(%d, %p, %d)", fd, vec, n);

	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = 0;
	for(i=0; i<n; i++){
		r = readfile(file, v[i].base, v[i].len);
		if(r < 0){
			if(ret == 0)
				ret = r;
			break;
		}
		ret += r;
		if(r < v[i].len)
			break;
	}
	putfile(file);

	return ret;
}

int seekfile(Ufile *file, vlong off, int whence)
{
	vlong end;

	if(devtab[file->dev]->size == nil)
		return -ESPIPE;

	switch(whence){
	case 0:
		file->off = off;
		return 0;
	case 1:
		file->off += off;
		return 0;
	case 2:
		end = devtab[file->dev]->size(file);
		if(end < 0)
			return end;
		file->off = end + off;
		return 0;
	}

	return -EINVAL;
}

ulong sys_lseek(int fd, ulong off, int whence)
{
	Ufile *file;
	int ret;

	trace("sys_lseek(%d, %lux, %d)", fd, off, whence);

	if((file = fdgetfile(fd)) == nil)
		return (ulong)-EBADF;
	ret = seekfile(file, off, whence);
	if(ret == 0)
		ret = file->off;
	putfile(file);

	return ret;
}

int sys_llseek(int fd, ulong hioff, ulong looff, vlong *res, int whence)
{
	Ufile *file;
	int ret;

	trace("sys_llseek(%d, %lux, %lux, %p, %d)", fd, hioff, looff, res, whence);

	if((file = fdgetfile(fd)) == nil)
		return -EBADF;
	ret = seekfile(file, ((vlong)hioff<<32) | ((vlong)looff), whence);
	if((ret == 0) && res)
		*res = file->off;
	putfile(file);

	return ret;
}

int sys_umask(int umask)
{
	int old;

	trace("sys_umask(%#o)", umask);

	old = current->umask;
	current->umask = (umask & 0777);
	return old;
}

int
chdirfile(Ufile *f)
{
	Ustat s;
	int err;

	trace("chdirfile(%s)", f->path);

	err = -ENOTDIR;
	if(f->path == nil)
		return err;
	if(devtab[f->dev]->fstat == nil)
		return err;
	if((err = devtab[f->dev]->fstat(f, &s)) < 0)
		return err;
	err = -ENOTDIR;
	if((s.mode & ~0777) != S_IFDIR)
		return err;
	free(current->cwd);
	current->cwd = kstrdup(fsrootpath(f->path));
	if(f->dev == ROOTDEV && chdir(f->path) == 0){
		free(current->kcwd);
		current->kcwd = kstrdup(f->path);
	}
	return 0;
}

int
sys_fchdir(int fd)
{
	Ufile *f;
	int err;

	trace("sys_fchdir(%d)", fd);

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	err = chdirfile(f);
	putfile(f);
	return err;
}

int
sys_fchown(int fd, int uid, int gid)
{
	int err;
	Ufile *f;

	trace("sys_fchown(%d, %d, %d)", fd, uid, gid);

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	err = -EPERM;
	if(devtab[f->dev]->fchown)
		err = devtab[f->dev]->fchown(f, uid, gid);
	putfile(f);

	return err;
}

int
sys_fchmod(int fd, int mode)
{
	int err;
	Ufile *f;

	trace("sys_fchmod(%d, %#o)", fd, mode);

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	err = -EPERM;
	if(devtab[f->dev]->fchmod)
		err = devtab[f->dev]->fchmod(f, mode);
	putfile(f);

	return err;
}

int
sys_ftruncate(int fd, ulong size)
{
	int err;
	Ufile *f;

	trace("sys_ftruncate(%d, %lux)", fd, size);

	if((f = fdgetfile(fd)) == nil)
		return -EBADF;
	err = -EPERM;
	if(devtab[f->dev]->ftruncate)
		err = devtab[f->dev]->ftruncate(f, (uvlong)size);
	putfile(f);

	return err;
}

void initfile(void)
{
	current->fdtab = newfdtab();
	current->umask = 022;
}

void exitfile(Uproc *proc)
{
	Fdtab *tab;

	if(tab = proc->fdtab){
		proc->fdtab = nil;
		putfdtab(tab);
	}
}

void clonefile(Uproc *new, int copy)
{
	Fdtab *tab;

	if((tab = current->fdtab) == nil){
		new->fdtab = nil;
		return;
	}
	new->fdtab = getfdtab(tab, copy);
}

void closexfds(void)
{
	Fdtab *tab;
	int i;

	if((tab = current->fdtab) == nil)
		return;
	qlock(tab);
	for(i=0; i<tab->nfd; i++){
		Ufile *f;

		if((f = tab->fd[i].file) == nil)
			continue;
		if((tab->fd[i].flags & FD_CLOEXEC) == 0)
			continue;

		tab->fd[i].file = nil;
		tab->fd[i].flags = 0;

		putfile(f);
	}
	qunlock(tab);
}

int sys_flock(int fd, int cmd)
{
	trace("sys_flock(%d, %d)", fd, cmd);
	return 0;
}

int sys_fsync(int fd)
{
	trace("sys_fsync(%d)", fd);
	return 0;
}

