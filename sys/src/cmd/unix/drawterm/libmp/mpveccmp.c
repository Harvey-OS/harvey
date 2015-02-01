/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "os.h"
#include <mp.h>
#include "dat.h"

// prereq: alen >= blen
int
mpveccmp(mpdigit *a, int alen, mpdigit *b, int blen)
{
	mpdigit x;

	while(alen > blen)
		if(a[--alen] != 0)
			return 1;
	while(blen > alen)
		if(b[--blen] != 0)
			return -1;
	while(alen > 0){
		--alen;
		x = a[alen] - b[alen];
		if(x == 0)
			continue;
		if(x > a[alen])
			return -1;
		else
			return 1;
	}
	return 0;
}
