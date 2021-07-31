/* Copyright (C) 1990, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gxarith.h */
/* Arithmetic macros for Ghostscript library */

/* Define an in-line abs function, good for any signed numeric type. */
#define any_abs(x) ((x) < 0 ? -(x) : (x))

/* Test whether an integral value fits in a given number of bits. */
/* This works for all integral types. */
#define fits_in_bits(i, n)\
  (sizeof(i) <= sizeof(int) ? fits_in_ubits((i) + (1 << ((n) - 1)), (n) + 1) :\
   fits_in_ubits((i) + (1L << ((n) - 1)), (n) + 1))
#define fits_in_ubits(i, n) (((i) >> (n)) == 0)

/* Test floating point values against constants. */
/* gxfarith.h redefines these on machines with very slow floating point. */
/* Only use these macros on variables, not on expressions. */
#define is_fzero(f) ((f) == 0.0)
#define is_fzero2(f1,f2) ((f1) == 0.0 && (f2) == 0.0)
#define is_fneg(f) ((f) < 0.0)
#define is_fge1(f) ((f) >= 1.0)
/* Test whether a floating point value fits in a given number of bits. */
#define f_fits_in_bits(f, n)\
  ((f) >= -2.0 * (1L << ((n) - 2)) && (f) < 2.0 * (1L << ((n) - 2)))
#define f_fits_in_ubits(f, n)\
  ((f) >= 0 && (f) < 4.0 * (1L << ((n) - 2)))

/*
 * Define a macro for computing log2(n), where n=1,2,4,...,128.
 * Because some compilers limit the total size of a statement,
 * this macro must only mention n once.  The macro should really
 * only be used with compile-time constant arguments, but it will work
 * even if n is an expression computed at run-time.
 */
#define small_exact_log2(n)\
 ((uint)(05637042010L >> ((((n) % 11) - 1) * 3)) & 7)

/*
 * The following doesn't give rise to a macro, but is used in several
 * places in Ghostscript.  We observe that if M = 2^n-1 and V < M^2,
 * then the quotient Q and remainder R can be computed as:
 *		Q = V / M = (V + (V >> n) + 1) >> n;
 *		R = V % M = (V + (V / M)) & M = V - (Q << n) + Q.
 */
