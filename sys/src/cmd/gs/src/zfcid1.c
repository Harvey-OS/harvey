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

/*$Id: zfcid1.c,v 1.2.2.1 2000/11/09 23:04:44 rayjj Exp $ */
/* CIDFontType 1 and 2 operators */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsmatrix.h"
#include "gsccode.h"
#include "gsstruct.h"
#include "gxfcid.h"
#include "bfont.h"
#include "icid.h"
#include "idict.h"
#include "idparam.h"
#include "ifcid.h"
#include "ifont42.h"
#include "store.h"

/* ---------------- CIDFontType 1 (FontType 10) ---------------- */

/* <string|name> <font_dict> .buildfont10 <string|name> <font> */
private int
zbuildfont10(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    build_proc_refs build;
    int code = build_gs_font_procs(op, &build);
    gs_cid_system_info_t cidsi;
    gs_font_base *pfont;

    if (code < 0)
	return code;
    code = cid_font_system_info_param(&cidsi, op);
    if (code < 0)
	return code;
    make_null(&build.BuildChar);	/* only BuildGlyph */
    code = build_gs_simple_font(i_ctx_p, op, &pfont, ft_CID_user_defined,
				&st_gs_font_cid1, &build,
				bf_Encoding_optional |
				bf_FontBBox_required |
				bf_UniqueID_ignored);
    if (code < 0)
	return code;
    ((gs_font_cid1 *)pfont)->cidata.CIDSystemInfo = cidsi;
    return define_gs_font((gs_font *)pfont);
}

/* ---------------- CIDFontType 2 (FontType 11) ---------------- */

/* ------ Accessing ------ */

/* Map a glyph CID to a TrueType glyph number using the CIDMap. */
private int
z11_CIDMap_proc(gs_font_cid2 *pfont, gs_glyph glyph)
{
    const ref *pcidmap = &pfont_data(pfont)->u.type42.CIDMap;
    ulong cid = glyph - gs_min_cid_glyph;
    int gdbytes = pfont->cidata.common.GDBytes;
    int gnum = 0;
    const byte *data;
    int i, code;
    ref rcid;
    ref *prgnum;

    switch (r_type(pcidmap)) {
    case t_string:
	if (cid >= r_size(pcidmap) / gdbytes)
	    return_error(e_rangecheck);
	data = pcidmap->value.const_bytes + cid * gdbytes;
	break;
    case t_integer:
	return cid + pcidmap->value.intval;
    case t_dictionary:
	make_int(&rcid, cid);
	code = dict_find(pcidmap, &rcid, &prgnum);
	if (code <= 0)
	    return (code < 0 ? code : gs_note_error(e_undefined));
	if (!r_has_type(prgnum, t_integer))
	    return_error(e_typecheck);
	return prgnum->value.intval;
    default:			/* array type */
	code = string_array_access_proc(pcidmap, 1, cid * gdbytes,
					gdbytes, &data);

	if (code < 0)
	    return code;
    }
    for (i = 0; i < gdbytes; ++i)
	gnum = (gnum << 8) + data[i];
    return gnum;
}

/* Handle MetricsCount when accessing outline or metrics information. */
private int
z11_get_outline(gs_font_type42 * pfont, uint glyph_index,
		gs_const_string * pgstr)
{
    gs_font_cid2 *const pfcid = (gs_font_cid2 *)pfont;
    int skip = pfcid->cidata.MetricsCount << 1;
    int code = pfcid->cidata.orig_procs.get_outline(pfont, glyph_index, pgstr);

    if (code >= 0) {
	byte *data = (byte *)pgstr->data;  /* break const */
	uint size = pgstr->size;

	if (size <= skip) {
	    if (code > 0 && size != 0)
		gs_free_string(pfont->memory, data, size, "z11_get_outline");
	    pgstr->data = 0, pgstr->size = 0;
	} else {
	    if (code > 0) {
		/* Newly allocated, freeble string. */
		memmove(data, data + skip, size - skip);
		pgstr->data = gs_resize_string(pfont->memory, data,
					       size, size - skip,
					       "z11_get_outline");
	    } else {
		pgstr->data += skip;
	    }
	    pgstr->size = size - skip;
	}
    }
    return code;
}
private int
z11_get_metrics(gs_font_type42 * pfont, uint glyph_index, int wmode,
		float sbw[4])
{
    gs_font_cid2 *const pfcid = (gs_font_cid2 *)pfont;
    int skip = pfcid->cidata.MetricsCount << 1;
    gs_const_string gstr;
    const byte *pmetrics;
    int lsb, width;

    if (wmode > skip >> 2 ||
	pfcid->cidata.orig_procs.get_outline(pfont, glyph_index, &gstr) < 0 ||
	gstr.size < skip
	)
	return pfcid->cidata.orig_procs.get_metrics(pfont, glyph_index, wmode,
						    sbw);
    pmetrics = gstr.data + skip - (wmode << 2);
    lsb = (pmetrics[2] << 8) + pmetrics[3];
    width = (pmetrics[0] << 8) + pmetrics[1];
    {
	double factor = 1.0 / pfont->data.unitsPerEm;

	if (wmode) {
	    sbw[0] = 0, sbw[1] = -lsb * factor;
	    sbw[2] = 0, sbw[3] = -width * factor;
	} else {
	    sbw[0] = lsb * factor, sbw[1] = 0;
	    sbw[2] = width * factor, sbw[3] = 0;
	}
    }
    return 0;
}

/* ------ Defining ------ */

/* <string|name> <font_dict> .buildfont11 <string|name> <font> */
private int
zbuildfont11(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_font_cid_data common;
    gs_font_type42 *pfont;
    gs_font_cid2 *pfcid;
    int MetricsCount;
    ref rcidmap, ignore_gdir;
    int code = cid_font_data_param(op, &common, &ignore_gdir);

    if (code < 0 ||
	(code = dict_int_param(op, "MetricsCount", 0, 4, 0, &MetricsCount)) < 0
	)
	return code;
    if (MetricsCount & 1)	/* only allowable values are 0, 2, 4 */
	return_error(e_rangecheck);
    code = font_string_array_param(op, "CIDMap", &rcidmap);
    switch (code) {
    case 0:			/* in PLRM3 */
    gdb:
	/* GDBytes is required for indexing a string or string array. */
	if (common.GDBytes == 0)
	    return_error(e_rangecheck);
	break;
    default:
	return code;
    case e_typecheck:
	switch (r_type(&rcidmap)) {
	case t_string:		/* in PLRM3 */
	    goto gdb;
	case t_dictionary:	/* added in 3011 */
	case t_integer:		/* added in 3011 */
	    break;
	default:
	    return code;
	}
	break;
    }
    code = build_gs_TrueType_font(i_ctx_p, op, &pfont, ft_CID_TrueType,
				  &st_gs_font_cid2,
				  (const char *)0, "%Type11BuildGlyph",
				  bf_Encoding_optional |
				  bf_FontBBox_required |
				  bf_UniqueID_ignored |
				  bf_CharStrings_optional);
    if (code < 0)
	return code;
    pfcid = (gs_font_cid2 *)pfont;
    pfcid->cidata.common = common;
    pfcid->cidata.MetricsCount = MetricsCount;
    ref_assign(&pfont_data(pfont)->u.type42.CIDMap, &rcidmap);
    pfcid->cidata.CIDMap_proc = z11_CIDMap_proc;
    if (MetricsCount) {
	/* "Wrap" the glyph accessor procedures. */
	pfcid->cidata.orig_procs.get_outline = pfont->data.get_outline;
	pfont->data.get_outline = z11_get_outline;
	pfcid->cidata.orig_procs.get_metrics = pfont->data.get_metrics;
	pfont->data.get_metrics = z11_get_metrics;
    }
    return define_gs_font((gs_font *)pfont);
}

/* <cid11font> <cid> .type11mapcid <glyph_index> */
private int
ztype11mapcid(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_font *pfont;
    int code = font_param(op - 1, &pfont);

    if (code < 0)
	return code;
    if (pfont->FontType != ft_CID_TrueType)
	return_error(e_invalidfont);
    check_type(*op, t_integer);
    code = z11_CIDMap_proc((gs_font_cid2 *)pfont,
			   (gs_glyph)(gs_min_cid_glyph + op->value.intval));
    if (code < 0)
	return code;
    make_int(op - 1, code);
    pop(1);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zfcid1_op_defs[] =
{
    {"2.buildfont10", zbuildfont10},
    {"2.buildfont11", zbuildfont11},
    {"2.type11mapcid", ztype11mapcid},
    op_def_end(0)
};
