#include	<u.h>
#include	<libc.h>
#include	<ctype.h>

enum
{
	IDIGIT	= 40,
	MAXCONV	= 40,
	FDIGIT	= 30,
	FDEFLT	= 6,
	NONE	= -1000,
	MAXFMT	= 512,
	NSIGNIF	= 17,

	FPLUS	= 1<<0,
	FMINUS	= 1<<1,
	FSHARP	= 1<<2,
	FLONG	= 1<<3,
	FUNSIGN	= 1<<5,
	FVLONG	= 1<<6,
	FPOINTER= 1<<7,
};

static	int	xadd(char*, int, int);
static	int	xsub(char*, int, int);

int
fltconv(va_list *arg, Fconv *fp)
{
	char s1[NSIGNIF+10], s2[FDIGIT+10];
	double f, g, h;
	int e, d, i, n;
	int c1, c2, c3, c4, f2, ucase, sign;

	f2 = fp->f2;
	fp->f2 = NONE;

	f = va_arg(*arg, double);
	if(isNaN(f)) {
		strconv("NaN", fp);
		return 0;
	}
	if(isInf(f, 1)) {
		strconv("+Inf", fp);
		return 0;
	}
	if(isInf(f, -1)) {
		strconv("-Inf", fp);
		return 0;
	}
	sign = 0;
	if(f < 0) {
		f = -f;
		sign++;
	}
	ucase = 0;
	if(isupper(fp->chr)) {
		ucase = 1;
		fp->chr = tolower(fp->chr);
	}

	e = 0;
	g = f;
	if(g != 0) {
		frexp(f, &e);
		e = e * .301029995664;
		if(e >= -150 && e <= +150) {
			d = 0;
			h = f;
		} else {
			d = e/2;
			h = f * pow10(-d);
		}
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

	/*
	 * convert NSIGNIF digits and convert
	 * back to get accuracy.
	 */
	for(i=0; i<NSIGNIF; i++) {
		d = g;
		s1[i] = d + '0';
		g = (g - d) * 10;
	}
	s1[i] = 0;

	/*
	 * try decimal rounding to solve 9s problems
	 */
	c2 = f2 + 1;
	if(fp->chr == 'f')
		c2 += e;
	if(c2 >= NSIGNIF-2) {
		strcpy(s2, s1);
		d = e;
		s1[NSIGNIF-2] = '0';
		s1[NSIGNIF-1] = '0';
		sprint(s1+NSIGNIF, "e%d", e-NSIGNIF+1);
		g = strtod(s1, nil);
		if(g == f)
			goto found;
		if(xadd(s1, NSIGNIF-3, 1)) {
			e++;
			sprint(s1+NSIGNIF, "e%d", e-NSIGNIF+1);
		}
		g = strtod(s1, nil);
		if(g == f)
			goto found;
		strcpy(s1, s2);
		e = d;
	}

	/*
	 * convert back so s1 gets exact answer
	 */
	for(;;) {
		sprint(s1+NSIGNIF, "e%d", e-NSIGNIF+1);
		g = strtod(s1, nil);
		if(f > g) {
			if(xadd(s1, NSIGNIF-1, 1))
				e--;
			continue;
		}
		if(f < g) {
			if(xsub(s1, NSIGNIF-1, 1))
				e++;
			continue;
		}
		break;
	}

found:
	if(f2 == NONE)
		f2 = FDEFLT;
	if(f2 > FDIGIT)
		f2 = FDIGIT;

	/*
	 * sign
	 */
	d = 0;
	i = 0;
	if(sign)
		s2[d++] = '-';
	else
	if(fp->f3 & FPLUS)
		s2[d++] = '+';

	/*
	 * copy into final place
	 * c1 digits of leading '0'
	 * c2 digits from conversion
	 * c3 digits of trailing '0'
	 * c4 digits after '.'
	 */
	c1 = 0;
	c2 = f2 + 1;
	c3 = 0;
	c4 = f2;
	switch(fp->chr) {
	default:
		if(xadd(s1, c2, 5))
			e++;
		break;
	case 'g':
		/*
		 * decide on 'e' of 'f' style convers
		 */
		if(xadd(s1, c2, 5))
			e++;
		if(e >= -5 && e <= f2) {
			c1 = -e - 1;
			c4 = f2 - e;
			fp->chr = 'h';	// flag for 'f' style
		}
		break;
	case 'f':
		if(xadd(s1, c2+e, 5))
			e++;
		c1 = -e;
		if(c1 > f2)
			c1 = c2;
		c2 += e;
		break;
	}

	/*
	 * clean up c1 c2 and c3
	 */
	if(c1 < 0)
		c1 = 0;
	if(c2 < 0)
		c2 = 0;
	if(c2 > NSIGNIF) {
		c3 = c2-NSIGNIF;
		c2 = NSIGNIF;
	}

	/*
	 * copy digits
	 */
	while(c1 > 0) {
		if(c1+c2+c3 == c4)
			s2[d++] = '.';
		s2[d++] = '0';
		c1--;
	}
	while(c2 > 0) {
		if(c2+c3 == c4)
			s2[d++] = '.';
		s2[d++] = s1[i++];
		c2--;
	}
	while(c3 > 0) {
		if(c3 == c4)
			s2[d++] = '.';
		s2[d++] = '0';
		c3--;
	}

	/*
	 * strip trailing '0' on g conv
	 */
	if(fp->f3 & FSHARP) {
		if(0 == c4)
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

static int
xadd(char *a, int n, int v)
{
	char *b;
	int c;

	if(n < 0 || n >= NSIGNIF)
		return 0;
	for(b = a+n; b >= a; b--) {
		c = *b + v;
		if(c <= '9') {
			*b = c;
			return 0;
		}
		*b = '0';
		v = 1;
	}
	*a = '1';	// overflow adding
	return 1;
}

static int
xsub(char *a, int n, int v)
{
	char *b;
	int c;

	for(b = a+n; b >= a; b--) {
		c = *b - v;
		if(c >= '0') {
			*b = c;
			return 0;
		}
		*b = '9';
		v = 1;
	}
	*a = '9';	// underflow subtracting
	return 1;
}
