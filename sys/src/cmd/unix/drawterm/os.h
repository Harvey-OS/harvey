#if defined(LINUX) || defined(IRIX) || defined(SOLARIS) || defined(OSF1) || defined(FREEBSD)
#	include "unix.h"
#	ifdef MACOSX
#		define abort dt_abort	/* avoid conflicts with libSystem */
#		define panic dt_panic
#	endif
#elif defined(WINDOWS)
#	include "windows.h"
#else
#	error "Define an OS"
#endif
