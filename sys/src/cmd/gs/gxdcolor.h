/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxdcolor.h */
/* Device color representation for Ghostscript */

#ifndef gxdcolor_INCLUDED
#  define gxdcolor_INCLUDED

#include "gsrefct.h"
#include "gxbitmap.h"
#include "gsstruct.h"			/* for extern_st, GC procs */

/* Define the type for device color indices. */
typedef unsigned long gx_color_index;
/* Define the 'transparent' color index. */
#define gx_no_color_index_value (-1)	/* no cast -> can be used in #if */
/* The SGI C compiler provided with Irix 5.2 gives error messages */
/* if we use the proper definition of gx_no_color_index: */
/*#define gx_no_color_index ((gx_color_index)gx_no_color_index_value)*/
/* Instead, we must spell out the typedef: */
#define gx_no_color_index ((unsigned long)gx_no_color_index_value)

/*
 * Device colors are 'objects'.  Currently they only have two procedures,
 * but they might have more in the future.  In order to simplify
 * memory management, we use a union, but since different variants may have
 * different pointer tracing procedures, we have to include those
 * procedures in the type.
 */

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

#ifndef gx_ht_tile_DEFINED
#  define gx_ht_tile_DEFINED
typedef struct gx_ht_tile_s gx_ht_tile;
#endif

typedef struct gx_device_color_procs_s {

	/* If necessary and possible, load the halftone cache */
	/* with the rendering of this color. */

#define dev_color_proc_load(proc)\
  int proc(P2(gx_device_color *pdevc, const gs_state *pgs))
	dev_color_proc_load((*load));

	/* Fill a rectangle with the color.  We only pass the gs_state */
	/* because it supplies the halftone phase; eventually */
	/* we should incorporate the state in the cached bits. */
	/* We pass the device so that pattern fills can substitute */
	/* a tiled mask clipping device. */

#define dev_color_proc_fill_rectangle(proc)\
  int proc(P7(const gx_device_color *pdevc, int x, int y, int w, int h,\
    gx_device *dev, const gs_state *pgs))
	dev_color_proc_fill_rectangle((*fill_rectangle));

	/* Trace the pointers for the garbage collector. */

	struct_proc_enum_ptrs((*enum_ptrs));
	struct_proc_reloc_ptrs((*reloc_ptrs));

} gx_device_color_procs;

/*
 * A device color consists of a base color and an optional (tiled) mask.
 * The base color may be a pure color, a binary halftone, or a colored
 * bitmap (color halftone or colored Pattern).  The mask is used for
 * both colored and uncolored Patterns.
 *
 * The device color in the graphics state is computed from client color
 * specifications, and kept current through changes in transfer function
 * or device.  (gx_set_dev_color sets the device color if needed.)
 * For binary halftones (and eventually colored halftones as well),
 * the bitmaps are only cached, so internal clients (the painting operators)
 * must call gx_color_load to ensure that the bitmap is available.
 * Device color elements set by gx_color_load are marked with @ below.
 *
 * Base colors are represented as follows:
 *
 *	Pure color (gx_dc_pure):
 *		colors.pure = the color;
 *	Binary halftone (gx_dc_ht_binary):
 *		colors.binary.color[0] = the color for 0s (darker);
 *		colors.binary.color[1] = the color for 1s (lighter);
 *		colors.binary.b_level = the number of pixels to lighten,
 *		  0 < halftone_level < P, the number of pixels in the tile;
 *	@	colors.binary.b_tile points to an entry in the binary
 *		  tile cache.
 *	Colored halftone (gx_dc_ht_colored):
 *		colors.colored.c_level[0..N-1] = the halftone levels,
 *		  like b_level;
 *		colors.colored.c_base[0..N-1] = the base colors;
 *		  N=3 for RGB devices, 4 for CMYK devices;
 *		  0 <= c_level[i] < P;
 *		  0 <= c_base[i] <= dither_rgb;
 *		colors.colored.alpha = the opacity.
 *	Colored pattern (gx_dc_pattern):
 *		(id and mask are also set, see below)
 *	@	colors.pattern.p_tile points to a gx_color_tile in
 *		  the pattern cache.
 * The id and mask elements of a device color are only used for patterns:
 *	Non-pattern:
 *		id and mask are unused.
 *	Pattern:
 *		id gives the ID of the pattern (and its mask);
 *	@	mask points to a gx_color_tile in the pattern cache.
 *		  (The 'bits' of the tile are not accessed.)
 *		  For colored patterns requiring a mask, p_tile and mask
 *		  point to the same cache entry.
 * For masked colors, gx_set_dev_color replaces the type with a different
 * type that applies the mask when painting.  These types are not defined
 * here, because they are only used in Level 2.
 */
typedef struct gx_color_tile_s gx_color_tile;

/* A device color type is just a pointer to the procedures. */
typedef const gx_device_color_procs _ds *gx_device_color_type;

struct gx_device_color_s {
	/* See the comment above for descriptions of the members. */
	/* We use b_, c_, and p_ member names because */
	/* some old compilers don't allow the same name to be used for */
	/* two different structure members even when it's unambiguous. */
	union _c {
		gx_color_index pure;
		struct _bin {
			gx_color_index color[2];
			uint b_level;
			gx_ht_tile *b_tile;
		} binary;
		struct _col {
			byte c_base[4];
			uint c_level[4];
			ushort /*gx_color_value*/ alpha;
		} colored;
		struct _pat {
			gx_color_tile *p_tile;
		} /*(colored)*/ pattern;
	} colors;
	gx_bitmap_id id;
	gx_color_tile *mask;
	/* We put the type last to preserve word alignment */
	/* on platforms with short ints. */
	gx_device_color_type type;
};
extern_st(st_device_color);
#define public_st_device_color() /* in gxcmap.c */\
  gs_public_st_composite(st_device_color, gx_device_color, "gx_device_color",\
    device_color_enum_ptrs, device_color_reloc_ptrs)
#define st_device_color_max_ptrs 2

/* Define the standard device color types. */
extern const gx_device_color_procs
	gx_dc_none, gx_dc_pure, gx_dc_ht_binary,
	gx_dc_ht_colored;
/* We don't declare gx_dc_pattern here, so as not to create */
/* a spurious external reference in Level 1 systems. */
#define color_is_set(pdevc)\
  ((pdevc)->type != &gx_dc_none)
#define color_unset(pdevc)\
  ((pdevc)->type = &gx_dc_none)
#define color_is_pure(pdevc)\
  ((pdevc)->type == &gx_dc_pure)
#define color_set_pure(pdevc, color)\
  ((pdevc)->colors.pure = (color),\
   (pdevc)->type = &gx_dc_pure)
#define color_set_binary_halftone(pdevc, color1, color2, level)\
  ((pdevc)->colors.binary.color[0] = (color1),\
   (pdevc)->colors.binary.color[1] = (color2),\
   (pdevc)->colors.binary.b_level = (level),\
   (pdevc)->type = &gx_dc_ht_binary)
#define _color_set_c(pdevc, i, b, l)\
  ((pdevc)->colors.colored.c_base[i] = (b),\
   (pdevc)->colors.colored.c_level[i] = (l))
#define color_set_rgb_halftone(pdevc, br, lr, bg, lg, bb, lb, a)\
  (_color_set_c(pdevc, 0, br, lr),\
   _color_set_c(pdevc, 1, bg, lg),\
   _color_set_c(pdevc, 2, bb, lb),\
   (pdevc)->colors.colored.alpha = (a),\
   (pdevc)->type = &gx_dc_ht_colored)
#define color_set_cmyk_halftone(pdevc, bc, lc, bm, lm, by, ly, bk, lk)\
  (_color_set_c(pdevc, 0, bc, lc),\
   _color_set_c(pdevc, 1, bm, lm),\
   _color_set_c(pdevc, 2, by, ly),\
   _color_set_c(pdevc, 3, bk, lk),\
   (pdevc)->colors.colored.alpha = max_ushort,\
   (pdevc)->type = &gx_dc_ht_colored)
#define color_set_pattern(pdevc, pt)\
 ((pdevc)->colors.pattern.p_tile = (pt),\
  (pdevc)->type = &gx_dc_pattern)

/* Define the implementation of standard device color types. */
/* These are exported for the implementation of Pattern colors. */
dev_color_proc_load(gx_dc_no_load);
dev_color_proc_fill_rectangle(gx_dc_no_fill_rectangle);
dev_color_proc_load(gx_dc_pure_load);
dev_color_proc_fill_rectangle(gx_dc_pure_fill_rectangle);
dev_color_proc_load(gx_dc_ht_binary_load);
dev_color_proc_fill_rectangle(gx_dc_ht_binary_fill_rectangle);
dev_color_proc_load(gx_dc_ht_colored_load);
dev_color_proc_fill_rectangle(gx_dc_ht_colored_fill_rectangle);

/* Set up device color 1 for writing into a mask cache */
/* (e.g., the character cache). */
void gx_set_device_color_1(P1(gs_state *pgs));

/* Remap the color if necessary. */
int gx_remap_color(P1(gs_state *));
#define gx_set_dev_color(pgs)\
  if ( !color_is_set((pgs)->dev_color) )\
   { int code_dc = gx_remap_color(pgs);\
     if ( code_dc != 0 ) return code_dc;\
   }

/* Indicate that the device color needs remapping. */
#define gx_unset_dev_color(pgs)\
  color_unset((pgs)->dev_color)

/* Load the halftone cache in preparation for drawing. */
#define gx_color_load(pdevc, pgs)\
  (*(pdevc)->type->load)(pdevc, pgs)

/* Fill a rectangle with a color. */
#define gx_fill_rectangle(x, y, w, h, pdevc, pgs)\
  (*(pdevc)->type->fill_rectangle)(pdevc, x, y, w, h, pgs->device, pgs)

#endif					/* gxdcolor_INCLUDED */
