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

/* $Id: zfont0.c,v 1.7 2004/08/04 19:36:13 stefan Exp $ */
/* Composite font creation operator */
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"
/*
 * The following lines used to say:
 *      #include "gsmatrix.h"
 *      #include "gxdevice.h"           /. for gxfont.h ./
 * Tony Li says the longer list is necessary to keep the GNU compiler
 * happy, but this is pretty hard to understand....
 */
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"		/* must precede gxdevice */
#include "gxdevice.h"		/* must precede gxfont */
#include "gxfcmap.h"
#include "gxfont.h"
#include "gxfont0.h"
#include "bfont.h"
#include "ialloc.h"
#include "iddict.h"
#include "idparam.h"
#include "igstate.h"
#include "iname.h"
#include "store.h"

/* Imported from zfcmap.c */
int ztype0_get_cmap(const gs_cmap_t ** ppcmap, const ref * pfdepvector,
		    const ref * op, gs_memory_t *imem);

/* Forward references */
private font_proc_define_font(ztype0_define_font);
private font_proc_make_font(ztype0_make_font);
private int ensure_char_entry(i_ctx_t *, os_ptr, const char *, byte *, int);

/* <string|name> <font_dict> .buildfont0 <string|name> <font> */
/* Build a type 0 (composite) font. */
private int
zbuildfont0(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_type0_data data;
    ref fdepvector;
    ref *pprefenc;
    gs_font_type0 *pfont;
    font_data *pdata;
    ref save_FID;
    int i;
    int code = 0;

    check_type(*op, t_dictionary);
    {
	ref *pfmaptype;
	ref *pfdepvector;

	if (dict_find_string(op, "FMapType", &pfmaptype) <= 0 ||
	    !r_has_type(pfmaptype, t_integer) ||
	    pfmaptype->value.intval < (int)fmap_type_min ||
	    pfmaptype->value.intval > (int)fmap_type_max ||
	    dict_find_string(op, "FDepVector", &pfdepvector) <= 0 ||
	    !r_is_array(pfdepvector)
	    )
	    return_error(e_invalidfont);
	data.FMapType = (fmap_type) pfmaptype->value.intval;
	/*
	 * Adding elements below could cause the font dictionary to be
	 * resized, which would invalidate pfdepvector.
	 */
	fdepvector = *pfdepvector;
    }
    /* Check that every element of the FDepVector is a font. */
    data.fdep_size = r_size(&fdepvector);
    for (i = 0; i < data.fdep_size; i++) {
	ref fdep;
	gs_font *psub;

	array_get(imemory, &fdepvector, i, &fdep);
	if ((code = font_param(&fdep, &psub)) < 0)
	    return code;
	/*
	 * Check the inheritance rules.  Allowed configurations
	 * (paths from root font) are defined by the regular
	 * expression:
	 *      (shift | double_escape escape* | escape*)
	 *        non_modal* non_composite
	 */
	if (psub->FontType == ft_composite) {
	    const gs_font_type0 *const psub0 = (const gs_font_type0 *)psub;
	    fmap_type fmt = psub0->data.FMapType;

	    if (fmt == fmap_double_escape ||
		fmt == fmap_shift ||
		(fmt == fmap_escape &&
		 !(data.FMapType == fmap_escape ||
		   data.FMapType == fmap_double_escape))
		)
		return_error(e_invalidfont);
	}
    }
    switch (data.FMapType) {
	case fmap_escape:
	case fmap_double_escape:	/* need EscChar */
	    code = ensure_char_entry(i_ctx_p, op, "EscChar", &data.EscChar, 255);
	    break;
	case fmap_shift:	/* need ShiftIn & ShiftOut */
	    code = ensure_char_entry(i_ctx_p, op, "ShiftIn", &data.ShiftIn, 15);
	    if (code >= 0)
		code = ensure_char_entry(i_ctx_p, op, "ShiftOut", &data.ShiftOut, 14);
	    break;
	case fmap_SubsVector:	/* need SubsVector */
	    {
		ref *psubsvector;
		uint svsize;

		if (dict_find_string(op, "SubsVector", &psubsvector) <= 0 ||
		    !r_has_type(psubsvector, t_string) ||
		    (svsize = r_size(psubsvector)) == 0 ||
		(data.subs_width = (int)*psubsvector->value.bytes + 1) > 4 ||
		    (svsize - 1) % data.subs_width != 0
		    )
		    return_error(e_invalidfont);
		data.subs_size = (svsize - 1) / data.subs_width;
		data.SubsVector.data = psubsvector->value.bytes + 1;
		data.SubsVector.size = svsize - 1;
	    } break;
	case fmap_CMap:	/* need CMap */
	    code = ztype0_get_cmap(&data.CMap, (const ref *)&fdepvector,
				   (const ref *)op, imemory);
	    break;
	default:
	    ;
    }
    if (code < 0)
	return code;
    /*
     * Save the old FID in case we have to back out.
     * build_gs_font will return an error if there is a FID entry
     * but it doesn't reference a valid font.
     */
    {
	ref *pfid;

	if (dict_find_string(op, "FID", &pfid) <= 0)
	    make_null(&save_FID);
	else
	    save_FID = *pfid;
    }
    {
	build_proc_refs build;

	code = build_proc_name_refs(imemory, &build,
				    "%Type0BuildChar", "%Type0BuildGlyph");
	if (code < 0)
	    return code;
	code = build_gs_font(i_ctx_p, op, (gs_font **) & pfont,
			     ft_composite, &st_gs_font_type0, &build,
			     bf_options_none);
    }
    if (code != 0)
	return code;
    /* Fill in the rest of the basic font data. */
    pfont->procs.init_fstack = gs_type0_init_fstack;
    pfont->procs.define_font = ztype0_define_font;
    pfont->procs.make_font = ztype0_make_font;
    pfont->procs.next_char_glyph = gs_type0_next_char_glyph;
    if (dict_find_string(op, "PrefEnc", &pprefenc) <= 0) {
	ref nul;

	make_null_new(&nul);
	if ((code = idict_put_string(op, "PrefEnc", &nul)) < 0)
	    goto fail;
    }
    /* Fill in the font data */
    pdata = pfont_data(pfont);
    data.encoding_size = r_size(&pdata->Encoding);
    data.Encoding =
	(uint *) ialloc_byte_array(data.encoding_size, sizeof(uint),
				   "buildfont0(Encoding)");
    if (data.Encoding == 0) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    /* Fill in the encoding vector, checking to make sure that */
    /* each element is an integer between 0 and fdep_size-1. */
    for (i = 0; i < data.encoding_size; i++) {
	ref enc;

	array_get(imemory, &pdata->Encoding, i, &enc);
	if (!r_has_type(&enc, t_integer)) {
	    code = gs_note_error(e_typecheck);
	    goto fail;
	}
	if ((ulong) enc.value.intval >= data.fdep_size) {
	    code = gs_note_error(e_rangecheck);
	    goto fail;
	}
	data.Encoding[i] = (uint) enc.value.intval;
    }
    data.FDepVector =
	ialloc_struct_array(data.fdep_size, gs_font *,
			    &st_gs_font_ptr_element,
			    "buildfont0(FDepVector)");
    if (data.FDepVector == 0) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    for (i = 0; i < data.fdep_size; i++) {
	ref fdep;
	ref *pfid;

	array_get(pfont->memory, &fdepvector, i, &fdep);
	/* The lookup can't fail, because of the pre-check above. */
	dict_find_string(&fdep, "FID", &pfid);
	data.FDepVector[i] = r_ptr(pfid, gs_font);
    }
    pfont->data = data;
    code = define_gs_font((gs_font *) pfont);
    if (code >= 0)
	return code;
fail:
	/* Undo the insertion of the FID entry in the dictionary. */
    if (r_has_type(&save_FID, t_null)) {
	ref rnfid;

	name_enter_string(pfont->memory, "FID", &rnfid);
	idict_undef(op, &rnfid);
    } else
	idict_put_string(op, "FID", &save_FID);
    gs_free_object(pfont->memory, pfont, "buildfont0(font)");
    return code;
}
/* If a newly defined or scaled composite font had to scale */
/* any composite sub-fonts, adjust the parent font's FDepVector. */
/* This is called only if gs_type0_define/make_font */
/* actually changed the FDepVector. */
private int
ztype0_adjust_FDepVector(gs_font_type0 * pfont)
{
    gs_memory_t *mem = pfont->memory;
    /* HACK: We know the font was allocated by the interpreter. */
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    gs_font **pdep = pfont->data.FDepVector;
    ref newdep;
    uint fdep_size = pfont->data.fdep_size;
    ref *prdep;
    uint i;
    int code = gs_alloc_ref_array(imem, &newdep, a_readonly, fdep_size,
				  "ztype0_adjust_matrix");

    if (code < 0)
	return code;
    for (prdep = newdep.value.refs, i = 0; i < fdep_size; i++, prdep++) {
	const ref *pdict = pfont_dict(pdep[i]);

	ref_assign(prdep, pdict);
	r_set_attrs(prdep, imemory_new_mask(imem));
    }
    /*
     * The FDepVector is an existing key in the parent's dictionary,
     * so it's safe to pass NULL as the dstack pointer to dict_put_string.
     */
    return dict_put_string(pfont_dict(pfont), "FDepVector", &newdep, NULL);
}
private int
ztype0_define_font(gs_font_dir * pdir, gs_font * pfont)
{
    gs_font_type0 *const pfont0 = (gs_font_type0 *)pfont;
    gs_font **pdep = pfont0->data.FDepVector;
    int code = gs_type0_define_font(pdir, pfont);

    if (code < 0 || pfont0->data.FDepVector == pdep)
	return code;
    return ztype0_adjust_FDepVector(pfont0);
}
private int
ztype0_make_font(gs_font_dir * pdir, const gs_font * pfont,
		 const gs_matrix * pmat, gs_font ** ppfont)
{
    gs_font_type0 **const ppfont0 = (gs_font_type0 **)ppfont;
    gs_font **pdep = (*ppfont0)->data.FDepVector;
    int code;

    code = zdefault_make_font(pdir, pfont, pmat, ppfont);
    if (code < 0)
	return code;
    code = gs_type0_make_font(pdir, pfont, pmat, ppfont);
    if (code < 0)
	return code;
    if ((*ppfont0)->data.FDepVector == pdep)
	return 0;
    return ztype0_adjust_FDepVector(*ppfont0);
}

/* ------ Internal routines ------ */

/* Find or add a character entry in a font dictionary. */
private int
ensure_char_entry(i_ctx_t *i_ctx_p, os_ptr op, const char *kstr,
		  byte * pvalue, int default_value)
{
    ref *pentry;

    if (dict_find_string(op, kstr, &pentry) <= 0) {
	ref ent;

	make_int(&ent, default_value);
	*pvalue = (byte) default_value;
	return idict_put_string(op, kstr, &ent);
    } else {
	check_int_leu_only(*pentry, 255);
	*pvalue = (byte) pentry->value.intval;
	return 0;
    }
}

/* ------ Initialization procedure ------ */

const op_def zfont0_op_defs[] =
{
    {"2.buildfont0", zbuildfont0},
    op_def_end(0)
};
