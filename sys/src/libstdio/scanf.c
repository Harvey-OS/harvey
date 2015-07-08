/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- scanf
 */
#include "iolib.h"
int scanf(const char *fmt, ...){
	int n;
	va_list va, args;
	va_start(va, fmt);
	va_copy(args, va);
	va_end(va);
	n=vfscanf(stdin, fmt, args);
	va_end(args);
	return n;
}
