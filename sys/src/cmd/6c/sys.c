/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "/sys/src/libc/9syscall/sys.h"

vlong	_sysargs[6*4];
vlong _callsys(void);

/*
 * syscalls
 */

int
getpid(void)
{
	_sysargs[0] = -1;
	return _callsys();
}

long
pread(int fd, void *a, long n, vlong)
{
	_sysargs[0] = PREAD;
	_sysargs[1] = fd;
	_sysargs[2] = (vlong)a;
	_sysargs[3] = n;
	return _callsys();
}

long
pwrite(int fd, void *a, long n, vlong)
{
	_sysargs[0] = PWRITE;
	_sysargs[1] = fd;
	_sysargs[2] = (vlong)a;
	_sysargs[3] = n;
	return _callsys();
}

int
close(int fd)
{
	_sysargs[0] = CLOSE;
	_sysargs[1] = fd;
	return _callsys();
}

int
open(char *name, int mode)
{
	_sysargs[0] = OPEN;
	_sysargs[1] = (vlong)name;
	_sysargs[2] = mode;
	return _callsys();
}

int
create(char *f, int mode, ulong perm)
{
	_sysargs[0] = CREATE;
	_sysargs[1] = (vlong)f;
	_sysargs[2] = mode;
	_sysargs[3] = perm;
	return _callsys();
}

void
_exits(char *s)
{
	_sysargs[0] = EXITS;
	_sysargs[1] = s!=nil? strlen(s): 0;
	_callsys();
}

int
dup(int f, int t)
{
	_sysargs[0] = DUP;
	_sysargs[1] = f;
	_sysargs[2] = t;
	return _callsys();
}

int
errstr(char *buf, uint n)
{
	_sysargs[0] = ERRSTR;
	_sysargs[1] = (vlong)buf;
	_sysargs[2] = n;
	return _callsys();
}

int
brk_(void *a)
{
	_sysargs[0] = BRK_;
	_sysargs[1] = (vlong)a;
	return _callsys();
}

void*
sbrk(ulong n)
{
	_sysargs[0] = -2;
	_sysargs[1] = n;
	return (void*)_callsys();
}
