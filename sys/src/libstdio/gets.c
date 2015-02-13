/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- gets
 */
#include "iolib.h"
int8_t *gets(int8_t *as){
#ifdef secure
	stdin->flags|=ERR;
	return NULL;
#else
	int8_t *s=as;
	int c;
	while((c=getchar())!='\n' && c!=EOF) *s++=c;
	if(c!=EOF || s!=as) *s='\0';
	else return NULL;
	return as;
#endif
}
