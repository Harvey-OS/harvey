#include <alef.h>

enum
{
	NANEXP	= (2047<<20),
	NANMASK	= (2047<<20),
	NANSIGN = (1<<31),
};

union Cheat
{
	float	d;
	int	x[2];
};

float
NaN(void)
{
	Cheat a;

	a.x[1] = NANEXP;
	a.x[0] = 1;
	return a.d;
}

int
isNaN(float d)
{
	Cheat a;

	a.d = d;
	if((a.x[1] & NANMASK) != NANEXP)
		return 0;
	return !isInf(d, 0);
}

float
Inf(int sign)
{
	Cheat a;

	a.x[1] = NANEXP;
	a.x[0] = 0;
	if(sign < 0)
		a.x[1] |= NANSIGN;
	return a.d;
}

int
isInf(float d, int sign)
{
	Cheat a;

	a.d = d;
	if(a.x[0] != 0)
		return 0;
	if(a.x[1] == NANEXP)
		return sign >= 0;
	if(a.x[1] == (NANEXP|NANSIGN))
		return sign <= 0;
	return 0;
}
