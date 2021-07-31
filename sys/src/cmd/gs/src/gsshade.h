/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gsshade.h,v 1.3 2000/09/19 19:00:32 lpd Exp $ */
/* Definitions for shading */

#ifndef gsshade_INCLUDED
#  define gsshade_INCLUDED

#include "gsccolor.h"
#include "gscspace.h"
#include "gsdsrc.h"
#include "gsfunc.h"
#include "gsmatrix.h"
#include "gxfixed.h"

/* ---------------- Types and structures ---------------- */

/* Define the shading types. */
typedef enum {
    shading_type_Function_based = 1,
    shading_type_Axial = 2,
    shading_type_Radial = 3,
    shading_type_Free_form_Gouraud_triangle = 4,
    shading_type_Lattice_form_Gouraud_triangle = 5,
    shading_type_Coons_patch = 6,
    shading_type_Tensor_product_patch = 7
} gs_shading_type_t;

/*
 * Define information common to all shading types.  We separate the private
 * part from the parameters so that clients can create parameter structures
 * without having to know the structure of the implementation.
 */
#define gs_shading_params_common\
  gs_color_space *ColorSpace;\
  gs_client_color *Background;\
  bool have_BBox;\
  gs_rect BBox;\
  bool AntiAlias

typedef struct gs_shading_params_s {
    gs_shading_params_common;
} gs_shading_params_t;

/* Define the type-specific procedures for shadings. */
#ifndef gs_shading_t_DEFINED
#  define gs_shading_t_DEFINED
typedef struct gs_shading_s gs_shading_t;
#endif
#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/*
 * Fill a user space rectangle.  This will paint every pixel that is in the
 * intersection of the rectangle and the shading's geometry, but it may
 * leave some pixels in the rectangle unpainted, and it may also paint
 * outside the rectangle: the caller is responsible for setting up a
 * clipping device if necessary.
 */
#define SHADING_FILL_RECTANGLE_PROC(proc)\
  int proc(P4(const gs_shading_t *psh, const gs_rect *prect, gx_device *dev,\
	      gs_imager_state *pis))
typedef SHADING_FILL_RECTANGLE_PROC((*shading_fill_rectangle_proc_t));
#define gs_shading_fill_rectangle(psh, prect, dev, pis)\
  ((psh)->head.procs.fill_rectangle(psh, prect, dev, pis))

/* Define the generic shading structures. */
typedef struct gs_shading_procs_s {
    shading_fill_rectangle_proc_t fill_rectangle;
} gs_shading_procs_t;
typedef struct gs_shading_head_s {
    gs_shading_type_t type;
    gs_shading_procs_t procs;
} gs_shading_head_t;

/* Define a generic shading, for use as the target type of pointers. */
struct gs_shading_s {
    gs_shading_head_t head;
    gs_shading_params_t params;
};
#define ShadingType(psh) ((psh)->head.type)
#define private_st_shading()	/* in gsshade.c */\
  gs_private_st_ptrs2(st_shading, gs_shading_t, "gs_shading_t",\
    shading_enum_ptrs, shading_reloc_ptrs,\
    params.ColorSpace, params.Background)

/* Define Function-based shading. */
typedef struct gs_shading_Fb_params_s {
    gs_shading_params_common;
    float Domain[4];
    gs_matrix Matrix;
    gs_function_t *Function;
} gs_shading_Fb_params_t;

#define private_st_shading_Fb()	/* in gsshade.c */\
  gs_private_st_suffix_add1(st_shading_Fb, gs_shading_Fb_t,\
    "gs_shading_Fb_t", shading_Fb_enum_ptrs, shading_Fb_reloc_ptrs,\
    st_shading, params.Function)

/* Define Axial shading. */
typedef struct gs_shading_A_params_s {
    gs_shading_params_common;
    float Coords[4];
    float Domain[2];
    gs_function_t *Function;
    bool Extend[2];
} gs_shading_A_params_t;

#define private_st_shading_A()	/* in gsshade.c */\
  gs_private_st_suffix_add1(st_shading_A, gs_shading_A_t,\
    "gs_shading_A_t", shading_A_enum_ptrs, shading_A_reloc_ptrs,\
    st_shading, params.Function)

/* Define Radial shading. */
typedef struct gs_shading_R_params_s {
    gs_shading_params_common;
    float Coords[6];
    float Domain[2];
    gs_function_t *Function;
    bool Extend[2];
} gs_shading_R_params_t;

#define private_st_shading_R()	/* in gsshade.c */\
  gs_private_st_suffix_add1(st_shading_R, gs_shading_R_t,\
    "gs_shading_R_t", shading_R_enum_ptrs, shading_R_reloc_ptrs,\
    st_shading, params.Function)

/* Define common parameters for mesh shading. */
#define gs_shading_mesh_params_common\
  gs_shading_params_common;\
  gs_data_source_t DataSource;\
  int BitsPerCoordinate;\
  int BitsPerComponent;\
  float *Decode;\
  gs_function_t *Function
/* The following are for internal use only. */
typedef struct gs_shading_mesh_params_s {
    gs_shading_mesh_params_common;
} gs_shading_mesh_params_t;
typedef struct gs_shading_mesh_s {
    gs_shading_head_t head;
    gs_shading_mesh_params_t params;
} gs_shading_mesh_t;

#define private_st_shading_mesh()	/* in gsshade.c */\
  gs_private_st_suffix_add2(st_shading_mesh, gs_shading_mesh_t,\
    "gs_shading_mesh_t", shading_mesh_enum_ptrs, shading_mesh_reloc_ptrs,\
    st_shading, params.Decode, params.Function)

/* Define Free-form Gouraud triangle mesh shading. */
typedef struct gs_shading_FfGt_params_s {
    gs_shading_mesh_params_common;
    int BitsPerFlag;
} gs_shading_FfGt_params_t;

#define private_st_shading_FfGt()	/* in gsshade.c */\
  gs_private_st_suffix_add0_local(st_shading_FfGt, gs_shading_FfGt_t,\
    "gs_shading_FfGt_t", shading_mesh_enum_ptrs, shading_mesh_reloc_ptrs,\
    st_shading_mesh)

/* Define Lattice-form Gouraud triangle mesh shading. */
typedef struct gs_shading_LfGt_params_s {
    gs_shading_mesh_params_common;
    int VerticesPerRow;
} gs_shading_LfGt_params_t;

#define private_st_shading_LfGt()	/* in gsshade.c */\
  gs_private_st_suffix_add0_local(st_shading_LfGt, gs_shading_LfGt_t,\
    "gs_shading_LfGt_t", shading_mesh_enum_ptrs, shading_mesh_reloc_ptrs,\
    st_shading_mesh)

/* Define Coons patch mesh shading. */
typedef struct gs_shading_Cp_params_s {
    gs_shading_mesh_params_common;
    int BitsPerFlag;
} gs_shading_Cp_params_t;

#define private_st_shading_Cp()	/* in gsshade.c */\
  gs_private_st_suffix_add0_local(st_shading_Cp, gs_shading_Cp_t,\
    "gs_shading_Cp_t", shading_mesh_enum_ptrs, shading_mesh_reloc_ptrs,\
    st_shading_mesh)

/* Define Tensor product patch mesh shading. */
typedef struct gs_shading_Tpp_params_s {
    gs_shading_mesh_params_common;
    int BitsPerFlag;
} gs_shading_Tpp_params_t;

#define private_st_shading_Tpp()	/* in gsshade.c */\
  gs_private_st_suffix_add0_local(st_shading_Tpp, gs_shading_Tpp_t,\
    "gs_shading_Tpp_t", shading_mesh_enum_ptrs, shading_mesh_reloc_ptrs,\
    st_shading_mesh)

/* ---------------- Procedures ---------------- */

/* Initialize shading parameters of specific types. */
void gs_shading_Fb_params_init(P1(gs_shading_Fb_params_t * params));
void gs_shading_A_params_init(P1(gs_shading_A_params_t * params));
void gs_shading_R_params_init(P1(gs_shading_R_params_t * params));
void gs_shading_FfGt_params_init(P1(gs_shading_FfGt_params_t * params));
void gs_shading_LfGt_params_init(P1(gs_shading_LfGt_params_t * params));
void gs_shading_Cp_params_init(P1(gs_shading_Cp_params_t * params));
void gs_shading_Tpp_params_init(P1(gs_shading_Tpp_params_t * params));

/* Create (initialize) shadings of specific types. */
int gs_shading_Fb_init(P3(gs_shading_t ** ppsh,
			  const gs_shading_Fb_params_t * params,
			  gs_memory_t * mem));
int gs_shading_A_init(P3(gs_shading_t ** ppsh,
			 const gs_shading_A_params_t * params,
			 gs_memory_t * mem));
int gs_shading_R_init(P3(gs_shading_t ** ppsh,
			 const gs_shading_R_params_t * params,
			 gs_memory_t * mem));
int gs_shading_FfGt_init(P3(gs_shading_t ** ppsh,
			    const gs_shading_FfGt_params_t * params,
			    gs_memory_t * mem));
int gs_shading_LfGt_init(P3(gs_shading_t ** ppsh,
			    const gs_shading_LfGt_params_t * params,
			    gs_memory_t * mem));
int gs_shading_Cp_init(P3(gs_shading_t ** ppsh,
			  const gs_shading_Cp_params_t * params,
			  gs_memory_t * mem));
int gs_shading_Tpp_init(P3(gs_shading_t ** ppsh,
			   const gs_shading_Tpp_params_t * params,
			   gs_memory_t * mem));

/*
 * Fill a path or a (device-space) rectangle with a shading.  Both the path
 * and the rectangle are optional, but at least one must be non-NULL; if
 * both are specified, the filled region is their intersection. This is the
 * only externally accessible procedure for rendering a shading.
 * fill_background indicates whether to fill portions of the path outside
 * the shading's geometry: it is true for filling with a pattern, false for
 * shfill.
 */
#ifndef gx_path_DEFINED
#  define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif
int gs_shading_fill_path(P6(const gs_shading_t *psh, /*const*/ gx_path *ppath,
			    const gs_fixed_rect *prect, gx_device *dev,
			    gs_imager_state *pis, bool fill_background));

#endif /* gsshade_INCLUDED */
