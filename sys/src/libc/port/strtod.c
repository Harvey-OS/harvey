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
#include <ctype.h>

/*
 * This routine will convert to arbitrary precision
 * floating point entirely in multi-precision fixed.
 * The answer is the closest floating point number to
 * the given decimal number. Exactly half way are
 * rounded ala ieee rules.
 * Method is to scale input decimal between .500 and .999...
 * with external power of 2, then binary search for the
 * closest mantissa to this decimal number.
 * Nmant is is the required precision. (53 for ieee dp)
 * Nbits is the max number of bits/word. (must be <= 28)
 * Prec is calculated - the number of words of fixed mantissa.
 */
enum
{
	Nbits	= 28,				// bits safely represented in a ulong
	Nmant	= 53,				// bits of precision required
	Bias		= 1022,
	Prec	= (Nmant+Nbits+1)/Nbits,	// words of Nbits each to represent mantissa
	Sigbit	= 1<<(Prec*Nbits-Nmant),	// first significant bit of Prec-th word
	Ndig	= 1500,
	One	= (uint32_t)(1<<Nbits),
	Half	= (uint32_t)(One>>1),
	Maxe	= 310,
	Fsign	= 1<<0,		// found -
	Fesign	= 1<<1,		// found e-
	Fdpoint	= 1<<2,		// found .

	S0	= 0,		// _		_S0	+S1	#S2	.S3
	S1,			// _+		#S2	.S3
	S2,			// _+#		#S2	.S4	eS5
	S3,			// _+.		#S4
	S4,			// _+#.#	#S4	eS5
	S5,			// _+#.#e	+S6	#S7
	S6,			// _+#.#e+	#S7
	S7,			// _+#.#e+#	#S7
};

static	int	xcmp(const char*, char*);
static	int	fpcmp(char*, uint32_t*);
static	void	frnorm(uint32_t*);
static	void	divascii(char*, int*, int*, int*);
static	void	mulascii(char*, int*, int*, int*);
static	void	divby(char*, int*, int);

typedef	struct	Tab	Tab;
struct	Tab
{
	int	bp;
	int	siz;
	char*	cmp;
};

double
strtod(const char *as, char **aas)
{
	int na, ona, ex, dp, bp, c, i, flag, state;
	uint32_t low[Prec], hig[Prec], mid[Prec], num, den;
	double d;
	const char *s;
	char a[Ndig];

	flag = 0;	// Fsign, Fesign, Fdpoint
	na = 0;		// number of digits of a[]
	dp = 0;		// na of decimal point
	ex = 0;		// exonent

	state = S0;
	for(s=as;; s++) {
		c = *s;
		if(c >= '0' && c <= '9') {
			switch(state) {
			case S0:
			case S1:
			case S2:
				state = S2;
				break;
			case S3:
			case S4:
				state = S4;
				break;

			case S5:
			case S6:
			case S7:
				state = S7;
				ex = ex*10 + (c-'0');
				continue;
			}
			if(na == 0 && c == '0') {
				dp--;
				continue;
			}
			if(na < Ndig-50)
				a[na++] = c;
			continue;
		}
		switch(c) {
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
			if(state == S0)
				continue;
			break;
		case '-':
			if(state == S0)
				flag |= Fsign;
			else
				flag |= Fesign;
		case '+':
			if(state == S0)
				state = S1;
			else
			if(state == S5)
				state = S6;
			else
				break;	// syntax
			continue;
		case '.':
			flag |= Fdpoint;
			dp = na;
			if(state == S0 || state == S1) {
				state = S3;
				continue;
			}
			if(state == S2) {
				state = S4;
				continue;
			}
			break;
		case 'e':
		case 'E':
			if(state == S2 || state == S4) {
				state = S5;
				continue;
			}
			break;
		}
		break;
	}

	/*
	 * clean up return char-pointer
	 */
	switch(state) {
	case S0:
		if(xcmp(s, "nan") == 0) {
			if(aas != nil)
				*aas = (char *)s+3;
			goto retnan;
		}
	case S1:
		if(xcmp(s, "infinity") == 0) {
			if(aas != nil)
				*aas = (char *)s+8;
			goto retinf;
		}
		if(xcmp(s, "inf") == 0) {
			if(aas != nil)
				*aas = (char *)s+3;
			goto retinf;
		}
	case S3:
		if(aas != nil)
			*aas = (char *)as;
		goto ret0;	// no digits found
	case S6:
		s--;		// back over +-
	case S5:
		s--;		// back over e
		break;
	}
	if(aas != nil)
		*aas = (char *)s;

	if(flag & Fdpoint)
	while(na > 0 && a[na-1] == '0')
		na--;
	if(na == 0)
		goto ret0;	// zero
	a[na] = 0;
	if(!(flag & Fdpoint))
		dp = na;
	if(flag & Fesign)
		ex = -ex;
	dp += ex;
	if(dp < -Maxe-Nmant/3)	/* actually -Nmant*log(2)/log(10), but Nmant/3 close enough */
		goto ret0;	// underflow by exp
	else
	if(dp > +Maxe)
		goto retinf;	// overflow by exp

	/*
	 * normalize the decimal ascii number
	 * to range .[5-9][0-9]* e0
	 */
	bp = 0;		// binary exponent
	while(dp > 0)
		divascii(a, &na, &dp, &bp);
	while(dp < 0 || a[0] < '5')
		mulascii(a, &na, &dp, &bp);
	a[na] = 0;

	/*
	 * very small numbers are represented using
	 * bp = -Bias+1.  adjust accordingly.
	 */
	if(bp < -Bias+1){
		ona = na;
		divby(a, &na, -bp-Bias+1);
		if(na < ona){
			memmove(a+ona-na, a, na);
			memset(a, '0', ona-na);
			na = ona;
		}
		a[na] = 0;
		bp = -Bias+1;
	}

	/* close approx by naive conversion */
	num = 0;
	den = 1;
	for(i=0; i<9 && (c=a[i]); i++) {
		num = num*10 + (c-'0');
		den *= 10;
	}
	low[0] = umuldiv(num, One, den);
	hig[0] = umuldiv(num+1, One, den);
	for(i=1; i<Prec; i++) {
		low[i] = 0;
		hig[i] = One-1;
	}

	/* binary search for closest mantissa */
	for(;;) {
		/* mid = (hig + low) / 2 */
		c = 0;
		for(i=0; i<Prec; i++) {
			mid[i] = hig[i] + low[i];
			if(c)
				mid[i] += One;
			c = mid[i] & 1;
			mid[i] >>= 1;
		}
		frnorm(mid);

		/* compare */
		c = fpcmp(a, mid);
		if(c > 0) {
			c = 1;
			for(i=0; i<Prec; i++)
				if(low[i] != mid[i]) {
					c = 0;
					low[i] = mid[i];
				}
			if(c)
				break;	// between mid and hig
			continue;
		}
		if(c < 0) {
			for(i=0; i<Prec; i++)
				hig[i] = mid[i];
			continue;
		}

		/* only hard part is if even/odd roundings wants to go up */
		c = mid[Prec-1] & (Sigbit-1);
		if(c == Sigbit/2 && (mid[Prec-1]&Sigbit) == 0)
			mid[Prec-1] -= c;
		break;	// exactly mid
	}

	/* normal rounding applies */
	c = mid[Prec-1] & (Sigbit-1);
	mid[Prec-1] -= c;
	if(c >= Sigbit/2) {
		mid[Prec-1] += Sigbit;
		frnorm(mid);
	}
	d = 0;
	for(i=0; i<Prec; i++)
		d = d*One + mid[i];
	if(flag & Fsign)
		d = -d;
	d = ldexp(d, bp - Prec*Nbits);
	return d;

ret0:
	return 0;

retnan:
	return NaN();

retinf:
	if(flag & Fsign)
		return Inf(-1);
	return Inf(+1);
}

static void
frnorm(uint32_t *f)
{
	int i, c;

	c = 0;
	for(i=Prec-1; i>0; i--) {
		f[i] += c;
		c = f[i] >> Nbits;
		f[i] &= One-1;
	}
	f[0] += c;
}

static int
fpcmp(char *a, uint32_t* f)
{
	uint32_t tf[Prec];
	int i, d, c;

	for(i=0; i<Prec; i++)
		tf[i] = f[i];

	for(;;) {
		/* tf *= 10 */
		for(i=0; i<Prec; i++)
			tf[i] = tf[i]*10;
		frnorm(tf);
		d = (tf[0] >> Nbits) + '0';
		tf[0] &= One-1;

		/* compare next digit */
		c = *a;
		if(c == 0) {
			if('0' < d)
				return -1;
			if(tf[0] != 0)
				goto cont;
			for(i=1; i<Prec; i++)
				if(tf[i] != 0)
					goto cont;
			return 0;
		}
		if(c > d)
			return +1;
		if(c < d)
			return -1;
		a++;
	cont:;
	}
}

static void
_divby(char *a, int *na, int b)
{
	int n, c;
	char *p;

	p = a;
	n = 0;
	while(n>>b == 0) {
		c = *a++;
		if(c == 0) {
			while(n) {
				c = n*10;
				if(c>>b)
					break;
				n = c;
			}
			goto xx;
		}
		n = n*10 + c-'0';
		(*na)--;
	}
	for(;;) {
		c = n>>b;
		n -= c<<b;
		*p++ = c + '0';
		c = *a++;
		if(c == 0)
			break;
		n = n*10 + c-'0';
	}
	(*na)++;
xx:
	while(n) {
		n = n*10;
		c = n>>b;
		n -= c<<b;
		*p++ = c + '0';
		(*na)++;
	}
	*p = 0;
}

static void
divby(char *a, int *na, int b)
{
	while(b > 9){
		_divby(a, na, 9);
		a[*na] = 0;
		b -= 9;
	}
	if(b > 0)
		_divby(a, na, b);
}

static	Tab	tab1[] =
{
	 1,  0, "",
	 3,  1, "7",
	 6,  2, "63",
	 9,  3, "511",
	13,  4, "8191",
	16,  5, "65535",
	19,  6, "524287",
	23,  7, "8388607",
	26,  8, "67108863",
	27,  9, "134217727",
};

static void
divascii(char *a, int *na, int *dp, int *bp)
{
	int b, d;
	Tab *t;

	d = *dp;
	if(d >= nelem(tab1))
		d = nelem(tab1)-1;
	t = tab1 + d;
	b = t->bp;
	if(memcmp(a, t->cmp, t->siz) > 0)
		d--;
	*dp -= d;
	*bp += b;
	divby(a, na, b);
}

static void
mulby(char *a, char *p, char *q, int b)
{
	int n, c;

	n = 0;
	*p = 0;
	for(;;) {
		q--;
		if(q < a)
			break;
		c = *q - '0';
		c = (c<<b) + n;
		n = c/10;
		c -= n*10;
		p--;
		*p = c + '0';
	}
	while(n) {
		c = n;
		n = c/10;
		c -= n*10;
		p--;
		*p = c + '0';
	}
}

static	Tab	tab2[] =
{
	 1,  1, "",				// dp = 0-0
	 3,  3, "125",
	 6,  5, "15625",
	 9,  7, "1953125",
	13, 10, "1220703125",
	16, 12, "152587890625",
	19, 14, "19073486328125",
	23, 17, "11920928955078125",
	26, 19, "1490116119384765625",
	27, 19, "7450580596923828125",		// dp 8-9
};

static void
mulascii(char *a, int *na, int *dp, int *bp)
{
	char *p;
	int d, b;
	Tab *t;

	d = -*dp;
	if(d >= nelem(tab2))
		d = nelem(tab2)-1;
	t = tab2 + d;
	b = t->bp;
	if(memcmp(a, t->cmp, t->siz) < 0)
		d--;
	p = a + *na;
	*bp -= b;
	*dp += d;
	*na += d;
	mulby(a, p+d, p, b);
}

static int
xcmp(const char *a, char *b)
{
	int c1, c2;

	while(c1 = *b++) {
		c2 = *a++;
		if(isupper(c2))
			c2 = tolower(c2);
		if(c1 != c2)
			return 1;
	}
	return 0;
}
