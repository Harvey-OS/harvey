/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gstype42.c,v 1.7.2.2 2000/11/09 21:32:30 rayjj Exp $ */
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
#include "gxistate.h"

/* Structure descriptor */
public_st_gs_font_type42();

/* Forward references */
private int append_outline(P4(uint glyph_index, const gs_matrix_fixed * pmat,
			      gx_path * ppath, gs_font_type42 * pfont));
private int default_get_outline(P3(gs_font_type42 *pfont, uint glyph_index,
				   gs_const_string *pgstr));
private int default_get_metrics(P4(gs_font_type42 *pfont, uint glyph_index,
				   int wmode, float sbw[4]));

/* Set up a pointer to a substring of the font data. */
/* Free variables: pfont, string_proc. */
#define ACCESS(base, length, vptr)\
  BEGIN\
    code = (*string_proc)(pfont, (ulong)(base), length, &vptr);\
    if ( code < 0 ) return code;\
  END

/* Get 2- or 4-byte quantities from a table. */
#define U8(p) ((uint)((p)[0]))
#define S8(p) (int)((U8(p) ^ 0x80) - 0x80)
#define U16(p) (((uint)((p)[0]) << 8) + (p)[1])
#define S16(p) (int)((U16(p) ^ 0x8000) - 0x8000)
#define u32(p) get_u32_msb(p)

/* ---------------- Font level ---------------- */

/* Initialize the cached values in a Type 42 font. */
/* Note that this initializes get_outline and the font procedures as well. */
int
gs_type42_font_init(gs_font_type42 * pfont)
{
    int (*string_proc)(P4(gs_font_type42 *, ulong, uint, const byte **)) =
	pfont->data.string_proc;
    const byte *OffsetTable;
    uint numTables;
    const byte *TableDirectory;
    uint i;
    int code;
    byte head_box[8];
    ulong loca_size = 0;

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
    /* Clear optional entries. */
    memset(pfont->data.metrics, 0, sizeof(pfont->data.metrics));
    for (i = 0; i < numTables; ++i) {
	const byte *tab = TableDirectory + i * 16;
	ulong offset = u32(tab + 8);

	if (!memcmp(tab, "glyf", 4))
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
	float upem = pfont->data.unitsPerEm;

	pfont->FontBBox.p.x = S16(head_box) / upem;
	pfont->FontBBox.p.y = S16(head_box + 2) / upem;
	pfont->FontBBox.q.x = S16(head_box + 4) / upem;
	pfont->FontBBox.q.y = S16(head_box + 6) / upem;
    }
    pfont->data.get_outline = default_get_outline;
    pfont->data.get_metrics = default_get_metrics;
    pfont->procs.glyph_outline = gs_type42_glyph_outline;
    pfont->procs.glyph_info = gs_type42_glyph_info;
    pfont->procs.enumerate_glyph = gs_type42_enumerate_glyph;
    return 0;
}

/* ---------------- Glyph level ---------------- */

/* Define the bits in the component glyph flags. */
#define cg_argsAreWords 1
#define cg_argsAreXYValues 2
#define cg_roundXYToGrid 4
#define cg_haveScale 8
#define cg_moreComponents 32
#define cg_haveXYScale 64
#define cg_have2x2 128
#define cg_useMyMetrics 512

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
    if (flags & cg_argsAreXYValues) {
	int arg1, arg2;
	gs_fixed_point pt;

	if (flags & cg_argsAreWords)
	    arg1 = S16(gdata), arg2 = S16(gdata + 2), gdata += 4;
	else
	    arg1 = S8(gdata), arg2 = S8(gdata + 1), gdata += 2;
	if (flags & cg_roundXYToGrid) {
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
	if (flags & cg_argsAreWords) {
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
    if (flags & cg_haveScale) {
	scale_mat.xx = scale_mat.yy = S2_14(gdata);
	scale_mat.xy = scale_mat.yx = 0;
	gdata += 2;
    } else if (flags & cg_haveXYScale) {
	scale_mat.xx = S2_14(gdata);
	scale_mat.yy = S2_14(gdata + 2);
	scale_mat.xy = scale_mat.yx = 0;
	gdata += 4;
    } else if (flags & cg_have2x2) {
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
    gs_const_string glyph_string;
    int code = pfont->data.get_outline(pfont, glyph_index, &glyph_string);
    const byte *gdata = glyph_string.data;

    if (code < 0)
	return code;
    if (glyph_string.size == 0)
	return 0;
    if (S16(gdata) != -1) {
	/* This is a simple glyph. */
	int numContours = S16(gdata);
	const byte *pends = gdata + 10;
	const byte *pinstr = pends + numContours * 2;

	return (numContours == 0 ? 0 : U16(pinstr - 2) + 1);
    }
    /* This is a composite glyph.  Add up the components. */
    gdata += 10;
    {
	uint flags;
	gs_matrix_fixed mat;
	int total = 0;

	memset(&mat, 0, sizeof(mat)); /* arbitrary */
	do {
	    code = total_points(pfont, U16(gdata + 2));
	    if (code < 0)
		return code;
	    total += code;
	    parse_component(&gdata, &flags, &mat, NULL, pfont, &mat);
	}
	while (flags & cg_moreComponents);
	return total;
    }
}

/* Define the default implementation for getting the outline data for */
/* a glyph, using indexToLocFormat and the loca and glyf tables. */
/* Set pglyph->data = 0 if the glyph is empty. */
private int
default_get_outline(gs_font_type42 * pfont, uint glyph_index,
		    gs_const_string * pglyph)
{
    int (*string_proc) (P4(gs_font_type42 *, ulong, uint, const byte **)) =
    pfont->data.string_proc;
    const byte *ploca;
    ulong glyph_start;
    uint glyph_length;
    int code;

    /*
     * We can't assume that consecutive loca entries are stored
     * contiguously in memory: we have to access each entry
     * individually.
     */
    if (pfont->data.indexToLocFormat) {
	ACCESS(pfont->data.loca + glyph_index * 4, 4, ploca);
	glyph_start = u32(ploca);
	ACCESS(pfont->data.loca + glyph_index * 4 + 4, 4, ploca);
	glyph_length = u32(ploca) - glyph_start;
    } else {
	ACCESS(pfont->data.loca + glyph_index * 2, 2, ploca);
	glyph_start = (ulong) U16(ploca) << 1;
	ACCESS(pfont->data.loca + glyph_index * 2 + 2, 2, ploca);
	glyph_length = ((ulong) U16(ploca) << 1) - glyph_start;
    }
    pglyph->size = glyph_length;
    if (glyph_length == 0)
	pglyph->data = 0;
    else
	ACCESS(pfont->data.glyf + glyph_start, glyph_length, pglyph->data);
    return 0;
}

/* Parse a glyph into pieces, if any. */
private int
parse_pieces(gs_font_type42 *pfont, gs_glyph glyph, gs_glyph *pieces,
	     int *pnum_pieces)
{
    uint glyph_index = glyph - gs_min_cid_glyph;
    gs_const_string glyph_string;
    int code = pfont->data.get_outline(pfont, glyph_index, &glyph_string);

    if (code < 0)
	return code;
    if (glyph_string.size != 0 && S16(glyph_string.data) == -1) {
	/* This is a composite glyph. */
	int i = 0;
	uint flags = cg_moreComponents;
	const byte *gdata = glyph_string.data + 10;
	gs_matrix_fixed mat;

	memset(&mat, 0, sizeof(mat)); /* arbitrary */
	for (i = 0; flags & cg_moreComponents; ++i) {
	    if (pieces)
		pieces[i] = U16(gdata + 2) + gs_min_cid_glyph;
	    parse_component(&gdata, &flags, &mat, NULL, pfont, &mat);
	}
	*pnum_pieces = i;
    } else
	*pnum_pieces = 0;
    return 0;
}

/* Define the font procedures for a Type 42 font. */
int
gs_type42_glyph_outline(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
			gx_path *ppath)
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;
    uint glyph_index = glyph - gs_min_cid_glyph;
    gs_fixed_point origin;
    int code;
    gs_glyph_info_t info;
    gs_matrix_fixed fmat;
    static const gs_matrix imat = { identity_matrix_body };

    if (pmat == 0)
	pmat = &imat;
    if ((code = gs_matrix_fixed_from_matrix(&fmat, pmat)) < 0 ||
	(code = gx_path_current_point(ppath, &origin)) < 0 ||
	(code = append_outline(glyph_index, &fmat, ppath, pfont)) < 0 ||
	(code = font->procs.glyph_info(font, glyph, pmat,
				       GLYPH_INFO_WIDTH, &info)) < 0
	)
	return code;
    return gx_path_add_point(ppath, origin.x + float2fixed(info.width[0].x),
			     origin.y + float2fixed(info.width[0].y));
}
int
gs_type42_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		     int members, gs_glyph_info_t *info)
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;
    uint glyph_index = glyph - gs_min_cid_glyph;
    int default_members =
	members & ~(GLYPH_INFO_WIDTHS | GLYPH_INFO_NUM_PIECES |
		    GLYPH_INFO_PIECES);
    gs_const_string outline;
    int code = 0;

    if (default_members) {
	code = gs_default_glyph_info(font, glyph, pmat, default_members, info);

	if (code < 0)
	    return code;
    } else if ((code = pfont->data.get_outline(pfont, glyph_index, &outline)) < 0)
	return code;		/* non-existent glyph */
    else
	info->members = 0;
    if (members & GLYPH_INFO_WIDTH) {
	int i;

	for (i = 0; i < 2; ++i)
	    if (members & (GLYPH_INFO_WIDTH0 << i)) {
		float sbw[4];

		code = gs_type42_wmode_metrics(pfont, glyph_index, i, sbw);
		if (code < 0)
		    return code;
		if (pmat)
		    code = gs_point_transform(sbw[2], sbw[3], pmat,
					      &info->width[i]);
		else
		    info->width[i].x = sbw[2], info->width[i].y = sbw[3];
	    }
	info->members |= members & GLYPH_INFO_WIDTH;
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
gs_type42_enumerate_glyph(gs_font *font, int *pindex,
			  gs_glyph_space_t glyph_space, gs_glyph *pglyph)
{
    gs_font_type42 *const pfont = (gs_font_type42 *)font;

    while (++*pindex <= pfont->data.numGlyphs) {
	gs_const_string outline;
	uint glyph_index = *pindex - 1;
	int code = pfont->data.get_outline(pfont, glyph_index, &outline);

	if (code < 0)
	    return code;
	if (outline.data == 0)
	    continue;		/* empty (undefined) glyph */
	*pglyph = glyph_index + gs_min_cid_glyph;
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
    int (*string_proc)(P4(gs_font_type42 *, ulong, uint, const byte **)) =
	pfont->data.string_proc;
    double factor = 1.0 / pfont->data.unitsPerEm;
    uint width;
    int lsb;
    int code;

    {
	const gs_type42_mtx_t *pmtx = &pfont->data.metrics[wmode];
	uint num_metrics = pmtx->numMetrics;
	const byte *pmetrics;

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
private int
default_get_metrics(gs_font_type42 * pfont, uint glyph_index, int wmode,
		    float sbw[4])
{
    gs_const_string glyph_string;
    int code = pfont->data.get_outline(pfont, glyph_index, &glyph_string);

    if (code < 0)
	return code;
    if (glyph_string.size != 0 && S16(glyph_string.data) == -1) {
	/* This is a composite glyph. */
	uint flags;
	const byte *gdata = glyph_string.data + 10;
	gs_matrix_fixed mat;

	memset(&mat, 0, sizeof(mat)); /* arbitrary */
	do {
	    uint comp_index = U16(gdata + 2);

	    parse_component(&gdata, &flags, &mat, NULL, pfont, &mat);
	    if (flags & cg_useMyMetrics) {
		return gs_type42_wmode_metrics(pfont, comp_index, wmode, sbw);
	    }
	}
	while (flags & cg_moreComponents);
    }
    return simple_glyph_metrics(pfont, glyph_index, wmode, sbw);
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
		 bool charpath_flag, int paint_type, gs_font_type42 * pfont)
{
    int code = append_outline(glyph_index, &pis->ctm, ppath, pfont);

    if (code < 0)
	return code;
    /* Set the flatness for curve rendering. */
    return gs_imager_setflat(pis, gs_char_flatness(pis, 1.0));
}

/*
 * Append a simple glyph outline to a path (ppath != 0) and/or return
 * its list of points (ppts != 0).
 */
private int
append_simple(const byte *gdata, float sbw[4], const gs_matrix_fixed *pmat,
	      gx_path *ppath, gs_fixed_point *ppts, gs_font_type42 * pfont)
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
	gs_fixed_point pt;
	double factor = 1.0 / pfont->data.unitsPerEm;
	/*
	 * Decode the first flag byte outside the loop, to avoid a
	 * compiler warning about uninitialized variables.
	 */
	byte flags = *pflags++;
	uint reps = (flags & gf_Repeat ? *pflags++ + 1 : 1);

	/*
	 * The TrueType documentation gives no clue as to how the lsb
	 * should affect placement of the outline.  Our best guess is
	 * that the outline should be translated by lsb - xMin.
	 */
	gs_point_transform2fixed(pmat, sbw[0] - S16(gdata + 2) * factor,
				 0.0, &pt);
	for (i = 0, np = 0; i < numContours; ++i) {
	    bool move = true;
	    uint last_point = U16(pends + i * 2);
	    float dx, dy;
	    int off_curve = 0;
	    gs_fixed_point start;
	    gs_fixed_point cpoints[3];

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
		if (ppath) {	/* append to a path */
#define control1(xy) cpoints[1].xy
#define control2(xy) cpoints[2].xy
#define control3off(xy) ((cpoints[1].xy + pt.xy) / 2)
#define control4off(xy) ((cpoints[0].xy + 2 * cpoints[1].xy) / 3)
#define control5off(xy) ((2 * cpoints[1].xy + cpoints[2].xy) / 3)
#define control6off(xy) ((2 * cpoints[1].xy + pt.xy) / 3)
#define control7off(xy) ((2 * cpoints[1].xy + start.xy) / 3)
		    if (move) {
			if_debug2('1', "[1t]start (%g,%g)\n",
				  fixed2float(pt.x), fixed2float(pt.y));
			start = pt;
			code = gx_path_add_point(ppath, pt.x, pt.y);
			cpoints[0] = pt;
			move = false;
		    } else if (flags & gf_OnCurve) {
			if_debug2('1', "[1t]ON (%g,%g)\n",
				  fixed2float(pt.x), fixed2float(pt.y));
			if (off_curve)
			    code = gx_path_add_curve(ppath, control4off(x),
						control4off(y), control6off(x),
						control6off(y), pt.x, pt.y);
			else
			    code = gx_path_add_line(ppath, pt.x, pt.y);
			cpoints[0] = pt;
			off_curve = 0;
		    } else {
			if_debug2('1', "[1t]...off (%g,%g)\n",
				  fixed2float(pt.x), fixed2float(pt.y));
			switch (off_curve++) {
			default:	/* >= 1 */
			    control2(x) = control3off(x);
			    control2(y) = control3off(y);
			    code = gx_path_add_curve(ppath,
						control4off(x), control4off(y),
						control5off(x), control5off(y),
						control2(x), control2(y));
			    cpoints[0] = cpoints[2];
			    off_curve = 1;
			    /* falls through */
			case 0:
			    cpoints[1] = pt;
			}
		    }
		    if (code < 0)
			return code;
		}
	    }
	    if (ppath) {
		if (off_curve)
		    code = gx_path_add_curve(ppath,
					control4off(x), control4off(y),
					control7off(x), control7off(y),
					start.x, start.y);
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
		const byte **pgdata)
{
    gs_const_string glyph_string;
    const byte *gdata;
    float sbw[4];
    int numContours;
    int code;

    code = pfont->data.get_outline(pfont, glyph_index, &glyph_string);
    if (code < 0)
	return code;
    gdata = glyph_string.data;
    if (gdata == 0 || glyph_string.size == 0)	/* empty glyph */
	return 0;
    numContours = S16(gdata);
    if (numContours >= 0) {
	simple_glyph_metrics(pfont, glyph_index, pfont->WMode, sbw);
	code = append_simple(gdata, sbw, pmat, ppath, ppts, pfont);
	return (code < 0 ? code : 0); /* simple */
    }
    if (numContours != -1)
	return_error(gs_error_rangecheck);
    *pgdata = gdata;
    return 1;			/* composite */
}
private int
append_component(uint glyph_index, const gs_matrix_fixed * pmat,
		 gx_path * ppath, gs_fixed_point *ppts, int point_index,
		 gs_font_type42 * pfont)
{
    const byte *gdata;
    int code;

    code = check_component(glyph_index, pmat, ppath, pfont, ppts + point_index,
			   &gdata);
    if (code != 1)
	return code;
    /*
     * This is a composite glyph.  Because of the "point matching" feature,
     * we have to do an extra pass over each component to fill in the
     * table of points.
     */
    {
	uint flags;

	gdata += 10;
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
					point_index, pfont);
		if (code < 0)
		    return code;
		diff.x = pfrom->x - pto->x;
		diff.y = pfrom->y - pto->y;
		mat.tx = fixed2float(mat.tx_fixed += diff.x);
		mat.ty = fixed2float(mat.ty_fixed += diff.y);
	    }
	    code = append_component(comp_index, &mat, ppath, ppts,
				    point_index, pfont);
	    if (code < 0)
		return code;
	    point_index += total_points(pfont, comp_index);
	}
	while (flags & cg_moreComponents);
    }
    return 0;
}
private int
append_outline(uint glyph_index, const gs_matrix_fixed * pmat,
	       gx_path * ppath, gs_font_type42 * pfont)
{
    {
	const byte *gdata;
	int code =
	    check_component(glyph_index, pmat, ppath, pfont, NULL, &gdata);

	if (code != 1)
	    return code;
    }
    {
	/*
	 * Set up the points array (only needed for point matching, sigh).
	 * We use stack allocation if possible, to avoid creating a sandbar
	 * (pts will be allocated before, but also freed before, any path
	 * elements).
	 */
#define MAX_STACK_PTS 150	/* usually enough */
	int num_points = total_points(pfont, glyph_index);

	if (num_points <= MAX_STACK_PTS) {
	    gs_fixed_point pts[MAX_STACK_PTS];

	    return append_component(glyph_index, pmat, ppath, pts, 0, pfont);
	} else {
	    gs_memory_t *mem = pfont->memory; /* any memory will do */
	    gs_fixed_point *ppts = (gs_fixed_point *)
		gs_alloc_byte_array(mem, num_points, sizeof(gs_fixed_point),
				    "append_outline");
	    int code;

	    if (ppts == 0)
		return_error(gs_error_VMerror);
	    code = append_component(glyph_index, pmat, ppath, ppts, 0, pfont);
	    gs_free_object(mem, ppts, "append_outline");
	    return code;
	}
#undef MAX_STACK_PTS
    }
}
