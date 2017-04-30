/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"all.h"

#define	PTR	sizeof(char*)
#define	SHORT	sizeof(int)
#define	INT	sizeof(int)
#define	LONG	sizeof(int32_t)
#define	IDIGIT	30
#define	MAXCON	30

static	int	convcount  = { 10 };

#define	PUT(o, c)	if((o)->p < (o)->ep) *(o)->p++ = c

static	int	noconv(Op*);
static	int	cconv(Op*);
static	int	dconv(Op*);
static	int	hconv(Op*);
static	int	lconv(Op*);
static	int	oconv(Op*);
static	int	sconv(Op*);
static	int	uconv(Op*);
static	int	xconv(Op*);
static	int	percent(Op*);

static
int	(*fmtconv[MAXCON])(Op*) =
{
	noconv,
	cconv, dconv, hconv, lconv,
	oconv, sconv, uconv, xconv,
	percent,
};
static
char	fmtindex[128] =
{
	['c'] = 1,
	['d'] = 2,
	['h'] = 3,
	['l'] = 4,
	['o'] = 5,
	['s'] = 6,
	['u'] = 7,
	['x'] = 8,
	['%'] = 9,
};

int
fmtinstall(char c, int (*f)(Op*))
{

	c &= 0177;
	if(fmtindex[c] == 0) {
		if(convcount >= MAXCON)
			return 1;
		fmtindex[c] = convcount++;
	}
	fmtconv[fmtindex[c]] = f;
	return 0;
}

char*
doprint(char *p, char *ep, char *fmt, void *argp)
{
	int sf1, c;
	Op o;

	o.p = p;
	o.ep = ep;
	o.argp = argp;

loop:
	c = *fmt++;
	if(c != '%') {
		if(c == 0) {
			if(o.p < o.ep)
				*o.p = 0;
			return o.p;
		}
		PUT(&o, c);
		goto loop;
	}
	o.f1 = 0;
	o.f2 = -1;
	o.f3 = 0;
	c = *fmt++;
	sf1 = 0;
	if(c == '-') {
		sf1 = 1;
		c = *fmt++;
	}
	while(c >= '0' && c <= '9') {
		o.f1 = o.f1*10 + c-'0';
		c = *fmt++;
	}
	if(sf1)
		o.f1 = -o.f1;
	if(c != '.')
		goto l1;
	c = *fmt++;
	while(c >= '0' && c <= '9') {
		if(o.f2 < 0)
			o.f2 = 0;
		o.f2 = o.f2*10 + c-'0';
		c = *fmt++;
	}
l1:
	if(c == 0)
		fmt--;
	c = (*fmtconv[fmtindex[c&0177]])(&o);
	if(c < 0) {
		o.f3 |= -c;
		c = *fmt++;
		goto l1;
	}
	o.argp = (char*)o.argp + c;
	goto loop;
}

int
numbconv(Op *op, int base)
{
	char b[IDIGIT];
	int i, f, n, r;
	int32_t v;
	int16_t h;

	f = 0;
	switch(op->f3 & (FLONG|FSHORT|FUNSIGN)) {
	case FLONG:
		v = *(int32_t*)op->argp;
		r = LONG;
		break;

	case FUNSIGN|FLONG:
		v = *(uint32_t*)op->argp;
		r = LONG;
		break;

	case FSHORT:
		h = *(int*)op->argp;
		v = h;
		r = SHORT;
		break;

	case FUNSIGN|FSHORT:
		h = *(int*)op->argp;
		v = (uint16_t)h;
		r = SHORT;
		break;

	default:
		v = *(int*)op->argp;
		r = INT;
		break;

	case FUNSIGN:
		v = *(unsigned*)op->argp;
		r = INT;
		break;
	}
	if(!(op->f3 & FUNSIGN) && v < 0) {
		v = -v;
		f = 1;
	}
	b[IDIGIT-1] = 0;
	for(i = IDIGIT-2;; i--) {
		n = (uint32_t)v % base;
		n += '0';
		if(n > '9')
			n += 'a' - ('9'+1);
		b[i] = n;
		if(i < 2)
			break;
		v = (uint32_t)v / base;
		if(op->f2 >= 0 && i >= IDIGIT-op->f2)
			continue;
		if(v <= 0)
			break;
	}
sout:
	if(f)
		b[--i] = '-';
	strconv(b+i, op, op->f1, -1);
	return r;
}

void
strconv(char *o, Op *op, int f1, int f2)
{
	int n, c;
	char *p;

	n = strlen(o);
	if(f1 >= 0)
		while(n < f1) {
			PUT(op, ' ');
			n++;
		}
	for(p=o; c = *p++;)
		if(f2 != 0) {
			PUT(op, c);
			f2--;
		}
	if(f1 < 0) {
		f1 = -f1;
		while(n < f1) {
			PUT(op, ' ');
			n++;
		}
	}
}

static	int
noconv(Op *op)
{

	strconv("***", op, 0, -1);
	return 0;
}

static	int
cconv(Op *op)
{
	char b[2];

	b[0] = *(int*)op->argp;
	b[1] = 0;
	strconv(b, op, op->f1, -1);
	return INT;
}

static	int
dconv(Op *op)
{

	return numbconv(op, 10);
}

static	int
hconv(Op *op)
{
	USED(op);
	return -FSHORT;
}

static	int
lconv(Op *op)
{
	USED(op);
	return -FLONG;
}

static	int
oconv(Op *op)
{
	USED(op);
	return numbconv(op, 8);
}

static	int
sconv(Op *op)
{

	strconv(*(char**)op->argp, op, op->f1, op->f2);
	return PTR;
}

static	int
uconv(Op *op)
{
	USED(op);
	return -FUNSIGN;
}

static	int
xconv(Op *op)
{

	return numbconv(op, 16);
}

static	int
percent(Op *op)
{

	PUT(op, '%');
	return 0;
}
