#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

enum {
	RLIMIT_CPU = 0,
	RLIMIT_FSIZE,
	RLIMIT_DATA,
	RLIMIT_STACK,
	RLIMIT_CORE,
	RLIMIT_RSS,
	RLIMIT_NPROC,
	RLIMIT_NOFILE,
	RLIMIT_MEMLOCK,
	RLIMIT_AS,
	RLIMIT_LOCKS,
	RLIMIT_SIGPENDING,
	RLIMIT_MSGQUEUE,

	RLIM_NLIMITS,

	RLIM_INFINITY		= ~0UL,
};

struct linux_rlimit
{
	uvlong	rlim_cur;
	uvlong	rlim_max;
};

int sys_getrlimit(long resource, void *rlim)
{
	struct linux_rlimit *r = rlim;

	trace("sys_getrlimit(%ld, %p)", resource, rlim);

	if(resource >= RLIM_NLIMITS)
		return -EINVAL;
	if(rlim == nil)
		return -EFAULT;

	r->rlim_cur = RLIM_INFINITY;
	r->rlim_max = RLIM_INFINITY;

	switch(resource){
	case RLIMIT_STACK:
		r->rlim_cur = USTACK;
		r->rlim_max = USTACK;
		break;
	case RLIMIT_CORE:
		r->rlim_cur = 0;
		break;
	case RLIMIT_NPROC:
		r->rlim_cur = MAXPROC;
		r->rlim_max = MAXPROC;
		break;
	case RLIMIT_NOFILE:
		r->rlim_cur = MAXFD;
		r->rlim_max = MAXFD;
		break;
	}
	return 0;
}

int sys_setrlimit(long resource, void *rlim)
{
	trace("sys_setrlimit(%ld, %p)", resource, rlim);

	if(resource >= RLIM_NLIMITS)
		return -EINVAL;
	if(rlim == nil)
		return -EFAULT;

	return -EPERM;
}

