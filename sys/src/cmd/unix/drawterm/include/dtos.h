#if defined(linux) || defined(IRIX) || defined(SOLARIS) || defined(OSF1) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__NetBSD__) || defined(__sun) || defined(sun) || defined(__OpenBSD__)
#	include "unix.h"
#	ifdef __APPLE__
#		define panic dt_panic
#	endif
#elif defined(WINDOWS)
#	include "9windows.h"
#	define main mymain
#else
#	error "Define an OS"
#endif

#ifdef IRIX
typedef int socklen_t;
#endif
