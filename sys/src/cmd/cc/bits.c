#include	"cc.h"

Bits
bor(Bits a, Bits b)
{
	Bits c;
	int i;

	for(i=0; i<BITS; i++)
		c.b[i] = a.b[i] | b.b[i];
	return c;
}

Bits
band(Bits a, Bits b)
{
	Bits c;
	int i;

	for(i=0; i<BITS; i++)
		c.b[i] = a.b[i] & b.b[i];
	return c;
}

/*
Bits
bnot(Bits a)
{
	Bits c;
	int i;

	for(i=0; i<BITS; i++)
		c.b[i] = ~a.b[i];
	return c;
}
*/

int
bany(Bits *a)
{
	int i;

	for(i=0; i<BITS; i++)
		if(a->b[i])
			return 1;
	return 0;
}

int
beq(Bits a, Bits b)
{
	int i;

	for(i=0; i<BITS; i++)
		if(a.b[i] != b.b[i])
			return 0;
	return 1;
}

int
bnum(Bits a)
{
	int i;
	long b;

	for(i=0; i<BITS; i++)
		if(b = a.b[i])
			return BI2LONG*i + bitno(b);
	diag(Z, "bad in bnum");
	return 0;
}

Bits
blsh(uint n)
{
	Bits c;

	c = zbits;
	assert(n/BI2LONG < BITS);
	c.b[n/BI2LONG] = 1L << (n%BI2LONG);
	return c;
}

int
bset(Bits a, uint n)
{
	assert(n/BI2LONG < BITS);
	if(a.b[n/BI2LONG] & (1L << (n%BI2LONG)))
		return 1;
	return 0;
}
