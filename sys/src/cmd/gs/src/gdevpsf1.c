/* Copyright (C) 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsf1.c,v 1.21 2005/03/17 15:45:52 igor Exp $ */
/* Write an embedded Type 1 font */
#include "memory_.h"
#include <assert.h>
#include "gx.h"
#include "gserrors.h"
#include "gsccode.h"
#include "gsmatrix.h"
#include "gxfixed.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxmatrix.h"		/* for gxtype1.h */
#include "gxtype1.h"
#include "strimpl.h"		/* required by Watcom compiler (why?) */
#include "stream.h"
#include "sfilter.h"
#include "spsdf.h"
#include "sstring.h"
#include "spprint.h"
#include "gdevpsf.h"

/* ------ Utilities shared with CFF writer ------ */

/* Gather glyph information for a Type 1 or Type 2 font. */
int
psf_type1_glyph_data(gs_font_base *pbfont, gs_glyph glyph,
		     gs_glyph_data_t *pgd, gs_font_type1 **ppfont)
{
    gs_font_type1 *const pfont = (gs_font_type1 *)pbfont;

    *ppfont = pfont;
    return pfont->data.procs.glyph_data(pfont, glyph, pgd);
}
int
psf_get_type1_glyphs(psf_outline_glyphs_t *pglyphs, gs_font_type1 *pfont,
		     gs_glyph *subset_glyphs, uint subset_size)
{
    return psf_get_outline_glyphs(pglyphs, (gs_font_base *)pfont,
				  subset_glyphs, subset_size,
				  psf_type1_glyph_data);
}

/* ------ Main program ------ */

/* Write a (named) array of floats. */
private int
write_float_array(gs_param_list *plist, const char *key, const float *values,
		  int count)
{
    if (count != 0) {
	gs_param_float_array fa;

	fa.size = count;
	fa.data = values;
	return param_write_float_array(plist, key, &fa);
    }
    return 0;
}

/* Write a UniqueID and/or XUID. */
private void
write_uid(stream *s, const gs_uid *puid)
{
    if (uid_is_UniqueID(puid))
	pprintld1(s, "/UniqueID %ld def\n", puid->id);
    else if (uid_is_XUID(puid)) {
	uint i, n = uid_XUID_size(puid);

	stream_puts(s, "/XUID [");
	for (i = 0; i < n; ++i)
	    pprintld1(s, "%ld ", uid_XUID_values(puid)[i]);
	stream_puts(s, "] readonly def\n");
    }
}

/* Write the font name. */
private void
write_font_name(stream *s, const gs_font_type1 *pfont,
		const gs_const_string *alt_font_name)
{
    if (alt_font_name)
	stream_write(s, alt_font_name->data, alt_font_name->size);
    else
	stream_write(s, pfont->font_name.chars, pfont->font_name.size);
}
/*
 * Write the Encoding array.  This is a separate procedure only for
 * readability.
 */
private int
write_Encoding(stream *s, gs_font_type1 *pfont, int options,
	      gs_glyph *subset_glyphs, uint subset_size, gs_glyph notdef)
{
    stream_puts(s, "/Encoding ");
    switch (pfont->encoding_index) {
	case ENCODING_INDEX_STANDARD:
	    stream_puts(s, "StandardEncoding");
	    break;
	case ENCODING_INDEX_ISOLATIN1:
	    /* ATM only recognizes StandardEncoding. */
	    if (options & WRITE_TYPE1_POSTSCRIPT) {
		stream_puts(s, "ISOLatin1Encoding");
		break;
	    }
	default:{
		gs_char i;

		stream_puts(s, "256 array\n");
		stream_puts(s, "0 1 255 {1 index exch /.notdef put} for\n");
		for (i = 0; i < 256; ++i) {
		    gs_glyph glyph =
			(*pfont->procs.encode_char)
			((gs_font *)pfont, (gs_char)i, GLYPH_SPACE_NAME);
		    gs_const_string namestr;

		    if (subset_glyphs && subset_size) {
			/*
			 * Only write Encoding entries for glyphs in the
			 * subset.  Use binary search to check each glyph,
			 * since subset_glyphs are sorted.
			 */
			if (!psf_sorted_glyphs_include(subset_glyphs,
							subset_size, glyph))
			    continue;
		    }
		    if (glyph != gs_no_glyph && glyph != notdef &&
			pfont->procs.glyph_name((gs_font *)pfont, glyph,
						&namestr) >= 0
			) {
			pprintd1(s, "dup %d /", (int)i);
			stream_write(s, namestr.data, namestr.size);
			stream_puts(s, " put\n");
		    }
		}
		stream_puts(s, "readonly");
	    }
    }
    stream_puts(s, " def\n");
    return 0;
}

/*
 * Write the Private dictionary.  This is a separate procedure only for
 * readability.  write_CharString is a parameter so that we can encrypt
 * Subrs and CharStrings when the font's lenIV == -1 but we are writing
 * the font with lenIV = 0.
 */
private int
write_Private(stream *s, gs_font_type1 *pfont,
	      gs_glyph *subset_glyphs, uint subset_size,
	      gs_glyph notdef, int lenIV,
	      int (*write_CharString)(stream *, const void *, uint),
	      const param_printer_params_t *ppp)
{
    const gs_type1_data *const pdata = &pfont->data;
    printer_param_list_t rlist;
    gs_param_list *const plist = (gs_param_list *)&rlist;
    int code = s_init_param_printer(&rlist, ppp, s);

    if (code < 0)
	return 0;
    stream_puts(s, "dup /Private 17 dict dup begin\n");
    stream_puts(s, "/-|{string currentfile exch readstring pop}executeonly def\n");
    stream_puts(s, "/|-{noaccess def}executeonly def\n");
    stream_puts(s, "/|{noaccess put}executeonly def\n");
    {
	private const gs_param_item_t private_items[] = {
	    {"BlueFuzz", gs_param_type_int,
	     offset_of(gs_type1_data, BlueFuzz)},
	    {"BlueScale", gs_param_type_float,
	     offset_of(gs_type1_data, BlueScale)},
	    {"BlueShift", gs_param_type_float,
	     offset_of(gs_type1_data, BlueShift)},
	    {"ExpansionFactor", gs_param_type_float,
	     offset_of(gs_type1_data, ExpansionFactor)},
	    {"ForceBold", gs_param_type_bool,
	     offset_of(gs_type1_data, ForceBold)},
	    {"LanguageGroup", gs_param_type_int,
	     offset_of(gs_type1_data, LanguageGroup)},
	    {"RndStemUp", gs_param_type_bool,
	     offset_of(gs_type1_data, RndStemUp)},
	    gs_param_item_end
	};
	gs_type1_data defaults;

	defaults.BlueFuzz = 1;
	defaults.BlueScale = (float)0.039625;
	defaults.BlueShift = 7.0;
	defaults.ExpansionFactor = (float)0.06;
	defaults.ForceBold = false;
	defaults.LanguageGroup = 0;
	defaults.RndStemUp = true;
	code = gs_param_write_items(plist, pdata, &defaults, private_items);
	if (code < 0)
	    return code;
	if (lenIV != 4) {
	    code = param_write_int(plist, "lenIV", &lenIV);
	    if (code < 0)
		return code;
	}
	write_float_array(plist, "BlueValues", pdata->BlueValues.values,
			  pdata->BlueValues.count);
	write_float_array(plist, "OtherBlues", pdata->OtherBlues.values,
			  pdata->OtherBlues.count);
	write_float_array(plist, "FamilyBlues", pdata->FamilyBlues.values,
			  pdata->FamilyBlues.count);
	write_float_array(plist, "FamilyOtherBlues", pdata->FamilyOtherBlues.values,
			  pdata->FamilyOtherBlues.count);
	write_float_array(plist, "StdHW", pdata->StdHW.values,
			  pdata->StdHW.count);
	write_float_array(plist, "StdVW", pdata->StdVW.values,
			  pdata->StdVW.count);
	write_float_array(plist, "StemSnapH", pdata->StemSnapH.values,
			  pdata->StemSnapH.count);
	write_float_array(plist, "StemSnapV", pdata->StemSnapV.values,
			  pdata->StemSnapV.count);
    }
    write_uid(s, &pfont->UID);
    stream_puts(s, "/MinFeature{16 16} def\n");
    stream_puts(s, "/password 5839 def\n");

    /*
     * Write the Subrs.  We always write them all, even for subsets.
     * (We will fix this someday.)
     */

    {
	int n, i;
	gs_glyph_data_t gdata;
	int code;

	gdata.memory = pfont->memory;
	for (n = 0;
	     (code = pdata->procs.subr_data(pfont, n, false, &gdata)) !=
		 gs_error_rangecheck;
	     ) {
	    ++n;
	    if (code >= 0)
		gs_glyph_data_free(&gdata, "write_Private(Subrs)");
	}
	pprintd1(s, "/Subrs %d array\n", n);
	for (i = 0; i < n; ++i)
	    if ((code = pdata->procs.subr_data(pfont, i, false, &gdata)) >= 0) {
		char buf[50];

		if (gdata.bits.size) {
		    sprintf(buf, "dup %d %u -| ", i, gdata.bits.size);
		    stream_puts(s, buf);
		    write_CharString(s, gdata.bits.data, gdata.bits.size);
		    stream_puts(s, " |\n");
		}
		gs_glyph_data_free(&gdata, "write_Private(Subrs)");
	    }
	stream_puts(s, "|-\n");
    }

    /* We don't write OtherSubrs -- there had better not be any! */

    /* Write the CharStrings. */

    {
	int num_chars = 0;
	gs_glyph glyph;
	psf_glyph_enum_t genum;
	gs_glyph_data_t gdata;
	int code;

	gdata.memory = pfont->memory;
	psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont, subset_glyphs,
				    (subset_glyphs ? subset_size : 0),
				    GLYPH_SPACE_NAME);
	for (glyph = gs_no_glyph;
	     (code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1;
	     )
	    if (code == 0 &&
		(code = pdata->procs.glyph_data(pfont, glyph, &gdata)) >= 0
		) {
		++num_chars;
		gs_glyph_data_free(&gdata, "write_Private(CharStrings)");
	    }
	pprintd1(s, "2 index /CharStrings %d dict dup begin\n", num_chars);
	psf_enumerate_glyphs_reset(&genum);
	for (glyph = gs_no_glyph;
	     (code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1;
	    )
	    if (code == 0 &&
		(code = pdata->procs.glyph_data(pfont, glyph, &gdata)) >= 0
		) {
		gs_const_string gstr;
		int code;

		code = pfont->procs.glyph_name((gs_font *)pfont, glyph, &gstr);
		assert(code >= 0);
		stream_puts(s, "/");
		stream_write(s, gstr.data, gstr.size);
		pprintd1(s, " %d -| ", gdata.bits.size);
		write_CharString(s, gdata.bits.data, gdata.bits.size);
		stream_puts(s, " |-\n");
		gs_glyph_data_free(&gdata, "write_Private(CharStrings)");
	    }
    }

    /* Wrap up. */

    stream_puts(s, "end\nend\nreadonly put\nnoaccess put\n");
    s_release_param_printer(&rlist);
    return 0;
}

/* Encrypt and write a CharString. */
private int
stream_write_encrypted(stream *s, const void *ptr, uint count)
{
    const byte *const data = ptr;
    crypt_state state = crypt_charstring_seed;
    byte buf[50];		/* arbitrary */
    uint left, n;
    int code = 0;

    for (left = count; left > 0; left -= n) {
	n = min(left, sizeof(buf));
	gs_type1_encrypt(buf, data + count - left, n, &state);
	code = stream_write(s, buf, n);
    }
    return code;
}

/* Write one FontInfo entry. */
private void
write_font_info(stream *s, const char *key, const gs_const_string *pvalue,
		int do_write)
{
    if (do_write) {
	pprints1(s, "\n/%s ", key);
	s_write_ps_string(s, pvalue->data, pvalue->size, PRINT_HEX_NOT_OK);
	stream_puts(s, " def");
    }
}

/* Write the definition of a Type 1 font. */
int
psf_write_type1_font(stream *s, gs_font_type1 *pfont, int options,
		      gs_glyph *orig_subset_glyphs, uint orig_subset_size,
		      const gs_const_string *alt_font_name, int lengths[3])
{
    stream *es = s;
    long start = stell(s);
    param_printer_params_t ppp;
    printer_param_list_t rlist;
    gs_param_list *const plist = (gs_param_list *)&rlist;
    stream AXE_stream;
    stream_AXE_state AXE_state;
    byte AXE_buf[200];		/* arbitrary */
    stream exE_stream;
    stream_exE_state exE_state;
    byte exE_buf[200];		/* arbitrary */
    psf_outline_glyphs_t glyphs;
    int lenIV = pfont->data.lenIV;
    int (*write_CharString)(stream *, const void *, uint) = stream_write;
    int code = psf_get_type1_glyphs(&glyphs, pfont, orig_subset_glyphs,
				     orig_subset_size);

    if (code < 0)
	return code;

    /* Initialize the parameter printer. */

    ppp = param_printer_params_default;
    ppp.item_suffix = " def\n";
    ppp.print_ok =
	(options & WRITE_TYPE1_ASCIIHEX ? 0 : PRINT_BINARY_OK) |
	PRINT_HEX_NOT_OK;
    code = s_init_param_printer(&rlist, &ppp, s);
    if (code < 0)
	return code;

    /* Write the font header. */

    stream_puts(s, "%!FontType1-1.0: ");
    write_font_name(s, pfont, alt_font_name);
    stream_puts(s, "\n11 dict begin\n");

    /* Write FontInfo. */

    stream_puts(s, "/FontInfo 5 dict dup begin");
    {
	gs_font_info_t info;
	int code = pfont->procs.font_info((gs_font *)pfont, NULL,
			(FONT_INFO_COPYRIGHT | FONT_INFO_NOTICE |
			 FONT_INFO_FAMILY_NAME | FONT_INFO_FULL_NAME),
					  &info);

	if (code >= 0) {
	    write_font_info(s, "Copyright", &info.Copyright,
			    info.members & FONT_INFO_COPYRIGHT);
	    write_font_info(s, "Notice", &info.Notice,
			    info.members & FONT_INFO_NOTICE);
	    write_font_info(s, "FamilyName", &info.FamilyName,
			    info.members & FONT_INFO_FAMILY_NAME);
	    write_font_info(s, "FullName", &info.FullName,
			    info.members & FONT_INFO_FULL_NAME);
	}
    }
    stream_puts(s, "\nend readonly def\n");

    /* Write the main font dictionary. */

    stream_puts(s, "/FontName /");
    write_font_name(s, pfont, alt_font_name);
    stream_puts(s, " def\n");
    code = write_Encoding(s, pfont, options, glyphs.subset_glyphs,
			  glyphs.subset_size, glyphs.notdef);
    if (code < 0)
	return code;
    pprintg6(s, "/FontMatrix [%g %g %g %g %g %g] readonly def\n",
	     pfont->FontMatrix.xx, pfont->FontMatrix.xy,
	     pfont->FontMatrix.yx, pfont->FontMatrix.yy,
	     pfont->FontMatrix.tx, pfont->FontMatrix.ty);
    write_uid(s, &pfont->UID);
    pprintg4(s, "/FontBBox {%g %g %g %g} readonly def\n",
	     pfont->FontBBox.p.x, pfont->FontBBox.p.y,
	     pfont->FontBBox.q.x, pfont->FontBBox.q.y);
    {
	private const gs_param_item_t font_items[] = {
	    {"FontType", gs_param_type_int,
	     offset_of(gs_font_type1, FontType)},
	    {"PaintType", gs_param_type_int,
	     offset_of(gs_font_type1, PaintType)},
	    {"StrokeWidth", gs_param_type_float,
	     offset_of(gs_font_type1, StrokeWidth)},
	    gs_param_item_end
	};

	code = gs_param_write_items(plist, pfont, NULL, font_items);
	if (code < 0)
	    return code;
    }
    {
	const gs_type1_data *const pdata = &pfont->data;

	write_float_array(plist, "WeightVector", pdata->WeightVector.values,
			  pdata->WeightVector.count);
    }
    stream_puts(s, "currentdict end\n");

    /* Write the Private dictionary. */

    if (lenIV < 0 && (options & WRITE_TYPE1_WITH_LENIV)) {
	/* We'll have to encrypt the CharStrings. */
	lenIV = 0;
	write_CharString = stream_write_encrypted;
    }
    if (options & WRITE_TYPE1_EEXEC) {
	stream_puts(s, "currentfile eexec\n");
	lengths[0] = stell(s) - start;
	start = stell(s);
	if (options & WRITE_TYPE1_ASCIIHEX) {
	    s_init(&AXE_stream, s->memory);
	    s_init_state((stream_state *)&AXE_state, &s_AXE_template, NULL);
	    AXE_state.EndOfData = false;
	    s_init_filter(&AXE_stream, (stream_state *)&AXE_state,
			  AXE_buf, sizeof(AXE_buf), es);
	    es = &AXE_stream;
	}
	s_init(&exE_stream, s->memory);
	s_init_state((stream_state *)&exE_state, &s_exE_template, NULL);
	exE_state.cstate = 55665;
	s_init_filter(&exE_stream, (stream_state *)&exE_state,
		      exE_buf, sizeof(exE_buf), es);
	es = &exE_stream;
	/*
	 * Note: eexec encryption always writes/skips 4 initial bytes, not
	 * the number of initial bytes given by pdata->lenIV.
	 */
	stream_puts(es, "****");
    }
    code = write_Private(es, pfont, glyphs.subset_glyphs, glyphs.subset_size,
			 glyphs.notdef, lenIV, write_CharString, &ppp);
    if (code < 0)
	return code;
    stream_puts(es, "dup/FontName get exch definefont pop\n");
    if (options & WRITE_TYPE1_EEXEC) {
	if (options & (WRITE_TYPE1_EEXEC_PAD | WRITE_TYPE1_EEXEC_MARK))
	    stream_puts(es, "mark ");
	stream_puts(es, "currentfile closefile\n");
	s_close_filters(&es, s);
	lengths[1] = stell(s) - start;
	start = stell(s);
	if (options & WRITE_TYPE1_EEXEC_PAD) {
	    int i;

	    for (i = 0; i < 8; ++i)
		stream_puts(s, "\n0000000000000000000000000000000000000000000000000000000000000000");
	    stream_puts(s, "\ncleartomark\n");
	}
	lengths[2] = stell(s) - start;
    } else {
	lengths[0] = stell(s) - start;
	lengths[1] = lengths[2] = 0;
    }

    /* Wrap up. */

    s_release_param_printer(&rlist);
    return 0;
}
