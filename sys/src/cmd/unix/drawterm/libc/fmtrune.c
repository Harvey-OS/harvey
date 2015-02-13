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
#include "fmtdef.h"

int
fmtrune(Fmt *f, int r)
{
	Rune *rt;
	int8_t *t;
	int n;

	if(f->runes){
		rt = (Rune*)f->to;
		FMTRCHAR(f, rt, f->stop, r);
		f->to = rt;
		n = 1;
	}else{
		t = (int8_t*)f->to;
		FMTRUNE(f, t, f->stop, r);
		n = t - (int8_t*)f->to;
		f->to = t;
	}
	f->nfmt += n;
	return 0;
}
