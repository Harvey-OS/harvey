/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxfcmap.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internal CMap data definition */

/* This file should be called gxcmap.h, except that name is already used. */

#ifndef gxfcmap_INCLUDED
#  define gxfcmap_INCLUDED

#include "gsfcmap.h"
#include "gsuid.h"

/* Define the structure for CIDSystemInfo. */
typedef struct gs_cid_system_info_s {
    gs_const_string Registry;
    gs_const_string Ordering;
    int Supplement;
} gs_cid_system_info;
/*extern_st(st_cid_system_info);*/
extern_st(st_cid_system_info_element);
#define private_st_cid_system_info() /* in gsfcmap.c */\
  gs_public_st_const_strings2(st_cid_system_info, gs_cid_system_info,\
    "gs_cid_system_info", cid_si_enum_ptrs, cid_si_reloc_ptrs,\
    Registry, Ordering)
#define public_st_cid_system_info_element() /* in gsfcmap.c */\
  gs_public_st_element(st_cid_system_info_element, gs_cid_system_info,\
    "gs_cid_system_info[]", cid_si_elt_enum_ptrs, cid_si_elt_reloc_ptrs,\
    st_cid_system_info)

/*
 * The main body of data in a CMap is two code maps, one for defined
 * characters, one for notdefs.  Each code map is a multi-level tree,
 * one level per byte decoded from the input string.  Each node of
 * the tree may be:
 *      a character code (1-4 bytes)
 *      a character name (gs_glyph)
 *      a CID (gs_glyph)
 *      a subtree
 */
typedef enum {
    cmap_char_code,
    cmap_glyph,			/* character name or CID */
    cmap_subtree
} gx_code_map_type;
typedef struct gx_code_map_s gx_code_map;
struct gx_code_map_s {
    byte first;			/* first char code covered by this node */
    byte last;			/* last char code ditto */
         uint /*gx_code_map_type */ type:2;
    uint num_bytes1:2;		/* # of bytes -1 for char_code */
         uint /*bool */ add_offset:1;	/* if set, add char - first to ccode / glyph */
    /* We would like to combine the two unions into a union of structs, */
    /* but no compiler seems to do the right thing about packing. */
    union bd_ {
	byte font_index;	/* for leaf, font index */
	/* (only non-zero if rearranged font) */
	byte count1;		/* for subtree, # of entries -1 */
    } byte_data;
    union d_ {
	gs_char ccode;		/* num_bytes bytes */
	gs_glyph glyph;
	gx_code_map *subtree;	/* [count] */
    } data;
    gs_cmap *cmap;		/* point back to CMap for GC mark proc */
};

/* The GC information for a gx_code_map is complex, because names must be */
/* traced. */
extern_st(st_code_map);
extern_st(st_code_map_element);
#define public_st_code_map()	/* in gsfcmap.c */\
  gs_public_st_composite(st_code_map, gx_code_map, "gx_code_map",\
    code_map_enum_ptrs, code_map_reloc_ptrs)
#define public_st_code_map_element() /* in gsfcmap.c */\
  gs_public_st_element(st_code_map_element, gx_code_map, "gx_code_map[]",\
    code_map_elt_enum_ptrs, code_map_elt_reloc_ptrs, st_code_map)

/* A CMap proper is relatively simple. */
struct gs_cmap_s {
    gs_uid uid;
    gs_cid_system_info *CIDSystemInfo;
    int num_fonts;
    int WMode;
    gx_code_map def;		/* defined characters (cmap_subtree) */
    gx_code_map notdef;		/* notdef characters (cmap_subtree) */
    gs_glyph_mark_proc_t mark_glyph;	/* glyph marking procedure for GC */
    void *mark_glyph_data;	/* closure data */
};

extern_st(st_cmap);
#define public_st_cmap()	/* in gsfcmap.c */\
  gs_public_st_ptrs5(st_cmap, gs_cmap, "gs_cmap",\
    cmap_enum_ptrs, cmap_reloc_ptrs, CIDSystemInfo,\
    uid.xvalues, def.data.subtree, notdef.data.subtree, mark_glyph_data)

#endif /* gxfcmap_INCLUDED */
