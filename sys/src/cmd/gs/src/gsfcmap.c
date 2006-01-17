/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfcmap.c,v 1.27 2004/12/08 21:35:13 stefan Exp $ */
/* CMap character decoding */
#include <assert.h>
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxfcmap.h"

typedef struct gs_cmap_identity_s {
    GS_CMAP_COMMON;
    int num_bytes;
    int varying_bytes;
    int code;			/* 0 or num_bytes */
} gs_cmap_identity_t;

/* GC descriptors */
public_st_cmap();
gs_private_st_suffix_add0_local(st_cmap_identity, gs_cmap_identity_t,
				"gs_cmap_identity_t", cmap_ptrs, cmap_data,
				st_cmap);

/* ---------------- Client procedures ---------------- */

    /* ------ Initialization/creation ------ */

/*
 * Create an Identity CMap.
 */
private uint
get_integer_bytes(const byte *src, int count)
{
    uint v = 0;
    int i;

    for (i = 0; i < count; ++i)
	v = (v << 8) + src[i];
    return v;
}
private int
identity_decode_next(const gs_cmap_t *pcmap, const gs_const_string *str,
		     uint *pindex, uint *pfidx,
		     gs_char *pchr, gs_glyph *pglyph)
{
    const gs_cmap_identity_t *const pcimap =
	(const gs_cmap_identity_t *)pcmap;
    int num_bytes = pcimap->num_bytes;
    uint value;

    if (str->size < *pindex + num_bytes) {
	*pglyph = gs_no_glyph;
	return (*pindex == str->size ? 2 : -1);
    }
    value = get_integer_bytes(str->data + *pindex, num_bytes);
    *pglyph = gs_min_cid_glyph + value;
    *pchr = value;
    *pindex += num_bytes;
    *pfidx = 0;
    return pcimap->code;
}
private int
identity_next_range(gs_cmap_ranges_enum_t *penum)
{
    if (penum->index == 0) {
	const gs_cmap_identity_t *const pcimap =
	    (const gs_cmap_identity_t *)penum->cmap;

	memset(penum->range.first, 0, pcimap->num_bytes);
	memset(penum->range.last, 0xff, pcimap->num_bytes);
	penum->index = 1;
	return 0;
    }
    return 1;
}
private const gs_cmap_ranges_enum_procs_t identity_range_procs = {
    identity_next_range
};
private void
identity_enum_ranges(const gs_cmap_t *pcmap, gs_cmap_ranges_enum_t *pre)
{
    gs_cmap_ranges_enum_setup(pre, pcmap, &identity_range_procs);
}
private int
identity_next_lookup(gs_cmap_lookups_enum_t *penum)
{
    if (penum->index[0] == 0) {
	const gs_cmap_identity_t *const pcimap =
	    (const gs_cmap_identity_t *)penum->cmap;
	int num_bytes = pcimap->num_bytes;

	memset(penum->entry.key[0], 0, num_bytes);
	memset(penum->entry.key[1], 0xff, num_bytes);
	memset(penum->entry.key[1], 0, num_bytes - pcimap->varying_bytes);
	penum->entry.key_size = num_bytes;
	penum->entry.key_is_range = true;
	penum->entry.value_type =
	    (pcimap->code ? CODE_VALUE_CHARS : CODE_VALUE_CID);
	penum->entry.value.size = num_bytes;
	penum->entry.font_index = 0;
	penum->index[0] = 1;
	return 0;
    }
    return 1;
}
private int
no_next_lookup(gs_cmap_lookups_enum_t *penum)
{
    return 1;
}
private int
identity_next_entry(gs_cmap_lookups_enum_t *penum)
{
    const gs_cmap_identity_t *const pcimap =
	(const gs_cmap_identity_t *)penum->cmap;
    int num_bytes = pcimap->num_bytes;
    int i = num_bytes - pcimap->varying_bytes;

    memcpy(penum->temp_value, penum->entry.key[0], num_bytes);
    memcpy(penum->entry.key[0], penum->entry.key[1], i);
    while (--i >= 0)
	if (++(penum->entry.key[1][i]) != 0) {
	    penum->entry.value.data = penum->temp_value;
	    return 0;
	}
    return 1;
}

private const gs_cmap_lookups_enum_procs_t identity_lookup_procs = {
    identity_next_lookup, identity_next_entry
};
const gs_cmap_lookups_enum_procs_t gs_cmap_no_lookups_procs = {
    no_next_lookup, 0
};
private void
identity_enum_lookups(const gs_cmap_t *pcmap, int which,
		      gs_cmap_lookups_enum_t *pre)
{
    gs_cmap_lookups_enum_setup(pre, pcmap,
			       (which ? &gs_cmap_no_lookups_procs :
				&identity_lookup_procs));
}
private bool
identity_is_identity(const gs_cmap_t *pcmap, int font_index_only)
{
    return true;
}

private const gs_cmap_procs_t identity_procs = {
    identity_decode_next, identity_enum_ranges, identity_enum_lookups, identity_is_identity
};

private int
gs_cmap_identity_alloc(gs_cmap_t **ppcmap, int num_bytes, int varying_bytes,
		       int return_code, const char *cmap_name, int wmode,
		       gs_memory_t *mem)
{
    /*
     * We could allow any value of num_bytes between 1 and
     * min(MAX_CMAP_CODE_SIZE, 4), but if num_bytes != 2, we can't name
     * the result "Identity-[HV]".
     */
    static const gs_cid_system_info_t identity_cidsi = {
	{ (const byte *)"Adobe", 5 },
	{ (const byte *)"Identity", 8 },
	0
    };
    int code;
    gs_cmap_identity_t *pcimap;

    if (num_bytes != 2)
	return_error(gs_error_rangecheck);
    code = gs_cmap_alloc(ppcmap, &st_cmap_identity, wmode,
			 (const byte *)cmap_name, strlen(cmap_name),
			 &identity_cidsi, 1, &identity_procs, mem);
    if (code < 0)
	return code;
    pcimap = (gs_cmap_identity_t *)*ppcmap;
    pcimap->num_bytes = num_bytes;
    pcimap->varying_bytes = varying_bytes;
    pcimap->code = return_code;
    return 0;
}
int
gs_cmap_create_identity(gs_cmap_t **ppcmap, int num_bytes, int wmode,
			gs_memory_t *mem)
{
    return gs_cmap_identity_alloc(ppcmap, num_bytes, num_bytes, 0,
				  (wmode ? "Identity-V" : "Identity-H"),
				  wmode, mem);
}
int
gs_cmap_create_char_identity(gs_cmap_t **ppcmap, int num_bytes, int wmode,
			     gs_memory_t *mem)
{
    return gs_cmap_identity_alloc(ppcmap, num_bytes, 1, num_bytes,
				  (wmode ? "Identity-BF-V" : "Identity-BF-H"),
				  wmode, mem);
}

    /* ------ Check identity ------ */

/*
 * Check for identity CMap. Uses a fast check for special cases.
 */
int
gs_cmap_is_identity(const gs_cmap_t *pcmap, int font_index_only)
{
    return pcmap->procs->is_identity(pcmap, font_index_only);
}

    /* ------ Decoding ------ */

/*
 * Decode and map a character from a string using a CMap.
 * See gsfcmap.h for details.
 */
int
gs_cmap_decode_next(const gs_cmap_t *pcmap, const gs_const_string *str,
		    uint *pindex, uint *pfidx,
		    gs_char *pchr, gs_glyph *pglyph)
{
    return pcmap->procs->decode_next(pcmap, str, pindex, pfidx, pchr, pglyph);
}

    /* ------ Enumeration ------ */

/*
 * Initialize the enumeration of the code space ranges, and enumerate
 * the next range.  See gxfcmap.h for details.
 */
void
gs_cmap_ranges_enum_init(const gs_cmap_t *pcmap, gs_cmap_ranges_enum_t *penum)
{
    pcmap->procs->enum_ranges(pcmap, penum);
}
int
gs_cmap_enum_next_range(gs_cmap_ranges_enum_t *penum)
{
    return penum->procs->next_range(penum);
}

/*
 * Initialize the enumeration of the lookups, and enumerate the next
 * the next lookup or entry.  See gxfcmap.h for details.
 */
void
gs_cmap_lookups_enum_init(const gs_cmap_t *pcmap, int which,
			  gs_cmap_lookups_enum_t *penum)
{
    pcmap->procs->enum_lookups(pcmap, which, penum);
}
int
gs_cmap_enum_next_lookup(gs_cmap_lookups_enum_t *penum)
{
    return penum->procs->next_lookup(penum);
}
int
gs_cmap_enum_next_entry(gs_cmap_lookups_enum_t *penum)
{
    return penum->procs->next_entry(penum);
}

/* ---------------- Implementation procedures ---------------- */

    /* ------ Initialization/creation ------ */

/*
 * Initialize a just-allocated CMap, to ensure that all pointers are clean
 * for the GC.  Note that this only initializes the common part.
 */
void
gs_cmap_init(const gs_memory_t *mem, gs_cmap_t *pcmap, int num_fonts)
{
    memset(pcmap, 0, sizeof(*pcmap));
    /* We reserve a range of IDs for pdfwrite needs,
       to allow an identification of submaps for a particular subfont.
     */
    pcmap->id = gs_next_ids(mem, num_fonts);
    pcmap->num_fonts = num_fonts;
    uid_set_invalid(&pcmap->uid);
}

/*
 * Allocate and initialize (the common part of) a CMap.
 */
int
gs_cmap_alloc(gs_cmap_t **ppcmap, const gs_memory_struct_type_t *pstype,
	      int wmode, const byte *map_name, uint name_size,
	      const gs_cid_system_info_t *pcidsi_in, int num_fonts,
	      const gs_cmap_procs_t *procs, gs_memory_t *mem)
{
    gs_cmap_t *pcmap =
	gs_alloc_struct(mem, gs_cmap_t, pstype, "gs_cmap_alloc(CMap)");
    gs_cid_system_info_t *pcidsi =
	gs_alloc_struct_array(mem, num_fonts, gs_cid_system_info_t,
			      &st_cid_system_info_element,
			      "gs_cmap_alloc(CIDSystemInfo)");

    if (pcmap == 0 || pcidsi == 0) {
	gs_free_object(mem, pcidsi, "gs_cmap_alloc(CIDSystemInfo)");
	gs_free_object(mem, pcmap, "gs_cmap_alloc(CMap)");
	return_error(gs_error_VMerror);
    }
    gs_cmap_init(mem, pcmap, num_fonts);	/* id, uid, num_fonts */
    pcmap->CMapType = 1;
    pcmap->CMapName.data = map_name;
    pcmap->CMapName.size = name_size;
    if (pcidsi_in)
	memcpy(pcidsi, pcidsi_in, sizeof(*pcidsi) * num_fonts);
    else
	memset(pcidsi, 0, sizeof(*pcidsi) * num_fonts);
    pcmap->CIDSystemInfo = pcidsi;
    pcmap->CMapVersion = 1.0;
    /* uid = 0, UIDOffset = 0 */
    pcmap->WMode = wmode;
    /* from_Unicode = 0 */
    /* not glyph_name, glyph_name_data */
    pcmap->procs = procs;
    *ppcmap = pcmap;
    return 0;
}

/*
 * Initialize an enumerator with convenient defaults (index = 0).
 */
void
gs_cmap_ranges_enum_setup(gs_cmap_ranges_enum_t *penum,
			  const gs_cmap_t *pcmap,
			  const gs_cmap_ranges_enum_procs_t *procs)
{
    penum->cmap = pcmap;
    penum->procs = procs;
    penum->index = 0;
}
void
gs_cmap_lookups_enum_setup(gs_cmap_lookups_enum_t *penum,
			   const gs_cmap_t *pcmap,
			   const gs_cmap_lookups_enum_procs_t *procs)
{
    penum->cmap = pcmap;
    penum->procs = procs;
    penum->index[0] = penum->index[1] = 0;
}

/* 
 * For a random CMap, compute whether it is identity.
 * It is not applicable to gs_cmap_ToUnicode_t due to
 * different sizes of domain keys and range values.
 */
bool
gs_cmap_compute_identity(const gs_cmap_t *pcmap, int font_index_only)
{
    const int which = 0;
    gs_cmap_lookups_enum_t lenum;
    int code;

    for (gs_cmap_lookups_enum_init(pcmap, which, &lenum);
	 (code = gs_cmap_enum_next_lookup(&lenum)) == 0; ) {
	if (font_index_only >= 0 && lenum.entry.font_index != font_index_only)
	    continue;
	if (font_index_only < 0 && lenum.entry.font_index > 0)
	    return false;
	while (gs_cmap_enum_next_entry(&lenum) == 0) {
	    switch (lenum.entry.value_type) {
	    case CODE_VALUE_CID:
		break;
	    case CODE_VALUE_CHARS:
		return false; /* Not implemented yet. */
	    case CODE_VALUE_GLYPH:
		return false;
	    default :
		return false; /* Must not happen. */
	    }
	    if (lenum.entry.key_size != lenum.entry.value.size)
		return false;
	    if (memcmp(lenum.entry.key[0], lenum.entry.value.data, 
		lenum.entry.key_size))
		return false;
	}
    }
    return true;
}

/* ================= ToUnicode CMap ========================= */

/*
 * This kind of CMaps keeps character a mapping from a random
 * PS encoding to Unicode, being defined in PDF reference, "ToUnicode CMaps".
 * It represents ranges in a closure data, without using 
 * gx_cmap_lookup_range_t. A special function gs_cmap_ToUnicode_set
 * allows to write code pairs into the closure data.
 */

private const int gs_cmap_ToUnicode_code_bytes = 2;

typedef struct gs_cmap_ToUnicode_s {
    GS_CMAP_COMMON;
    int num_codes;
    int key_size;
    bool is_identity;
} gs_cmap_ToUnicode_t;

gs_private_st_suffix_add0(st_cmap_ToUnicode, gs_cmap_ToUnicode_t,
    "gs_cmap_ToUnicode_t", cmap_ToUnicode_enum_ptrs, cmap_ToUnicode_reloc_ptrs,
    st_cmap);

private int
gs_cmap_ToUnicode_next_range(gs_cmap_ranges_enum_t *penum)
{   const gs_cmap_ToUnicode_t *cmap = (gs_cmap_ToUnicode_t *)penum->cmap;
    if (penum->index == 0) {
	memset(penum->range.first, 0, cmap->key_size);
	memset(penum->range.last, 0xff, cmap->key_size);
	penum->range.size = cmap->key_size; 
	penum->index = 1;
	return 0;
    }
    return 1;
}

private const gs_cmap_ranges_enum_procs_t gs_cmap_ToUnicode_range_procs = {
    gs_cmap_ToUnicode_next_range
};

private int
gs_cmap_ToUnicode_decode_next(const gs_cmap_t *pcmap, const gs_const_string *str,
		     uint *pindex, uint *pfidx,
		     gs_char *pchr, gs_glyph *pglyph)
{
    assert(0); /* Unsupported, because never used. */
    return 0;
}

private void
gs_cmap_ToUnicode_enum_ranges(const gs_cmap_t *pcmap, gs_cmap_ranges_enum_t *pre)
{
    gs_cmap_ranges_enum_setup(pre, pcmap, &gs_cmap_ToUnicode_range_procs);
}

private int
gs_cmap_ToUnicode_next_lookup(gs_cmap_lookups_enum_t *penum)
{   const gs_cmap_ToUnicode_t *cmap = (gs_cmap_ToUnicode_t *)penum->cmap;
    
    if (penum->index[0]++ > 0)
	return 1;
    penum->entry.value.data = penum->temp_value;
    penum->entry.value.size = gs_cmap_ToUnicode_code_bytes;
    penum->index[1] = 0;
    penum->entry.key_is_range = true;
    penum->entry.value_type = CODE_VALUE_CHARS;
    penum->entry.key_size = cmap->key_size;
    penum->entry.value.size = gs_cmap_ToUnicode_code_bytes;
    penum->entry.font_index = 0;
    return 0;
}

private int
gs_cmap_ToUnicode_next_entry(gs_cmap_lookups_enum_t *penum)
{   const gs_cmap_ToUnicode_t *cmap = (gs_cmap_ToUnicode_t *)penum->cmap;
    const uchar *map = cmap->glyph_name_data;
    const int num_codes = cmap->num_codes;
    uint index = penum->index[1], i, j;
    uchar c0, c1, c2;

    /* Warning : this hardcodes gs_cmap_ToUnicode_num_code_bytes = 2 */
    for (i = index; i < num_codes; i++)
	if (map[i + i + 0] != 0 || map[i + i + 1] != 0)
	    break;
    if (i >= num_codes)
	return 1;
    c0 = map[i + i + 0];
    c1 = map[i + i + 1];
    for (j = i + 1, c2 = c1 + 1; j < num_codes; j++, c2++) {
	/* Due to PDF spec, *bfrange boundaries may differ 
	   in the last byte only. */
	if (j % 256 == 0)
	    break;
	if ((uchar)c2 == 0)
	    break;
	if (map[j + j + 0] != c0 || map[j + j + 1] != c2)
	    break;
    }
    penum->index[1] = j;
    penum->entry.key[0][0] = (uchar)(i >> 8);
    penum->entry.key[0][cmap->key_size - 1] = (uchar)(i & 0xFF);
    penum->entry.key[1][0] = (uchar)(j >> 8);
    penum->entry.key[1][cmap->key_size - 1] = (uchar)((j - 1) & 0xFF);
    memcpy(penum->temp_value, map + i * gs_cmap_ToUnicode_code_bytes, 
			gs_cmap_ToUnicode_code_bytes);
    return 0;
}

private const gs_cmap_lookups_enum_procs_t gs_cmap_ToUnicode_lookup_procs = {
    gs_cmap_ToUnicode_next_lookup, gs_cmap_ToUnicode_next_entry
};

private void
gs_cmap_ToUnicode_enum_lookups(const gs_cmap_t *pcmap, int which,
		      gs_cmap_lookups_enum_t *pre)
{
    gs_cmap_lookups_enum_setup(pre, pcmap,
			       (which ? &gs_cmap_no_lookups_procs : /* fixme */
				&gs_cmap_ToUnicode_lookup_procs));
}

private bool
gs_cmap_ToUnicode_is_identity(const gs_cmap_t *pcmap, int font_index_only)
{   const gs_cmap_ToUnicode_t *cmap = (gs_cmap_ToUnicode_t *)pcmap;
    return cmap->is_identity;
}

private const gs_cmap_procs_t gs_cmap_ToUnicode_procs = {
    gs_cmap_ToUnicode_decode_next,
    gs_cmap_ToUnicode_enum_ranges,
    gs_cmap_ToUnicode_enum_lookups,
    gs_cmap_ToUnicode_is_identity
};

/*
 * Allocate and initialize a ToUnicode CMap.
 */
int
gs_cmap_ToUnicode_alloc(gs_memory_t *mem, int id, int num_codes, int key_size, gs_cmap_t **ppcmap)
{   int code;
    uchar *map, *cmap_name = NULL;
    gs_cmap_ToUnicode_t *cmap;
    int name_len = 0;
#   if 0
	/* We don't write a CMap name to ToUnicode CMaps,
	 * becsue (1) there is no conventional method for
	 * generating them, and (2) Acrobat Reader ignores them.
	 * But we'd like to keep this code until beta-testing completes,
	 * and we ensure that other viewers do not need the names.
	 */
	char sid[10], *pref = "aux-";
	int sid_len, pref_len = strlen(pref);
    
	sprintf(sid, "%d", id);
	sid_len = strlen(sid);
	name_len = pref_len + sid_len;
	cmap_name = gs_alloc_string(mem, name_len, "gs_cmap_ToUnicode_alloc");
	if (cmap_name == 0)
	    return_error(gs_error_VMerror);
	memcpy(cmap_name, pref, pref_len);
	memcpy(cmap_name + pref_len, sid, sid_len);
#   endif
    code = gs_cmap_alloc(ppcmap, &st_cmap_ToUnicode,
	      0, cmap_name, name_len, NULL, 0, &gs_cmap_ToUnicode_procs, mem);
    if (code < 0)
	return code;
    map = (uchar *)gs_alloc_bytes(mem, num_codes * gs_cmap_ToUnicode_code_bytes, 
                                  "gs_cmap_ToUnicode_alloc");
    if (map == NULL)
	return_error(gs_error_VMerror);
    memset(map, 0, num_codes * gs_cmap_ToUnicode_code_bytes);
    cmap = (gs_cmap_ToUnicode_t *)*ppcmap;
    cmap->glyph_name_data = map;
    cmap->CMapType = 2;
    cmap->num_fonts = 1;
    cmap->key_size = key_size;
    cmap->num_codes = num_codes;
    cmap->ToUnicode = true;
    cmap->is_identity = true;
    return 0;
}

/*
 * Write a code pair to ToUnicode CMap.
 */
void
gs_cmap_ToUnicode_add_pair(gs_cmap_t *pcmap, int code0, int code1)
{   gs_cmap_ToUnicode_t *cmap = (gs_cmap_ToUnicode_t *)pcmap;
    uchar *map = pcmap->glyph_name_data;
    const int num_codes = ((gs_cmap_ToUnicode_t *)pcmap)->num_codes;
    
    if (code0 >= num_codes)
	return; /* must not happen. */
    map[code0 * gs_cmap_ToUnicode_code_bytes + 0] = (uchar)(code1 >> 8);
    map[code0 * gs_cmap_ToUnicode_code_bytes + 1] = (uchar)(code1 & 0xFF);
    cmap->is_identity &= (code0 == code1);
}
