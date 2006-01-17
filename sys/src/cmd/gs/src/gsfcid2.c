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

/* $Id: gsfcid2.c,v 1.6 2004/08/04 19:36:12 stefan Exp $ */
/* Create a CIDFontType 2 font from a Type 42 font. */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gxfont.h"
#include "gxfcid.h"
#include "gxfcmap.h"
#include "gxfont0c.h"

/*
 * Create a Type 2 CIDFont from a Type 42 font.
 */
private int
identity_CIDMap_proc(gs_font_cid2 *pfont, gs_glyph glyph)
{
    ulong cid = glyph - gs_min_cid_glyph;

    if (cid >= pfont->cidata.common.CIDCount)
	return_error(gs_error_rangecheck);
    return (int)cid;
}
int
gs_font_cid2_from_type42(gs_font_cid2 **ppfcid, gs_font_type42 *pfont42,
			 int wmode, gs_memory_t *mem)
{
    gs_font_cid2 *pfcid =
	gs_alloc_struct(mem, gs_font_cid2, &st_gs_font_cid2,
			"gs_font_cid2_from_type42");

    if (pfcid == 0)
	return_error(gs_error_VMerror);

    /* CIDFontType 2 is a subclass (extension) of FontType 42. */
    memcpy(pfcid, pfont42, sizeof(*pfont42));
    pfcid->memory = mem;
    pfcid->next = pfcid->prev = 0; /* probably not necessary */
    pfcid->is_resource = 0;
    gs_font_notify_init((gs_font *)pfcid);
    pfcid->id = gs_next_ids(mem, 1);
    pfcid->base = (gs_font *)pfcid;
    pfcid->FontType = ft_CID_TrueType;
    /* Fill in the rest of the CIDFont data. */
    cid_system_info_set_null(&pfcid->cidata.common.CIDSystemInfo);
    pfcid->cidata.common.CIDCount = pfont42->data.numGlyphs;
    pfcid->cidata.common.GDBytes = 2; /* not used */
    pfcid->cidata.MetricsCount = 0;
    pfcid->cidata.CIDMap_proc = identity_CIDMap_proc;
    /* Since MetricsCount == 0, don't need orig_procs. */

    *ppfcid = pfcid;
    return 0;
}

/* Set up a pointer to a substring of the font data. */
#define ACCESS(base, length, vptr)\
  BEGIN\
    code = pfont->data.string_proc(pfont, (ulong)(base), length, &vptr);\
    if ( code < 0 ) return code;\
  END
#define U16(p) (((uint)((p)[0]) << 8) + (p)[1])
#define U32(p) get_u32_msb(p)
#define PUT16(p, v)\
  BEGIN (p)[0] = (byte)((v) >> 8); (p)[1] = (byte)(v); END

/*
 * Define a subclass of gs_cmap_t that accesses the most common type of
 * TrueType cmap (Platform 3, Encoding 1, Format 4) directly.
 */
typedef struct gs_cmap_tt_16bit_format4_s {
    GS_CMAP_COMMON;
    gs_font_type42 *font;
    uint segCount2;
    ulong endCount, startCount, idDelta, idRangeOffset, glyphIdArray;
} gs_cmap_tt_16bit_format4_t;
gs_private_st_suffix_add1(st_cmap_tt_16bit_format4, gs_cmap_tt_16bit_format4_t,
  "gs_cmap_tt_16bit_format4_t",
  cmap_tt_16bit_format4_enum_ptrs, cmap_tt_16bit_format4_reloc_ptrs,
  st_cmap, font);

private int
tt_16bit_format4_decode_next(const gs_cmap_t * pcmap_in,
		       const gs_const_string * pstr,
		       uint * pindex, uint * pfidx,
		       gs_char * pchr, gs_glyph * pglyph)
{
    const gs_cmap_tt_16bit_format4_t *pcmap =
	(const gs_cmap_tt_16bit_format4_t *)pcmap_in;
    gs_font_type42 *pfont = pcmap->font;
    const byte *ttdata;
    int code;
    uint chr, value = 0;
    uint segment2;

    if (pstr->size < *pindex + 2) {
	*pglyph = gs_no_glyph;
	return (*pindex == pstr->size ? 2 : -1);
    }
    chr = U16(pstr->data + *pindex);
    /* The table is sorted, but we use linear search for simplicity. */
    for (segment2 = 0; segment2 < pcmap->segCount2; segment2 += 2) {
	uint start, delta, roff;

	ACCESS(pcmap->endCount + segment2, 2, ttdata);
	if (chr > U16(ttdata))
	    continue;
	ACCESS(pcmap->startCount + segment2, 2, ttdata);
	start = U16(ttdata);
	if (chr < start)
	    continue;
	ACCESS(pcmap->idDelta + segment2, 2, ttdata);
	delta = U16(ttdata);
	ACCESS(pcmap->idRangeOffset + segment2, 2, ttdata);
	roff = U16(ttdata);
	if (roff) {
	    ulong gidoff = pcmap->idRangeOffset + segment2 + roff +
		(chr - start) * 2;

	    ACCESS(gidoff, 2, ttdata);
	    value = U16(ttdata);
	    if (value != 0)
		value += delta;
	} else
	    value = chr + delta;
	break;
    }
    *pglyph = gs_min_cid_glyph + (value & 0xffff);
    *pchr = chr;
    *pindex += 2;
    *pfidx = 0;
    return 0;
}
private int
tt_16bit_format4_next_range(gs_cmap_ranges_enum_t *penum)
{
    /* There is just a single 2-byte range. */
    if (penum->index == 0) {
	penum->range.first[0] = penum->range.first[1] = 0;
	penum->range.last[0] = penum->range.last[1] = 0xff;
	penum->range.size = 2;
	penum->index = 1;
	return 0;
    }
    return 1;
}
private const gs_cmap_ranges_enum_procs_t tt_16bit_format4_range_procs = {
    tt_16bit_format4_next_range
};
private void
tt_16bit_format4_enum_ranges(const gs_cmap_t *pcmap,
			     gs_cmap_ranges_enum_t *pre)
{
    gs_cmap_ranges_enum_setup(pre, pcmap, &tt_16bit_format4_range_procs);
}
private int
tt_16bit_format4_next_lookup(gs_cmap_lookups_enum_t *penum)
{
    if (penum->index[0] == 0) {
	penum->entry.key_size = 2;
	penum->entry.key_is_range = true;
	penum->entry.value_type = CODE_VALUE_CID;
	penum->entry.value.size = 2;
	penum->entry.font_index = 0;
	penum->index[0] = 1;
	return 0;
    }
    return 1;
}
private int
tt_16bit_format4_next_entry(gs_cmap_lookups_enum_t *penum)
{
    /* index[1] is segment # << 17 + first code. */
    uint segment2 = penum->index[1] >> 16;
    uint next = penum->index[1] & 0xffff;
    const gs_cmap_tt_16bit_format4_t *pcmap =
	(const gs_cmap_tt_16bit_format4_t *)penum->cmap;
    gs_font_type42 *pfont = pcmap->font;
    const byte *ttdata;
    int code;
    uint start, end, delta, roff;
    uint value;

 top:
    if (segment2 >= pcmap->segCount2)
	return 1;
    ACCESS(pcmap->endCount + segment2, 2, ttdata);
    end = U16(ttdata);
    if (next > end) {
	segment2 += 2;
	goto top;
    }
    ACCESS(pcmap->startCount + segment2, 2, ttdata);
    start = U16(ttdata);
    if (next < start)
	next = start;
    PUT16(penum->entry.key[0], next);
    ACCESS(pcmap->idDelta + segment2, 2, ttdata);
    delta = U16(ttdata);
    ACCESS(pcmap->idRangeOffset + segment2, 2, ttdata);
    roff = U16(ttdata);
    if (roff) {
	/* Non-zero offset, table lookup. */
	ulong gidoff = pcmap->idRangeOffset + segment2 + roff;

	ACCESS(gidoff, 2, ttdata);
	value = U16(ttdata);
	if (value != 0)
	    value += delta;
	++next;
    } else {
	/* Zero offset, account for high-order byte changes. */
	value = next + delta;
	next = min(end, (next | 0xff)) + 1;
    }
    PUT16(penum->entry.key[1], next - 1);
    PUT16(penum->temp_value, value);
    penum->entry.value.data = penum->temp_value;
    penum->entry.value.size = 2;
    penum->index[1] = (segment2 << 16) + next;
    return 0;
}
private const gs_cmap_lookups_enum_procs_t tt_16bit_format4_lookup_procs = {
    tt_16bit_format4_next_lookup, tt_16bit_format4_next_entry
};
private void
tt_16bit_format4_enum_lookups(const gs_cmap_t *pcmap, int which,
			gs_cmap_lookups_enum_t *pre)
{
    gs_cmap_lookups_enum_setup(pre, pcmap,
			       (which ? &gs_cmap_no_lookups_procs :
				&tt_16bit_format4_lookup_procs));
}

private const gs_cmap_procs_t tt_16bit_format4_procs = {
    tt_16bit_format4_decode_next,
    tt_16bit_format4_enum_ranges,
    tt_16bit_format4_enum_lookups,
    gs_cmap_compute_identity
};

/*
 * Create a CMap from a TrueType Platform 3, Encoding 1, Format 4 cmap.
 */
int
gs_cmap_from_type42_cmap(gs_cmap_t **ppcmap, gs_font_type42 *pfont,
			 int wmode, gs_memory_t *mem)
{
    ulong origin = pfont->data.cmap;
    gs_cmap_tt_16bit_format4_t *pcmap;
    int code;
    const byte *ttdata;
    ulong offset = origin;
    uint segCount2;

    if (origin == 0)
	return_error(gs_error_invalidfont);

    /*
     * Find the desired cmap sub-table, if any.
     */
    {
	uint cmap_count;
	uint i;
	
	ACCESS(origin + 2, 2, ttdata);
	cmap_count = U16(ttdata);
	for (i = 0; i < cmap_count; ++i) {
	    ACCESS(origin + 4 + i * 8, 8, ttdata);
	    if (U16(ttdata) != 3 || /* platform ID */
		U16(ttdata + 2) != 1 /* encoding ID */
		)
		continue;
	    offset = origin + U32(ttdata + 4);
	    ACCESS(offset, 2, ttdata);
	    if (U16(ttdata) != 4 /* format */)
		continue;
	    break;
	}
	if (i >= cmap_count)	/* not found */
	    return_error(gs_error_invalidfont);
	ACCESS(offset + 6, 2, ttdata);
	segCount2 = U16(ttdata);
    }

    /* Allocate the CMap. */
    {
	static const gs_cid_system_info_t null_cidsi = {
	    { (const byte*) "none", 4 },
	    { (const byte*) "none", 4 },
	    0
	};
	code = gs_cmap_alloc(ppcmap, &st_cmap_tt_16bit_format4, wmode,
			     (const byte *)"none", 4, &null_cidsi, 1,
			     &tt_16bit_format4_procs, mem);
	if (code < 0)
	    return code;
    }
    pcmap = (gs_cmap_tt_16bit_format4_t *)*ppcmap;
    pcmap->from_Unicode = true;
    pcmap->font = pfont;
    pcmap->segCount2 = segCount2;
    pcmap->endCount = offset + 14;
    pcmap->startCount = pcmap->endCount + segCount2 + 2;
    pcmap->idDelta = pcmap->startCount + segCount2;
    pcmap->idRangeOffset = pcmap->idDelta + segCount2;
    pcmap->glyphIdArray = pcmap->idRangeOffset + segCount2;
    return 0;
}
