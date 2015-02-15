/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __UTIME_H
#define __UTIME_H

#pragma lib "/$M/lib/ape/libap.a"

struct utimbuf
{
	time_t actime;
	time_t modtime;
};

#ifdef __cplusplus
extern "C" {
#endif

extern int utime(const char *, const struct utimbuf *);

#ifdef __cplusplus
}
#endif

#endif
