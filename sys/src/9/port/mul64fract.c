#include <u.h>

// multiply two 64 numbers and return the middle 64 bits of the 128 bit result.
// the assummption is that one of the numbers is a fixed point number with the
// decimal to the left of the low order 32 bits.
//
// there should be an assembler version of this routine for each architecture.
// this one is provided to make ports easier.

void
mul64fract(uvlong *r, uvlong a, uvlong b)
{
	uvlong bh, bl;
	uvlong ah, al;
	uvlong res;

	bl = b & 0xffffffffULL;
	bh = b >> 32;
	al = a & 0xffffffffULL;
	ah = a >> 32;

	res = (al*bl)>>32;
	res += (al*bh);
	res += (ah*bl);
	res += (ah*bh)<<32;

	*r = res;
}
