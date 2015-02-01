/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "sys9.h"

time_t
time(time_t *tp)
{
	char b[20];
	int f;
	time_t t;

	memset(b, 0, sizeof(b));
	f = _OPEN("/dev/time", 0);
	if(f >= 0) {
		_PREAD(f, b, sizeof(b), 0);
		_CLOSE(f);
	}
	t = atol(b);
	if(tp)
		*tp = t;
	return t;
}
