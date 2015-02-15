/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- vsprintf
 */
#include "iolib.h"
int vsprintf(char *buf, const char *fmt, va_list args){
	int n;
	FILE *f=_IO_sopenw();
	if(f==NULL)
		return 0;
	setvbuf(f, buf, _IOFBF, 100000);
	n=vfprintf(f, fmt, args);
	_IO_sclose(f);
	return n;
}
