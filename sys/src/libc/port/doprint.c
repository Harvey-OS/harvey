#include	<u.h>
#include	<libc.h>

int	printcol;

enum
{
	SIZE	= 1024,
	IDIGIT	= 30,
	MAXCONV	= 40,
	FDIGIT	= 30,
	FDEFLT	= 6,
	NONE	= -1000,
	MAXFMT	= 512,

	FPLUS	= (1<<0),
	FMINUS	= (1<<1),
	FSHARP	= (1<<2),
	FLONG	= (1<<3),
	FSHORT	= (1<<4),
	FUNSIGN	= (1<<5),
	FVLONG	= (1<<6),
};

#define	PTR	sizeof(char*)
#define	SHORT	sizeof(int)
#define	INT	sizeof(int)
#define	LONG	sizeof(long)
#define	VLONG	sizeof(vlong)
#define	FLOAT	sizeof(double)

static	char	*out, *eout;
static	int	convcount;
static	char	fmtindex[MAXFMT];

static	int	noconv(void*, int, int, int, int);
static	int	flags(void*, int, int, int, int);

static	int	cconv(void*, int, int, int, int);
static	int	rconv(void*, int, int, int, int);
static	int	sconv(void*, int, int, int, int);
static	int	percent(void*, int, int, int, int);

int	numbconv(void*, int, int, int, int);
int	fltconv(void*, int, int, int, int);

static
int	(*fmtconv[MAXCONV])(void*, int, int, int, int) =
{
	noconv
};

static
void
initfmt(void)
{
	int cc;

	cc = 0;
	fmtconv[cc] = noconv;
	cc++;

	fmtconv[cc] = flags;
	fmtindex['+'] = cc;
	fmtindex['-'] = cc;
	fmtindex['#'] = cc;
	fmtindex['h'] = cc;
	fmtindex['l'] = cc;
	fmtindex['u'] = cc;
	cc++;

	fmtconv[cc] = numbconv;
	fmtindex['d'] = cc;
	fmtindex['o'] = cc;
	fmtindex['x'] = cc;
	fmtindex['X'] = cc;
	cc++;

	fmtconv[cc] = fltconv;
	fmtindex['e'] = cc;
	fmtindex['f'] = cc;
	fmtindex['g'] = cc;
	fmtindex['E'] = cc;
	fmtindex['G'] = cc;
	cc++;

	fmtconv[cc] = cconv;
	fmtindex['c'] = cc;
	fmtindex['C'] = cc;
	cc++;

	fmtconv[cc] = rconv;
	fmtindex['r'] = cc;
	cc++;

	fmtconv[cc] = sconv;
	fmtindex['s'] = cc;
	fmtindex['S'] = cc;
	cc++;

	fmtconv[cc] = percent;
	fmtindex['%'] = cc;
	cc++;

	convcount = cc;
}

int
fmtinstall(int c, int (*f)(void*, int, int, int, int))
{

	if(convcount == 0)
		initfmt();
	if(c < 0 || c >= MAXFMT)
		return 1;
	if(convcount >= MAXCONV)
		return 1;
	fmtconv[convcount] = f;
	fmtindex[c] = convcount;
	convcount++;
	return 0;
}

char*
doprint(char *s, char *es, char *fmt, void *argp)
{
	int f1, f2, f3, n, c;
	Rune rune;
	char *sout, *seout;

	sout = out;
	seout = eout;
	out = s;
	eout = es-4;		/* room for multi-byte character and 0 */

loop:
	c = *fmt & 0xff;
	if(c >= Runeself) {
		n = chartorune(&rune, fmt);
		fmt += n;
		c = rune;
	} else
		fmt++;
	switch(c) {
	case 0:
		*out = 0;
		s = out;
		out = sout;
		eout = seout;
		return s;
	
	default:
		printcol++;
		goto common;

	case '\n':
		printcol = 0;
		goto common;

	case '\t':
		printcol = (printcol+8) & ~7;
		goto common;

	common:
		if(out < eout)
			if(c >= Runeself) {
				rune = c;
				n = runetochar(out, &rune);
				out += n;
			} else
				*out++ = c;
		goto loop;

	case '%':
		break;
	}
	f1 = NONE;
	f2 = NONE;
	f3 = 0;

	/*
	 * read one of the following
	 *	1. number, => f1, f2 in order.
	 *	2. '*' same as number (from args)
	 *	3. '.' ignored (separates numbers)
	 *	4. flag => f3
	 *	5. verb and terminate
	 */
l0:
	c = *fmt & 0xff;
	if(c >= Runeself) {
		n = chartorune(&rune, fmt);
		fmt += n;
		c = rune;
	} else
		fmt++;

l1:
	if(c == 0) {
		fmt--;
		goto loop;
	}
	if(c == '.') {
		if(f1 == NONE)
			f1 = 0;
		f2 = 0;
		goto l0;
	}
	if((c >= '1' && c <= '9') ||
	   (c == '0' && f1 != NONE)) {	/* '0' is a digit for f2 */
		n = 0;
		while(c >= '0' && c <= '9') {
			n = n*10 + c-'0';
			c = *fmt++;
		}
		if(f1 == NONE)
			f1 = n;
		else
			f2 = n;
		goto l1;
	}
	if(c == '*') {
		n = *(int*)argp;
		argp = (char*)argp + INT;
		if(f1 == NONE)
			f1 = n;
		else
			f2 = n;
		goto l0;
	}
	n = 0;
	if(c >= 0 && c < MAXFMT)
		n = fmtindex[c];
	n = (*fmtconv[n])(argp, f1, f2, f3, c);
	if(n < 0) {
		f3 |= -n;
		goto l0;
	}
	argp = (char*)argp + n;
	goto loop;
}

int
numbconv(void *o, int f1, int f2, int f3, int chr)
{
	char s[IDIGIT];
	int i, f, n, r, b, ucase;
	short h;
	long v;
	vlong vl;

	SET(v);
	SET(vl);

	ucase = 0;
	b = chr;
	switch(chr) {
	case 'u':
		f3 |= FUNSIGN;
	case 'd':
		b = 10;
		break;

	case 'o':
		b = 8;
		break;

	case 'X':
		ucase = 1;
	case 'x':
		b = 16;
		break;
	}

	f = 0;
	switch(f3 & (FVLONG|FLONG|FSHORT|FUNSIGN)) {
	case FVLONG|FLONG:
		vl = *(vlong*)o;
		r = VLONG;
		break;

	case FLONG:
		v = *(long*)o;
		r = LONG;
		break;

	case FUNSIGN|FLONG:
		v = *(ulong*)o;
		r = LONG;
		break;

	case FSHORT:
		h = *(int*)o;
		v = h;
		r = SHORT;
		break;

	case FUNSIGN|FSHORT:
		h = *(int*)o;
		v = (ushort)h;
		r = SHORT;
		break;

	default:
		v = *(int*)o;
		r = INT;
		break;

	case FUNSIGN:
		v = *(unsigned*)o;
		r = INT;
		break;
	}
	if(!(f3 & FUNSIGN) && v < 0) {
		v = -v;
		f = 1;
	}
	s[IDIGIT-1] = 0;
	for(i = IDIGIT-2;; i--) {
		if(f3 & FVLONG)
			n = vl % b;
		else
			n = (ulong)v % b;
		n += '0';
		if(n > '9') {
			n += 'a' - ('9'+1);
			if(ucase)
				n += 'A'-'a';
		}
		s[i] = n;
		if(i < 2)
			break;
		if(f3 & FVLONG)
			vl = vl / b;
		else
			v = (ulong)v / b;
		if(f2 != NONE && i >= IDIGIT-f2)
			continue;
		if(f3 & FVLONG)
			if(vl <= 0)
				break;
		if(v <= 0)
			break;
	}
	if(f3 & FSHARP)
	if(s[i] != '0') {
		if(b == 8)
			s[--i] = '0';
		else
		if(b == 16) {
			if(ucase)
				s[--i] = 'X';
			else
				s[--i] = 'x';
			s[--i] = '0';
		}
	}
	if(f)
		s[--i] = '-';
	strconv(s+i, f1, NONE, f3);
	return r;
}

void
Strconv(Rune *s, int f1, int f2, int f3)
{
	int n, c, i;
	Rune rune;

	if(f3 & FMINUS)
		f1 = -f1;
	n = 0;
	if(f1 != NONE && f1 >= 0) {
		for(; s[n]; n++)
			;
		while(n < f1) {
			if(out < eout)
				*out++ = ' ';
			printcol++;
			n++;
		}
	}
	for(;;) {
		c = *s++;
		if(c == 0)
			break;
		n++;
		if(f2 == NONE || f2 > 0) {
			if(out < eout)
				if(c >= Runeself) {
					rune = c;
					i = runetochar(out, &rune);
					out += i;
				} else
					*out++ = c;
			if(f2 != NONE)
				f2--;
			switch(c) {
			default:
				printcol++;
				break;
			case '\n':
				printcol = 0;
				break;
			case '\t':
				printcol = (printcol+8) & ~7;
				break;
			}
		}
	}
	if(f1 != NONE && f1 < 0) {
		f1 = -f1;
		while(n < f1) {
			if(out < eout)
				*out++ = ' ';
			printcol++;
			n++;
		}
	}
}

void
strconv(char *s, int f1, int f2, int f3)
{
	int n, c, i;
	Rune rune;

	if(f3 & FMINUS)
		f1 = -f1;
	n = 0;
	if(f1 != NONE && f1 >= 0) {
		n = utflen(s);
		while(n < f1) {
			if(out < eout)
				*out++ = ' ';
			printcol++;
			n++;
		}
	}
	for(;;) {
		c = *s & 0xff;
		if(c >= Runeself) {
			i = chartorune(&rune, s);
			s += i;
			c = rune;
		} else
			s++;
		if(c == 0)
			break;
		n++;
		if(f2 == NONE || f2 > 0) {
			if(out < eout)
				if(c >= Runeself) {
					rune = c;
					i = runetochar(out, &rune);
					out += i;
				} else
					*out++ = c;
			if(f2 != NONE)
				f2--;
			switch(c) {
			default:
				printcol++;
				break;
			case '\n':
				printcol = 0;
				break;
			case '\t':
				printcol = (printcol+8) & ~7;
				break;
			}
		}
	}
	if(f1 != NONE && f1 < 0) {
		f1 = -f1;
		while(n < f1) {
			if(out < eout)
				*out++ = ' ';
			printcol++;
			n++;
		}
	}
}

static
int
noconv(void *o, int f1, int f2, int f3, int chr)
{
	int n;
	char s[10];

	if(convcount == 0) {
		initfmt();
		n = 0;
		if(chr >= 0 && chr < MAXFMT)
			n = fmtindex[chr];
		return (*fmtconv[n])(o, f1, f2, f3, chr);
	}
	sprint(s, "*%c*", chr);
	strconv(s, 0, NONE, 0);
	return 0;
}

static
int
rconv(void *o, int f1, int f2, int f3, int chr)
{
	char s[ERRLEN];

	USED(o);
	USED(f2);
	USED(chr);

	errstr(s);
	strconv(s, f1, NONE, f3);
	return 0;
}

static
int
cconv(void *o, int f1, int f2, int f3, int chr)
{
	char s[10];
	Rune rune;

	USED(f2);

	rune = *(int*)o;
	if(chr == 'c')
		rune &= 0xff;
	s[runetochar(s, &rune)] = 0;

	strconv(s, f1, NONE, f3);
	return INT;
}

static
int
sconv(void *o, int f1, int f2, int f3, int chr)
{
	char *s;
	Rune *r;

	if(chr == 's') {
		s = *(char**)o;
		if(s == 0)
			s = "<null>";
		strconv(s, f1, f2, f3);
	} else {
		r = *(Rune**)o;
		if(r == 0)
			r = L"<null>";
		Strconv(r, f1, f2, f3);
	}
	return PTR;
}

static
int
percent(void *o, int f1, int f2, int f3, int chr)
{

	USED(o);
	USED(f1);
	USED(f2);
	USED(f3);
	USED(chr);

	if(out < eout)
		*out++ = '%';
	return 0;
}

static
int
flags(void *o, int f1, int f2, int f3, int chr)
{
	int f;

	USED(o);
	USED(f1);
	USED(f2);

	f = 0;
	switch(chr) {
	case '+':
		f = FPLUS;
		break;

	case '-':
		f = FMINUS;
		break;

	case '#':
		f = FSHARP;
		break;

	case 'h':
		f = FSHORT;
		break;

	case 'l':
		f = FLONG;
		if(f3 & FLONG)
			f = FVLONG;
		break;

	case 'u':
		f = FUNSIGN;
		break;
	}
	return -f;
}

int
fltconv(void *o, int f1, int f2, int f3, int chr)
{
	char s1[FDIGIT+10], s2[FDIGIT+10];
	double f, g, h;
	int e, d, i, n, s;
	int c1, c2, c3, ucase;

	f = *(double*)o;
	if(isNaN(f)){
		strconv("NaN", f1, NONE, f3);
		return FLOAT;
	}
	if(isInf(f, 1)){
		strconv("+Inf", f1, NONE, f3);
		return FLOAT;
	}
	if(isInf(f, -1)){
		strconv("-Inf", f1, NONE, f3);
		return FLOAT;
	}
	s = 0;
	if(f < 0) {
		f = -f;
		s++;
	}
	ucase = 0;
	if(chr >= 'A' && chr <= 'Z') {
		ucase = 1;
		chr += 'a'-'A';
	}

loop:
	e = 0;
	if(f != 0) {
		frexp(f, &e);
		e = e * .30103;
		d = e/2;
		h = f * pow10(-d);		/* 10**-e in 2 parts */
		g = h * pow10(d-e);
		while(g < 1) {
			e--;
			g = h * pow10(d-e);
		}
		while(g >= 10) {
			e++;
			g = h * pow10(d-e);
		}
	}
	if(f2 == NONE)
		f2 = FDEFLT;
	if(chr == 'g' && f2 > 0)
		f2--;
	if(f2 > FDIGIT)
		f2 = FDIGIT;

	/*
	 * n is number of digits to convert
	 * 1 before, f2 after, 1 extra for rounding
	 */
	n = f2 + 2;
	if(chr == 'f') {

		/*
		 * e+1 before, f2 after, 1 extra
		 */
		n += e;
		if(n <= 0)
			n = 1;
	}
	if(n >= FDIGIT+2) {
		if(chr == 'e')
			f2 = -1;
		chr = 'e';
		goto loop;
	}

	/*
	 * convert n digits
	 */
	g = f;
	if(e < 0) {
		if(e < -55) {
			g *= pow10(50);
			g *= pow10(-e-51);
		} else
			g *= pow10(-e-1);
	}
	for(i=0; i<n; i++) {
		d = e-i;
		if(d >= 0) {
			h = pow10(d);
			d = floor(g/h);
			g -= d * h;
		} else {
			g *= 10;
			d = floor(g);
			g -= d;
		}
		s1[i+1] = d + '0';
	}

	/*
	 * round by adding .5 into extra digit
	 */
	d = 5;
	for(i=n-1; i>=0; i--) {
		s1[i+1] += d;
		d = 0;
		if(s1[i+1] > '9') {
			s1[i+1] -= 10;
			d++;
		}
	}
	i = 1;
	if(d) {
		s1[0] = '1';
		e++;
		i = 0;
	}

	/*
	 * copy into final place
	 * c1 digits of leading '0'
	 * c2 digits from conversion
	 * c3 digits after '.'
	 */
	d = 0;
	if(s)
		s2[d++] = '-';
	else
	if(f3 & FPLUS)
		s2[d++] = '+';
	c1 = 0;
	c2 = f2 + 1;
	c3 = f2;
	if(chr == 'g')
	if(e >= -5 && e <= f2) {
		c1 = -e - 1;
		if(c1 < 0)
			c1 = 0;
		c3 = f2 - e;
		chr = 'h';
	}
	if(chr == 'f') {
		c1 = -e;
		if(c1 < 0)
			c1 = 0;
		if(c1 > f2)
			c1 = c2;
		c2 += e;
		if(c2 < 0)
			c2 = 0;
	}
	while(c1 > 0) {
		if(c1+c2 == c3)
			s2[d++] = '.';
		s2[d++] = '0';
		c1--;
	}
	while(c2 > 0) {
		if(c1+c2 == c3)
			s2[d++] = '.';
		s2[d++] = s1[i++];
		c2--;
	}

	/*
	 * strip trailing '0' on g conv
	 */
	if(f3 & FSHARP) {
		if(c1+c2 == c3)
			s2[d++] = '.';
	} else
	if(chr == 'g' || chr == 'h') {
		for(n=d-1; n>=0; n--)
			if(s2[n] != '0')
				break;
		for(i=n; i>=0; i--)
			if(s2[i] == '.') {
				d = n;
				if(i != n)
					d++;
				break;
			}
	}
	if(chr == 'e' || chr == 'g') {
		if(ucase)
			s2[d++] = 'E';
		else
			s2[d++] = 'e';
		c1 = e;
		if(c1 < 0) {
			s2[d++] = '-';
			c1 = -c1;
		} else
			s2[d++] = '+';
		if(c1 >= 100) {
			s2[d++] = c1/100 + '0';
			c1 = c1%100;
		}
		s2[d++] = c1/10 + '0';
		s2[d++] = c1%10 + '0';
	}
	s2[d] = 0;
	strconv(s2, f1, NONE, f3);
	return FLOAT;
}
