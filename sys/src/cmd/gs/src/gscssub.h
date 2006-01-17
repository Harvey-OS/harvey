/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscssub.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Client interface to color space substitution */

#ifndef gscssub_INCLUDED
#  define gscssub_INCLUDED

#include "gscspace.h"

/*
 * Color space substitution at the library level is similar to, but not
 * identical to, the operation of UseCIEColor in the PostScript language.
 * When the Boolean UseCIEColor parameter of the current device is false,
 * everything operates as normal.  When UseCIEColor is true, the following
 * procedures may substitute another color space for the implied one:
 *
 *	gs_setgray, gs_setrgbcolor, gs_sethsbcolor, gs_setcmykcolor
 *	gs_current_Device{Gray,RGB,CMYK}_space
 *
 * Unlike the PostScript facility, the substitution *is* visible to
 * gs_currentcolorspace, and does *not* affect gs_setcolorspace, or the
 * ColorSpace members of images or shadings.  However, the following
 * procedures recognize when substitution has occurred and return the
 * value(s) appropriate for the pre-substitution space:
 *
 *	gs_currentgray, gs_currentrgbcolor, gs_currenthsbcolor,
 *	  gs_currentcmykcolor
 *
 * Thus gs_{current,set}{gray,{rgb,hsb,cmyk}color} are always mutually
 * consistent, concealing any substitution, and gs_{current,set}{colorspace}
 * are mutually consistent, reflecting any substitution.
 * gs_{current,set}color are consistent with the other color accessors,
 * since color space substitution doesn't affect color values.
 *
 * As in PostScript, color space substitutions are not affected by
 * (ordinary) grestore or by setgstate.  Graphics states created by gsave or
 * gstate, or overwritten by currentgstate or copygstate, share
 * substitutions with the state from which they were copied.
 */

/* If pcs is NULL, it means undo any substitution. */
int gs_setsubstitutecolorspace(gs_state *pgs, gs_color_space_index csi,
			       const gs_color_space *pcs);
const gs_color_space *
    gs_currentsubstitutecolorspace(const gs_state *pgs,
				   gs_color_space_index csi);

/*
 * The following procedures are primarily for internal use, to provide
 * fast access to specific color spaces.
 */
const gs_color_space *gs_current_DeviceGray_space(const gs_state *pgs);
const gs_color_space *gs_current_DeviceRGB_space(const gs_state *pgs);
const gs_color_space *gs_current_DeviceCMYK_space(const gs_state *pgs);

#endif /* gscssub_INCLUDED */
