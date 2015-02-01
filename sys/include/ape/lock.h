/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#if !defined(_RESEARCH_SOURCE) && !defined(_PLAN9_SOURCE)
   This header file is an extension of ANSI/POSIX
#endif

#ifndef __LOCK_H
#define __LOCK_H
#pragma lib "/$M/lib/ape/libap.a"

#include <u.h>

typedef struct
{
	long	key;
	long	sem;
} Lock;

#ifdef __cplusplus
extern "C" {
#endif

extern	void	lock(Lock*);
extern	void	unlock(Lock*);
extern	int	canlock(Lock*);
extern	int	tas(int*);

#ifdef __cplusplus
}
#endif

#endif
