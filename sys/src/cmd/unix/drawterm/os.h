#if defined(LINUX) || defined(IRIX) || defined(SOLARIS) || defined(OSF1) || defined(FREEBSD)
#	include "unix.h"
#elif defined(WINDOWS)
#	include "windows.h"
#else
#	error "Define an OS"
#endif
