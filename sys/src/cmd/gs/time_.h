/* Copyright (C) 1991, 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* time_.h */
/* Generic substitute for Unix sys/time.h */

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

/* The location (or existence) of certain system headers is */
/* environment-dependent. We detect this in the makefile */
/* and conditionally define switches in gconfig_.h. */
#include "gconfig_.h"

/* Some System V environments don't include sys/time.h. */
/* The SYSTIME_H switch in gconfig_.h reflects this. */
#ifdef Plan9
#  include <time.h>
#  include <sys/time.h>
#else
#ifdef SYSTIME_H
#  include <sys/time.h>
#  if defined(M_UNIX)	/* SCO needs both time.h and sys/time.h! */
#    include <time.h>
#  endif
#else
#  include <time.h>
struct timeval {
	long tv_sec, tv_usec;
};
struct timezone {
	int tz_minuteswest, tz_dsttime;
};
#endif
#endif

#if defined(ultrix) && defined(mips)
/* Apparently some versions of Ultrix for the DECstation include */
/* time_t in sys/time.h, and some don't.  If you get errors */
/* compiling gp_unix.c, uncomment the next line. */
/*	typedef	int	time_t;	*/
#endif

/* In SVR4 (but not other System V implementations), */
/* gettimeofday doesn't take a timezone argument. */
#ifdef SVR4
#  define gettimeofday_no_timezone 1
#else
#  define gettimeofday_no_timezone 0
#endif
