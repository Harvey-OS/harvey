/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>

extern	char **environ;

char *
getenv(const char *name)
{
	char **p = environ;
	char *s1, *s2;

	while (*p != NULL){
		for(s1 = (char *)name, s2 = *p++; *s1 == *s2; s1++, s2++)
			continue;
		if(*s1 == '\0' && *s2 == '=')
			return s2+1;
	}
	return NULL ;
}
