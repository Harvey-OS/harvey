/* Copyright (C) 1996, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zfont42.c,v 1.23 2005/06/19 21:03:32 igor Exp $ */
/* Type 42 font creation operator */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsccode.h"
#include "gsmatrix.h"
#include "gxfont.h"
#include "gxfont42.h"
#include "bfont.h"
#include "icharout.h"
#include "idict.h"
#include "idparam.h"
#include "ifont42.h"
#include "ichar1.h"
#include "iname.h"
#include "store.h"

/* Forward references */
private int z42_string_proc(gs_font_type42 *, ulong, uint, const byte **);
private uint z42_get_glyph_index(gs_font_type42 *, gs_glyph);
private int z42_gdir_get_outline(gs_font_type42 *, uint, gs_glyph_data_t *);
private font_proc_enumerate_glyph(z42_enumerate_glyph);
private font_proc_enumerate_glyph(z42_gdir_enumerate_glyph);
private font_proc_encode_char(z42_encode_char);
private font_proc_glyph_info(z42_glyph_info);
private font_proc_glyph_outline(z42_glyph_outline);

/* <string|name> <font_dict> .buildfont11/42 <string|name> <font> */
/* Build a type 11 (TrueType CID-keyed) or 42 (TrueType) font. */
int
build_gs_TrueType_font(i_ctx_t *i_ctx_p, os_ptr op, gs_font_type42 **ppfont,
		       font_type ftype, gs_memory_type_ptr_t pstype,
		       const char *bcstr, const char *bgstr,
		       build_font_options_t options)
{
    build_proc_refs build;
    ref sfnts, GlyphDirectory;
    gs_font_type42 *pfont;
    font_data *pdata;
    int code;

    code = build_proc_name_refs(imemory, &build, bcstr, bgstr);
    if (code < 0)
	return code;
    check_type(*op, t_dictionary);
    /*
     * Since build_gs_primitive_font may resize the dictionary and cause
     * pointers to become invalid, we save sfnts and GlyphDirectory.
     */
    if ((code = font_string_array_param(imemory, op, "sfnts", &sfnts)) < 0 ||
	(code = font_GlyphDirectory_param(op, &GlyphDirectory)) < 0
	)
	return code;
    code = build_gs_primitive_font(i_ctx_p, op, (gs_font_base **)ppfont,
				   ftype, pstype, &build, options);
    if (code != 0)
	return code;
    pfont = *ppfont;
    pdata = pfont_data(pfont);
    ref_assign(&pdata->u.type42.sfnts, &sfnts);
    make_null_new(&pdata->u.type42.CIDMap);
    ref_assign(&pdata->u.type42.GlyphDirectory, &GlyphDirectory);
    pfont->data.string_proc = z42_string_proc;
    pfont->data.proc_data = (char *)pdata;
    code = gs_type42_font_init(pfont);
    if (code < 0)
	return code;
    /*
     * If the font has a GlyphDictionary, this replaces loca and glyf for
     * accessing character outlines.  In this case, we use alternate
     * get_outline and enumerate_glyph procedures.
     */
    if (!r_has_type(&GlyphDirectory, t_null)) {
	pfont->data.get_outline = z42_gdir_get_outline;
	pfont->procs.enumerate_glyph = z42_gdir_enumerate_glyph;
    } else
	pfont->procs.enumerate_glyph = z42_enumerate_glyph;
    /*
     * The procedures that access glyph information must accept either
     * glyph names or glyph indexes.
     */
    pfont->data.get_glyph_index = z42_get_glyph_index;
    pfont->procs.encode_char = z42_encode_char;
    pfont->procs.glyph_info = z42_glyph_info;
    pfont->procs.glyph_outline = z42_glyph_outline;
    return 0;
}
private int
zbuildfont42(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_font_type42 *pfont;
    int code = build_gs_TrueType_font(i_ctx_p, op, &pfont, ft_TrueType,
				      &st_gs_font_type42, "%Type42BuildChar",
				      "%Type42BuildGlyph", bf_options_none);

    if (code < 0)
	return code;
    return define_gs_font((gs_font *)pfont);
}

/*
 * Check a parameter for being an array of strings.  Return the parameter
 * value even if it is of the wrong type.
 */
int
font_string_array_param(const gs_memory_t *mem, os_ptr op, const char *kstr, ref *psa)
{
    ref *pvsa;
    ref rstr0;
    int code;

    if (dict_find_string(op, kstr, &pvsa) <= 0)
	return_error(e_invalidfont);
    *psa = *pvsa;
    /*
     * We only check the first element of the array now, as a sanity test;
     * elements are checked as needed by string_array_access_proc.
     */
    if ((code = array_get(mem, pvsa, 0L, &rstr0)) < 0)
	return code;
    if (!r_has_type(&rstr0, t_string))
	return_error(e_typecheck);
    return 0;
}

/*
 * Get a GlyphDirectory if present.  Return 0 if present, 1 if absent,
 * or an error code.
 */
int
font_GlyphDirectory_param(os_ptr op, ref *pGlyphDirectory)
{
    ref *pgdir;

    if (dict_find_string(op, "GlyphDirectory", &pgdir) <= 0)
	make_null(pGlyphDirectory);
    else if (!r_has_type(pgdir, t_dictionary) && !r_is_array(pgdir))
	return_error(e_typecheck);
    else
	*pGlyphDirectory = *pgdir;
    return 0;
}

/*
 * Access a given byte offset and length in an array of strings.
 * This is used for sfnts and for CIDMap.  The int argument is 2 for sfnts
 * (because of the strange behavior of odd-length strings), 1 for CIDMap.
 * Return code : 0 - success, <0 - error, 
 *               >0 - number of accessible bytes (client must cycle).
 */
int
string_array_access_proc(const gs_memory_t *mem, 
			 const ref *psa, int modulus, ulong offset,
			 uint length, const byte **pdata)
{
    ulong left = offset;
    uint index = 0;

    if (length == 0)
        return 0;
    for (;; ++index) {
	ref rstr;
	int code = array_get(mem, psa, index, &rstr);
	uint size;

	if (code < 0)
	    return code;
	if (!r_has_type(&rstr, t_string))
	    return_error(e_typecheck);
	/*
	 * NOTE: According to the Adobe documentation, each sfnts
	 * string should have even length.  If the length is odd,
	 * the additional byte is padding and should be ignored.
	 */
	size = r_size(&rstr) & -modulus;
	if (left < size) {
	    *pdata = rstr.value.const_bytes + left;
	    if (left + length > size)
		return size - left;
	    return 0;
	}
	left -= size;
    }
}

/* ------ Initialization procedure ------ */

const op_def zfont42_op_defs[] =
{
    {"2.buildfont42", zbuildfont42},
    op_def_end(0)
};

/* Reduce a glyph name to a glyph index if needed. */
private gs_glyph
glyph_to_index(const gs_font *font, gs_glyph glyph)
{
    ref gref;
    ref *pcstr;

    if (glyph >= GS_MIN_GLYPH_INDEX)
	return glyph;
    name_index_ref(font->memory, glyph, &gref);
    if (dict_find(&pfont_data(font)->CharStrings, &gref, &pcstr) > 0 &&
	r_has_type(pcstr, t_integer)
	) {
	gs_glyph index_glyph = pcstr->value.intval + GS_MIN_GLYPH_INDEX;

	if (index_glyph >= GS_MIN_GLYPH_INDEX && index_glyph <= gs_max_glyph)
	    return index_glyph;
    }
    return GS_MIN_GLYPH_INDEX;	/* glyph 0 is notdef */
}
private uint
z42_get_glyph_index(gs_font_type42 *pfont, gs_glyph glyph)
{
    gs_glyph gid = glyph_to_index((gs_font *)pfont, glyph);

    return gid - GS_MIN_GLYPH_INDEX;
}

/*
 * Get a glyph outline from GlyphDirectory.  Return an empty string if
 * the glyph is missing or out of range.
 */
int
font_gdir_get_outline(const gs_memory_t *mem, 
		      const ref *pgdir, 
		      long glyph_index,
		      gs_glyph_data_t *pgd)
{
    ref iglyph;
    ref gdef;
    ref *pgdef;
    int code;

    if (r_has_type(pgdir, t_dictionary)) {
	make_int(&iglyph, glyph_index);
	code = dict_find(pgdir, &iglyph, &pgdef) - 1; /* 0 => not found */
    } else {
	code = array_get(mem, pgdir, glyph_index, &gdef);
	pgdef = &gdef;
    }
    if (code < 0) {
	gs_glyph_data_from_null(pgd);
    } else if (!r_has_type(pgdef, t_string)) {
	return_error(e_typecheck);
    } else {
	gs_glyph_data_from_string(pgd, pgdef->value.const_bytes, r_size(pgdef),
				  NULL);
    }
    return 0;
}
private int
z42_gdir_get_outline(gs_font_type42 * pfont, uint glyph_index,
		     gs_glyph_data_t *pgd)
{
    const font_data *pfdata = pfont_data(pfont);
    const ref *pgdir = &pfdata->u.type42.GlyphDirectory;

    return font_gdir_get_outline(pfont->memory, pgdir, (long)glyph_index, pgd);
}

/* Enumerate glyphs from CharStrings or loca / glyf. */
private int
z42_enumerate_glyph(gs_font *font, int *pindex, gs_glyph_space_t glyph_space,
		    gs_glyph *pglyph)
{
    if (glyph_space == GLYPH_SPACE_INDEX)
	return gs_type42_enumerate_glyph(font, pindex, glyph_space, pglyph);
    else {
	const ref *pcsdict = &pfont_data(font)->CharStrings;

	return zchar_enumerate_glyph(font->memory, pcsdict, pindex, pglyph);
    }
}

/* Enumerate glyphs (keys) from GlyphDirectory instead of loca / glyf. */
private int
z42_gdir_enumerate_glyph(gs_font *font, int *pindex,
			 gs_glyph_space_t glyph_space, gs_glyph *pglyph)
{
    const ref *pgdict;
    int code;

    if (glyph_space == GLYPH_SPACE_INDEX) {
	pgdict = &pfont_data(font)->u.type42.GlyphDirectory;
	if (!r_has_type(pgdict, t_dictionary)) {
	    ref gdef;

	    for (;; (*pindex)++) {
		if (array_get(font->memory, pgdict, (long)*pindex, &gdef) < 0) {
		    *pindex = 0;
		    return 0;
		}
		if (!r_has_type(&gdef, t_null)) {
		    *pglyph = GS_MIN_GLYPH_INDEX + (*pindex)++;
		    return 0;
		}
	    }
	}
    } else
	pgdict = &pfont_data(font)->CharStrings;
    /* A trick : use zchar_enumerate_glyph to enumerate GIDs : */
    code = zchar_enumerate_glyph(font->memory, pgdict, pindex, pglyph);
    if (*pindex != 0 && *pglyph >= gs_min_cid_glyph)
	*pglyph	= *pglyph - gs_min_cid_glyph + GS_MIN_GLYPH_INDEX;
    return code;
}

/*
 * Define font procedures that accept either a character name or a glyph
 * index as the glyph.
 */
private gs_glyph
z42_encode_char(gs_font *font, gs_char chr, gs_glyph_space_t glyph_space)
{
    gs_glyph glyph = zfont_encode_char(font, chr, glyph_space);

    return (glyph_space == GLYPH_SPACE_INDEX && glyph != gs_no_glyph ?
	    glyph_to_index(font, glyph) : glyph);
}
private int
z42_glyph_outline(gs_font *font, int WMode, gs_glyph glyph, const gs_matrix *pmat,
		  gx_path *ppath, double sbw[4])
{
    return gs_type42_glyph_outline(font, WMode, glyph_to_index(font, glyph),
				   pmat, ppath, sbw);
}
private int
z42_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
	       int members, gs_glyph_info_t *info)
{   /* fixme : same as z1_glyph_info. */
    int wmode = font->WMode;

    return z1_glyph_info_generic(font, glyph, pmat, members, info, gs_type42_glyph_info, wmode);
}

/* Procedure for accessing the sfnts array.
 * Return code : 0 - success, <0 - error, 
 *               >0 - number of accessible bytes (client must cycle).
 */
private int
z42_string_proc(gs_font_type42 * pfont, ulong offset, uint length,
		const byte ** pdata)
{
    return string_array_access_proc(pfont->memory, &pfont_data(pfont)->u.type42.sfnts, 2,
				    offset, length, pdata);
}
