/****************************************************************
 *
 * The author of this software (_dtoa, strtod) is David M. Gay.
 *
 * Copyright (c) 1991 by AT&T.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* Please send bug reports to
	David M. Gay
	AT&T Bell Laboratories, Room 2C-463
	600 Mountain Avenue
	Murray Hill, NJ 07974-2070
	U.S.A.
	dmg@research.att.com or research!dmg
 */

/* Version by Howard Trickey: only IEEE, print modes needed for stdio */

#include <u.h>
#include <libc.h>
#define _RESEARCH_SOURCE
#include <ape/float.h>

/*
 * #define IEEE_8087 for IEEE-arithmetic machines where the least
 *	significant byte has the lowest address.
 * #define IEEE_MC68k for IEEE-arithmetic machines where the most
 *	significant byte has the lowest address.
 * #define Unsigned_Shifts if >> does treats its left operand as unsigned.
 * #define Sudden_Underflow 1 for IEEE-format machines without gradual
 *	underflow (i.e., that flush to zero on underflow).
 * #define Check_FLT_ROUNDS 1 if FLT_ROUNDS can assume the values 2 or 3.
 * #define ROUND_BIASED 1 for IEEE-format with biased rounding.
 * #define Inaccurate_Divide 1 for IEEE-format with correctly rounded
 *	products but inaccurate quotients, e.g., for Intel i860.
 */

#ifdef IEEE_8087
#ifdef IEEE_MC68k
Exactly one of IEEE_8087 or IEEE_MC68k
#endif
#else
#ifndef IEEE_MC68k
Exactly one of IEEE_8087 or IEEE_MC68k
#endif
#endif

#ifndef Sudden_Underflow
#define Sudden_Underflow 0
#endif
#ifndef Check_FLT_ROUNDS
#define Check_FLT_ROUNDS 0
#endif
#ifndef ROUND_BIASED
#define ROUND_BIASED 0
#endif

typedef union {
	double d;
	unsigned long ul[2];
} Dul;

#ifdef IEEE_8087
#define word0(x) ((x).ul[1])
#define word1(x) ((x).ul[0])
#else
#define word0(x) ((x).ul[0])
#define word1(x) ((x).ul[1])
#endif

#ifdef Unsigned_Shifts
#define Sign_Extend(a,b) if (b < 0) a |= 0xffff0000;
#else
#define Sign_Extend(a,b) /*no-op*/
#endif

/* The following definition of Storeinc is appropriate for MIPS processors.
 * An alternative that might be better on some machines is
 * #define Storeinc(a,b,c) (*a++ = b << 16 | c & 0xffff)
 */
#ifdef IEEE_8087
#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b, \
((unsigned short *)a)[0] = (unsigned short)c, a++)
#else
#define Storeinc(a,b,c) (((unsigned short *)a)[0] = (unsigned short)b, \
((unsigned short *)a)[1] = (unsigned short)c, a++)
#endif

/* #define P DBL_MANT_DIG */
/* Ten_pmax = floor(P*log(2)/log(5)) */
/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */

#define Exp_shift  20
#define Exp_shift1 20
#define Exp_msk1    0x100000
#define Exp_msk11   0x100000
#define Exp_mask  0x7ff00000
#define P 53
#define Bias 1023
#define IEEE_Arith
#define Emin (-1022)
#define Exp_1  0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask  0xfffff
#define Frac_mask1 0xfffff
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask  0xfffff
#define Bndry_mask1 0xfffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14
#define Infinite(x) (word0(x) == 0x7ff00000) /* sufficient test for here */

#define Big0 (Frac_mask1 | Exp_msk1*(DBL_MAX_EXP+Bias-1))
#define Big1 0xffffffff

#define Kmax 15

 struct
Bigint {
	struct Bigint *next;
	int k, maxwds, sign, wds;
	unsigned long x[1];
	};

 typedef struct Bigint Bigint;

/* This routines shouldn't be visible externally */
extern Bigint	*_Balloc(int);
extern void	_Bfree(Bigint *);
extern Bigint	*_multadd(Bigint *, int, int);
extern int	_hi0bits(unsigned long);
extern Bigint	*_mult(Bigint *, Bigint *);
extern Bigint	*_pow5mult(Bigint *, int);
extern Bigint	*_lshift(Bigint *, int);
extern int	_cmp(Bigint *, Bigint *);
extern Bigint	*_diff(Bigint *, Bigint *);
extern Bigint	*_d2b(double, int *, int *);
extern Bigint	*_i2b(int);

extern double	_tens[], _bigtens[], _tinytens[];

#define n_bigtens 5

#define Balloc(x) _Balloc(x)
#define Bfree(x) _Bfree(x)
#define Bcopy(x,y) memcpy((char *)&x->sign, (char *)&y->sign, \
y->wds*sizeof(long) + 2*sizeof(int))
#define multadd(x,y,z) _multadd(x,y,z)
#define hi0bits(x) _hi0bits(x)
#define i2b(x) _i2b(x)
#define mult(x,y) _mult(x,y)
#define pow5mult(x,y) _pow5mult(x,y)
#define lshift(x,y) _lshift(x,y)
#define cmp(x,y) _cmp(x,y)
#define diff(x,y) _diff(x,y)
#define d2b(x,y,z) _d2b(x,y,z)

#define tens _tens
#define bigtens _bigtens
#define tinytens _tinytens
