/* Copyright (C) 1995, 1996 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxcvalue.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Definition of device color values */

#ifndef gxcvalue_INCLUDED
#  define gxcvalue_INCLUDED

/* Define the type for gray or RGB values at the driver interface. */
typedef unsigned short gx_color_value;

#define arch_sizeof_gx_color_value arch_sizeof_short
/* We might use less than the full range someday. */
/* ...bits must lie between 8 and 16. */
#define gx_color_value_bits (sizeof(gx_color_value) * 8)
#define gx_max_color_value ((gx_color_value)((1L << gx_color_value_bits) - 1))
#define gx_color_value_to_byte(cv)\
  ((cv) >> (gx_color_value_bits - 8))
#define gx_color_value_from_byte(cb)\
  (((cb) << (gx_color_value_bits - 8)) + ((cb) >> (16 - gx_color_value_bits)))

/* Convert between gx_color_values and fracs. */
#define frac2cv(fr) frac2ushort(fr)
#define cv2frac(cv) ushort2frac(cv)

#endif /* gxcvalue_INCLUDED */
