/* Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* zfont1.c */
/* Type 1 font creation operator */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gxfixed.h"
#include "gsmatrix.h"
#include "gxdevice.h"
#include "gschar.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "bfont.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "store.h"

/* Type 1 auxiliary procedures (defined in zchar1.c) */
extern int z1_subr_proc(P3(gs_type1_data *, int, gs_const_string *));
extern int z1_seac_proc(P3(gs_type1_data *, int, gs_const_string *));
extern int z1_push_proc(P3(gs_type1_data *, const fixed *, int));
extern int z1_pop_proc(P2(gs_type1_data *, fixed *));

/* Default value of lenIV */
#define default_lenIV 4

/* <string|name> <font_dict> .buildfont1 <string|name> <font> */
/* Build a type 1 (Adobe encrypted) font. */
int
zbuildfont1(os_ptr op)
{	gs_type1_data data1;
	int painttype;
	ref *pothersubrs;
	ref *psubrs;
	ref *pcharstrings;
	ref *pprivate;
	static ref no_subrs;
	gs_font_type1 *pfont;
	font_data *pdata;
	int code;

	check_type(*op, t_dictionary);
	code = dict_int_param(op, "PaintType", 0, 3, 0, &painttype);
	if ( code < 0 )
	  return code;
	if ( dict_find_string(op, "CharStrings", &pcharstrings) <= 0 ||
	    !r_has_type(pcharstrings, t_dictionary) ||
	    dict_find_string(op, "Private", &pprivate) <= 0 ||
	    !r_has_type(pprivate, t_dictionary)
	   )
		return_error(e_invalidfont);
	make_empty_array(&no_subrs, 0);
	if ( dict_find_string(pprivate, "OtherSubrs", &pothersubrs) > 0 )
	{	check_array_else(*pothersubrs, return_error(e_invalidfont));
	}
	else
		pothersubrs = &no_subrs;
	if ( dict_find_string(pprivate, "Subrs", &psubrs) > 0 )
	{	check_array_else(*psubrs, return_error(e_invalidfont));
	}
	else
		psubrs = &no_subrs;
	/* Get the rest of the information from the Private dictionary. */
	if ( (code = dict_int_param(pprivate, "lenIV", 0, 255,
				    default_lenIV, &data1.lenIV)) < 0 ||
	     (code = dict_int_param(pprivate, "BlueFuzz", 0, 1999, 1,
				    &data1.BlueFuzz)) < 0 ||
	     (code = dict_float_param(pprivate, "BlueScale", 0.039625,
				      &data1.BlueScale)) < 0 ||
	     (code = dict_float_param(pprivate, "BlueShift", 7.0,
				    &data1.BlueShift)) < 0 ||
	     (code = data1.BlueValues.count = dict_float_array_param(pprivate,
		"BlueValues", max_BlueValues * 2,
		&data1.BlueValues.values[0], NULL)) < 0 ||
	     (code = dict_float_param(pprivate, "ExpansionFactor", 0.06,
				      &data1.ExpansionFactor)) < 0 ||
	     (code = data1.FamilyBlues.count = dict_float_array_param(pprivate,
		"FamilyBlues", max_FamilyBlues * 2,
		&data1.FamilyBlues.values[0], NULL)) < 0 ||
	     (code = data1.FamilyOtherBlues.count = dict_float_array_param(pprivate,
		"FamilyOtherBlues", max_FamilyOtherBlues * 2,
		&data1.FamilyOtherBlues.values[0], NULL)) < 0 ||
	     (code = dict_bool_param(pprivate, "ForceBold", false,
				     &data1.ForceBold)) < 0 ||
	     (code = dict_int_param(pprivate, "LanguageGroup", 0, 1, 0,
				    &data1.LanguageGroup)) < 0 ||
	     (code = data1.OtherBlues.count = dict_float_array_param(pprivate,
		"OtherBlues", max_OtherBlues * 2,
		&data1.OtherBlues.values[0], NULL)) < 0 ||
	     (code = dict_bool_param(pprivate, "RndStemUp", false,
				     &data1.RndStemUp)) < 0 ||
	     (code = data1.StdHW.count = dict_float_array_param(pprivate,
		"StdHW", 1, &data1.StdHW.values[0], NULL)) < 0 ||
	     (code = data1.StdVW.count = dict_float_array_param(pprivate,
		"StdVW", 1, &data1.StdVW.values[0], NULL)) < 0 ||
	     (code = data1.StemSnapH.count = dict_float_array_param(pprivate,
		"StemSnapH", max_StemSnap,
		&data1.StemSnapH.values[0], NULL)) < 0 ||
	     (code = data1.StemSnapV.count = dict_float_array_param(pprivate,
		"StemSnapV", max_StemSnap,
		&data1.StemSnapV.values[0], NULL)) < 0
	   )
		return code;
	/* Do the work common to all non-composite font types. */
	{	build_proc_refs build;
		code = build_proc_name_refs(&build,
			"Type1BuildChar", "Type1BuildGlyph");
		if ( code < 0 )
		  return code;
		code = build_gs_simple_font(op, (gs_font_base **)&pfont,
					    ft_encrypted,
					    &st_gs_font_type1, &build);
	}
	if ( code != 0 )
	  return code;
	pfont->PaintType = painttype;
	/* This is a new font, fill it in. */
	pdata = pfont_data(pfont);
	pfont->data = data1;
	ref_assign(&pdata->CharStrings, pcharstrings);
	ref_assign(&pdata->OtherSubrs, pothersubrs);
	ref_assign(&pdata->Subrs, psubrs);
	pfont->data.subr_proc = z1_subr_proc;
	pfont->data.seac_proc = z1_seac_proc;
	pfont->data.push_proc = z1_push_proc;
	pfont->data.pop_proc = z1_pop_proc;
	pfont->data.proc_data = (char *)pdata;
	/* Check that the UniqueIDs match.  This is part of the */
	/* Adobe protection scheme, but we may as well emulate it. */
	if ( uid_is_valid(&pfont->UID) &&
	     !dict_check_uid_param(op, &pfont->UID)
	   )
		uid_set_invalid(&pfont->UID);
	return define_gs_font((gs_font *)pfont);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfont1_op_defs) {
	{"2.buildfont1", zbuildfont1},
END_OP_DEFS(0) }
