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

/*$Id: gdevpsf.h,v 1.7.2.1 2000/11/09 20:37:29 rayjj Exp $ */
/* PostScript/PDF font writing interface */

#ifndef gdevpsf_INCLUDED
#  define gdevpsf_INCLUDED

#include "gsccode.h"

/* ---------------- Embedded font writing ---------------- */

#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif
#ifndef gs_font_base_DEFINED
#  define gs_font_base_DEFINED
typedef struct gs_font_base_s gs_font_base;
#endif

/*
 * Define the structure used for enumerating the glyphs in a font or a
 * font subset.  This type is opaque to clients: we declare it here only
 * so that clients can allocate it on the stack.
 */
typedef struct psf_glyph_enum_s psf_glyph_enum_t;
struct psf_glyph_enum_s {
    gs_font *font;
    struct su_ {
	union sus_ {
	    const gs_glyph *list;	/* if subset given by a list */
	    const byte *bits;	/* if CID or TT subset given by a bitmap */
	} selected;
	uint size;
    } subset;
    gs_glyph_space_t glyph_space;
    ulong index;
    int (*enumerate_next)(P2(psf_glyph_enum_t *, gs_glyph *));
};

/*
 * Begin enumerating the glyphs in a font or a font subset.  If subset_size
 * > 0 but subset_glyphs == 0, enumerate all glyphs in [0 .. subset_size-1]
 * (as integer glyphs, i.e., offset by gs_min_cid_glyph).
 */
void psf_enumerate_list_begin(P5(psf_glyph_enum_t *ppge, gs_font *font,
				 const gs_glyph *subset_list,
				 uint subset_size,
				 gs_glyph_space_t glyph_space));
/* Backward compatibility */
#define psf_enumerate_glyphs_begin psf_enumerate_list_begin

/* Begin enumerating CID or TT glyphs in a subset given by a bit vector. */
/* Note that subset_size is given in bits, not in bytes. */
void psf_enumerate_bits_begin(P5(psf_glyph_enum_t *ppge, gs_font *font,
				 const byte *subset_bits, uint subset_size,
				 gs_glyph_space_t glyph_space));
/* Backward compatibility */
#define psf_enumerate_cids_begin(ppge, font, bits, size)\
   psf_enumerate_bits_begin(ppge, font, bits, size, GLYPH_SPACE_NAME)

/*
 * Reset a glyph enumeration.
 */
void psf_enumerate_glyphs_reset(P1(psf_glyph_enum_t *ppge));

/*
 * Enumerate the next glyph in a font or a font subset.
 * Return 0 if more glyphs, 1 if done, <0 if error.
 */
int psf_enumerate_glyphs_next(P2(psf_glyph_enum_t *ppge, gs_glyph *pglyph));

/*
 * Get the set of referenced glyphs (indices) for writing a subset font.
 * Does not sort or remove duplicates.
 */
int psf_subset_glyphs(P3(gs_glyph glyphs[256], gs_font *font,
			 const byte used[32]));

/*
 * Add composite glyph pieces to a list of glyphs.  Does not sort or
 * remove duplicates.  max_pieces is the maximum number of pieces that a
 * single glyph can have: if this value is not known, the caller should
 * use max_count.
 */
int psf_add_subset_pieces(P5(gs_glyph *glyphs, uint *pcount, uint max_count,
			     uint max_pieces, gs_font *font));

/*
 * Sort a list of glyphs and remove duplicates.  Return the number of glyphs
 * in the result.
 */
int psf_sort_glyphs(P2(gs_glyph *glyphs, int count));

/*
 * Return the index of a given glyph in a sorted list of glyphs, or -1
 * if the glyph is not present.
 */
int psf_sorted_glyphs_index_of(P3(const gs_glyph *glyphs, int count,
				  gs_glyph glyph));
/*
 * Determine whether a sorted list of glyphs includes a given glyph.
 */
bool psf_sorted_glyphs_include(P3(const gs_glyph *glyphs, int count,
				  gs_glyph glyph));

/*
 * Define the internal structure that holds glyph information for an
 * outline-based font to be written.  Currently this only applies to
 * Type 1, Type 2, and CIDFontType 0 fonts, but someday it might also
 * be usable with TrueType (Type 42) and CIDFontType 2 fonts.
 */
typedef struct psf_outline_glyphs_s {
    gs_glyph notdef;
    gs_glyph subset_data[256 * 3 + 1]; /* *3 for seac, +1 for .notdef */
    gs_glyph *subset_glyphs;	/* 0 or subset_data */
    uint subset_size;
} psf_outline_glyphs_t;

#ifndef gs_font_type1_DEFINED
#  define gs_font_type1_DEFINED
typedef struct gs_font_type1_s gs_font_type1;
#endif

/* Define the type for the glyph data callback procedure. */
typedef int (*glyph_data_proc_t)(P4(gs_font_base *, gs_glyph,
				    gs_const_string *, gs_font_type1 **));

/* Check that all selected glyphs can be written. */
int psf_check_outline_glyphs(P3(gs_font_base *pfont,
				psf_glyph_enum_t *ppge,
				glyph_data_proc_t glyph_data));

/*
 * Gather glyph information for a Type 1, Type 2, or CIDFontType 0 font.
 * Note that the glyph_data procedure returns both the outline string and
 * a gs_font_type1 (Type 1 or Type 2) font: for Type 1 or Type 2 fonts,
 * this is the original font, but for CIDFontType 0 fonts, it is the
 * FDArray element.
 */
int psf_get_outline_glyphs(P5(psf_outline_glyphs_t *pglyphs,
			      gs_font_base *pfont, gs_glyph *subset_glyphs,
			      uint subset_size, glyph_data_proc_t glyph_data));

/* ------ Exported by gdevpsf1.c ------ */

/* Gather glyph information for a Type 1 or Type 2 font. */
int psf_type1_glyph_data(P4(gs_font_base *, gs_glyph, gs_const_string *,
			    gs_font_type1 **));
int psf_get_type1_glyphs(P4(psf_outline_glyphs_t *pglyphs,
			    gs_font_type1 *pfont,
			    gs_glyph *subset_glyphs, uint subset_size));

/*
 * Write out a Type 1 font definition.  This procedure does not allocate
 * or free any data.
 */
#define WRITE_TYPE1_EEXEC 1
#define WRITE_TYPE1_ASCIIHEX 2  /* use ASCII hex rather than binary */
#define WRITE_TYPE1_EEXEC_PAD 4  /* add 512 0s */
#define WRITE_TYPE1_EEXEC_MARK 8  /* assume 512 0s will be added */
#define WRITE_TYPE1_POSTSCRIPT 16  /* don't observe ATM restrictions */
#define WRITE_TYPE1_WITH_LENIV 32  /* don't allow lenIV = -1 */
int psf_write_type1_font(P7(stream *s, gs_font_type1 *pfont, int options,
			    gs_glyph *subset_glyphs, uint subset_size,
			    const gs_const_string *alt_font_name,
			    int lengths[3]));


/* ------ Exported by gdevpsf2.c ------ */

/*
 * Write out a Type 1 or Type 2 font definition as CFF.
 * This procedure does not allocate or free any data.
 */
#define WRITE_TYPE2_NO_LENIV 1	/* always use lenIV = -1 */
#define WRITE_TYPE2_CHARSTRINGS 2 /* convert T1 charstrings to T2 */
#define WRITE_TYPE2_AR3 4	/* work around bugs in Acrobat Reader 3 */
int psf_write_type2_font(P6(stream *s, gs_font_type1 *pfont, int options,
			    gs_glyph *subset_glyphs, uint subset_size,
			    const gs_const_string *alt_font_name));

#ifndef gs_font_cid0_DEFINED
#  define gs_font_cid0_DEFINED
typedef struct gs_font_cid0_s gs_font_cid0;
#endif

/*
 * Write out a CIDFontType 0 font definition as CFF.  The options are
 * the same as for psf_write_type2_font.  subset_cids is a bit vector of
 * subset_size bits (not bytes).
 * This procedure does not allocate or free any data.
 */
int psf_write_cid0_font(P6(stream *s, gs_font_cid0 *pfont, int options,
			   const byte *subset_cids, uint subset_size,
			   const gs_const_string *alt_font_name));

/* ------ Exported by gdevpsfm.c ------ */

/*
 * Write out a CMap in its customary (source) form.
 */
#ifndef gs_cmap_DEFINED
#  define gs_cmap_DEFINED
typedef struct gs_cmap_s gs_cmap_t;
#endif
typedef int (*psf_put_name_proc_t)(P3(stream *, const byte *, uint));
int psf_write_cmap(P4(stream *s, const gs_cmap_t *pcmap,
		      psf_put_name_proc_t put_name,
		      const gs_const_string *alt_cmap_name));

/* ------ Exported by gdevpsft.c ------ */

/*
 * Write out a TrueType (Type 42) font definition.
 * This procedure does not allocate or free any data.
 */
#ifndef gs_font_type42_DEFINED
#  define gs_font_type42_DEFINED
typedef struct gs_font_type42_s gs_font_type42;
#endif
#define WRITE_TRUETYPE_CMAP 1	/* generate cmap from the Encoding */
#define WRITE_TRUETYPE_NAME 2	/* generate name if missing */
#define WRITE_TRUETYPE_POST 4	/* generate post if missing */
#define WRITE_TRUETYPE_NO_TRIMMED_TABLE 8  /* not OK to use cmap format 6 */
int psf_write_truetype_font(P6(stream *s, gs_font_type42 *pfont, int options,
			       gs_glyph *subset_glyphs, uint subset_size,
			       const gs_const_string *alt_font_name));

#ifndef gs_font_cid2_DEFINED
#  define gs_font_cid2_DEFINED
typedef struct gs_font_cid2_s gs_font_cid2;
#endif

/*
 * Write out a CIDFontType 2 font definition.  This procedure is identical
 * to psf_write_truetype_font except that the subset, if any, is specified
 * as a bit vector (as for psf_write_cid0_font) rather than a list of glyphs.
 * NOTE: it is the client's responsibility to ensure that if the subset
 * contains any composite glyphs, the components of the composites are
 * included explicitly in the subset.
 * This procedure does not allocate or free any data.
 */
int psf_write_cid2_font(P6(stream *s, gs_font_cid2 *pfont, int options,
			   const byte *subset_glyphs, uint subset_size,
			   const gs_const_string *alt_font_name));

/* ------ Exported by gdevpsfx.c ------ */

/*
 * Convert a Type 1 CharString to (unencrypted) Type 2.
 * This procedure does not allocate or free any data.
 * NOTE: this procedure expands all Subrs in-line.
 */
int psf_convert_type1_to_type2(P3(stream *s, const gs_const_string *pstr,
				  gs_font_type1 *pfont));

#endif /* gdevpsf_INCLUDED */
