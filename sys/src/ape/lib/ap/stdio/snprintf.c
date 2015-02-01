/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- sprintf
 */
#include "iolib.h"

int snprintf(char *buf, size_t nbuf, const char *fmt, ...){
	int n;
	va_list args;
	FILE *f=_IO_sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, nbuf);
	va_start(args, fmt);
	n=vfprintf(f, fmt, args);
	va_end(args);
	_IO_sclose(f);
	return n;
}
