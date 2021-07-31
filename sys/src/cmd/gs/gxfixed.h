/* Copyright (C) 1989, 1990, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxfixed.h */
/* Fixed-point arithmetic for Ghostscript */

#ifndef gxfixed_INCLUDED
#  define gxfixed_INCLUDED

/*
 * Coordinates are generally represented internally by fixed-point
 * quantities: integers lose accuracy in crucial places,
 * and floating point arithmetic is slow.
 */
typedef long fixed;
#define max_fixed max_long
#define min_fixed min_long
#define fixed_0 0L
#define fixed_epsilon 1L
/*
 * 12 bits of fraction provides both the necessary accuracy and
 * a sufficiently large range of coordinates.
 */
#define _fixed_shift 12
#define _fixed_scale (1<<_fixed_shift)
#define _fixed_rshift(x) arith_rshift(x,_fixed_shift)
#define _fixed_round_v (_fixed_scale>>1)
#define _fixed_fraction_v (_fixed_scale-1)
#define fixed_int_bits (sizeof(fixed) * 8 - _fixed_shift)

/*
 * Most operations can be done directly on fixed-point quantities:
 * addition, subtraction, shifting, multiplication or division by
 * (integer) constants; assignment, assignment with zero;
 * comparison, comparison against zero.
 * Multiplication and division by floats is OK if the result is
 * explicitly cast back to fixed.
 * Conversion to and from int and float types must be done explicitly.
 * Note that if we are casting a fixed to a float in a context where
 * only ratios and not actual values are involved, we don't need to take
 * the scale factor into account: we can simply cast to float directly.
 */
#define int2fixed(i) ((fixed)(i)<<_fixed_shift)
/* Define some useful constants. */
/* Avoid casts, so strict ANSI compilers will accept them in #ifs. */
#define fixed_1 (fixed_epsilon << _fixed_shift)
#define fixed_half (fixed_1 >> 1)
/*
 * On 16-bit systems, we can convert fixed variables to ints more efficiently
 * than general fixed quantities.  For this reason, we define two separate
 * sets of conversion macros.
 */
#define fixed2int(x) ((int)_fixed_rshift(x))
#define fixed2int_rounded(x) ((int)_fixed_rshift((x)+_fixed_round_v))
#define fixed2int_ceiling(x) ((int)_fixed_rshift((x)+_fixed_fraction_v))
#if arch_ints_are_short & !arch_is_big_endian
/* Do some of the shifting and extraction ourselves. */
#  define _fixed_hi(x) *((uint *)&(x)+1)
#  define _fixed_lo(x) *((uint *)&(x))
#  define fixed2int_var(x)\
	((int)((_fixed_hi(x) << (16-_fixed_shift)) +\
	       (_fixed_lo(x) >> _fixed_shift)))
#  define fixed2int_var_rounded(x)\
	((int)((_fixed_hi(x) << (16-_fixed_shift)) +\
	       (((_fixed_lo(x) >> (_fixed_shift-1))+1)>>1)))
#  define fixed2int_var_ceiling(x)\
	(fixed2int_var(x) -\
	 arith_rshift((int)-(_fixed_lo(x) & _fixed_fraction_v), _fixed_shift))
#else
/* Use reasonable definitions. */
#  define fixed2int_var(x) fixed2int(x)
#  define fixed2int_var_rounded(x) fixed2int_rounded(x)
#  define fixed2int_var_ceiling(x) fixed2int_ceiling(x)
#endif
#define fixed2long(x) ((long)_fixed_rshift(x))
#define fixed2long_rounded(x) ((long)_fixed_rshift((x)+_fixed_round_v))
#define fixed2long_ceiling(x) ((long)_fixed_rshift((x)+_fixed_fraction_v))
#define float2fixed(f) ((fixed)((f)*(float)_fixed_scale))
/* Note that fixed2float actually produces a double result. */
#define fixed2float(x) ((x)*(1.0/_fixed_scale))

/* Rounding and truncation on fixeds */
#define fixed_floor(x) ((x)&(-1L<<_fixed_shift))
#define fixed_rounded(x) (((x)+_fixed_round_v)&(-1L<<_fixed_shift))
#define fixed_ceiling(x) (((x)+_fixed_fraction_v)&(-1L<<_fixed_shift))
#define fixed_fraction(x) ((int)(x)&_fixed_fraction_v)
/* I don't see how to do truncation towards 0 so easily.... */
#define fixed_truncated(x) ((x) < 0 ? fixed_ceiling(x) : fixed_floor(x))

/*
 * Transforming coordinates involves multiplying two floats, or a float
 * and a double, and then converting the result to a fixed.  Since this
 * operation is so common, we provide an alternative implementation of it
 * on machines that use IEEE floating point representation but don't have
 * floating point hardware.  The implementation may be in either C or
 * assembler.
 */

#ifndef USE_FPU
#  define USE_FPU 0
#endif
#if USE_FPU < 0 && arch_sizeof_short == 2 && arch_sizeof_long == 4
int set_fmul2fixed_(P3(fixed *, long, long));
#define set_fmul2fixed_vars(vr,vfa,vfb,dtemp)\
  set_fmul2fixed_(&vr, *(long *)&vfa, *(long *)&vfb)
int set_dfmul2fixed_(P4(fixed *, ulong, long, long));
#  if arch_is_big_endian
#  define set_dfmul2fixed_vars(vr,vda,vfb,dtemp)\
     set_dfmul2fixed_(&vr, ((ulong *)&vda)[1], *(long *)&vfb, *(long *)&vda)
#  else
#  define set_dfmul2fixed_vars(vr,vda,vfb,dtemp)\
     set_dfmul2fixed_(&vr, *(ulong *)&vda, *(long *)&vfb, ((long *)&vda)[1])
#  endif
#else			/* don't bother */
#  define set_fmul2fixed_vars(vr,vfa,vfb,dtemp)\
     (dtemp = (vfa) * (vfb),\
      (f_fits_in_bits(dtemp, fixed_int_bits) ? (vr = float2fixed(dtemp), 0) :\
       gs_note_error(gs_error_limitcheck)))
#  define set_dfmul2fixed_vars(vr,vda,vfb,dtemp)\
     (dtemp = (vda) * (vfb),\
      (f_fits_in_bits(dtemp, fixed_int_bits) ? (vr = float2fixed(dtemp), 0) :\
       gs_note_error(gs_error_limitcheck)))
#endif

/* A point with fixed coordinates */
typedef struct gs_fixed_point_s {
	fixed x, y;
} gs_fixed_point;

/* A rectangle with fixed coordinates */
typedef struct gs_fixed_rect_s {
	gs_fixed_point p, q;
} gs_fixed_rect;

#endif					/* gxfixed_INCLUDED */
