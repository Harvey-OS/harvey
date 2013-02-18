#include        "dat.h"
#include        "fns.h"
#include        "error.h"
#include        "kernel.h"

/*
void
_libexits(char*)
{
}
*/

// XXX: I want to make it possible to boot multiple plan9s 
// XXX: make it close to 9vx.
// XXX: but not virtualization and runs on windows and friends.


void
libabort(void)
{
	panic("abort unimplemented");

}

int
libaccess(char*, int)
{
	panic("access unimplemented");
	return -1;

}

long
libalarm(ulong)
{
	panic("alarm unimplemented");
	return -1;

}

int
libawait(char*, int)
{
	panic("await unimplemented");
	return -1;

}

int
libbind(char*, char*, int)
{
	panic("bind unimplemented");
	return -1;

}

int
libbrk(void*)
{
	panic("brk unimplemented");
	return -1;

}

int
libchdir(char*)
{
	panic("chdir unimplemented");
	return -1;
	

}

int
libclose(int fd)
{
	return kclose(fd);

}

int
libcreate(char *path, int mode, ulong perm)
{
	
	return kcreate(path, mode, perm);

}

int
libdup(int, int)
{
	panic("dup unimplemented");
	return -1;
	

}

int
liberrstr(char*, uint)
{
	panic("errstr unimplemented");
	return -1;

}

int
libexec(char*, char*[])
{
	panic("exec unimplemented");
	return -1;

}

int
libexecl(char*, ...)
{
	panic("execl unimplemented");
	return -1;

}

int
libfork(void)
{
	panic("fork unimplemented");
	return -1;

}

int
librfork(int)
{
	return -1;
}

int
libfauth(int fd, char *aname)
{
	return  kfauth(fd, aname);
}

int
libfstat(int, uchar*, int)
{
	panic("fstat unimplemented");
	return -1;

}

int
libfwstat(int fd, uchar *buf, int n)
{
	return kfwstat(fd, buf, n);
}

// XXX: just implement the k[a-z]+ functions and do the macro that way?

int
libfversion(int fd, int msize, char *vers, int arglen)
{
	return kfversion(fd, msize, vers, arglen);
}

int
libmount(int fd, int afd, char *old, int flags, char *spec)
{
	return kmount( fd, afd, old, flags, spec);

}

int
libunmount(char *old, char *new)
{
	return kunmount(old, new);
}

int
libnoted(int)
{
	panic("noted unimplemented");
	return -1;

}

int
libnotify(void(*)(void*, char*))
{
	panic("notify unimplemented");
	return -1;

}

int
libopen(char *name, int mode)
{
	return kopen(name, mode);
}

int
libfd2path(int fd, char *s, int huh)
{
	USED(huh);
	panic("fd2path: still don't understand semantics");
	s = kfd2path(fd);
	// XXX: what are the semantics here?
	if(s == nil)
		return -1;
	return 1;

}

int
libpipe(int *fd)
{
	return kpipe(fd);
}

long
libpread(int, void*, long, vlong)
{
	panic("pread unimplemented");
	return -1;

}

long
libpreadv(int, IOchunk*, int, vlong)
{
	panic("preadv unimplemented");
	return -1;

}

long
libpwrite(int, void*, long, vlong)
{
	panic("pwrite unimplemented");
	return -1;

}

long
libpwritev(int, IOchunk*, int, vlong)
{
	panic("pwritev unimplemented");
	return -1;

}

long
libread(int fd, void *buf, long n)
{
	return kread(fd, buf, n);
}

long
libreadn(int, void*, long)
{
	panic("readn unimplemented");
	return -1;

}

long
libreadv(int, IOchunk*, int)
{
	panic("readv unimplemented");
	return -1;

}

int
libremove(char *path)
{
	return kremove(path);
}

void*
libsbrk(ulong top)
{
	return sbrk(top);

}

long
liboseek(int, long, int)
{
	panic("oseek unimplemented");
	return -1;

}

vlong
libseek(int fd, vlong off, int whence)
{
	return kseek(fd, off, whence);
}

// XXX: why do these fail, need to figure out where they are imported from
/*
void*
libsegattach(int, char*, void*, ulong)
{
}

void*
libsegbrk(void*, void*)
{
}
*/

int
libsegdetach(void*)
{
	panic("segdetach unimplemented");
	return -1;

}

int
libsegflush(void*, ulong)
{
	panic("segflush unimplemented");
	return -1;

}

int
libsegfree(void*, ulong)
{
	panic("segfree unimplemented");
	return -1;

}

int
libsemacquire(long*, int)
{
	panic("semacquire unimplemented");
	return -1;

}

long
libsemrelease(long*, long)
{
	panic("semrelease unimplemented");
	return -1;

}

int
libsleep(long)
{
	panic("sleep unimplemented");
	return -1;

}

int
libstat(char *path, uchar *buf, int n)
{
	return kstat(path, buf, n);
}

int
libtsemacquire(long*, ulong)
{
	panic("tsemacquire unimplemented");
	return -1;

}

Waitmsg*
libwait(void)
{
	panic("wait unimplemented");
	return nil;

}

int
libwaitpid(void)
{
	panic("waitpid unimplemented");
	return -1;

}

long
libwrite(int fd, void *buf, long n)
{
	return kwrite(fd, buf, n);
}

long
libwritev(int, IOchunk*, int)
{
	panic("writev unimplemented");
	return -1;

}

int
libwstat(char *path, uchar *buf, int n)
{
	return kwstat(path, buf, n);

}

void*
librendezvous(void*, void*)
{
	panic("rendezvous unimplemented");
	return nil;

}


Dir*
libdirstat(char *name)
{
	return kdirstat(name);

}

Dir*
libdirfstat(int fd)
{
	return kdirfstat(fd);
}

int
libdirwstat(char *name, Dir *dir)
{
	return kdirwstat(name, dir);

}

int
libdirfwstat(int fd, Dir* dir)
{
	return kdirfwstat(fd, dir);
}

long
libdirread(int fd, Dir** d)
{
	return kdirread(fd, d);
}

void
libnulldir(Dir*)
{
	panic("nulldir unimplemented");

}

long
libdirreadall(int fd, Dir** d)
{
	// XXX: needs to be all of the directories
	return kdirread(fd, d);
}

int
libgetpid(void)
{
	panic("getpid unimplemented");
	return -1;

}

int
libgetppid(void)
{
	panic("getppid unimplemented");
	return -1;

}

void
librerrstr(char*, uint)
{
	panic("rerrstr unimplemented");

}

char*
libsysname(void)
{
	panic("sysname unimplemented");
	return nil;

}

void
libwerrstr(char*, ...)
{
	panic("werrstr unimplemented");

}

void
libsyslog(int cons, char *logname, char *fmt, ...)
{
	USED(cons, logname, fmt);
	panic("syslog unimplemented");

}

void
libsysfatal(char *fmt, ...)
{
	USED(fmt);
	panic("sysfatal unimplemented");
	
}