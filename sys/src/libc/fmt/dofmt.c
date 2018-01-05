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
dofmt(Fmt *f, const char *fmt)
{
	Rune rune, *rt, *rs;
	int r;
	char *t, *s;
	int n, nfmt;

	nfmt = f->nfmt;
	for(;;){
		if(f->runes){
			rt = f->to;
			rs = f->stop;
			while((r = *(uint8_t*)fmt) && r != '%'){
				if(r < Runeself)
					fmt++;
				else{
					fmt += chartorune(&rune, fmt);
					r = rune;
				}
				FMTRCHAR(f, rt, rs, r);
			}
			fmt++;
			f->nfmt += rt - (Rune *)f->to;
			f->to = rt;
			if(!r)
				return f->nfmt - nfmt;
			f->stop = rs;
		}else{
			t = f->to;
			s = f->stop;
			while((r = *(uint8_t*)fmt) && r != '%'){
				if(r < Runeself){
					FMTCHAR(f, t, s, r);
					fmt++;
				}else{
					n = chartorune(&rune, fmt);
					if(t + n > s){
						t = _fmtflush(f, t, n);
						if(t != nil)
							s = f->stop;
						else
							return -1;
					}
					while(n--)
						*t++ = *fmt++;
				}
			}
			fmt++;
			f->nfmt += t - (char *)f->to;
			f->to = t;
			if(!r)
				return f->nfmt - nfmt;
			f->stop = s;
		}

		fmt = _fmtdispatch(f, fmt, 0);
		if(fmt == nil)
			return -1;
	}
}

void *
_fmtflush(Fmt *f, void *t, int len)
{
	if(f->runes)
		f->nfmt += (Rune*)t - (Rune*)f->to;
	else
		f->nfmt += (char*)t - (char *)f->to;
	f->to = t;
	if(f->flush == 0 || (*f->flush)(f) == 0 || (char*)f->to + len > (char*)f->stop){
		f->stop = f->to;
		return nil;
	}
	return f->to;
}

/*
 * put a formatted block of memory sz bytes long of n runes into the output buffer,
 * left/right justified in a field of at least f->width charactes
 */
int
_fmtpad(Fmt *f, int n)
{
	char *t, *s;
	int i;

	t = f->to;
	s = f->stop;
	for(i = 0; i < n; i++)
		FMTCHAR(f, t, s, ' ');
	f->nfmt += t - (char *)f->to;
	f->to = t;
	return 0;
}

int
_rfmtpad(Fmt *f, int n)
{
	Rune *t, *s;
	int i;

	t = f->to;
	s = f->stop;
	for(i = 0; i < n; i++)
		FMTRCHAR(f, t, s, ' ');
	f->nfmt += t - (Rune *)f->to;
	f->to = t;
	return 0;
}

int
_fmtcpy(Fmt *f, const void *vm, int n, int sz)
{
	Rune *rt, *rs, r;
	char *t, *s;
	const char *m, *me;
	uint32_t fl;
	int nc, w;

	m = vm;
	me = m + sz;
	w = f->width;
	fl = f->flags;
	if((fl & FmtPrec) && n > f->prec)
		n = f->prec;
	if(f->runes){
		if(!(fl & FmtLeft) && _rfmtpad(f, w - n) < 0)
			return -1;
		rt = f->to;
		rs = f->stop;
		for(nc = n; nc > 0; nc--){
			r = *(uint8_t*)m;
			if(r < Runeself)
				m++;
			else if((me - m) >= UTFmax || fullrune(m, me-m))
				m += chartorune(&r, m);
			else
				break;
			FMTRCHAR(f, rt, rs, r);
		}
		f->nfmt += rt - (Rune *)f->to;
		f->to = rt;
		if(fl & FmtLeft && _rfmtpad(f, w - n) < 0)
			return -1;
	}else{
		if(!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
			return -1;
		t = f->to;
		s = f->stop;
		for(nc = n; nc > 0; nc--){
			r = *(uint8_t*)m;
			if(r < Runeself)
				m++;
			else if((me - m) >= UTFmax || fullrune(m, me-m))
				m += chartorune(&r, m);
			else
				break;
			FMTRUNE(f, t, s, r);
		}
		f->nfmt += t - (char *)f->to;
		f->to = t;
		if(fl & FmtLeft && _fmtpad(f, w - n) < 0)
			return -1;
	}
	return 0;
}

int
_fmtrcpy(Fmt *f, const void *vm, int n)
{
	Rune r, *rt, *rs;
	const Rune *m, *me;
	char *t, *s;
	uint32_t fl;
	int w;

	m = vm;
	w = f->width;
	fl = f->flags;
	if((fl & FmtPrec) && n > f->prec)
		n = f->prec;
	if(f->runes){
		if(!(fl & FmtLeft) && _rfmtpad(f, w - n) < 0)
			return -1;
		rt = f->to;
		rs = f->stop;
		for(me = m + n; m < me; m++)
			FMTRCHAR(f, rt, rs, *m);
		f->nfmt += rt - (Rune *)f->to;
		f->to = rt;
		if(fl & FmtLeft && _rfmtpad(f, w - n) < 0)
			return -1;
	}else{
		if(!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
			return -1;
		t = f->to;
		s = f->stop;
		for(me = m + n; m < me; m++){
			r = *m;
			FMTRUNE(f, t, s, r);
		}
		f->nfmt += t - (char *)f->to;
		f->to = t;
		if(fl & FmtLeft && _fmtpad(f, w - n) < 0)
			return -1;
	}
	return 0;
}

/* fmt out one character */
int
_charfmt(Fmt *f)
{
	char x[1];

	x[0] = va_arg(f->args, int);
	f->prec = 1;
	return _fmtcpy(f, x, 1, 1);
}

/* fmt out one rune */
int
_runefmt(Fmt *f)
{
	Rune x[1];

	x[0] = va_arg(f->args, int);
	return _fmtrcpy(f, x, 1);
}

/* public helper routine: fmt out a null terminated string already in hand */
int
fmtstrcpy(Fmt *f, const char *s)
{
	int i, j;
	Rune r;

	if(!s)
		return _fmtcpy(f, "<nil>", 5, 5);
	/* if precision is specified, make sure we don't wander off the end */
	if(f->flags & FmtPrec){
		i = 0;
		for(j=0; j<f->prec && s[i]; j++)
			i += chartorune(&r, s+i);
		return _fmtcpy(f, s, j, i);
	}
	return _fmtcpy(f, s, utflen(s), strlen(s));
}

/* fmt out a null terminated utf string */
int
_strfmt(Fmt *f)
{
	char *s;

	s = va_arg(f->args, char *);
	return fmtstrcpy(f, s);
}

/* public helper routine: fmt out a null terminated rune string already in hand */
int
fmtrunestrcpy(Fmt *f, const Rune *s)
{
	const Rune *e;
	int n, p;

	if(!s)
		return _fmtcpy(f, "<nil>", 5, 5);
	/* if precision is specified, make sure we don't wander off the end */
	if(f->flags & FmtPrec){
		p = f->prec;
		for(n = 0; n < p; n++)
			if(s[n] == 0)
				break;
	}else{
		for(e = s; *e; e++)
			;
		n = e - s;
	}
	return _fmtrcpy(f, s, n);
}

/* fmt out a null terminated rune string */
int
_runesfmt(Fmt *f)
{
	Rune *s;

	s = va_arg(f->args, Rune *);
	return fmtrunestrcpy(f, s);
}

/* fmt a % */
int
_percentfmt(Fmt *f)
{
	Rune x[1];

	x[0] = f->r;
	f->prec = 1;
	return _fmtrcpy(f, x, 1);
}

enum {
	/* %,#llb could emit a sign, "0b" and 64 digits with 21 commas */
	Maxintwidth = 1 + 2 + 64 + 64/3,
};

/* fmt an integer */
int
_ifmt(Fmt *f)
{
	char buf[Maxintwidth + 1], *p, *conv;
	uint64_t vu;
	uint32_t u;
	uintptr pu;
	int neg, base, i, n, fl, w, isv;

	neg = 0;
	fl = f->flags;
	isv = 0;
	vu = 0;
	u = 0;
	if(f->r == 'p'){
		pu = va_arg(f->args, uintptr);
		if(sizeof(uintptr) == sizeof(uint64_t)){
			vu = pu;
			isv = 1;
		}else
			u = pu;
		f->r = 'x';
		fl |= FmtUnsigned;
	}else if(fl & FmtVLong){
		isv = 1;
		if(fl & FmtUnsigned)
			vu = va_arg(f->args, uint64_t);
		else
			vu = va_arg(f->args, int64_t);
	}else if(fl & FmtLong){
		if(fl & FmtUnsigned)
			u = va_arg(f->args, uint32_t);
		else
			u = va_arg(f->args, int32_t);
	}else if(fl & FmtByte){
		if(fl & FmtUnsigned)
			u = (uint8_t)va_arg(f->args, int);
		else
			u = (char)va_arg(f->args, int);
	}else if(fl & FmtShort){
		if(fl & FmtUnsigned)
			u = (uint16_t)va_arg(f->args, int);
		else
			u = (int16_t)va_arg(f->args, int);
	}else{
		if(fl & FmtUnsigned)
			u = va_arg(f->args, uint);
		else
			u = va_arg(f->args, int);
	}
	conv = "0123456789abcdef";
	switch(f->r){
	case 'd':
		base = 10;
		break;
	case 'u':
		base = 10;
		f->flags |= FmtUnsigned;
		break;
	case 'x':
		base = 16;
		break;
	case 'X':
		base = 16;
		conv = "0123456789ABCDEF";
		break;
	case 'b':
		base = 2;
		break;
	case 'o':
		base = 8;
		break;
	default:
		return -1;
	}
	if(!(fl & FmtUnsigned)){
		if(isv && (int64_t)vu < 0){
			vu = -(int64_t)vu;
			neg = 1;
		}else if(!isv && (int32_t)u < 0){
			u = -(int32_t)u;
			neg = 1;
		}
	}
	p = buf + sizeof buf - 1;
	n = 0;
	if(isv){
		while(vu){
			i = vu % base;
			vu /= base;
			if((fl & FmtComma) && n % 4 == 3){
				*p-- = ',';
				n++;
			}
			*p-- = conv[i];
			n++;
		}
	}else{
		while(u){
			i = u % base;
			u /= base;
			if((fl & FmtComma) && n % 4 == 3){
				*p-- = ',';
				n++;
			}
			*p-- = conv[i];
			n++;
		}
	}
	if(n == 0){
		*p-- = '0';
		n = 1;
	}
	for(w = f->prec; n < w && p > buf+3; n++)
		*p-- = '0';
	if(neg || (fl & (FmtSign|FmtSpace)))
		n++;
	if(fl & FmtSharp){
		if(base == 16)
			n += 2;
		else if(base == 8){
			if(p[1] == '0')
				fl &= ~FmtSharp;
			else
				n++;
		}
	}
	if((fl & FmtZero) && !(fl & (FmtLeft|FmtPrec))){
		for(w = f->width; n < w && p > buf+3; n++)
			*p-- = '0';
		f->width = 0;
	}
	if(fl & FmtSharp){
		if(base == 16)
			*p-- = f->r;
		if(base == 16 || base == 8)
			*p-- = '0';
	}
	if(neg)
		*p-- = '-';
	else if(fl & FmtSign)
		*p-- = '+';
	else if(fl & FmtSpace)
		*p-- = ' ';
	f->flags &= ~FmtPrec;
	return _fmtcpy(f, p + 1, n, n);
}

int
_countfmt(Fmt *f)
{
	void *p;
	uint32_t fl;

	fl = f->flags;
	p = va_arg(f->args, void*);
	if(fl & FmtVLong){
		*(int64_t*)p = f->nfmt;
	}else if(fl & FmtLong){
		*(int32_t*)p = f->nfmt;
	}else if(fl & FmtByte){
		*(char*)p = f->nfmt;
	}else if(fl & FmtShort){
		*(int16_t*)p = f->nfmt;
	}else{
		*(int*)p = f->nfmt;
	}
	return 0;
}

int
_flagfmt(Fmt *f)
{
	switch(f->r){
	case ',':
		f->flags |= FmtComma;
		break;
	case '-':
		f->flags |= FmtLeft;
		break;
	case '+':
		f->flags |= FmtSign;
		break;
	case '#':
		f->flags |= FmtSharp;
		break;
	case ' ':
		f->flags |= FmtSpace;
		break;
	case 'h':
		if(f->flags & FmtShort)
			f->flags |= FmtByte;
		f->flags |= FmtShort;
		break;
	case 'l':
		if(f->flags & FmtLong)
			f->flags |= FmtVLong;
		f->flags |= FmtLong;
		break;
	}
	return 1;
}

/* default error format */
int
_badfmt(Fmt *f)
{
	Rune x[3];

	x[0] = '%';
	x[1] = f->r;
	x[2] = '%';
	f->prec = 3;
	_fmtrcpy(f, x, 3);
	return 0;
}
