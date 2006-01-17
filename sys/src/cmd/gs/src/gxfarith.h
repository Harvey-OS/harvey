/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfarith.h,v 1.7 2004/06/23 18:50:19 stefan Exp $ */
/* Floating point arithmetic macros for Ghostscript library */

#ifndef gxfarith_INCLUDED
#  define gxfarith_INCLUDED

#include "gconfigv.h"		/* for USE_FPU */
#include "gxarith.h"

/*
 * The following macros replace the ones in gxarith.h on machines
 * that are likely to have very slow floating point.
 *
 * None of these macros would be necessary if compilers had a clue
 * about generating good floating point comparisons on machines with
 * slow (or no) floating point hardware.
 */

# if USE_FPU <= 0 && arch_floats_are_IEEE && (arch_sizeof_float == arch_sizeof_int || arch_sizeof_float == arch_sizeof_long)

#  if arch_sizeof_float == arch_sizeof_int
typedef int _f_int_t;
typedef uint _f_uint_t;

#  else				/* arch_sizeof_float == arch_sizeof_long */
typedef long _f_int_t;
typedef ulong _f_uint_t;

#  endif
#  define _f_as_int(f) *(const _f_int_t *)(&(f))
#  define _f_as_uint(f) *(const _f_uint_t *)(&(f))

#  if arch_sizeof_double == arch_sizeof_int
#    define _d_int_t int
#  else
#   if arch_sizeof_double == arch_sizeof_long
#    define _d_int_t long
#   endif
#  endif
#  define _d_uint_t unsigned _d_int_t
#  define _d_as_int(f) *(const _d_int_t *)(&(f))
#  define _d_as_uint(f) *(const _d_uint_t *)(&(f))

#  define _ftest(v,f,n)\
     (sizeof(v)==sizeof(float)?(f):(n))
#  ifdef _d_int_t
#    define _fdtest(v,f,d,n)\
	(sizeof(v)==sizeof(float)?(f):sizeof(v)==sizeof(double)?(d):(n))
#  else
#    define _fdtest(v,f,d,n)\
	_ftest(v,f,n)
#  endif

#  undef is_fzero
#  define is_fzero(f)	/* must handle both +0 and -0 */\
     _fdtest(f, (_f_as_int(f) << 1) == 0, (_d_as_int(f) << 1) == 0,\
       (f) == 0.0)

#  undef is_fzero2
#  define is_fzero2(f1,f2)\
     (sizeof(f1) == sizeof(float) && sizeof(f2) == sizeof(float) ?\
      ((_f_as_int(f1) | _f_as_int(f2)) << 1) == 0 :\
      (f1) == 0.0 && (f2) == 0.0)

#  undef is_fneg
#  if arch_is_big_endian
#    define _is_fnegb(f) (*(const byte *)&(f) >= 0x80)
#  else
#    define _is_fnegb(f) (((const byte *)&(f))[sizeof(f) - 1] >= 0x80)
#  endif
#  if arch_sizeof_float == arch_sizeof_int
#    define is_fneg(f)\
       (sizeof(f) == sizeof(float) ? _f_as_int(f) < 0 :\
	_is_fnegb(f))
#  else
#    define is_fneg(f) _is_fnegb(f)
#  endif

#  define IEEE_expt 0x7f800000	/* IEEE exponent mask */
#  define IEEE_f1 0x3f800000	/* IEEE 1.0 */

#  undef is_fge1
#  if arch_sizeof_float == arch_sizeof_int
#    define is_fge1(f)\
       (sizeof(f) == sizeof(float) ?\
	(_f_as_int(f)) >= IEEE_f1 :\
	(f) >= 1.0)
#  else				/* arch_sizeof_float == arch_sizeof_long */
#    define is_fge1(f)\
       (sizeof(f) == sizeof(float) ?\
	(int)(_f_as_int(f) >> 16) >= (IEEE_f1 >> 16) :\
	(f) >= 1.0)
#  endif

#  undef f_fits_in_ubits
#  undef f_fits_in_bits
#  define _f_bits(n) (4.0 * (1L << ((n) - 2)))
#  define f_fits_in_ubits(f, n)\
	_ftest(f, _f_as_uint(f) < (_f_uint_t)IEEE_f1 + ((_f_uint_t)(n) << 23),\
	  (f) >= 0 && (f) < _f_bits(n))
#  define f_fits_in_bits(f, n)\
	_ftest(f, (_f_as_uint(f) & IEEE_expt) < IEEE_f1 + ((_f_uint_t)((n)-1) << 23),\
	  (f) >= -_f_bits((n)-1) && (f) < _f_bits((n)-1))

# endif				/* USE_FPU <= 0 & ... */

/*
 * Define sine and cosine functions that take angles in degrees rather than
 * radians, hit exact values at multiples of 90 degrees, and are implemented
 * efficiently on machines with slow (or no) floating point.
 */
double gs_sin_degrees(double angle);
double gs_cos_degrees(double angle);
typedef struct gs_sincos_s {
    double sin, cos;
    bool orthogonal;		/* angle is multiple of 90 degrees */
} gs_sincos_t;
void gs_sincos_degrees(double angle, gs_sincos_t * psincos);

/*
 * Define an atan2 function that returns an angle in degrees and uses
 * the PostScript quadrant rules.  Note that it may return
 * gs_error_undefinedresult.
 */
int gs_atan2_degrees(double y, double x, double *pangle);

#endif /* gxfarith_INCLUDED */
