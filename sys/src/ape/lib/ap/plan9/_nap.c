/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <unistd.h>
#include <time.h>
#include "sys9.h"

/*
 * This is an extension to POSIX
 */
unsigned int
_nap(unsigned int millisecs)
{
	time_t t0, t1;

	t0 = time(0);
	if(_SLEEP(millisecs) < 0){
		t1 = time(0);
		return t1-t0;
	}
	return 0;
}
