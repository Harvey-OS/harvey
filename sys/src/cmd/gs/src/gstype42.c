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

/* $Id: gstype42.c,v 1.51 2005/03/15 11:36:37 igor Exp $ */
/* Type 42 (TrueType) font library routines */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsccode.h"
#include "gsline.h"		/* for gs_imager_setflat */
#include "gsmatrix.h"
#include "gsutil.h"
#include "gxchrout.h"
#include "gxfixed.h"		/* for gxpath.h */
#include "gxpath.h"
#include "gxfont.h"
#include "gxfont42.h"
#include "gxttf.h"
#include "gxttfb.h"
#include "gxfcache.h"
#include "gxistate.h"
#include "stream.h"

/* Structure descriptor */
public_st_gs_font_type42();

/* Forward references */
private int append_outline_fitted(uint glyph_index, const gs_matrix * pmat,
	       gx_path * ppath, cached_fm_pair * pair, 
	       const gs_log2_scale_point * pscale, bool design_grid);
private uint default_get_glyph_index(gs_font_type42 *pfont, gs_glyph glyph);
private int default_get_outline(gs_font_type42 *pfont, uint glyph_index,
				gs_glyph_data_t *pgd);

/* Set up a pointer to a substring of the font data. */
/* Free variables: pfont, string_proc. */
#define ACCESS(base, length, vptr)\
  BEGIN\
    code = (*string_proc)(pfont, (ulong)(base), length, &vptr);\
    if ( code < 0 ) return code;\
    if ( code > 0 ) return_error(gs_error_invalidfont);\
  END

/* Get 2- or 4-byte quantities from a table. */
#define U8(p) ((uint)((p)[0]))
#define S8(p) (int)((U8(p) ^ 0x80) - 0x80)
#define U16(p) (((uint)((p)[0]) << 8) + (p)[1])
#define S16(p) (int)((U16(p) ^ 0x8000) - 0x8000)
#define u32(p) get_u32_msb(p)

/* ---------------- Font level ---------------- */

GS_NOTIFY_PROC(gs_len_glyphs_release);

/* Get the offset to a glyph using the loca table */
private inline ulong
get_glyph_offset(gs_font_type42 *pfont, uint glyph_index) 
{
    int (*string_proc)(gs_font_type42 *, ulong, uint, const byte **) =
	pfont->data.string_proc;
    const byte *ploca;
    ulong result;
    int code;		/* hidden variable used by ACCESS */

    if (pfont->data.indexToLocFormat) {
	ACCESS(pfont->data.loca + glyph_index * 4, 4, ploca);
	result = u32(ploca);
    } else {
	ACCESS(pfont->data.loca + glyph_index * 2, 2, ploca);
	result = (ulong) U16(ploca) << 1;
    }
    return result;
}

/*
 * Initialize the cached values in a Type 42 font.
 * Note that this initializes the type42_data procedures other than
 * string_proc, and the font procedures as well.
 */
int
gs_type42_font_init(gs_font_type42 * pfont)
{
    int (*string_proc)(gs_font_type42 *, ulong, uint, const byte **) =
	pfont->data.string_proc;
    const byte *OffsetTable;
    uint numTables;
    const byte *TableDirectory;
    uint i;
    int code;
    byte head_box[8];
    ulong loca_size = 0;
    ulong glyph_start, glyph_offset, glyph_length;

    ACCESS(0, 12, OffsetTable);
    {
	static const byte version1_0[4] = {0, 1, 0, 0};
	static const byte version_true[4] = {'t', 'r', 'u', 'e'};

	if (memcmp(OffsetTable, version1_0, 4) &&
	    memcmp(OffsetTable, version_true, 4))
	    return_error(gs_error_invalidfont);
    }
    numTables = U16(OffsetTable + 4);
    ACCESS(12, numTables * 16, TableDirectory);
    /* Clear all non-client-supplied data. */
    {
	void *proc_data = pfont->data.proc_data;

	memset(&pfont->data, 0, sizeof(pfont->data));
	pfont->data.string_proc = string_proc;
	pfont->data.proc_data = proc_data;
    }
    for (i = 0; i < numTables; ++i) {
	const byte *tab = TableDirectory + i * 16;
	ulong offset = u32(tab + 8);

	if (!memcmp(tab, "cmap", 4))
	    pfont->data.cmap = offset;
	else if (!memcmp(tab, "glyf", 4))
	    pfont->data.glyf = offset;
	else if (!memcmp(tab, "head", 4)) {
	    const byte *head;

	    ACCESS(offset, 54, head);
	    pfont->data.unitsPerEm = U16(head + 18);
	    memcpy(head_box, head + 36, 8);
	    pfont->data.indexToLocFormat = U16(head + 50);
	} else if (!memcmp(tab, "hhea", 4)) {
	    const byte *hhea;

	    ACCESS(offset, 36, hhea);
	    pfont->data.metrics[0].numMetrics = U16(hhea + 34);
	} else if (!memcmp(tab, "hmtx", 4)) {
	    pfont->data.metrics[0].offset = offset;
	    pfont->data.metrics[0].length = (uint)u32(tab + 12);
	} else if (!memcmp(tab, "loca", 4)) {
	    pfont->data.loca = offset;
	    loca_size = u32(tab + 12);
	} else if (!memcmp(tab, "maxp", 4)) {
	    const byte *maxp;

	    ACCESS(offset, 30, maxp);
	    pfont->data.trueNumGlyphs = U16(maxp + 4);
	} else if (!memcmp(tab, "vhea", 4)) {
	    const byte *vhea;

	    ACCESS(offset, 36, vhea);
	    pfont->data.metrics[1].numMetrics = U16(vhea + 34);
	} else if (!memcmp(tab, "vmtx", 4)) {
	    pfont->data.metrics[1].offset = offset;
	    pfont->data.metrics[1].length = (uint)u32(tab + 12);
	}
    }
    loca_size >>= pfont->data.indexToLocFormat + 1;
    pfont->data.numGlyphs = (loca_size == 0 ? 0 : loca_size - 1);
    /* Now build the len_glyphs array since 'loca' may not be properly sorted */
    pfont->data.len_glyphs = (uint *)gs_alloc_bytes(pfont->memory, pfont->data.numGlyphs * sizeof(uint),
	    "gs_type42_font");
    if (pfont->data.len_glyphs == 0)
	return_error(gs_error_VMerror);
    gs_font_notify_register((gs_font *)pfont, gs_len_glyphs_release, (void *)pfont);
 
    /* The 'loca' may not be in order, so we construct a glyph length array */
    /* Since 'loca' is usually sorted, first try the simple linear scan to  */
    /* avoid the need to perform the more expensive process. */
    glyph_start = get_glyph_offset(pfont, 0);
    for (i=1; i < loca_size; i++) {
	glyph_offset = get_glyph_offset(pfont, i);
	glyph_length = glyph_offset - glyph_start;
	if (glyph_length > 0x80000000)
	    break;				/* out of order loca */
	pfont->data.len_glyphs[i-1] = glyph_length;
	glyph_start = glyph_offset;
    }
    if (i < loca_size) {
        uint j;
	ulong trial_glyph_length;
        /*
         * loca was out of order, build the len_glyphs the hard way      
	 * Assume that some of the len_glyphs built so far may be wrong 
	 * For each starting offset, find the next higher ending offset
	 * Note that doing this means that there can only be zero length
	 * glyphs that have loca table offset equal to the last 'dummy'
         * entry. Otherwise we assume that it is a duplicated entry.
	 */
	for (i=0; i < loca_size-1; i++) {
	    glyph_start = get_glyph_offset(pfont, i);
	    for (j=1, glyph_length = 0x80000000; j<loca_size; j++) {
		glyph_offset = get_glyph_offset(pfont, j);
		trial_glyph_length = glyph_offset - glyph_start;
		if ((trial_glyph_length > 0) && (trial_glyph_length < glyph_length))
		    glyph_length = trial_glyph_length;
	    }
	    pfont->data.len_glyphs[i] = glyph_length < 0x80000000 ? glyph_length : 0;
	}
    }
    /*
     * If the font doesn't have a valid FontBBox, compute one from the
     * 'head' information.  Since the Adobe PostScript driver sometimes
     * outputs garbage FontBBox values, we use a "reasonableness" check
     * here.
     */
    if (pfont->FontBBox.p.x >= pfont->FontBBox.q.x ||
	pfont->FontBBox.p.y >= pfont->FontBBox.q.y ||
	pfont->FontBBox.p.x < -0.5 || pfont->FontBBox.p.x > 0.5 ||
	pfont->FontBBox.p.y < -0.5 || pfont->FontBBox.p.y > 0.5
	) {
	float upem = (float)pfont->data.unitsPerEm;

	pfont->FontBBox.p.x = S16(head_box) / upem;
	pfont->FontBBox.p.y = S16(head_box + 2) / upem;
	pfont->FontBBox.q.x = S16(head_box + 4) / upem;
	pfont->FontBBox.q.y = S16(head_box + 6) / upem;
    }
    pfont->data.warning_patented = false;
    pfont->data.warning_bad_instruction = false;
    pfont->data.get_glyph_index = default_get_glyph_index;
    pfont->data.get_outline = default_get_outline;
    pfont->data.get_metrics = gs_type42_default_get_metrics;
    pfont->procs.glyph_outline = gs_type42_glyph_outline;
    pfont->procs.glyph_info = gs_type42_glyph_info;
    pfont->procs.enumerate_glyph = gs_type42_enumerate_glyph;
    return 0;
}

int
gs_len_glyphs_release(void *data, void *event)
{   
    gs_font_type42 *pfont = (gs_font_type42 *)data;

    gs_font_notify_unregister((gs_font *)pfont, gs_len_glyphs_release, (void *)data);
    gs_free_object(pfont->memory, pfont->data.len_glyphs, "gs_len_glyphs_release");
    return 0;
}

/* ---------------- Glyph level ---------------- */

/*
 * Parse the definition of one component of a composite glyph.  We don't
 * bother to parse the component index, since the caller can do this so
 * easily.
 */
private void
parse_component(const byte **pdata, uint *pflags, gs_matrix_fixed *psmat,
		int *pmp /*[2], may be null*/, const gs_font_type42 *pfont,
		const gs_matrix_fixed *pmat)
{
    const byte *gdata = *pdata;
    uint flags;
    double factor = 1.0 / pfont->data.unitsPerEm;
    gs_matrix_fixed mat;
    gs_matrix scale_mat;

    flags = U16(gdata);
    gdata += 4;
    mat = *pmat;
    if (flags & TT_CG_ARGS_ARE_XY_VALUES) {
	int arg1, arg2;
	gs_fixed_point pt;

	if (flags & TT_CG_ARGS_ARE_WORDS)
	    arg1 = S16(gdata), arg2 = S16(gdata + 2), gdata += 4;
	else
	    arg1 = S8(gdata), arg2 = S8(gdata + 1), gdata += 2;
	if (flags & TT_CG_ROUND_XY_TO_GRID) {
	    /* We should do something here, but we don't. */
	}
	gs_point_transform2fixed(pmat, arg1 * factor,
				 arg2 * factor, &pt);
	/****** HACK: WE KNOW ABOUT FIXED MATRICES ******/
	mat.tx = fixed2float(mat.tx_fixed = pt.x);
	mat.ty = fixed2float(mat.ty_fixed = pt.y);
	if (pmp)
	    pmp[0] = pmp[1] = -1;
    } else {
	if (flags & TT_CG_ARGS_ARE_WORDS) {
	    if (pmp)
		pmp[0] = U16(gdata), pmp[1] = S16(gdata + 2);
	    gdata += 4;
	} else {
	    if (pmp)
		pmp[0] = U8(gdata), pmp[1] = U8(gdata + 1);
	    gdata += 2;
	}
    }
#define S2_14(p) (S16(p) / 16384.0)
    if (flags & TT_CG_HAVE_SCALE) {
	scale_mat.xx = scale_mat.yy = S2_14(gdata);
	scale_mat.xy = scale_mat.yx = 0;
	gdata += 2;
    } else if (flags & TT_CG_HAVE_XY_SCALE) {
	scale_mat.xx = S2_14(gdata);
	scale_mat.yy = S2_14(gdata + 2);
	scale_mat.xy = scale_mat.yx = 0;
	gdata += 4;
    } else if (flags & TT_CG_HAVE_2X2) {
	scale_mat.xx = S2_14(gdata);
	scale_mat.xy = S2_14(gdata + 2);
	scale_mat.yx = S2_14(gdata + 4);
	scale_mat.yy = S2_14(gdata + 6);
	gdata += 8;
    } else
	goto no_scale;
#undef S2_14
    scale_mat.tx = 0;
    scale_mat.ty = 0;
    /* The scale doesn't affect mat.t{x,y}, so we don't */
    /* need to update the fixed components. */
    gs_matrix_multiply(&scale_mat, (const gs_matrix *)&mat,
		       (gs_matrix *)&mat);
no_scale:
    *pdata = gdata;
    *pflags = flags;
    *psmat = mat;
}

/* Compute the total number of points in a (possibly composite) glyph. */
private int
total_points(gs_font_type42 *pfont, uint glyph_index)
{
    gs_glyph_data_t glyph_data;
    int code;
    int ocode;
    const byte *gdata;
    int total;

    glyph_data.memory = pfont->memory;
    ocode = pfont->data.get_outline(pfont, glyph_index, &glyph_data);
    if (ocode < 0)
	return ocode;
    if (glyph_data.bits.size == 0)
	return 0;
    gdata = glyph_data.bits.data;
    if (S16(gdata) != -1) {
	/* This is a simple glyph. */
	int numContours = S16(gdata);
	const byte *pends = gdata + 10;
	const byte *pinstr = pends + numContours * 2;

	total = (numContours == 0 ? 0 : U16(pinstr - 2) + 1);
    } else {
	/* This is a composite glyph.  Add up the components. */
	uint flags;
	gs_matrix_fixed mat;

	gdata += 10;
	memset(&mat, 0, sizeof(mat)); /* arbitrary */
	total = 0;
	do {
	    code = total_points(pfont, U16(gdata + 2));
	    if (code < 0)
		return code;
	    total += code;
	    parse_component(&gdata, &flags, &mat, NULL, pfont, &mat);
	}
	while (flags & TT_CG_MORE_COMPONENTS);
    }
    gs_glyph_data_free(&glyph_data, "total_points");
    return total;
}

/*
 * Define the default implementation for getting the glyph index from a
 * gs_glyph.  This is trivial for integer ("CID" but representing a GID)
 * gs_glyph values, and not implemented for name glyphs.
 */
private uint
default_get_glyph_index(gs_font_type42 *pfont, gs_glyph glyph)
{
    return (glyph < GS_MIN_CID_GLYPH ? 0 : /* undefined */
	    glyph - GS_MIN_CID_GLYPH);
}

/* Define the default implementation for getting the outline data for */
/* a glyph, using indexToLocFormat and the loca and glyf tables. */
/* Set pglyph->data = 0 if the glyph is empty. */
private int
default_get_outline(gs_font_type42 * pfont, uint glyph_index,
		    gs_glyph_data_t *pgd)
{
    int (*string_proc) (gs_font_type42 *, ulong, uint, const byte **) =
	pfont->data.string_proc;
    ulong glyph_start;
    uint glyph_length;
    int code;

    glyph_start = get_glyph_offset(pfont, glyph_index);
    glyph_length = pfont->data.len_glyphs[glyph_index];
    if (glyph_length == 0)
	gs_glyph_data_from_null(pgd);
    else {
	const byte *data;
	byte *buf;

	code = (*string_proc)(pfont, (ulong)(pfont->data.glyf + glyph_start),
			      glyph_length, &data);
	if (code < 0) 
	    return code;
	if (code == 0)
	    gs_glyph_data_from_string(pgd, data, glyph_length, NULL);
	else {
	    /*
	     * The glyph is segmented in sfnts. 
	     * It is not allowed with Type 42 specification.
	     * Perhaps we can handle it (with a low performance),
	     * making a contiguous copy.
	     */
	    uint left = glyph_length;

	    /* 'code' is the returned length */
	    buf = (byte *)gs_alloc_string(pgd->memory, glyph_length, "default_get_outline");
	    if (buf == 0)
		return_error(gs_error_VMerror);
	    gs_glyph_data_from_string(pgd, buf, glyph_length, (gs_font *)pfont);
	    for (;;) {
		memcpy(buf + glyph_length - left, data, code);
		if (!(left -= code))
		    return 0;
		code = (*string_proc)(pfont, (ulong)(pfont->data.glyf + glyph_start + 
		              glyph_length - left), left, &data);
		if (code < 0) 
		    return code;
		if (code == 0) 
		    code = left;
	    }
	}
    }
    return 0;
}

/* Take outline data from a True Type font file. */
int
gs_type42_get_outline_from_TT_file(gs_font_type42 * pfont, stream *s, uint glyph_index,
		gs_glyph_data_t *pgd)
{
    uchar lbuf[8];
    ulong glyph_start;
    uint glyph_length, count;

    /* fixme: Since this function is being called multiple times per glyph,
     * we should cache glyph data in a buffer associated with the font.
     */
    if (pfont->data.indexToLocFormat) {
	sseek(s, pfont->data.loca + glyph_index * 4);
        sgets(s, lbuf, 8, &count);
	if (count < 8)
	    return_error(gs_error_invalidfont);
	glyph_start = u32(lbuf);
	glyph_length = u32(lbuf + 4) - glyph_start;
    } else {
	sseek(s, pfont->data.loca + glyph_index * 2);
        sgets(s, lbuf, 4, &count);
	if (count < 4)
	    return_error(gs_error_invalidfont);
	glyph_start = (ulong) U16(lbuf) << 1;
	glyph_length = ((ulong) U16(lbuf + 2) << 1) - glyph_start;
    }
    if (glyph_length == 0)
	gs_glyph_data_from_null(pgd);
    else {
	byte *buf;

	sseek(s, pfont->data.glyf + glyph_start);
	buf = (byte *)gs_alloc_string(pgd->memory, glyph_length, "default_get_outline");
	if (buf == 0)
	    return_error(gs_error_VMerror);
	gs_glyph_data_from_string(pgd, buf, glyph_length, (gs_font *)pfont);
        sgets(s, buf, glyph_length, &count);
	if (count < glyph_length)
	    return_error(gs_error_invalidfont);
    }
    return 0;
}

/* Parse a glyph into pieces, if any. */
private int
parse_pieces(gs_font_type42 *pfont, gs_glyph glyph, gs_glyph *pieces,
	     int *pnum_pieces)
{
    uint glyph_index = (glyph >= GS_MIN_GLYPH_INDEX 
			? glyph - GS_MIN_GLYPH_INDEX 
			: pfont->data.get_glyph_index(pfont, glyph));
    gs_glyph_data_t glyph_data;
    int code;

    glyph_data.memory = pfont->memory;
    code = pfont->data.get_outline(pfont, glyph_index, &glyph_data);
    if (code < 0)
	return code;
    if (glyph_data.bits.size != 0 && S16(glyph_data.bits.data) == -1) {
	/* This is a composite glyph. */
	int i = 0;
	uint flags = TT_CG_MORE_COMPONENTS;
	const byte *gdata = glyph_data.bits.data + 10;
	gs_matrix_fixed mat;

	memset(&mat, 0, sizeof(mat)); /* arbitrary */
	for (i = 0; flags & TT_CG_MORE_COMPONENTS; ++i) {
	    if (pieces)
		pieces[i] = U16(gdata + 2) + GS_MIN_GLYPH_INDEX;
	    parse_component(&gdata, &flags, &mat, NULL, pfont, &mat);
	}
	*pnum_pieces = i;
    } else
	*pnum_pieces = 0;
    gs_glyph_data_free(&glyph_data, "parse_pieces");
    return 0;
}

/* Define the font procedures for a Type 42 font. */
int
gs_type42_glyph_outline(gs_font *font, int WMode, gs_glyph glyph, const gs_matrix *pmat,
			gx_path *ppath, double sbw[4])
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;
    uint glyph_index = (glyph >= GS_MIN_GLYPH_INDEX 
		? glyph - GS_MIN_GLYPH_INDEX 
		: pfont->data.get_glyph_index(pfont, glyph));
    gs_fixed_point origin;
    int code;
    gs_glyph_info_t info;
    static const gs_matrix imat = { identity_matrix_body };
    bool design_grid = true;
    const gs_log2_scale_point log2_scale = {0, 0}; 
    /* fixme : The subpixel numbers doesn't pass through the font_proc_glyph_outline interface.
       High level devices can't get a proper grid fitting with AlignToPixels = 1.
       Currently font_proc_glyph_outline is only used by pdfwrite for computing a
       character bbox, which doesn't need a grid fitting.
       We apply design grid here.
     */
    cached_fm_pair *pair;
    code = gx_lookup_fm_pair(font, pmat, &log2_scale, design_grid, &pair);

    if (code < 0)
	return code;
    if (pmat == 0)
	pmat = &imat;
    if ((code = gx_path_current_point(ppath, &origin)) < 0 ||
	(code = append_outline_fitted(glyph_index, pmat, ppath, pair, 
					&log2_scale, design_grid)) < 0 ||
	(code = font->procs.glyph_info(font, glyph, pmat,
				       GLYPH_INFO_WIDTH0 << WMode, &info)) < 0
	)
	return code;
    return gx_path_add_point(ppath, origin.x + float2fixed(info.width[WMode].x),
			     origin.y + float2fixed(info.width[WMode].y));
}

/* Get glyph info by glyph index. */
int
gs_type42_glyph_info_by_gid(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		     int members, gs_glyph_info_t *info, uint glyph_index)
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;
    int default_members =
	members & ~(GLYPH_INFO_WIDTHS | GLYPH_INFO_NUM_PIECES |
		    GLYPH_INFO_PIECES | GLYPH_INFO_OUTLINE_WIDTHS |
		    GLYPH_INFO_VVECTOR0 | GLYPH_INFO_VVECTOR1);
    gs_glyph_data_t outline;
    int code = 0;

    outline.memory = pfont->memory;
    if (default_members) {
	code = gs_default_glyph_info(font, glyph, pmat, default_members, info);

	if (code < 0)
	    return code;
    } else if ((code = pfont->data.get_outline(pfont, glyph_index, &outline)) < 0)
	return code;		/* non-existent glyph */
    else {
	gs_glyph_data_free(&outline, "gs_type42_glyph_info");
	info->members = 0;
    }
    if (members & GLYPH_INFO_WIDTHS) {
	int i;

	for (i = 0; i < 2; ++i)
	    if (members & (GLYPH_INFO_WIDTH0 << i)) {
		float sbw[4];

		code = gs_type42_wmode_metrics(pfont, glyph_index, i, sbw);
		if (code < 0) {
		    code = 0;
		    continue;
		}
		if (pmat) {
		    code = gs_point_transform(sbw[2], sbw[3], pmat,
					      &info->width[i]);
		    if (code < 0)
			return code;
		    code = gs_point_transform(sbw[0], sbw[1], pmat,
					      &info->v);
		} else {
		    info->width[i].x = sbw[2], info->width[i].y = sbw[3];
		    info->v.x = sbw[0], info->v.y = sbw[1];
		}
		info->members |= (GLYPH_INFO_VVECTOR0 << i);
		info->members |= (GLYPH_INFO_WIDTH << i);
	    }
	
    }
    if (members & (GLYPH_INFO_NUM_PIECES | GLYPH_INFO_PIECES)) {
	gs_glyph *pieces =
	    (members & GLYPH_INFO_PIECES ? info->pieces : (gs_glyph *)0);
	int code = parse_pieces(pfont, glyph, pieces, &info->num_pieces);

	if (code < 0)
	    return code;
	info->members |= members & (GLYPH_INFO_NUM_PIECES | GLYPH_INFO_PIECES);
    }
    return code;
}
int
gs_type42_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		     int members, gs_glyph_info_t *info)
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;
    uint glyph_index;
    
    if (glyph >= GS_MIN_GLYPH_INDEX)
	glyph_index = glyph - GS_MIN_GLYPH_INDEX;
    else {
	glyph_index = pfont->data.get_glyph_index(pfont, glyph);
	if (glyph_index == GS_NO_GLYPH)
	    return_error(gs_error_undefined);
    }
    return gs_type42_glyph_info_by_gid(font, glyph, pmat, members, info, glyph_index);

}
int
gs_type42_enumerate_glyph(gs_font *font, int *pindex,
			  gs_glyph_space_t glyph_space, gs_glyph *pglyph)
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;

    while (++*pindex <= pfont->data.numGlyphs) {
	gs_glyph_data_t outline;
	uint glyph_index = *pindex - 1;
	int code;

	outline.memory = pfont->memory;
	code = pfont->data.get_outline(pfont, glyph_index, &outline);
	if (code < 0)
	    return code;
	if (outline.bits.data == 0)
	    continue;		/* empty (undefined) glyph */
	*pglyph = glyph_index + GS_MIN_GLYPH_INDEX;
	gs_glyph_data_free(&outline, "gs_type42_enumerate_glyph");
	return 0;
    }
    /* We are done. */
    *pindex = 0;
    return 0;
}

/* Get the metrics of a simple glyph. */
private int
simple_glyph_metrics(gs_font_type42 * pfont, uint glyph_index, int wmode,
		     float sbw[4])
{
    int (*string_proc)(gs_font_type42 *, ulong, uint, const byte **) =
	pfont->data.string_proc;
    double factor = 1.0 / pfont->data.unitsPerEm;
    uint width;
    int lsb;
    int code;

    {
	const gs_type42_mtx_t *pmtx = &pfont->data.metrics[wmode];
	uint num_metrics = pmtx->numMetrics;
	const byte *pmetrics;

	if (pmtx->length == 0)
	    return_error(gs_error_rangecheck);
	if (glyph_index < num_metrics) {
	    ACCESS(pmtx->offset + glyph_index * 4, 4, pmetrics);
	    width = U16(pmetrics);
	    lsb = S16(pmetrics + 2);
	} else {
	    uint offset = pmtx->offset + num_metrics * 4;
	    uint glyph_offset = (glyph_index - num_metrics) * 2;
	    const byte *plsb;

	    ACCESS(offset - 4, 4, pmetrics);
	    width = U16(pmetrics);
	    if (glyph_offset >= pmtx->length)
		glyph_offset = pmtx->length - 2;
	    ACCESS(offset + glyph_offset, 2, plsb);
	    lsb = S16(plsb);
	}
    }
    if (wmode) {
	factor = -factor;	/* lsb and width go down the page */
	sbw[0] = 0, sbw[1] = lsb * factor;
	sbw[2] = 0, sbw[3] = width * factor;
    } else {
	sbw[0] = lsb * factor, sbw[1] = 0;
	sbw[2] = width * factor, sbw[3] = 0;
    }
    return 0;
}

/* Get the metrics of a glyph. */
int
gs_type42_default_get_metrics(gs_font_type42 * pfont, uint glyph_index,
			      int wmode, float sbw[4])
{
    gs_glyph_data_t glyph_data;
    int code;
    int result;

    glyph_data.memory = pfont->memory;
    code = pfont->data.get_outline(pfont, glyph_index, &glyph_data);
    if (code < 0)
	return code;
    if (glyph_data.bits.size != 0 && S16(glyph_data.bits.data) == -1) {
	/* This is a composite glyph. */
	uint flags;
	const byte *gdata = glyph_data.bits.data + 10;
	gs_matrix_fixed mat;

	memset(&mat, 0, sizeof(mat)); /* arbitrary */
	do {
	    uint comp_index = U16(gdata + 2);

	    parse_component(&gdata, &flags, &mat, NULL, pfont, &mat);
	    if (flags & TT_CG_USE_MY_METRICS) {
		result = gs_type42_wmode_metrics(pfont, comp_index, wmode, sbw);
		goto done;
	    }
	}
	while (flags & TT_CG_MORE_COMPONENTS);
    }
    result = simple_glyph_metrics(pfont, glyph_index, wmode, sbw);
 done:
    gs_glyph_data_free(&glyph_data, "gs_type42_default_get_metrics");
    return result;
}
int
gs_type42_wmode_metrics(gs_font_type42 * pfont, uint glyph_index, int wmode,
			float sbw[4])
{
    return pfont->data.get_metrics(pfont, glyph_index, wmode, sbw);
}
int
gs_type42_get_metrics(gs_font_type42 * pfont, uint glyph_index,
		      float sbw[4])
{
    return gs_type42_wmode_metrics(pfont, glyph_index, pfont->WMode, sbw);
}

/* Define the bits in the glyph flags. */
#define gf_OnCurve 1
#define gf_xShort 2
#define gf_yShort 4
#define gf_Repeat 8
#define gf_xPos 16		/* xShort */
#define gf_xSame 16		/* !xShort */
#define gf_yPos 32		/* yShort */
#define gf_ySame 32		/* !yShort */

/* Append a TrueType outline to a path. */
/* Note that this does not append the final moveto for the width. */
int
gs_type42_append(uint glyph_index, gs_imager_state * pis,
		 gx_path * ppath, const gs_log2_scale_point * pscale,
		 bool charpath_flag, int paint_type, cached_fm_pair *pair)
{
    int code = append_outline_fitted(glyph_index, &ctm_only(pis), ppath, 
			pair, pscale, charpath_flag);

    if (code < 0)
	return code;
    code = gx_setcurrentpoint_from_path(pis, ppath);
    if (code < 0)
	return code;
    /* Set the flatness for curve rendering. */
    return gs_imager_setflat(pis, gs_char_flatness(pis, 1.0));
}

/* Add 2nd degree Bezier to the path */
private int
add_quadratic_curve(gx_path * const ppath, const gs_fixed_point * const a,
     const gs_fixed_point * const b, const gs_fixed_point * const c)
{
    return gx_path_add_curve(ppath, (a->x + 2*b->x)/3, (a->y + 2*b->y)/3,
	(c->x + 2*b->x)/3, (c->y + 2*b->y)/3, c->x, c->y);
}

/*
 * Append a simple glyph outline to a path (ppath != 0) and/or return
 * its list of points (ppts != 0).
 */
private int
append_simple(const byte *gdata, float sbw[4], const gs_matrix_fixed *pmat,
	      gx_path *ppath, gs_fixed_point *ppts, gs_font_type42 * pfont,
	      bool subglyph)
{
    int numContours = S16(gdata);
    const byte *pends = gdata + 10;
    const byte *pinstr = pends + numContours * 2;
    const byte *pflags;
    uint npoints;
    const byte *pxc, *pyc;
    int code;

    if (numContours == 0)
	return 0;
    /*
     * It appears that the only way to find the X and Y coordinate
     * tables is to parse the flags.  If this is true, it is an
     * incredible piece of bad design.
     */
    {
	const byte *pf = pflags = pinstr + 2 + U16(pinstr);
	uint xbytes = npoints = U16(pinstr - 2) + 1;
	uint np = npoints;

	while (np > 0) {
	    byte flags = *pf++;
	    uint reps = (flags & gf_Repeat ? *pf++ + 1 : 1);

	    if (!(flags & gf_xShort)) {
		if (flags & gf_xSame)
		    xbytes -= reps;
		else
		    xbytes += reps;
	    }
	    np -= reps;
	}
	pxc = pf;
	pyc = pxc + xbytes;
    }

    /* Interpret the contours. */

    {
	uint i, np;
	float offset = 0;
	gs_fixed_point pt;
	double factor = 1.0 / pfont->data.unitsPerEm;
	/*
	 * Decode the first flag byte outside the loop, to avoid a
	 * compiler warning about uninitialized variables.
	 */
	byte flags = *pflags++;
	uint reps = (flags & gf_Repeat ? *pflags++ + 1 : 1);

	if (!subglyph) {
	    int xmin = S16(gdata + 2); /* We like to see it with debugger. */

	    offset = sbw[0] - xmin * factor;
	}
	gs_point_transform2fixed(pmat, offset, 0.0, &pt);
	for (i = 0, np = 0; i < numContours; ++i) {
	    bool move = true;
	    bool off_curve = false;
            bool is_start_off = false;
            uint last_point = U16(pends + i * 2);
	    float dx, dy;
	    gs_fixed_point start,pt_start_off;
	    gs_fixed_point cpoints[2];

            if_debug1('1', "[1t]start %d\n", i);
            
            for (; np <= last_point; --reps, ++np) {
		gs_fixed_point dpt;

		if (reps == 0) {
		    flags = *pflags++;
		    reps = (flags & gf_Repeat ? *pflags++ + 1 : 1);
		}
		if (flags & gf_xShort) {
		    /*
		     * A bug in the Watcom compiler prevents us from doing
		     * the following with the obvious conditional expression.
		     */
		    if (flags & gf_xPos)
			dx = *pxc++ * factor;
		    else
			dx = -(int)*pxc++ * factor;
		} else if (!(flags & gf_xSame))
		    dx = S16(pxc) * factor, pxc += 2;
		else
		    dx = 0;
		if (flags & gf_yShort) {
		    /* See above under dx. */
		    if (flags & gf_yPos)
			dy = *pyc++ * factor;
		    else
			dy = -(int)*pyc++ * factor;
		} else if (!(flags & gf_ySame))
		    dy = S16(pyc) * factor, pyc += 2;
		else
		    dy = 0;
		code = gs_distance_transform2fixed(pmat, dx, dy, &dpt);
		if (code < 0)
		    return code;
		pt.x += dpt.x, pt.y += dpt.y;
		
                if (ppts)	/* return the points */
		    ppts[np] = pt;
		
                if (ppath) {
                    /* append to a path */
		    if_debug3('1', "[1t]%s (%g %g)\n",
		    	(flags & gf_OnCurve ? "on " : "off"), fixed2float(pt.x), fixed2float(pt.y));
                    
                    if (move) {
                        if(is_start_off) {
                            if(flags & gf_OnCurve)
                                start = pt;
                            else { 
                                start.x = (pt_start_off.x + pt.x)/2;
			        start.y = (pt_start_off.y + pt.y)/2;
                                cpoints[1]=pt;
			        off_curve=true;
                            }
                            move = false;
                            cpoints[0] = start;
                            code = gx_path_add_point(ppath, start.x, start.y);
                        } else { 
                            if(flags & gf_OnCurve) { 
                                cpoints[0] = start = pt;
			        code = gx_path_add_point(ppath, pt.x, pt.y);
			        move = false;
                            } else { 
                                is_start_off = true;
                                pt_start_off = pt;
                            }
                        }
		    } else if (flags & gf_OnCurve) {
                        if (off_curve)
			    code = add_quadratic_curve(ppath, cpoints, cpoints+1, &pt);
			else
			    code = gx_path_add_line(ppath, pt.x, pt.y);
			cpoints[0] = pt;
			off_curve = false;
		    } else {
                        if(off_curve) {
			    gs_fixed_point p;
                            p.x = (cpoints[1].x + pt.x)/2;
			    p.y = (cpoints[1].y + pt.y)/2;
			    code = add_quadratic_curve(ppath, cpoints, cpoints+1, &p);
			    cpoints[0] = p;
			}
                        off_curve = true;
		        cpoints[1] = pt;
		    }
		    if (code < 0)
			return code;
		}
	    }
	    if (ppath) {
		if (is_start_off) { 
                    if (off_curve) { 
                        gs_fixed_point p;
                        p.x = (cpoints[1].x + pt_start_off.x)/2;
	                p.y = (cpoints[1].y + pt_start_off.y)/2;
                        code = add_quadratic_curve(ppath, cpoints, cpoints+1, &p);
		        if (code < 0)
		            return code;
                        code = add_quadratic_curve(ppath, &p, &pt_start_off, &start);
		        if (code < 0)
		            return code;
                    } else { 
                        code = add_quadratic_curve(ppath, cpoints, &pt_start_off, &start);
		        if (code < 0)
		            return code;
                    }
                } else { 
                    if (off_curve) { 
                        code = add_quadratic_curve(ppath, cpoints, cpoints+1, &start);
		        if (code < 0)
		            return code;
                    }
                }
                code = gx_path_close_subpath(ppath);
		if (code < 0)
		    return code;
	    }
	}
    }
    return 0;
}

/* Append a glyph outline. */
private int
check_component(uint glyph_index, const gs_matrix_fixed *pmat,
		gx_path *ppath, gs_font_type42 *pfont, gs_fixed_point *ppts,
		gs_glyph_data_t *pgd, bool subglyph)
{
    gs_glyph_data_t glyph_data;
    const byte *gdata;
    float sbw[4];
    int numContours;
    int code;

    glyph_data.memory = pfont->memory;
    code = pfont->data.get_outline(pfont, glyph_index, &glyph_data);
    if (code < 0)
	return code;
    gdata = glyph_data.bits.data;
    if (gdata == 0 || glyph_data.bits.size == 0)	/* empty glyph */
	return 0;
    numContours = S16(gdata);
    if (numContours >= 0) {
	gs_type42_get_metrics(pfont, glyph_index, sbw);
	code = append_simple(gdata, sbw, pmat, ppath, ppts, pfont, subglyph);
	gs_glyph_data_free(&glyph_data, "check_component");
	return (code < 0 ? code : 0); /* simple */
    }
    if (numContours != -1)
	return_error(gs_error_rangecheck);
    *pgd = glyph_data;
    return 1;			/* composite */
}
private int
append_component(uint glyph_index, const gs_matrix_fixed * pmat,
		 gx_path * ppath, gs_fixed_point *ppts, int point_index,
		 gs_font_type42 * pfont, bool subglyph)
{
    gs_glyph_data_t glyph_data;
    int code;

    glyph_data.memory = pfont->memory;
    code = check_component(glyph_index, pmat, ppath, pfont, ppts + point_index,
			   &glyph_data, subglyph);
    if (code != 1)
	return code;
    /*
     * This is a composite glyph.  Because of the "point matching" feature,
     * we have to do an extra pass over each component to fill in the
     * table of points.
     */
    {
	uint flags;
	const byte *gdata = glyph_data.bits.data + 10;

	do {
	    uint comp_index = U16(gdata + 2);
	    gs_matrix_fixed mat;
	    int mp[2];

	    parse_component(&gdata, &flags, &mat, mp, pfont, pmat);
	    if (mp[0] >= 0) {
		/* Match up points.  What a nuisance! */
		const gs_fixed_point *const pfrom = ppts + mp[0];
		/*
		 * Contrary to the TrueType documentation, mp[1] is not
		 * relative to the start of the compound glyph, but is
		 * relative to the start of the component.
		 */
		const gs_fixed_point *const pto = ppts + point_index + mp[1];
		gs_fixed_point diff;

		code = append_component(comp_index, &mat, NULL, ppts,
					point_index, pfont, true);
		if (code < 0)
		    break;
		diff.x = pfrom->x - pto->x;
		diff.y = pfrom->y - pto->y;
		mat.tx = fixed2float(mat.tx_fixed += diff.x);
		mat.ty = fixed2float(mat.ty_fixed += diff.y);
	    }
	    code = append_component(comp_index, &mat, ppath, ppts,
				    point_index, pfont, true);
	    if (code < 0)
		break;
	    point_index += total_points(pfont, comp_index);
	}
	while (flags & TT_CG_MORE_COMPONENTS);
    }
    gs_glyph_data_free(&glyph_data, "append_component");
    return code;
}

private int
append_outline_fitted(uint glyph_index, const gs_matrix * pmat,
	       gx_path * ppath, cached_fm_pair * pair, 
	       const gs_log2_scale_point * pscale, bool design_grid)
{
    gs_font_type42 *pfont = (gs_font_type42 *)pair->font;
    int code;

    gx_ttfReader__set_font(pair->ttr, pfont);
    code = gx_ttf_outline(pair->ttf, pair->ttr, pfont, (uint)glyph_index, 
	pmat, pscale, ppath, design_grid);
    gx_ttfReader__set_font(pair->ttr, NULL);
    return code;
}

