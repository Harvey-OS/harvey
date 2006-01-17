/* Copyright (C) 1991, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zfont1.c,v 1.13 2004/09/22 13:52:33 igor Exp $ */
/* Type 1 and Type 4 font creation operators */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gxfixed.h"
#include "gsmatrix.h"
#include "gxdevice.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "bfont.h"
#include "ialloc.h"
#include "icharout.h"
#include "ichar1.h"
#include "idict.h"
#include "idparam.h"
#include "ifont1.h"
#include "iname.h"		/* for name_index in enumerate_glyph */
#include "store.h"

/* Type 1 font procedures (defined in zchar1.c) */
extern const gs_type1_data_procs_t z1_data_procs;
font_proc_glyph_info(z1_glyph_info);
/* Font procedures defined here */
private font_proc_same_font(z1_same_font);

/* ------ Private utilities ------ */

private void
find_zone_height(float *pmax_height, int count, const float *values)
{
    int i;
    float zone_height;

    for (i = 0; i < count; i += 2)
	if ((zone_height = values[i + 1] - values[i]) > *pmax_height)
	    *pmax_height = zone_height;
}

/* ------ Font procedures ------ */

private int
z1_enumerate_glyph(gs_font * pfont, int *pindex, gs_glyph_space_t ignored,
		   gs_glyph * pglyph)
{
    const gs_font_type1 *const pfont1 = (gs_font_type1 *)pfont;
    const ref *pcsdict = &pfont_data(pfont1)->CharStrings;

    return zchar_enumerate_glyph(pfont->memory, pcsdict, pindex, pglyph);
}

/* ------ Public procedures ------ */

/* Extract pointers to internal structures. */
int
charstring_font_get_refs(const_os_ptr op, charstring_font_refs_t *pfr)
{
    check_type(*op, t_dictionary);
    if (dict_find_string(op, "Private", &pfr->Private) <= 0 ||
	!r_has_type(pfr->Private, t_dictionary)
	)
	return_error(e_invalidfont);
    make_empty_array(&pfr->no_subrs, 0);
    if (dict_find_string(pfr->Private, "OtherSubrs", &pfr->OtherSubrs) > 0) {
	if (!r_is_array(pfr->OtherSubrs))
	    return_error(e_typecheck);
    } else
	pfr->OtherSubrs = &pfr->no_subrs;
    if (dict_find_string(pfr->Private, "Subrs", &pfr->Subrs) > 0) {
	if (!r_is_array(pfr->Subrs))
	    return_error(e_typecheck);
    } else
	pfr->Subrs = &pfr->no_subrs;
    pfr->GlobalSubrs = &pfr->no_subrs;
    return 0;
}

/* Get the parameters of a CharString-based font or a FDArray entry. */
int
charstring_font_params(const gs_memory_t *mem, 
		       const_os_ptr op, charstring_font_refs_t *pfr,
		       gs_type1_data *pdata1)
{
    const ref *pprivate = pfr->Private;
    int code;

    /* Get the rest of the information from the Private dictionary. */
    if ((code = dict_int_param(pprivate, "lenIV", -1, 255, pdata1->lenIV,
			       &pdata1->lenIV)) < 0 ||
	(code = dict_uint_param(pprivate, "subroutineNumberBias",
				0, max_uint, pdata1->subroutineNumberBias,
				&pdata1->subroutineNumberBias)) < 0 ||
	(code = dict_int_param(pprivate, "BlueFuzz", 0, 1999, 1,
			       &pdata1->BlueFuzz)) < 0 ||
	(code = dict_float_param(pprivate, "BlueScale", 0.039625,
				 &pdata1->BlueScale)) < 0 ||
	(code = dict_float_param(pprivate, "BlueShift", 7.0,
				 &pdata1->BlueShift)) < 0 ||
	(code = pdata1->BlueValues.count =
	 dict_float_array_param(mem, pprivate, "BlueValues", max_BlueValues * 2,
				&pdata1->BlueValues.values[0], NULL)) < 0 ||
	(code = dict_float_param(pprivate, "ExpansionFactor", 0.06,
				 &pdata1->ExpansionFactor)) < 0 ||
	(code = pdata1->FamilyBlues.count =
	 dict_float_array_param(mem, pprivate, "FamilyBlues", max_FamilyBlues * 2,
				&pdata1->FamilyBlues.values[0], NULL)) < 0 ||
	(code = pdata1->FamilyOtherBlues.count =
	 dict_float_array_param(mem, pprivate,
				"FamilyOtherBlues", max_FamilyOtherBlues * 2,
			    &pdata1->FamilyOtherBlues.values[0], NULL)) < 0 ||
	(code = dict_bool_param(pprivate, "ForceBold", false,
				&pdata1->ForceBold)) < 0 ||
    /*
     * We've seen a few fonts with out-of-range LanguageGroup values;
     * if it weren't for this, the only legal values should be 0 or 1.
     */
	(code = dict_int_param(pprivate, "LanguageGroup", min_int, max_int, 0,
			       &pdata1->LanguageGroup)) < 0 ||
	(code = pdata1->OtherBlues.count =
	 dict_float_array_param(mem, pprivate, "OtherBlues", max_OtherBlues * 2,
				&pdata1->OtherBlues.values[0], NULL)) < 0 ||
	(code = dict_bool_param(pprivate, "RndStemUp", true,
				&pdata1->RndStemUp)) < 0 ||
	(code = pdata1->StdHW.count =
	 dict_float_array_check_param(mem, pprivate, "StdHW", 1,
				      &pdata1->StdHW.values[0], NULL,
				      0, e_rangecheck)) < 0 ||
	(code = pdata1->StdVW.count =
	 dict_float_array_check_param(mem, pprivate, "StdVW", 1,
				      &pdata1->StdVW.values[0], NULL,
				      0, e_rangecheck)) < 0 ||
	(code = pdata1->StemSnapH.count =
	 dict_float_array_param(mem, pprivate, "StemSnapH", max_StemSnap,
				&pdata1->StemSnapH.values[0], NULL)) < 0 ||
	(code = pdata1->StemSnapV.count =
	 dict_float_array_param(mem, pprivate, "StemSnapV", max_StemSnap,
				&pdata1->StemSnapV.values[0], NULL)) < 0 ||
    /* The WeightVector is in the font dictionary, not Private. */
	(code = pdata1->WeightVector.count =
	 dict_float_array_param(mem, op, "WeightVector", max_WeightVector,
				pdata1->WeightVector.values, NULL)) < 0
	)
	return code;
    /*
     * According to section 5.6 of the "Adobe Type 1 Font Format",
     * there is a requirement that BlueScale times the maximum
     * alignment zone height must be less than 1.  Some fonts
     * produced by Fontographer have ridiculously large BlueScale
     * values, so we force BlueScale back into range here.
     */
    {
	float max_zone_height = 1.0;

#define SCAN_ZONE(z)\
    find_zone_height(&max_zone_height, pdata1->z.count, pdata1->z.values);

	SCAN_ZONE(BlueValues);
	SCAN_ZONE(OtherBlues);
	SCAN_ZONE(FamilyBlues);
	SCAN_ZONE(FamilyOtherBlues);

#undef SCAN_ZONE

	if (pdata1->BlueScale * max_zone_height > 1.0)
	    pdata1->BlueScale = 1.0 / max_zone_height;
    }
    /*
     * According to the same Adobe book, section 5.11, only values
     * 0 and 1 are allowed for LanguageGroup and we have encountered
     * fonts with other values. If the value is anything else, map it to 0
     * so that the remainder of the graphics library won't see an
     * unexpected value.
     */
    if (pdata1->LanguageGroup > 1 || pdata1->LanguageGroup < 0)
	pdata1->LanguageGroup = 0;
    return 0;
}

/* Fill in a newly built CharString-based font or FDArray entry. */
int
charstring_font_init(gs_font_type1 *pfont, const charstring_font_refs_t *pfr,
		     const gs_type1_data *pdata1)
{
    font_data *pdata;

    pdata = pfont_data(pfont);
    pfont->data = *pdata1;
    pfont->data.parent = NULL;
    ref_assign(&pdata->u.type1.OtherSubrs, pfr->OtherSubrs);
    ref_assign(&pdata->u.type1.Subrs, pfr->Subrs);
    ref_assign(&pdata->u.type1.GlobalSubrs, pfr->GlobalSubrs);
    pfont->data.procs = z1_data_procs;
    pfont->data.proc_data = (char *)pdata;
    pfont->procs.same_font = z1_same_font;
    pfont->procs.glyph_info = z1_glyph_info;
    pfont->procs.enumerate_glyph = z1_enumerate_glyph;
    pfont->procs.glyph_outline = zchar1_glyph_outline;
    return 0;
}

/* Build a Type 1, Type 2, or Type 4 font. */
int
build_charstring_font(i_ctx_t *i_ctx_p, os_ptr op, build_proc_refs *pbuild,
		      font_type ftype, charstring_font_refs_t *pfr,
		      gs_type1_data *pdata1, build_font_options_t options)
{
    int code = charstring_font_params(imemory, op, pfr, pdata1);
    gs_font_type1 *pfont;

    if (code < 0)
	return code;
    code = build_gs_primitive_font(i_ctx_p, op, (gs_font_base **)&pfont, ftype,
				   &st_gs_font_type1, pbuild, options);
    if (code != 0)
	return code;
    /* This is a new font, fill it in. */
    charstring_font_init(pfont, pfr, pdata1);
    return define_gs_font((gs_font *)pfont);
}

/* ------ Operators ------ */

/* Build a Type 1 or Type 4 font. */
private int
buildfont1or4(i_ctx_t *i_ctx_p, os_ptr op, build_proc_refs * pbuild,
	      font_type ftype, build_font_options_t options)
{
    charstring_font_refs_t refs;
    int code = charstring_font_get_refs(op, &refs);
    gs_type1_data data1;

    if (code < 0)
	return code;
    data1.interpret = gs_type1_interpret;
    data1.subroutineNumberBias = 0;
    data1.lenIV = DEFAULT_LENIV_1;
    return build_charstring_font(i_ctx_p, op, pbuild, ftype, &refs, &data1,
				 options);
}

/* <string|name> <font_dict> .buildfont1 <string|name> <font> */
/* Build a type 1 (Adobe encrypted) font. */
private int
zbuildfont1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    build_proc_refs build;
    int code = build_proc_name_refs(imemory, &build,
				    "%Type1BuildChar", "%Type1BuildGlyph");

    if (code < 0)
	return code;
    return buildfont1or4(i_ctx_p, op, &build, ft_encrypted,
			 bf_notdef_required);
}

/* <string|name> <font_dict> .buildfont4 <string|name> <font> */
/* Build a type 4 (disk-based Adobe encrypted) font. */
private int
zbuildfont4(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    build_proc_refs build;
    int code = build_gs_font_procs(op, &build);

    if (code < 0)
	return code;
    return buildfont1or4(i_ctx_p, op, &build, ft_disk_based, bf_options_none);
}

/* ------ Initialization procedure ------ */

const op_def zfont1_op_defs[] =
{
    {"2.buildfont1", zbuildfont1},
    {"2.buildfont4", zbuildfont4},
    op_def_end(0)
};

/* ------ Font procedures for Type 1 fonts ------ */

/* same_font procedure */
private bool
same_font_dict(const font_data *pdata, const font_data *podata,
	       const char *key)
{
    ref *pvalue;
    bool present = dict_find_string(&pdata->dict, key, &pvalue) > 0;
    ref *povalue;
    bool opresent = dict_find_string(&podata->dict, key, &povalue) > 0;
    dict *pdict = (&(podata->dict))->value.pdict;

    return (present == opresent &&
	    (present <= 0 || obj_eq(dict_mem(pdict), pvalue, povalue)));
}
private int
z1_same_font(const gs_font *font, const gs_font *ofont, int mask)
{
    if (ofont->FontType != font->FontType)
	return 0;
    while (font->base != font)
	font = font->base;
    while (ofont->base != ofont)
	ofont = ofont->base;
    if (ofont == font)
	return mask;
    {
	int same = gs_base_same_font(font, ofont, mask);
	int check = mask & ~same;
	const gs_font_type1 *const pfont1 = (const gs_font_type1 *)font;
	const font_data *const pdata = pfont_data(pfont1);
	const gs_font_type1 *pofont1 = (const gs_font_type1 *)ofont;
	const font_data *const podata = pfont_data(pofont1);

	if ((check & (FONT_SAME_OUTLINES | FONT_SAME_METRICS)) &&
	    !memcmp(&pofont1->data.procs, &z1_data_procs, sizeof(z1_data_procs)) &&
	    obj_eq(font->memory, &pdata->CharStrings, &podata->CharStrings) &&
	    /*
	     * We use same_font_dict for convenience: we know that
	     * both fonts do have Private dictionaries.
	     */
	    same_font_dict(pdata, podata, "Private")
	    )
	    same |= FONT_SAME_OUTLINES;

	if ((check & FONT_SAME_METRICS) && (same & FONT_SAME_OUTLINES) &&
	    !memcmp(&pofont1->data.procs, &z1_data_procs, sizeof(z1_data_procs)) &&
	    /* Metrics may be affected by CDevProc, Metrics, Metrics2. */
	    same_font_dict(pdata, podata, "Metrics") &&
	    same_font_dict(pdata, podata, "Metrics2") &&
	    same_font_dict(pdata, podata, "CDevProc")
	    )
	    same |= FONT_SAME_METRICS;

	if ((check & FONT_SAME_ENCODING) &&
	    pofont1->procs.same_font == z1_same_font &&
	    obj_eq(font->memory, &pdata->Encoding, &podata->Encoding)
	    )
	    same |= FONT_SAME_ENCODING;

	return same & mask;
    }
}

