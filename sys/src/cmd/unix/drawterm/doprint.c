#include	"lib9.h"

enum
{
	IDIGIT	= 40,
	MAXCONV	= 40,
	FDIGIT	= 30,
	FDEFLT	= 6,
	NONE	= -1000,
	MAXFMT	= 512,

	FPLUS	= 1<<0,
	FMINUS	= 1<<1,
	FSHARP	= 1<<2,
	FLONG	= 1<<3,
	FUNSIGN	= 1<<5,
	FVLONG	= 1<<6,
	FPOINTER= 1<<7
};

int	printcol;

static struct
{
	Lock lk;
	int	convcount;
	char	index[MAXFMT];
	int	(*conv[MAXCONV])(va_list*, Fconv*);
} fmtalloc;

static	int	noconv(va_list*, Fconv*);
static	int	flags(va_list*, Fconv*);

static	int	cconv(va_list*, Fconv*);
static	int	rconv(va_list*, Fconv*);
static	int	sconv(va_list*, Fconv*);
static	int	percent(va_list*, Fconv*);
static	int	column(va_list*, Fconv*);

extern	int	numbconv(va_list*, Fconv*);
extern	int	fltconv(va_list*, Fconv*);


static	void
initfmt(void)
{
	int cc;

	lock(&fmtalloc.lk);
	if(fmtalloc.convcount <= 0) {
		cc = 0;
		fmtalloc.conv[cc] = noconv;
		cc++;

		fmtalloc.conv[cc] = flags;
		fmtalloc.index['+'] = cc;
		fmtalloc.index['-'] = cc;
		fmtalloc.index['#'] = cc;
		fmtalloc.index['l'] = cc;
		fmtalloc.index['u'] = cc;
		cc++;

		fmtalloc.conv[cc] = numbconv;
		fmtalloc.index['d'] = cc;
		fmtalloc.index['o'] = cc;
		fmtalloc.index['x'] = cc;
		fmtalloc.index['X'] = cc;
		fmtalloc.index['p'] = cc;
		cc++;

		fmtalloc.conv[cc] = fltconv;
		fmtalloc.index['e'] = cc;
		fmtalloc.index['f'] = cc;
		fmtalloc.index['g'] = cc;
		fmtalloc.index['E'] = cc;
		fmtalloc.index['G'] = cc;
		cc++;

		fmtalloc.conv[cc] = cconv;
		fmtalloc.index['c'] = cc;
		fmtalloc.index['C'] = cc;
		cc++;

		fmtalloc.conv[cc] = rconv;
		fmtalloc.index['r'] = cc;
		cc++;

		fmtalloc.conv[cc] = sconv;
		fmtalloc.index['s'] = cc;
		fmtalloc.index['S'] = cc;
		cc++;

		fmtalloc.conv[cc] = percent;
		fmtalloc.index['%'] = cc;
		cc++;

		fmtalloc.conv[cc] = column;
		fmtalloc.index['|'] = cc;
		cc++;

		fmtalloc.convcount = cc;
	}
	unlock(&fmtalloc.lk);
}

int
fmtinstall(int c, int (*f)(va_list*, Fconv*))
{

	if(fmtalloc.convcount <= 0)
		initfmt();

	lock(&fmtalloc.lk);
	if(c < 0 || c >= MAXFMT) {
		unlock(&fmtalloc.lk);
		return -1;
	}
	if(fmtalloc.convcount >= MAXCONV) {
		unlock(&fmtalloc.lk);
		return -1;
	}
	fmtalloc.conv[fmtalloc.convcount] = f;
	fmtalloc.index[c] = fmtalloc.convcount;
	fmtalloc.convcount++;

	unlock(&fmtalloc.lk);
	return 0;
}

static	void
pchar(Rune c, Fconv *fp)
{
	int n;

	n = fp->eout - fp->out;
	if(n > 0) {
		if(c < Runeself) {
			*fp->out++ = c;
			return;
		}
		if(n >= UTFmax || n >= runelen(c)) {
			n = runetochar(fp->out, &c);
			fp->out += n;
			return;
		}
		fp->eout = fp->out;
	}
}

char*
doprint(char *s, char *es, char *fmt, va_list argp)
{
	int n, c;
	Rune rune;
	Fconv local;

	if(fmtalloc.convcount <= 0)
		initfmt();

	if(s >= es)
		return s;
	local.out = s;
	local.eout = es-1;

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
		*local.out = 0;
		return local.out;
	
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
		pchar(c, &local);
		goto loop;

	case '%':
		break;
	}
	local.f1 = NONE;
	local.f2 = NONE;
	local.f3 = 0;

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
		if(local.f1 == NONE)
			local.f1 = 0;
		local.f2 = 0;
		goto l0;
	}
	if((c >= '1' && c <= '9') ||
	   (c == '0' && local.f1 != NONE)) {	/* '0' is a digit for f2 */
		n = 0;
		while(c >= '0' && c <= '9') {
			n = n*10 + c-'0';
			c = *fmt++;
		}
		if(local.f1 == NONE)
			local.f1 = n;
		else
			local.f2 = n;
		goto l1;
	}
	if(c == '*') {
		n = va_arg(argp, int);
		if(local.f1 == NONE)
			local.f1 = n;
		else
			local.f2 = n;
		goto l0;
	}
	n = 0;
	if(c >= 0 && c < MAXFMT)
		n = fmtalloc.index[c];
	local.chr = c;
	n = (*fmtalloc.conv[n])(&argp, &local);
	if(n < 0) {
		local.f3 |= -n;
		goto l0;
	}
	goto loop;
}

int
numbconv(va_list *arg, Fconv *fp)
{
	char s[IDIGIT];
	int i, f, n, b, ucase;
	long v;
	vlong vl;

	SET(v);
	SET(vl);

	ucase = 0;
	b = fp->chr;
	switch(fp->chr) {
	case 'u':
		fp->f3 |= FUNSIGN;
	case 'd':
		b = 10;
		break;

	case 'b':
		b = 2;
		break;

	case 'o':
		b = 8;
		break;

	case 'X':
		ucase = 1;
	case 'x':
		b = 16;
		break;
	case 'p':
		fp->f3 |= FPOINTER|FUNSIGN;
		b = 16;
		break;
	}

	f = 0;
	switch(fp->f3 & (FVLONG|FLONG|FUNSIGN|FPOINTER)) {
	case FVLONG|FLONG:
		vl = va_arg(*arg, vlong);
		break;

	case FUNSIGN|FVLONG|FLONG:
		vl = va_arg(*arg, uvlong);
		break;

	case FUNSIGN|FPOINTER:
		v = (ulong)va_arg(*arg, void*);
		break;

	case FLONG:
		v = va_arg(*arg, long);
		break;

	case FUNSIGN|FLONG:
		v = va_arg(*arg, ulong);
		break;

	default:
		v = va_arg(*arg, int);
		break;

	case FUNSIGN:
		v = va_arg(*arg, unsigned);
		break;
	}
	if(fp->f3 & FVLONG) {
		if(!(fp->f3 & FUNSIGN) && vl < 0) {
			vl = -vl;
			f = 1;
		}
	} else {
		if(!(fp->f3 & FUNSIGN) && v < 0) {
			v = -v;
			f = 1;
		}
	}
	s[IDIGIT-1] = 0;
	for(i = IDIGIT-2;; i--) {
		if(fp->f3 & FVLONG)
			n = (uvlong)vl % b;
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
		if(fp->f3 & FVLONG)
			vl = (uvlong)vl / b;
		else
			v = (ulong)v / b;
		if(fp->f2 != NONE && i >= IDIGIT-fp->f2)
			continue;
		if(fp->f3 & FVLONG) {
			if(vl <= 0)
				break;
			continue;
		}
		if(v <= 0)
			break;
	}

	if(fp->f3 & FSHARP) {
		if(b == 8 && s[i] != '0')
			s[--i] = '0';
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
	fp->f2 = NONE;
	strconv(s+i, fp);
	return 0;
}

void
Strconv(Rune *s, Fconv *fp)
{
	int n, c;

	if(fp->f3 & FMINUS)
		fp->f1 = -fp->f1;
	n = 0;
	if(fp->f1 != NONE && fp->f1 >= 0) {
		for(; s[n]; n++)
			;
		while(n < fp->f1) {
			pchar(' ', fp);
			printcol++;
			n++;
		}
	}
	for(;;) {
		c = *s++;
		if(c == 0)
			break;
		n++;
		if(fp->f2 == NONE || fp->f2 > 0) {
			pchar(c, fp);
			if(fp->f2 != NONE)
				fp->f2--;
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
	if(fp->f1 != NONE && fp->f1 < 0) {
		fp->f1 = -fp->f1;
		while(n < fp->f1) {
			pchar(' ', fp);
			printcol++;
			n++;
		}
	}
}

void
strconv(char *s, Fconv *fp)
{
	int n, c, i;
	Rune rune;

	if(fp->f3 & FMINUS)
		fp->f1 = -fp->f1;
	n = 0;
	if(fp->f1 != NONE && fp->f1 >= 0) {
		n = utflen(s);
		while(n < fp->f1) {
			pchar(' ', fp);
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
		if(fp->f2 == NONE || fp->f2 > 0) {
			pchar(c, fp);
			if(fp->f2 != NONE)
				fp->f2--;
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
	if(fp->f1 != NONE && fp->f1 < 0) {
		fp->f1 = -fp->f1;
		while(n < fp->f1) {
			pchar(' ', fp);
			printcol++;
			n++;
		}
	}
}

static int
noconv(va_list *o, Fconv *fp)
{
	char s[10];

	USED(o);
	s[0] = '*';
	s[1] = fp->chr;
	s[2] = '*';
	s[3] = 0;
	fp->f1 = 0;
	fp->f2 = NONE;
	fp->f3 = 0;
	strconv(s, fp);
	return 0;
}

static int
rconv(va_list *o, Fconv *fp)
{
	char s[ERRLEN];

	USED(o);
	s[0] = 0;
	errstr(s);
	fp->f2 = NONE;
	strconv(s, fp);
	return 0;
}

static int
cconv(va_list *arg, Fconv *fp)
{
	char s[10];
	Rune rune;

	rune = va_arg(*arg, Rune);
	if(fp->chr == 'c')
		rune &= 0xff;
	s[runetochar(s, &rune)] = 0;

	fp->f2 = NONE;
	strconv(s, fp);
	return 0;
}

static	int
sconv(va_list *arg, Fconv *fp)
{
	char *s;
	Rune *r;

	if(fp->chr == 's') {
		s = va_arg(*arg, char*);
		if(s == 0)
			s = "<null>";
		strconv(s, fp);
	} else {
		r = va_arg(*arg, Rune*);
		if(r == 0)
			r = (Rune*)L"<null>";
		Strconv(r, fp);
	}
	return 0;
}

static	int
percent(va_list *o, Fconv *fp)
{
	USED(o);

	pchar('%', fp);
	printcol++;
	return 0;
}

static	int
column(va_list *arg, Fconv *fp)
{
	int col, pc;

	col = va_arg(*arg, int);
	while(printcol < col) {
		pc = (printcol+8) & ~7;
		if(pc <= col) {
			pchar('\t', fp);
			printcol = pc;
		} else {
			pchar(' ', fp);
			printcol++;
		}
	}
	return 0;
}

static	int
flags(va_list *o, Fconv *fp)
{
	int f;

	USED(o);
	f = 0;
	switch(fp->chr) {
	case '+':
		f = FPLUS;
		break;

	case '-':
		f = FMINUS;
		break;

	case '#':
		f = FSHARP;
		break;

	case 'l':
		f = FLONG;
		if(fp->f3 & FLONG)
			f = FVLONG;
		break;

	case 'u':
		f = FUNSIGN;
		break;
	}
	return -f;
}

int
fltconv(va_list *arg, Fconv *fp)
{
	char s1[FDIGIT+10], s2[FDIGIT+10];
	double f, g, h;
	int e, d, i, n, s;
	int c1, c2, c3, f2, ucase;

	f2 = fp->f2;
	fp->f2 = NONE;

	f = va_arg(*arg, double);
	if(isNaN(f)){
		strconv("NaN", fp);
		return 0;
	}
	if(isInf(f, 1)){
		strconv("+Inf", fp);
		return 0;
	}
	if(isInf(f, -1)){
		strconv("-Inf", fp);
		return 0;
	}
	s = 0;
	if(f < 0) {
		f = -f;
		s++;
	}
	ucase = 0;
	if(fp->chr >= 'A' && fp->chr <= 'Z') {
		ucase = 1;
		fp->chr += 'a'-'A';
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
	if(fp->chr == 'g' && f2 > 0)
		f2--;
	if(f2 > FDIGIT)
		f2 = FDIGIT;

	/*
	 * n is number of digits to convert
	 * 1 before, f2 after, 1 extra for rounding
	 */
	n = f2 + 2;
	if(fp->chr == 'f') {

		/*
		 * e+1 before, f2 after, 1 extra
		 */
		n += e;
		if(n <= 0)
			n = 1;
	}
	if(n >= FDIGIT+2) {
		if(fp->chr == 'e')
			f2 = -1;
		fp->chr = 'e';
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
	if(fp->f3 & FPLUS)
		s2[d++] = '+';
	c1 = 0;
	c2 = f2 + 1;
	c3 = f2;
	if(fp->chr == 'g')
	if(e >= -5 && e <= f2) {
		c1 = -e - 1;
		if(c1 < 0)
			c1 = 0;
		c3 = f2 - e;
		fp->chr = 'h';
	}
	if(fp->chr == 'f') {
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
	if(fp->f3 & FSHARP) {
		if(c1+c2 == c3)
			s2[d++] = '.';
	} else
	if(fp->chr == 'g' || fp->chr == 'h') {
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
	if(fp->chr == 'e' || fp->chr == 'g') {
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
	strconv(s2, fp);
	return 0;
}
