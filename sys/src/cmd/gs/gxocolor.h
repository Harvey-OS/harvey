/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gxocolor.h */
/* Concrete color representation for Ghostscript */

#ifndef gxocolor_INCLUDED
#  define gxocolor_INCLUDED

#include "gxfrac.h"

/*
 * A concrete color is a color that has been reduced to a device space
 * (gray, RGB, CMYK, or separation) but has not yet been subjected to
 * space conversion, transfer, or halftoning.  This is the level of
 * abstraction at which image resampling occurs.  (Currently,
 * this is the *only* use for concrete colors.)
 *
 * Concrete colors identify their color space, because they might have been
 * produced by converting an Indexed or Separation color.
 */

/* Define an opaque type for a color space. */
/* (The actual definition is in gscspace.h.) */
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif

#ifndef gx_concrete_color_DEFINED
#  define gx_concrete_color_DEFINED
typedef struct gx_concrete_color_s gx_concrete_color;
#endif

struct gx_concrete_color_s {
	frac values[4];
	const gs_color_space *color_space;
};

/* Concrete colors only exist transiently, */
/* so they don't need a structure type. */

#endif					/* gxocolor_INCLUDED */
