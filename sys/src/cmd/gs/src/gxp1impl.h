/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxp1impl.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* PatternType 1 implementation interface */
/* Requires gxpcolor.h */

#ifndef gxp1impl_INCLUDED
#  define gxp1impl_INCLUDED

/*
 * Declare the filling algorithms implemented in gxp1fill.c.
 * We use 'masked_fill_rect' instead of 'masked_fill_rectangle'
 * in order to limit identifier lengths to 32 characters.
 */
dev_color_proc_fill_rectangle(gx_dc_pattern_fill_rectangle);
dev_color_proc_fill_rectangle(gx_dc_pure_masked_fill_rect);
dev_color_proc_fill_rectangle(gx_dc_binary_masked_fill_rect);
dev_color_proc_fill_rectangle(gx_dc_colored_masked_fill_rect);

/*
 * Declare the Pattern color mapping procedures exported by gxpcmap.c.
 */
int gx_pattern_load(gx_device_color *, const gs_imager_state *,
		    gx_device *, gs_color_select_t);
pattern_proc_remap_color(gs_pattern1_remap_color);

#endif /* gxp1impl_INCLUDED */
