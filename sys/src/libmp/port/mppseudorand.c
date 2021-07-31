#include "os.h"
#include <mp.h>
#include "dat.h"

void
setupRC4state(RC4state *key, uchar *start, int n)
{
	int t;
	int index2;
	uchar *state;
	uchar *p, *e, *sp, *se;

	state = key->state;
	se = &state[256];
	for(sp = state; sp < se; sp++)
		*sp = sp - state;

	key->x = 0;
	key->y = 0;
	index2 = 0;
	e = start + n;
	p = start;
	for(sp = state; sp < se; sp++)
	{
		t = *sp;
		index2 = (*p + t + index2) & 255;
		*sp = state[index2];
		state[index2] = t;
		if(++p >= e)
			p = start;
	}
}

void
rc4(RC4state *key, uchar *p, int len)
{
	int tx, ty;
	int x, y;
	uchar *state;
	uchar *e;

	x = key->x;
	y = key->y;
	state = &key->state[0];
	for(e = p + len; p < e; p++)
	{
		x = (x+1)&255;
		tx = state[x];
		y = (y+tx)&255;
		ty = state[y];
		state[x] = ty;
		state[y] = tx;
		*p ^= state[(tx+ty)&255];
	}
	key->x = x;
	key->y = y;
}

void
rc4skip(RC4state *key, int len)
{
	int tx, ty;
	int x, y;
	uchar *state;
	int i;

	x = key->x;
	y = key->y;
	state = &key->state[0];
	for(i=0; i<len; i++)
	{
		x = (x+1)&255;
		tx = state[x];
		y = (y+tx)&255;
		ty = state[y];
		state[x] = ty;
		state[y] = tx;
	}
	key->x = x;
	key->y = y;
}

mpint*
mppseudorand(int bits, randstate)
{
	int i, m, n;
	mpint *b;

	b = mpnew(bits);

	n = DIGITS(bits)*Dbytes;
	b->top = n/Dbytes;

	return b;
}
