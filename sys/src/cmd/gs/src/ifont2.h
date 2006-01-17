/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ifont2.h,v 1.6 2003/08/21 14:55:14 igor Exp $ */
/* Type 2 font utilities 2 */

#ifndef ifont2_INCLUDED
#  define ifont2_INCLUDED

/* Default value of lenIV */
#define DEFAULT_LENIV_2 (-1)

/*
 * Get the additional parameters for a Type 2 font (or FontType 2 FDArray
 * entry in a CIDFontType 0 font), beyond those common to Type 1 and Type 2
 * fonts.
 */
int type2_font_params(const_os_ptr op, charstring_font_refs_t *pfr,
		      gs_type1_data *pdata1);

#endif /* ifont2_INCLUDED */
