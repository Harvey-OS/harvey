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

/* gxdda.h */
/* DDA definitions for Ghostscript line drawing */
/* Requires gxfixed.h */

/*
 * We use the familiar Bresenham DDA algorithm for several purposes:
 *	- tracking the edges when filling trapezoids;
 *	- tracking the current pixel corner coordinates when rasterizing
 *	skewed or rotated images;
 *	- converting curves to sequences of lines (this is a 3rd-order
 *	DDA, the others are 1st-order);
 *	- perhaps someday for drawing single-pixel lines.
 * In the case of trapezoids, lines, and curves, we need to use
 * the DDA to find the integer X values at integer+0.5 values of Y;
 * in the case of images, we use the DDA to compute both the (fixed)
 * X and Y values at (integer) source pixel corners.
 */
typedef struct gx_dda_int_s {
	int current, dq;
	fixed r, dr, max_r;
} gx_dda_int;
typedef struct gx_dda_fixed_s {
	fixed current, dq;
	uint r, dr, max_r;
} gx_dda_fixed;
#define dda_fixed_init(dda, v, d, n)\
  (dda).current = (v);\
  if ( (d) < 0 )\

/*
 * The incrementing algorithm is the same in all cases.
 * Note that this requires some adjustment if the difference is negative.
 */
#define dda_next(dda)\
  (dda).current +=\
    (((dda).r += (dda).dr) > (dda).max_r ?\
     ((dda).r -= (dda).dr, (dda).dq + 1) : (dda).dq)
