#define	EXTERN
#include "gc.h"

/*
Bits
bor(Bits a, Bits b)
{
	Bits c;
	int i;

	for(i=0; i<BITS; i++)
		c.b[i] = a.b[i] | b.b[i];
	return c;
}
*/

/*
Bits
band(Bits a, Bits b)
{
	Bits c;
	int i;

	for(i=0; i<BITS; i++)
		c.b[i] = a.b[i] & b.b[i];
	return c;
}
*/

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

/*
int
beq(Bits a, Bits b)
{
	int i;

	for(i=0; i<BITS; i++)
		if(a.b[i] != b.b[i])
			return 0;
	return 1;
}
*/

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
blsh(unsigned n)
{
	Bits c;

	c = zbits;
	c.b[n/32] = 1L << (n%32);
	return c;
}

/*
int
bset(Bits a, unsigned n)
{
	int i;

	if(a.b[n/32] & (1L << (n%32)))
		return 1;
	return 0;
}
*/

int
Bconv(va_list *arg, Fconv *fp)
{
	char str[STRINGSZ], ss[STRINGSZ], *s;
	Bits bits;
	int i;

	str[0] = 0;
	bits = va_arg(*arg, Bits);
	while(bany(&bits)) {
		i = bnum(bits);
		if(str[0])
			strcat(str, " ");
		if(var[i].sym == S) {
			sprint(ss, "$%ld", var[i].offset);
			s = ss;
		} else
			s = var[i].sym->name;
		if(strlen(str) + strlen(s) + 1 >= STRINGSZ)
			break;
		strcat(str, s);
		bits.b[i/32] &= ~(1L << (i%32));
	}
	strconv(str, fp);
	return 0;
}
