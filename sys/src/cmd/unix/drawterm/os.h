#if defined(LINUX) || defined(IRIX) || defined(SOLARIS) || defined(OSF1)
#	include "unix.h"
#elif defined(WINDOWS)
#	include "windows.h"
#else
#	error "Define an OS"
#endif
