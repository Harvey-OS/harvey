/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

main(int argc, char **argv)
{
	char *f, *s;
	int n;

	if(argc != 2){
		fprintf(stderr, "Usage: dirname string\n");
		exit(1);
	}
	s = argv[1];
	f = s + strlen(s) - 1;
	while(f > s && *f == '/')
		f--;
	*++f = 0;
	/* now f is after last char of string, trailing slashes removed */

	for(; f >= s; f--)
		if(*f == '/'){
			f++;
			break;
		}
	if(f < s) {
		*s = '.';
		s[1] = 0;
	} else {
		--f;
		while(f > s && *f == '/')
			f--;
		f[1] = 0;
	}

	printf("%s\n", s);
	return 0;
}
