/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxftype.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Definition of font type and bitmap font behavior */

#ifndef gxftype_INCLUDED
#  define gxftype_INCLUDED

/* Define the known font types. */
/* These numbers must be the same as the values of FontType */
/* in font dictionaries. */
typedef enum {
    ft_composite = 0,
    ft_encrypted = 1,
    ft_encrypted2 = 2,
    ft_user_defined = 3,
    ft_disk_based = 4,
    ft_CID_encrypted = 9,	/* CIDFontType 0 */
    ft_CID_user_defined = 10,	/* CIDFontType 1 */
    ft_CID_TrueType = 11,	/* CIDFontType 2 */
    ft_Chameleon = 14,
    ft_CID_bitmap = 32,		/* CIDFontType 4 */
    ft_TrueType = 42
} font_type;

/* Define the bitmap font behaviors. */
/* These numbers must be the same as the values of the ExactSize, */
/* InBetweenSize, and TransformedChar entries in font dictionaries. */
typedef enum {
    fbit_use_outlines = 0,
    fbit_use_bitmaps = 1,
    fbit_transform_bitmaps = 2
} fbit_type;

#endif /* gxftype_INCLUDED */
