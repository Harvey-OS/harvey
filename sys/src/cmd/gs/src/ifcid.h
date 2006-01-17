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

/* $Id: ifcid.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Interface to zfcid.c */

#ifndef ifcid_INCLUDED
#  define ifcid_INCLUDED

/* Get the CIDSystemInfo of a CIDFont. */
int cid_font_system_info_param(gs_cid_system_info_t *pcidsi,
			       const ref *prfont);

/* Get the additional information for a CIDFontType 0 or 2 CIDFont. */
int cid_font_data_param(os_ptr op, gs_font_cid_data *pdata,
			ref *pGlyphDirectory);

#endif /* ifcid_INCLUDED */
