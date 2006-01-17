/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsdcolor.h,v 1.14 2004/03/16 01:25:19 dan Exp $ */
/* Device color representation for drivers */

#ifndef gsdcolor_INCLUDED
#  define gsdcolor_INCLUDED

#include "gsccolor.h"
#include "gxarith.h"		/* for imod */
#include "gxbitmap.h"
#include "gxhttile.h"
#include "gxcindex.h"
#include "gxwts.h"

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

#ifndef gx_device_saved_color_DEFINED
#  define gx_device_saved_color_DEFINED
typedef struct gx_device_color_saved_s  gx_device_color_saved;
#endif

#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * The definitions in the following section of the file are the only
 * ones that should be used by read-only clients such as implementors
 * of high-level driver functions.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * A device color consists of a base color and an optional (tiled) mask.
 * The base color may be a pure color, a binary halftone, or a colored
 * bitmap (color halftone or colored Pattern).  The mask is used for
 * both colored and uncolored Patterns.
 */

/* Accessing a pure color. */
#define gx_dc_is_pure(pdc)\
  ((pdc)->type == gx_dc_type_pure)
#define gx_dc_writes_pure(pdc, lop)\
  (gx_dc_is_pure(pdc) && lop_no_S_is_T(lop))
#define gx_dc_pure_color(pdc)\
  ((pdc)->colors.pure)

/* Accessing the phase of a halftone. */
#define gx_dc_phase(pdc)\
  ((pdc)->phase)

/* Accessing a binary halftone. */
#define gx_dc_is_binary_halftone(pdc)\
  ((pdc)->type == gx_dc_type_ht_binary)
#define gx_dc_binary_tile(pdc)\
  (&(pdc)->colors.binary.b_tile->tiles)
#define gx_dc_binary_color0(pdc)\
  ((pdc)->colors.binary.color[0])
#define gx_dc_binary_color1(pdc)\
  ((pdc)->colors.binary.color[1])

/* Accessing a colored halftone. */
#define gx_dc_is_colored_halftone(pdc)\
  ((pdc)->type == gx_dc_type_ht_colored)

/*
 * Test device colors for equality.  Testing for equality is done
 * for determining when cache values, etc. can be used.  Thus these
 * routines should err toward false responses if there is any question
 * about the equality of the two device colors.
 */
bool gx_device_color_equal(const gx_device_color *pdevc1,
			   const gx_device_color *pdevc2);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * The definitions in the following section of the file, plus the ones
 * just above, are the only ones that should be used by clients that
 * set as well as read device colors.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define color_is_set(pdc)\
  ((pdc)->type != gx_dc_type_none)
#define color_unset(pdc)\
  ((pdc)->type = gx_dc_type_none)

#define gx_dc_is_null(pdc)\
  ((pdc)->type == gx_dc_type_null)
#define color_is_null(pdc) gx_dc_is_null(pdc)
#define color_set_null(pdc)\
  ((pdc)->type = gx_dc_type_null)

#define color_is_pure(pdc) gx_dc_is_pure(pdc)
#define color_writes_pure(pdc, lop) gx_dc_writes_pure(pdc, lop)
/*
 * Used to define 'pure' (solid - without halftoning or patterns) colors.
 * This macro assumes the colorspace and client color information is already
 * defined in the device color strucTure.  If not then see the next macro.
 */
#define color_set_pure(pdc, color)\
  ((pdc)->colors.pure = (color),\
   (pdc)->type = gx_dc_type_pure)
/*
 * Used to create special case device colors for which the colorspace
 * and client colors are not already contained in the device color.
 */
#define set_nonclient_dev_color(pdc, color)\
    color_set_pure(pdc, color);\
    (pdc)->ccolor_valid = false

/* Set the phase to an offset from the tile origin. */
#define color_set_phase(pdc, px, py)\
  ((pdc)->phase.x = (px),\
   (pdc)->phase.y = (py))
/* Set the phase from the halftone phase in a graphics state. */
#define color_set_phase_mod(pdc, px, py, tw, th)\
  color_set_phase(pdc, imod(-(px), tw), imod(-(py), th))

#define color_is_binary_halftone(pdc) gx_dc_is_binary_halftone(pdc)
#define color_set_binary_halftone_component(pdc, ht, index, color0, color1, level)\
  ((pdc)->colors.binary.b_ht = (ht),\
   (pdc)->colors.binary.b_index = (index),\
   (pdc)->colors.binary.color[0] = (color0),\
   (pdc)->colors.binary.color[1] = (color1),\
   (pdc)->colors.binary.b_level = (level),\
   (pdc)->type = gx_dc_type_ht_binary)
#define color_set_binary_halftone(pdc, ht, color0, color1, level)\
  color_set_binary_halftone_component(pdc, ht, -1, color0, color1, level)
#define color_set_binary_tile(pdc, color0, color1, tile)\
  ((pdc)->colors.binary.b_ht = 0,\
   (pdc)->colors.binary.color[0] = (color0),\
   (pdc)->colors.binary.color[1] = (color1),\
   (pdc)->colors.binary.b_index = -1, /* irrelevant */\
   (pdc)->colors.binary.b_tile = (tile),\
   (pdc)->type = gx_dc_type_ht_binary)

#define color_is_colored_halftone(pdc) gx_dc_is_colored_halftone(pdc)
#define _color_set_c(pdc, i, b, l)\
  ((pdc)->colors.colored.c_base[i] = (b),\
   (pdc)->colors.colored.c_level[i] = (l))

/* Some special clients set the individual components separately. */
void gx_complete_halftone(gx_device_color *pdevc, int num_comps,
                          gx_device_halftone *pdht);

/* Note that color_set_null_pattern doesn't set mask.ccolor. */
#define color_set_null_pattern(pdc)\
 ((pdc)->mask.id = gx_no_bitmap_id,\
  (pdc)->mask.m_tile = 0,\
  (pdc)->colors.pattern.p_tile = 0,\
  (pdc)->type = gx_dc_type_pattern)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * The remaining definitions are internal ones that are included in this
 * file only because C's abstraction mechanisms aren't strong enough to
 * allow us to keep them separate and still have in-line access to the
 * commonly used members.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Define opaque types for objects referenced by device colors. */

#ifndef gx_ht_tile_DEFINED
#  define gx_ht_tile_DEFINED
typedef struct gx_ht_tile_s gx_ht_tile;
#endif

#ifndef gx_color_tile_DEFINED
#  define gx_color_tile_DEFINED
typedef struct gx_color_tile_s gx_color_tile;
#endif

/*
 * The device color in the graphics state is computed from client color
 * specifications, and kept current through changes in transfer function,
 * device, and (if relevant) halftone phase.
 * (gx_set_dev_color sets the device color if needed.)
 * For binary halftones (and eventually colored halftones as well),
 * the bitmaps are only cached, so internal clients (the painting operators)
 * must call gx_color_load to ensure that the bitmap is available.
 * Device color elements set by gx_color_load are marked with @ below.
 *
 * Base colors are represented as follows:
 *
 *      Pure color (gx_dc_pure):
 *              colors.pure = the color;
 *      Binary halftone (gx_dc_ht_binary):
 *              colors.binary.b_ht = the device halftone;
 *		colors.binary.b_index = -1 if b_ht is the halftone,
 *		  otherwise an index in b_ht.components
 *              colors.binary.color[0] = the color for 0s (darker);
 *              colors.binary.color[1] = the color for 1s (lighter);
 *              colors.binary.b_level = the number of pixels to lighten,
 *                0 < halftone_level < P, the number of pixels in the tile;
 *      @       colors.binary.b_tile points to an entry in the binary
 *                tile cache.
 *      Colored halftone (gx_dc_ht_colored):
 *              colors.colored.c_ht = the device halftone;
 *              colors.colored.c_level[0..N-1] = the halftone levels,
 *                like b_level;
 *              colors.colored.c_base[0..N-1] = the base colors;
 *                N = the device color_info.num_components
 *		    (3 for RGB devices, 4 for CMYK devices, ? for DeviceN);
 *                0 <= c_level[i] < P;
 *                0 <= c_base[i] <= dither_rgb;
 *              colors.colored.alpha = the opacity.
 *		colors.colored.plane_mask: bit 2^i = 1 iff c_level[i] != 0
 *      Colored PatternType 1 pattern (gx_dc_pattern):
 *              (mask is also set, see below)
 *      @       colors.pattern.p_tile points to a gx_color_tile in
 *                the pattern cache, or is NULL for a null pattern.
 *
 * The phase element is used for all colors except pure ones.  It holds the
 * negative of the graphics state halftone phase, modulo the halftone tile
 * size.
 *
 * The ccolor element is used for all kinds of patterns.  It is needed for
 * rendering the pattern.
 *
 * The mask elements of a device color are only used for PatternType 1
 * patterns:
 *      Non-pattern:
 *              mask is unused.
 *      Pattern:
 *              mask.id gives the ID of the pattern (and its mask);
 *              mask.m_phase holds the negative of the graphics state
 *                halftone phase;
 *      @       mask.m_tile points to a gx_color_tile in the pattern cache,
 *                or is NULL for a pattern that doesn't require a mask.
 *                (The 'bits' of the tile are not accessed.)
 *                For colored patterns requiring a mask, p_tile and
 *                mask.m_tile point to the same cache entry.
 * For masked colors, gx_set_dev_color replaces the type with a different
 * type that applies the mask when painting.  These types are not defined
 * here, because they are only used in Level 2.
 */

/* Define the (opaque) type for device color types. */
/* The name is an unfortunate anachronism. */
typedef struct gx_device_color_type_s gx_device_color_type_t;
typedef const gx_device_color_type_t *gx_device_color_type;

struct gx_device_color_s {
    /*
     * Since some compilers don't allow static initialization of a
     * union, we put the type first.
     */
    gx_device_color_type type;
    /*
     * See the comment above for descriptions of the members.  We use
     * b_, c_, and p_ member names because some old compilers don't
     * allow the same name to be used for two different structure
     * members even when it's unambiguous.
     */
    union _c {
	gx_color_index pure;
	struct _bin {
	    const gx_device_halftone *b_ht;
	    gx_color_index color[2];
	    uint b_level;
	    int b_index;
	    gx_ht_tile *b_tile;
	} binary;
	struct _col {
	    gx_device_halftone *c_ht; /* non-const for setting cache ptr */
	    ushort num_components;
	    byte c_base[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    uint c_level[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    ushort /*gx_color_value */ alpha;
#if GX_DEVICE_COLOR_MAX_COMPONENTS <= ARCH_SIZEOF_SHORT * 8
	    ushort plane_mask;
#else
#if GX_DEVICE_COLOR_MAX_COMPONENTS <= ARCH_SIZEOF_INT * 8
	    uint plane_mask;
#else
	    gx_color_index plane_mask;
#endif
#endif
	} colored;
	struct _wts {
	    const gx_device_halftone *w_ht;
	    wts_screen_sample_t levels[GX_DEVICE_COLOR_MAX_COMPONENTS];
	    ushort num_components;

	    /* plane_mask and base_color would be an optimization */
	    gx_color_index plane_vector[GX_DEVICE_COLOR_MAX_COMPONENTS];
	} wts;
	struct _pat {
	    gx_color_tile *p_tile;
	} /*(colored) */ pattern;
    } colors;
    gs_int_point phase;
    /*
     * This flag indicates if the paint values in ccolor are valid.  They
     * are valid for most cases.  However there are some special cases
     * in which a device color is initialized to specific values (usually
     * black or white) instead of being created from a color space and
     * color values.
     */
    bool ccolor_valid;
    /*
     * 'ccolor' (a "client color") is valid iff 'ccolor_valid' is true.
     * For non-pattern colors, it contains the original paint values.
     * For pattern colors, it contains information required for remapping
     * the pattern.
     */
    gs_client_color ccolor;

    struct _mask {
	struct mp_ {
	    short x, y;
	} m_phase;
	gx_bitmap_id id;
	gx_color_tile *m_tile;
    } mask;
};

/*extern_st(st_device_color); *//* in gxdcolor.h */
#define public_st_device_color() /* in gxcmap.c */\
  gs_public_st_composite(st_device_color, gx_device_color, "gx_device_color",\
    device_color_enum_ptrs, device_color_reloc_ptrs)
#define st_device_color_max_ptrs (st_client_color_max_ptrs + 2)

/*
 * For the command list, it is useful to record the most recent device
 * color placed in a band, so as to avoid sending unnecessary
 * information. The following structure is used for that purpose. It is
 * created by the save_dc method, and can be utilized by the write
 * method. It should otherwise be considered opaque, though it is
 * guarranteed not to contain pointers to allocated memory (and thus does
 * not interact with the GC code for containing structures).
 *
 * The reason a structure distinct from the device color itself is used
 * for this purpose is related to an anomally involving reference to
 * device halftones. The gx_device_halftone structure is reference
 * counted, but a long standing (and not easily removable) convention
 * in the code states that only reference from imager (graphic) states
 * to halftones are counted; reference from device colors are not. The
 * pointer to a halftone in a saved device color may, therefore,
 * become a dangling pointer. This does not occur in other uses of a
 * device color, because a color can only be usable when the hafltone
 * it references is the current halftone in some imager state.
 *
 * Because halftones are large and seldom changed, they are always sent
 * as "all bands" commands. Individual device colors, by contrast, are
 * usually written just for the bands that make use of them. The
 * distinction between these two cases can only be handled by command
 * list writer code itself, so this structure does not involve the
 * halftone. If the halftone changes, however, the write method should
 * be passed a null pointer for the saved color operand; this will
 * ensure the the full device color information is written.
 *
 * Currently patterns cannot be passed through the command list,
 * however vector devices need to save a color for comparing
 * it with another color, which appears later.
 * We provide a minimal support, which is necessary
 * for the current implementation of pdfwrite.
 * It is not sufficient for restoring the pattern from the saved color.
 */

struct gx_device_color_saved_s {
    gx_device_color_type    type;
    union _svc {
        gx_color_index  pure;
        struct _svbin {
            gx_color_index  b_color[2];
            uint            b_level;
            int             b_index;
        }               binary;
        struct _svcol {
            byte    c_base[GX_DEVICE_COLOR_MAX_COMPONENTS];
            uint    c_level[GX_DEVICE_COLOR_MAX_COMPONENTS];
            ushort  alpha;
        }               colored;
        struct _swts {
            wts_screen_sample_t levels[GX_DEVICE_COLOR_MAX_COMPONENTS];
        }               wts;
	struct _pattern {
	    gs_id id;
	    gs_int_point phase;
	}		pattern;
	struct _pattern2 {
	    gs_id id;
	}		pattern2;
    }                       colors;
    gs_int_point            phase;
};


/*
 * Define the standard device color types.
 * We define them here as pointers to the real types only because a few
 * C compilers don't allow declaring externs with abstract struct types;
 * we redefine them as macros in gxdcolor.h where the concrete type for
 * gx_device_color_procs is available.
 * We spell out the definition of gx_device_color type because some
 * C compilers can't handle the typedef correctly.
 */
#ifndef gx_dc_type_none
extern const gx_device_color_type_t *const gx_dc_type_none;	/* gxdcolor.c */
#endif
#ifndef gx_dc_type_null
extern const gx_device_color_type_t *const gx_dc_type_null;	/* gxdcolor.c */
#endif
#ifndef gx_dc_type_pure
extern const gx_device_color_type_t *const gx_dc_type_pure;	/* gxdcolor.c */
#endif
		/*
		 * We don't declare gx_dc_pattern here, so as not to create
		 * a spurious external reference in Level 1 systems.
		 */
#ifndef gx_dc_type_pattern
/*extern const gx_device_color_type_t * const gx_dc_type_pattern; *//* gspcolor.c */
#endif
#ifndef gx_dc_type_ht_binary
extern const gx_device_color_type_t *const gx_dc_type_ht_binary;	/* gxht.c */
#endif
#ifndef gx_dc_type_ht_colored
extern const gx_device_color_type_t *const gx_dc_type_ht_colored;	/* gxcht.c */
#endif
#ifndef gx_dc_type_ht_colored
extern const gx_device_color_type_t *const gx_dc_type_wts;	/* gxwts.c */
#endif

#endif /* gsdcolor_INCLUDED */
