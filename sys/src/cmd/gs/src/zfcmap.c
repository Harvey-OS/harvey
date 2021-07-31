/* Copyright (C) 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: zfcmap.c,v 1.9 2001/04/01 00:35:26 raph Exp $ */
/* CMap creation operator */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsmatrix.h"		/* for gxfont.h */
#include "gsstruct.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfcmap.h"
#include "gxfont.h"
#include "ialloc.h"
#include "icid.h"
#include "iddict.h"
#include "idparam.h"
#include "ifont.h"		/* for zfont_mark_glyph_name */
#include "iname.h"
#include "store.h"

/*
 * Define whether to check the compatibility of CIDSystemInfo between the
 * CMap and the composite font.  PLRM2 says compatibility is required, but
 * PLRM3 says the interpreter doesn't check it.
 */
/*#define CHECK_CID_SYSTEM_INFO_COMPATIBILITY*/

/* ---------------- Internal procedures ---------------- */

/* Free a code map in case of VMerror. */
private void
free_code_map(gx_code_map_t * pcmap, gs_memory_t * mem)
{
    if (pcmap->lookup) {
	int i;

	for (i = 0; i < pcmap->num_lookup; ++i) {
	    gx_code_lookup_range_t *pclr = &pcmap->lookup[i];

	    if (pclr->value_type == CODE_VALUE_GLYPH)
		gs_free_string(mem, pclr->values.data, pclr->values.size,
			       "free_code_map(values)");
	}
	gs_free_object(mem, pcmap->lookup, "free_code_map(map)");
    }
}

/* Convert code ranges to internal form. */
private int
acquire_code_ranges(gs_cmap_t *cmap, const ref * pref, gs_memory_t * mem)
{
    uint num_ranges;
    gx_code_space_range_t *ranges;
    uint i;

    if (!r_is_array(pref) || (num_ranges = r_size(pref)) == 0 ||
	num_ranges & 1)
	return_error(e_rangecheck);
    num_ranges >>= 1;
    ranges = (gx_code_space_range_t *)
	gs_alloc_byte_array(mem, num_ranges, sizeof(gx_code_space_range_t),
			    "acquire_code_ranges");
    if (ranges == 0)
	return_error(e_VMerror);
    cmap->code_space.ranges = ranges;
    cmap->code_space.num_ranges = num_ranges;
    for (i = 0; i < num_ranges; ++i, ++ranges) {
	ref rfirst, rlast;
	int size;

	array_get(pref, i * 2L, &rfirst);
	array_get(pref, i * 2L + 1, &rlast);
	if (!r_has_type(&rfirst, t_string) ||
	    !r_has_type(&rlast, t_string) ||
	    (size = r_size(&rfirst)) == 0 || size > MAX_CMAP_CODE_SIZE ||
	    r_size(&rlast) != size ||
	    memcmp(rfirst.value.bytes, rlast.value.bytes, size) > 0)
	    return_error(e_rangecheck);
	memcpy(ranges->first, rfirst.value.bytes, size);
	memcpy(ranges->last, rlast.value.bytes, size);
	ranges->size = size;
    }
    return 0;
}

/* Convert a code map to internal form. */
private int
acquire_code_map(gx_code_map_t * pcmap, const ref * pref, gs_cmap_t * root,
		 gs_memory_t * mem)
{
    uint num_lookup;
    gx_code_lookup_range_t *pclr;
    long i;

    if (!r_is_array(pref) || (num_lookup = r_size(pref)) % 5 != 0)
	return_error(e_rangecheck);
    num_lookup /= 5;
    pclr = gs_alloc_struct_array(mem, num_lookup, gx_code_lookup_range_t,
				 &st_code_lookup_range_element,
				 "acquire_code_map(lookup ranges)");
    if (pclr == 0)
	return_error(e_VMerror);
    memset(pclr, 0, sizeof(*pclr) * num_lookup);
    pcmap->lookup = pclr;
    pcmap->num_lookup = num_lookup;
    for (i = 0; i < num_lookup * 5; i += 5, ++pclr) {
	ref rprefix, rmisc, rkeys, rvalues, rfxs;

	array_get(pref, i, &rprefix);
	array_get(pref, i + 1, &rmisc);
	array_get(pref, i + 2, &rkeys);
	array_get(pref, i + 3, &rvalues);
	array_get(pref, i + 4, &rfxs);

	if (!r_has_type(&rprefix, t_string) ||
	    !r_has_type(&rmisc, t_string) ||
	    !r_has_type(&rkeys, t_string) ||
	    !(r_has_type(&rvalues, t_string) || r_is_array(&rvalues)) ||
	    !r_has_type(&rfxs, t_integer)
	    )
	    return_error(e_typecheck);
	if (r_size(&rmisc) != 4 ||
	    rmisc.value.bytes[0] > MAX_CMAP_CODE_SIZE - r_size(&rprefix) ||
	    rmisc.value.bytes[1] > 1 ||
	    rmisc.value.bytes[2] > CODE_VALUE_MAX ||
	    rmisc.value.bytes[3] == 0)
	    return_error(e_rangecheck);
	pclr->cmap = root;
	pclr->key_size = rmisc.value.bytes[0];
	pclr->key_prefix_size = r_size(&rprefix);
	memcpy(pclr->key_prefix, rprefix.value.bytes, pclr->key_prefix_size);
	pclr->key_is_range = rmisc.value.bytes[1];
	if (pclr->key_size == 0) {
	    /* This is a single entry consisting only of the prefix. */
	    if (r_size(&rkeys) != 0)
		return_error(e_rangecheck);
	    pclr->num_keys = 1;
	} else {
	    int step = pclr->key_size * (pclr->key_is_range ? 2 : 1);

	    if (r_size(&rkeys) % step != 0)
		return_error(e_rangecheck);
	    pclr->num_keys = r_size(&rkeys) / step;
	}
	pclr->keys.data = rkeys.value.bytes,
	    pclr->keys.size = r_size(&rkeys);
	pclr->value_type = rmisc.value.bytes[2];
	pclr->value_size = rmisc.value.bytes[3];
	if (r_has_type(&rvalues, t_string)) {
	    if (pclr->value_type == CODE_VALUE_GLYPH)
		return_error(e_rangecheck);
	    if (r_size(&rvalues) % pclr->num_keys != 0 ||
		r_size(&rvalues) / pclr->num_keys != pclr->value_size)
		return_error(e_rangecheck);
	    pclr->values.data = rvalues.value.bytes,
		pclr->values.size = r_size(&rvalues);
	} else {
	    uint values_size = pclr->num_keys * pclr->value_size;
	    long k;
	    byte *pvalue;

	    if (pclr->value_type != CODE_VALUE_GLYPH ||
		r_size(&rvalues) != pclr->num_keys ||
		pclr->value_size > sizeof(gs_glyph))
		return_error(e_rangecheck);
	    pclr->values.data = gs_alloc_string(mem, values_size,
						"acquire_code_map(values)");
	    if (pclr->values.data == 0)
		return_error(e_VMerror);
	    pclr->values.size = values_size;
	    pvalue = pclr->values.data;
	    for (k = 0; k < pclr->num_keys; ++k) {
		ref rvalue;
		gs_glyph value;
		int i;

		array_get(&rvalues, k, &rvalue);
		if (!r_has_type(&rvalue, t_name))
		    return_error(e_rangecheck);
		value = name_index(&rvalue);
		/*
		 * We need a special check here because some CPUs cannot
		 * shift by the full size of an int or long.
		 */
		if (pclr->value_size < sizeof(value) &&
		    (value >> (pclr->value_size * 8)) != 0
		    )
		    return_error(e_rangecheck);
		for (i = pclr->value_size; --i >= 0; )
		    *pvalue++ = (byte)(value >> (i * 8));
	    }
	}
	check_int_leu_only(rfxs, 0xff);
	pclr->font_index = (int)rfxs.value.intval;
    }
    return 0;
}

/*
 * Acquire the CIDSystemInfo array from a dictionary.  If missing, fabricate
 * a 0-element array and return 1.
 */
private int
acquire_cid_system_info(ref *psia, const ref *op)
{
    ref *prcidsi;

    if (dict_find_string(op, "CIDSystemInfo", &prcidsi) <= 0) {
	make_empty_array(psia, a_readonly);
	return 1;
    }
    if (r_has_type(prcidsi, t_dictionary)) {
	make_array(psia, a_readonly, 1, prcidsi);
	return 0;
    }
    if (!r_is_array(prcidsi))
	return_error(e_typecheck);
    *psia = *prcidsi;
    return 0;
}

/*
 * Get one element of a CIDSystemInfo array.  If the element is missing or
 * null, return 1.
 */
private int
get_cid_system_info(gs_cid_system_info_t *pcidsi, const ref *psia, uint index)
{
    ref rcidsi;
    int code = array_get(psia, (long)index, &rcidsi);

    if (code < 0 || r_has_type(&rcidsi, t_null)) {
	cid_system_info_set_null(pcidsi);
	return 1;
    }
    return cid_system_info_param(pcidsi, &rcidsi);
}

#ifdef CHECK_CID_SYSTEM_INFO_COMPATIBILITY

/* Check compatibility of CIDSystemInfo. */
private bool
bytes_eq(const gs_const_string *pcs1, const gs_const_string *pcs2)
{
    return !bytes_compare(pcs1->data, pcs1->size,
			  pcs2->data, pcs2->size);
}
private bool
cid_system_info_compatible(const gs_cid_system_info_t * psi1,
			   const gs_cid_system_info_t * psi2)
{
    return bytes_eq(&psi1->Registry, &psi2->Registry) &&
	bytes_eq(&psi1->Ordering, &psi2->Ordering);
}

#endif /* CHECK_CID_SYSTEM_INFO_COMPATIBILITY */

/* ---------------- (Semi-)public procedures ---------------- */

/* Get the CodeMap from a Type 0 font, and check the CIDSystemInfo of */
/* its subsidiary fonts. */
int
ztype0_get_cmap(const gs_cmap_t **ppcmap, const ref *pfdepvector,
		const ref *op, gs_memory_t *imem)
{
    ref *prcmap;
    ref *pcodemap;
    const gs_cmap_t *pcmap;
    int code;
    uint num_fonts;
    uint i;

    if (dict_find_string(op, "CMap", &prcmap) <= 0 ||
	!r_has_type(prcmap, t_dictionary) ||
	dict_find_string(prcmap, "CodeMap", &pcodemap) <= 0 ||
	!r_has_stype(pcodemap, imem, st_cmap)
	)
	return_error(e_invalidfont);
    pcmap = r_ptr(pcodemap, gs_cmap_t);
    num_fonts = r_size(pfdepvector);
    for (i = 0; i < num_fonts; ++i) {
	ref rfdep, rfsi;

	array_get(pfdepvector, (long)i, &rfdep);
	code = acquire_cid_system_info(&rfsi, &rfdep);
	if (code < 0)
	    return code;
	if (code == 0) {
	    if (r_size(&rfsi) != 1)
		return_error(e_rangecheck);
#ifdef CHECK_CID_SYSTEM_INFO_COMPATIBILITY
	    {
		gs_cid_system_info_t cidsi;

		get_cid_system_info(&cidsi, &rfsi, 0);
		if (!cid_system_info_is_null(&cidsi) &&
		    !cid_system_info_compatible(&cidsi,
						pcmap->CIDSystemInfo + i))
		    return_error(e_rangecheck);
	    }
#endif
	}
    }
    *ppcmap = pcmap;
    return 0;
}

/* ---------------- Operators ---------------- */

/* <CMap> .buildcmap <CMap> */
/*
 * Create the internal form of a CMap.  The initial CMap must be read-write
 * and have an entry with key = CodeMap and value = null; the result is
 * read-only and has a real CodeMap.
 *
 * This operator reads the CMapType, CMapName, CIDSystemInfo, CMapVersion,
 * UIDOffset, XUID, WMode, and .CodeMapData elements of the CMap dictionary.
 * For details, see lib/gs_cmap.ps and the Adobe documentation.
 */
private int
zfcmap_glyph_name(gs_glyph glyph, gs_const_string *pstr, void *proc_data)
{
    ref nref, nsref;
    int code = 0;

    /*code = */name_index_ref((uint)glyph, &nref);
    if (code < 0)
	return code;
    name_string_ref(&nref, &nsref);
    pstr->data = nsref.value.const_bytes;
    pstr->size = r_size(&nsref);
    return 0;
}
private int
zbuildcmap(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    ref *pcmapname;
    ref *puidoffset;
    ref *pcodemapdata;
    ref *pcodemap;
    ref rname, rcidsi, rcoderanges, rdefs, rnotdefs;
    gs_cmap_t *pcmap = 0;
    gs_cid_system_info_t *pcidsi = 0;
    ref rcmap;
    uint i;

    check_type(*op, t_dictionary);
    check_dict_write(*op);
    pcmap = ialloc_struct(gs_cmap_t, &st_cmap, "zbuildcmap(cmap)");
    if (pcmap == 0) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    gs_cmap_init(pcmap);
    if ((code = dict_int_param(op, "CMapType", 0, 1, -1, &pcmap->CMapType)) < 0 ||
	(code = dict_float_param(op, "CMapVersion", 0.0, &pcmap->CMapVersion)) < 0 ||
	(code = dict_uid_param(op, &pcmap->uid, 0, imemory, i_ctx_p)) < 0 ||
	(code = dict_int_param(op, "WMode", 0, 1, 0, &pcmap->WMode)) < 0
	)
	goto fail;
    if ((code = dict_find_string(op, "CMapName", &pcmapname)) <= 0) {
	code = gs_note_error(e_rangecheck);
	goto fail;
    }
    if (!r_has_type(pcmapname, t_name)) {
	code = gs_note_error(e_typecheck);
	goto fail;
    }
    name_string_ref(pcmapname, &rname);
    pcmap->CMapName.data = rname.value.const_bytes;
    pcmap->CMapName.size = r_size(&rname);
    if (dict_find_string(op, "UIDOffset", &puidoffset) > 0) {
	if (!r_has_type(puidoffset, t_integer)) {
	    code = gs_note_error(e_typecheck);
	    goto fail;
	}
	pcmap->UIDOffset = puidoffset->value.intval; /* long, not int */
    }
    if (dict_find_string(op, ".CodeMapData", &pcodemapdata) <= 0 ||
	!r_has_type(pcodemapdata, t_array) ||
	r_size(pcodemapdata) != 3 ||
	dict_find_string(op, "CodeMap", &pcodemap) <= 0 ||
	!r_has_type(pcodemap, t_null)
	) {
	code = gs_note_error(e_rangecheck);
	goto fail;
    }
    if ((code = acquire_cid_system_info(&rcidsi, op)) < 0)
	goto fail;
    pcidsi = ialloc_struct_array(r_size(&rcidsi), gs_cid_system_info_t,
				 &st_cid_system_info_element,
				 "zbuildcmap(CIDSystemInfo)");
    if (pcidsi == 0) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    pcmap->CIDSystemInfo = pcidsi;
    pcmap->num_fonts = r_size(&rcidsi);
    for (i = 0; i < r_size(&rcidsi); ++i) {
	code = get_cid_system_info(pcidsi + i, &rcidsi, i);
	if (code < 0)
	    goto fail;
    }
    array_get(pcodemapdata, 0L, &rcoderanges);
    array_get(pcodemapdata, 1L, &rdefs);
    array_get(pcodemapdata, 2L, &rnotdefs);
    if ((code = acquire_code_ranges(pcmap, &rcoderanges, imemory)) < 0)
	goto fail;
    if ((code = acquire_code_map(&pcmap->def, &rdefs, pcmap, imemory)) < 0)
	goto fail;
    if ((code = acquire_code_map(&pcmap->notdef, &rnotdefs, pcmap, imemory)) < 0)
	goto fail;
    pcmap->mark_glyph = zfont_mark_glyph_name;
    pcmap->mark_glyph_data = 0;
    pcmap->glyph_name = zfcmap_glyph_name;
    pcmap->glyph_name_data = 0;
    make_istruct_new(&rcmap, a_readonly, pcmap);
    code = idict_put_string(op, "CodeMap", &rcmap);
    if (code < 0)
	goto fail;
    return zreadonly(i_ctx_p);
fail:
    free_code_map(&pcmap->notdef, imemory);
    free_code_map(&pcmap->def, imemory);
    ifree_object(pcmap, "zbuildcmap(cmap)");
    ifree_object(pcidsi, "zbuildcmap(CIDSystemInfo)");
    return code;
}

#if defined(DEBUG) || defined(PROFILE) || defined(TEST)

#include "stream.h"
#include "spprint.h"
#include "files.h"
#include "gdevpsf.h"

/* <file> <cmap> .writecmap - */
private int
zfcmap_put_name_default(stream *s, const byte *str, uint size)
{
    stream_putc(s, '/');
    stream_write(s, str, size);
    return 0;
}
private int
zwritecmap(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref *pcodemap;
    gs_cmap_t *pcmap;
    int code;
    stream *s;

    check_type(*op, t_dictionary);
    if (dict_find_string(op, "CodeMap", &pcodemap) <= 0 ||
	!r_is_struct(pcodemap)
	)
	return_error(e_typecheck);
    check_write_file(s, op - 1);
    pcmap = r_ptr(pcodemap, gs_cmap_t);
    code = psf_write_cmap(s, pcmap, zfcmap_put_name_default, NULL);
    if (code >= 0)
	pop(2);
    return code;
}

#endif

/* ------ Initialization procedure ------ */

const op_def zfcmap_op_defs[] =
{
    {"1.buildcmap", zbuildcmap},
#if defined(DEBUG) || defined(PROFILE) || defined(TEST)
    {"2.writecmap", zwritecmap},
#endif
    op_def_end(0)
};
