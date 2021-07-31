/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxistate.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Imager state definition */

#ifndef gxistate_INCLUDED
#  define gxistate_INCLUDED

#include "gscsel.h"
#include "gsrefct.h"
#include "gsropt.h"
#include "gxcvalue.h"
#include "gxcmap.h"
#include "gxfixed.h"
#include "gxline.h"
#include "gxmatrix.h"
#include "gxtmap.h"

/*
 * Define the subset of the PostScript graphics state that the imager
 * library API needs.  The definition of this subset is subject to change
 * as we come to understand better the boundary between the imager and
 * the interpreter.  In particular, the imager state currently INCLUDES
 * the following:
 *      line parameters: cap, join, miter limit, dash pattern
 *      transformation matrix (CTM)
 *      logical operation: RasterOp, transparency
 *      color modification: alpha, rendering algorithm
 *      overprint flag
 *      rendering tweaks: flatness, fill adjustment, stroke adjust flag,
 *        accurate curves flag, shading smoothness
 *      color rendering information:
 *          halftone, halftone phases
 *          transfer functions
 *          black generation, undercolor removal
 *          CIE rendering tables
 *          halftone and pattern caches
 *	shared (constant) device color spaces
 * The imager state currently EXCLUDES the following:
 *      graphics state stack
 *      default CTM
 *      path
 *      clipping path and stack
 *      color specification: color, color space, substitute color spaces
 *      font
 *      device
 *      caches for many of the above
 */

/*
 * Define the color rendering state information.
 * This should be a separate object (or at least a substructure),
 * but making this change would require editing too much code.
 */

/* Opaque types referenced by the color rendering state. */
#ifndef gs_halftone_DEFINED
#  define gs_halftone_DEFINED
typedef struct gs_halftone_s gs_halftone;
#endif
#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif
#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

/*
 * We need some special memory management for the components of a
 * c.r. state, as indicated by the following notations on the elements:
 *      (RC) means the element is reference-counted.
 *      (Shared) means the element is shared among an arbitrary number of
 *        c.r. states and is never freed.
 *      (Owned) means exactly one c.r. state references the element,
 *        and it is guaranteed that no references to it will outlive
 *        the c.r. state itself.
 */

/* Define the interior structure of a transfer function. */
typedef struct gx_transfer_colored_s {
    /* The components must be in this order: */
    gx_transfer_map *red;	/* (RC) */
    gx_transfer_map *green;	/* (RC) */
    gx_transfer_map *blue;	/* (RC) */
    gx_transfer_map *gray;	/* (RC) */
} gx_transfer_colored;
typedef union gx_transfer_s {
    gx_transfer_map *indexed[4];	/* (RC) */
    gx_transfer_colored colored;
} gx_transfer;

#define gs_color_rendering_state_common\
\
		/* Halftone screen: */\
\
	gs_halftone *halftone;			/* (RC) */\
	gs_int_point screen_phase[gs_color_select_count];\
		/* dev_ht depends on halftone and device resolution. */\
	gx_device_halftone *dev_ht;		/* (RC) */\
		/* The contents of ht_cache depend on dev_ht. */\
	struct gx_ht_cache_s *ht_cache;		/* (Shared) by all gstates */\
\
		/* Color (device-dependent): */\
\
	struct gs_cie_render_s *cie_render;	/* (RC) may be 0 */\
	gx_transfer_map *black_generation;	/* (RC) may be 0 */\
	gx_transfer_map *undercolor_removal;	/* (RC) may be 0 */\
		/* set_transfer holds the transfer functions specified by */\
		/* set[color]transfer; effective_transfer includes the */\
		/* effects of overrides by TransferFunctions in halftone */\
		/* dictionaries.  (In Level 1 systems, set_transfer and */\
		/* effective_transfer are always the same.) */\
	gx_transfer set_transfer;		/* members are (RC) */\
	gx_transfer effective_transfer;		/* see below */\
\
		/* Color caches: */\
\
		/* cie_joint_caches depend on cie_render and */\
		/* the color space. */\
	struct gx_cie_joint_caches_s *cie_joint_caches;		/* (RC) */\
		/* cmap_procs depend on the device's color_info. */\
	const struct gx_color_map_procs_s *cmap_procs;		/* static */\
		/* The contents of pattern_cache depend on the */\
		/* the color space and the device's color_info and */\
		/* resolution. */\
	struct gx_pattern_cache_s *pattern_cache	/* (Shared) by all gstates */

/*
 * Enumerate the reference-counted pointers in a c.r. state.  Note that
 * effective_transfer doesn't contribute to the reference count: it points
 * either to the same objects as set_transfer, or to objects in a halftone
 * structure that someone else worries about.
 */
#define gs_cr_state_do_rc_ptrs(m)\
  m(halftone) m(dev_ht) m(cie_render)\
  m(black_generation) m(undercolor_removal)\
  m(set_transfer.colored.red) m(set_transfer.colored.green)\
  m(set_transfer.colored.blue) m(set_transfer.colored.gray)\
  m(cie_joint_caches)

/* Enumerate the pointers in a c.r. state. */
#define gs_cr_state_do_ptrs(m)\
  m(0,halftone) m(1,dev_ht) m(2,ht_cache)\
  m(3,cie_render) m(4,black_generation) m(5,undercolor_removal)\
  m(6,set_transfer.colored.red) m(7,set_transfer.colored.green)\
  m(8,set_transfer.colored.blue) m(9,set_transfer.colored.gray)\
  m(10,effective_transfer.colored.red) m(11,effective_transfer.colored.green)\
  m(12,effective_transfer.colored.blue) m(13,effective_transfer.colored.gray)\
  m(14,cie_joint_caches) m(15,pattern_cache)
#define st_cr_state_num_ptrs 16

/*
 * Define constant values that can be allocated once and shared among
 * all imager states in an address space.
 */
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif
typedef union gx_device_color_spaces_s {
    struct dcn_ {
	gs_color_space *Gray;
	gs_color_space *RGB;
	gs_color_space *CMYK;
    } named;
    gs_color_space *indexed[3];
} gx_device_color_spaces_t;
typedef struct gs_imager_state_shared_s {
    rc_header rc;
    gx_device_color_spaces_t device_color_spaces;
} gs_imager_state_shared_t;

#define private_st_imager_state_shared()	/* in gsistate.c */\
  gs_private_st_ptrs3(st_imager_state_shared, gs_imager_state_shared_t,\
    "gs_imager_state_shared", imager_state_shared_enum_ptrs,\
    imager_state_shared_reloc_ptrs, device_color_spaces.named.Gray,\
    device_color_spaces.named.RGB, device_color_spaces.named.CMYK)

/* Define the imager state structure itself. */
#define gs_imager_state_common\
	gs_memory_t *memory;\
	void *client_data;\
	gs_imager_state_shared_t *shared;\
	gx_line_params line_params;\
	gs_matrix_fixed ctm;\
	gs_logical_operation_t log_op;\
	gx_color_value alpha;\
	bool overprint;\
	float flatness;\
	gs_fixed_point fill_adjust;	/* fattening for fill */\
	bool stroke_adjust;\
	bool accurate_curves;\
	float smoothness;\
	const gx_color_map_procs *\
	  (*get_cmap_procs)(P2(const gs_imager_state *, const gx_device *));\
	gs_color_rendering_state_common
#define st_imager_state_num_ptrs\
  (st_line_params_num_ptrs + st_cr_state_num_ptrs + 2)
/* Access macros */
#define ctm_only(pis) (*(const gs_matrix *)&(pis)->ctm)
#define ctm_only_writable(pis) (*(gs_matrix *)&(pis)->ctm)
#define set_ctm_only(pis, mat) (*(gs_matrix *)&(pis)->ctm = (mat))
#define gs_init_rop(pis) ((pis)->log_op = lop_default)
#define gs_currentflat_inline(pis) ((pis)->flatness)
#define gs_currentlineparams_inline(pis) (&(pis)->line_params)
#define gs_current_logical_op_inline(pis) ((pis)->log_op)
#define gs_set_logical_op_inline(pis, lop) ((pis)->log_op = (lop))

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif

struct gs_imager_state_s {
    gs_imager_state_common;
};

/* Initialization for gs_imager_state */
#define gs_imager_state_initial(scale)\
  0, 0, 0, { gx_line_params_initial },\
   { scale, 0.0, 0.0, -(scale), 0.0, 0.0 },\
  lop_default, gx_max_color_value, 0/*false*/, 1.0,\
   { fixed_half, fixed_half }, 0/*false*/, 0/*false*/, 1.0,\
  gx_default_get_cmap_procs

/* The imager state structure is public only for subclassing. */
#define public_st_imager_state()	/* in gsistate.c */\
  gs_public_st_composite(st_imager_state, gs_imager_state, "gs_imager_state",\
    imager_state_enum_ptrs, imager_state_reloc_ptrs)

/* Initialize an imager state, other than the parts covered by */
/* gs_imager_state_initial. */
int gs_imager_state_initialize(P2(gs_imager_state * pis, gs_memory_t * mem));

/* Make a temporary copy of a gs_imager_state.  Note that this does not */
/* do all the necessary reference counting, etc. */
gs_imager_state *
    gs_imager_state_copy(P2(const gs_imager_state * pis, gs_memory_t * mem));

/* Increment reference counts to note that an imager state has been copied. */
void gs_imager_state_copied(P1(gs_imager_state * pis));

/* Adjust reference counts before assigning one imager state to another. */
void gs_imager_state_pre_assign(P2(gs_imager_state *to,
				   const gs_imager_state *from));

/* Free device color spaces.  Perhaps this should be declared elsewhere? */
void gx_device_color_spaces_free(P3(gx_device_color_spaces_t *pdcs,
				    gs_memory_t *mem, client_name_t cname));

/* Release an imager state. */
void gs_imager_state_release(P1(gs_imager_state * pis));

#endif /* gxistate_INCLUDED */
