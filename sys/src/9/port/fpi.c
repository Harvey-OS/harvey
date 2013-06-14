/*
 * Floating Point Interpreter.
 * shamelessly stolen from an original by ark.
 *
 * NB: the Internal arguments to fpisub and fpidiv are reversed from
 * what you might naively expect: they compute y-x and y/x, respectively.
 */
#include "fpi.h"

void
fpiround(Internal *i)
{
	unsigned long guard;

	guard = i->l & GuardMask;
	i->l &= ~GuardMask;
	if(guard > (LsBit>>1) || (guard == (LsBit>>1) && (i->l & LsBit))){
		i->l += LsBit;
		if(i->l & CarryBit){
			i->l &= ~CarryBit;
			i->h++;
			if(i->h & CarryBit){
				if (i->h & 0x01)
					i->l |= CarryBit;
				i->l >>= 1;
				i->h >>= 1;
				i->e++;
			}
		}
	}
}

static void
matchexponents(Internal *x, Internal *y)
{
	int count;

	count = y->e - x->e;
	x->e = y->e;
	if(count >= 2*FractBits){
		x->l = x->l || x->h;
		x->h = 0;
		return;
	}
	if(count >= FractBits){
		count -= FractBits;
		x->l = x->h|(x->l != 0);
		x->h = 0;
	}
	while(count > 0){
		count--;
		if(x->h & 0x01)
			x->l |= CarryBit;
		if(x->l & 0x01)
			x->l |= 2;
		x->l >>= 1;
		x->h >>= 1;
	}
}

static void
shift(Internal *i)
{
	i->e--;
	i->h <<= 1;
	i->l <<= 1;
	if(i->l & CarryBit){
		i->l &= ~CarryBit;
		i->h |= 0x01;
	}
}

static void
normalise(Internal *i)
{
	while((i->h & HiddenBit) == 0)
		shift(i);
}

static void
renormalise(Internal *i)
{
	if(i->e < -2 * FractBits)
		i->e = -2 * FractBits;
	while(i->e < 1){
		i->e++;
		if(i->h & 0x01)
			i->l |= CarryBit;
		i->h >>= 1;
		i->l = (i->l>>1)|(i->l & 0x01);
	}
	if(i->e >= ExpInfinity)
		SetInfinity(i);
}

void
fpinormalise(Internal *x)
{
	if(!IsWeird(x) && !IsZero(x))
		normalise(x);
}

void
fpiadd(Internal *x, Internal *y, Internal *i)
{
	Internal *t;

	i->s = x->s;
	if(IsWeird(x) || IsWeird(y)){
		if(IsNaN(x) || IsNaN(y))
			SetQNaN(i);
		else
			SetInfinity(i);
		return;
	}
	if(x->e > y->e){
		t = x;
		x = y;
		y = t;
	}
	matchexponents(x, y);
	i->e = x->e;
	i->h = x->h + y->h;
	i->l = x->l + y->l;
	if(i->l & CarryBit){
		i->h++;
		i->l &= ~CarryBit;
	}
	if(i->h & (HiddenBit<<1)){
		if(i->h & 0x01)
			i->l |= CarryBit;
		i->l = (i->l>>1)|(i->l & 0x01);
		i->h >>= 1;
		i->e++;
	}
	if(IsWeird(i))
		SetInfinity(i);
}

void
fpisub(Internal *x, Internal *y, Internal *i)
{
	Internal *t;

	if(y->e < x->e
	   || (y->e == x->e && (y->h < x->h || (y->h == x->h && y->l < x->l)))){
		t = x;
		x = y;
		y = t;
	}
	i->s = y->s;
	if(IsNaN(y)){
		SetQNaN(i);
		return;
	}
	if(IsInfinity(y)){
		if(IsInfinity(x))
			SetQNaN(i);
		else
			SetInfinity(i);
		return;
	}
	matchexponents(x, y);
	i->e = y->e;
	i->h = y->h - x->h;
	i->l = y->l - x->l;
	if(i->l < 0){
		i->l += CarryBit;
		i->h--;
	}
	if(i->h == 0 && i->l == 0)
		SetZero(i);
	else while(i->e > 1 && (i->h & HiddenBit) == 0)
		shift(i);
}

#define	CHUNK		(FractBits/2)
#define	CMASK		((1<<CHUNK)-1)
#define	HI(x)		((short)((x)>>CHUNK) & CMASK)
#define	LO(x)		((short)(x) & CMASK)
#define	SPILL(x)	((x)>>CHUNK)
#define	M(x, y)		((long)a[x]*(long)b[y])
#define	C(h, l)		(((long)((h) & CMASK)<<CHUNK)|((l) & CMASK))

void
fpimul(Internal *x, Internal *y, Internal *i)
{
	long a[4], b[4], c[7], f[4];

	i->s = x->s^y->s;
	if(IsWeird(x) || IsWeird(y)){
		if(IsNaN(x) || IsNaN(y) || IsZero(x) || IsZero(y))
			SetQNaN(i);
		else
			SetInfinity(i);
		return;
	}
	else if(IsZero(x) || IsZero(y)){
		SetZero(i);
		return;
	}
	normalise(x);
	normalise(y);
	i->e = x->e + y->e - (ExpBias - 1);

	a[0] = HI(x->h); b[0] = HI(y->h);
	a[1] = LO(x->h); b[1] = LO(y->h);
	a[2] = HI(x->l); b[2] = HI(y->l);
	a[3] = LO(x->l); b[3] = LO(y->l);

	c[6] =                               M(3, 3);
	c[5] =                     M(2, 3) + M(3, 2) + SPILL(c[6]);
	c[4] =           M(1, 3) + M(2, 2) + M(3, 1) + SPILL(c[5]);
	c[3] = M(0, 3) + M(1, 2) + M(2, 1) + M(3, 0) + SPILL(c[4]);
	c[2] = M(0, 2) + M(1, 1) + M(2, 0)           + SPILL(c[3]);
	c[1] = M(0, 1) + M(1, 0)                     + SPILL(c[2]);
	c[0] = M(0, 0)                               + SPILL(c[1]);

	f[0] = c[0];
	f[1] = C(c[1], c[2]);
	f[2] = C(c[3], c[4]);
	f[3] = C(c[5], c[6]);

	if((f[0] & HiddenBit) == 0){
		f[0] <<= 1;
		f[1] <<= 1;
		f[2] <<= 1;
		f[3] <<= 1;
		if(f[1] & CarryBit){
			f[0] |= 1;
			f[1] &= ~CarryBit;
		}
		if(f[2] & CarryBit){
			f[1] |= 1;
			f[2] &= ~CarryBit;
		}
		if(f[3] & CarryBit){
			f[2] |= 1;
			f[3] &= ~CarryBit;
		}
		i->e--;
	}
	i->h = f[0];
	i->l = f[1];
	if(f[2] || f[3])
		i->l |= 1;
	renormalise(i);
}

void
fpidiv(Internal *x, Internal *y, Internal *i)
{
	i->s = x->s^y->s;
	if(IsNaN(x) || IsNaN(y)
	   || (IsInfinity(x) && IsInfinity(y)) || (IsZero(x) && IsZero(y))){
		SetQNaN(i);
		return;
	}
	else if(IsZero(x) || IsInfinity(y)){
		SetInfinity(i);
		return;
	}
	else if(IsInfinity(x) || IsZero(y)){
		SetZero(i);
		return;
	}
	normalise(x);
	normalise(y);
	i->h = 0;
	i->l = 0;
	i->e = y->e - x->e + (ExpBias + 2*FractBits - 1);
	do{
		if(y->h > x->h || (y->h == x->h && y->l >= x->l)){
			i->l |= 0x01;
			y->h -= x->h;
			y->l -= x->l;
			if(y->l < 0){
				y->l += CarryBit;
				y->h--;
			}
		}
		shift(y);
		shift(i);
	}while ((i->h & HiddenBit) == 0);
	if(y->h || y->l)
		i->l |= 0x01;
	renormalise(i);
}

int
fpicmp(Internal *x, Internal *y)
{
	if(IsNaN(x) && IsNaN(y))
		return 0;
	if(IsInfinity(x) && IsInfinity(y))
		return y->s - x->s;
	if(IsZero(x) && IsZero(y))
		return 0;
	if(x->e == y->e && x->h == y->h && x->l == y->l)
		return y->s - x->s;
	if(x->e < y->e
	   || (x->e == y->e && (x->h < y->h || (x->h == y->h && x->l < y->l))))
		return y->s ? 1: -1;
	return x->s ? -1: 1;
}
