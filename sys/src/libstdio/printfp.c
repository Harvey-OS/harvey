/*
 * pANS stdio -- vfprintf FP conversions
 */
#include "iolib.h"

int ocvt_flt(FILE *, va_list *, int, int, int, char);

int
ocvt_E(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_flt(f, args, flags, width, precision, 'E');
}

int
ocvt_G(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_flt(f, args, flags, width, precision, 'G');
}

int
ocvt_e(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_flt(f, args, flags, width, precision, 'e');
}

int
ocvt_f(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_flt(f, args, flags, width, precision, 'f');
}

int
ocvt_g(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_flt(f, args, flags, width, precision, 'g');
}

int
ocvt_flt(FILE *f, va_list *args, int flags, int width, int precision, char afmt)
{
	int echr;
	char *digits, *edigits;
	int exponent;
	char fmt;
	int sign;
	int ndig;
	int nout, i;
	char ebuf[20];	/* no sensible machine will overflow this */
	char *eptr;
	double d;

	echr = 'e';
	fmt = afmt;
	d = va_arg(*args, double);
	if(precision < 0) precision = 6;
	digits = edigits = 0;			/* silence used and not set */
	exponent = 0;
	switch(fmt){
	case 'f':
		digits = dtoa(d, 3, precision, &exponent, &sign, &edigits);
		break;
	case 'E':
		echr = 'E';
		fmt = 'e';
		/* fall through */
	case 'e':
		digits = dtoa(d, 2, 1+precision, &exponent, &sign, &edigits);
		break;
	case 'G':
		echr = 'E';
		/* fall through */
	case 'g':
		if (precision > 0)
			digits = dtoa(d, 2, precision, &exponent, &sign, &edigits);
		else {
			digits = dtoa(d, 0, precision, &exponent, &sign, &edigits);
			precision = edigits - digits;
			if (exponent > precision && exponent <= precision + 4)
				precision = exponent;
			}
		if(exponent >= -3 && exponent <= precision){
			fmt = 'f';
			precision -= exponent;
		}else{
			fmt = 'e';
			--precision;
		}
		break;
	}
	if (digits == 0)	/* in case of dummy dtoa for integer programs */
		return 0;
	if (exponent == 9999) {
		/* Infinity or Nan */
		precision = 0;
		exponent = edigits - digits;
		fmt = 'f';
	}
	ndig = edigits-digits;
	if((afmt=='g' || afmt=='G') && !(flags&ALT)){	/* knock off trailing zeros */
		if(fmt == 'f'){
			if(precision+exponent > ndig) {
				precision = ndig - exponent;
				if(precision < 0)
					precision = 0;
			}
		}
		else{
			if(precision > ndig-1) precision = ndig-1;
		}
	}
	nout = precision;				/* digits after decimal point */
	if(precision!=0 || flags&ALT) nout++;		/* decimal point */
	if(fmt=='f' && exponent>0) nout += exponent;	/* digits before decimal point */
	else nout++;					/* there's always at least one */
	if(sign || flags&(SPACE|SIGN)) nout++;		/* sign */
	eptr = nil;				/* silence used and not set */
	if(fmt != 'f'){					/* exponent */
		eptr = ebuf;
		for(i=exponent<=0?1-exponent:exponent-1; i; i/=10)
			*eptr++ = '0' + i%10;
		while(eptr<ebuf+2) *eptr++ = '0';
		nout += eptr-ebuf+2;			/* e+99 */
	}
	if(!(flags&ZPAD) && !(flags&LEFT))
		while(nout < width){
			putc(' ', f);
			nout++;
		}
	if(sign) putc('-', f);
	else if(flags&SIGN) putc('+', f);
	else if(flags&SPACE) putc(' ', f);
	if(flags&ZPAD)
		while(nout < width){
			putc('0', f);
			nout++;
		}
	if(fmt == 'f'){
		for(i=0; i<exponent; i++) putc(i<ndig?digits[i]:'0', f);
		if(i == 0) putc('0', f);
		if(precision>0 || flags&ALT) putc('.', f);
		for(i=0; i!=precision; i++)
			putc(0<=i+exponent && i+exponent<ndig?digits[i+exponent]:'0', f);
	}
	else{
		putc(digits[0], f);
		if(precision>0 || flags&ALT) putc('.', f);
		for(i=0; i!=precision; i++) putc(i<ndig-1?digits[i+1]:'0', f);
	}
	if(fmt != 'f'){
		putc(echr, f);
		putc(exponent<=0?'-':'+', f);
		while(eptr>ebuf) putc(*--eptr, f);
	}
	while(nout < width){
		putc(' ', f);
		nout++;
	}
	freedtoa(digits);
	return nout;
}
