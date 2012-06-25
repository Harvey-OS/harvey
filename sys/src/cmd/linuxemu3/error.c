#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

int
Efmt(Fmt *f)
{
	static char *t[] = {
	[EPERM]				"EPERM",
	[ENOENT]			"ENOENT",
	[ESRCH]				"ESRCH",
	[EINTR]				"EINTR",
	[EIO]				"EIO",
	[ENXIO]				"ENXIO",
	[E2BIG]				"E2BIG",
	[ENOEXEC]			"ENOEXEC",
	[EBADF]				"EBADF",
	[ECHILD]			"ECHILD",
	[EAGAIN]			"EAGAIN",
	[ENOMEM]			"ENOMEM",
	[EACCES]			"EACCES",
	[EFAULT]			"EFAULT",
	[ENOTBLK]			"ENOTBLK",
	[EBUSY]				"EBUSY",
	[EEXIST]			"EEXIST",
	[EXDEV]				"EXDEV",
	[ENODEV]			"ENODEV",
	[ENOTDIR]			"ENOTDIR",
	[EISDIR]			"EISDIR",
	[EINVAL]			"EINVAL",
	[ENFILE]			"ENFILE",
	[EMFILE]			"EMFILE",
	[ENOTTY]			"ENOTTY",
	[ETXTBSY]			"ETXTBSY",
	[EFBIG]				"EFBIG",
	[ENOSPC]			"ENOSPC",
	[ESPIPE]			"ESPIPE",
	[EROFS]				"EROFS",
	[EMLINK]			"EMLINK",
	[EPIPE]				"EPIPE",
	[EDOM]				"EDOM",
	[ERANGE]			"ERANGE",
	[EDEADLK]			"EDEADLK",
	[ENAMETOOLONG]		"ENAMETOOLONG",
	[ENOLCK]			"ENOLCK",
	[ENOSYS]			"ENOSYS",
	[ENOTEMPTY]			"ENOTEMPTY",
	[ELOOP]				"ELOOP",
	[ENOMSG]			"ENOMSG",
	[EIDRM]				"EIDRM",
	[ECHRNG]			"ECHRNG",
	[EL2NSYNC]			"EL2NSYNC",
	[EL3HLT]			"EL3HLT",
	[EL3RST]			"EL3RST",
	[ELNRNG]			"ELNRNG",
	[EUNATCH]			"EUNATCH",
	[ENOCSI]			"ENOCSI",
	[EL2HLT]			"EL2HLT",
	[EBADE]				"EBADE",
	[EBADR]				"EBADR",
	[EXFULL]			"EXFULL",
	[ENOANO]			"ENOANO",
	[EBADRQC]			"EBADRQC",
	[EBADSLT]			"EBADSLT",
	[EBFONT]			"EBFONT",
	[ENOSTR]			"ENOSTR",
	[ENODATA]			"ENODATA",
	[ETIME]				"ETIME",
	[ENOSR]				"ENOSR",
	[ENONET]			"ENONET",
	[ENOPKG]			"ENOPKG",
	[EREMOTE]			"EREMOTE",
	[ENOLINK]			"ENOLINK",
	[EADV]				"EADV",
	[ESRMNT]			"ESRMNT",
	[ECOMM]				"ECOMM",
	[EPROTO]			"EPROTO",
	[EMULTIHOP]			"EMULTIHOP",
	[EDOTDOT]			"EDOTDOT",
	[EBADMSG]			"EBADMSG",
	[EOVERFLOW]			"EOVERFLOW",
	[ENOTUNIQ]			"ENOTUNIQ",
	[EBADFD]			"EBADFD",
	[EREMCHG]			"EREMCHG",
	[ELIBACC]			"ELIBACC",
	[ELIBBAD]			"ELIBBAD",
	[ELIBSCN]			"ELIBSCN",
	[ELIBMAX]			"ELIBMAX",
	[ELIBEXEC]			"ELIBEXEC",
	[EILSEQ]			"EILSEQ",
	[ERESTART]			"ERESTART",
	[ESTRPIPE]			"ESTRPIPE",
	[EUSERS]			"EUSERS",
	[ENOTSOCK]			"ENOTSOCK",
	[EDESTADDRREQ]		"EDESTADDRREQ",
	[EMSGSIZE]			"EMSGSIZE",
	[EPROTOTYPE]		"EPROTOTYPE",
	[ENOPROTOOPT]		"ENOPROTOOPT",
	[EPROTONOSUPPORT]	"EPROTONOSUPPORT",
	[ESOCKTNOSUPPORT]	"ESOCKTNOSUPPORT",
	[EOPNOTSUPP]		"EOPNOTSUPP",
	[EPFNOSUPPORT]		"EPFNOSUPPORT",
	[EAFNOSUPPORT]		"EAFNOSUPPORT",
	[EADDRINUSE]		"EADDRINUSE",
	[EADDRNOTAVAIL]		"EADDRNOTAVAIL",
	[ENETDOWN]			"ENETDOWN",
	[ENETUNREACH]		"ENETUNREACH",
	[ENETRESET]			"ENETRESET",
	[ECONNABORTED]		"ECONNABORTED",
	[ECONNRESET]		"ECONNRESET",
	[ENOBUFS]			"ENOBUFS",
	[EISCONN]			"EISCONN",
	[ENOTCONN]			"ENOTCONN",
	[ESHUTDOWN]			"ESHUTDOWN",
	[ETOOMANYREFS]		"ETOOMANYREFS",
	[ETIMEDOUT]			"ETIMEDOUT",
	[ECONNREFUSED]		"ECONNREFUSED",
	[EHOSTDOWN]			"EHOSTDOWN",
	[EHOSTUNREACH]		"EHOSTUNREACH",
	[EALREADY]			"EALREADY",
	[EINPROGRESS]		"EINPROGRESS",
	[ESTALE]			"ESTALE",
	[EUCLEAN]			"EUCLEAN",
	[ENOTNAM]			"ENOTNAM",
	[ENAVAIL]			"ENAVAIL",
	[EISNAM]			"EISNAM",
	[EREMOTEIO]			"EREMOTEIO",
	[EDQUOT]			"EDQUOT",
	[ENOMEDIUM]			"ENOMEDIUM",
	[EMEDIUMTYPE]		"EMEDIUMTYPE",
	};

	int e;

	e = va_arg(f->args, int);
	if(e >= 0 || -e >= nelem(t))
		return fmtprint(f, "%d", e);
	return fmtprint(f, "%d [%s]", e, t[-e]);
}

int
mkerror(void)
{
	static struct {
		int		num;
		char	*msg;
	} t[] = {
	/* from /sys/src/9/port/errstr.h */
	{EINVAL,			"inconsistent mount"},
	{EINVAL,			"not mounted"},
	{EINVAL,			"not in union"},
	{EIO,				"mount rpc error"},
	{EIO,				"mounted device shut down"},
	{EPERM,				"mounted directory forbids creation"},
	{ENOENT,			"does not exist"},
	{ENXIO,				"unknown device in # filename"},
	{ENOTDIR,			"not a directory"},
	{EISDIR,			"file is a directory"},
	{EINVAL,			"bad character in file name"},
	{EINVAL,			"file name syntax"},
	{EPERM,				"permission denied"},
	{EPERM,				"inappropriate use of fd"},
	{EINVAL,			"bad arg in system call"},
	{EBUSY,				"device or object already in use"},
	{EIO,				"i/o error"},
	{EIO,				"read or write too large"},
	{EIO,				"read or write too small"},
	{EADDRINUSE,		"network port not available"},
	{ESHUTDOWN,			"write to hungup stream"},
	{ESHUTDOWN,			"i/o on hungup channel"},
	{EINVAL,			"bad process or channel control request"},
	{EBUSY,				"no free devices"},
	{ESRCH,				"process exited"},
	{ECHILD,			"no living children"},
	{EIO,				"i/o error in demand load"},
	{ENOMEM,			"virtual memory allocation failed"},
	{EBADF,				"fd out of range or not open"},
	{EMFILE,			"no free file descriptors"},
	{ESPIPE,			"seek on a stream"},
	{ENOEXEC,			"exec header invalid"},
	{ETIMEDOUT,			"connection timed out"},
	{ECONNREFUSED,		"connection refused"},
	{ECONNREFUSED,		"connection in use"},
	{ERESTART,			"interrupted"},
	{ENOMEM,			"kernel allocate failed"},
	{EINVAL,			"segments overlap"},
	{EIO,				"i/o count too small"},
	{EINVAL,			"bad attach specifier"},

	/* from exhausted() calls in kernel */
	{ENFILE,			"no free file descriptors"},
	{EBUSY,				"no free mount devices"},
	{EBUSY,				"no free mount rpc buffer"},
	{EBUSY,				"no free segments"},
	{ENOMEM,			"no free memory"},
	{ENOBUFS,			"no free Blocks"},
	{EBUSY,				"no free routes"},

	/* from ken */
	{EINVAL,			"attach -- bad specifier"},
	{EBADF,				"unknown fid"},
	{EINVAL,			"bad character in directory name"},
	{EBADF,				"read/write -- on non open fid"},
	{EIO,				"read/write -- count too big"},
	{EIO,				"phase error -- directory entry not allocated"},
	{EIO,				"phase error -- qid does not match"},
	{EACCES,			"access permission denied"},
	{ENOENT,			"directory entry not found"},
	{EINVAL,			"open/create -- unknown mode"},
	{ENOTDIR,			"walk -- in a non-directory"},
	{ENOTDIR,			"create -- in a non-directory"},
	{EIO,				"phase error -- cannot happen"},
	{EEXIST,			"create -- file exists"},
	{EINVAL,			"create -- . and .. illegal names"},
	{ENOTEMPTY,			"remove -- directory not empty"},
	{EINVAL,			"attach -- privileged user"},
	{EPERM,				"wstat -- not owner"},
	{EPERM,				"wstat -- not in group"},
	{EINVAL,			"create/wstat -- bad character in file name"},
	{EBUSY,				"walk -- too many (system wide)"},
	{EROFS,				"file system read only"},
	{ENOSPC,			"file system full"},
	{EINVAL,			"read/write -- offset negative"},
	{EBUSY,				"open/create -- file is locked"},
	{EBUSY,				"close/read/write -- lock is broken"},

	/* from sockets */
	{ENOTSOCK,			"not a socket"},
	{EPROTONOSUPPORT,	"protocol not supported"},
	{ECONNREFUSED,		"connection refused"},
	{EAFNOSUPPORT,		"address family not supported"},
	{ENOBUFS,			"insufficient buffer space"},
	{EOPNOTSUPP,		"operation not supported"},
	{EADDRINUSE,		"address in use"},

	/* other */
	{EEXIST,			"file already exists"},
	{EEXIST,			"is a directory"},
	{ENOTEMPTY,			"directory not empty"},
	};

	int r, i;
	char msg[ERRMAX];

	rerrstr(msg, sizeof(msg));

	r = -EIO;
	for(i=0; i<nelem(t); i++){
		if(strstr(msg, t[i].msg)){
			r = -t[i].num;
			break;
		}
	}

	trace("mkerror(%s): %E", msg, r);
	return r;
}

int sys_nosys(void)
{
	trace("syscall %s not implemented", current->syscall);
	return -ENOSYS;
}
