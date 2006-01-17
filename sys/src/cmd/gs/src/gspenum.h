/* Copyright (C) 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gspenum.h,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Common definitions for client interface to path enumeration */

#ifndef gspenum_INCLUDED
#  define gspenum_INCLUDED

/* Define the path element types. */
#define gs_pe_moveto 1
#define gs_pe_lineto 2
#define gs_pe_curveto 3
#define gs_pe_closepath 4

/* Define an abstract type for the path enumerator. */
typedef struct gs_path_enum_s gs_path_enum;

#endif /* gspenum_INCLUDED */
