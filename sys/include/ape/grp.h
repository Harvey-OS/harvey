/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __GRP
#define __GRP
#ifndef _POSIX_SOURCE
   This header file is not defined in pure ANSI
#endif
#pragma lib "/$M/lib/ape/libap.a"
#include <sys/types.h>

struct	group {
	char	*gr_name;
	gid_t	gr_gid;
	char	**gr_mem;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct group *getgrgid(gid_t);
extern struct group *getgrnam(const char *);

#ifdef __cplusplus
}
#endif

#endif
