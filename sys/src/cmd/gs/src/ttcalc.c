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

/* $Id: ttcalc.c,v 1.1 2003/10/01 13:44:56 igor Exp $ */

/* Changes after FreeType: cut out the TrueType instruction interpreter. */

/*******************************************************************
 *
 *  ttcalc.c
 *
 *    Arithmetic Computations (body).
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

#include "ttmisc.h"

#include "ttcalc.h"

/* support for 1-complement arithmetic has been totally dropped in this */
/* release. You can still write your own code if you need it..          */

  static const long  Roots[63] =
  {
       1,    1,    2,     3,     4,     5,     8,    11,
      16,   22,   32,    45,    64,    90,   128,   181,
     256,  362,  512,   724,  1024,  1448,  2048,  2896,
    4096, 5892, 8192, 11585, 16384, 23170, 32768, 46340,

      65536,   92681,  131072,   185363,   262144,   370727,
     524288,  741455, 1048576,  1482910,  2097152,  2965820,
    4194304, 5931641, 8388608, 11863283, 16777216, 23726566,

      33554432,   47453132,   67108864,   94906265,
     134217728,  189812531,  268435456,  379625062,
     536870912,  759250125, 1073741824, 1518500250,
    2147483647
  };


#ifdef LONG64

  Int32  MulDiv( Int32  a, Int32  b, Int32  c )
  {
    Int32  s;

    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);
    s ^= c; c = ABS(c);

    a = (Int64)a * b / c;
    return ((s < 0) ? -a : a);
  }


  Int32  MulDiv_Round( Int32  a, Int32  b, Int32  c )
  {
    int  s;

    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);
    s ^= c; c = ABS(c);

    a = ((Int64)a * b + c/2) / c;
    return ((s < 0) ? -a : a);
  }


  Int  Order64( Int64  z )
  {
    int  j = 0;
    while ( z )
    {
      z = (unsigned INT64)z >> 1;
      j++;
    }
    return j - 1;
  }


  Int32  Sqrt64( Int64  l )
  {
    Int64  r, s;

    if ( l <= 0 ) return 0;
    if ( l == 1 ) return 1;

    r = Roots[Order64( l )];

    do
    {
      s = r;
      r = ( r + l/r ) >> 1;
    }
    while ( r > s || r*r > l );

    return r;
  }

#else /* LONG64 */

  Int32  MulDiv( Int32  a, Int32  b, Int32  c )
  {
    Int64  temp;
    Int32  s;

    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);
    s ^= c; c = ABS(c);

    MulTo64( a, b, &temp );
    a = Div64by32( &temp, c );

    return ((s < 0) ? -a : a);
  }


  Int32  MulDiv_Round( Int32  a, Int32  b, Int32  c )
  {
    Int64  temp, temp2;
    Int32  s;

    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);
    s ^= c; c = ABS(c);

    MulTo64( a, b, &temp );
    temp2.hi = (Int32)(c >> 31);
    temp2.lo = (Word32)(c / 2);
    Add64( &temp, &temp2, &temp );
    a = Div64by32( &temp, c );

    return ((s < 0) ? -a : a);
  }


  static void  Neg64__( Int64*  x )
  {
    /* Remember that -(0x80000000) == 0x80000000 with 2-complement! */
    /* We take care of that here.                                   */

    x->hi ^= 0xFFFFFFFF;
    x->lo ^= 0xFFFFFFFF;
    x->lo++;

    if ( !x->lo )
    {
      x->hi++;
      if ( (Int32)x->hi == 0x80000000 )  /* Check -MaxInt32 - 1 */
      {
        x->lo--;
        x->hi--;  /* We return 0x7FFFFFFF! */
      }
    }
  }


  void  Add64( Int64*  x, Int64*  y, Int64*  z )
  {
    register Word32  lo, hi;

    hi = x->hi + y->hi;
    lo = x->lo + y->lo;

    if ( y->lo )
      if ( (Word32)x->lo >= (Word32)(-y->lo) ) hi++;

    z->lo = lo;
    z->hi = hi;
  }


  void  Sub64( Int64*  x, Int64*  y, Int64*  z )
  {
    register Word32  lo, hi;

    hi = x->hi - y->hi;
    lo = x->lo - y->lo;

    if ( x->lo < y->lo ) hi--;

    z->lo = lo;
    z->hi = hi;
  }


  void  MulTo64( Int32  x, Int32  y, Int64*  z )
  {
    Int32   s;
    Word32  lo1, hi1, lo2, hi2, lo, hi, i1, i2;

    s  = x; x = ABS(x);
    s ^= y; y = ABS(y);

    lo1 = x & 0x0000FFFF;  hi1 = x >> 16;
    lo2 = y & 0x0000FFFF;  hi2 = y >> 16;

    lo = lo1*lo2;
    i1 = lo1*hi2;
    i2 = lo2*hi1;
    hi = hi1*hi2;

    /* Check carry overflow of i1 + i2 */

    if ( i2 )
    {
      if ( i1 >= (Word32)-i2 ) hi += 1 << 16;
      i1 += i2;
    }

    i2 = i1 >> 16;
    i1 = i1 << 16;

    /* Check carry overflow of i1 + lo */
    if ( i1 )
    {
      if ( lo >= (Word32)-i1 ) hi++;
      lo += i1;
    }

    hi += i2;

    z->lo = lo;
    z->hi = hi;

    if (s < 0) Neg64__( z );
  }


  Int32  Div64by32( Int64*  x, Int32  y )
  {
    Int32   s;
    Word32  q, r, i, lo;

    s  = x->hi; if (s<0) Neg64__(x);
    s ^= y;     y = ABS(y);

    /* Shortcut */
    if ( x->hi == 0 )
    {
      q = x->lo / y;
      return ((s<0) ? -q : q);
    }

    r  = x->hi;
    lo = x->lo;

    if ( r >= (Word32)y )   /* we know y is to be treated as unsigned here */
      return ( (s<0) ? 0x80000001 : 0x7FFFFFFF );
                            /* Return Max/Min Int32 if divide overflow */
                            /* This includes division by zero!         */
    q = 0;
    for ( i = 0; i < 32; i++ )
    {
      r <<= 1;
      q <<= 1;
      r  |= lo >> 31;

      if ( r >= (Word32)y )
      {
        r -= y;
        q |= 1;
      }
      lo <<= 1;
    }

    return ( (s<0) ? -q : q );
  }


  Int  Order64( Int64*  z )
  {
    Word32  i;
    int     j;

    if ( z->hi )
    {
      i = z->hi;
      j = 32;
    }
    else
    {
      i = z->lo;
      j = 0;
    }

    while ( i > 0 )
    {
      i >>= 1;
      j++;
    }
    return j-1;
  }


  Int32  Sqrt64( Int64*  l )
  {
    Int64  l2;
    Int32  r, s;

    if ( (Int32)l->hi < 0          ||
        (l->hi == 0 && l->lo == 0) )  return 0;

    s = Order64( l );
    if ( s == 0 ) return 1;

    r = Roots[s];
    do
    {
      s = r;
      r = ( r + Div64by32(l,r) ) >> 1;
      MulTo64( r, r,   &l2 );
      Sub64  ( l, &l2, &l2 );
    }
    while ( r > s || (Int32)l2.hi < 0 );

    return r;
  }

#endif /* LONG64 */

#if 0  /* unused by the rest of the library */

  Int  Order32( Int32  z )
  {
    int j;

    j = 0;
    while ( z )
    {
      z = (Word32)z >> 1;
      j++;
    }
    return j - 1;
  }


  Int32  Sqrt32( Int32  l )
  {
    Int32  r, s;

    if ( l <= 0 ) return 0;
    if ( l == 1 ) return 1;

    r = Roots[Order32( l )];
    do
    {
      s = r;
      r = ( r + l/r ) >> 1;
    }
    while ( r > s || r*r > l );
    return r;
  }

#endif

/* END */
