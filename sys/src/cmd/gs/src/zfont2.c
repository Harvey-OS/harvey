/* Copyright (C) 1991, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: zfont2.c,v 1.3 2000/09/19 19:00:53 lpd Exp $ */
/* Type 2 font creation operators */
#include "ghost.h"
#include "oper.h"
#include "gxfixed.h"
#include "gsmatrix.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "bfont.h"
#include "idict.h"
#include "idparam.h"
#include "ifont1.h"
#include "ifont2.h"

/* Private utilities */
private uint
subr_bias(const ref * psubrs)
{
    uint size = r_size(psubrs);

    return (size < 1240 ? 107 : size < 33900 ? 1131 : 32768);
}

/*
 * Get the additional parameters for a Type 2 font (or FontType 2 FDArray
 * entry in a CIDFontType 0 font), beyond those common to Type 1 and Type 2
 * fonts.
 */
int
type2_font_params(const_os_ptr op, charstring_font_refs_t *pfr,
		  gs_type1_data *pdata1)
{
    int code;
    float dwx, nwx;

    pdata1->interpret = gs_type2_interpret;
    pdata1->lenIV = DEFAULT_LENIV_2;
    pdata1->subroutineNumberBias = subr_bias(pfr->Subrs);
    /* Get information specific to Type 2 fonts. */
    if (dict_find_string(pfr->Private, "GlobalSubrs", &pfr->GlobalSubrs) > 0) {
	if (!r_is_array(pfr->GlobalSubrs))
	    return_error(e_typecheck);
    }
    pdata1->gsubrNumberBias = subr_bias(pfr->GlobalSubrs);
    if ((code = dict_uint_param(pfr->Private, "gsubrNumberBias",
				0, max_uint, pdata1->gsubrNumberBias,
				&pdata1->gsubrNumberBias)) < 0 ||
	(code = dict_float_param(pfr->Private, "defaultWidthX", 0.0,
				 &dwx)) < 0 ||
	(code = dict_float_param(pfr->Private, "nominalWidthX", 0.0,
				 &nwx)) < 0
	)
	return code;
    pdata1->defaultWidthX = float2fixed(dwx);
    pdata1->nominalWidthX = float2fixed(nwx);
    {
	ref *pirs;

	if (dict_find_string(pfr->Private, "initialRandomSeed", &pirs) <= 0)
	    pdata1->initialRandomSeed = 0;
	else if (!r_has_type(pirs, t_integer))
	    return_error(e_typecheck);
	else
	    pdata1->initialRandomSeed = pirs->value.intval;
    }
    return 0;
}

/* <string|name> <font_dict> .buildfont2 <string|name> <font> */
/* Build a type 2 (compact Adobe encrypted) font. */
private int
zbuildfont2(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    charstring_font_refs_t refs;
    build_proc_refs build;
    int code = build_proc_name_refs(&build,
				    "%Type2BuildChar", "%Type2BuildGlyph");
    gs_type1_data data1;

    if (code < 0)
	return code;
    code = charstring_font_get_refs(op, &refs);
    if (code < 0)
	return code;
    code = type2_font_params(op, &refs, &data1);
    if (code < 0)
	return code;
    return build_charstring_font(i_ctx_p, op, &build, ft_encrypted2, &refs,
				 &data1, bf_notdef_required);
}

/* ------ Initialization procedure ------ */

const op_def zfont2_op_defs[] =
{
    {"2.buildfont2", zbuildfont2},
    op_def_end(0)
};
