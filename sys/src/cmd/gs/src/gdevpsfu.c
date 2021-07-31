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

/*$Id: gdevpsfu.c,v 1.4 2000/09/19 19:00:21 lpd Exp $ */
/* PostScript/PDF font writing utilities */
#include "memory_.h"
#include <stdlib.h>		/* for qsort */
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gxfont.h */
#include "gxfont.h"
#include "gdevpsf.h"

/* Begin enumerating the glyphs in a font or a font subset. */
private int
enumerate_font_next(psf_glyph_enum_t *ppge, gs_glyph *pglyph)
{
    gs_font *font = ppge->font;
    int index = (int)ppge->index;
    int code = font->procs.enumerate_glyph(font, &index,
					   ppge->glyph_space, pglyph);

    ppge->index = index;
    return (index == 0 ? 1 : code < 0 ? code : 0);
}
private int
enumerate_glyphs_next(psf_glyph_enum_t *ppge, gs_glyph *pglyph)
{
    if (ppge->index >= ppge->subset.size)
	return 1;
    *pglyph = ppge->subset.selected.list[ppge->index++];
    return 0;
}
private int
enumerate_range_next(psf_glyph_enum_t *ppge, gs_glyph *pglyph)
{
    if (ppge->index >= ppge->subset.size)
	return 1;
    *pglyph = (gs_glyph)(ppge->index++ + gs_min_cid_glyph);
    return 0;
}
void
psf_enumerate_list_begin(psf_glyph_enum_t *ppge, gs_font *font,
			 const gs_glyph *subset_list, uint subset_size,
			 gs_glyph_space_t glyph_space)
{
    ppge->font = font;
    ppge->subset.selected.list = subset_list;
    ppge->subset.size = subset_size;
    ppge->glyph_space = glyph_space;
    ppge->enumerate_next =
	(subset_list ? enumerate_glyphs_next :
	 subset_size ? enumerate_range_next : enumerate_font_next);
    psf_enumerate_glyphs_reset(ppge);
}

/* Begin enumerating CID or TT glyphs in a subset given by a bit vector. */
private int
enumerate_bits_next(psf_glyph_enum_t *ppge, gs_glyph *pglyph)
{
    for (; ppge->index < ppge->subset.size; ppge->index++)
	if (ppge->subset.selected.bits[ppge->index >> 3] & (0x80 >> (ppge->index & 7))) {
	    *pglyph = (gs_glyph)(ppge->index++ + gs_min_cid_glyph);
	    return 0;
	}
    return 1;
}
void
psf_enumerate_bits_begin(psf_glyph_enum_t *ppge, gs_font *font,
			 const byte *subset_bits, uint subset_size,
			 gs_glyph_space_t glyph_space)
{
    ppge->font = font;
    ppge->subset.selected.bits = subset_bits;
    ppge->subset.size = subset_size;
    ppge->glyph_space = glyph_space;
    ppge->enumerate_next =
	(subset_bits ? enumerate_bits_next :
	 subset_size ? enumerate_range_next : enumerate_font_next);
    psf_enumerate_glyphs_reset(ppge);
}

/* Reset a glyph enumeration. */
void
psf_enumerate_glyphs_reset(psf_glyph_enum_t *ppge)
{
    ppge->index = 0;
}

/* Enumerate the next glyph in a font or a font subset. */
/* Return 0 if more glyphs, 1 if done, <0 if error. */
int
psf_enumerate_glyphs_next(psf_glyph_enum_t *ppge, gs_glyph *pglyph)
{
    return ppge->enumerate_next(ppge, pglyph);
}

/*
 * Get the set of referenced glyphs (indices) for writing a subset font.
 * Does not sort or remove duplicates.
 */
int
psf_subset_glyphs(gs_glyph glyphs[256], gs_font *font, const byte used[32])
{
    int i, n;

    for (i = n = 0; i < 256; ++i)
	if (used[i >> 3] & (1 << (i & 7))) {
	    gs_glyph glyph = font->procs.encode_char(font, (gs_char)i,
						     GLYPH_SPACE_INDEX);

	    if (glyph != gs_no_glyph)
		glyphs[n++] = glyph;
	}
    return n;
}

/*
 * Add composite glyph pieces to a list of glyphs.  Does not sort or
 * remove duplicates.  max_pieces is the maximum number of pieces that a
 * single glyph can have: if this value is not known, the caller should
 * use max_count.
 */
int
psf_add_subset_pieces(gs_glyph *glyphs, uint *pcount, uint max_count,
		       uint max_pieces, gs_font *font)
{
    uint i;
    uint count = *pcount;

    for (i = 0; i < count; ++i) {
	gs_glyph_info_t info;
	int code;

	if (count + max_pieces > max_count) {
	    /* Check first to make sure there is enough room. */
	    code = font->procs.glyph_info(font, glyphs[i], NULL,
					  GLYPH_INFO_NUM_PIECES, &info);
	    if (code < 0)
		continue;
	    if (count + info.num_pieces > max_count)
		return_error(gs_error_rangecheck);
	}
	info.pieces = &glyphs[count];
	code = font->procs.glyph_info(font, glyphs[i], NULL,
				      GLYPH_INFO_NUM_PIECES |
				      GLYPH_INFO_PIECES, &info);
	if (code >= 0)
	    count += info.num_pieces;
    }
    *pcount = count;
    return 0;
}

/*
 * Sort a list of glyphs and remove duplicates.  Return the number of glyphs
 * in the result.
 */
private int
compare_glyphs(const void *pg1, const void *pg2)
{
    gs_glyph g1 = *(const gs_glyph *)pg1, g2 = *(const gs_glyph *)pg2;

    return (g1 < g2 ? -1 : g1 > g2 ? 1 : 0);
}
int
psf_sort_glyphs(gs_glyph *glyphs, int count)
{
    int i, n;

    qsort(glyphs, count, sizeof(*glyphs), compare_glyphs);
    for (i = n = 0; i < count; ++i)
	if (i == 0 || glyphs[i] != glyphs[i - 1])
	    glyphs[n++] = glyphs[i];
    return n;
}

/*
 * Return the index of a given glyph in a sorted list of glyphs, or -1
 * if the glyph is not present.
 */
int
psf_sorted_glyphs_index_of(const gs_glyph *glyphs, int count, gs_glyph glyph)
{
    int lo = 0, hi = count - 1;

    if (hi < 0)
	return -1;
    if (glyph < glyphs[0] || glyph > glyphs[hi])
	return -1;
    /*
     * Loop invariants: hi > lo;
     * glyphs[lo] <= glyph <= glyphs[hi].
     */
    while (hi - lo > 1) {
	int mid = (lo + hi) >> 1;

	if (glyph >= glyphs[mid])
	    lo = mid;
	else
	    hi = mid;
    }
    return (glyph == glyphs[lo] ? lo : glyph == glyphs[hi] ? hi : -1);
}
/* Determine whether a sorted list of glyphs includes a given glyph. */
bool
psf_sorted_glyphs_include(const gs_glyph *glyphs, int count, gs_glyph glyph)
{
    return psf_sorted_glyphs_index_of(glyphs, count, glyph) >= 0;
}

/* Check that all selected glyphs can be written. */
int
psf_check_outline_glyphs(gs_font_base *pfont, psf_glyph_enum_t *ppge,
			 glyph_data_proc_t glyph_data)
{
    uint members = GLYPH_INFO_WIDTH0 << pfont->WMode;
    gs_glyph glyph;
    int code;

    while ((code = psf_enumerate_glyphs_next(ppge, &glyph)) != 1) {
	gs_const_string gdata;
	gs_font_type1 *ignore_font;
	gs_glyph_info_t info;

	if (code < 0)
	    return code;
	code = glyph_data(pfont, glyph, &gdata, &ignore_font);
	/*
	 * If the glyph isn't defined by a CharString, glyph_data will
	 * return a typecheck error.  But if there's merely a glyph in
	 * in the Encoding that isn't defined, glyph_data will return an
	 * undefined error, which is OK.
	 */
	if (code < 0) {
	    if (code == gs_error_undefined)
		continue;
	    return code;
	}
	/*
	 * If the font has a CDevProc or calls a non-standard OtherSubr,
	 * glyph_info will return a rangecheck error.
	 */
	code = pfont->procs.glyph_info((gs_font *)pfont, glyph, NULL,
				       members, &info);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* Gather glyph information for a Type 1 or Type 2 font. */
int
psf_get_outline_glyphs(psf_outline_glyphs_t *pglyphs, gs_font_base *pfont,
		       gs_glyph *orig_subset_glyphs, uint orig_subset_size,
		       glyph_data_proc_t glyph_data)
{
    gs_glyph notdef = gs_no_glyph;
    gs_glyph *subset_glyphs = orig_subset_glyphs;
    uint subset_size = orig_subset_size;

    if (subset_glyphs) {
	if (subset_size > countof(pglyphs->subset_data))
	    return_error(gs_error_limitcheck);
	memcpy(pglyphs->subset_data, orig_subset_glyphs,
	       sizeof(gs_glyph) * subset_size);
	subset_glyphs = pglyphs->subset_data;
    }

    {
	/*
	 * Make sure that this font can be written out.  Specifically, it
	 * must have no CharStrings defined by PostScript procedures, no
	 * non-standard OtherSubrs, and no CDevProc.
	 */
	psf_glyph_enum_t genum;
	int code;

	psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont, subset_glyphs,
				    (subset_glyphs ? subset_size : 0),
				    GLYPH_SPACE_NAME);
	code = psf_check_outline_glyphs(pfont, &genum, glyph_data);
	if (code < 0)
	    return code;
    }

    {
	/*
	 * Detect the .notdef glyph, needed for subset fonts and to
	 * eliminate unnecessary Encoding assignments.
	 */
	psf_glyph_enum_t genum;
	gs_glyph glyph;
	int code;

	psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont, NULL, 0,
				    GLYPH_SPACE_NAME);
	while ((code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1) {
	    uint namelen;
	    const char *namestr =
		(*pfont->procs.callbacks.glyph_name)(glyph, &namelen);

	    if (namestr && namelen == 7 && !memcmp(namestr, ".notdef", 7)) {
		notdef = glyph;
		break;
	    }
	}
    }

    if (subset_glyphs) {
	/*
	 * For subset fonts, we must ensure that characters referenced
	 * by seac are also included.  Note that seac creates at most
	 * 2 pieces.
	 */
	int code = psf_add_subset_pieces(subset_glyphs, &subset_size,
					  countof(pglyphs->subset_data) - 1, 2,
					  (gs_font *)pfont);

	if (code < 0)
	    return code;
	/* Subset fonts require .notdef. */
	if (notdef == gs_no_glyph)
	    return_error(gs_error_rangecheck);
	/* Sort the glyphs now.  Make sure .notdef is included. */
	subset_glyphs[subset_size++] = notdef;
	subset_size = psf_sort_glyphs(subset_glyphs, subset_size);
    }

    pglyphs->notdef = notdef;
    pglyphs->subset_glyphs = subset_glyphs;
    pglyphs->subset_size = subset_size;
    return 0;
}
