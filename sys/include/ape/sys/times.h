/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __TIMES_H
#define __TIMES_H
#pragma lib "/$M/lib/ape/libap.a"

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long clock_t;
#endif

struct tms {
	clock_t	tms_utime;
	clock_t	tms_stime;
	clock_t	tms_cutime;
	clock_t	tms_cstime;
};

#ifdef __cplusplus
extern "C" {
#endif

clock_t times(struct tms *);

#ifdef __cplusplus
}
#endif

#endif
