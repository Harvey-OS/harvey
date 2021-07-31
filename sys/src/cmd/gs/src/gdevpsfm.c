/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpsfm.c,v 1.5 2001/04/01 00:35:26 raph Exp $ */
/* Write a CMap */
#include "gx.h"
#include "gserrors.h"
#include "gxfcmap.h"
#include "stream.h"
#include "spprint.h"
#include "spsdf.h"
#include "gdevpsf.h"

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
cmap_put_code_map(stream *s, const gx_code_map_t *pccmap,
		  const gs_cmap_t *pcmap, const cmap_operators_t *pcmo,
		  psf_put_name_chars_proc_t put_name_chars, int *pfont_index)
{
    /* For simplicity, produce one entry for each lookup range. */
    const gx_code_lookup_range_t *pclr = pccmap->lookup;
    int nl = pccmap->num_lookup;

    for (; nl > 0; ++pclr, --nl) {
	const byte *pkey = pclr->keys.data;
	const byte *pvalue = pclr->values.data;
	int gi;

	if (pclr->font_index != *pfont_index) {
	    pprintd1(s, "%d usefont\n", pclr->font_index);
	    *pfont_index = pclr->font_index;
	}
	for (gi = 0; gi < pclr->num_keys; gi += 100) {
	    int i = gi, ni = min(i + 100, pclr->num_keys);
	    const char *end;

	    pprintd1(s, "%d ", ni - i);
	    if (pclr->key_is_range) {
		if (pclr->value_type == CODE_VALUE_CID) {
		    stream_puts(s, pcmo->beginrange);
		    end = pcmo->endrange;
		} else {	/* must be def, not notdef */
		    stream_puts(s, "beginbfrange\n");
		    end = "endbfrange\n";
		}
	    } else {
		if (pclr->value_type == CODE_VALUE_CID) {
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

		for (j = 0; j <= pclr->key_is_range; ++j) {
		    stream_putc(s, '<');
		    pput_hex(s, pclr->key_prefix, pclr->key_prefix_size);
		    pput_hex(s, pkey, pclr->key_size);
		    stream_putc(s, '>');
		    pkey += pclr->key_size;
		}
		for (j = 0, value = 0; j < pclr->value_size; ++j)
		    value = (value << 8) + *pvalue++;
		switch (pclr->value_type) {
		case CODE_VALUE_CID:
		    pprintld1(s, "%ld", value);
		    break;
		case CODE_VALUE_CHARS:
		    stream_putc(s, '<');
		    pput_hex(s, pvalue - pclr->value_size, pclr->value_size);
		    stream_putc(s, '>');
		    break;
		case CODE_VALUE_GLYPH: {
		    gs_const_string str;
		    int code = pcmap->glyph_name((gs_glyph)value, &str,
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
		    return_error(gs_error_rangecheck);
		}
		stream_putc(s, '\n');
	    }
	    stream_puts(s, end);
	}
    }
    return 0;
}

/* ---------------- Main program ---------------- */

/* Write a CMap in its standard (source) format. */
int
psf_write_cmap(stream *s, const gs_cmap_t *pcmap,
	       psf_put_name_chars_proc_t put_name_chars,
	       const gs_const_string *alt_cmap_name)
{
    const gs_const_string *const cmap_name =
	(alt_cmap_name ? alt_cmap_name : &pcmap->CMapName);
    const gs_cid_system_info_t *const pcidsi = pcmap->CIDSystemInfo;

    switch (pcmap->CMapType) {
    case 0: case 1:
	break;
    default:
	return_error(gs_error_rangecheck);
    }

    /* Write the header. */

    stream_puts(s, "%!PS-Adobe-3.0 Resource-CMap\n");
    stream_puts(s, "%%DocumentNeededResources: ProcSet (CIDInit)\n");
    stream_puts(s, "%%IncludeResource: ProcSet (CIDInit)\n");
    pput_string_entry(s, "%%BeginResource: CMap (", cmap_name);
    pput_string_entry(s, ")\n%%Title: (", cmap_name);
    pput_string_entry(s, " ", &pcidsi->Registry);
    pput_string_entry(s, " ", &pcidsi->Ordering);
    pprintd1(s, " %d)\n", pcidsi->Supplement);
    pprintg1(s, "%%%%Version: %g\n", pcmap->CMapVersion);
    stream_puts(s, "/CIDInit /ProcSet findresource begin\n");
    stream_puts(s, "12 dict begin\nbegincmap\n");

    /* Write the fixed entries. */

    pprintd1(s, "/CMapType %d def\n", pcmap->CMapType);
    stream_puts(s, "/CMapName/");
    put_name_chars(s, cmap_name->data, cmap_name->size);
    stream_puts(s, " def\n/CIDSystemInfo");
    if (pcmap->num_fonts == 1) {
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
    pprintg1(s, "def\n/CMapVersion %g def\n", pcmap->CMapVersion);
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

    /* Write the code space ranges. */

    {
	const gx_code_space_range_t *pcsr = pcmap->code_space.ranges;
	int gi;

	for (gi = 0; gi < pcmap->code_space.num_ranges; gi += 100) {
	    int i = gi, ni = min(i + 100, pcmap->code_space.num_ranges);

	    pprintd1(s, "%d begincodespacerange\n", ni - i);
	    for (; i < ni; ++i, ++pcsr) {
		stream_puts(s, "<");
		pput_hex(s, pcsr->first, pcsr->size);
		stream_puts(s, "><");
		pput_hex(s, pcsr->last, pcsr->size);
		stream_puts(s, ">\n");
	    }
	    stream_puts(s, "endcodespacerange\n");
	}
    }

    /* Write the code and notdef data. */

    {
	int font_index = (pcmap->num_fonts <= 1 ? 0 : -1);

	cmap_put_code_map(s, &pcmap->notdef, pcmap, &cmap_notdef_operators,
			  put_name_chars, &font_index);
	cmap_put_code_map(s, &pcmap->def, pcmap, &cmap_cid_operators,
			  put_name_chars, &font_index);
    }

    /* Write the trailer. */

    stream_puts(s, "endcmap\n");
    stream_puts(s, "CMapName currentdict /CMap defineresource pop\nend end\n");
    stream_puts(s, "%%EndResource\n");
    stream_puts(s, "%%EOF\n");

    return 0;
}
