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

/* gzstate.h */
/* Private graphics state definition for Ghostscript library */
#include "gxdcolor.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gsstate.h"
#include "gxdevice.h"
#include "gxtmap.h"

/* Opaque types referenced by the graphics state. */
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif
#ifndef gs_halftone_DEFINED
#  define gs_halftone_DEFINED
typedef struct gs_halftone_s gs_halftone;
#endif
#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

/* Composite components of the graphics state. */
typedef struct gx_transfer_colored_s {
	/* The components must be in this order: */
	gx_transfer_map *red;
	gx_transfer_map *green;
	gx_transfer_map *blue;
	gx_transfer_map *gray;
} gx_transfer_colored;
typedef union gx_transfer_s {
	gx_transfer_map *indexed[4];
	gx_transfer_colored colored;
} gx_transfer;

/*
 * We allocate a large subset of the graphics state as a single object.
 * Its type is opaque here, defined in gsstate.c.
 * The components marked with @ must be allocated in the contents.
 * (Consult gsstate.c for more details about gstate storage management.)
 */
typedef struct gs_state_contents_s gs_state_contents;

/* Graphics state structure. */

struct gs_state_s {
	gs_state *saved;		/* previous state from gsave */
	gs_memory_t *memory;
	gs_state_contents *contents;

		/* Transformation: */

	gs_matrix_fixed ctm;
#define ctm_only(pgs) *(gs_matrix *)&(pgs)->ctm
	gs_matrix ctm_inverse;
	bool inverse_valid;		/* true if ctm_inverse = ctm^-1 */

		/* Paths: */

	struct gx_path_s *path;
	struct gx_clip_path_s *clip_path;	/* @ */
	int clip_rule;

		/* Lines: */

	struct gx_line_params_s *line_params;

		/* Halftone screen: */

	gs_halftone *halftone;
	gx_device_halftone *dev_ht;
	gs_int_point ht_phase;
	struct gx_ht_cache_s *ht_cache;	/* shared by all GCs */

		/* Color (device-independent): */

	struct gs_color_space_s *color_space;
	struct gs_client_color_s *ccolor;
	gx_color_value alpha;

		/* Color (device-dependent): */

	struct gs_cie_render_s *cie_render;
	bool overprint;
	gx_transfer_map *black_generation;	/* may be 0 */
	gx_transfer_map *undercolor_removal;	/* may be 0 */
		/* set_transfer holds the transfer functions specified by */
		/* set[color]transfer; effective_transfer includes the */
		/* effects of overrides by TransferFunctions in halftone */
		/* dictionaries.  (In Level 1 systems, set_transfer and */
		/* effective_transfer are always the same.) */
	gx_transfer set_transfer;
	gx_transfer effective_transfer;

		/* Color caches: */

	gx_device_color *dev_color;
	struct gx_cie_joint_caches_s *cie_joint_caches;
	const struct gx_color_map_procs_s *cmap_procs;
	struct gx_pattern_cache_s *pattern_cache; /* shared by all GCs */

		/* Font: */

	gs_font *font;
	gs_font *root_font;
	gs_matrix_fixed char_tm;	/* font matrix * ctm */
#define char_tm_only(pgs) *(gs_matrix *)&(pgs)->char_tm
	bool char_tm_valid;		/* true if char_tm is valid */
	byte in_cachedevice;		/* 0 if not in setcachedevice, */
					/* 1 if in setcachdevice but not */
					/* actually caching, */
					/* 2 if in setcachdevice and */
					/* actually caching */
	byte in_charpath;		/* 0 if not in charpath, */
					/* 1 if false charpath, */
					/* 2 if true charpath */
					/* (see charpath_flag in */
					/* gs_show_enum_s) */
	gs_state *show_gstate;		/* gstate when show was invoked */
					/* (so charpath can append to path) */

		/* Other stuff: */

	int level;			/* incremented by 1 per gsave */
	float flatness;
	fixed fill_adjust;		/* fattening for fill */
	bool stroke_adjust;
	gx_device *device;
#undef gs_currentdevice_inline
#define gs_currentdevice_inline(pgs) ((pgs)->device)

		/* Client data: */

	void *client_data;
#undef gs_state_client_data_inline
#define gs_state_client_data_inline(pgs) ((pgs)->client_data)
	gs_state_client_procs client_procs;
};
#define private_st_gs_state()	/* in gsstate.c */\
  gs_private_st_composite(st_gs_state, gs_state, "gs_state",\
    gs_state_enum_ptrs, gs_state_reloc_ptrs)
