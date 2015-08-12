/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __TIME_H
#define __TIME_H
#pragma lib "/$M/lib/ape/libap.a"

#include <stddef.h>

#define CLOCKS_PER_SEC 1000

/* obsolsecent, but required */
#define CLK_TCK CLOCKS_PER_SEC

#ifndef _CLOCK_T
#define _CLOCK_T
typedef int32_t clock_t;
#endif
#ifndef _TIME_T
#define _TIME_T
typedef int32_t time_t;
#endif

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

#ifdef __cplusplus
extern "C" {
#endif

extern clock_t clock(void);
extern double difftime(time_t, time_t);
extern time_t mktime(struct tm*);
extern time_t time(time_t*);
extern char* asctime(const struct tm*);
extern char* ctime(const time_t*);
extern struct tm* gmtime(const time_t*);
extern struct tm* localtime(const time_t*);
extern size_t strftime(char*, size_t, const char*, const struct tm*);

#ifdef _REENTRANT_SOURCE
extern struct tm* gmtime_r(const time_t*, struct tm*);
extern struct tm* localtime_r(const time_t*, struct tm*);
extern char* ctime_r(const time_t*, char*);
#endif

#ifdef _POSIX_SOURCE
extern void tzset(void);
#endif

#ifdef __cplusplus
}
#endif

#ifdef _POSIX_SOURCE
extern char* tzname[2];
#endif

#endif /* __TIME_H */
