/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfcmap1.c,v 1.7 2004/08/04 19:36:12 stefan Exp $ */
/* Adobe-based CMap character decoding */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxfcmap1.h"

/* Get a big-endian integer. */
inline private ulong
bytes2int(const byte *p, int n)
{
    ulong v = 0;
    int i;

    for (i = 0; i < n; ++i)
        v = (v << 8) + p[i];
    return v;
}

/* ---------------- GC descriptors ---------------- */

public_st_cmap_adobe1();
/* Because lookup ranges can be elements of arrays, */
/* their enum_ptrs procedure must never return 0 prematurely. */
private 
ENUM_PTRS_WITH(cmap_lookup_range_enum_ptrs,
               gx_cmap_lookup_range_t *pclr) return 0;
case 0:
    if (pclr->value_type == CODE_VALUE_GLYPH) {
        const byte *pv = pclr->values.data;
	int size = pclr->value_size;
        int k;

        for (k = 0; k < pclr->num_entries; ++k, pv += size) {
            gs_glyph glyph = bytes2int(pv, size);

            pclr->cmap->mark_glyph(mem, glyph, pclr->cmap->mark_glyph_data);
        }
    }
    return ENUM_OBJ(pclr->cmap);
case 1: return ENUM_STRING(&pclr->keys);
case 2: return ENUM_STRING(&pclr->values);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(cmap_lookup_range_reloc_ptrs, gx_cmap_lookup_range_t *pclr)
    RELOC_VAR(pclr->cmap);
    RELOC_STRING_VAR(pclr->keys);
    RELOC_STRING_VAR(pclr->values);
RELOC_PTRS_END
public_st_cmap_lookup_range();
public_st_cmap_lookup_range_element();

/* ---------------- Procedures ---------------- */

    /* ------ Decoding ------ */

/*
 * multi-dimensional range comparator
 */

private void
print_msg_str_in_range(const byte *str,
                       const byte *key_lo, const byte *key_hi,
                       int key_size)
{
    debug_print_string_hex(str, key_size);
    dlprintf(" in ");
    debug_print_string_hex(key_lo, key_size);
    dlprintf(" - ");
    debug_print_string_hex(key_hi, key_size);
    dlprintf("\n");
}

private int
gs_cmap_get_shortest_chr(const gx_code_map_t * pcmap, uint *pfidx)
{
    int i;
    int len_shortest = MAX_CMAP_CODE_SIZE;
    uint fidx_shortest = 0; /* font index for this fallback */

    for (i = pcmap->num_lookup - 1; i >= 0; --i) {
        const gx_cmap_lookup_range_t *pclr = &pcmap->lookup[i];
        if ((pclr->key_prefix_size + pclr->key_size) <= len_shortest) {
           len_shortest = (pclr->key_prefix_size + pclr->key_size);
           fidx_shortest = pclr->font_index;
        }
    }

    *pfidx = fidx_shortest;
    return len_shortest;
}

/*
 * multi-dimensional relative position calculator
 *
 * Returns offset of the given CID, considering CID range
 * as array of CIDs (the last index changes fastest).
 */
private int
gs_multidim_CID_offset(const byte *key_str,
                        const byte *key_lo, const byte *key_hi,
			int key_size)
{

    int i;	/* index for current dimension */
    int CID_offset = 0;

    if (gs_debug_c('J')) {
        dlprintf("[J]gmCo()         calc CID_offset for 0x");
        print_msg_str_in_range(key_str, key_lo, key_hi, key_size);
    }

    for (i = 0; i < key_size; i++)
        CID_offset = CID_offset * (key_hi[i] - key_lo[i] + 1) +
            key_str[i] - key_lo[i];
 
    if_debug1('J', "[J]gmCo()         CID_offset = %d\n", CID_offset);
    return CID_offset;
}

/*
 * Decode a character from a string using a code map, updating the index.
 * Return 0 for a CID or name, N > 0 for a character code where N is the
 * number of bytes in the code, or an error.  Store the decoded bytes in
 * *pchr.  For undefined characters, set *pglyph = gs_no_glyph and return 0.
 */
private int
code_map_decode_next_multidim_regime(const gx_code_map_t * pcmap,
                     const gs_const_string * pstr,
                     uint * pindex, uint * pfidx,
                     gs_char * pchr, gs_glyph * pglyph)
{
    const byte *str = pstr->data + *pindex;
    uint ssize = pstr->size - *pindex;
    /*
     * The keys are not sorted due to 'usecmap'.  Possible optimization :
     * merge and sort keys in 'zbuildcmap', then use binary search here.
     * This would be valuable for UniJIS-UTF8-H, which contains about 7000
     * keys.
     */
    int i;

    /*
     * In the fallback of CMap decoding procedure, there is "partial matching".
     * For detail, refer PostScript Ref. Manual v3 at the end of Fonts chapter.
     */

    /* "pm" stands for partial match (not pointer), temporal use. */
    int pm_maxlen = 0;		/* partial match: max length */
    int pm_index = *pindex;	/* partial match: ptr index (in str) */
    uint pm_fidx = *pfidx;	/* partial match: ptr font index */
    gs_char pm_chr = *pchr;	/* partial match: ptr character */

    *pchr = '\0';

    if (gs_debug_c('J')) {
        dlprintf("[J]CMDNmr() is called: str=(");
        debug_print_string_hex(str, ssize);
        dlprintf3(") @ 0x%lx ssize=%d, %d ranges to check\n",
                       str, ssize, pcmap->num_lookup);
    }
 
    for (i = pcmap->num_lookup - 1; i >= 0; --i) {
	/* main loop - scan the map passed via pcmap */
	/* reverse scan order due to 'usecmap' */

        const gx_cmap_lookup_range_t *pclr = &pcmap->lookup[i];
        int pre_size = pclr->key_prefix_size, key_size = pclr->key_size,
            chr_size = pre_size + key_size;

        int j = 0;
	/* length of the given byte stream is shorter than
         * chr-length of current range, no need for further check,
         * skip to the next range.
         */
        if (ssize < chr_size)
            continue;

        if (0 < pre_size) {
            const byte * prefix = pclr->key_prefix;
            /* check partial match in prefix */
            for (j = 0; j < pre_size; j++)
               if (prefix[j] != str[j])
                   break;

            if (0 == j)			/* no match, skip to next i */
                continue;
            else if (j < pre_size) {	/* not exact, partial match */
                if (gs_debug_c('J')) {
                    dlprintf("[J]CMDNmr() partial match with prefix:");
                    print_msg_str_in_range(str, prefix,
                                                prefix, pre_size);
                }

                if (pm_maxlen < j) {
                    pm_maxlen = chr_size;
                    pm_chr = bytes2int(str, chr_size);
                    pm_index = (*pindex) + chr_size;
                    pm_fidx = pclr->font_index;
                }
                continue ; /* no need to check key, skip to next i */
            }

            if (gs_debug_c('J')) {
                dlprintf("[J]CMDNmr()   full match with prefix:");
                print_msg_str_in_range(str, prefix, prefix, pre_size);
            }

        } /* if (0 < pre_size) */

        /* full match in prefix. check key */
        {
            const byte *key = pclr->keys.data;
            int step = key_size;
            int k, l;
            const byte *pvalue = NULL;

	    /* when range is "range", 2 keys for lo-end and hi-end
             * are stacked. So twice the step. current "key" points
             * lo-end of current range, and the pointer for hi-end
             * is calculated by (key + step - key_size).
             */

            if (pclr->key_is_range)
		step <<=1; 	/* step = step * 2; */

            for (k = 0; k < pclr->num_entries; ++k, key += step) {

                if_debug0('j', "[j]CMDNmr()     check key:");
                if (gs_debug_c('j'))
                    print_msg_str_in_range(str + pre_size,
                        key, key + step - key_size, key_size) ;
 
                for (l = 0; l < key_size; l++) {
                    byte c = str[l + pre_size];
                    if (c < key[l] || c > key[step - key_size + l])
                        break;
                }

		if (pm_maxlen < pre_size + l) {
                    pm_maxlen = chr_size;
                    pm_chr = bytes2int(str, chr_size);
                    pm_index = (*pindex) + chr_size;
                    pm_fidx = pclr->font_index;
                }
                if (l == key_size)
                        break;
	    }

            /* all keys are tried, but found no match. */
            /* go to next prefix. */
            if (k == pclr->num_entries)
                continue;

            /* We have a match.  Return the result. */
            *pchr = bytes2int(str, chr_size);
            *pindex += chr_size;
            *pfidx = pclr->font_index;
            pvalue = pclr->values.data + k * pclr->value_size;

            if (gs_debug_c('J')) {
                dlprintf("[J]CMDNmr()     full matched pvalue=(");
                debug_print_string_hex(pvalue, pclr->value_size);
                dlprintf(")\n");
            }

            switch (pclr->value_type) {
            case CODE_VALUE_CID:
                *pglyph = gs_min_cid_glyph +
                    bytes2int(pvalue, pclr->value_size) +
                    gs_multidim_CID_offset(str + pre_size,
                        key, key + step - key_size, key_size);
                return 0;
            case CODE_VALUE_NOTDEF:
                *pglyph = gs_min_cid_glyph +
                    bytes2int(pvalue, pclr->value_size);
                return 0;
            case CODE_VALUE_GLYPH:
                *pglyph = bytes2int(pvalue, pclr->value_size);
                return 0;
            case CODE_VALUE_CHARS:
                *pglyph =
                    bytes2int(pvalue, pclr->value_size) +
                    bytes2int(str + pre_size, key_size) -
                    bytes2int(key, key_size);
                return pclr->value_size;
            default:            /* shouldn't happen */
                return_error(gs_error_rangecheck);
            }
        }
    }
    /* No mapping. */
    *pchr = pm_chr;
    *pindex = pm_index;
    *pfidx = pm_fidx;
    *pglyph = gs_no_glyph;
    if (gs_debug_c('J')) {
        dlprintf("[J]CMDNmr()     no full match, use partial match for (");
        debug_print_string_hex(str, pm_maxlen);
        dlprintf(")\n");
    }
    return 0;
}

/*
 * Decode a character from a string using a CMap.
 * Return like code_map_decode_next.
 * At present, the range specification by (begin|end)codespacerange
 * is not used in this function. Therefore, this function accepts
 * some invalid CMap which def & undef maps exceed the codespacerange.
 * It should be checked in this function, or some procedure in gs_cmap.ps.
 */
private int
gs_cmap_adobe1_decode_next(const gs_cmap_t * pcmap_in,
			   const gs_const_string * pstr,
			   uint * pindex, uint * pfidx,
			   gs_char * pchr, gs_glyph * pglyph)
{
    const gs_cmap_adobe1_t *pcmap = (const gs_cmap_adobe1_t *)pcmap_in;
    uint save_index = *pindex;
    int code;

    uint pm_index;
    uint pm_fidx;
    gs_char pm_chr;

    /* For first, check defined map */
    if_debug0('J', "[J]GCDN() check def CMap\n");
    code =
        code_map_decode_next_multidim_regime(&pcmap->def, pstr, pindex, pfidx, pchr, pglyph);

    /* This is defined character */
    if (code != 0 || *pglyph != gs_no_glyph)
        return code;

    /* In here, this is NOT defined character */
    /* save partially matched results */
    pm_index = *pindex;
    pm_fidx = *pfidx;
    pm_chr = *pchr;

    /* check notdef map. */
    if_debug0('J', "[J]GCDN() check notdef CMap\n");
    *pindex = save_index;
    code =
	code_map_decode_next_multidim_regime(&pcmap->notdef, pstr, pindex, pfidx, pchr, pglyph);

    /* This is defined "notdef" character. */
    if (code != 0 || *pglyph != gs_no_glyph)
        return code;

    /*
     * This is undefined in def & undef maps,
     * use partially matched result with default notdef (CID = 0).
     */ 
    if (save_index < pm_index) {

	/* there was some partially matched */

        *pglyph = gs_min_cid_glyph;	/* CID = 0 */
        *pindex = pm_index;
        *pfidx = pm_fidx;
        *pchr = '\0';
         return 0; /* should return some error for partial matched .notdef? */
    }
    else {
	/* no match */

	/* Even partial match is failed.
         * Getting the shortest length from defined characters,
         * and take the leading bytes (with same length of the shortest
         * defined chr) as an unidentified character: CID = 0.
	 * Also this procedure is specified in PS Ref. Manual v3,
         * at the end of Fonts chapter. 
         */

	const byte *str = pstr->data + save_index;
	uint ssize = pstr->size - save_index;
	int chr_size_shortest = 
		gs_cmap_get_shortest_chr(&pcmap->def, pfidx);

	if (chr_size_shortest <= ssize) {
            *pglyph = gs_min_cid_glyph;	/* CID = 0, this is CMap fallback */
            *pindex = save_index + chr_size_shortest;
	    *pchr = '\0';
            if (gs_debug_c('J')) {
                dlprintf1("[J]GCDN() no partial match, skip %d byte (",
                                               chr_size_shortest);
                debug_print_string_hex(str, chr_size_shortest);
                dlprintf(")\n");
            }
            return 0; /* should return some error for fallback .notdef? */
	}
	else {
            /* Undecodable string is shorter than the shortest character,
             * there's no way except to return error.
             */
            if (gs_debug_c('J')) {
                dlprintf2("[J]GCDN() left data in buffer (%d) is shorter than shortest defined character (%d)\n",
                  ssize, chr_size_shortest);
            }
            *pglyph = gs_no_glyph;
            return_error(gs_error_rangecheck);
	}
    }
}

    /* ------ Initialization/creation ------ */

/*
 * Allocate and initialize an Adobe1 CMap.  The caller must still fill in
 * the code space ranges, lookup tables, keys, and values.
 */

private int
adobe1_next_range(gs_cmap_ranges_enum_t *penum)
{
    const gs_cmap_adobe1_t *const pcmap =
	(const gs_cmap_adobe1_t *)penum->cmap;

    if (penum->index >= pcmap->code_space.num_ranges)
	return 1;
    penum->range = pcmap->code_space.ranges[penum->index++];
    return 0;
}
private const gs_cmap_ranges_enum_procs_t adobe1_range_procs = {
    adobe1_next_range
};
private void
gs_cmap_adobe1_enum_ranges(const gs_cmap_t *pcmap, gs_cmap_ranges_enum_t *pre)
{
    gs_cmap_ranges_enum_setup(pre, pcmap, &adobe1_range_procs);
}
private int
adobe1_next_lookup(gs_cmap_lookups_enum_t *penum, const gx_code_map_t *pcm)
{
    const gx_cmap_lookup_range_t *lookup = &pcm->lookup[penum->index[0]];

    if (penum->index[0] >= pcm->num_lookup)
	return 1;
    penum->entry.key_size = lookup->key_prefix_size + lookup->key_size;
    penum->entry.key_is_range = lookup->key_is_range;
    penum->entry.value_type = lookup->value_type;
    penum->entry.value.size = lookup->value_size;
    penum->entry.font_index = lookup->font_index;
    penum->index[0]++;
    penum->index[1] = 0;
    return 0;
}
private int
adobe1_next_lookup_def(gs_cmap_lookups_enum_t *penum)
{
    return adobe1_next_lookup(penum,
			&((const gs_cmap_adobe1_t *)penum->cmap)->def);
}
private int
adobe1_next_lookup_notdef(gs_cmap_lookups_enum_t *penum)
{
    return adobe1_next_lookup(penum,
			&((const gs_cmap_adobe1_t *)penum->cmap)->notdef);
}
private int
adobe1_next_entry(gs_cmap_lookups_enum_t *penum, const gx_code_map_t *pcm)
{
    const gx_cmap_lookup_range_t *lookup = &pcm->lookup[penum->index[0] - 1];
    int psize = lookup->key_prefix_size;
    int ksize = lookup->key_size;
    const byte *key =
	lookup->keys.data + penum->index[1] * ksize *
	(lookup->key_is_range ? 2 : 1);
    int i;

    if (penum->index[1] >= lookup->num_entries)
	return 1;
    if (psize + ksize > MAX_CMAP_CODE_SIZE)
	return_error(gs_error_rangecheck);
    for (i = 0; i < 2; ++i, key += ksize) {
	memcpy(penum->entry.key[i], lookup->key_prefix, psize);
	memcpy(penum->entry.key[i] + psize, key, ksize);
    }
    penum->entry.value.data =
	lookup->values.data + penum->index[1] * lookup->value_size;
    penum->entry.value.size = lookup->value_size;
    penum->index[1]++;
    return 0;
}
private int
adobe1_next_entry_def(gs_cmap_lookups_enum_t *penum)
{
    return adobe1_next_entry(penum,
			&((const gs_cmap_adobe1_t *)penum->cmap)->def);
}
private int
adobe1_next_entry_notdef(gs_cmap_lookups_enum_t *penum)
{
    return adobe1_next_entry(penum,
			&((const gs_cmap_adobe1_t *)penum->cmap)->notdef);
}
private const gs_cmap_lookups_enum_procs_t adobe1_lookup_def_procs = {
    adobe1_next_lookup_def, adobe1_next_entry_def
};
private const gs_cmap_lookups_enum_procs_t adobe1_lookup_notdef_procs = {
    adobe1_next_lookup_notdef, adobe1_next_entry_notdef
};
private void
gs_cmap_adobe1_enum_lookups(const gs_cmap_t *pcmap, int which,
			    gs_cmap_lookups_enum_t *pre)
{
    gs_cmap_lookups_enum_setup(pre, pcmap,
			       (which ? &adobe1_lookup_notdef_procs :
				&adobe1_lookup_def_procs));
}

private const gs_cmap_procs_t cmap_adobe1_procs = {
    gs_cmap_adobe1_decode_next,
    gs_cmap_adobe1_enum_ranges,
    gs_cmap_adobe1_enum_lookups,
    gs_cmap_compute_identity
};

int
gs_cmap_adobe1_alloc(gs_cmap_adobe1_t **ppcmap, int wmode,
		     const byte *map_name, uint name_size,
		     uint num_fonts, uint num_ranges, uint num_lookups,
		     uint keys_size, uint values_size,
		     const gs_cid_system_info_t *pcidsi_in, gs_memory_t *mem)
{
    gs_cmap_t *pcmap;
    gs_cmap_adobe1_t *pcmap1;
    gx_code_space_range_t *ranges = (gx_code_space_range_t *)
	gs_alloc_byte_array(mem, num_ranges, sizeof(gx_code_space_range_t),
			    "gs_cmap_alloc(code space ranges)");
    gx_cmap_lookup_range_t *lookups =
	(num_lookups == 0 ? NULL :
	 gs_alloc_struct_array(mem, num_lookups, gx_cmap_lookup_range_t,
			       &st_cmap_lookup_range,
			       "gs_cmap_alloc(lookup ranges)"));
    byte *keys =
	(keys_size == 0 ? NULL :
	 gs_alloc_string(mem, keys_size, "gs_cmap_alloc(keys)"));
    byte *values =
	(values_size == 0 ? NULL :
	 gs_alloc_string(mem, values_size, "gs_cmap_alloc(values)"));
    int code =
	gs_cmap_alloc(&pcmap, &st_cmap_adobe1, wmode, map_name, name_size,
		      pcidsi_in, num_fonts, &cmap_adobe1_procs, mem);
    uint i;

    if (code < 0 || ranges == 0 || (num_lookups != 0 && lookups == 0) ||
	(keys_size != 0 && keys == 0) || (values_size != 0 && values == 0)) {
	gs_free_string(mem, values, values_size, "gs_cmap_alloc(values)");
	gs_free_string(mem, keys, keys_size, "gs_cmap_alloc(keys)");
	gs_free_object(mem, lookups, "gs_cmap_alloc(lookup ranges)");
	gs_free_object(mem, ranges, "gs_cmap_alloc(code space ranges)");
	return_error(gs_error_VMerror);
    }
    *ppcmap = pcmap1 = (gs_cmap_adobe1_t *)pcmap;
    pcmap1->code_space.ranges = ranges;
    pcmap1->code_space.num_ranges = num_ranges;
    if (num_lookups > 0) {
	for (i = 0; i < num_lookups; ++i) {
	    memset(&lookups[i], 0, sizeof(*lookups));
	    lookups[i].cmap = pcmap1;
	}
	lookups[0].keys.data = keys;
	lookups[0].keys.size = keys_size;
	lookups[0].values.data = values;
	lookups[0].values.size = values_size;
    }
    pcmap1->def.lookup = lookups;
    pcmap1->def.num_lookup = num_lookups;
    pcmap1->notdef.lookup = 0;
    pcmap1->notdef.num_lookup = 0;
    /* no mark_glyph, mark_glyph_data, glyph_name, glyph_name_data */
    return 0;
}
