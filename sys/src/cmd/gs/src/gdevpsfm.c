/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsfm.c,v 1.15 2004/08/19 19:33:09 stefan Exp $ */
/* Write a CMap */
#include "gx.h"
#include "gserrors.h"
#include "gxfcmap.h"
#include "stream.h"
#include "spprint.h"
#include "spsdf.h"
#include "gdevpsf.h"
#include "memory_.h"

/* ---------------- Utilities ---------------- */

typedef struct cmap_operators_s {
    const char *beginchar;
    const char *endchar;
    const char *beginrange;
    const char *endrange;
} cmap_operators_t;
private const cmap_operators_t
  cmap_cid_operators = {
    "begincidchar\n", "endcidchar\n",
    "begincidrange\n", "endcidrange\n"
  },
  cmap_notdef_operators = {
    "beginnotdefchar\n", "endnotdefchar\n",
    "beginnotdefrange\n", "endnotdefrange\n"
  };

/* Write a gs_string with a prefix. */
private void
pput_string_entry(stream *s, const char *prefix, const gs_const_string *pstr)
{
    stream_puts(s, prefix);
    stream_write(s, pstr->data, pstr->size);
}

/* Write a hex string. */
private void
pput_hex(stream *s, const byte *pcid, int size)
{
    int i;
    static const char *const hex_digits = "0123456789abcdef";

    for (i = 0; i < size; ++i) {
	stream_putc(s, hex_digits[pcid[i] >> 4]);
	stream_putc(s, hex_digits[pcid[i] & 0xf]);
    }
}

/* Write a list of code space ranges. */
private void
cmap_put_ranges(stream *s, const gx_code_space_range_t *pcsr, int count)
{
    int i;

    pprintd1(s, "%d begincodespacerange\n", count);
    for (i = 0; i < count; ++i, ++pcsr) {
	stream_puts(s, "<");
	pput_hex(s, pcsr->first, pcsr->size);
	stream_puts(s, "><");
	pput_hex(s, pcsr->last, pcsr->size);
	stream_puts(s, ">\n");
    }
    stream_puts(s, "endcodespacerange\n");
}

/* Write one CIDSystemInfo dictionary. */
private void
cmap_put_system_info(stream *s, const gs_cid_system_info_t *pcidsi)
{
    if (cid_system_info_is_null(pcidsi)) {
	stream_puts(s, " null ");
    } else {
	stream_puts(s, " 3 dict dup begin\n");
	stream_puts(s, "/Registry ");
	s_write_ps_string(s, pcidsi->Registry.data, pcidsi->Registry.size, 0);
	stream_puts(s, " def\n/Ordering ");
	s_write_ps_string(s, pcidsi->Ordering.data, pcidsi->Ordering.size, 0);
	pprintd1(s, " def\n/Supplement %d def\nend ", pcidsi->Supplement);
    }
}

/* Write one code map. */
private int
cmap_put_code_map(const gs_memory_t *mem,
		  stream *s, int which, const gs_cmap_t *pcmap,
		  const cmap_operators_t *pcmo,
		  psf_put_name_chars_proc_t put_name_chars, 
		  int font_index_only)
{
    /* For simplicity, produce one entry for each lookup range. */
    gs_cmap_lookups_enum_t lenum;
    int font_index = (pcmap->num_fonts <= 1 ? 0 : -1);
    int code;

    for (gs_cmap_lookups_enum_init(pcmap, which, &lenum);
	 (code = gs_cmap_enum_next_lookup(&lenum)) == 0; ) {
	gs_cmap_lookups_enum_t counter;
	int num_entries = 0;
	int gi;

	if (font_index_only >= 0 && lenum.entry.font_index != font_index_only)
	    continue;
	if (font_index_only < 0 && lenum.entry.font_index != font_index) {
	    pprintd1(s, "%d usefont\n", lenum.entry.font_index);
	    font_index = lenum.entry.font_index;
	}
	/* Count the number of entries in this lookup range. */
	counter = lenum;
	while (gs_cmap_enum_next_entry(&counter) == 0)
	    ++num_entries;
	for (gi = 0; gi < num_entries; gi += 100) {
	    int i = gi, ni = min(i + 100, num_entries);
	    const char *end;

	    pprintd1(s, "%d ", ni - i);
	    if (lenum.entry.key_is_range) {
		if (lenum.entry.value_type == CODE_VALUE_CID || lenum.entry.value_type == CODE_VALUE_NOTDEF) {
		    stream_puts(s, pcmo->beginrange);
		    end = pcmo->endrange;
		} else {	/* must be def, not notdef */
		    stream_puts(s, "beginbfrange\n");
		    end = "endbfrange\n";
		}
	    } else {
		if (lenum.entry.value_type == CODE_VALUE_CID || lenum.entry.value_type == CODE_VALUE_NOTDEF) {
		    stream_puts(s, pcmo->beginchar);
		    end = pcmo->endchar;
		} else {	/* must be def, not notdef */
		    stream_puts(s, "beginbfchar\n");
		    end = "endbfchar\n";
		}
	    }
	    for (; i < ni; ++i) {
		int j;
		long value;
		int value_size;

		DISCARD(gs_cmap_enum_next_entry(&lenum)); /* can't fail */
		value_size = lenum.entry.value.size;
		for (j = 0; j <= lenum.entry.key_is_range; ++j) {
		    stream_putc(s, '<');
		    pput_hex(s, lenum.entry.key[j], lenum.entry.key_size);
		    stream_putc(s, '>');
		}
		for (j = 0, value = 0; j < value_size; ++j)
		    value = (value << 8) + lenum.entry.value.data[j];
		switch (lenum.entry.value_type) {
		case CODE_VALUE_CID:
		case CODE_VALUE_NOTDEF:
		    pprintld1(s, "%ld", value);
		    break;
		case CODE_VALUE_CHARS:
		    stream_putc(s, '<');
		    pput_hex(s, lenum.entry.value.data, value_size);
		    stream_putc(s, '>');
		    break;
		case CODE_VALUE_GLYPH: {
		    gs_const_string str;
		    int code = pcmap->glyph_name(mem, (gs_glyph)value, &str,
						 pcmap->glyph_name_data);

		    if (code < 0)
			return code;
		    stream_putc(s, '/');
		    code = put_name_chars(s, str.data, str.size);
		    if (code < 0)
			return code;
		}
		    break;
		default:	/* not possible */
		    return_error(gs_error_unregistered);
		}
		stream_putc(s, '\n');
	    }
	    stream_puts(s, end);
	}
    }
    return code;
}

/* ---------------- Main program ---------------- */

/* Write a CMap in its standard (source) format. */
int
psf_write_cmap(const gs_memory_t *mem, 
	       stream *s, const gs_cmap_t *pcmap,
	       psf_put_name_chars_proc_t put_name_chars,
	       const gs_const_string *alt_cmap_name, int font_index_only)
{
    const gs_const_string *const cmap_name =
	(alt_cmap_name ? alt_cmap_name : &pcmap->CMapName);
    const gs_cid_system_info_t *const pcidsi = pcmap->CIDSystemInfo;

    switch (pcmap->CMapType) {
    case 0: case 1: case 2:
	break;
    default:
	return_error(gs_error_rangecheck);
    }

    /* Write the header. */

    if (!pcmap->ToUnicode) {
	stream_puts(s, "%!PS-Adobe-3.0 Resource-CMap\n");
	stream_puts(s, "%%DocumentNeededResources: ProcSet (CIDInit)\n");
	stream_puts(s, "%%IncludeResource: ProcSet (CIDInit)\n");
	pput_string_entry(s, "%%BeginResource: CMap (", cmap_name);
	pput_string_entry(s, ")\n%%Title: (", cmap_name);
	pput_string_entry(s, " ", &pcidsi->Registry);
	pput_string_entry(s, " ", &pcidsi->Ordering);
	pprintd1(s, " %d)\n", pcidsi->Supplement);
	pprintg1(s, "%%%%Version: %g\n", pcmap->CMapVersion);
    }
    stream_puts(s, "/CIDInit /ProcSet findresource begin\n");
    stream_puts(s, "12 dict begin\nbegincmap\n");

    /* Write the fixed entries. */

    pprintd1(s, "/CMapType %d def\n", pcmap->CMapType);
    if (!pcmap->ToUnicode) {
	pprintg1(s, "/CMapVersion %g def\n", pcmap->CMapVersion);
	stream_puts(s, "/CMapName/");
	put_name_chars(s, cmap_name->data, cmap_name->size);
	stream_puts(s, " def\n");
	stream_puts(s, "/CIDSystemInfo");
	if (font_index_only >= 0 && font_index_only < pcmap->num_fonts) {
	    cmap_put_system_info(s, pcidsi + font_index_only);
	} else if (pcmap->num_fonts == 1) {
	    cmap_put_system_info(s, pcidsi);
	} else {
	    int i;

	    pprintd1(s, " %d array\n", pcmap->num_fonts);
	    for (i = 0; i < pcmap->num_fonts; ++i) {
		pprintd1(s, "dup %d", i);
		cmap_put_system_info(s, pcidsi + i);
		stream_puts(s, "put\n");
	    }
	}
	stream_puts(s, " def\n");
	if (uid_is_XUID(&pcmap->uid)) {
	    uint i, n = uid_XUID_size(&pcmap->uid);
	    const long *values = uid_XUID_values(&pcmap->uid);

	    stream_puts(s, "/XUID [");
	    for (i = 0; i < n; ++i)
		pprintld1(s, " %ld", values[i]);
	    stream_puts(s, "] def\n");
	}
	pprintld1(s, "/UIDOffset %ld def\n", pcmap->UIDOffset);
	pprintd1(s, "/WMode %d def\n", pcmap->WMode);
    }

    /* Write the code space ranges. */

    {
	gs_cmap_ranges_enum_t renum;
#define MAX_RANGES 100
	gx_code_space_range_t ranges[MAX_RANGES];
	int code, count = 0;

	for (gs_cmap_ranges_enum_init(pcmap, &renum);
	     (code = gs_cmap_enum_next_range(&renum)) == 0; ) {
	    if (count == MAX_RANGES) {
		cmap_put_ranges(s, ranges, count);
		count = 0;
	    }
	    ranges[count++] = renum.range;
	}
	if (code < 0)
	    return code;
	if (count)
	    cmap_put_ranges(s, ranges, count);
#undef MAX_RANGES
    }

    /* Write the code and notdef data. */

    {
	int code;

	code = cmap_put_code_map(mem, s, 1, pcmap, &cmap_notdef_operators,
			         put_name_chars, font_index_only);
	if (code < 0)
	    return code;
	code = cmap_put_code_map(mem, s, 0, pcmap, &cmap_cid_operators,
			         put_name_chars, font_index_only);
	if (code < 0)
	    return code;
    }

    /* Write the trailer. */

    stream_puts(s, "endcmap\n");
    stream_puts(s, "CMapName currentdict /CMap defineresource pop\nend end\n");
    if (!pcmap->ToUnicode) {
	stream_puts(s, "%%EndResource\n");
	stream_puts(s, "%%EOF\n");
    }

    return 0;
}

