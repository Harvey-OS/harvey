/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __UTSNAME
#define __UTSNAME
#pragma lib "/$M/lib/ape/libap.a"

struct utsname {
	char	*sysname;
	char	*nodename;
	char	*release;
	char	*version;
	char	*machine;
};

#ifdef __cplusplus
extern "C" {
#endif

int uname(struct utsname *);

#ifdef __cplusplus
}
#endif

#endif
