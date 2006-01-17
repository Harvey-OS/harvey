/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.

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

/* $Id: ttcalc.h,v 1.1 2003/10/01 13:44:56 igor Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */

/*******************************************************************
 *
 *  ttcalc.h
 *
 *    Arithmetic Computations (specification).
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute 
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#ifndef TTCALC_H
#define TTCALC_H

#include "ttcommon.h"
#include "tttypes.h"

  /* IntN types:                                                        */
  /*                                                                    */
  /*   Used to guarantee the size of some specific integers.            */
  /*                                                                    */
  /*  Of course, they are equivalent to Short, UShort, Long, etc.       */
  /*  but parts of this unit could be used by different programs.       */
  /*                                                                    */

  typedef signed short    Int16;
  typedef unsigned short  Word16;

#if SIZEOF_INT == 4

  typedef signed int      Int32;
  typedef unsigned int    Word32;

#elif SIZEOF_LONG == 4

  typedef signed long     Int32;
  typedef unsigned long   Word32;

#else
#error "no 32bit type found"
#endif

#if SIZEOF_LONG == 8

/* LONG64 must be defined when a 64-bit type is available */
#define LONG64
#define INT64   long

#else

/* GCC provides the non-ANSI 'long long' 64-bit type.  You can activate */
/* by defining the _GNUC_LONG64_ macro in 'ttconfig.h'.  Note that this */
/* will produce many -ansi warnings during library compilation.         */
#ifdef _GNUC_LONG64_

#define LONG64
#define INT64   long long

#endif /* __GNUC__ */

/* Microsoft Visual C++ also supports 64-bit integers */
#ifdef _MSC_VER
#if _MSC_VER >= 1100

#define LONG64
#define INT64   __int64

#endif
#endif

#endif


#ifdef __cplusplus
  extern "C" {
#endif

#ifdef LONG64

  typedef INT64  Int64;

/* Fast muldiv */
#define FMulDiv( x, y, z )        ( (Int64)(x) * (y) / (z) )

/* Fast muldiv_round */
#define FMulDiv_Round( x, y, z )  ( ( (Int64)(x) * (y) + (z)/2 ) / (z) )

#define ADD_64( x, y, z )  ( (z) = (x) + (y) )
#define SUB_64( x, y, z )  ( (z) = (x) - (y) )
#define MUL_64( x, y, z )  ( (z) = (Int64)(x) * (y) )

#define DIV_64( x, y )     ( (x) / (y) )

#define SQRT_64( x )       Sqrt64( x )
#define SQRT_32( x )       Sqrt32( x )

  Int32  MulDiv( Int32  a, Int32  b, Int32  c );
  Int32  MulDiv_Round( Int32  a, Int32  b, Int32  c );

  Int32  Sqrt32( Int32  l );
  Int32  Sqrt64( Int64  l );

#else /* LONG64 */

  struct  _Int64
  {
    Word32  lo;
    Word32  hi;
  };

  typedef struct _Int64  Int64;

#define FMulDiv( x, y, z )        MulDiv( x, y, z )
#define FMulDiv_Round( x, y, z )  MulDiv_Round( x, y, z )

#define ADD_64( x, y, z )  Add64( &x, &y, &z )
#define SUB_64( x, y, z )  Sub64( &x, &y, &z )
#define MUL_64( x, y, z )  MulTo64( x, y, &z )

#define DIV_64( x, y )     Div64by32( &x, y )

#define SQRT_64( x )       Sqrt64( &x )
#define SQRT_32( x )       Sqrt32( x )

  Int32  MulDiv( Int32  a, Int32  b, Int32  c );
  Int32  MulDiv_Round( Int32  a, Int32  b, Int32  c );

  void  Add64( Int64*  x, Int64*  y, Int64*  z );
  void  Sub64( Int64*  x, Int64*  y, Int64*  z );

  void  MulTo64( Int32  x, Int32  y, Int64*  z );

  Int32  Div64by32( Int64*  x, Int32  y );

  Int  Order64( Int64*  z );
  Int  Order32( Int32   z );

  Int32  Sqrt32( Int32   l );
  Int32  Sqrt64( Int64*  l );

#endif /* LONG64 */


#define MUL_FIXED( a, b )      MulDiv_Round( (a), (b), 1 << 16 )
#define INT_TO_F26DOT6( x )    ( (Long)(x) << 6  )
#define INT_TO_F2DOT14( x )    ( (Long)(x) << 14 )
#define INT_TO_FIXED( x )      ( (Long)(x) << 16 )
#define F2DOT14_TO_FIXED( x )  ( (Long)(x) << 2  )
#define FLOAT_TO_FIXED( x )    ( (Long)((x) * 65536.0) )

#define ROUND_F26DOT6( x )     ( (x) >= 0 ? (   ((x) + 32) & -64) \
                                          : ( -((32 - (x)) & -64) ) )

#ifdef __cplusplus
  }
#endif

#endif /* TTCALC_H */


/* END */
