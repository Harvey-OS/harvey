/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __ERROR_H
#define __ERROR_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in pure ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libv.a"

#ifdef __cplusplus
extern "C" {
#endif

extern char *_progname;		/* program name */
extern void _perror(char *);	/* perror but with _progname */

#ifdef __cplusplus
}
#endif

#endif /* _ERROR_H */
