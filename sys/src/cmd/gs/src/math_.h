/* Copyright (C) 1989, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: math_.h,v 1.7 2004/04/08 16:18:25 giles Exp $ */
/* Generic substitute for math.h */

#ifndef math__INCLUDED
#  define math__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

#if defined(VMS) && defined(__GNUC__)
/*  DEC VAX/VMS C comes with a math.h file, but GNU VAX/VMS C does not. */
#  include "vmsmath.h"
#else
#  include <math.h>
#endif

/* math.h is different for Turbo and Unix.... */
#ifndef M_PI
#  ifdef PI
#    define M_PI PI
#  else
#    define M_PI 3.14159265358979324
#  endif
#endif

/* Factors for converting between degrees and radians */
#define degrees_to_radians (M_PI / 180.0)
#define radians_to_degrees (180.0 / M_PI)

/*
 * Define the maximum value of a single-precision float.
 * This doesn't seem to be defined in any standard place,
 * and we need an exact value for it.
 */
#undef MAX_FLOAT		/* just in case */
#if defined(vax) || defined(VAX) || defined(__vax) || defined(__VAX)
/* Maximum exponent is +127, 23 bits of fraction. */
#  define MAX_FLOAT\
     ((0x800000 - 1.0) * 0x1000000 * 0x1000000 * 0x10000000 * 0x10000000)
#else
/* IEEE, maximum exponent is +127, 23 bits of fraction + an implied '1'. */
#  define MAX_FLOAT\
     ((0x1000000 - 1.0) * 0x1000000 * 0x1000000 * 0x10000000 * 0x10000000)
#endif

/* Define the hypot procedure on those few systems that don't provide it. */
#if defined(_IBMR2)
/* The RS/6000 has hypot, but math.h doesn't declare it! */
extern double hypot(double, double);
#elif defined(_MSC_VER)
#  define hypot(x,y) _hypot(x,y)
#elif !defined(__TURBOC__) && !defined(BSD4_2) && !defined(VMS) && !defined(__MWERKS__) && !defined(HAVE_HYPOT)
#  define hypot(x,y) sqrt((double)(x)*(x)+(double)(y)*(y))
#endif

#ifdef OSK
/* OSK has atan2 and ldexp, but math.h doesn't declare them! */
extern double atan2(), ldexp();
#endif

/* Intercept calls on sqrt for debugging. */
extern double gs_sqrt(double, const char *, int);
#ifdef DEBUG
#undef sqrt
#define sqrt(x) gs_sqrt(x, __FILE__, __LINE__)
#endif /* DEBUG */

#endif /* math__INCLUDED */
