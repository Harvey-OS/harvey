/* Copyright (C) 1989 - 1995, 2001-2003 artofcode LLC.  All rights reserved.
  
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

/* $Id: gp_mac.c,v 1.17 2004/12/08 05:04:29 giles Exp $ */
/* platform-specific routines for MacOS */

#ifndef __CARBON__
#include <Palettes.h>
#include <Aliases.h>
#include <Quickdraw.h>
#include <QDOffscreen.h>
#include <AppleEvents.h>
#include <Fonts.h>
#include <Controls.h>
#include <Script.h>
#include <Timer.h>
#include <Time.h>
#include <Folders.h>
#include <Resources.h>
#include <Sound.h>
#include <ToolUtils.h>
#include <Menus.h>
#include <LowMem.h>
#include <Devices.h>
#include <Scrap.h>
#include <StringCompare.h>
#include <Fonts.h>
#include <FixMath.h>
#include <Resources.h>
#else
#include <Carbon.h>
#endif

#include <stdlib.h>
#include "math_.h"
#include "string_.h"
#include "time_.h"
#include "memory_.h"
#include "string_.h"

#include "gx.h"
#include "gp.h"
#include "gsdll.h"
#include "gpcheck.h"
#include "gp_mac.h"
#include "gdebug.h"
/*#include "gpgetenv.h"*/
#include "gsexit.h"

HWND hwndtext;	/* used as identifier for the dll instance */


char *
mygetenv(const char * env)
{
	return (NULL);	
}

void
gp_init (void)
{
	extern char    *gs_lib_default_path;
	extern char    *gs_init_file;
	
#if 0
	/*...Initialize Ghostscript's default library paths and initialization file...*/

	{
		int			i;
		char	  **p;


		for (i = iGSLibPathStr, p = &gs_lib_default_path;
			 i <= iGSInitFileStr;
			 i++, p = &gs_init_file)
		{
			GetIndString (string, MACSTRS_RES_ID, i);
			(void) PtoCstr (string);
			*p = malloc ((size_t) (strlen ((char *) string) + 1));
			strcpy (*p, (char *) string);
		}
	}
#endif
}

void
gp_exit(int exit_status, int code)
{
}

/* Exit the program. */
void
gp_do_exit(int exit_status)
{
    exit(exit_status);
}

/* gettimeofday */
#ifndef HZ
#  define	HZ	100	/* see sys/param.h */
#endif
int
gettimeofday(struct timeval *tvp)
{
    struct tm tms;
    static long offset = 0;
    long ticks;

    if (!offset) {
	time(&offset);
	offset -= (time((long *)&tms) / HZ);
    }
    ticks = time((long *)&tms);
    tvp->tv_sec = ticks / HZ + offset;
    tvp->tv_usec = (ticks % HZ) * (1000 * 1000 / HZ);
    return 0;
}

/* ------ Date and time ------ */

#define gettimeofday_no_timezone 0

/* Read the current time (in seconds since Jan. 1, 1970) */
/* and fraction (in nanoseconds). */
void
gp_get_realtime(long *pdt)
{
    struct timeval tp;

	if (gettimeofday(&tp) == -1) {
	    lprintf("Ghostscript: gettimeofday failed!\n");
	    gs_abort(NULL);
	}

    /* tp.tv_sec is #secs since Jan 1, 1970 */
    pdt[0] = tp.tv_sec;

    /* Some Unix systems (e.g., Interactive 3.2 r3.0) return garbage */
    /* in tp.tv_usec.  Try to filter out the worst of it here. */
    pdt[1] = tp.tv_usec >= 0 && tp.tv_usec < 1000000 ? tp.tv_usec * 1000 : 0;
	
#ifdef DEBUG_CLOCK
    printf("tp.tv_sec = %d  tp.tv_usec = %d  pdt[0] = %ld  pdt[1] = %ld\n",
	   tp.tv_sec, tp.tv_usec, pdt[0], pdt[1]);
#endif
}

/* Read the current user CPU time (in seconds) */
/* and fraction (in nanoseconds).  */
void
gp_get_usertime(long *pdt)
{
    gp_get_realtime(pdt);	/* Use an approximation on other hosts.  */
	pdt[0] -= (char)rand(); // was needed, if used for random generator seed (g3 is too fast)
}


/*
 * Get the string corresponding to an OS error number.
 * If no string is available, return NULL.  The caller may assume
 * the string is allocated statically and permanently.
 */
const char *	gp_strerror(int)
{
	return NULL;
}


/* ------ Date and time ------ */

/* Read the current date (in days since Jan. 1, 1980) */
/* and time (in milliseconds since midnight). */

void
gp_get_clock (long *pdt) {

   gp_get_realtime(pdt);	/* Use an approximation on other hosts.  */
  
}

void
gpp_get_clock (long *pdt)

{
	long				secs;
	DateTimeRec			dateRec;
	static DateTimeRec	baseDateRec = {1980, 1, 1, 0, 0, 0, 1};
	long				pdtmp[2];
	void 				do_get_clock (DateTimeRec *dateRec, long *pdt);


	GetDateTime ((unsigned long *) &secs);
//	SecsondsToDate (secs, &dateRec);

	do_get_clock (&dateRec	  , pdt);
	do_get_clock (&baseDateRec, pdtmp);

	/* If the date is reasonable, subtract the days since Jan. 1, 1980 */

	if (pdtmp[0] < pdt[0])
		pdt[0] -= pdtmp[0];

#ifdef DEBUG_CLOCK
	printf("pdt[0] = %ld  pdt[1] = %ld\n", pdt[0], pdt[1]);
#endif
}

UnsignedWide beginMicroTickCount={0,0};

/* Read the current date (in days since Jan. 1, 1980) */
/* and time (in nanoseconds since midnight). */

void
gpp_get_realtime (long *pdt)
{

    UnsignedWide microTickCount,
    	nMicroTickCount;

	long idate;
	static const int mstart[12] =
	   { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	long				secs;
	DateTimeRec			dateRec;
	static DateTimeRec	baseDateRec = {1980, 1, 1, 0, 0, 0, 1};
	void 				do_get_clock (DateTimeRec *dateRec, long *pdt);


    if ((beginMicroTickCount.lo == 0)&&(beginMicroTickCount.hi == 0) )  {
    	Microseconds(&beginMicroTickCount);
    }
    	

   	Microseconds(&microTickCount);
    
    nMicroTickCount.lo = microTickCount.lo - beginMicroTickCount.lo;
    nMicroTickCount.hi = microTickCount.hi - beginMicroTickCount.hi;
    
	GetDateTime ((unsigned long *) &secs);
	SecondsToDate (secs, &dateRec);


	/* If the date is reasonable, subtract the days since Jan. 1, 1980 */

	idate = ((long) dateRec.year - 1980) * 365 +	/* days per year */
	  	(((long) dateRec.year - 1)/4 - 1979/4) +	/* intervening leap days */
		(1979/100 - ((long) dateRec.year - 1)/100) +
		(((long) dateRec.month - 1)/400 - 1979/400) +
		mstart[dateRec.month - 1] +		/* month is 1-origin */
		dateRec.day - 1;			/* day of month is 1-origin */
	idate += (2 < dateRec.month
		  && (dateRec.year % 4 == 0
		      && (dateRec.year % 100 != 0 || dateRec.year % 400 == 0)));
	pdt[0] = ((idate*24 + dateRec.hour) * 60 + dateRec.minute) * 60 + dateRec.second;
	pdt[1] = nMicroTickCount.lo * 100;

//#define DEBUG_CLOCK 1
#ifdef DEBUG_CLOCK
	fprintf(stderr,"pdt[0] = %ld  pdt[1] = %ld\n", pdt[0], pdt[1]);
	fprintf(stderr,"b hi[0] = %ld  lo[1] = %ld\n",  beginMicroTickCount.hi, beginMicroTickCount.lo);
	fprintf(stderr,"m hi[0] = %ld  lo[1] = %ld\n", microTickCount.hi, microTickCount.lo);
#endif
}


static void
do_get_clock (DateTimeRec *dateRec, long *pdt)

{
long idate;
static const int mstart[12] =
   { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	/* This gets UTC, not local time */
	/* We have no way of knowing the time zone correction */
	idate = ((long) dateRec->year - 1980) * 365 +	/* days per year */
	  	(((long) dateRec->year - 1)/4 - 1979/4) +	/* intervening leap days */
		(1979/100 - ((long) dateRec->year - 1)/100) +
		(((long) dateRec->month - 1)/400 - 1979/400) +
		mstart[dateRec->month - 1] +		/* month is 1-origin */
		dateRec->day - 1;			/* day of month is 1-origin */
	idate += (2 < dateRec->month
		  && (dateRec->year % 4 == 0
		      && (dateRec->year % 100 != 0 || dateRec->year % 400 == 0)));
	pdt[0] = ((idate*24 + dateRec->hour) * 60 + dateRec->minute) * 60 + dateRec->second;
	pdt[1] = 0; //dateRec->milisecond * 1000000;

}

void
gpp_get_usertime(long *pdt)
{
	gp_get_realtime(pdt);	/* Use an approximation for now.  */
}


/* ------ Persistent data cache ------*/

/* insert a buffer under a (type, key) pair */
int gp_cache_insert(int type, byte *key, int keylen, void *buffer, int buflen)
{
    /* not yet implemented */
    return 0;
}

/* look up a (type, key) in the cache */
int gp_cache_query(int type, byte* key, int keylen, void **buffer,
    gp_cache_alloc alloc, void *userdata)
{
    /* not yet implemented */
    return -1;
}


/* ------ Screen management ------ */

/* Initialize the console. */
/* do nothing, we did it in gp_init()! */
void
gp_init_console(void)
{
}


/* Write a string to the console. */

void
gp_console_puts (const char *str, uint size)
{
/*	fwrite (str, 1, size, stdout);*/
	return;
}

const char *
gp_getenv_display(void)
{
	return NULL;
}

