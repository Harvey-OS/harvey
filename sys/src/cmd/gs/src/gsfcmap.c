/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsfcmap.c,v 1.8.2.2 2000/10/26 12:45:11 igorm Exp $ */
/* CMap character decoding */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfcmap.h"

/* GC descriptors */
public_st_cmap();
/* Because lookup ranges can be elements of arrays, */
/* their enum_ptrs procedure must never return 0 prematurely. */
private 
ENUM_PTRS_WITH(code_lookup_range_enum_ptrs,
               gx_code_lookup_range_t *pclr) return 0;
case 0:
    if (pclr->value_type == CODE_VALUE_GLYPH) {
        const byte *pv = pclr->values.data;
        int k;

        for (k = 0; k < pclr->num_keys; ++k) {
            gs_glyph glyph = 0;
            int i;

            for (i = 0; i < pclr->value_size; ++i)
                glyph = (glyph << 8) + *pv++;
            pclr->cmap->mark_glyph(glyph, pclr->cmap->mark_glyph_data);
        }
    }
    return ENUM_OBJ(pclr->cmap);
case 1: return ENUM_STRING(&pclr->keys);
case 2: return ENUM_STRING(&pclr->values);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(code_lookup_range_reloc_ptrs, gx_code_lookup_range_t *pclr)
    RELOC_VAR(pclr->cmap);
    RELOC_STRING_VAR(pclr->keys);
    RELOC_STRING_VAR(pclr->values);
RELOC_PTRS_END
public_st_code_lookup_range();
public_st_code_lookup_range_element();

/* ---------------- Procedures ---------------- */

/*
 * Initialize a just-allocated CMap, to ensure that all pointers are clean
 * for the GC.
 */
void
gs_cmap_init(gs_cmap_t *pcmap)
{
    memset(pcmap, 0, sizeof(*pcmap));
    uid_set_invalid(&pcmap->uid);
}

/* Get a big-endian integer. */
private uint
bytes2int(const byte *p, int n)
{
    uint v = 0;
    int i;

    for (i = 0; i < n; ++i)
        v = (v << 8) + p[i];
    return v;
}

/*
 * Decode a character from a string using a code map, updating the index.
 * Return 0 for a CID or name, N > 0 for a character code where N is the
 * number of bytes in the code, or an error.  Shift the decoded bytes into
 * *pchr.  For undefined characters, set *pglyph = gs_no_glyph and return 0.
 */
private int
code_map_decode_next(const gx_code_map_t * pcmap, const gs_const_string * pstr,
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

    for (i = pcmap->num_lookup - 1; i >= 0; --i) { /* reverse scan order due to 'usecmap' */
        const gx_code_lookup_range_t *pclr = &pcmap->lookup[i];
        int pre_size = pclr->key_prefix_size, key_size = pclr->key_size,
            chr_size = pre_size + key_size;

        if (ssize < chr_size)
            continue;
        if (memcmp(str, pclr->key_prefix, pre_size))
            continue;
        /* Search the lookup range. We could use binary search. */
        {
            const byte *key = pclr->keys.data;
            int step = key_size;
            int k;
            const byte *pvalue;

            if (pclr->key_is_range) {
                step <<= 1;
                for (k = 0; k < pclr->num_keys; ++k, key += step)
                    if (memcmp(str + pre_size, key, key_size) >= 0 &&
                        memcmp(str + pre_size, key + key_size, key_size) <= 0)
                        break;
            } else {
                for (k = 0; k < pclr->num_keys; ++k, key += step)
                    if (!memcmp(str + pre_size, key, key_size))
                        break;
            }
            if (k == pclr->num_keys)
                continue;
            /* We have a match.  Return the result. */
            *pchr = (*pchr << (chr_size * 8)) + bytes2int(str, chr_size);
            *pindex += chr_size;
            *pfidx = pclr->font_index;
            pvalue = pclr->values.data + k * pclr->value_size;
            switch (pclr->value_type) {
            case CODE_VALUE_CID:
                *pglyph = gs_min_cid_glyph +
                    bytes2int(pvalue, pclr->value_size) +
                    bytes2int(str + pre_size, key_size) -
                    bytes2int(key, key_size);
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
    *pglyph = gs_no_glyph;
    return 0;
}

/*
 * Decode a character from a string using a CMap.
 * Return like code_map_decode_next.
 */
int
gs_cmap_decode_next(const gs_cmap_t * pcmap, const gs_const_string * pstr,
                    uint * pindex, uint * pfidx,
                    gs_char * pchr, gs_glyph * pglyph)
{
    uint save_index = *pindex;
    int code;

    *pchr = 0;
    code =
        code_map_decode_next(&pcmap->def, pstr, pindex, pfidx, pchr, pglyph);
    if (code != 0 || *pglyph != gs_no_glyph)
        return code;
    /* This is an undefined character.  Use the notdef map. */
    *pindex = save_index;
    *pchr = 0;
    return code_map_decode_next(&pcmap->notdef, pstr, pindex, pfidx,
                                pchr, pglyph);
}
