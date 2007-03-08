#include	"u.h"
#include	"lib.h"

#define	PTR	sizeof(char*)
#define	SHORT	sizeof(int)
#define	INT	sizeof(int)
#define	LONG	sizeof(long)
#define	IDIGIT	30
#define	MAXCON	30

#define	FLONG	(1<<0)
#define	FSHORT	(1<<1)
#define	FUNSIGN	(1<<2)

typedef struct Op	Op;
struct Op
{
	char	*p;
	char	*ep;
	void	*argp;
	int	f1;
	int	f2;
	int	f3;
};

static	int	noconv(Op*);
static	int	cconv(Op*);
static	int	dconv(Op*);
static	int	hconv(Op*);
static	int	lconv(Op*);
static	int	oconv(Op*);
static	int	sconv(Op*);
static	int	uconv(Op*);
static	int	xconv(Op*);
static	int	Xconv(Op*);
static	int	percent(Op*);

static
int	(*fmtconv[MAXCON])(Op*) =
{
	noconv,
	cconv, dconv, hconv, lconv,
	oconv, sconv, uconv, xconv,
	Xconv, percent,
};
static
char	fmtindex[128] =
{
	['c'] 1,
	['d'] 2,
	['h'] 3,
	['l'] 4,
	['o'] 5,
	['s'] 6,
	['u'] 7,
	['x'] 8,
	['X'] 9,
	['%'] 10,
};

static	int	convcount  = { 11 };
static	int	ucase;

static void
PUT(Op *o, int c)
{
	static int pos;
	int opos;

	if(c == '\t'){
		opos = pos;
		pos = (opos+8) & ~7;
		while(opos++ < pos && o->p < o->ep)
			*o->p++ = ' ';
		return;
	}
	if(o->p < o->ep){
		*o->p++ = c;
		pos++;
	}
	if(c == '\n')
		pos = 0;
}

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
donprint(char *p, char *ep, char *fmt, void *argp)
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

int
numbconv(Op *op, int base)
{
	char b[IDIGIT];
	int i, f, n, r;
	long v;
	short h;

	f = 0;
	switch(op->f3 & (FLONG|FSHORT|FUNSIGN)) {
	case FLONG:
		v = *(long*)op->argp;
		r = LONG;
		break;

	case FUNSIGN|FLONG:
		v = *(ulong*)op->argp;
		r = LONG;
		break;

	case FSHORT:
		h = *(int*)op->argp;
		v = h;
		r = SHORT;
		break;

	case FUNSIGN|FSHORT:
		h = *(int*)op->argp;
		v = (ushort)h;
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
		n = (ulong)v % base;
		n += '0';
		if(n > '9'){
			n += 'a' - ('9'+1);
			if(ucase)
				n += 'A'-'a';
		}
		b[i] = n;
		if(i < 2)
			break;
		v = (ulong)v / base;
		if(op->f2 >= 0 && i >= IDIGIT-op->f2)
			continue;
		if(v <= 0)
			break;
	}
	if(f)
		b[--i] = '-';
	strconv(b+i, op, op->f1, -1);
	return r;
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
hconv(Op*)
{
	return -FSHORT;
}

static	int
lconv(Op*)
{
	return -FLONG;
}

static	int
oconv(Op *op)
{
	return numbconv(op, 8);
}

static	int
sconv(Op *op)
{
	strconv(*(char**)op->argp, op, op->f1, op->f2);
	return PTR;
}

static	int
uconv(Op*)
{
	return -FUNSIGN;
}

static	int
xconv(Op *op)
{
	return numbconv(op, 16);
}

static	int
Xconv(Op *op)
{
	int r;

	ucase = 1;
	r = numbconv(op, 16);
	ucase = 0;
	return r;
}

static	int
percent(Op *op)
{

	PUT(op, '%');
	return 0;
}
