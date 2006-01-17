/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfcmap1.h,v 1.1 2002/04/08 21:02:44 lpd Exp $ */
/* Adobe CMapType 1 CMap structure definitions */

#ifndef gxfcmap1_INCLUDED
#  define gxfcmap1_INCLUDED

#include "gxfcmap.h"

/*
 * This is the concrete subclass of gs_cmap_t that directly implements
 * the Adobe CMapType 1/2 specification.
 *
 * In this implementation, the two maps (defs and notdefs) each consist of
 * two separate parts:
 *
 *	- A key map, which maps a parsed character to an index in a value
 *	table.
 *
 *	- A value table in which each entry specifies a string, a name, or a
 *	CID, as described in gxfcmap.h.
 *
 * We separate the value tables from the key maps so that large, closely
 * related CMaps such as UniCNS-UCS2-H and UniCNS-UTF8-H can (someday)
 * share the value tables but not the code space or key maps.
 */
typedef struct gs_cmap_adobe1_s gs_cmap_adobe1_t;

/*
 * To save space in lookup tables, all keys within each lookup range share
 * a prefix (which may be empty).
 *
 * The strings in this structure are all const after initialization.
 */
typedef struct gx_cmap_lookup_range_s {
    gs_cmap_adobe1_t *cmap;	/* back pointer for glyph marking */
    int num_entries;
    /* Keys */
    byte key_prefix[MAX_CMAP_CODE_SIZE];
    int key_prefix_size;	/* 0 .. MAX_CMAP_CODE_SIZE */
    int key_size;		/* 0 .. MAX_CMAP_CODE_SIZE - key_prefix_s' */
    bool key_is_range;
    gs_string keys;		/* [num_entries * key_size * (key_is_range+1)] */
    /* Values */
    gx_cmap_code_value_type_t value_type;
    int value_size;		/* bytes per value */
    gs_string values;		/* [num_entries * value_size] */
    int font_index;
} gx_cmap_lookup_range_t;
/*
 * The GC descriptor for lookup ranges is complex, because it must mark
 * names.
 */
extern_st(st_cmap_lookup_range_element);
#define public_st_cmap_lookup_range() /* in gsfcmap.c */\
  gs_public_st_composite(st_cmap_lookup_range, gx_cmap_lookup_range_t,\
    "gx_cmap_lookup_range_t", cmap_lookup_range_enum_ptrs,\
    cmap_lookup_range_reloc_ptrs)
#define public_st_cmap_lookup_range_element() /* in gsfcmap.c */\
  gs_public_st_element(st_cmap_lookup_range_element, gx_cmap_lookup_range_t,\
    "gx_cmap_lookup_range_t[]", cmap_lookup_range_elt_enum_ptrs,\
    cmap_lookup_range_elt_reloc_ptrs, st_cmap_lookup_range)

/*
 * The main body of data in a CMap is two code maps, one for defined
 * characters, one for notdefs.
 */
typedef struct gx_code_space_s {
    gx_code_space_range_t *ranges;
    int num_ranges;
} gx_code_space_t;
typedef struct gx_code_map_s {
    gx_cmap_lookup_range_t *lookup;
    int num_lookup;
} gx_code_map_t;
struct gs_cmap_adobe1_s {
    GS_CMAP_COMMON;
    gx_code_space_t code_space;
    gx_code_map_t def;		/* defined characters */
    gx_code_map_t notdef;	/* notdef characters */
    gs_glyph_mark_proc_t mark_glyph;	/* glyph marking procedure for GC */
    void *mark_glyph_data;	/* closure data */
};

extern_st(st_cmap_adobe1);
#define public_st_cmap_adobe1()	/* in gsfcmap1.c */\
  gs_public_st_suffix_add4(st_cmap_adobe1, gs_cmap_adobe1_t,\
    "gs_cmap_adobe1_t", cmap_adobe1_enum_ptrs, cmap_adobe1_reloc_ptrs,\
    st_cmap,\
    code_space.ranges, def.lookup, notdef.lookup, mark_glyph_data)

/* ---------------- Procedures ---------------- */

/*
 * Allocate and initialize an Adobe1 CMap.  The caller must still fill in
 * the code space ranges, lookup tables, keys, and values.
 */
int gs_cmap_adobe1_alloc(gs_cmap_adobe1_t **ppcmap, int wmode,
			 const byte *map_name, uint name_size,
			 uint num_fonts, uint num_ranges, uint num_lookups,
			 uint keys_size, uint values_size,
			 const gs_cid_system_info_t *pcidsi, gs_memory_t *mem);

#endif /* gxfcmap1_INCLUDED */
