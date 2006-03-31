#include <u.h>
#include <libc.h>
#include "fmtdef.h"

/* format the output into f->to and return the number of characters fmted  */

int
dorfmt(Fmt *f, Rune *fmt)
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

		fmt = _fmtdispatch(f, fmt, 1);
		if(fmt == nil)
			return -1;
	}
}
