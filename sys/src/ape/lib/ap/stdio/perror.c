/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- perror
 */
#include "iolib.h"
void perror(const char *s){
	extern int errno;
	if(s!=NULL && *s != '\0') fputs(s, stderr), fputs(": ", stderr);
	fputs(strerror(errno), stderr);
	putc('\n', stderr);
	fflush(stderr);
}
