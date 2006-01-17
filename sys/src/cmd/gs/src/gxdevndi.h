/* Copyright (C) 1994, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxdevndi.h,v 1.1 2002/08/22 07:12:29 henrys Exp $ */
/* Interface to gxdevndi.c */

/* This is the halfoning handlers for DeviceN colors */

#ifndef gxdevndi_INCLUDED
#  define gxdevndi_INCLUDED

#include "gxfrac.h"

#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

/* Render a color, possibly by halftoning. */
/* Return as for gx_render_[device_]gray. */
int gx_render_device_color_devn(P10(frac red, frac green, frac blue, frac white,
			       bool cmyk, gx_color_value alpha,
			       gx_device_color * pdevc, gx_device * dev,
			       gx_device_halftone * pdht,
			       const gs_int_point * ht_phase));


#endif /* gxdevndi_INCLUDED */
