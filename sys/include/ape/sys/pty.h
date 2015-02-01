/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Pty support
 */
#ifndef __SYS_PTY_H__
#define __SYS_PTY_H__

#ifndef _BSD_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif

#pragma lib "/$M/lib/ape/libbsd.a"

char*	ptsname(int);
char*	ptmname(int);

int	_getpty(void);

#endif /* !__SYS_UIO_H__ */
