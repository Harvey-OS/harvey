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

/*$Id: gxfcmap.h,v 1.5 2000/09/19 19:00:36 lpd Exp $ */
/* Internal CMap structure definitions */

/* This file should be called gxcmap.h, except that name is already used. */

#ifndef gxfcmap_INCLUDED
#  define gxfcmap_INCLUDED

#include "gsfcmap.h"
#include "gsuid.h"
#include "gxcid.h"

/*
 * A CMap has 3 separate parts, each designed for a different function:
 *
 *	- The code space, used for parsing the input string.
 *
 *	- Two key maps, one for defined characters, one for notdefs.
 *	Each of these maps a parsed character to an index in a value table.
 *
 *	- Two value tables in which each entry specifies a string, a name,
 *	or a CID.
 *
 * We separate the value tables from the key maps so that large, closely
 * related CMaps such as UniCNS-UCS2-H and UniCNS-UTF8-H can (someday)
 * share the value tables but not the code space or key maps.
 */

/*
 * A code space is a non-empty array of code space ranges.  The array is
 * sorted lexicographically.  Ranges must not overlap.  In each range,
 * first[i] <= last[i] for 0 <= i < num_bytes.
 */
#define MAX_CMAP_CODE_SIZE 4
typedef struct gx_code_space_range_s {
    byte first[MAX_CMAP_CODE_SIZE];
    byte last[MAX_CMAP_CODE_SIZE];
    int size;			/* 1 .. MAX_CMAP_CODE_SIZE */
} gx_code_space_range_t;
typedef struct gx_code_space_s {
    gx_code_space_range_t *ranges;
    int num_ranges;
} gx_code_space_t;

/*
 * A lookup table is a non-empty array of lookup ranges.  Each range has an
 * associated sorted lookup table, indexed by the num_key_bytes low-order
 * code bytes.  If key_is_range is true, each key is a range (2 x key_size
 * bytes); if false, each key is a single code (key_size bytes).
 */
typedef enum {
    CODE_VALUE_CID,		/* CIDs */
    CODE_VALUE_GLYPH,		/* glyphs */
    CODE_VALUE_CHARS		/* character(s) */
#define CODE_VALUE_MAX CODE_VALUE_CHARS
} gx_code_value_type_t;
/* The strings in this structure are all const after initialization. */
typedef struct gx_code_lookup_range_s {
    gs_cmap_t *cmap;		/* back pointer for glyph marking */
    /* Keys */
    byte key_prefix[MAX_CMAP_CODE_SIZE];
    int key_prefix_size;	/* 1 .. MAX_CMAP_CODE_SIZE */
    int key_size;		/* 0 .. MAX_CMAP_CODE_SIZE - key_prefix_s' */
    int num_keys;
    bool key_is_range;
    gs_string keys;		/* [num_keys * key_size * (key_is_range+1)] */
    /* Values */
    gx_code_value_type_t value_type;
    int value_size;		/* bytes per value */
    gs_string values;		/* [num_keys * value_size] */
    int font_index;
} gx_code_lookup_range_t;
/*
 * The GC descriptor for lookup ranges is complex, because it must mark
 * names.
 */
extern_st(st_code_lookup_range_element);
#define public_st_code_lookup_range() /* in gsfcmap.c */\
  gs_public_st_composite(st_code_lookup_range, gx_code_lookup_range_t,\
    "gx_code_lookup_range_t", code_lookup_range_enum_ptrs,\
    code_lookup_range_reloc_ptrs)
#define public_st_code_lookup_range_element() /* in gsfcmap.c */\
  gs_public_st_element(st_code_lookup_range_element, gx_code_lookup_range_t,\
    "gx_code_lookup_range_t[]", code_lookup_range_elt_enum_ptrs,\
    code_lookup_range_elt_reloc_ptrs, st_code_lookup_range)

/*
 * The main body of data in a CMap is two code maps, one for defined
 * characters, one for notdefs.
 */
typedef struct gx_code_map_s {
    gx_code_lookup_range_t *lookup;
    int num_lookup;
} gx_code_map_t;

/* A CMap proper is relatively simple. */
struct gs_cmap_s {
    int CMapType;		/* must be first; must be 0 or 1 */
    gs_const_string CMapName;
    gs_cid_system_info_t *CIDSystemInfo; /* [num_fonts] */
    int num_fonts;
    float CMapVersion;
    gs_uid uid;			/* XUID or nothing */
    long UIDOffset;
    int WMode;
    gx_code_space_t code_space;
    gx_code_map_t def;		/* defined characters */
    gx_code_map_t notdef;	/* notdef characters */
    gs_glyph_mark_proc_t mark_glyph;	/* glyph marking procedure for GC */
    void *mark_glyph_data;	/* closure data */
    gs_glyph_name_proc_t glyph_name;	/* glyph name procedure for printing */
    void *glyph_name_data;	/* closure data */
};

extern_st(st_cmap);
#define public_st_cmap()	/* in gsfcmap.c */\
  BASIC_PTRS(cmap_ptrs) {\
    GC_CONST_STRING_ELT(gs_cmap_t, CMapName),\
    GC_OBJ_ELT3(gs_cmap_t, CIDSystemInfo, uid.xvalues, code_space.ranges),\
    GC_OBJ_ELT2(gs_cmap_t, def.lookup, notdef.lookup),\
    GC_OBJ_ELT2(gs_cmap_t, mark_glyph_data, glyph_name_data)\
  };\
  gs_public_st_basic(st_cmap, gs_cmap_t, "gs_cmap",\
    cmap_ptrs, cmap_data)

/*
 * Initialize a just-allocated CMap, to ensure that all pointers are clean
 * for the GC.
 */
void gs_cmap_init(P1(gs_cmap_t *));

#endif /* gxfcmap_INCLUDED */
