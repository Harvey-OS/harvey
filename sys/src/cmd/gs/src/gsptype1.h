/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsptype1.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Client interface to PatternType 1 Patterns */

#ifndef gsptype1_INCLUDED
#  define gsptype1_INCLUDED

#include "gspcolor.h"
#include "gxbitmap.h"

/* ---------------- Types and structures ---------------- */

/* PatternType 1 template */

typedef struct gs_pattern1_template_s {
    /*
     * The common template must come first.  It defines type, uid,
     * PatternType, and client_data.
     */
    gs_pattern_template_common;
    int PaintType;
    int TilingType;
    gs_rect BBox;
    float XStep;
    float YStep;
    int (*PaintProc) (P2(const gs_client_color *, gs_state *));
} gs_pattern1_template_t;

#define private_st_pattern1_template() /* in gspcolor.c */\
  gs_private_st_suffix_add0(st_pattern1_template,\
    gs_pattern1_template_t, "gs_pattern1_template_t",\
    pattern1_template_enum_ptrs, pattern1_template_reloc_ptrs,\
    st_pattern_template)
#define st_pattern1_template_max_ptrs st_pattern_template_max_ptrs

/* Backward compatibility */
typedef gs_pattern1_template_t gs_client_pattern;

/* ---------------- Procedures ---------------- */

/*
 * Construct a PatternType 1 Pattern color space.  If the base space is
 * NULL, the color space can only be used with colored patterns.
 */
extern int gs_cspace_build_Pattern1(P3(
				       gs_color_space ** ppcspace,
				       const gs_color_space * pbase_cspace,
				       gs_memory_t * pmem
				       ));

/* Initialize a PatternType 1 pattern. */
void gs_pattern1_init(P1(gs_pattern1_template_t *));

/* Backward compatibility */
#define gs_client_pattern_init(ppat) gs_pattern1_init(ppat)

/*
 * Define versions of make_pattern and get_pattern specifically for
 * PatternType 1 patterns.
 *
 * The gs_memory_t argument for gs_makepattern may be NULL, meaning use the
 * same allocator as for the gs_state argument.  Note that gs_makepattern
 * uses rc_alloc_struct_1 to allocate pattern instances.
 */
int gs_makepattern(P5(gs_client_color *, const gs_client_pattern *,
		      const gs_matrix *, gs_state *, gs_memory_t *));
const gs_client_pattern *gs_getpattern(P1(const gs_client_color *));

/*
 * Make a pattern from a bitmap or pixmap. The pattern may be colored or
 * uncolored, as determined by the mask operand. This code is intended
 * primarily for use by PCL.
 *
 * By convention, if pmat is null the identity matrix will be used, and if
 * id is no_UniqueID the code will assign a unique id. Thes conventions allow
 * gs_makebitmappattern to be implemented as a macro. Also, if mem is a
 * null pointer, the memory allocator for the graphic state is used.
 *
 * For mask patterns, pix_depth must be 1, while pcspace and white_index are
 * ignored; the polarity of the mask considers ones part of the mask, while
 * zeros are not. For colored patterns pspace must point to an indexed color
 * space and the image must used the canoncial Decode array for this color
 * space. For both cases no interpolation or adjustment is provided.
 *
 * For backwards compatibility, if mask is false, pcspace is null, and
 * pix_depth is 1, the pattern will be rendered with a color space that maps
 * 0 to white and 1 to black.
 *
 * The image must be described by a gx_tile_bitmap structure (this is actually
 * somewhat awkward, but the only option available at the moment), and the
 * pattern step will exactly match the image size. The client need not maintain
 * the gx_tile_bitmap structure after the completion of this call, but the
 * raw image data itself must be kept until the pattern is no longer needed.
 *
 * NB: For proper handling of transparency in PCL, there must be only a single
 *     white value accessed by the pattern image. If the palette contains
 *     multiple white values, the PCL component must remap the image data to
 *     ensure that all white indices are mapped to the single, given white
 *     index.
 */
extern int gs_makepixmappattern(P9(
				   gs_client_color * pcc,
				   const gs_depth_bitmap * pbitmap,
				   bool mask,
				   const gs_matrix * pmat,
				   long id,
				   const gs_color_space * pcspace,
				   uint white_index,
				   gs_state * pgs,
				   gs_memory_t * mem
				   ));

/*
 *  Backwards compatibility feature, to allow the existing
 *  gs_makebitmappattern operation to still function.
 */
extern int gs_makebitmappattern_xform(P7(
					 gs_client_color * pcc,
					 const gx_tile_bitmap * ptile,
					 bool mask,
					 const gs_matrix * pmat,
					 long id,
					 gs_state * pgs,
					 gs_memory_t * mem
					 ));

#define gs_makebitmappattern(pcc, tile, mask, pgs, mem)                 \
    gs_makebitmappattern_xform(pcc, tile, mask, 0, no_UniqueID, pgs, mem)

#endif /* gsptype1_INCLUDED */
