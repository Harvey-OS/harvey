/* Copyright (C) 1992 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gp_sysv.c */
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

extern long timezone;		/* doesn't seem to be declared anywhere */

/* rename */
int
rename(const char *a, const char *b)
{	if  (access(a, 0) == -1 )
		return(-1);
	unlink(b);
	if ( link(a, b) == -1 )
		return(-1);
	if ( unlink(a) == -1 )
	{	unlink(b);		/* ??? */
		return(-1);
	}
	return(0);
}

/* gettimeofday */
#ifndef HZ
#  define	HZ	100		/* see sys/param.h */
#endif
int
gettimeofday(struct timeval *tvp, struct timezone *tzp)
{	struct tms tms;
	static long offset = 0;
	long ticks;

	if (!offset)
	{	time(&offset);
		/* call localtime to set the timezone variable */
		localtime(&offset);
		offset -= (times(&tms) / HZ);
	}
	ticks = times(&tms);
	tvp->tv_sec = ticks/HZ + offset;
	tvp->tv_usec = (ticks % HZ) * (1000*1000/HZ);
	tzp->tz_minuteswest = timezone/60;
	return 0;
}
