/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gp_unix.c */
/* Unix-specific routines for Ghostscript */
#include "string_.h"
#include "gx.h"
#include "gsexit.h"
#include "gp.h"
#include "time_.h"

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));
/* Because of inconsistent (and sometimes incorrect) header files, */
/* we must omit the argument list. */
extern FILE *popen( /* P2(const char *, const char *) */ );
extern int pclose(P1(FILE *));

/* Do platform-dependent initialization. */
void
gp_init(void)
{
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
}

/* ------ Miscellaneous ------ */

/* Get the string corresponding to an OS error number. */
/* Unix systems support this so inconsistently that we don't attempt */
/* to figure out whether it's available. */
const char *
gp_strerror(int errnum)
{	return NULL;
}

/* ------ Date and time ------ */

/* Read the current date (in days since Jan. 1, 1980) */
/* and time (in milliseconds since midnight). */
void
gp_get_clock(long *pdt)
{	long secs_since_1980;
	struct timeval tp;
	time_t tsec;
	struct tm *tm, *localtime(P1(const time_t *));

#if gettimeofday_no_timezone			/* SVR4 */
	{	if ( gettimeofday(&tp) == -1 )
		  {	lprintf("Ghostscript: gettimeofday failed!\n");
			gs_exit(1);
		  }
		secs_since_1980 = 0;
	}
#else						/* All other systems */
	{	struct timezone tzp;
		if ( gettimeofday(&tp, &tzp) == -1 )
		  {	lprintf("Ghostscript: gettimeofday failed!\n");
			gs_exit(1);
		  }
		/*
		 * Don't adjust for timezone!  That structure returns the
		 * kernel's notion of the timezone, which may be different
		 * from the one specified in the TZ environment variable.
		 * In particular, the tz_minuteswest value is documented to
		 * need adjustment according to the daylight savings time
		 * rules indicated by tz_dsttime.  The latter indicates
		 * only the applicable rules and doesn't answer the
		 * question whether DST currently is in effect or not.
		 */
		/*secs_since_1980 = -(tzp.tz_minuteswest * 60);*/
		secs_since_1980 = 0;
	}
#endif

	/* tp.tv_sec is #secs since Jan 1, 1970 */

	/* subtract off number of seconds in 10 years */
	/* leap seconds are not accounted for */
	secs_since_1980 += tp.tv_sec - (long)(60 * 60 * 24 * 365.25 * 10);

	/* adjust for daylight savings time - assume dst offset is 1 hour */
	tsec = tp.tv_sec;
	tm = localtime(&tsec);
	if ( tm->tm_isdst )
		secs_since_1980 += (60 * 60);

	/* divide secs by #secs/day to get #days (integer division truncates) */
	pdt[0] = secs_since_1980 / (60 * 60 * 24);
	/* modulo + microsecs/1000 gives number of millisecs since midnight */
	pdt[1] = (secs_since_1980 % (60 * 60 * 24)) * 1000;
	/* Some Unix systems (e.g., Interactive 3.2 r3.0) return garbage */
	/* in tp.tv_usec.  Try to filter out the worst of it here. */
	if ( tp.tv_usec >= 0 && tp.tv_usec < 1000000 )
		pdt[1] += tp.tv_usec / 1000;

#ifdef DEBUG_CLOCK
	printf("tp.tv_sec = %d  tp.tv_usec = %d  pdt[0] = %ld  pdt[1] = %ld\n",
		tp.tv_sec, tp.tv_usec, pdt[0], pdt[1]);
#endif
}

/* ------ Screen management ------ */

/* Get the environment variable that specifies the display to use. */
const char *
gp_getenv_display(void)
{	return getenv("DISPLAY");
}

/* ------ Printer accessing ------ */

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* "|command" opens an output pipe. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(char *fname, int binary_mode)
{	return
	  (strlen(fname) == 0 ?
	   gp_open_scratch_file(gp_scratch_file_name_prefix, fname, "w") :
	   fname[0] == '|' ?
	   popen(fname + 1, "w") :
	   fopen(fname, "w"));
}

/* Close the connection to the printer. */
void
gp_close_printer(FILE *pfile, const char *fname)
{	if ( fname[0] == '|' )
		pclose(pfile);
	else
		fclose(pfile);
}
