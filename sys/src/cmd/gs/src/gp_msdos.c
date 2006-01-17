/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_msdos.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Common platform-specific routines for MS-DOS (any compiler) */
#include "stdio_.h"
#include "string_.h"		/* for strerror */
#include "dos_.h"
#include "gstypes.h"
#include "gsmemory.h"		/* for gp.h */
#include "gp.h"

/* ------ Miscellaneous ------ */

/* Get the string corresponding to an OS error number. */
/* This is compiler-, not OS-, specific, but it is ANSI-standard and */
/* all MS-DOS and MS Windows compilers support it. */
const char *
gp_strerror(int errnum)
{
    return strerror(errnum);
}

/* ------ Date and time ------ */

/* Read the current time (in seconds since Jan. 1, 1980) */
/* and fraction (in nanoseconds). */
void
gp_get_realtime(long *pdt)
{
    union REGS osdate, ostime;
    long idate;
    static const int mstart[12] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    osdate.h.ah = 0x2a;		/* get date */
    intdos(&osdate, &osdate);
#define da_year rshort.cx
#define da_mon h.dh
#define da_day h.dl
    ostime.h.ah = 0x2c;		/* get time */
    intdos(&ostime, &ostime);
#define ti_hour h.ch
#define ti_min h.cl
#define ti_sec h.dh
#define ti_hund h.dl
    idate = (long)osdate.da_year * 365 +
	(			/* intervening leap days */
	    ((osdate.da_year + 1979) / 4 - 1979 / 4) +
	    (1979 / 100 - (osdate.da_year + 1979) / 100) +
	    ((osdate.da_year + 1979) / 400 - 1979 / 400) +
	    mstart[osdate.da_mon - 1] +		/* month is 1-origin */
	    osdate.da_day - 1);	/* day of month is 1-origin */
    idate += (2 < osdate.da_mon
	      && (osdate.da_year % 4 == 0
		  && ((osdate.da_year + 1980) % 100 != 0
		      || (osdate.da_year + 1980) % 400 == 0)));
    pdt[0] =
	((idate * 24 + ostime.ti_hour) * 60 + ostime.ti_min) * 60 +
	ostime.ti_sec;
    pdt[1] = ostime.ti_hund * 10000000;
}

/* Read the current user CPU time (in seconds) */
/* and fraction (in nanoseconds).  */
void
gp_get_usertime(long *pdt)
{
    gp_get_realtime(pdt);	/* Use an approximation for now.  */
}

/* ------ Console management ------ */

/* Answer whether a given file is the console (input or output). */
/* This is not a standard gp procedure, */
/* but the MS Windows configuration needs it, */
/* and other MS-DOS configurations might need it someday. */
int
gp_file_is_console(FILE * f)
{
    union REGS regs;

#ifdef __DLL__
    if (f == NULL)
	return 1;
#else
    if (f == NULL)
	return 0;
#endif
    regs.h.ah = 0x44;		/* ioctl */
    regs.h.al = 0;		/* get device info */
    regs.rshort.bx = fileno(f);
    intdos(&regs, &regs);
    return ((regs.h.dl & 0x80) != 0 && (regs.h.dl & 3) != 0);
}

/* ------ Screen management ------ */

/* Get the environment variable that specifies the display to use. */
const char *
gp_getenv_display(void)
{
    return NULL;
}

/* ------ File names ------ */

/* Define the default scratch file name prefix. */
const char gp_scratch_file_name_prefix[] = "_temp_";

/* Define the name of the null output file. */
const char gp_null_file_name[] = "nul";

/* Define the name that designates the current directory. */
const char gp_current_directory_name[] = ".";
