/* Copyright (C) 1989, 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gsfemu.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Floating point emulator for gcc */

/* We actually only need arch.h + uint and ulong, but because signal.h */
/* may include <sys/types.h>, we have to include std.h to handle (avoid) */
/* redefinition of type names. */
#include "std.h"
#include <signal.h>

/*#define TEST */

/*
 * This package is not a fully general IEEE floating point implementation.
 * It implements only one rounding mode (round to nearest) and does not
 * generate or properly handle NaNs.
 *
 * The names and interfaces of the routines in this package were designed to
 * work with gcc.  This explains some peculiarities such as the presence of
 * single-precision arithmetic (forbidden by the ANSI standard) and the
 * omission of unsigned-to-float conversions (which gcc implements with
 * truly grotesque inline code).
 *
 * The following routines do not yet handle denormalized numbers:
 *      addd3 (operands or result)
 *      __muldf3 (operands or result)
 *      __divdf3 (operands or result)
 *      __truncdfsf2 (result)
 *      __extendsfdf2 (operand)
 */

/*
 * IEEE single-precision floats have the format:
 *      sign(1) exponent(8) mantissa(23)
 * The exponent is biased by +127.
 * The mantissa is a fraction with an implicit integer part of 1,
 * unless the exponent is zero.
 */
#define fx(ia) (((ia) >> 23) & 0xff)
#define fx_bias 127
/*
 * IEEE double-precision floats have the format:
 *      sign(1) exponent(11) mantissa(52)
 * The exponent is biased by +1023.
 */
#define dx(ld) ((ld[msw] >> 20) & 0x7ff)
#define dx_bias 1023

#if arch_is_big_endian
#  define msw 0
#  define lsw 1
#else
#  define msw 1
#  define lsw 0
#endif
/* Double arguments/results */
#define la ((const long *)&a)
#define ula ((const ulong *)&a)
#define lb ((const long *)&b)
#define ulb ((const ulong *)&b)
#define dc (*(const double *)lc)
/* Float arguments/results */
#define ia (*(const long *)&a)
#define ua (*(const ulong *)&a)
#define ib (*(const long *)&b)
#define ub (*(const ulong *)&b)
#define fc (*(const float *)&lc)

/* Round a double-length fraction by adding 1 in the lowest bit and */
/* then shifting right by 1. */
#define roundr1(ms, uls)\
  if ( uls == 0xffffffff ) ms++, uls = 0;\
  else uls++;\
  uls = (uls >> 1) + (ms << 31);\
  ms >>= 1

/* Extend a non-zero float to a double. */
#define extend(lc, ia)\
  ((lc)[msw] = ((ia) & 0x80000000) + (((ia) & 0x7fffffff) >> 3) + 0x38000000,\
   (lc)[lsw] = (ia) << 29)

/* ---------------- Arithmetic ---------------- */

/* -------- Add/subtract/negate -------- */

double
__negdf2(double a)
{
    long lc[2];

    lc[msw] = la[msw] ^ 0x80000000;
    lc[lsw] = la[lsw];
    return dc;
}
float
__negsf2(float a)
{
    long lc = ia ^ 0x80000000;

    return fc;
}

double
__adddf3(double a, double b)
{
    long lc[2];
    int expt = dx(la);
    int shift = expt - dx(lb);
    long sign;
    ulong msa, lsa;
    ulong msb, lsb;

    if (shift < 0) {		/* Swap a and b so that expt(a) >= expt(b). */
	double temp = a;

	a = b, b = temp;
	expt += (shift = -shift);
    }
    if (shift >= 54)		/* also picks up most cases where b == 0 */
	return a;
    if (!(lb[msw] & 0x7fffffff))
	return a;
    sign = la[msw] & 0x80000000;
    msa = (la[msw] & 0xfffff) + 0x100000, lsa = la[lsw];
    msb = (lb[msw] & 0xfffff) + 0x100000, lsb = lb[lsw];
    if ((la[msw] ^ lb[msw]) >= 0) {	/* Adding numbers of the same sign. */
	if (shift >= 32)
	    lsb = msb, msb = 0, shift -= 32;
	if (shift) {
	    --shift;
	    lsb = (lsb >> shift) + (msb << (32 - shift));
	    msb >>= shift;
	    roundr1(msb, lsb);
	}
	if (lsb > (ulong) 0xffffffff - lsa)
	    msa++;
	lsa += lsb;
	msa += msb;
	if (msa > 0x1fffff) {
	    roundr1(msa, lsa);
	    /* In principle, we have to worry about exponent */
	    /* overflow here, but we don't. */
	    ++expt;
	}
    } else {			/* Adding numbers of different signs. */
	if (shift > 53)
	    return a;		/* b can't affect the result, even rounded */
	if (shift == 0 && (msb > msa || (msb == msa && lsb >= lsa))) {	/* This is the only case where the sign of the result */
	    /* differs from the sign of the first operand. */
	    sign ^= 0x80000000;
	    msa = msb - msa;
	    if (lsb < lsa)
		msa--;
	    lsa = lsb - lsa;
	} else {
	    if (shift >= 33) {
		lsb = ((msb >> (shift - 32)) + 1) >> 1;		/* round */
		msb = 0;
	    } else if (shift) {
		lsb = (lsb >> (shift - 1)) + (msb << (33 - shift));
		msb >>= shift - 1;
		roundr1(msb, lsb);
	    }
	    msa -= msb;
	    if (lsb > lsa)
		msa--;
	    lsa -= lsb;
	}
	/* Now renormalize the result. */
	/* For the moment, we do this the slow way. */
	if (!(msa | lsa))
	    return 0;
	while (msa < 0x100000) {
	    msa = (msa << 1) + (lsa >> 31);
	    lsa <<= 1;
	    expt -= 1;
	}
	if (expt <= 0) {	/* Underflow.  Return 0 rather than a denorm. */
	    lc[msw] = sign;
	    lc[lsw] = 0;
	    return dc;
	}
    }
    lc[msw] = sign + ((ulong) expt << 20) + (msa & 0xfffff);
    lc[lsw] = lsa;
    return dc;
}
double
__subdf3(double a, double b)
{
    long nb[2];

    nb[msw] = lb[msw] ^ 0x80000000;
    nb[lsw] = lb[lsw];
    return a + *(const double *)nb;
}

float
__addsf3(float a, float b)
{
    long lc;
    int expt = fx(ia);
    int shift = expt - fx(ib);
    long sign;
    ulong ma, mb;

    if (shift < 0) {		/* Swap a and b so that expt(a) >= expt(b). */
	long temp = ia;

	*(long *)&a = ib;
	*(long *)&b = temp;
	expt += (shift = -shift);
    }
    if (shift >= 25)		/* also picks up most cases where b == 0 */
	return a;
    if (!(ib & 0x7fffffff))
	return a;
    sign = ia & 0x80000000;
    ma = (ia & 0x7fffff) + 0x800000;
    mb = (ib & 0x7fffff) + 0x800000;
    if ((ia ^ ib) >= 0) {	/* Adding numbers of the same sign. */
	if (shift) {
	    --shift;
	    mb = ((mb >> shift) + 1) >> 1;
	}
	ma += mb;
	if (ma > 0xffffff) {
	    ma = (ma + 1) >> 1;
	    /* In principle, we have to worry about exponent */
	    /* overflow here, but we don't. */
	    ++expt;
	}
    } else {			/* Adding numbers of different signs. */
	if (shift > 24)
	    return a;		/* b can't affect the result, even rounded */
	if (shift == 0 && mb >= ma) {
	    /* This is the only case where the sign of the result */
	    /* differs from the sign of the first operand. */
	    sign ^= 0x80000000;
	    ma = mb - ma;
	} else {
	    if (shift) {
		--shift;
		mb = ((mb >> shift) + 1) >> 1;
	    }
	    ma -= mb;
	}
	/* Now renormalize the result. */
	/* For the moment, we do this the slow way. */
	if (!ma)
	    return 0;
	while (ma < 0x800000) {
	    ma <<= 1;
	    expt -= 1;
	}
	if (expt <= 0) {
	    /* Underflow.  Return 0 rather than a denorm. */
	    lc = sign;
	    return fc;
	}
    }
    lc = sign + ((ulong)expt << 23) + (ma & 0x7fffff);
    return fc;
}

float
__subsf3(float a, float b)
{
    long lc = ib ^ 0x80000000;

    return a + fc;
}

/* -------- Multiplication -------- */

double
__muldf3(double a, double b)
{
    long lc[2];
    ulong sign;
    uint H, I, h, i;
    ulong p0, p1, p2;
    ulong expt;

    if (!(la[msw] & 0x7fffffff) || !(lb[msw] & 0x7fffffff))
	return 0;
    /*
     * We now have to multiply two 53-bit fractions to produce a 53-bit
     * result.  Since the idiots who specified the standard C libraries
     * don't allow us to use the 32 x 32 => 64 multiply that every
     * 32-bit CPU provides, we have to chop these 53-bit numbers up into
     * 14-bit chunks so we can use 32 x 32 => 32 multiplies.
     */
#define chop_ms(ulx, h, i)\
  h = ((ulx[msw] >> 7) & 0x1fff) | 0x2000,\
  i = ((ulx[msw] & 0x7f) << 7) | (ulx[lsw] >> 25)
#define chop_ls(ulx, j, k)\
  j = (ulx[lsw] >> 11) & 0x3fff,\
  k = (ulx[lsw] & 0x7ff) << 3
    chop_ms(ula, H, I);
    chop_ms(ulb, h, i);
#undef chop
#define prod(m, n) ((ulong)(m) * (n))	/* force long multiply */
    p0 = prod(H, h);
    p1 = prod(H, i) + prod(I, h);
    /* If these doubles were produced from floats or ints, */
    /* all the other products will be zero.  It's worth a check. */
    if ((ula[lsw] | ulb[lsw]) & 0x1ffffff) {	/* No luck. */
	/* We can almost always avoid computing p5 and p6, */
	/* but right now it's not worth the trouble to check. */
	uint J, K, j, k;

	chop_ls(ula, J, K);
	chop_ls(ulb, j, k);
	{
	    ulong p6 = prod(K, k);
	    ulong p5 = prod(J, k) + prod(K, j) + (p6 >> 14);
	    ulong p4 = prod(I, k) + prod(J, j) + prod(K, i) + (p5 >> 14);
	    ulong p3 = prod(H, k) + prod(I, j) + prod(J, i) + prod(K, h) +
	    (p4 >> 14);

	    p2 = prod(H, j) + prod(I, i) + prod(J, h) + (p3 >> 14);
	}
    } else {
	p2 = prod(I, i);
    }
    /* Now p0 through p2 hold the result. */
/****** ASSUME expt IS 32 BITS WIDE. ******/
    expt = (la[msw] & 0x7ff00000) + (lb[msw] & 0x7ff00000) -
	(dx_bias << 20);
    /* Now expt is in the range [-1023..3071] << 20. */
    /* Values outside the range [1..2046] are invalid. */
    p1 += p2 >> 14;
    p0 += p1 >> 14;
    /* Now the 56-bit result consists of p0, the low 14 bits of p1, */
    /* and the low 14 bits of p2. */
    /* p0 can be anywhere between 2^26 and almost 2^28, so we might */
    /* need to adjust the exponent by 1. */
    if (p0 < 0x8000000) {
	p0 = (p0 << 1) + ((p1 >> 13) & 1);
	p1 = (p1 << 1) + ((p2 >> 13) & 1);
	p2 <<= 1;
    } else
	expt += 0x100000;
    /* The range of expt is now [-1023..3072] << 20. */
    /* Round the mantissa to 53 bits. */
    if (!((p2 += 4) & 0x3ffc) && !(++p1 & 0x3fff) && ++p0 >= 0x10000000) {
	p0 >>= 1, p1 = 0x2000;
	/* Check for exponent overflow, just in case. */
	if ((ulong) expt < 0xc0000000)
	    expt += 0x100000;
    }
    sign = (la[msw] ^ lb[msw]) & 0x80000000;
    if (expt == 0) {		/* Underflow.  Return 0 rather than a denorm. */
	lc[msw] = sign;
	lc[lsw] = 0;
    } else if ((ulong) expt >= 0x7ff00000) {	/* Overflow or underflow. */
	if ((ulong) expt <= 0xc0000000) {	/* Overflow. */
	    raise(SIGFPE);
	    lc[msw] = sign + 0x7ff00000;
	    lc[lsw] = 0;
	} else {		/* Underflow.  See above. */
	    lc[msw] = sign;
	    lc[lsw] = 0;
	}
    } else {
	lc[msw] = sign + expt + ((p0 >> 7) & 0xfffff);
	lc[lsw] = (p0 << 25) | ((p1 & 0x3fff) << 11) | ((p2 >> 3) & 0x7ff);
    }
    return dc;
#undef prod
}
float
__mulsf3(float a, float b)
{
    uint au, al, bu, bl, cu, cl, sign;
    long lc;
    uint expt;

    if (!(ia & 0x7fffffff) || !(ib & 0x7fffffff))
	return 0;
    au = ((ia >> 8) & 0x7fff) | 0x8000;
    bu = ((ib >> 8) & 0x7fff) | 0x8000;
    /* Since 0x8000 <= au,bu <= 0xffff, 0x40000000 <= cu <= 0xfffe0001. */
    cu = au * bu;
    if ((al = ia & 0xff) != 0) {
	cl = bu * al;
    } else
	cl = 0;
    if ((bl = ib & 0xff) != 0) {
	cl += au * bl;
	if (al)
	    cl += (al * bl) >> 8;
    }
    cu += cl >> 8;
    sign = (ia ^ ib) & 0x80000000;
    expt = (ia & 0x7f800000) + (ib & 0x7f800000) - (fx_bias << 23);
    /* expt is now in the range [-127..383] << 23. */
    /* Values outside [1..254] are invalid. */
    if (cu <= 0x7fffffff)
	cu <<= 1;
    else
	expt += 1 << 23;
    cu = ((cu >> 7) + 1) >> 1;
    if (expt < 1 << 23)
	lc = sign;		/* underflow */
    else if (expt > (uint)(254 << 23)) {
	if (expt <= 0xc0000000) { /* overflow */
	    raise(SIGFPE);
	    lc = sign + 0x7f800000;
	} else {		/* underflow */
	    lc = sign;
	}
    } else
	lc = sign + expt + cu - 0x800000;
    return fc;
}

/* -------- Division -------- */

double
__divdf3(double a, double b)
{
    long lc[2];

    /*
     * Multi-precision division is really, really awful.
     * We generate the result more or less brute force,
     * 11 bits at a time.
     */
    ulong sign = (la[msw] ^ lb[msw]) & 0x80000000;
    ulong msa = (la[msw] & 0xfffff) | 0x100000, lsa = la[lsw];
    ulong msb = (lb[msw] & 0xfffff) | 0x100000, lsb = lb[lsw];
    uint qn[5];
    int i;
    ulong msq, lsq;
    int expt = dx(la) - dx(lb) + dx_bias;

    if (!(lb[msw] & 0x7fffffff)) {	/* Division by zero. */
	raise(SIGFPE);
	lc[lsw] = 0;
	lc[msw] =
	    (la[msw] & 0x7fffffff ?
	     sign + 0x7ff00000 /* infinity */ :
	     0x7ff80000 /* NaN */ );
	return dc;
    }
    if (!(la[msw] & 0x7fffffff))
	return 0;
    for (i = 0; i < 5; ++i) {
	uint q;
	ulong msp, lsp;

	msa = (msa << 11) + (lsa >> 21);
	lsa <<= 11;
	q = msa / msb;
	/* We know that 2^20 <= msb < 2^21; q could be too high by 1, */
	/* but it can't be too low. */
	/* Set p = d * q. */
	msp = q * msb;
	lsp = q * (lsb & 0x1fffff);
	{
	    ulong midp = q * (lsb >> 21);

	    msp += (midp + (lsp >> 21)) >> 11;
	    lsp += midp << 21;
	}
	/* Set a = a - p, i.e., the tentative remainder. */
	if (msp > msa || (lsp > lsa && msp == msa)) {	/* q was too large. */
	    --q;
	    if (lsb > lsp)
		msp--;
	    lsp -= lsb;
	    msp -= msb;
	}
	if (lsp > lsa)
	    msp--;
	lsa -= lsp;
	msa -= msp;
	qn[i] = q;
    }
    /* Pack everything together again. */
    msq = (qn[0] << 9) + (qn[1] >> 2);
    lsq = (qn[1] << 30) + (qn[2] << 19) + (qn[3] << 8) + (qn[4] >> 3);
    if (msq < 0x100000) {
	msq = (msq << 1) + (lsq >> 31);
	lsq <<= 1;
	expt--;
    }
    /* We should round the quotient, but we don't. */
    if (expt <= 0) {		/* Underflow.  Return 0 rather than a denorm. */
	lc[msw] = sign;
	lc[lsw] = 0;
    } else if (expt >= 0x7ff) {	/* Overflow. */
	raise(SIGFPE);
	lc[msw] = sign + 0x7ff00000;
	lc[lsw] = 0;
    } else {
	lc[msw] = sign + (expt << 20) + (msq & 0xfffff);
	lc[lsw] = lsq;
    }
    return dc;
}
float
__divsf3(float a, float b)
{
    return (float)((double)a / (double)b);
}

/* ---------------- Comparisons ---------------- */

static int
compared2(const long *pa, const long *pb)
{
#define upa ((const ulong *)pa)
#define upb ((const ulong *)pb)
    if (pa[msw] == pb[msw]) {
	int result = (upa[lsw] < upb[lsw] ? -1 :
		      upa[lsw] > upb[lsw] ? 1 : 0);

	return (pa[msw] < 0 ? -result : result);
    }
    if ((pa[msw] & pb[msw]) < 0)
	return (pa[msw] < pb[msw] ? 1 : -1);
    /* We have to treat -0 and +0 specially. */
    else if (!((pa[msw] | pb[msw]) & 0x7fffffff) && !(pa[lsw] | pb[lsw]))
	return 0;
    else
	return (pa[msw] > pb[msw] ? 1 : -1);
#undef upa
#undef upb
}

/* 0 means true */
int
__eqdf2(double a, double b)
{
    return compared2(la, lb);
}

/* !=0 means true */
int
__nedf2(double a, double b)
{
    return compared2(la, lb);
}

/* >0 means true */
int
__gtdf2(double a, double b)
{
    return compared2(la, lb);
}

/* >=0 means true */
int
__gedf2(double a, double b)
{
    return compared2(la, lb);
}

/* <0 means true */
int
__ltdf2(double a, double b)
{
    return compared2(la, lb);
}

/* <=0 means true */
int
__ledf2(double a, double b)
{
    return compared2(la, lb);
}

static int
comparef2(long va, long vb)
{				/* We have to treat -0 and +0 specially. */
    if (va == vb)
	return 0;
    if ((va & vb) < 0)
	return (va < vb ? 1 : -1);
    else if (!((va | vb) & 0x7fffffff))
	return 0;
    else
	return (va > vb ? 1 : -1);
}

/* See the __xxdf2 functions above for the return values. */
int
__eqsf2(float a, float b)
{
    return comparef2(ia, ib);
}
int
__nesf2(float a, float b)
{
    return comparef2(ia, ib);
}
int
__gtsf2(float a, float b)
{
    return comparef2(ia, ib);
}
int
__gesf2(float a, float b)
{
    return comparef2(ia, ib);
}
int
__ltsf2(float a, float b)
{
    return comparef2(ia, ib);
}
int
__lesf2(float a, float b)
{
    return comparef2(ia, ib);
}

/* ---------------- Conversion ---------------- */

long
__fixdfsi(double a)
{				/* This routine does check for overflow. */
    long i = (la[msw] & 0xfffff) + 0x100000;
    int expt = dx(la) - dx_bias;

    if (expt < 0)
	return 0;
    if (expt <= 20)
	i >>= 20 - expt;
    else if (expt >= 31 &&
	     (expt > 31 || i != 0x100000 || la[msw] >= 0 ||
	      ula[lsw] >= 1L << 21)
	) {
	raise(SIGFPE);
	i = (la[msw] < 0 ? 0x80000000 : 0x7fffffff);
    } else
	i = (i << (expt - 20)) + (ula[lsw] >> (52 - expt));
    return (la[msw] < 0 ? -i : i);
}

long
__fixsfsi(float a)
{				/* This routine does check for overflow. */
    long i = (ia & 0x7fffff) + 0x800000;
    int expt = fx(ia) - fx_bias;

    if (expt < 0)
	return 0;
    if (expt <= 23)
	i >>= 23 - expt;
    else if (expt >= 31 && (expt > 31 || i != 0x800000 || ia >= 0)) {
	raise(SIGFPE);
	i = (ia < 0 ? 0x80000000 : 0x7fffffff);
    } else
	i <<= expt - 23;
    return (ia < 0 ? -i : i);
}

double
__floatsidf(long i)
{
    long msc;
    ulong v;
    long lc[2];

    if (i > 0)
	msc = 0x41e00000 - 0x100000, v = i;
    else if (i < 0)
	msc = 0xc1e00000 - 0x100000, v = -i;
    else			/* i == 0 */
	return 0;
    while (v < 0x01000000)
	v <<= 8, msc -= 0x00800000;
    if (v < 0x10000000)
	v <<= 4, msc -= 0x00400000;
    while (v < 0x80000000)
	v <<= 1, msc -= 0x00100000;
    lc[msw] = msc + (v >> 11);
    lc[lsw] = v << 21;
    return dc;
}

float
__floatsisf(long i)
{
    long lc;

    if (i == 0)
	lc = 0;
    else {
	ulong v;

	if (i < 0)
	    lc = 0xcf000000, v = -i;
	else
	    lc = 0x4f000000, v = i;
	while (v < 0x01000000)
	    v <<= 8, lc -= 0x04000000;
	while (v < 0x80000000)
	    v <<= 1, lc -= 0x00800000;
	v = ((v >> 7) + 1) >> 1;
	if (v > 0xffffff)
	    v >>= 1, lc += 0x00800000;
	lc += v & 0x7fffff;
    }
    return fc;
}

float
__truncdfsf2(double a)
{				/* This routine does check for overflow, but it converts */
    /* underflows to zero rather than to a denormalized number. */
    long lc;

    if ((la[msw] & 0x7ff00000) < 0x38100000)
	lc = la[msw] & 0x80000000;
    else if ((la[msw] & 0x7ff00000) >= 0x47f00000) {
	raise(SIGFPE);
	lc = (la[msw] & 0x80000000) + 0x7f800000;	/* infinity */
    } else {
	lc = (la[msw] & 0xc0000000) +
	    ((la[msw] & 0x07ffffff) << 3) +
	    (ula[lsw] >> 29);
	/* Round the mantissa.  Simply adding 1 works even if the */
	/* mantissa overflows, because it increments the exponent and */
	/* sets the mantissa to zero! */
	if (ula[lsw] & 0x10000000)
	    ++lc;
    }
    return fc;
}

double
__extendsfdf2(float a)
{
    long lc[2];

    if (!(ia & 0x7fffffff))
	lc[msw] = ia, lc[lsw] = 0;
    else
	extend(lc, ia);
    return dc;
}

/* ---------------- Test program ---------------- */

#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

int
test(double v1)
{
    double v3 = v1 * 3;
    double vh = v1 / 2;
    double vd = v3 - vh;
    double vdn = v1 - v3;

    printf("%g=1 %g=3 %g=0.5 %g=2.5 %g=-2\n", v1, v3, vh, vd, vdn);
    return 0;
}

float
randf(void)
{
    int v = rand();

    v = (v << 16) ^ rand();
    if (!(v & 0x7f800000))
	return 0;
    if ((v & 0x7f800000) == 0x7f800000)
	return randf();
    return *(float *)&v;
}

int
main(int argc, char *argv[])
{
    int i;

    test(1.0);
    for (i = 0; i < 10; ++i) {
	float a = randf(), b = randf(), r;
	int c;

	switch ((rand() >> 12) & 3) {
	    case 0:
		r = a + b;
		c = '+';
		break;
	    case 1:
		r = a - b;
		c = '-';
		break;
	    case 2:
		r = a * b;
		c = '*';
		break;
	    case 3:
		if (b == 0)
		    continue;
		r = a / b;
		c = '/';
		break;
	}
	printf("0x%08x %c 0x%08x = 0x%08x\n",
	       *(int *)&a, c, *(int *)&b, *(int *)&r);
    }
}

/*
   Results from compiling with hardware floating point:

   1=1 3=3 0.5=0.5 2.5=2.5 -2=-2
   0x6f409924 - 0x01faa67a = 0x6f409924
   0x00000000 + 0x75418107 = 0x75418107
   0xe32fab71 - 0xc88f7816 = 0xe32fab71
   0x94809067 * 0x84ddaeee = 0x00000000
   0x2b0a5b7d + 0x38f70f50 = 0x38f70f50
   0xa5efcef3 - 0xc5dc1a2c = 0x45dc1a2c
   0xe7124521 * 0x3f4206d2 = 0xe6ddb891
   0x8d175f17 * 0x2ed2c1c0 = 0x80007c9f
   0x419e7a6d / 0xf2f95a35 = 0x8e22b404
   0xe0b2f48f * 0xc72793fc = 0x686a49f8

 */


#endif
