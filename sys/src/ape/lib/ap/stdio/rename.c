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
int rename(const int8_t *old, const int8_t *new){
	if(link((int8_t *)old, (int8_t *)new)<0) return -1;
	if(unlink((int8_t *)old)<0){
		unlink((int8_t *)new);
		return -1;
	}
	return 0;
}
