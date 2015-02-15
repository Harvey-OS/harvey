/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>

int
getfields(char *str, char **args, int max, int mflag, char *set)
{
	Rune r;
	int nr, intok, narg;

	if(max <= 0)
		return 0;

	narg = 0;
	args[narg] = str;
	if(!mflag)
		narg++;
	intok = 0;
	for(;; str += nr) {
		nr = chartorune(&r, str);
		if(r == 0)
			break;
		if(utfrune(set, r)) {
			if(narg >= max)
				break;
			*str = 0;
			intok = 0;
			args[narg] = str + nr;
			if(!mflag)
				narg++;
		} else {
			if(!intok && mflag)
				narg++;
			intok = 1;
		}
	}
	return narg;
}

int
tokenize(char *str, char **args, int max)
{
	return getfields(str, args, max, 1, " \t\n\r");
}
