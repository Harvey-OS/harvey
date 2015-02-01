/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- fopen
 */
#include "iolib.h"
FILE *fopen(const char *name, const char *mode){
	FILE *f;
	for(f=_IO_stream;f!=&_IO_stream[FOPEN_MAX];f++)
		if(f->state==CLOSED)
			return freopen(name, mode, f);
	return NULL;
}
