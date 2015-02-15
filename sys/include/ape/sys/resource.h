/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#ifndef _BSD_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif

struct rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	int32_t	ru_maxrss;		/* max resident set size */
#define	ru_first	ru_ixrss
	int32_t	ru_ixrss;		/* integral shared memory size */
	int32_t	ru_idrss;		/* integral unshared data " */
	int32_t	ru_isrss;		/* integral unshared stack " */
	int32_t	ru_minflt;		/* page reclaims */
	int32_t	ru_majflt;		/* page faults */
	int32_t	ru_nswap;		/* swaps */
	int32_t	ru_inblock;		/* block input operations */
	int32_t	ru_oublock;		/* block output operations */
	int32_t	ru_msgsnd;		/* messages sent */
	int32_t	ru_msgrcv;		/* messages received */
	int32_t	ru_nsignals;		/* signals received */
	int32_t	ru_nvcsw;		/* voluntary context switches */
	int32_t	ru_nivcsw;		/* involuntary " */
#define	ru_last		ru_nivcsw
};

#endif /* !__RESOURCE_H__ */
