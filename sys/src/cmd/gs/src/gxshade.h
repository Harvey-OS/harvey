/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxshade.h,v 1.12 2005/01/31 03:08:43 igor Exp $ */
/* Internal definitions for shading rendering */

#ifndef gxshade_INCLUDED
#  define gxshade_INCLUDED

#include "gsshade.h"
#include "gxfixed.h"		/* for gxmatrix.h */
#include "gxmatrix.h"		/* for gs_matrix_fixed */
#include "stream.h"

/*
   All shadings are defined with respect to some parameter that varies
   continuously over some range; the shading defines a mapping from the
   parameter values to colors and user space coordinates.  Here are the
   mappings for the 7 currently defined shading types:

   Type Param space     Param => color          Param => User space
   ---- -----------     --------------          -------------------
   1    2-D Domain      Function                Matrix
   2    1-D Domain      Function + Extend       perp. to Coords
   3    1-D Domain      Function + Extend       circles per Coords
   4,5  triangle x      Gouraud interp. on      Gouraud interp. on
	2-D in tri.	Decode => corner        triangle corners
			values => Function
   6    patch x (u,v)   Decode => bilinear      Sc + Sd - Sb on each patch
	in patch	interp. on corner
			values => Function
   7    see 6           see 6                   Sum(i) Sum(j) Pij*Bi(u)*Bj(v)

   To be able to render a portion of a shading usefully, we must be able to
   do two things:

   - Determine what range of parameter values is sufficient to cover
   the region being filled;

   - Evaluate the color at enough points to fill the region (in
   device space).

   Note that the latter may be implemented by a mix of evaluation and
   interpolation, especially for types 3, 6, and 7 where an exact mapping
   may be prohibitively expensive.

   Except for type 3, where circles turn into ellipses, the CTM can get
   folded into the parameter => user space mapping, since in all other
   cases, the mapping space is closed under linear transformations of
   the output.
 */

/* Define types and rendering procedures for the individual shadings. */
typedef struct gs_shading_Fb_s {
    gs_shading_head_t head;
    gs_shading_Fb_params_t params;
} gs_shading_Fb_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_Fb_fill_rectangle);

typedef struct gs_shading_A_s {
    gs_shading_head_t head;
    gs_shading_A_params_t params;
} gs_shading_A_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_A_fill_rectangle);

typedef struct gs_shading_R_s {
    gs_shading_head_t head;
    gs_shading_R_params_t params;
} gs_shading_R_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_R_fill_rectangle);

typedef struct gs_shading_FfGt_s {
    gs_shading_head_t head;
    gs_shading_FfGt_params_t params;
} gs_shading_FfGt_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_FfGt_fill_rectangle);

typedef struct gs_shading_LfGt_s {
    gs_shading_head_t head;
    gs_shading_LfGt_params_t params;
} gs_shading_LfGt_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_LfGt_fill_rectangle);

typedef struct gs_shading_Cp_s {
    gs_shading_head_t head;
    gs_shading_Cp_params_t params;
} gs_shading_Cp_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_Cp_fill_rectangle);

typedef struct gs_shading_Tpp_s {
    gs_shading_head_t head;
    gs_shading_Tpp_params_t params;
} gs_shading_Tpp_t;
SHADING_FILL_RECTANGLE_PROC(gs_shading_Tpp_fill_rectangle);

/* Define a stream for decoding packed coordinate values. */
typedef struct shade_coord_stream_s shade_coord_stream_t;
struct shade_coord_stream_s {
    stream ds;			/* stream if DataSource isn't one already -- */
				/* first for GC-ability (maybe unneeded?) */
    stream *s;			/* DataSource or &ds */
    uint bits;			/* shifted bits of current byte */
    int left;			/* # of bits left in bits */
    bool ds_EOF;                /* The 'ds' stream reached EOF. */
    const gs_shading_mesh_params_t *params;
    const gs_matrix_fixed *pctm;
    int (*get_value)(shade_coord_stream_t *cs, int num_bits, uint *pvalue);
    int (*get_decoded)(shade_coord_stream_t *cs, int num_bits,
		       const float decode[2], float *pvalue);
    bool (*is_eod)(const shade_coord_stream_t *cs);
};

/* Define one vertex of a mesh. */
typedef struct mesh_vertex_s {
    gs_fixed_point p;
    float cc[GS_CLIENT_COLOR_MAX_COMPONENTS];
} mesh_vertex_t;

/* Define a structure for mesh or patch vertex. */
typedef struct shading_vertex_s shading_vertex_t;

/* Initialize a packed value stream. */
void shade_next_init(shade_coord_stream_t * cs,
		     const gs_shading_mesh_params_t * params,
		     const gs_imager_state * pis);

/* Get the next flag value. */
int shade_next_flag(shade_coord_stream_t * cs, int BitsPerFlag);

/* Get one or more coordinate pairs. */
int shade_next_coords(shade_coord_stream_t * cs, gs_fixed_point * ppt,
		      int num_points);

/* Get a color.  Currently all this does is look up Indexed colors. */
int shade_next_color(shade_coord_stream_t * cs, float *pc);

/* Get the next vertex for a mesh element. */
int shade_next_vertex(shade_coord_stream_t * cs, shading_vertex_t * vertex);

/*
   Currently, all shading fill procedures follow the same algorithm:

   - Conservatively inverse-transform the rectangle being filled to a linear
   or rectangular range of values in the parameter space.

   - Compute the color values at the extrema of the range.

   - If possible, compute the parameter range corresponding to a single
   device pixel.

   - Recursively do the following, passing the parameter range and extremal
   color values as the recursion arguments:

   - If the color values are equal to within the tolerance given by the
   smoothness in the graphics state, or if the range of parameters maps
   to a single device pixel, fill the range with the (0) or (0,0) color.

   - Otherwise, subdivide and recurse.  If the parameter range is 2-D,
   subdivide the axis with the largest color difference.

   For shadings based on a function, if the function is not monotonic, the
   smoothness test must only be applied when the parameter range extrema are
   all interpolated from the same entries in the Function.  (We don't
   currently do this.)

 */

/*
 * Define the common structure for recursive subdivision.
 *
 * direct_space is the same as the original ColorSpace unless the
 * original space is an Indexed space, in which case direct_space is the
 * base space of the original space.  This is the space in which color
 * computations are done.
 */
#define shading_fill_state_common\
  gx_device *dev;\
  gs_imager_state *pis;\
  const gs_color_space *direct_space;\
  int num_components;		/* # of color components in direct_space */\
  float cc_max_error[GS_CLIENT_COLOR_MAX_COMPONENTS]
typedef struct shading_fill_state_s {
    shading_fill_state_common;
} shading_fill_state_t;

/* Initialize the common parts of the recursion state. */
void shade_init_fill_state(shading_fill_state_t * pfs,
			   const gs_shading_t * psh, gx_device * dev,
			   gs_imager_state * pis);

/* Fill one piece of a shading. */
#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif
int shade_fill_path(const shading_fill_state_t * pfs, gx_path * ppath,
		    gx_device_color * pdevc, const gs_fixed_point *fill_adjust);

#endif /* gxshade_INCLUDED */
