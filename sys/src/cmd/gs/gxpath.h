/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxpath.h */
/* Lower-level path routines for Ghostscript library */
/* Requires gxfixed.h */
#include "gspath.h"

/* The routines and types in this interface use */
/* device, rather than user, coordinates, and fixed-point, */
/* rather than floating, representation. */

/* Opaque type for a path */
typedef struct gx_path_s gx_path;

/* Define the two insideness rules */
#define gx_rule_winding_number (-1)
#define gx_rule_even_odd 1

/* Debugging routines */
#ifdef DEBUG
void gx_dump_path(P2(const gx_path *, const char *));
void gx_path_print(P1(const gx_path *));
#endif

/* Path constructors. */
void	gx_path_init(P2(gx_path *, gs_memory_t *)),
	gx_path_reset(P1(gx_path *)),
	gx_path_release(P1(gx_path *)),
	gx_path_share(P1(gx_path *));
int	gx_path_add_point(P3(gx_path *, fixed, fixed)),
	gx_path_add_relative_point(P3(gx_path *, fixed, fixed)),
	gx_path_add_line(P3(gx_path *, fixed, fixed)),
	gx_path_add_lines(P3(gx_path *, const gs_fixed_point *, int)),
	gx_path_add_rectangle(P5(gx_path *, fixed, fixed, fixed, fixed)),
	gx_path_add_curve(P7(gx_path *, fixed, fixed, fixed, fixed, fixed, fixed)),
	gx_path_add_flattened_curve(P8(gx_path *, fixed, fixed, fixed, fixed, fixed, fixed, floatp)),
	gx_path_add_arc(P8(gx_path *, fixed, fixed, fixed, fixed, fixed, fixed, floatp)),
	gx_path_add_path(P2(gx_path *, gx_path *)),
	gx_path_close_subpath(P1(gx_path *)),
	gx_path_pop_close_subpath(P1(gx_path *));
/* The last argument to gx_path_add_arc is a fraction for computing */
/* the curve parameters.  Here is the correct value for quarter-circles. */
/* (stroke uses this to draw round caps and joins.) */
#define quarter_arc_fraction 0.552285

/* Path accessors */

int	gx_path_current_point(P2(const gx_path *, gs_fixed_point *)),
	gx_path_bbox(P2(gx_path *, gs_fixed_rect *)),
	gx_path_has_curves(P1(const gx_path *)),
	gx_path_is_void(P1(const gx_path *)),
	gx_path_is_rectangle(P2(const gx_path *, gs_fixed_rect *));
/* Inline versions of the above */
#define gx_path_is_void_inline(ppath) ((ppath)->first_subpath == 0)
#define gx_path_has_curves_inline(ppath) ((ppath)->curve_count != 0)

/* Path transformers */

int	gx_path_copy(P3(const gx_path *	/*old*/, gx_path * /*new*/, bool /*init*/)),
	gx_path_flatten(P4(const gx_path * /*old*/, gx_path * /*new*/, floatp, bool /*in_BuildCharGlyph*/)),
	gx_path_expand_dashes(P3(const gx_path * /*old*/, gx_path * /*new*/, gs_state *)),
	gx_path_copy_reversed(P3(const gx_path * /*old*/, gx_path * /*new*/, bool /*init*/)),
	gx_path_translate(P3(gx_path *, fixed, fixed)),
	gx_path_scale_exp2(P3(gx_path *, int, int));
void	gx_point_scale_exp2(P3(gs_fixed_point *, int, int)),
	gx_rect_scale_exp2(P3(gs_fixed_rect *, int, int));

/* ------ Clipping paths ------ */

/* Opaque type for a path */
typedef struct gx_clip_path_s gx_clip_path;

int	gx_clip_to_rectangle(P2(gs_state *, gs_fixed_rect *)),
	gx_clip_to_path(P1(gs_state *)),
	gx_cpath_init(P2(gx_clip_path *, gs_memory_t *)),
	gx_cpath_from_rectangle(P3(gx_clip_path *, gs_fixed_rect *, gs_memory_t *)),
	gx_cpath_intersect(P4(gs_state *, gx_clip_path *, gx_path *, int)),
	gx_cpath_scale_exp2(P3(gx_clip_path *, int, int));
void	gx_cpath_release(P1(gx_clip_path *)),
	gx_cpath_share(P1(gx_clip_path *));
int	gx_cpath_path(P2(gx_clip_path *, gx_path *)),
	gx_cpath_inner_box(P2(const gx_clip_path *, gs_fixed_rect *)),
	gx_cpath_outer_box(P2(const gx_clip_path *, gs_fixed_rect *)),
	gx_cpath_includes_rectangle(P5(const gx_clip_path *, fixed, fixed, fixed, fixed));

/* Opaque type for a clip list. */
typedef struct gx_clip_list_s gx_clip_list;
