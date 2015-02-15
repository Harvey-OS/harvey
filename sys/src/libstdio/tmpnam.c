/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- tmpnam
 */
#include "iolib.h"

char *tmpnam(char *s){
	static char name[]="/tmp/tn000000000000";
	char *p;
	do{
		p=name+7;
		while(*p=='9') *p++='0';
		if(*p=='\0') return NULL;
		++*p;
	}while(access(name, 0)==0);
	if(s){
		strcpy(s, name);
		return s;
	}
	return name;
}
