#include "lib.h"
#include "sys9.h"
#include <string.h>
#include <errno.h>

/* make this global, so programs can look at it if errno is unilluminating */

char _plan9err[ERRLEN];

static struct errmap {
	int	errno;
	char	*ename;
} map[] = {
	/* from power errstr.h */
	{EINVAL,	"never mind"},
	{ENFILE,	"no free file descriptors"},
	{EBADF,		"fd out of range or not open"},
	{EPERM,		"inappropriate use of fd"},
	{EINVAL,	"bad arg in system call"},
	{ENOENT,	"file does not exist"},
	{EINVAL,	"file name syntax"},
	{EINVAL,	"bad character in file name"},
	{ENXIO,		"unknown device in # filename"},
	{ENOEXEC,	"a.out header invalid"},
	{EIO,		"i/o error in demand load"},
	{EPERM,		"permission denied"},
	{ENOTDIR,	"not a directory"},
	{ECHILD,	"no living children"},
	{EBUSY,		"no free segments"},
	{EINVAL,	"inconsistent mount"},
	{EBUSY,		"mount table full"},
	{EBUSY,		"no free mount devices"},
	{EIO,		"mounted device shut down"},
	{EBUSY,		"device or object already in use"},
	{EIO,		"i/o error"},
	{EISDIR,	"file is a directory"},
	{EIO,		"directory read not quantized"},
	{EFAULT,	"illegal segment addresses or size"},
	{EBUSY,		"no free environment resources"},
	{ESRCH,		"process exited"},
	{EPERM,		"mounted directory forbids creation"},
	{EPERM,		"attempt to union with non-mounted directory"},
	{EINVAL,	"inconsistent mount"},
	{EBUSY,		"no free server slots"},
	{EBUSY, 	"no free stream queues"},
	{EINVAL,	"illegal line discipline"},
	{EBUSY,		"no free stream heads"},
	{EIO,		"read or write too large"},
	{EIO,		"read or write too small"},
	{EPIPE,		"write to hungup stream"},
	{EDOM,		"illegal network address"},
	{EINVAL,	"bad interface or no free interface slots"},
	{EBUSY,		"no free devices"},
	{EINVAL,	"bad process or stream control request"},
	{EINTR,		"note overflow"},
	{EINTR,		"interrupted"},
	{EBUSY,		"datakit destination busy"},
	{EBUSY,		"datakit controller busy"},
	{EIO,		"datakit destination out to lunch"},
	{EIO,		"datakit controller out to lunch"},
	{EIO,		"datakit destination rejected call"},
	{EIO,		"short message"},
	{EIO,		"format error or mismatch in message"},
	{EIO,		"read count greater than requested"},
	{EINVAL,	"listening on an unannounced network connection"},
	{ENOMEM,	"virtual memory allocation failed"},
	{EBUSY,		"out of async stream modules"},
	{EBUSY,		"out of pipes"},
	{EIO,		"message is too big for protocol"},
	{EBUSY,		"network port not available"},
	{EBUSY,		"network device is busy or allocated"},
	{EINVAL,	"network address not found"},
	{EINVAL,	"network unreachable"},
	{EIO,		"connection timed out"},
	{EINVAL,	"connection refused"},
	{ENOSYS,	"network protocol not supported"},
	{ENOSYS,	"operation not supported by network protocol"},
	{ESPIPE,	"seek on a stream"},
	{EIO,		"ken has implemented datakit"},

	/* from ken */
	{EINVAL,	"attach -- bad specifier"},
	{EBADF,		"unknown fid"},
	{EINVAL,	"bad character in directory name"},
	{EBADF,		"read/write -- on non open fid"},
	{EIO,		"read/write -- count too big"},
	{EIO,		"phase error -- directory entry not allocated"},
	{EIO,		"phase error -- qid does not match"},
	{EACCES,	"access permission denied"},
	{ENOENT,	"directory entry not found"},
	{EINVAL,	"open/create -- unknown mode"},
	{ENOTDIR,	"walk -- in a non-directory"},
	{ENOTDIR,	"create -- in a non-directory"},
	{EIO,		"phase error -- cannot happen"},
	{EEXIST,	"create -- file exists"},
	{EINVAL,	"create -- . and .. illegal names"},
	{ENOTEMPTY,	"remove -- directory not empty"},
	{EINVAL,	"attach -- privileged user"},
	{EPERM,		"wstat -- not owner"},
	{EPERM,		"wstat -- not in group"},
	{EINVAL,	"create/wstat -- bad character in file name"},
	{EBUSY,		"walk -- too many (system wide)"},
	{EROFS,		"file system read only"},
	{ENOSPC,	"file system full"},
	{EINVAL,	"read/write -- offset negative"},
	{EBUSY,		"open/create -- file is locked"},
	{EBUSY,		"close/read/write -- lock is broken"},
};

#define NERRMAP	(sizeof(map)/sizeof(struct errmap))

/* convert last system call error to an errno */
void
_syserrno(void)
{
	int i;

	if(_ERRSTR(_plan9err) < 0)
		errno = EIO;
	for(i = 0; i < NERRMAP; i++)
		if(strcmp(_plan9err, map[i].ename) == 0)
			break;
	errno = (i < NERRMAP)? map[i].errno : EIO;
}
