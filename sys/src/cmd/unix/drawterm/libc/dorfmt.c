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

/* format the output into f->to and return the number of characters fmted  */

int
dorfmt(Fmt *f, const Rune *fmt)
{
	Rune *rt, *rs;
	int r;
	char *t, *s;
	int nfmt;

	nfmt = f->nfmt;
	for(;;){
		if(f->runes){
			rt = f->to;
			rs = f->stop;
			while((r = *fmt++) && r != '%'){
				FMTRCHAR(f, rt, rs, r);
			}
			f->nfmt += rt - (Rune *)f->to;
			f->to = rt;
			if(!r)
				return f->nfmt - nfmt;
			f->stop = rs;
		}else{
			t = f->to;
			s = f->stop;
			while((r = *fmt++) && r != '%'){
				FMTRUNE(f, t, f->stop, r);
			}
			f->nfmt += t - (char *)f->to;
			f->to = t;
			if(!r)
				return f->nfmt - nfmt;
			f->stop = s;
		}

		fmt = __fmtdispatch(f, (Rune*)fmt, 1);
		if(fmt == nil)
			return -1;
	}
	return 0;		/* not reached */
}
