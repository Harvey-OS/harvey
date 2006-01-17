/* Copyright (C) 1994, 1995, 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxpaint.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Internal interface to fill/stroke */
/* Requires gsropt.h, gxfixed.h, gxpath.h */

#ifndef gxpaint_INCLUDED
#  define gxpaint_INCLUDED

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif

#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;
#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

/* ------ Graphics-state-aware procedures ------ */

/*
 * The following procedures use information from the graphics state.
 * They are implemented in gxpaint.c.
 */

int gx_fill_path(gx_path * ppath, gx_device_color * pdevc, gs_state * pgs,
		 int rule, fixed adjust_x, fixed adjust_y);
int gx_stroke_fill(gx_path * ppath, gs_state * pgs);
int gx_stroke_add(gx_path *ppath, gx_path *to_path, const gs_state * pgs);
/*
 * gx_imager_stroke_add needs a device for the sake of absolute-length
 * dots (and for no other reason).
 */
int gx_imager_stroke_add(gx_path *ppath, gx_path *to_path,
			 gx_device *dev, const gs_imager_state *pis);

/* ------ Imager procedures ------ */

/*
 * Tweak the fill adjustment if necessary so that (nearly) empty
 * rectangles are guaranteed to produce some output.
 */
void gx_adjust_if_empty(const gs_fixed_rect *, gs_fixed_point *);

/*
 * Compute the amount by which to expand a stroked bounding box to account
 * for line width, caps and joins.  If the amount is too large to fit in a
 * gs_fixed_point, return gs_error_limitcheck.  Return 0 if the result is
 * exact, 1 if it is conservative.
 *
 * This procedure is fast, but the result may be conservative by a large
 * amount if the miter limit is large.  If this matters, use strokepath +
 * pathbbox.
 */
int gx_stroke_path_expansion(const gs_imager_state *pis,
			     const gx_path *ppath, gs_fixed_point *ppt);

/* Backward compatibility */
#define gx_stroke_expansion(pis, ppt)\
  gx_stroke_path_expansion(pis, (const gx_path *)0, ppt)

/*
 * The following procedures do not need a graphics state.
 * These procedures are implemented in gxfill.c and gxstroke.c.
 */

/* Define the parameters passed to the imager's filling routine. */
#ifndef gx_fill_params_DEFINED
#  define gx_fill_params_DEFINED
typedef struct gx_fill_params_s gx_fill_params;
#endif
struct gx_fill_params_s {
    int rule;			/* -1 = winding #, 1 = even/odd */
    gs_fixed_point adjust;
    float flatness;
    bool fill_zero_width;	/* if true, make zero-width/height */
    /* rectangles one pixel wide/high */
};

#define gx_fill_path_only(ppath, dev, pis, params, pdevc, pcpath)\
  (*dev_proc(dev, fill_path))(dev, pis, ppath, params, pdevc, pcpath)

/* Define the parameters passed to the imager's stroke routine. */
#ifndef gx_stroke_params_DEFINED
#  define gx_stroke_params_DEFINED
typedef struct gx_stroke_params_s gx_stroke_params;
#endif
struct gx_stroke_params_s {
    float flatness;
};

int gx_stroke_path_only(gx_path * ppath, gx_path * to_path, gx_device * dev,
			const gs_imager_state * pis,
			const gx_stroke_params * params,
			const gx_device_color * pdevc,
			const gx_clip_path * pcpath);

#endif /* gxpaint_INCLUDED */
