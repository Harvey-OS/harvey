#include "os.h"
#include <mp.h>
#include "dat.h"

// for converting between int's and mpint's
#define MAXUINT ((uint)-1)
#define MAXINT (MAXUINT>>1)
#define MININT (MAXINT+1)

static mpdigit _mptwodata[1] = { 2 };
static mpint _mptwo =
{
	1,
	1,
	1,
	_mptwodata,
	MPstatic
};
mpint *mptwo = &_mptwo;

static mpdigit _mponedata[1] = { 1 };
static mpint _mpone =
{
	1,
	1,
	1,
	_mponedata,
	MPstatic
};
mpint *mpone = &_mpone;

static mpdigit _mpzerodata[1] = { 0 };
static mpint _mpzero =
{
	1,
	1,
	0,
	_mpzerodata,
	MPstatic
};
mpint *mpzero = &_mpzero;

static int mpmindigits = 33;

// set minimum digit allocation
void
mpsetminbits(int n)
{
	if(n == 0)
		n = 1;
	mpmindigits = DIGITS(n);
}

// allocate an n bit 0'd number 
mpint*
mpnew(int n)
{
	mpint *b;

	b = mallocz(sizeof(mpint), 1);
	if(b == nil)
		sysfatal("mpnew");
	n = DIGITS(n);
	if(n < mpmindigits)
		n = mpmindigits;
	n = n;
	b->p = (mpdigit*)mallocz(n*Dbytes, 1);
	if(b->p == nil)
		sysfatal("mpnew");
	b->size = n;
	b->sign = 1;

	return b;
}

// guarantee at least n significant bits
void
mpbits(mpint *b, int m)
{
	int n;

	n = DIGITS(m);
	if(b->size >= n){
		if(b->top >= n){
			b->top = n;
			return;
		}
		memset(&b->p[b->top], 0, Dbytes*(n - b->top));
		b->top = n;
		return;
	}
	b->p = (mpdigit*)realloc(b->p, n*Dbytes);
	if(b->p == nil)
		sysfatal("mpbits");
	memset(&b->p[b->top], 0, Dbytes*(n - b->top));
	b->size = n;
	b->top = n;
}

void
mpfree(mpint *b)
{
	if(b == nil)
		return;
	if(b->flags & MPstatic)
		sysfatal("freeing mp constant");
	memset(b->p, 0, b->top*Dbytes);	// information hiding
	free(b->p);
	free(b);
}

void
mpnorm(mpint *b)
{
	int i;

	for(i = b->top-1; i >= 0; i--)
		if(b->p[i] != 0)
			break;
	b->top = i+1;
	if(b->top == 0)
		b->sign = 1;
}

mpint*
itomp(int i, mpint *b)
{
	if(b == nil)
		b = mpnew(0);
	mpassign(mpzero, b);
	if(i != 0)
		b->top = 1;
	if(i < 0){
		b->sign = -1;
		*b->p = -i;
	} else
		*b->p = i;
	return b;
}

int
mptoi(mpint *b)
{
	uint x;

	x = *b->p;
	if(b->sign > 0){
		if(b->top > 1 || (x > MAXINT))
			x = (int)MAXINT;
		else
			x = (int)x;
	} else {
		if(b->top > 1 || x > MAXINT+1)
			x = (int)MININT;
		else
			x = -(int)x;
	}
	return x;
}

mpint*
uitomp(uint i, mpint *b)
{
	if(b == nil)
		b = mpnew(0);
	mpassign(mpzero, b);
	if(i != 0)
		b->top = 1;
	*b->p = i;
	return b;
}

uint
mptoui(mpint *b)
{
	uint x;

	x = *b->p;
	if(b->sign < 0){
		x = 0;
	} else {
		if(b->top > 1 || x > MAXUINT)
			x =  MAXUINT;
	}
	return x;
}

mpint*
mpcopy(mpint *old)
{
	mpint *new;

	new = mpnew(Dbits*old->size);
	new->top = old->top;
	new->sign = old->sign;
	memmove(new->p, old->p, Dbytes*old->top);
	return new;
}

void
mpassign(mpint *old, mpint *new)
{
	mpbits(new, Dbits*old->top);
	new->sign = old->sign;
	memmove(new->p, old->p, Dbytes*old->top);
}

// number of significant bits in mantissa
int
mpsignif(mpint *n)
{
	int i, j;
	mpdigit d;

	if(n->top == 0)
		return 0;
	for(i = n->top-1; i >= 0; i--){
		d = n->p[i];
		for(j = Dbits-1; j >= 0; j--){
			if(d & (((mpdigit)1)<<j))
				return i*Dbits + j + 1;
		}
	}
	return 0;
}

// k, where n = 2**k * q for odd q
int
mplowbits0(mpint *n)
{
	int k, bit, digit;
	mpdigit d;

	if(n->top==0)
		return 0;
	k = 0;
	bit = 0;
	digit = 0;
	d = n->p[0];
	for(;;){
		if(d & (1<<bit))
			break;
		k++;
		bit++;
		if(bit==Dbits){
			if(++digit >= n->top)
				return 0;
			d = n->p[digit];
			bit = 0;
		}
	}
	return k;
}

