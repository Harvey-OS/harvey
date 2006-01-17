/* Copyright (C) 1992, 1993, 1994, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_win32.c,v 1.5 2002/10/07 08:28:56 ghostgum Exp $ */
/* Common platform-specific routines for MS-Windows WIN32 */
/* originally hacked from gp_msdos.c by Russell Lang */
#include "malloc_.h"
#include "stdio_.h"
#include "string_.h"		/* for strerror */
#include "gstypes.h"
#include "gsmemory.h"		/* for gp.h */
#include "gserror.h"
#include "gserrors.h"
#include "gp.h"
#include "windows_.h"

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
    SYSTEMTIME st;
    long idate;
    static const int mstart[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    /* This gets UTC, not local time */
    /* We have no way of knowing the time zone correction */
    GetSystemTime(&st);
    idate = (st.wYear - 1980) * 365 +	/* days per year */
	((st.wYear - 1) / 4 - 1979 / 4) +	/* intervening leap days */
	(1979 / 100 - (st.wYear - 1) / 100) +
	((st.wYear - 1) / 400 - 1979 / 400) +
	mstart[st.wMonth - 1] +	/* month is 1-origin */
	st.wDay - 1;		/* day of month is 1-origin */
    idate += (2 < st.wMonth
	      && (st.wYear % 4 == 0
		  && (st.wYear % 100 != 0 || st.wYear % 400 == 0)));
    pdt[0] = ((idate * 24 + st.wHour) * 60 + st.wMinute) * 60 + st.wSecond;
    pdt[1] = st.wMilliseconds * 1000000;
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
#ifdef __DLL__
    if (f == NULL)
	return 1;
#else
    if (f == NULL)
	return 0;
#endif
    if (fileno(f) <= 2)
	return 1;
    return 0;
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
