/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- fgets
 */
#include "iolib.h"
char *fgets(char *as, int n, FILE *f){
	int c=0;
	char *s=as;
	while(n>1 && (c=getc(f))!=EOF){
		*s++=c;
		--n;
		if(c=='\n') break;
	}
	if(c==EOF && s==as
	|| ferror(f)) return NULL;
	if(n) *s='\0';
	return as;
}
