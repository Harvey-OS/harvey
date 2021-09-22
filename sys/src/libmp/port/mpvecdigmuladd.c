#include "os.h"
#include <mp.h>
#include "dat.h"

#define LO(x) ((x) & ((1ll<<(Dbits/2))-1))
#define HI(x) ((x) >> (Dbits/2))

static void
mpdigmul(mpdigit a, mpdigit b, mpdigit *p)
{
	mpdigit x, ah, al, bh, bl, p1, p2, p3, p4;
	int carry;

	// half digits
	ah = HI(a);
	al = LO(a);
	bh = HI(b);
	bl = LO(b);

	// partial products
	p1 = ah*bl;
	p2 = bh*al;
	p3 = bl*al;
	p4 = ah*bh;

	// p = ((p1+p2)<<(Dbits/2)) + (p4<<Dbits) + p3
	x = p1<<(Dbits/2);
	p3 += x;
	carry = p3 < x;
	x = p2<<(Dbits/2);
	p3 += x;
	carry += p3 < x;
	p4 += carry + HI(p1) + HI(p2);	// can't carry out of the high digit
	p[0] = p3;
	p[1] = p4;
}

// prereq: p must have room for n+1 digits
void
mpvecdigmuladd(mpdigit *b, int n, mpdigit m, mpdigit *p)
{
	int i;
	mpdigit carry, x, y, part[2];

	carry = 0;
	part[1] = 0;
	for(i = 0; i < n; i++){
		x = part[1] + carry;
		carry = x < carry;
		y = *p;
		mpdigmul(*b++, m, part);
		x += part[0];
		carry += x < part[0];
		x += y;
		carry += x < y;
		*p++ = x;
	}
	*p = part[1] + carry;
}

// prereq: p must have room for n+1 digits
int
mpvecdigmulsub(mpdigit *b, int n, mpdigit m, mpdigit *p)
{
	int i;
	mpdigit x, y, part[2], borrow;

	borrow = 0;
	part[1] = 0;
	for(i = 0; i < n; i++){
		x = *p;
		y = x - borrow;
		borrow = y > x;
		x = part[1];
		mpdigmul(*b++, m, part);
		x += part[0];
		borrow += x < part[0];
		x = y - x;
		borrow += x > y;
		*p++ = x;
	}

	x = *p;
	y = x - borrow - part[1];
	*p = y;
	if(y > x)
		return -1;
	else
		return 1;
}
