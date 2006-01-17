/* Copyright (C) 1996, 1997, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zcharout.c,v 1.13 2004/09/22 13:52:33 igor Exp $ */
/* Common code for outline (Type 1 / 4 / 42) fonts */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gscrypt1.h"
#include "gstext.h"
#include "gxdevice.h"		/* for gxfont.h */
#include "gxfont.h"
#include "gxfont1.h"
#include "dstack.h"		/* only for systemdict */
#include "estack.h"
#include "ichar.h"
#include "icharout.h"
#include "idict.h"
#include "ifont.h"
#include "igstate.h"
#include "iname.h"
#include "store.h"

/*
 * Execute an outline defined by a PostScript procedure.
 * The top elements of the stack are:
 *      <font> <code|name> <name> <outline_id>
 */
int
zchar_exec_char_proc(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
	/*
	 * The definition is a PostScript procedure.  Execute
	 *      <code|name> proc
	 * within a systemdict begin/end and a font begin/end.
	 */
    es_ptr ep;

    check_estack(5);
    ep = esp += 5;
    make_op_estack(ep - 4, zend);
    make_op_estack(ep - 3, zend);
    ref_assign(ep - 2, op);
    make_op_estack(ep - 1, zbegin);
    make_op_estack(ep, zbegin);
    ref_assign(op - 1, systemdict);
    {
	ref rfont;

	ref_assign(&rfont, op - 3);
	ref_assign(op - 3, op - 2);
	ref_assign(op - 2, &rfont);
    }
    pop(1);
    return o_push_estack;
}

/*
 * Get the metrics for a character from the Metrics dictionary of a base
 * font.  If present, store the l.s.b. in psbw[0,1] and the width in
 * psbw[2,3].
 */
int				/*metrics_present*/
zchar_get_metrics(const gs_font_base * pbfont, const ref * pcnref,
		  double psbw[4])
{
    const ref *pfdict = &pfont_data(gs_font_parent(pbfont))->dict;
    ref *pmdict;

    if (dict_find_string(pfdict, "Metrics", &pmdict) > 0) {
	ref *pmvalue;

	check_type_only(*pmdict, t_dictionary);
	check_dict_read(*pmdict);
	if (dict_find(pmdict, pcnref, &pmvalue) > 0) {
	    if (num_params(pmvalue, 1, psbw + 2) >= 0) {	/* <wx> only */
		psbw[3] = 0;
		return metricsWidthOnly;
	    } else {
		int code;

		check_read_type_only(*pmvalue, t_array);
		switch (r_size(pmvalue)) {
		    case 2:	/* [<sbx> <wx>] */
			code = num_params(pmvalue->value.refs + 1,
					  2, psbw);
			psbw[2] = psbw[1];
			psbw[1] = psbw[3] = 0;
			break;
		    case 4:	/* [<sbx> <sby> <wx> <wy>] */
			code = num_params(pmvalue->value.refs + 3,
					  4, psbw);
			break;
		    default:
			return_error(e_rangecheck);
		}
		if (code < 0)
		    return code;
		return metricsSideBearingAndWidth;
	    }
	}
    }
    return metricsNone;
}

/* Get the vertical metrics for a character from Metrics2, if present. */
int
zchar_get_metrics2(const gs_font_base * pbfont, const ref * pcnref,
		   double pwv[4])
{
    const ref *pfdict = &pfont_data(gs_font_parent(pbfont))->dict;
    ref *pmdict;

    if (dict_find_string(pfdict, "Metrics2", &pmdict) > 0) {
	ref *pmvalue;

	check_type_only(*pmdict, t_dictionary);
	check_dict_read(*pmdict);
	if (dict_find(pmdict, pcnref, &pmvalue) > 0) {
	    check_read_type_only(*pmvalue, t_array);
	    if (r_size(pmvalue) == 4) {
		int code = num_params(pmvalue->value.refs + 3, 4, pwv);

		return (code < 0 ? code : metricsSideBearingAndWidth);
	    }
	}
    }
    return metricsNone;
}

/*
 * Get CDevProc.
 */
bool
zchar_get_CDevProc(const gs_font_base * pbfont, ref **ppcdevproc)
{
    const ref *pfdict = &pfont_data(gs_font_parent(pbfont))->dict;

    return dict_find_string(pfdict, "CDevProc", ppcdevproc) > 0;
}

/*
 * Consult Metrics2 and CDevProc, and call setcachedevice[2].  Return
 * o_push_estack if we had to call a CDevProc, or if we are skipping the
 * rendering process (only getting the metrics).
 * Returns exec_cont - a function, which must be called by caller after this function.
 */
int
zchar_set_cache(i_ctx_t *i_ctx_p, const gs_font_base * pbfont,
		const ref * pcnref, const double psb[2],
		const double pwidth[2], const gs_rect * pbbox,
		op_proc_t cont, op_proc_t *exec_cont,
		const double Metrics2_sbw_default[4])
{
    os_ptr op = osp;
    ref *pcdevproc;
    int have_cdevproc;
    ref rpop;
    bool metrics2;
    bool metrics2_use_default = false;
    double w2[10];
    gs_text_enum_t *penum = op_show_find(i_ctx_p);

    w2[0] = pwidth[0], w2[1] = pwidth[1];

    /* Adjust the bounding box for stroking if needed. */

    w2[2] = pbbox->p.x, w2[3] = pbbox->p.y;
    w2[4] = pbbox->q.x, w2[5] = pbbox->q.y;
    if (pbfont->PaintType != 0) {
	double expand = max(1.415, gs_currentmiterlimit(igs)) *
	gs_currentlinewidth(igs) / 2;

	w2[2] -= expand, w2[3] -= expand;
	w2[4] += expand, w2[5] += expand;
    }

    /* Check for Metrics2. */

    {
	int code = zchar_get_metrics2(pbfont, pcnref, w2 + 6);

	if (code < 0)
	    return code;
	metrics2 = code > 0;
    }

    /*
     * For FontType 9 and 11, if Metrics2 is missing, the caller provides
     * default Metrics2 values derived from the FontBBox.
     */
    if (!metrics2 && Metrics2_sbw_default != NULL) {
        w2[6] = Metrics2_sbw_default[2];
        w2[7] = Metrics2_sbw_default[3];
        w2[8] = Metrics2_sbw_default[0];
        w2[9] = Metrics2_sbw_default[1];
	metrics2 = true;
	metrics2_use_default = true;
    }

    /* Check for CDevProc or "short-circuiting". */

    have_cdevproc = zchar_get_CDevProc(pbfont, &pcdevproc);
    if (have_cdevproc || zchar_show_width_only(penum)) {
	int i;
	op_proc_t zsetc;
	int nparams;

	if (have_cdevproc) {
	    check_proc_only(*pcdevproc);
	    zsetc = zsetcachedevice2;
	    
	    /* If we have cdevproc and the font type is CID type 0,
	       we'll throw away Metrics2_sbw_default that is calculated 
	       from FontBBox. */
	    if (!metrics2 
		|| (penum->current_font->FontType == ft_CID_encrypted
		    && metrics2_use_default)) {
		w2[6] = w2[0], w2[7] = w2[1];
		w2[8] = w2[9] = 0;
	    }
	    nparams = 10;
	} else {
	    make_oper(&rpop, 0, zpop);
	    pcdevproc = &rpop;
	    if (metrics2)
		zsetc = zsetcachedevice2, nparams = 10;
	    else
		zsetc = zsetcachedevice, nparams = 6;
	}
	check_estack(3);
	/* Push the l.s.b. for .type1addpath if necessary. */
	if (psb != 0) {
	    push(nparams + 3);
	    make_real(op - (nparams + 2), psb[0]);
	    make_real(op - (nparams + 1), psb[1]);
	} else {
	    push(nparams + 1);
	}
	for (i = 0; i < nparams; ++i)
	    make_real(op - nparams + i, w2[i]);
	ref_assign(op, pcnref);
	push_op_estack(cont);
	push_op_estack(zsetc);
	++esp;
	ref_assign(esp, pcdevproc);
	return o_push_estack;
    } {
	int code =
	    (metrics2 ? gs_text_setcachedevice2(penum, w2) :
	     gs_text_setcachedevice(penum, w2));

	if (code < 0)
	    return code;
    }

    /* No metrics modification, do the stroke or fill now. */

    /* Push the l.s.b. for .type1addpath if necessary. */
    if (psb != 0) {
	push(2);
	make_real(op - 1, psb[0]);
	make_real(op, psb[1]);
    }
    *exec_cont = cont;
    return 0;
}

/*
 * Get the CharString data corresponding to a glyph.  Return typecheck
 * if it isn't a string.
 */
private bool charstring_is_notdef_proc(const gs_memory_t *mem, const ref *);
private int charstring_make_notdef(gs_glyph_data_t *, gs_font *);
int
zchar_charstring_data(gs_font *font, const ref *pgref, gs_glyph_data_t *pgd)
{
    ref *pcstr;

    if (dict_find(&pfont_data(font)->CharStrings, pgref, &pcstr) <= 0)
	return_error(e_undefined);
    if (!r_has_type(pcstr, t_string)) {
	/*
	 * The ADOBEPS4 Windows driver replaces the .notdef entry of
	 * otherwise normal Type 1 fonts with the procedure
	 *	{pop 0 0 setcharwidth}
	 * To prevent this from making the font unembeddable in PDF files
	 * (with our present font-writing code), we recognize this as a
	 * special case and return a Type 1 CharString consisting of
	 *	0 0 hsbw endchar
	 */
	if (font->FontType == ft_encrypted &&
	    charstring_is_notdef_proc(font->memory, pcstr)
	    )
	    return charstring_make_notdef(pgd, font);
	else
	    return_error(e_typecheck);
    }
    gs_glyph_data_from_string(pgd, pcstr->value.const_bytes, r_size(pcstr),
			      NULL);
    return 0;
}
private bool
charstring_is_notdef_proc(const gs_memory_t *mem, const ref *pcstr)
{
    if (r_is_array(pcstr) && r_size(pcstr) == 4) {
	ref elts[4];
	long i;

	for (i = 0; i < 4; ++i)
	    array_get(mem, pcstr, i, &elts[i]);
	if (r_has_type(&elts[0], t_name) &&
	    r_has_type(&elts[1], t_integer) && elts[1].value.intval == 0 &&
	    r_has_type(&elts[2], t_integer) && elts[2].value.intval == 0 &&
	    r_has_type(&elts[3], t_name)
	    ) {
	    ref nref;

	    name_enter_string(mem, "pop", &nref);
	    if (name_eq(&elts[0], &nref)) {
	        name_enter_string(mem, "setcharwidth", &nref);
		if (name_eq(&elts[3], &nref))
		    return true;
	    }
	}
    }
    return false;
}
private int
charstring_make_notdef(gs_glyph_data_t *pgd, gs_font *font)
{
    gs_font_type1 *const pfont = (gs_font_type1 *)font;
    static const byte char_data[4] = {
	139,			/* 0 */
	139,			/* 0 */
	c1_hsbw,
	cx_endchar
    };
    uint len = max(pfont->data.lenIV, 0) + sizeof(char_data);
    byte *chars = gs_alloc_string(font->memory, len, "charstring_make_notdef");

    if (chars == 0)
	return_error(e_VMerror);
    gs_glyph_data_from_string(pgd, chars, len, font);
    if (pfont->data.lenIV < 0)
	memcpy(chars, char_data, sizeof(char_data));
    else {
	crypt_state state = crypt_charstring_seed;

	memcpy(chars + pfont->data.lenIV, char_data, sizeof(char_data));
	gs_type1_encrypt(chars, chars, len, &state);
    }
    return 0;
}

/*
 * Enumerate the next glyph from a directory.  This is essentially a
 * wrapper around dict_first/dict_next to implement the enumerate_glyph
 * font procedure.
 *
 * Note that *prdict will be null if the font is a subfont of a
 * CIDFontType 0 CIDFont.
 */
int
zchar_enumerate_glyph(const gs_memory_t *mem, const ref *prdict, int *pindex, gs_glyph *pglyph)
{
    int index = *pindex - 1;
    ref elt[2];

    if (!r_has_type(prdict, t_dictionary))
	return 0;		/* *pindex was 0, is still 0 */
    if (index < 0)
	index = dict_first(prdict);
next:
    index = dict_next(prdict, index, elt);
    *pindex = index + 1;
    if (index >= 0) {
	switch (r_type(elt)) {
	    case t_integer:
		*pglyph = gs_min_cid_glyph + elt[0].value.intval;
		break;
	    case t_name:
	        *pglyph = name_index(mem, elt);
		break;
	    default:		/* can't handle it */
		goto next;
	}
    }
    return 0;
}
