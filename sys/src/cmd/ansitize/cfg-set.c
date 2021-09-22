#include "a.h"

/*
 * Bit sets
 */
Yset*
yynewset(uint n)
{
	Yset *s;
	int nw;

	nw = (n+YW-1)/YW;
	s = mallocz(sizeof(Yset)+(nw-1)*sizeof(uint), 1);
	s->n = nw;
	return s;
}

void
yyfreeset(Yset *s)
{
	free(s);
}

static int
bits(uint b)
{
	uint n;

	n = b;
	n = (n&0x55555555)+((n&0xAAAAAAAA)>>1);
	n = (n&0x33333333)+((n&0xCCCCCCCC)>>2);
	n = (n&0x0F0F0F0F)+((n&0xF0F0F0F0)>>4);
	n = (n&0x00FF00FF)+((n&0xFF00FF00)>>8);
	n = (n&0x0000FFFF)+((n&0xFFFF0000)>>16);
	return n;
}

int
yycountset(Yset *s)
{
	int i, n;

	n = 0;
	for(i=0; i<s->n; i++)
		n += bits(s->a[i]);
	return n;
}

int
yyaddset(Yset *s, Yset *t)
{
	int i, did;

	did = 0;
	for(i=0; i<s->n; i++){
		if(~s->a[i] & t->a[i])
			did = 1;
		s->a[i] |= t->a[i];
	}
	return did;
}

