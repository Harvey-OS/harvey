/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsrect.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Rectangle utilities */

#ifndef gsrect_INCLUDED
#  define gsrect_INCLUDED

#include "gxfixed.h"

/* Check whether one rectangle is included entirely within another. */
#define rect_within(inner, outer)\
  ((inner).q.y <= (outer).q.y && (inner).q.x <= (outer).q.x &&\
   (inner).p.y >= (outer).p.y && (inner).p.x >= (outer).p.x)

/*
 * Intersect two rectangles, replacing the first.  The result may be
 * anomalous (q < p) if the intersection is empty.
 */
#define rect_intersect(to, from)\
  BEGIN\
    if ((from).p.x > (to).p.x) (to).p.x = (from).p.x;\
    if ((from).q.x < (to).q.x) (to).q.x = (from).q.x;\
    if ((from).p.y > (to).p.y) (to).p.y = (from).p.y;\
    if ((from).q.y < (to).q.y) (to).q.y = (from).q.y;\
  END

/*
 * Merge two rectangles, replacing the first.  The result may be
 * anomalous (q < p) if the first rectangle was anomalous.
 */
#define rect_merge(to, from)\
  BEGIN\
    if ((from).p.x < (to).p.x) (to).p.x = (from).p.x;\
    if ((from).q.x > (to).q.x) (to).q.x = (from).q.x;\
    if ((from).p.y < (to).p.y) (to).p.y = (from).p.y;\
    if ((from).q.y > (to).q.y) (to).q.y = (from).q.y;\
  END

/*
 * Calculate the difference of two rectangles, a list of up to 4 rectangles.
 * Return the number of rectangles in the list, and set the first rectangle
 * to the intersection.  The resulting first rectangle is guaranteed not to
 * be anomalous (q < p) iff it was not anomalous originally.
 *
 * Note that unlike the macros above, we need different versions of this
 * depending on the data type of the individual values: we'll only implement
 * the variations that we need.
 */
int int_rect_difference(P3(gs_int_rect * outer, const gs_int_rect * inner,
			   gs_int_rect * diffs /*[4] */ ));

/*
 * Check whether a parallelogram is a rectangle.
 */
#define PARALLELOGRAM_IS_RECT(ax, ay, bx, by)\
  ( ((ax) | (by)) == 0 || ((bx) | (ay)) == 0 )

/*
 * Convert a rectangular parallelogram to a rectangle, carefully following
 * the center-of-pixel rule in all cases.
 */
#define INT_RECT_FROM_PARALLELOGRAM(prect, px, py, ax, ay, bx, by)\
  BEGIN\
    int px_ = fixed2int_pixround(px);\
    int py_ = fixed2int_pixround(py);\
    int qx_ = fixed2int_pixround((px) + (ax) + (bx));  /* only one is non-zero */\
    int qy_ = fixed2int_pixround((py) + (ay) + (by));  /* ditto */\
\
    if (qx_ < px_)\
      (prect)->p.x = qx_, (prect)->q.x = px_;\
    else\
      (prect)->p.x = px_, (prect)->q.x = qx_;\
    if (qy_ < py_)\
      (prect)->p.y = qy_, (prect)->q.y = py_;\
    else\
      (prect)->p.y = py_, (prect)->q.y = qy_;\
  END

#endif /* gsrect_INCLUDED */
