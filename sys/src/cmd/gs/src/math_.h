/* Copyright (C) 1989, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: math_.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
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
#ifdef _IBMR2
/* The RS/6000 has hypot, but math.h doesn't declare it! */
extern double hypot(double, double);
#else
#  if !defined(__TURBOC__) && !defined(BSD4_2) && !defined(VMS) && !defined(__MWERKS__)
#    define hypot(x,y) sqrt((x)*(x)+(y)*(y))
#  endif
#endif

#ifdef OSK
/* OSK has atan2 and ldexp, but math.h doesn't declare them! */
extern double atan2(), ldexp();
#endif

/* Intercept calls on sqrt for debugging. */
extern double gs_sqrt(P3(double, const char *, int));
#ifdef DEBUG
#undef sqrt
#define sqrt(x) gs_sqrt(x, __FILE__, __LINE__)
#endif /* DEBUG */

#endif /* math__INCLUDED */
