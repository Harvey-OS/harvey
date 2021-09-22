/*
 * pANS stdio -- vfprintf
 */
#include "iolib.h"
/*
 * Trailing flags
 */
#define	SHORT	32		/* 'h' convert a short integer */
#define	LONG	64		/* 'l' convert a long integer */
#define	LDBL	128		/* 'L' convert a long double */
#define	PTR	256		/*     convert a void * (%p) */

static int lflag[] = {	/* leading flags */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^@ ^A ^B ^C ^D ^E ^F ^G */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^H ^I ^J ^K ^L ^M ^N ^O */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^P ^Q ^R ^S ^T ^U ^V ^W */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^X ^Y ^Z ^[ ^\ ^] ^^ ^_ */
SPACE,	0,	0,	ALT,	0,	0,	0,	0,	/* sp  !  "  #  $  %  &  ' */
0,	0,	0,	SIGN,	0,	LEFT,	0,	0,	/*  (  )  *  +  ,  -  .  / */
ZPAD,	0,	0,	0,	0,	0,	0,	0,	/*  0  1  2  3  4  5  6  7 */
0,	0,	0,	0,	0,	0,	0,	0,	/*  8  9  :  ;  <  =  >  ? */
0,	0,	0,	0,	0,	0,	0,	0,	/*  @  A  B  C  D  E  F  G */
0,	0,	0,	0,	0,	0,	0,	0,	/*  H  I  J  K  L  M  N  O */
0,	0,	0,	0,	0,	0,	0,	0,	/*  P  Q  R  S  T  U  V  W */
0,	0,	0,	0,	0,	0,	0,	0,	/*  X  Y  Z  [  \  ]  ^  _ */
0,	0,	0,	0,	0,	0,	0,	0,	/*  `  a  b  c  d  e  f  g */
0,	0,	0,	0,	0,	0,	0,	0,	/*  h  i  j  k  l  m  n  o */
0,	0,	0,	0,	0,	0,	0,	0,	/*  p  q  r  s  t  u  v  w */
0,	0,	0,	0,	0,	0,	0,	0,	/*  x  y  z  {  |  }  ~ ^? */

0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
};

static int tflag[] = {	/* trailing flags */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^@ ^A ^B ^C ^D ^E ^F ^G */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^H ^I ^J ^K ^L ^M ^N ^O */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^P ^Q ^R ^S ^T ^U ^V ^W */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^X ^Y ^Z ^[ ^\ ^] ^^ ^_ */
0,	0,	0,	0,	0,	0,	0,	0,	/* sp  !  "  #  $  %  &  ' */
0,	0,	0,	0,	0,	0,	0,	0,	/*  (  )  *  +  ,  -  .  / */
0,	0,	0,	0,	0,	0,	0,	0,	/*  0  1  2  3  4  5  6  7 */
0,	0,	0,	0,	0,	0,	0,	0,	/*  8  9  :  ;  <  =  >  ? */
0,	0,	0,	0,	0,	0,	0,	0,	/*  @  A  B  C  D  E  F  G */
0,	0,	0,	0,	LDBL,	0,	0,	0,	/*  H  I  J  K  L  M  N  O */
0,	0,	0,	0,	0,	0,	0,	0,	/*  P  Q  R  S  T  U  V  W */
0,	0,	0,	0,	0,	0,	0,	0,	/*  X  Y  Z  [  \  ]  ^  _ */
0,	0,	0,	0,	0,	0,	0,	0,	/*  `  a  b  c  d  e  f  g */
SHORT,	0,	0,	0,	LONG,	0,	0,	0,	/*  h  i  j  k  l  m  n  o */
0,	0,	0,	0,	0,	0,	0,	0,	/*  p  q  r  s  t  u  v  w */
0,	0,	0,	0,	0,	0,	0,	0,	/*  x  y  z  {  |  }  ~ ^? */

0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
};

	int ocvt_E(FILE *, va_list *, int, int, int);
	int ocvt_G(FILE *, va_list *, int, int, int);
static int ocvt_X(FILE *, va_list *, int, int, int);
static int ocvt_c(FILE *, va_list *, int, int, int);
static int ocvt_d(FILE *, va_list *, int, int, int);
	int ocvt_e(FILE *, va_list *, int, int, int);
	int ocvt_f(FILE *, va_list *, int, int, int);
	int ocvt_g(FILE *, va_list *, int, int, int);
static int ocvt_n(FILE *, va_list *, int, int, int);
static int ocvt_o(FILE *, va_list *, int, int, int);
static int ocvt_p(FILE *, va_list *, int, int, int);
static int ocvt_s(FILE *, va_list *, int, int, int);
static int ocvt_u(FILE *, va_list *, int, int, int);
static int ocvt_x(FILE *, va_list *, int, int, int);

static int(*ocvt[])(FILE *, va_list *, int, int, int) = {
0,	0,	0,	0,	0,	0,	0,	0,	/* ^@ ^A ^B ^C ^D ^E ^F ^G */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^H ^I ^J ^K ^L ^M ^N ^O */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^P ^Q ^R ^S ^T ^U ^V ^W */
0,	0,	0,	0,	0,	0,	0,	0,	/* ^X ^Y ^Z ^[ ^\ ^] ^^ ^_ */
0,	0,	0,	0,	0,	0,	0,	0,	/* sp  !  "  #  $  %  &  ' */
0,	0,	0,	0,	0,	0,	0,	0,	/*  (  )  *  +  ,  -  .  / */
0,	0,	0,	0,	0,	0,	0,	0,	/*  0  1  2  3  4  5  6  7 */
0,	0,	0,	0,	0,	0,	0,	0,	/*  8  9  :  ;  <  =  >  ? */
0,	0,	0,	0,	0,	ocvt_E,	0,	ocvt_G,	/*  @  A  B  C  D  E  F  G */
0,	0,	0,	0,	0,	0,	0,	0,	/*  H  I  J  K  L  M  N  O */
0,	0,	0,	0,	0,	0,	0,	0,	/*  P  Q  R  S  T  U  V  W */
ocvt_X,	0,	0,	0,	0,	0,	0,	0,	/*  X  Y  Z  [  \  ]  ^  _ */
0,	0,	0,	ocvt_c,	ocvt_d,	ocvt_e,	ocvt_f,	ocvt_g,	/*  `  a  b  c  d  e  f  g */
0,	ocvt_d,	0,	0,	0,	0,	ocvt_n,	ocvt_o,	/*  h  i  j  k  l  m  n  o */
ocvt_p,	0,	0,	ocvt_s,	0,	ocvt_u,	0,	0,	/*  p  q  r  s  t  u  v  w */
ocvt_x,	0,	0,	0,	0,	0,	0,	0,	/*  x  y  z  {  |  }  ~ ^? */

0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
0,	0,	0,	0,	0,	0,	0,	0,
};

static int nprint;

QLock _stdiolk;

int
vfprintf(FILE *f, const char *as, va_list args)
{
	int flags, width, precision;
	uchar *s;

	qlock(&_stdiolk);

	s = (uchar *)as;
	nprint = 0;
	while(*s){
		if(*s != '%'){
			putc(*s++, f);
			nprint++;
			continue;
		}
		s++;
		flags = 0;
		while(lflag[*s]) flags |= lflag[*s++];
		if(*s == '*'){
			width = va_arg(args, int);
			s++;
			if(width<0){
				flags |= LEFT;
				width = -width;
			}
		}
		else{
			width = 0;
			while('0'<=*s && *s<='9') width = width*10 + *s++ - '0';
		}
		if(*s == '.'){
			s++;
			if(*s == '*'){
				precision = va_arg(args, int);
				s++;
			}
			else{
				precision = 0;
				while('0'<=*s && *s<='9') precision = precision*10 + *s++ - '0';
			}
		}
		else
			precision = -1;
		while(tflag[*s]) flags |= tflag[*s++];
		if(ocvt[*s]) nprint += (*ocvt[*s++])(f, &args, flags, width, precision);
		else if(*s){
			putc(*s++, f);
			nprint++;
		}
	}

	qunlock(&_stdiolk);

	if(ferror(f)){
		if((f->flags&STRING) && f->wp==f->rp && f->wp>f->buf){
			*(f->wp-1) = '\0';
			return nprint;
		}
		return -1;
	}
	return nprint;
}

static int
ocvt_c(FILE *f, va_list *args, int flags, int width, int precision)
{
	int i;

	USED(precision);
	if(!(flags&LEFT)) for(i=1; i<width; i++) putc(' ', f);
	putc((unsigned char)va_arg(*args, int), f);
	if(flags&LEFT) for(i=1; i<width; i++) putc(' ', f);
	return width<1 ? 1 : width;
}

static int
ocvt_s(FILE *f, va_list *args, int flags, int width, int precision)
{
	int i, n = 0;
	char *s;

	s = va_arg(*args, char *);
	if (s == 0)
		s = "(null)";
	if(!(flags&LEFT)){
		if(precision >= 0)
			for(i=0; i!=precision && s[i]; i++)
				;
		else
			for(i=0; s[i]; i++)
				;
		for(; i<width; i++){
			putc(' ', f);
			n++;
		}
	}
	if(precision >= 0){
		for(i=0; i!=precision && *s; i++){
			putc(*s++, f);
			n++;
		}
	} else{
		for(i=0;*s;i++){
			putc(*s++, f);
			n++;
		}
	}
	if(flags&LEFT){
		for(; i<width; i++){
			putc(' ', f);
			n++;
		}
	}
	return n;
}

static int
ocvt_n(FILE *f, va_list *args, int flags, int width, int precision)
{
	USED(precision, width, f);
	if(flags&SHORT)
		*va_arg(*args, short *) = nprint;
	else if(flags&LONG)
		*va_arg(*args, long *) = nprint;
	else
		*va_arg(*args, int *) = nprint;
	return 0;
}

/*
 * Generic fixed-point conversion
 *	f is the output FILE *;
 *	args is the va_list * from which to get the number;
 *	flags, width and precision are the results of printf-cracking;
 *	radix is the number base to print in;
 *	alphabet is the set of digits to use;
 *	prefix is the prefix to print before non-zero numbers when
 *	using ``alternate form.''
 */
static int
ocvt_fixed(FILE *f, va_list *args, int flags, int width, int precision,
	int radix, int sgned, char alphabet[], char *prefix)
{
	char digits[128];	/* no reasonable machine will ever overflow this */
	char *sign;
	char *dp;
	long snum;
	unsigned long num;
	int nout, npad, nlzero;

	if(sgned){
		if(flags&PTR) snum = (uintptr)va_arg(*args, void *);
		else if(flags&SHORT) snum = va_arg(*args, short);
		else if(flags&LONG) snum = va_arg(*args, long);
		else snum = va_arg(*args, int);
		if(snum < 0){
			sign = "-";
			num = -snum;
		} else{
			if(flags&SIGN) sign = "+";
			else if(flags&SPACE) sign = " ";
			else sign = "";
			num = snum;
		}
	} else {
		sign = "";
		if(flags&PTR) num = (uintptr)va_arg(*args, void *);
		else if(flags&SHORT) num = va_arg(*args, unsigned short);
		else if(flags&LONG) num = va_arg(*args, unsigned long);
		else num = va_arg(*args, unsigned int);
	}
	if(num == 0) prefix = "";
	dp = digits;
	do{
		*dp++ = alphabet[num%radix];
		num /= radix;
	}while(num);
	if(precision==0 && dp-digits==1 && dp[-1]=='0')
		dp--;
	nlzero = precision-(dp-digits);
	if(nlzero < 0) nlzero = 0;
	if(flags&ALT){
		if(radix == 8) if(dp[-1]=='0' || nlzero) prefix = "";
	}
	else prefix = "";
	nout = dp-digits+nlzero+strlen(prefix)+strlen(sign);
	npad = width-nout;
	if(npad < 0) npad = 0;
	nout += npad;
	if(!(flags&LEFT)){
		if(flags&ZPAD && precision <= 0){
			fputs(sign, f);
			fputs(prefix, f);
			while(npad){
				putc('0', f);
				--npad;
			}
		} else{
			while(npad){
				putc(' ', f);
				--npad;
			}
			fputs(sign, f);
			fputs(prefix, f);
		}
		while(nlzero){
			putc('0', f);
			--nlzero;
		}
		while(dp!=digits) putc(*--dp, f);
	}
	else{
		fputs(sign, f);
		fputs(prefix, f);
		while(nlzero){
			putc('0', f);
			--nlzero;
		}
		while(dp != digits) putc(*--dp, f);
		while(npad){
			putc(' ', f);
			--npad;
		}
	}
	return nout;	
}

static int
ocvt_X(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_fixed(f, args, flags, width, precision, 16, 0, "0123456789ABCDEF", "0X");
}

static int
ocvt_d(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_fixed(f, args, flags, width, precision, 10, 1, "0123456789", "");
}

static int
ocvt_o(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_fixed(f, args, flags, width, precision, 8, 0, "01234567", "0");
}

static int
ocvt_p(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_fixed(f, args, flags|PTR|ALT, width, precision, 16, 0,
		"0123456789ABCDEF", "0X");
}

static int
ocvt_u(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_fixed(f, args, flags, width, precision, 10, 0, "0123456789", "");
}

static int
ocvt_x(FILE *f, va_list *args, int flags, int width, int precision)
{
	return ocvt_fixed(f, args, flags, width, precision, 16, 0, "0123456789abcdef", "0x");
}
