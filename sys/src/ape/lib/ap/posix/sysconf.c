#include	<stdio.h>
#include	<unistd.h>
#include	<limits.h>
#include	<time.h>
#include	<errno.h>
#include	<sys/limits.h>

long
sysconf(int name)
{
	switch(name)
	{
	case _SC_ARG_MAX:
		return ARG_MAX;
	case _SC_CHILD_MAX:
		return CHILD_MAX;
	case _SC_CLK_TCK:
		return CLOCKS_PER_SEC;
	case _SC_NGROUPS_MAX:
		return NGROUPS_MAX;
	case _SC_OPEN_MAX:
		return OPEN_MAX;
	case _SC_JOB_CONTROL:
#ifdef	_POSIX_JOB_CONTROL
		return _POSIX_JOB_CONTROL;
#else
		return -1;
#endif
	case _SC_SAVED_IDS:
#ifdef	_POSIX_SAVED_IDS
		return _POSIX_SAVED_IDS;
#else
		return -1;
#endif
	case _SC_VERSION:
		return _POSIX_VERSION;
	case _SC_LOGIN_NAME_MAX:
		return L_cuserid;
	}
	errno = EINVAL;
	return -1;
}
