/* Copyright (C) 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxline.h,v 1.7 2005/08/17 14:40:11 igor Exp $ */
/* Private line parameter definitions */

#ifndef gxline_INCLUDED
#  define gxline_INCLUDED

#include "gslparam.h"
#include "gsmatrix.h"

/* Line parameter structures */
/* gx_dash_params are never instantiated by themselves. */
typedef struct gx_dash_params_s {
    float *pattern;
    uint pattern_size;
    float offset;
    bool adapt;
    /* The rest of the parameters are computed from the above */
    float pattern_length;	/* total of all pattern elements */
    bool init_ink_on;		/* true if ink is initially on */
    int init_index;		/* initial index in pattern */
    float init_dist_left;
} gx_dash_params;

#define gx_dash_params_initial\
  NULL, 0, 0.0, 0/*false*/, 0.0, 1/*true*/, 0, 0.0
typedef struct gx_line_params_s {
    float half_width;		/* one-half line width */
    gs_line_cap cap;
    gs_line_join join;
    int curve_join;		/* <0 means use join between segments of */
				/* flattened curves, >=0 means use this join */
    float miter_limit;
    float miter_check;		/* computed from miter limit, */
				/* see gx_set_miter_limit and gs_stroke */
    float dot_length;
    bool dot_length_absolute;	/* if true, dot_length is 1/72" units */
    gs_matrix dot_orientation;	/* dot length is aligned with (1,0); */
				/* must be xxyy or xyyx */
    gx_dash_params dash;
} gx_line_params;

#define gx_set_line_width(plp, wid)\
  ((plp)->half_width = (wid) / 2)
#define gx_current_line_width(plp)\
  ((plp)->half_width * 2)
int gx_set_miter_limit(gx_line_params *, floatp);

#define gx_current_miter_limit(plp)\
  ((plp)->miter_limit)
int gx_set_dash(gx_dash_params *, const float *, uint, floatp, gs_memory_t *);

#define gx_set_dash_adapt(pdp, adpt) ((pdp)->adapt = (adpt))
int gx_set_dot_length(gx_line_params *, floatp, bool);

/* See gsline.c for the computation of miter_check. */
#define gx_line_params_initial\
 0.0, gs_cap_butt, gs_join_miter, -1,\
 10.0, (float)0.20305866, 0.0, 0/*false*/,\
  { identity_matrix_body }, { gx_dash_params_initial }

#endif /* gxline_INCLUDED */
