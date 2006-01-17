/* Copyright (C) 2004 Artifex Software Inc., artofcode LLC.  All rights reserved.
  
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

/*$Id: gsequivc.h,v 1.2 2004/06/24 05:03:36 dan Exp $ */
/* Header for routines for determining equivalent color for spot colors */

/* For more information, see comment at the start of src/gsequivc.c */

#ifndef gsequivc_INCLUDED
# define gsequivc_INCLUDED

/*
 * Structure for holding a CMYK color.
 */
typedef struct cmyk_color_s {
    bool color_info_valid;
    frac c;
    frac m;
    frac y;
    frac k;
} cmyk_color;

/*
 * Structure for holding parameters for collecting the equivalent CMYK
 * for a spot colorant.
 */
typedef struct equivalent_cmyk_color_params_s {
    bool all_color_info_valid;
    cmyk_color color[GX_DEVICE_MAX_SEPARATIONS];
} equivalent_cmyk_color_params;

/* If possible, update the equivalent CMYK color for a spot color */
void update_spot_equivalent_cmyk_colors(gx_device * pdev,
		const gs_state * pgs, gs_devn_params * pdevn_params,
		equivalent_cmyk_color_params * pparams);

#endif		/* define gsequivc_INCLUDED */
