/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

int	strcomment = '#';

int
strparse(char *p, int arsize, char **arv)
{
	int arc = 0;

	/*print("parse: 0x%lux = \"%s\"\n", p, p);/**/
	while(p){
		while(*p == ' ' || *p == '\t')
			p++;
		if(*p == 0 || *p == strcomment)
			break;
		if(arc >= arsize-1)
			break;
		arv[arc++] = p;
		while(*p && *p != ' ' && *p != '\t')
			p++;
		if(*p == 0)
			break;
		*p++ = 0;
	}
	arv[arc] = 0;
	/*while(*arv){
		print("\t0x%lux = \"%s\"\n", *arv, *arv);
		++arv;
	}/**/
	return arc;
}
