/* Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gscspace.h */
/* Client interface to color spaces */

#ifndef gscspace_INCLUDED
#  define gscspace_INCLUDED

#include "gsccolor.h"
#include "gsstruct.h"		/* needed for enum_ptrs & reloc_ptrs */
#include "gxfrac.h"		/* for concrete colors */

/* Color space type indices */
typedef enum {
		/* Supported in all configurations */
	gs_color_space_index_DeviceGray = 0,
	gs_color_space_index_DeviceRGB,
		/* Supported in extended Level 1, and in Level 2 */
	gs_color_space_index_DeviceCMYK,
		/* Supported in Level 2 only */
	gs_color_space_index_CIEBasedABC,
	gs_color_space_index_CIEBasedA,
	gs_color_space_index_Separation,
	gs_color_space_index_Indexed,
	gs_color_space_index_Pattern
} gs_color_space_index;

/*
 * Color spaces are complicated because different spaces involve
 * different kinds of parameters, as follows:

Space		Space parameters		Color parameters
-----		----------------		----------------
DeviceGray	(none)				1 real [0-1]
DeviceRGB	(none)				3 reals [0-1]
DeviceCMYK	(none)				4 reals [0-1]
CIEBasedABC	dictionary			3 reals
CIEBasedA	dictionary			1 real
Separation	name, alt_space, tint_xform	1 real [0-1]
Indexed		base_space, hival, lookup	1 int [0-hival]
Pattern		colored: (none)			dictionary
		uncolored: base_space		dictionary + base space params

Space		Underlying or alternate space
-----		-----------------------------
Separation	Device, CIE
Indexed		Device, CIE
Pattern		Device, CIE, Separation, Indexed

 * Logically speaking, each color space type should be a different
 * structure type at the allocator level.  This would potentially require
 * either reference counting or garbage collector for color spaces, but that
 * is probably better than the current design, which uses fixed-size color
 * space objects and a second level of type discrimination.
 */

/* Color space type objects */
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif

/* Define an opaque type for device colors. */
/* (The actual definition is in gxdcolor.h.) */
#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

/* Color space types (classes): */
typedef struct gs_color_space_type_s {

	gs_color_space_index index;

	/* Define the number of components in a color of this space. */
	/* For Pattern spaces, where the number of components depends on */
	/* the underlying space, this value is -1. */

	int num_components;

	/* Define whether the space can be the base space for */
	/* an Indexed color space or the alternate space for */
	/* a Separation color space. */

	bool can_be_base_space;

	/* ------ Procedures ------ */

	/* Construct the initial color value for this space. */

#define cs_proc_init_color(proc)\
  void proc(P2(gs_client_color *, const gs_color_space *))
#define cs_init_color(pcc, pcs)\
  (*(pcs)->type->init_color)(pcc, pcs)
#define cs_full_init_color(pcc, pcs)\
  ((pcc)->pattern = 0, cs_init_color(pcc, pcs))
	cs_proc_init_color((*init_color));

	/* Return the concrete color space underlying this one. */
	/* (Not defined for Pattern spaces.) */

#define cs_proc_concrete_space(proc)\
  const gs_color_space *proc(P2(const gs_color_space *, const gs_state *))
#define cs_concrete_space(pcs, pgs)\
  (*(pcs)->type->concrete_space)(pcs, pgs)
	cs_proc_concrete_space((*concrete_space));

	/* Reduce a color to a concrete color. */
	/* (Not defined for Pattern spaces.) */

#define cs_proc_concretize_color(proc)\
  int proc(P4(const gs_client_color *, const gs_color_space *,\
    frac *, const gs_state *))
	cs_proc_concretize_color((*concretize_color));

	/* Map a concrete color to a device color. */
	/* (Only defined for concrete color spaces.) */

#define cs_proc_remap_concrete_color(proc)\
  int proc(P3(const frac *, gx_device_color *, const gs_state *))
	cs_proc_remap_concrete_color((*remap_concrete_color));

	/* Map a color directly to a device color. */

#define cs_proc_remap_color(proc)\
  int proc(P4(const gs_client_color *, const gs_color_space *,\
    gx_device_color *, const gs_state *))
	cs_proc_remap_color((*remap_color));

	/* Install the color space in a graphics state. */

#define cs_proc_install_cspace(proc)\
  int proc(P2(gs_color_space *, gs_state *))
	cs_proc_install_cspace((*install_cspace));

	/* Adjust reference counts of indirect color space components. */
#define cs_proc_adjust_cspace_count(proc)\
  void proc(P3(const gs_color_space *, gs_state *, int))
#define cs_adjust_cspace_count(pgs, delta)\
  (*(pgs)->color_space->type->adjust_cspace_count)((pgs)->color_space, pgs, delta)
	cs_proc_adjust_cspace_count((*adjust_cspace_count));

	/* Adjust reference counts of indirect color components. */

#define cs_proc_adjust_color_count(proc)\
  void proc(P4(const gs_client_color *, const gs_color_space *, gs_state *, int))
#define cs_adjust_color_count(pgs, delta)\
  (*(pgs)->color_space->type->adjust_color_count)((pgs)->ccolor, (pgs)->color_space, pgs, delta)
	cs_proc_adjust_color_count((*adjust_color_count));

/* Adjust both reference counts. */
#define cs_adjust_counts(pgs, delta)\
  cs_adjust_color_count(pgs, delta), cs_adjust_cspace_count(pgs, delta)

	/* Enumerate the pointers in a color space. */

	struct_proc_enum_ptrs((*enum_ptrs));

	/* Relocate the pointers in a color space. */

	struct_proc_reloc_ptrs((*reloc_ptrs));

} gs_color_space_type;
/* Standard color space procedures */
cs_proc_init_color(gx_init_paint_1);
cs_proc_init_color(gx_init_paint_3);
cs_proc_concrete_space(gx_no_concrete_space);
cs_proc_concrete_space(gx_same_concrete_space);
cs_proc_concretize_color(gx_no_concretize_color);
cs_proc_remap_color(gx_default_remap_color);
cs_proc_install_cspace(gx_no_install_cspace);
cs_proc_adjust_cspace_count(gx_no_adjust_cspace_count);
cs_proc_adjust_color_count(gx_no_adjust_color_count);
#define gx_no_cspace_enum_ptrs gs_no_struct_enum_ptrs
#define gx_no_cspace_reloc_ptrs gs_no_struct_reloc_ptrs

/* Macro for defining color space procedures. */
#define cs_declare_procs(scope, concretize, install, adjust, enum_p, reloc_p)\
  scope cs_proc_concretize_color(concretize);\
  scope cs_proc_install_cspace(install);\
  scope cs_proc_adjust_cspace_count(adjust);\
  scope struct_proc_enum_ptrs(enum_p);\
  scope struct_proc_reloc_ptrs(reloc_p)

/* Standard color space types */
extern const gs_color_space_type
	gs_color_space_type_DeviceGray,
	gs_color_space_type_DeviceRGB,
	gs_color_space_type_DeviceCMYK;

	/* Base color spaces (Device and CIE) */

typedef struct gs_cie_abc_s gs_cie_abc;
typedef struct gs_cie_a_s gs_cie_a;
#define gs_base_cspace_params\
	gs_cie_abc *abc;\
	gs_cie_a *a
typedef struct gs_base_color_space_s {
	const gs_color_space_type _ds *type;
	union {
		gs_base_cspace_params;
	} params;
} gs_base_color_space;

	/* Paint (non-pattern) color spaces (base + Separation + Indexed) */

typedef ulong gs_separation_name;		/* BOGUS */

typedef struct gs_indexed_map_s gs_indexed_map;
typedef struct gs_separation_params_s {
	gs_separation_name sname;
	gs_base_color_space alt_space;
	gs_indexed_map *map;
} gs_separation_params;
typedef struct gs_indexed_params_s {
	gs_base_color_space base_space;
	int hival;
	union {
		gs_const_string table;	/* size is implicit */
		gs_indexed_map *map;
	} lookup;
	bool use_proc;		/* 0 = use table, 1 = use proc & map */
} gs_indexed_params;
#define gs_paint_cspace_params\
	gs_base_cspace_params;\
	gs_separation_params separation;\
	gs_indexed_params indexed
typedef struct gs_paint_color_space_s {
	const gs_color_space_type _ds *type;
	union {
		gs_paint_cspace_params;
	} params;
} gs_paint_color_space;

	/* General color spaces (including patterns) */

typedef struct gs_pattern_params_s {
	bool has_base_space;
	gs_paint_color_space base_space;
} gs_pattern_params;
struct gs_color_space_s {
	const gs_color_space_type _ds *type;
	union {
		gs_paint_cspace_params;
		gs_pattern_params pattern;
	} params;
};
extern_st(st_color_space);
#define public_st_color_space()	/* in gscolor.c */\
  gs_public_st_composite(st_color_space, gs_color_space,\
    "gs_color_space", color_space_enum_ptrs, color_space_reloc_ptrs)
#define st_color_space_max_ptrs 2 /* 1 base + 1 indexed */

#endif					/* gscspace_INCLUDED */
