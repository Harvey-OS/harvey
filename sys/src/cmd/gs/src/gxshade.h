/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxshade.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
shading_fill_rectangle_proc(gs_shading_Fb_fill_rectangle);

typedef struct gs_shading_A_s {
    gs_shading_head_t head;
    gs_shading_A_params_t params;
} gs_shading_A_t;
shading_fill_rectangle_proc(gs_shading_A_fill_rectangle);

typedef struct gs_shading_R_s {
    gs_shading_head_t head;
    gs_shading_R_params_t params;
} gs_shading_R_t;
shading_fill_rectangle_proc(gs_shading_R_fill_rectangle);

typedef struct gs_shading_FfGt_s {
    gs_shading_head_t head;
    gs_shading_FfGt_params_t params;
} gs_shading_FfGt_t;
shading_fill_rectangle_proc(gs_shading_FfGt_fill_rectangle);

typedef struct gs_shading_LfGt_s {
    gs_shading_head_t head;
    gs_shading_LfGt_params_t params;
} gs_shading_LfGt_t;
shading_fill_rectangle_proc(gs_shading_LfGt_fill_rectangle);

typedef struct gs_shading_Cp_s {
    gs_shading_head_t head;
    gs_shading_Cp_params_t params;
} gs_shading_Cp_t;
shading_fill_rectangle_proc(gs_shading_Cp_fill_rectangle);

typedef struct gs_shading_Tpp_s {
    gs_shading_head_t head;
    gs_shading_Tpp_params_t params;
} gs_shading_Tpp_t;
shading_fill_rectangle_proc(gs_shading_Tpp_fill_rectangle);

/* Define a stream for decoding packed coordinate values. */
typedef struct shade_coord_stream_s shade_coord_stream_t;
struct shade_coord_stream_s {
    stream ds;			/* stream if DataSource isn't one already -- */
				/* first for GC-ability (maybe unneeded?) */
    stream *s;			/* DataSource or &ds */
    uint bits;			/* shifted bits of current byte */
    int left;			/* # of bits left in bits */
    const gs_shading_mesh_params_t *params;
    const gs_matrix_fixed *pctm;
    int (*get_value)(P3(shade_coord_stream_t *cs, int num_bits, uint *pvalue));
    int (*get_decoded)(P4(shade_coord_stream_t *cs, int num_bits,
			  const float decode[2], float *pvalue));
};

/* Define one vertex of a mesh. */
typedef struct mesh_vertex_s {
    gs_fixed_point p;
    float cc[GS_CLIENT_COLOR_MAX_COMPONENTS];
} mesh_vertex_t;

/* Initialize a packed value stream. */
void shade_next_init(P3(shade_coord_stream_t * cs,
			const gs_shading_mesh_params_t * params,
			const gs_imager_state * pis));

/* Get the next flag value. */
int shade_next_flag(P2(shade_coord_stream_t * cs, int BitsPerFlag));

/* Get one or more coordinate pairs. */
int shade_next_coords(P3(shade_coord_stream_t * cs, gs_fixed_point * ppt,
			 int num_points));

/* Get a color.  Currently all this does is look up Indexed colors. */
int shade_next_color(P2(shade_coord_stream_t * cs, float *pc));

/* Get the next vertex for a mesh element. */
int shade_next_vertex(P2(shade_coord_stream_t * cs, mesh_vertex_t * vertex));

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

/* Define the common structure for recursive subdivision. */
#define shading_fill_state_common\
  gx_device *dev;\
  gs_imager_state *pis;\
  int num_components;		/* # of color components */\
  float cc_max_error[GS_CLIENT_COLOR_MAX_COMPONENTS]
typedef struct shading_fill_state_s {
    shading_fill_state_common;
} shading_fill_state_t;

/* Initialize the common parts of the recursion state. */
void shade_init_fill_state(P4(shading_fill_state_t * pfs,
			      const gs_shading_t * psh, gx_device * dev,
			      gs_imager_state * pis));

/* Transform a bounding box into device space. */
int shade_bbox_transform2fixed(P3(const gs_rect * rect,
				  const gs_imager_state * pis,
				  gs_fixed_rect * rfixed));

/* Check whether 4 colors fall within the smoothness criterion. */
bool shade_colors4_converge(P2(const gs_client_color cc[4],
			       const shading_fill_state_t * pfs));

/* Fill one piece of a shading. */
#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif
int shade_fill_path(P3(const shading_fill_state_t * pfs, gx_path * ppath,
		       gx_device_color * pdevc));

#endif /* gxshade_INCLUDED */
