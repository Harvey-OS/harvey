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
			return 32*i + bitno(b);
	diag(Z, "bad in bnum");
	return 0;
}

Bits
blsh(uint n)
{
	Bits c;

	c = zbits;
	c.b[n/32] = 1L << (n%32);
	return c;
}

int
bset(Bits a, uint n)
{
	if(a.b[n/32] & (1L << (n%32)))
		return 1;
	return 0;
}
