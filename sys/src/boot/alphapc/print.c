#include	"u.h"
#include	"lib.h"

enum
{
	SIZE	= 1024,
	IDIGIT	= 30,
	MAXCONV	= 40,
	FDIGIT	= 30,
	FDEFLT	= 6,
	NONE	= -1000,
	MAXFMT	= 512,

	FPLUS	= 1<<0,
	FMINUS	= 1<<1,
	FSHARP	= 1<<2,
	FLONG	= 1<<3,
	FSHORT	= 1<<4,
	FUNSIGN	= 1<<5,
	FVLONG	= 1<<6,
};

int	printcol;

static	int	convcount;
static	char	fmtindex[MAXFMT];

static	int	noconv(va_list*, Fconv*);
static	int	flags(va_list*, Fconv*);

static	int	cconv(va_list*, Fconv*);
static	int	sconv(va_list*, Fconv*);
static	int	percent(va_list*, Fconv*);

int	numbconv(va_list*, Fconv*);

static
int	(*fmtconv[MAXCONV])(va_list*, Fconv*) =
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

	fmtconv[cc] = cconv;
	fmtindex['c'] = cc;
	fmtindex['C'] = cc;
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
fmtinstall(int c, int (*f)(va_list*, Fconv*))
{

	if(convcount == 0)
		initfmt();
	if(c < 0 || c >= MAXFMT)
		return -1;
	if(convcount >= MAXCONV)
		return -1;
	fmtconv[convcount] = f;
	fmtindex[c] = convcount;
	convcount++;
	return 0;
}

char*
doprint(char *s, char *es, char *fmt, va_list argp)
{
	int n, c;
	Rune rune;
	Fconv local;

	local.out = s;
	local.eout = es-UTFmax-1;

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
		if(local.out < local.eout)
			if(c >= Runeself) {
				rune = c;
				n = runetochar(local.out, &rune);
				local.out += n;
			} else
				*local.out++ = c;
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
		n = fmtindex[c];
	local.chr = c;
	n = (*fmtconv[n])(&argp, &local);
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
	short h;
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
	switch(fp->f3 & (FVLONG|FLONG|FSHORT|FUNSIGN)) {
	case FVLONG|FLONG:
		vl = va_arg(*arg, vlong);
		break;

	case FUNSIGN|FVLONG|FLONG:
		vl = va_arg(*arg, uvlong);
		break;

	case FLONG:
		v = va_arg(*arg, long);
		break;

	case FUNSIGN|FLONG:
		v = va_arg(*arg, ulong);
		break;

	case FSHORT:
		h = va_arg(*arg, int);
		v = h;
		break;

	case FUNSIGN|FSHORT:
		h = va_arg(*arg, int);
		v = (ushort)h;
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
	int n, c, i;
	Rune rune;

	if(fp->f3 & FMINUS)
		fp->f1 = -fp->f1;
	n = 0;
	if(fp->f1 != NONE && fp->f1 >= 0) {
		for(; s[n]; n++)
			;
		while(n < fp->f1) {
			if(fp->out < fp->eout)
				*fp->out++ = ' ';
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
			if(fp->out < fp->eout)
				if(c >= Runeself) {
					rune = c;
					i = runetochar(fp->out, &rune);
					fp->out += i;
				} else
					*fp->out++ = c;
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
			if(fp->out < fp->eout)
				*fp->out++ = ' ';
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
			if(fp->out < fp->eout)
				*fp->out++ = ' ';
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
			if(fp->out < fp->eout)
				if(c >= Runeself) {
					rune = c;
					i = runetochar(fp->out, &rune);
					fp->out += i;
				} else
					*fp->out++ = c;
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
			if(fp->out < fp->eout)
				*fp->out++ = ' ';
			printcol++;
			n++;
		}
	}
}

static
int
noconv(va_list *arg, Fconv *fp)
{
	int n;
	char s[10];

	if(convcount == 0) {
		initfmt();
		n = 0;
		if(fp->chr >= 0 && fp->chr < MAXFMT)
			n = fmtindex[fp->chr];
		return (*fmtconv[n])(arg, fp);
	}
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

static
int
cconv(va_list *arg, Fconv *fp)
{
	char s[10];
	Rune rune;

	rune = va_arg(*arg, int);
	if(fp->chr == 'c')
		rune &= 0xff;
	s[runetochar(s, &rune)] = 0;

	fp->f2 = NONE;
	strconv(s, fp);
	return 0;
}

static
int
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
			r = L"<null>";
		Strconv(r, fp);
	}
	return 0;
}

static
int
percent(va_list *arg, Fconv *fp)
{

	USED(arg);
	if(fp->out < fp->eout)
		*fp->out++ = '%';
	printcol++;
	return 0;
}

static
int
flags(va_list *arg, Fconv *fp)
{
	int f;

	USED(arg);

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

	case 'h':
		f = FSHORT;
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
