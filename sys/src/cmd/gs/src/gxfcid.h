/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxfcid.h,v 1.6 2000/09/19 19:00:36 lpd Exp $ */
/* Definitions for CID-keyed fonts */

#ifndef gxfcid_INCLUDED
#  define gxfcid_INCLUDED

#include "gxcid.h"		/* for CIDSystemInfo */
#include "gxfont.h"
#include "gxfont42.h"

/* ---------------- Structures ---------------- */

/*
 * Define the data common to CIDFontType 0 (FontType 9, analogous to 1)
 * and 2 (FontType 11, analogous to 42).
 */
#define MAX_GDBytes 4
typedef struct gs_font_cid_data_s {
    gs_cid_system_info_t CIDSystemInfo;
    int CIDCount;
    int GDBytes;		/* optional, for standard glyph_data */
    /*int PaintType;*/
    /*float StrokeWidth;*/
} gs_font_cid_data;

extern_st(st_gs_font_cid_data);
#define public_st_gs_font_cid_data()	/* in gsfcid.c */\
  gs_public_st_suffix_add0_final(st_gs_font_cid_data,\
    gs_font_cid_data, "gs_font_cid_data",\
    font_cid_data_enum_ptrs, font_cid_data_reloc_ptrs,\
    gs_font_finalize, st_cid_system_info)
#define st_gs_font_cid_data_num_ptrs\
  st_cid_system_info_num_ptrs

/*
 * Define the structures for CIDFontType 0, 1, and 2 fonts.  In principle,
 * each of these should be in its own header file, but they're so small
 * that we include them here.
 */

/* CIDFontType 0 references an array of (partial) FontType 1 fonts. */

#ifndef gs_font_type1_DEFINED
#  define gs_font_type1_DEFINED
typedef struct gs_font_type1_s gs_font_type1;
#endif

#ifndef gs_font_cid0_DEFINED
#  define gs_font_cid0_DEFINED
typedef struct gs_font_cid0_s gs_font_cid0;
#endif

#define MAX_FDBytes 4
typedef struct gs_font_cid0_data_s {
    gs_font_cid_data common;
    ulong CIDMapOffset;		/* optional, for standard glyph_data */
    gs_font_type1 **FDArray;
    uint FDArray_size;
    int FDBytes;		/* optional, for standard glyph_data */
    /*
     * The third argument of glyph_data may be NULL if only the font number
     * is wanted.  glyph_data returns 1 if the string is newly allocated
     * (using the font's allocator) and can be freed by the client.
     */
    int (*glyph_data)(P4(gs_font_base *, gs_glyph, gs_const_string *,
			 int *));
    void *proc_data;
} gs_font_cid0_data;
struct gs_font_cid0_s {
    gs_font_base_common;
    gs_font_cid0_data cidata;
};

extern_st(st_gs_font_cid0);
#define public_st_gs_font_cid0() /* in gsfcid.c */\
  gs_public_st_composite_use_final(st_gs_font_cid0,\
    gs_font_cid0, "gs_font_cid0",\
    font_cid0_enum_ptrs, font_cid0_reloc_ptrs, gs_font_finalize)
#define st_gs_font_cid0_max_ptrs\
  (st_gs_font_max_ptrs + st_gs_font_cid_data_num_ptrs + 2)

/* CIDFontType 1 doesn't reference any additional structures. */

typedef struct gs_font_cid1_data_s {
    gs_cid_system_info_t CIDSystemInfo;
} gs_font_cid1_data;
typedef struct gs_font_cid1_s {
    gs_font_base_common;
    gs_font_cid1_data cidata;
} gs_font_cid1;

extern_st(st_gs_font_cid1);
#define public_st_gs_font_cid1() /* in gsfcid.c */\
  gs_public_st_composite_use_final(st_gs_font_cid1,\
    gs_font_cid1, "gs_font_cid1",\
    font_cid1_enum_ptrs, font_cid1_reloc_ptrs, gs_font_finalize)
#define st_gs_font_cid1_max_ptrs\
  (st_gs_font_max_ptrs + st_cid_system_info_num_ptrs)

/* CIDFontType 2 is a subclass of FontType 42. */

#ifndef gs_font_cid2_DEFINED
#  define gs_font_cid2_DEFINED
typedef struct gs_font_cid2_s gs_font_cid2;
#endif
typedef struct gs_font_cid2_data_s {
    gs_font_cid_data common;
    int MetricsCount;
    int (*CIDMap_proc)(P2(gs_font_cid2 *, gs_glyph));
    /*
     * "Wrapper" get_outline and glyph_info procedures are needed, to
     * handle MetricsCount.  Save the original ones here.
     */
    struct o_ {
	int (*get_outline)(P3(gs_font_type42 *, uint, gs_const_string *));
	int (*get_metrics)(P4(gs_font_type42 *, uint, int, float [4]));
    } orig_procs;
} gs_font_cid2_data;
struct gs_font_cid2_s {
    gs_font_type42_common;
    gs_font_cid2_data cidata;
};

extern_st(st_gs_font_cid2);
#define public_st_gs_font_cid2() /* in gsfcid.c */\
  gs_public_st_composite_use_final(st_gs_font_cid2,\
    gs_font_cid2, "gs_font_cid2",\
    font_cid2_enum_ptrs, font_cid2_reloc_ptrs, gs_font_finalize)
#define st_gs_font_cid2_max_ptrs\
  (st_gs_font_type42_max_ptrs + st_gs_font_cid_data_num_ptrs)

/* ---------------- Procedures ---------------- */

/*
 * Get the CIDSystemInfo of a font.  If the font is not a CIDFont,
 * return NULL.
 */
const gs_cid_system_info_t *gs_font_cid_system_info(P1(const gs_font *));

/*
 * Provide a default enumerate_glyph procedure for CIDFontType 0 fonts.
 * Built for simplicity, not for speed.
 */
font_proc_enumerate_glyph(gs_font_cid0_enumerate_glyph);

#endif /* gxfcid_INCLUDED */
