/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxcspace.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Implementation of color spaces */
/* Requires gsstruct.h */

#ifndef gxcspace_INCLUDED
#  define gxcspace_INCLUDED

#include "gscspace.h"		/* client interface */
#include "gsccolor.h"
#include "gscsel.h"
#include "gxfrac.h"		/* for concrete colors */

/* Define opaque types. */

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/* Color space types (classes): */
/*typedef struct gs_color_space_type_s gs_color_space_type; */
struct gs_color_space_type_s {

    gs_color_space_index index;

    /*
     * Define whether the space can be the base space for an Indexed
     * color space or the alternate space for a Separation or DeviceN
     * color space.
     */

    bool can_be_base_space;
    bool can_be_alt_space;

    /*
     * Define the true structure type for this variant of the color
     * space union.
     */

    gs_memory_type_ptr_t stype;

    /* ------ Procedures ------ */

    /*
     * Define the number of components in a color of this space.  For
     * Pattern spaces, where the number of components depends on the
     * underlying space, this value is -1 for colored Patterns,
     * -N-1 for uncolored Patterns, where N is the number of components
     * in the base space.
     */

#define cs_proc_num_components(proc)\
  int proc(P1(const gs_color_space *))
#define cs_num_components(pcs)\
  (*(pcs)->type->num_components)(pcs)
                         cs_proc_num_components((*num_components));

    /*
     * Return the base or alternate color space underlying this one.
     * Only defined for Indexed, Separation, DeviceN, and
     * uncolored Pattern spaces; returns NULL for all others.
     */

#define cs_proc_base_space(proc)\
  const gs_color_space *proc(P1(const gs_color_space *))
#define cs_base_space(pcs)\
  (*(pcs)->type->base_space)(pcs)
                         cs_proc_base_space((*base_space));

    /* Construct the initial color value for this space. */

#define cs_proc_init_color(proc)\
  void proc(P2(gs_client_color *, const gs_color_space *))
#define cs_init_color(pcc, pcs)\
  (*(pcs)->type->init_color)(pcc, pcs)
#define cs_full_init_color(pcc, pcs)\
  ((pcc)->pattern = 0, cs_init_color(pcc, pcs))
                         cs_proc_init_color((*init_color));

    /* Force a client color into its legal range. */

#define cs_proc_restrict_color(proc)\
  void proc(P2(gs_client_color *, const gs_color_space *))
                         cs_proc_restrict_color((*restrict_color));

    /* Return the concrete color space underlying this one. */
    /* (Not defined for Pattern spaces.) */

#define cs_proc_concrete_space(proc)\
  const gs_color_space *proc(P2(const gs_color_space *,\
				const gs_imager_state *))
#define cs_concrete_space(pcs, pis)\
  (*(pcs)->type->concrete_space)(pcs, pis)
                         cs_proc_concrete_space((*concrete_space));

    /*
     * Reduce a color to a concrete color.  A concrete color is one
     * that the device can handle directly (possibly with halftoning):
     * a DeviceGray/RGB/CMYK/Pixel color, or a Separation or DeviceN
     * color that does not use the alternate space.
     * (Not defined for Pattern spaces.)
     */

#define cs_proc_concretize_color(proc)\
  int proc(P4(const gs_client_color *, const gs_color_space *,\
    frac *, const gs_imager_state *))
#define cs_concretize_color(pcc, pcs, values, pis)\
  (*(pcs)->type->concretize_color)(pcc, pcs, values, pis)
                         cs_proc_concretize_color((*concretize_color));

    /* Map a concrete color to a device color. */
    /* (Only defined for concrete color spaces.) */

#define cs_proc_remap_concrete_color(proc)\
  int proc(P5(const frac *, gx_device_color *, const gs_imager_state *,\
    gx_device *, gs_color_select_t))
                         cs_proc_remap_concrete_color((*remap_concrete_color));

    /* Map a color directly to a device color. */

#define cs_proc_remap_color(proc)\
  int proc(P6(const gs_client_color *, const gs_color_space *,\
    gx_device_color *, const gs_imager_state *, gx_device *,\
    gs_color_select_t))
                         cs_proc_remap_color((*remap_color));

    /* Install the color space in a graphics state. */

#define cs_proc_install_cspace(proc)\
  int proc(P2(const gs_color_space *, gs_state *))
                         cs_proc_install_cspace((*install_cspace));

    /* Adjust reference counts of indirect color space components. */

#define cs_proc_adjust_cspace_count(proc)\
  void proc(P2(const gs_color_space *, int))
#define cs_adjust_cspace_count(pgs, delta)\
  (*(pgs)->color_space->type->adjust_cspace_count)((pgs)->color_space, delta)
	 cs_proc_adjust_cspace_count((*adjust_cspace_count));

    /* Adjust reference counts of indirect color components. */
    /*
     * Note: the color space argument may be NULL, which indicates that the
     * caller warrants that any subsidiary colors don't have allocation
     * issues.  This is a hack for an application that needs to be able to
     * release Pattern colors.
     */

#define cs_proc_adjust_color_count(proc)\
  void proc(P3(const gs_client_color *, const gs_color_space *, int))
#define cs_adjust_color_count(pgs, delta)\
  (*(pgs)->color_space->type->adjust_color_count)\
    ((pgs)->ccolor, (pgs)->color_space, delta)
	 cs_proc_adjust_color_count((*adjust_color_count));

/* Adjust both reference counts. */
#define cs_adjust_counts(pgs, delta)\
  (cs_adjust_color_count(pgs, delta), cs_adjust_cspace_count(pgs, delta))

};

/* Standard color space structure types */
extern_st(st_base_color_space);
#define public_st_base_color_space()	/* in gscspace.c */\
  gs_public_st_simple(st_base_color_space, gs_base_color_space,\
    "gs_base_color_space")
/*extern_st(st_paint_color_space); *//* (not needed) */

/* Standard color space procedures */
cs_proc_num_components(gx_num_components_1);
cs_proc_num_components(gx_num_components_3);
cs_proc_num_components(gx_num_components_4);
cs_proc_base_space(gx_no_base_space);
cs_proc_init_color(gx_init_paint_1);
cs_proc_init_color(gx_init_paint_3);
cs_proc_init_color(gx_init_paint_4);
cs_proc_restrict_color(gx_restrict01_paint_1);
cs_proc_restrict_color(gx_restrict01_paint_3);
cs_proc_restrict_color(gx_restrict01_paint_4);
cs_proc_concrete_space(gx_no_concrete_space);
cs_proc_concrete_space(gx_same_concrete_space);
cs_proc_concretize_color(gx_no_concretize_color);
cs_proc_remap_color(gx_default_remap_color);
cs_proc_install_cspace(gx_no_install_cspace);
cs_proc_adjust_cspace_count(gx_no_adjust_cspace_count);
cs_proc_adjust_color_count(gx_no_adjust_color_count);

/*
 * Define the implementation procedures for the standard device color
 * spaces.  These are implemented in gxcmap.c.
 */
cs_proc_remap_color(gx_remap_DeviceGray);
cs_proc_concretize_color(gx_concretize_DeviceGray);
cs_proc_remap_concrete_color(gx_remap_concrete_DGray);
cs_proc_remap_color(gx_remap_DeviceRGB);
cs_proc_concretize_color(gx_concretize_DeviceRGB);
cs_proc_remap_concrete_color(gx_remap_concrete_DRGB);
cs_proc_remap_color(gx_remap_DeviceCMYK);
cs_proc_concretize_color(gx_concretize_DeviceCMYK);
cs_proc_remap_concrete_color(gx_remap_concrete_DCMYK);

/* Define the allocator type for color spaces. */
extern_st(st_color_space);

/*
 * Initialize the type and memory fields of a color space, possibly
 * allocating it first.  This is only used by color space implementations.
 */
void gs_cspace_init(P3(gs_color_space *pcs,
		       const gs_color_space_type *pcstype,
		       gs_memory_t *mem));
int gs_cspace_alloc(P3(gs_color_space **ppcspace,
		       const gs_color_space_type *pcstype,
		       gs_memory_t *mem));

#endif /* gxcspace_INCLUDED */
