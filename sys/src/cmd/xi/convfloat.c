#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"

ulong
dtodsp(double d)
{
	double in;
	long v;
	int exp, neg;

	neg = 0;
	if(d < 0.0){
		neg = 1;
		d = -d;
	}

	/*
	 * frexp returns with d in [1/2, 1)
	 * we want Mbits bits in the mantissa,
	 * plus a bit to make d in [1, 2)
	 */
	d = frexp(d, &exp);
	exp--;
	d = modf(d * (1 << (Mbits + 1)), &in);

	v = in;
	if(d >= 0.5)
		v++;
	if(v >= (1 << (Mbits + 1))){
		v >>= 1;
		exp++;
	}
	if(neg){
		v = -v;
		/*
		 * normalize for hidden 0 bit
		 */
		if(v & (1 << Mbits)){
			v <<= 1;
			exp--;
		}
	}
	if(v == 0 || exp < -(Bias-1))
		return 0;
	if(exp > 255 - Bias){
		if(neg)
			v = 0;
		else
			v = ~0;
		exp = 255 - Bias;
	}
	v &= Mmask;
	v = (v << Ebits) | ((exp + Bias) & Emask);
	if(neg)
		v |= 0x80000000;
	return v;
}

/*
 * convert a dsp floating value to native double
 * format: N = (-2^sign + 0.mant) * 2^(exp-128)
 * so abs(mant) >= 1 and  abs(mant) <= 2
 */
double
dsptod(ulong v)
{
	long mant, exp;
	int sign;

	exp = v & Emask;
	if(!exp)
		return 0.0;
	exp -= Bias;
	sign = v >> 31;
	mant = (v >> Ebits) & Mmask;
	/*
	 * add hidden bits and sign
	 */
	if(sign)
		mant = (-2<<Mbits) + mant;	/* -2. + 0.mant */
	else
		mant = (1<<Mbits) + mant;	/* 1. + 0.mant */
	return ldexp(mant, exp - Mbits);
}

enum
{
	ULAWINV	= 0xff,		/* bits with inverted value in ulaw */
	ALAWINV	= 0xab,		/* bits with inverted value in alaw */
};

/*
 * scramble the mantissa and power
 */
static int
mpowtoi(int v, int m, int pow)
{
	v |= (m & (1<<0)) << (7-0);
	v |= (m & (1<<1)) << (6-1);
	v |= (m & (1<<2)) << (5-2);
	v |= (m & (1<<3)) << (4-3);
	v |= (pow & (1<<0)) << (3-0);
	v |= (pow & (1<<1)) << (2-1);
	v |= (pow & (1<<2)) >> (2-1);
	return v;
}

/*
 * convert to mulaw
 */
int
dtoulaw(double d)
{
	int pow, m, v;

	v = 0;
	if(d < 0){
		v = 1;
		d = -d;
	}
	d += 16.5;

	pow = 0;
	for(; d >= 32.; d /= 2.){
		pow++;
		if(pow >= 7)	/* saturate */
			return (0xfe + v) ^ ULAWINV;
	}
	m = floor(d - 16.);

	return mpowtoi(v, m, pow) ^ ULAWINV;
}

/*
 * convert to Alaw
 */
int
dtoalaw(double d)
{
	int pow, m, v;

	v = 0;
	if(d < 0){
		v = 1;
		d = -d;
	}

	pow = 0;
	for(; d >= 32.; d /= 2.){
		pow++;
		if(pow >= 7)	/* saturate */
			return (0xfe + v) ^ ALAWINV;
	}
	m = floor(d - 16.);
	if(pow == 0)
		m = floor(d / 2.);
	return mpowtoi(v, m, pow) ^ ALAWINV;
}

/*
 * scramble the mantissa and power
 */
static int
itom(int v)
{
	int m;

	m = (v & (1<<7)) >> (7-0);
	m |= (v & (1<<6)) >> (6-1);
	m |= (v & (1<<5)) >> (5-2);
	return m | (v & (1<<4)) >> (4-3);
}

static int
itopow(int v)
{
	int pow;

	pow = (v & (1<<3)) >> (3-0);
	pow |= (v & (1<<2)) >> (2-1);
	return pow | (v & (1<<1)) << (2-1);
}

double
ulawtod(int v)
{
	double d;
	int neg, m, pow;

	v = (v & 0xff) ^ ULAWINV;
	neg = v & 1;
	m = itom(v);
	pow = itopow(v);
	d = (16.5 + m) * (1 << pow) - 16.5;
	if(neg)
		return -d;
	return d;
}

double
alawtod(int v)
{
	double d;
	int neg, m, pow;

	v = (v & 0xff) ^ ALAWINV;
	neg = v & 1;
	m = itom(v);
	pow = itopow(v);
	if(pow)
		d = (16.5 + m) * (1 << pow);
	else
		d = (0.5 + m) * 2.;
	if(neg)
		return -d;
	return d;
}
