/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxshade4.h,v 1.41 2005/04/19 12:22:08 igor Exp $ */
/* Internal definitions for triangle shading rendering */

#ifndef gxshade4_INCLUDED
#  define gxshade4_INCLUDED

/* Configuration flags for development needs only. Users should not modify them. */
#define USE_LINEAR_COLOR_PROCS 1 /* Old code = 0, new code = 1. */

#define QUADRANGLES 0 /* 0 = decompose by triangles, 1 = by quadrangles. */
/* The code QUADRANGLES 1 appears unuseful.
   We keep it because it stores a valuable code for constant_color_quadrangle,
   which decomposes a random quadrangle into 3 or 4 trapezoids.
   The color approximation looks worse than with triangles, and works some slower.
 */
#define INTERPATCH_PADDING (fixed_1 / 2) /* Emulate a trapping for poorly designed documents. */
/* When INTERPATCH_PADDING > 0, it generates paddings between patches,
   i.e. performs a patch expansion, being similar
   to the path adjustment in the filling algorithm.
   The expansion is an emulation of Adobe's trapping.
   The value specifies the width of paddings.
   We did some testing of Adobe RIP, and it looks as applying 
   same logicks as for clipping - any part of pixel inside.
   Therefore the expansion should be half pixel size.
 */
#define COLOR_CONTIGUITY 1 /* A smothness divisor for triangulation. */
/* This is a coefficient used to rich
   a better color contiguity. The value 1 corresponds to PLRM,
   bigger values mean more contiguity. The speed decreases as
   a square of COLOR_CONTIGUITY.
 */
#define LAZY_WEDGES 1 /* 0 = fill immediately, 1 = fill lazy. */
/* This mode delays creating wedges for a boundary until
   both neoghbour areas are painted. At that moment we can know
   all subdivision points for both right and left areas,
   and skip wedges for common points. Therefore the number of wadges 
   dramatically reduces, causing a significant speedup.
   The LAZY_WEDGES 0 mode was not systematically tested.
 */
#define VD_TRACE_DOWN 1 /* Developer's needs, not important for production. */
#define NOFILL_TEST 0 /* Developer's needs, must be off for production. */
#define SKIP_TEST 0 /* Developer's needs, must be off for production. */
/* End of configuration flags (we don't mean that users should modify the rest). */

#define mesh_max_depth (16 * 3 + 1)	/* each recursion adds 3 entries */
typedef struct mesh_frame_s {	/* recursion frame */
    mesh_vertex_t va, vb, vc;	/* current vertices */
    bool check_clipping;
} mesh_frame_t;
/****** NEED GC DESCRIPTOR ******/

/*
 * Define the fill state structure for triangle shadings.  This is used
 * both for the Gouraud triangle shading types and for the Coons and
 * tensor patch types.
 *
 * The shading pointer is named pshm rather than psh in case subclasses
 * also want to store a pointer of a more specific type.
 */
#define mesh_fill_state_common\
  shading_fill_state_common;\
  const gs_shading_mesh_t *pshm;\
  gs_fixed_rect rect;\
  int depth;\
  mesh_frame_t frames[mesh_max_depth]
typedef struct mesh_fill_state_s {
    mesh_fill_state_common;
} mesh_fill_state_t;
/****** NEED GC DESCRIPTOR ******/

typedef struct wedge_vertex_list_elem_s wedge_vertex_list_elem_t;
struct wedge_vertex_list_elem_s {
    gs_fixed_point p;
    int level;
    bool divide_count;
    wedge_vertex_list_elem_t *next, *prev;
};
typedef struct {
    bool last_side;
    wedge_vertex_list_elem_t *beg, *end;    
} wedge_vertex_list_t;

#define LAZY_WEDGES_MAX_LEVEL 9 /* memory consumption is 
    sizeof(wedge_vertex_list_elem_t) * LAZY_WEDGES_MAX_LEVEL * (1 << LAZY_WEDGES_MAX_LEVEL) */

/* Define the common state for rendering Coons and tensor patches. */
typedef struct patch_fill_state_s {
    mesh_fill_state_common;
    const gs_function_t *Function;
    bool vectorization;
    int n_color_args;
    fixed max_small_coord; /* Length restriction for intersection_of_small_bars. */
    wedge_vertex_list_elem_t *wedge_vertex_list_elem_buffer;
    wedge_vertex_list_elem_t *free_wedge_vertex;
    int wedge_vertex_list_elem_count;
    int wedge_vertex_list_elem_count_max;
    gs_client_color color_domain;
    fixed fixed_flat;
    double smoothness;
    bool maybe_self_intersecting;
    bool monotonic_color;
    bool linear_color;
    bool unlinear;
    bool inside;
} patch_fill_state_t;
/* Define a color to be used in curve rendering. */
/* This may be a real client color, or a parametric function argument. */
typedef struct patch_color_s {
    float t[2];			/* parametric value */
    gs_client_color cc;
} patch_color_t;

/* Define a structure for mesh or patch vertex. */
struct shading_vertex_s {
    gs_fixed_point p;
    patch_color_t c;
};

/* Define one segment (vertex and next control points) of a curve. */
typedef struct patch_curve_s {
    mesh_vertex_t vertex;
    gs_fixed_point control[2];
    bool straight;
} patch_curve_t;

/* Initialize the fill state for triangle shading. */
int mesh_init_fill_state(mesh_fill_state_t * pfs,
			  const gs_shading_mesh_t * psh,
			  const gs_fixed_rect * rect_clip,
			  gx_device * dev, gs_imager_state * pis);

int init_patch_fill_state(patch_fill_state_t *pfs);
void term_patch_fill_state(patch_fill_state_t *pfs);

int mesh_triangle(patch_fill_state_t *pfs, 
    const shading_vertex_t *p0, const shading_vertex_t *p1, const shading_vertex_t *p2);

int mesh_padding(patch_fill_state_t *pfs, const gs_fixed_point *p0, const gs_fixed_point *p1, 
	    const patch_color_t *c0, const patch_color_t *c1);

int patch_fill(patch_fill_state_t * pfs, const patch_curve_t curve[4],
	   const gs_fixed_point interior[4],
	   void (*transform) (gs_fixed_point *, const patch_curve_t[4],
			      const gs_fixed_point[4], floatp, floatp));

int wedge_vertex_list_elem_buffer_alloc(patch_fill_state_t *pfs);
void wedge_vertex_list_elem_buffer_free(patch_fill_state_t *pfs);

void patch_resolve_color(patch_color_t * ppcr, const patch_fill_state_t *pfs);

int gx_shade_background(gx_device *pdev, const gs_fixed_rect *rect, 
	const gx_device_color *pdevc, gs_logical_operation_t log_op);

#endif /* gxshade4_INCLUDED */
