/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <time.h>
#include <sys/times.h>

clock_t
clock(void)
{
	struct tms t;

	if(times(&t) == (clock_t)-1)
		return (clock_t)-1;
	return t.tms_utime+t.tms_stime;
}
