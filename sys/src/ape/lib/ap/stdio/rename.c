/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- rename
 */
#include "iolib.h"
int rename(const char *old, const char *new){
	if(link((char *)old, (char *)new)<0) return -1;
	if(unlink((char *)old)<0){
		unlink((char *)new);
		return -1;
	}
	return 0;
}
