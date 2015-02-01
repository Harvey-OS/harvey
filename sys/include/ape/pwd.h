/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __PWD
#define __PWD
#ifndef _POSIX_SOURCE
   This header file is not defined in pure ANSI
#endif
#pragma lib "/$M/lib/ape/libap.a"
#include <sys/types.h>

struct passwd {
	char	*pw_name;
	uid_t	pw_uid;
	gid_t	pw_gid;
	char	*pw_dir;
	char	*pw_shell;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct passwd *getpwuid(uid_t);
extern struct passwd *getpwnam(const char *);

#ifdef __cplusplus
}
#endif

#endif
