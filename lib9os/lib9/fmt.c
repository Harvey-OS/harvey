/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include "lib9.h"
#include "fmtdef.h"

enum
{
	Maxfmt = 64
};

typedef struct Convfmt Convfmt;
struct Convfmt
{
	int	c;
	volatile	Fmts	fmt;	/* for spin lock in fmtfmt; avoids race due to write order */
};

struct
{
	/* lock by calling _fmtlock, _fmtunlock */
	int	nfmt;
	Convfmt	fmt[Maxfmt];
} fmtalloc;

static Convfmt knownfmt[] = {
	' ',	_flagfmt,
	'#',	_flagfmt,
	'%',	_percentfmt,
	'+',	_flagfmt,
	',',	_flagfmt,
	'-',	_flagfmt,
	'C',	_runefmt,
	'S',	_runesfmt,
	'X',	_ifmt,
	'b',	_ifmt,
	'c',	_charfmt,
	'd',	_ifmt,
	'h',	_flagfmt,
	'l',	_flagfmt,
	'n',	_countfmt,
	'o',	_ifmt,
	'p',	_ifmt,
	'r',	errfmt,
	's',	_strfmt,
	'u',	_flagfmt,
	'x',	_ifmt,
	0,	nil,
};

int	(*doquote)(int);

static Fmts
fmtfmt(int c)
{
	Convfmt *p, *ep;

	ep = &fmtalloc.fmt[fmtalloc.nfmt];
	for(p=fmtalloc.fmt; p<ep; p++)
		if(p->c == c)
			return p->fmt;

	/* is this a predefined format char? */
	for(p=knownfmt; p->c; p++)
		if(p->c == c){
			/* no need to lock; fmtinstall is idempotent */
			fmtinstall(p->c, p->fmt);
			while(p->fmt == nil)	/* loop until value is updated */
				;
			return p->fmt;
		}

	return _badfmt;
}

int
fmtinstall(int c, Fmts f)
{
	Convfmt *p, *ep;

	if(c<=0 || c>=65536)
		return -1;
	if(!f)
		f = _badfmt;

	_fmtlock();

	ep = &fmtalloc.fmt[fmtalloc.nfmt];
	for(p=fmtalloc.fmt; p<ep; p++)
		if(p->c == c)
			break;

	if(p == &fmtalloc.fmt[Maxfmt]){
		_fmtunlock();
		return -1;
	}

	p->fmt = f;
	if(p == ep){	/* installing a new format character */
		fmtalloc.nfmt++;
		p->c = c;
	}

	_fmtunlock();
	return 0;
}

void*
_fmtdispatch(Fmt *f, void *fmt, int isrunes)
{
	Rune rune, r;
	int i, n;

	f->flags = 0;
	f->width = f->prec = 0;

	for(;;){
		if(isrunes){
			r = *(Rune*)fmt;
			fmt = (Rune*)fmt + 1;
		}else{
			fmt = (char*)fmt + chartorune(&rune, fmt);
			r = rune;
		}
		f->r = r;
		switch(r){
		case '\0':
			return nil;
		case '.':
			f->flags |= FmtWidth|FmtPrec;
			continue;
		case '0':
			if(!(f->flags & FmtWidth)){
				f->flags |= FmtZero;
				continue;
			}
			/* fall through */
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			i = 0;
			while(r >= '0' && r <= '9'){
				i = i * 10 + r - '0';
				if(isrunes){
					r = *(Rune*)fmt;
					fmt = (Rune*)fmt + 1;
				}else{
					r = *(char*)fmt;
					fmt = (char*)fmt + 1;
				}
			}
			if(isrunes)
				fmt = (Rune*)fmt - 1;
			else
				fmt = (char*)fmt - 1;
		numflag:
			if(f->flags & FmtWidth){
				f->flags |= FmtPrec;
				f->prec = i;
			}else{
				f->flags |= FmtWidth;
				f->width = i;
			}
			continue;
		case '*':
			i = va_arg(f->args, int);
			if(i < 0){
				i = -i;
				f->flags |= FmtLeft;
			}
			goto numflag;
		}
		n = (*fmtfmt(r))(f);
		if(n < 0)
			return nil;
		if(n == 0)
			return fmt;
	}
}
