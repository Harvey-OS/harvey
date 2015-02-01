/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
