/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1992 Aladdin Enterprises.  All rights reserved.

  This software is provided AS-IS with no warranty, either express or
  implied.

  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.

  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gp_sysv.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* System V Unix-specific routines for Ghostscript */

/* This file contains a couple of standard Unix library procedures */
/* that a few System V platforms don't provide. */
/* Note that this file is NOT used for SVR4 platforms. */
#include <errno.h>
#include "stdio_.h"
#include "time_.h"
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/param.h>

/* rename */
int
rename(const char* a, const char* b)
{
	if(access(a, 0) == -1)
		return (-1);
	unlink(b);
	if(link(a, b) == -1)
		return (-1);
	if(unlink(a) == -1) {
		unlink(b); /* ??? */
		return (-1);
	}
	return (0);
}

/* gettimeofday */
#ifndef HZ
#define HZ 100 /* see sys/param.h */
#endif
int
gettimeofday(struct timeval* tvp, struct timezone* tzp)
{
	struct tms tms;
	static int32_t offset = 0;
	int32_t ticks;

	if(!offset) {
		time(&offset);
		offset -= (times(&tms) / HZ);
	}
	ticks = times(&tms);
	tvp->tv_sec = ticks / HZ + offset;
	tvp->tv_usec = (ticks % HZ) * (1000 * 1000 / HZ);
	return 0;
}
