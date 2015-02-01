/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __SELECT_H
#define __SELECT_H
#ifndef _BSD_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/libap.a"

#ifndef _FD_SET_T
#define _FD_SET_T
/* BSD select, and adjunct types and macros */

/* assume 96 fds is sufficient for fdset size */

typedef struct fd_set {
	long fds_bits[3];
} fd_set;

#define FD_SET(n,p)	((p)->fds_bits[(n)>>5] |= (1 << ((n) &0x1f)))
#define FD_CLR(n,p)	((p)->fds_bits[(n)>>5] &= ~(1 << ((n) &0x1f)))
#define FD_ISSET(n,p)	((p)->fds_bits[(n)>>5] & (1 << ((n) &0x1f)))
#define FD_ZERO(p)	((p)->fds_bits[0] =0, (p)->fds_bits[1] =0, (p)->fds_bits[2] =0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int select(int, fd_set*, fd_set*, fd_set*, struct timeval *);

#ifdef __cplusplus
}
#endif

#endif
