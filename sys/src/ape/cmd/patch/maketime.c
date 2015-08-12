/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Convert struct partime into time_t.  */

/* Copyright 1992, 1993, 1994, 1995, 1997 Paul Eggert
   Distributed under license by the Free Software Foundation, Inc.

   This file is part of RCS.

   RCS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   RCS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with RCS; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Report problems and direct all questions to:

        rcs-bugs@cs.purdue.edu

 */

#if has_conf_h
#include <conf.h>
#else
#if HAVE_CONFIG_H
#include <config.h>
#else
#ifndef __STDC__
#define const
#endif
#endif
/* MIPS RISCOS4.52 defines time_t in <sys/types.h> not <time.h>.  */
#include <sys/types.h>
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef LONG_MIN
#define LONG_MIN (-1 - 2147483647L)
#endif
#if STDC_HEADERS
#include <stdlib.h>
#endif
#include <time.h>
#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif
#endif

#include <partime.h>
#include <maketime.h>

char const maketId[] =
    "$Id: maketime.c,v 5.15 1997/06/17 16:54:36 eggert Exp $";

static int isleap P((int));
static int month_days P((struct tm const*));
static time_t maketime P((struct partime const*, time_t));

/* For maximum portability, use only localtime and gmtime.
   Make no assumptions about the time_t epoch or the range of time_t values.
   Avoid mktime because it's not universal and because there's no easy,
   portable way for mktime to yield the inverse of gmtime.  */

#define TM_YEAR_ORIGIN 1900

static int isleap(y) int y;
{
	return (y & 3) == 0 && (y % 100 != 0 || y % 400 == 0);
}

/* days in year before start of months 0-12 */
static int const month_yday[] = {0,   31,  59,  90,  120, 151, 181,
                                 212, 243, 273, 304, 334, 365};

/* Yield the number of days in TM's month.  */
static int month_days(tm) struct tm const* tm;
{
	int m = tm->tm_mon;
	return (month_yday[m + 1] - month_yday[m] +
	        (m == 1 && isleap(tm->tm_year + TM_YEAR_ORIGIN)));
}

/* Convert UNIXTIME to struct tm form.
   Use gmtime if available and if !LOCALZONE, localtime otherwise.  */
struct tm* time2tm(unixtime, localzone) time_t unixtime;
int localzone;
{
	struct tm* tm;
#ifdef TZ_is_unset
	static char const* TZ;
	if(!TZ && !(TZ = getenv("TZ")))
		TZ_is_unset("The TZ environment variable is not set; please "
		            "set it to your timezone");
#endif
	if(localzone || !(tm = gmtime(&unixtime)))
		tm = localtime(&unixtime);
	return tm;
}

/* Yield A - B, measured in seconds.  */
time_t difftm(a, b) struct tm const* a;
struct tm const* b;
{
	int ay = a->tm_year + (TM_YEAR_ORIGIN - 1);
	int by = b->tm_year + (TM_YEAR_ORIGIN - 1);
	int ac = ay / 100 - (ay % 100 < 0);
	int bc = by / 100 - (by % 100 < 0);
	int difference_in_day_of_year = a->tm_yday - b->tm_yday;
	int intervening_leap_days =
	    (((ay >> 2) - (by >> 2)) - (ac - bc) + ((ac >> 2) - (bc >> 2)));
	time_t difference_in_years = ay - by;
	time_t difference_in_days =
	    (difference_in_years * 365 +
	     (intervening_leap_days + difference_in_day_of_year));
	return (((((difference_in_days * 24 + (a->tm_hour - b->tm_hour)) * 60) +
	          (a->tm_min - b->tm_min)) *
	         60) +
	        (a->tm_sec - b->tm_sec));
}

/*
   * Adjust time T by adding SECONDS.  SECONDS must be at most 24 hours' worth.
   * Adjust only T's year, mon, mday, hour, min and sec members;
   * plus adjust wday if it is defined.
 */
void adjzone(t, seconds) register struct tm* t;
int32_t seconds;
{
	/*
	   * This code can be off by a second if SECONDS is not a multiple of
	 * 60,
	   * if T is local time, and if a leap second happens during this
	 * minute.
	   * But this bug has never occurred, and most likely will not ever
	 * occur.
	   * Liberia, the last country for which SECONDS % 60 was nonzero,
	   * switched to UTC in May 1972; the first leap second was in June
	 * 1972.
	 */
	int leap_second = t->tm_sec == 60;
	int32_t sec = seconds + (t->tm_sec - leap_second);
	if(sec < 0) {
		if((t->tm_min -= (59 - sec) / 60) < 0) {
			if((t->tm_hour -= (59 - t->tm_min) / 60) < 0) {
				t->tm_hour += 24;
				if(TM_DEFINED(t->tm_wday) && --t->tm_wday < 0)
					t->tm_wday = 6;
				if(--t->tm_mday <= 0) {
					if(--t->tm_mon < 0) {
						--t->tm_year;
						t->tm_mon = 11;
					}
					t->tm_mday = month_days(t);
				}
			}
			t->tm_min += 24 * 60;
		}
		sec += 24L * 60 * 60;
	} else if(60 <= (t->tm_min += sec / 60))
		if(24 <= (t->tm_hour += t->tm_min / 60)) {
			t->tm_hour -= 24;
			if(TM_DEFINED(t->tm_wday) && ++t->tm_wday == 7)
				t->tm_wday = 0;
			if(month_days(t) < ++t->tm_mday) {
				if(11 < ++t->tm_mon) {
					++t->tm_year;
					t->tm_mon = 0;
				}
				t->tm_mday = 1;
			}
		}
	t->tm_min %= 60;
	t->tm_sec = (int)(sec % 60) + leap_second;
}

/*
   * Convert TM to time_t, using localtime if LOCALZONE and gmtime otherwise.
   * Use only TM's year, mon, mday, hour, min, and sec members.
   * Ignore TM's old tm_yday and tm_wday, but fill in their correct values.
   * Yield -1 on failure (e.g. a member out of range).
   * Posix 1003.1-1990 doesn't allow leap seconds, but some implementations
   * have them anyway, so allow them if localtime/gmtime does.
 */
time_t tm2time(tm, localzone) struct tm* tm;
int localzone;
{
	/* Cache the most recent t,tm pairs; 1 for gmtime, 1 for localtime.  */
	static time_t t_cache[2];
	static struct tm tm_cache[2];

	time_t d, gt;
	struct tm const* gtm;
	/*
	   * The maximum number of iterations should be enough to handle any
	   * combinations of leap seconds, time zone rule changes, and solar
	 * time.
	   * 4 is probably enough; we use a bigger number just to be safe.
	 */
	int remaining_tries = 8;

	/* Avoid subscript errors.  */
	if(12 <= (unsigned)tm->tm_mon)
		return -1;

	tm->tm_yday = month_yday[tm->tm_mon] + tm->tm_mday -
	              (tm->tm_mon < 2 || !isleap(tm->tm_year + TM_YEAR_ORIGIN));

	/* Make a first guess.  */
	gt = t_cache[localzone];
	gtm = gt ? &tm_cache[localzone] : time2tm(gt, localzone);

	/* Repeatedly use the error from the guess to improve the guess.  */
	while((d = difftm(tm, gtm)) != 0) {
		if(--remaining_tries == 0)
			return -1;
		gt += d;
		gtm = time2tm(gt, localzone);
	}

/*
   * Check that the guess actually matches;
   * overflow can cause difftm to yield 0 even on differing times,
   * or tm may have members out of range (e.g. bad leap seconds).
 */
#define TM_DIFFER(a, b)                                                        \
	(((a)->tm_year ^ (b)->tm_year) | ((a)->tm_mon ^ (b)->tm_mon) |         \
	 ((a)->tm_mday ^ (b)->tm_mday) | ((a)->tm_hour ^ (b)->tm_hour) |       \
	 ((a)->tm_min ^ (b)->tm_min) | ((a)->tm_sec ^ (b)->tm_sec))
	if(TM_DIFFER(tm, gtm)) {
		/*
		   * If gt is a leap second, try gt+1; if it is one greater than
		   * a leap second, try gt-1; otherwise, it doesn't matter.
		   * Leap seconds always fall at month end.
		 */
		int yd = tm->tm_year - gtm->tm_year;
		gt += yd + (yd ? 0 : tm->tm_mon - gtm->tm_mon);
		gtm = time2tm(gt, localzone);
		if(TM_DIFFER(tm, gtm))
			return -1;
	}
	t_cache[localzone] = gt;
	tm_cache[localzone] = *gtm;

	tm->tm_wday = gtm->tm_wday;
	return gt;
}

/*
   * Check *PT and convert it to time_t.
   * If it is incompletely specified, use DEFAULT_TIME to fill it out.
   * Use localtime if PT->zone is the special value TM_LOCAL_ZONE.
   * Yield -1 on failure.
   * ISO 8601 day-of-year and week numbers are not yet supported.
 */
static time_t maketime(pt, default_time) struct partime const* pt;
time_t default_time;
{
	int localzone, wday;
	struct tm tm;
	struct tm* tm0 = 0;
	time_t r;

	tm0 = 0; /* Keep gcc -Wall happy.  */
	localzone = pt->zone == TM_LOCAL_ZONE;

	tm = pt->tm;

	if(TM_DEFINED(pt->ymodulus) || !TM_DEFINED(tm.tm_year)) {
		/* Get tm corresponding to default time.  */
		tm0 = time2tm(default_time, localzone);
		if(!localzone)
			adjzone(tm0, pt->zone);
	}

	if(TM_DEFINED(pt->ymodulus))
		tm.tm_year += (tm0->tm_year + TM_YEAR_ORIGIN) / pt->ymodulus *
		              pt->ymodulus;
	else if(!TM_DEFINED(tm.tm_year)) {
		/* Set default year, month, day from current time.  */
		tm.tm_year = tm0->tm_year + TM_YEAR_ORIGIN;
		if(!TM_DEFINED(tm.tm_mon)) {
			tm.tm_mon = tm0->tm_mon;
			if(!TM_DEFINED(tm.tm_mday))
				tm.tm_mday = tm0->tm_mday;
		}
	}

	/* Convert from partime year (Gregorian) to Posix year.  */
	tm.tm_year -= TM_YEAR_ORIGIN;

	/* Set remaining default fields to be their minimum values.  */
	if(!TM_DEFINED(tm.tm_mon))
		tm.tm_mon = 0;
	if(!TM_DEFINED(tm.tm_mday))
		tm.tm_mday = 1;
	if(!TM_DEFINED(tm.tm_hour))
		tm.tm_hour = 0;
	if(!TM_DEFINED(tm.tm_min))
		tm.tm_min = 0;
	if(!TM_DEFINED(tm.tm_sec))
		tm.tm_sec = 0;

	if(!localzone)
		adjzone(&tm, -pt->zone);
	wday = tm.tm_wday;

	/* Convert and fill in the rest of the tm.  */
	r = tm2time(&tm, localzone);

	/* Check weekday.  */
	if(r != -1 && TM_DEFINED(wday) && wday != tm.tm_wday)
		return -1;

	return r;
}

/* Parse a free-format date in SOURCE, yielding a Unix format time.  */
time_t str2time(source, default_time, default_zone) char const* source;
time_t default_time;
int32_t default_zone;
{
	struct partime pt;

	if(*partime(source, &pt))
		return -1;
	if(pt.zone == TM_UNDEFINED_ZONE)
		pt.zone = default_zone;
	return maketime(&pt, default_time);
}

#if TEST
#include <stdio.h>
int main(argc, argv) int argc;
char** argv;
{
	time_t default_time = time((time_t*)0);
	int32_t default_zone = argv[1] ? atol(argv[1]) : 0;
	char buf[1000];
	while(fgets(buf, sizeof(buf), stdin)) {
		time_t t = str2time(buf, default_time, default_zone);
		printf("%s", asctime(gmtime(&t)));
	}
	return 0;
}
#endif
