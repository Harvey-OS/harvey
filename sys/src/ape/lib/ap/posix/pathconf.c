#include	<unistd.h>
#include	<limits.h>
#include	<errno.h>
#include	<sys/limits.h>

long
pathconf(const char *path, int name)
{
#pragma ref path

	switch(name)
	{
	case _PC_LINK_MAX:
		return LINK_MAX;
	case _PC_MAX_CANON:
		return MAX_CANON;
	case _PC_MAX_INPUT:
		return MAX_INPUT;
	case _PC_NAME_MAX:
		return NAME_MAX;
	case _PC_PATH_MAX:
		return PATH_MAX;
	case _PC_PIPE_BUF:
		return PIPE_BUF;
	case _PC_CHOWN_RESTRICTED:
#ifdef _POSIX_CHOWN_RESTRICTED
		return _POSIX_CHOWN_RESTRICTED;
#else
		return -1;
#endif
	case _PC_NO_TRUNC:
#ifdef _POSIX_NO_TRUNC
		return _POSIX_NO_TRUNC;
#else
		return -1;
#endif
	case _PC_VDISABLE:
#ifdef _POSIX_VDISABLE
		return _POSIX_VDISABLE;
#else
		return -1;
#endif
	}
	errno = EINVAL;
	return -1;
}

long
fpathconf(int fd, int name)
{
#pragma ref fd

	return pathconf(0, name);
}

