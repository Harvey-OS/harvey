/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gdevpdft.c,v 1.3 2000/03/16 01:21:24 lpd Exp $ */
/* Text handling for PDF-writing driver. */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfixed.h"		/* for gxfcache.h */
#include "gxfont.h"
#include "gxfont0.h"
#include "gxfont1.h"
#include "gxfont42.h"
#include "gxfcache.h"		/* for orig_fonts list */
#include "gxpath.h"		/* for getting current point */
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "scommon.h"

/*
 * The PDF documentation does a pretty shoddy job of specifying use of fonts
 * beyond the base 14, and on top of that, Acrobat Reader has some serious
 * bugs.  Specifically, Acrobat Reader 3's Print function has a bug that
 * make re-encoded characters print as blank if the font is substituted (not
 * embedded or one of the base 14).
 *
 * We do have to handle re-encoded Type 1 fonts, because TeX output makes
 * constant use of them.  We have several alternatives for how to handle
 * characters whose encoding doesn't match their encoding in the base font's
 * built-in encoding.  If a character's glyph doesn't match the character's
 * glyph in the encoding built up so far, we check if the font has that
 * glyph at all; if not, we fall back to a bitmap.  Otherwise, we use one or
 * both of the following algorithms:
 *
 *      1. If this is the first time a character at this position has been
 *      seen, assign its glyph to that position in the encoding.
 *      We do this step if the device parameter ReAssignCharacters is true.
 *      (This is the default.)
 *
 *      2. If the glyph is present in the encoding at some other position,
 *      substitute that position for the character; otherwise, assign the
 *      glyph to an unoccupied (.notdef) position.
 *      We do this step if the device parameter ReEncodeCharacters is true.
 *      (This is the default.)
 *
 *      3. Finally, fall back to using a bitmap.
 *
 * If it is essential that all strings in the output contain exactly the
 * same character codes as the input, set ReEncodeCharacters to false.  If
 * it is important that strings be searchable, but some non-searchable
 * strings can be tolerated, the defaults are appropriate.  If searchability
 * is not important, set ReAssignCharacters to false.
 * 
 * Since the chars_used bit vector keeps track of characters by their
 * position in the Encoding, not by their glyph identifier, it can't record
 * use of unencoded characters.  Therefore, if we ever use an unencoded
 * character in an embedded font, we force writing the entire font rather
 * than a subset.  This is inelegant but (for the moment) too awkward to
 * fix.
 *
 * Because of the AR3 printing bug, if CompatibilityLevel <= 1.2, for
 * non-embedded non-base fonts, no substitutions or re-encodings are allowed.
 */

/* Forward references */
private int pdf_set_font_and_size(P3(gx_device_pdf * pdev, pdf_font_t * font,
				     floatp size));
private int pdf_set_text_matrix(P2(gx_device_pdf * pdev,
				   const gs_matrix * pmat));
private int pdf_append_chars(P3(gx_device_pdf * pdev, const byte * str,
				uint size));

/* Define the text enumerator. */
typedef struct pdf_text_enum_s {
    gs_text_enum_common;
    gs_text_enum_t *pte_default;
    gs_fixed_point origin;
} pdf_text_enum_t;
extern_st(st_gs_text_enum);
gs_private_st_suffix_add1(st_pdf_text_enum, pdf_text_enum_t, "pdf_text_enum_t",
  pdf_text_enum_enum_ptrs, pdf_text_enum_reloc_ptrs, st_gs_text_enum,
  pte_default);

/*
 * Define quantities derived from the current font and CTM, used within
 * the text processing loop.
 */
typedef struct pdf_text_process_state_s {
    float chars;		/* scaled character spacing (Tc) */
    float words;		/* scaled word spacing (Tw) */
    float size;			/* font size for Tf */
    gs_matrix text_matrix;	/* normalized FontMatrix * CTM for Tm */
    pdf_font_t *pdfont;
} pdf_text_process_state_t;

/* Define the auxiliary procedures for text processing. */
private int
pdf_text_resync(gs_text_enum_t *pte, const gs_text_enum_t *pfrom)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if ((pte->text.operation ^ pfrom->text.operation) & ~TEXT_FROM_ANY)
	return_error(gs_error_rangecheck);
    if (penum->pte_default) {
	int code = gs_text_resync(penum->pte_default, pfrom);

	if (code < 0)
	    return code;
    }
    pte->text = pfrom->text;
    gs_text_enum_copy_dynamic(pte, pfrom, false);
    return 0;
}
private bool
pdf_text_is_width_only(const gs_text_enum_t *pte)
{
    const pdf_text_enum_t *const penum = (const pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_is_width_only(penum->pte_default);
    return false;
}
private int
pdf_text_current_width(const gs_text_enum_t *pte, gs_point *pwidth)
{
    const pdf_text_enum_t *const penum = (const pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_current_width(penum->pte_default, pwidth);
    return_error(gs_error_rangecheck); /* can't happen */
}
private int
pdf_text_set_cache(gs_text_enum_t *pte, const double *pw,
		   gs_text_cache_control_t control)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_set_cache(penum->pte_default, pw, control);
    return_error(gs_error_rangecheck); /* can't happen */
}
private int
pdf_text_retry(gs_text_enum_t *pte)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if (penum->pte_default)
	return gs_text_retry(penum->pte_default);
    return_error(gs_error_rangecheck); /* can't happen */
}
private void
pdf_text_release(gs_text_enum_t *pte, client_name_t cname)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;

    if (penum->pte_default) {
	gs_text_release(penum->pte_default, cname);
	penum->pte_default = 0;
    }
    gx_default_text_release(pte, cname);
}

/* Begin processing text. */
private text_enum_proc_process(pdf_text_process);
private const gs_text_enum_procs_t pdf_text_procs = {
    pdf_text_resync, pdf_text_process,
    pdf_text_is_width_only, pdf_text_current_width,
    pdf_text_set_cache, pdf_text_retry,
    pdf_text_release
};
int
gdev_pdf_text_begin(gx_device * dev, gs_imager_state * pis,
		    const gs_text_params_t *text, gs_font * font,
		    gx_path * path, const gx_device_color * pdcolor,
		    const gx_clip_path * pcpath,
		    gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)dev;
    pdf_text_enum_t *penum;
    gs_fixed_point cpt;
    int code;

    if ((text->operation &
	 ~(TEXT_FROM_STRING | TEXT_FROM_BYTES |
	   TEXT_ADD_TO_ALL_WIDTHS | TEXT_ADD_TO_SPACE_WIDTH |
	   TEXT_REPLACE_WIDTHS |
	   TEXT_DO_DRAW | TEXT_RETURN_WIDTH)) != 0 ||
	!gx_dc_is_pure(pdcolor) ||
	gx_path_current_point(path, &cpt) < 0
	)
	return gx_default_text_begin(dev, pis, text, font, path, pdcolor,
				     pcpath, mem, ppte);

    /* Set the clipping path and color. */

    if (pdf_must_put_clip_path(pdev, pcpath)) {
	int code = pdf_open_page(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
	pdf_put_clip_path(pdev, pcpath);
    }
    pdf_set_color(pdev, gx_dc_pure_color(pdcolor), &pdev->fill_color,
		  &psdf_set_fill_color_commands);

    /* Allocate and initialize the enumerator. */

    rc_alloc_struct_1(penum, pdf_text_enum_t, &st_pdf_text_enum, mem,
		      return_error(gs_error_VMerror), "gdev_pdf_text_begin");
    penum->rc.free = rc_free_text_enum;
    penum->pte_default = 0; 
    code = gs_text_enum_init((gs_text_enum_t *)penum, &pdf_text_procs,
			     dev, pis, text, font, path, pdcolor, pcpath, mem);
    if (code < 0) {
	gs_free_object(mem, penum, "gdev_pdf_text_begin");
	return code;
    }

    if (text->operation & TEXT_RETURN_WIDTH)
	gx_path_current_point(path, &penum->origin);
    *ppte = (gs_text_enum_t *)penum;

    return 0;
}

/*
 * Create a new pdf_font for a gs_font.  This is a separate procedure only
 * for readability: it is only called from one place in
 * pdf_update_text_state.
 */
private int
create_pdf_font(gx_device_pdf *pdev, gs_font *font, const gs_matrix *pomat,
		pdf_font_t **pppf)
{
    int index = -1;
    pdf_font_t ftemp;
    int BaseEncoding = ENCODING_INDEX_UNKNOWN;
    int same = 0, base_same = 0;
    pdf_font_embed_t embed =
	pdf_font_embed_status(pdev, font, &index, &same);
    bool have_widths = false;
    bool is_standard = false;
    long ffid = 0;
    pdf_font_descriptor_t *pfd;
    gs_font *base_font = font;
    gs_font *below;
    pdf_font_descriptor_t fdesc;
    pdf_font_t *ppf;
    int code;
#define BASE_UID(fnt) (&((const gs_font_base *)(fnt))->UID)

    /* Find the "lowest" base font that has the same outlines. */
    while ((below = base_font->base) != base_font &&
	   base_font->procs.same_font(base_font, below,
				      FONT_SAME_OUTLINES))
	base_font = below;
 set_base:
    if (base_font == font)
	base_same = same;
    else
	embed = pdf_font_embed_status(pdev, base_font, &index, &base_same);
    if (embed == FONT_EMBED_STANDARD && pdev->std_fonts[index].font != 0) {
	/* Use the standard font as the base font. */
	base_font = pdev->std_fonts[index].font;
	is_standard = true;
    } else if (embed == FONT_EMBED_YES &&
	       base_font->FontType != ft_composite &&
	       uid_is_valid(BASE_UID(base_font)) &&
	       !base_font->is_resource
	       ) {
	/*
	 * The base font has a UID, but it isn't a resource.  Look for a
	 * resource with the same UID, in the hope that that will be
	 * longer-lived.
	 */
	gs_font *orig = base_font->dir->orig_fonts;

	for (; orig; orig = orig->next)
	    if (orig != base_font && orig->FontType == base_font->FontType &&
		orig->is_resource &&
		uid_equal(BASE_UID(base_font), BASE_UID(orig))
		) {
		/* Use this as the base font instead. */
		base_font = orig;
		/*
		 * Recompute the embedding status of the base font.  This
		 * can't lead to a loop, because base_font->is_resource is
		 * now known to be true.
		 */
		goto set_base;
	    }
    }
	 
    /* See if we already have a descriptor for this base font. */
    pfd = (pdf_font_descriptor_t *)
	pdf_find_resource_by_gs_id(pdev, resourceFontDescriptor,
				   base_font->id);
    if (pfd != 0 && pfd->base_font != base_font)
	pfd = 0;

    /* Create an appropriate font and descriptor. */

    switch (embed) {
    case FONT_EMBED_YES:
	/*
	 * HACK: Acrobat Reader 3 has a bug that makes cmap formats 4
	 * and 6 not work in embedded TrueType fonts.  Consequently, it
	 * can only handle embedded TrueType fonts if all the glyphs
	 * referenced by the Encoding have numbers 0-255.  Check for
	 * this now.
	 */
	if (font->FontType == ft_TrueType &&
	    pdev->CompatibilityLevel <= 1.2
	    ) {
	    int i;

	    for (i = 0; i <= 0xff; ++i) {
		gs_glyph glyph =
		    font->procs.encode_char(font, (gs_char)i,
					    GLYPH_SPACE_INDEX);

		if (glyph == gs_no_glyph ||
		    (glyph >= gs_min_cid_glyph &&
		     glyph <= gs_min_cid_glyph + 0xff)
		    )
		    continue;
		/* Can't embed, punt. */
		return_error(gs_error_rangecheck);
	    }
	}
	/*
	 * Acrobat Reader doesn't accept embedded Multiple Master
	 * instances, and we haven't been able to figure out why.
	 */
	if (font->FontType == ft_encrypted &&
	    ((const gs_font_type1 *)font)->data.WeightVector.count > 0)
	    return_error(gs_error_rangecheck);
	code = pdf_compute_font_descriptor(pdev, &fdesc, font, NULL);
	if (code < 0)
	    return code;
	ffid = pdf_obj_ref(pdev);
	goto wf;
    case FONT_EMBED_NO:
	/*
	 * Per the PDF 1.3 documentation, there are only 3 BaseEncoding
	 * values allowed for non-embedded fonts.  Pick one here.
	 */
	BaseEncoding =
	    ((const gs_font_base *)base_font)->nearest_encoding_index;
	switch (BaseEncoding) {
	default:
	    BaseEncoding = ENCODING_INDEX_WINANSI;
	case ENCODING_INDEX_WINANSI:
	case ENCODING_INDEX_MACROMAN:
	case ENCODING_INDEX_MACEXPERT:
	    break;
	}
	code = pdf_compute_font_descriptor(pdev, &fdesc, font, NULL);
	if (code < 0)
	    return code;
    wf:
	/* The font isn't standard: make sure we write the Widths. */
	same &= ~FONT_SAME_METRICS;
	/* falls through */
    case FONT_EMBED_STANDARD:
	if (~same & (FONT_SAME_METRICS | FONT_SAME_ENCODING)) {
	    /*
	     * Before allocating the font resource, check that we can
	     * get all the widths.
	     */
	    int i;

	    memset(&ftemp, 0, sizeof(ftemp));
	    for (i = 0; i <= 255; ++i) {
		code = pdf_char_width(&ftemp, i, font, NULL);
		if (code < 0 && code != gs_error_undefined)
		    return code;
	    }
	    have_widths = true;
	}
	if (pfd) {
	    code = pdf_alloc_font(pdev, font->id, &ppf, gs_no_id);
	    if (code < 0)
		return code;
	    if_debug4('_',
		      "[_]created pdf_font_t 0x%lx, id %ld, FontDescriptor 0x%lx, id %ld (old)\n",
		      (ulong)ppf, pdf_resource_id((pdf_resource_t *)ppf),
		      (ulong)pfd, pdf_resource_id((pdf_resource_t *)pfd));
	    ppf->FontDescriptor = pfd;
	} else {
	    int name_index = index;

	    code = pdf_alloc_font(pdev, font->id, &ppf, base_font->id);
	    if (code < 0)
		return code;
	    pfd = ppf->FontDescriptor;
	    if_debug4('_',
		      "[_]created pdf_font_t 0x%lx, id %ld, FontDescriptor 0x%lx, id %ld (new)\n",
		      (ulong)ppf, pdf_resource_id((pdf_resource_t *)ppf),
		      (ulong)pfd, pdf_resource_id((pdf_resource_t *)pfd));
	    if (index < 0) {
		int ignore_same;

		memcpy(pfd->FontName.chars, base_font->font_name.chars,
		       base_font->font_name.size);
		pfd->FontName.size = base_font->font_name.size;
		pfd->values = fdesc.values;
		pfd->FontFile_id = ffid;
		pfd->base_font = base_font;
		pfd->orig_matrix = *pomat;
		/* Don't allow non-standard fonts with standard names. */
		pdf_font_embed_status(pdev, base_font, &name_index,
				      &ignore_same);
	    } else {
		/* Use the standard name. */
		const pdf_standard_font_t *ppsf = &pdf_standard_fonts[index];
		const char *fnchars = ppsf->fname;
		uint fnsize = strlen(fnchars);

		memcpy(pfd->FontName.chars, fnchars, fnsize);
		pfd->FontName.size = fnsize;
	    }
	    if (!is_standard) {
		code = pdf_adjust_font_name(pdev, pfd, name_index >= 0);
		if (code < 0)
		    return code;
	    }
	}
	ppf->FontType = font->FontType;
	ppf->index = index;
	switch (font->FontType) {
	case ft_encrypted:
	case ft_encrypted2:
	    ppf->is_MM_instance =
		((const gs_font_type1 *)font)->data.WeightVector.count > 0;
	default:
	    DO_NOTHING;
	}
	ppf->BaseEncoding = BaseEncoding;
	ppf->fname = pfd->FontName;
	ppf->font = font;
	if (~same & FONT_SAME_METRICS) {
	    /*
	     * Contrary to the PDF 1.3 documentation, FirstChar and
	     * LastChar are *not* simply a way to strip off initial and
	     * final entries in the Widths array that are equal to
	     * MissingWidth.  Acrobat Reader assumes that characters
	     * with codes less than FirstChar or greater than LastChar
	     * are undefined, without bothering to consult the Encoding.
	     * Therefore, the implicit value of MissingWidth is pretty
	     * useless, because there must be explicit Width entries for
	     * every character in the font that is ever used.
	     * Furthermore, if there are several subsets of the same
	     * font in a document, it appears to be random as to which
	     * one Acrobat Reader uses to decide what the FirstChar and
	     * LastChar values are.  Therefore, we must write the Widths
	     * array for the entire font even for subsets.
	     */
	    ppf->write_Widths = true;
	    pdf_find_char_range(font, &ppf->FirstChar, &ppf->LastChar);
	}
	if (have_widths) {
	    /*
	     * C's bizarre coercion rules make us use memcpy here
	     * rather than direct assignments, even though sizeof()
	     * gives the correct value....
	     */
	    memcpy(ppf->Widths, ftemp.Widths, sizeof(ppf->Widths));
	    memcpy(ppf->widths_known, ftemp.widths_known,
		   sizeof(ppf->widths_known));
	}
	code = pdf_register_font(pdev, font, ppf);
    }

    *pppf = ppf;
    return code;
}

/*
 * Compute the cached values in the text state from the text parameters,
 * current_font, and pis->ctm.
 */
private int
pdf_update_text_state(pdf_text_process_state_t *ppts,
		      const pdf_text_enum_t *penum, const gs_matrix *pfmat)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)penum->dev;
    gs_font *font = penum->current_font;
    pdf_font_t *ppf;
    gs_fixed_point cpt;
    gs_matrix orig_matrix, smat, tmat;
    double
	sx = pdev->HWResolution[0] / 72.0,
	sy = pdev->HWResolution[1] / 72.0;
    float chars, words, size;
    int code = gx_path_current_point(penum->path, &cpt);

    if (code < 0)
	return code;

    /* PDF always uses 1000 units per em for font metrics. */
    switch (font->FontType) {
    case ft_TrueType:
	/*
	 * ****** HACK ALERT ******
	 *
	 * The code above that calls pdf_find_char_range does so the first
	 * time the font is used.  This causes an incorrect (too small)
	 * Widths array to be written if the font is downloaded
	 * incrementally.  In practice, this appears only to be a problem
	 * for TrueType fonts written by certain Windows drivers
	 * (including AdobePS5 for Windows NT).  Fortunately, it is easy
	 * to determine whether the font is an incremental one: this is
	 * the case iff the font doesn't have glyf and loca entries.
	 * In this case, we punt and treat the font as bitmaps.
	 */
	if (((const gs_font_type42 *)font)->data.glyf == 0)
	    return_error(gs_error_rangecheck); /* incremental */
	/* The TrueType FontMatrix is 1 unit per em, which is what we want. */
	gs_make_identity(&orig_matrix);
	break;
    case ft_encrypted:
    case ft_encrypted2:
	gs_make_scaling(0.001, 0.001, &orig_matrix);
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    DISCARD(pdf_find_orig_font(pdev, font, &orig_matrix));

    /* Compute the scaling matrix and combined matrix. */

    gs_matrix_invert(&orig_matrix, &smat);
    gs_matrix_multiply(&smat, pfmat, &smat);
    tmat = ctm_only(penum->pis);
    tmat.tx = tmat.ty = 0;
    gs_matrix_multiply(&smat, &tmat, &tmat);

    /* Try to find a reasonable size value.  This isn't necessary, */
    /* but it's worth a little effort. */

    size = fabs(tmat.yy) / sy;
    if (size < 0.01)
	size = fabs(tmat.xx) / sx;
    if (size < 0.01)
	size = 1;

    /* Check for spacing parameters we can handle, and transform them. */

    if (penum->text.operation & TEXT_ADD_TO_ALL_WIDTHS) {
	gs_point pt;

	gs_distance_transform_inverse(penum->text.delta_all.x,
				      penum->text.delta_all.y,
				      &smat, &pt);
	if (pt.y != 0)
	    return_error(gs_error_rangecheck);
	chars = pt.x * size;
    } else
	chars = 0.0;

    if (penum->text.operation & TEXT_ADD_TO_SPACE_WIDTH) {
	gs_point pt;

	gs_distance_transform_inverse(penum->text.delta_space.x,
				      penum->text.delta_space.y,
				      &smat, &pt);
	if (pt.y != 0 || penum->text.space.s_char != 32)
	    return_error(gs_error_rangecheck);
	words = pt.x * size;
    } else
	words = 0.0;

    /* Find or create the font resource. */

    ppf = (pdf_font_t *)
	pdf_find_resource_by_gs_id(pdev, resourceFont, font->id);
    /* Check for the possibility that the base font has been freed. */
    if (ppf && ppf->FontDescriptor->written)
	ppf = 0;
    if (ppf == 0 || ppf->font == 0) {
	code = create_pdf_font(pdev, font, &orig_matrix, &ppf);
	if (code < 0)
	    return code;
    }

    /* Store the updated values. */

    tmat.xx /= size;
    tmat.xy /= size;
    tmat.yx /= size;
    tmat.yy /= size;
    tmat.tx += fixed2float(cpt.x);
    tmat.ty += fixed2float(cpt.y);

    ppts->chars = chars;
    ppts->words = words;
    ppts->size = size;
    ppts->text_matrix = tmat;
    ppts->pdfont = ppf;

    return 0;
}

/* Check whether a glyph exists in a (pseudo-)encoding. */
private bool
encoding_has_glyph(gs_font_base *bfont, gs_glyph font_glyph,
		   gs_encoding_index_t index)
{
    int ch;
    gs_glyph glyph;

    for (ch = 0;
	 (glyph = bfont->procs.callbacks.known_encode((gs_char)ch, index)) !=
	     gs_no_glyph;
	 ++ch)
	if (glyph == font_glyph)
	    return true;
    return false;
}

/*
 * For a given character, check whether the encoding of bfont (the current
 * font) is compatible with that of the underlying unscaled, possibly
 * standard, base font, and if not, whether we can re-encode the character
 * using the base font's encoding.  Return the (possibly re-encoded)
 * character if successful.
 */
inline private void
record_used(pdf_font_descriptor_t *pfd, int c)
{
    pfd->chars_used[c >> 3] |= 1 << (c & 7);
}
private int
pdf_encode_char(gx_device_pdf *pdev, int chr, gs_font_base *bfont,
		pdf_font_t *ppf)
{
    pdf_font_descriptor_t *const pfd = ppf->FontDescriptor;
    /*
     * bfont is the current font in which the text is being shown.
     * ei is its encoding_index.
     */
    gs_encoding_index_t ei = bfont->encoding_index;
    /*
     * base_font is the font that underlies this PDF font (i.e., this PDF
     * font is base_font plus some possible Encoding and Widths differences,
     * and possibly a different FontMatrix).  base_font is 0 iff this PDF
     * font is one of the standard 14 (i.e., ppf->index !=
     * ENCODING_INDEX_UNKNOWN).  bei is the index of the BaseEncoding
     * (explicit or, for the standard fonts, implicit) that will be written
     * in the PDF file: it is not necessarily the same as
     * base_font->encoding_index, or even base_font->nearest_encoding_index.
     */
    gs_font *base_font = pfd->base_font;
    bool have_font = base_font != 0 && base_font->FontType != ft_composite;
    bool is_standard = ppf->index != ENCODING_INDEX_UNKNOWN;
    gs_encoding_index_t bei =
	(ppf->BaseEncoding != ENCODING_INDEX_UNKNOWN ? ppf->BaseEncoding :
	 is_standard ? pdf_standard_fonts[ppf->index].base_encoding :
	 ENCODING_INDEX_UNKNOWN);
    pdf_encoding_element_t *pdiff = ppf->Differences;
    /*
     * If set, font_glyph is the glyph currently associated with chr in
     * base_font + bei + diffs; glyph is the glyph corresponding to chr in
     * bfont.
     */
    gs_glyph font_glyph, glyph;
#define IS_USED(c)\
  (((pfd)->chars_used[(c) >> 3] & (1 << ((c) & 7))) != 0)

    if (ei == bei && ei != ENCODING_INDEX_UNKNOWN && pdiff == 0) {
	/*
	 * Just note that the character has been used with its original
	 * encoding.
	 */
	record_used(pfd, chr);
	return chr;
    }
    if (!is_standard && !have_font)
	return_error(gs_error_undefined); /* can't encode */

#define ENCODE_NO_DIFF(ch)\
   (bei != ENCODING_INDEX_UNKNOWN ?\
    bfont->procs.callbacks.known_encode((gs_char)(ch), bei) :\
    /* have_font */ bfont->procs.encode_char(base_font, chr, GLYPH_SPACE_NAME))
#define HAS_DIFF(ch) (pdiff != 0 && pdiff[ch].str.data != 0)
#define ENCODE_DIFF(ch) (pdiff[ch].glyph)
#define ENCODE(ch)\
  (HAS_DIFF(ch) ? ENCODE_DIFF(ch) : ENCODE_NO_DIFF(ch))

    font_glyph = ENCODE(chr);
    glyph =
	(ei == ENCODING_INDEX_UNKNOWN ?
	 bfont->procs.encode_char((gs_font *)bfont, chr, GLYPH_SPACE_NAME) :
	 bfont->procs.callbacks.known_encode(chr, ei));
    if (glyph == font_glyph) {
	record_used(pfd, chr);
	return chr;
    }

    if (ppf->index == ENCODING_INDEX_UNKNOWN && pfd->FontFile_id == 0 &&
	pdev->CompatibilityLevel <= 1.2
	) {
	/*
	 * Work around the bug in Acrobat Reader 3's Print function
	 * that makes re-encoded characters in substituted fonts
	 * print as blank.
	 */
	return_error(gs_error_undefined);
    }

    /*
     * TrueType fonts don't allow Differences in the encoding, but we might
     * be able to re-encode the character if it appears elsewhere in the
     * encoding.
     */
    if (bfont->FontType == ft_TrueType) {
	if (pdev->ReEncodeCharacters) {
	    int c;

	    for (c = 0; c < 256; ++c)
		if (ENCODE_NO_DIFF(c) == glyph) {
		    record_used(pfd, c);
		    return c;
		}
	}
	return_error(gs_error_undefined);
    }

    /*
     * If the font isn't going to be embedded, check whether this glyph is
     * available in the base font's glyph set at all.
     */
    if (pfd->FontFile_id == 0) {
	switch (bei) {
	case ENCODING_INDEX_STANDARD:
	case ENCODING_INDEX_ISOLATIN1:
	case ENCODING_INDEX_WINANSI:
	case ENCODING_INDEX_MACROMAN:
	    /* Check the full Adobe glyph set(s). */
	    if (!encoding_has_glyph(bfont, glyph, ENCODING_INDEX_ALOGLYPH) &&
		(pdev->CompatibilityLevel < 1.3 ||
		 !encoding_has_glyph(bfont, glyph, ENCODING_INDEX_ALXGLYPH))
		)
		return_error(gs_error_undefined);
	default:
	    break;
	}
    }

    if (pdev->ReAssignCharacters) {
	/*
	 * If this is the first time we've seen this character,
	 * assign the glyph to its position in the encoding.
	 */
	if (!HAS_DIFF(chr) && !IS_USED(chr)) {
	    int code =
		pdf_add_encoding_difference(pdev, ppf, chr, bfont, glyph);

	    if (code >= 0) {
		/*
		 * As noted in the comments at the beginning of this file,
		 * we don't have any way to record, for the purpose of
		 * subsetting, the fact that a particular unencoded glyph
		 * was used.  If this glyph doesn't appear anywhere else in
		 * the base encoding, fall back to writing the entire font.
		 */
		int c;

		for (c = 0; c < 256; ++c)
		    if (ENCODE_NO_DIFF(c) == glyph)
			break;
		if (c < 256)	/* found */
		    record_used(pfd, c);
		else		/* not found */
		    pfd->subset_ok = false;
		return chr;
	    }
	}
    }

    if (pdev->ReEncodeCharacters) {
	/*
	 * Look for the character at some other position in the
	 * encoding.
	 */
	int c, code;

	for (c = 0; c < 256; ++c) {
	    if (HAS_DIFF(c)) {
		if (ENCODE_DIFF(c) == glyph)
		    return c;
	    } else if (ENCODE_NO_DIFF(c) == glyph) {
		record_used(pfd, c);
		return c;
	    }
	}
	/*
	 * The character isn't encoded anywhere.  Look for a
	 * never-referenced .notdef position where we can put it.
	 */
	for (c = 0; c < 256; ++c) {
	    gs_const_string gnstr;

	    if (HAS_DIFF(c) || IS_USED(c))
		continue; /* slot already referenced */
	    font_glyph = ENCODE_NO_DIFF(c);
	    if (font_glyph == gs_no_glyph)
		break;
	    gnstr.data = (const byte *)
		bfont->procs.callbacks.glyph_name(font_glyph,
						  &gnstr.size);
	    if (gnstr.size == 7 &&
		!memcmp(gnstr.data, ".notdef", 7)
		)
		break;
	}
	if (c == 256)	/* no .notdef positions left */
	    return_error(gs_error_undefined);
	code = pdf_add_encoding_difference(pdev, ppf, c, bfont, glyph);
	if (code < 0)
	    return code;
	/* See under ReAssignCharacters above regarding the following: */
	pfd->subset_ok = false;
	return c;
    }

    return_error(gs_error_undefined);

#undef IS_USED
#undef ENCODE_NO_DIFF
#undef HAS_DIFF
#undef ENCODE_DIFF
#undef ENCODE
}

/*
 * Write out commands to make the output state match the processing state.
 */
private int
pdf_write_text_process_state(gx_device_pdf *pdev,
			     const pdf_text_process_state_t *ppts,
			     const gs_const_string *pstr)
{
    int code;
    stream *s;

    pdf_set_font_and_size(pdev, ppts->pdfont, ppts->size);
    code = pdf_set_text_matrix(pdev, &ppts->text_matrix);
    if (code < 0)
	return code;

    if (pdev->text.character_spacing != ppts->chars &&
	pstr->size + pdev->text.buffer_count > 1
	) {
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	s = pdev->strm;
	pprintg1(s, "%g Tc\n", ppts->chars);
	pdev->text.character_spacing = ppts->chars;
    }

    if (pdev->text.word_spacing != ppts->words &&
	(memchr(pstr->data, 32, pstr->size) ||
	 memchr(pdev->text.buffer, 32, pdev->text.buffer_count))
	) {
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	s = pdev->strm;
	pprintg1(s, "%g Tw\n", ppts->words);
	pdev->text.word_spacing = ppts->words;
    }

    return 0;
}

/*
 * Continue processing text.  Per the check in pdf_text_begin, we know the
 * operation is TEXT_FROM_STRING/BYTES, TEXT_DO_DRAW, and possibly
 * TEXT_ADD_TO_ALL_WIDTHS, TEXT_ADD_TO_SPACE_WIDTH, TEXT_REPLACE_WIDTHS,
 * and/or TEXT_RETURN_WIDTH.
 */
private int
pdf_text_process(gs_text_enum_t *pte)
{
    pdf_text_enum_t *const penum = (pdf_text_enum_t *)pte;
    gx_device_pdf *const pdev = (gx_device_pdf *)penum->dev;
    gs_text_enum_t *pte_default = penum->pte_default;
    gs_font *font;
    gs_matrix fmat;
    const gs_text_params_t *text = &pte->text;
    gs_text_params_t alt_text;
    pdf_text_process_state_t text_state;
    gs_const_string str;
    int i;
    byte strbuf[200];		/* arbitrary */
    int code;

 top:
    if (pte_default) {
	/* Continue processing using the default algorithms. */
	code = gs_text_process(pte_default);
	gs_text_enum_copy_dynamic(pte, pte_default, true);
	if (code)
	    return code;
	gs_text_release(pte_default, "pdf_text_process");
	penum->pte_default = pte_default = 0;
	/****** FOR NOW ******/
	return 0;
    }
    str.data = text->data.bytes;
    str.size = text->size;
    font = penum->current_font;
    fmat = font->FontMatrix;
 fnt:
    switch (font->FontType) {
    case ft_TrueType:
    case ft_encrypted:
    case ft_encrypted2:
	break;
    case ft_composite: {
	gs_font_type0 *const font0 = (gs_font_type0 *)font;
	/*
	 * Support just one special format, which is used by some versions
	 * of the Adobe PS4 driver even for single-byte text.
	 */
	switch (font0->data.FMapType) {
	case fmap_8_8:
	    if (str.size > sizeof(strbuf) * 2)
		goto dflt;
	    for (i = 0; i < str.size; i += 2) {
		if (str.data[i] != 0)
		    goto dflt;
		strbuf[i >> 1] = str.data[i + 1];
	    }
	    str.data = strbuf;
	    str.size >>= 1;
	    font = penum->current_font = font0->data.FDepVector[0];
	    /*
	     * The FontMatrix of descendant base fonts is not scaled by
	     * scalefont/makefont.
	     */
	    if (font->FontType != ft_composite)
		gs_matrix_multiply(&fmat, &font->FontMatrix, &fmat);
	    goto fnt;
	default:
	    goto dflt;
	}
	break;
    }
    default:
	goto dflt;
    }

    code = pdf_update_text_state(&text_state, penum, &fmat);
    if (code < 0)
	goto dflt;

    /* Check that all characters can be encoded. */

    for (i = 0; i < str.size; ++i) {
	int chr = str.data[i];
	int code = pdf_encode_char(pdev, chr, (gs_font_base *)font,
				   text_state.pdfont);

	if (code < 0)
	    goto dflt;
	if (code != chr) {
	    /*
	     * It really simplifies things if we can buffer
	     * the entire string locally in one piece....
	     */
	    if (str.data != strbuf) {
		if (str.size > sizeof(strbuf))
		    goto dflt;
		memcpy(strbuf, str.data, str.size);
		str.data = strbuf;
	    }
	    strbuf[i] = (byte)code;
	}
    }

    /* Bring the text-related parameters in the output up to date. */

    code = pdf_write_text_process_state(pdev, &text_state, &str);
    if (code < 0)
	goto dflt;

    if (text->operation & TEXT_REPLACE_WIDTHS) {
	gs_point w;
	gs_matrix tmat;

	w.x = w.y = 0;
	tmat = text_state.text_matrix;
	for (; pte->index < str.size; pte->index++, pte->xy_index++) {
	    gs_point d, dpt;

	    code = pdf_append_chars(pdev, str.data + pte->index, 1);
	    if (code < 0)
		return code;
	    gs_text_replaced_width(&pte->text, pte->xy_index, &d);
	    w.x += d.x, w.y += d.y;
	    gs_distance_transform(d.x, d.y, &ctm_only(pte->pis), &dpt);
	    tmat.tx += dpt.x;
	    tmat.ty += dpt.y;
	    if (pte->index + 1 < str.size) {
		code = pdf_set_text_matrix(pdev, &tmat);
		if (code < 0)
		    return code;
	    }
	}
	pte->returned.total_width = w;
	return
	    (text->operation & TEXT_RETURN_WIDTH ?
	     gx_path_add_point(pte->path, float2fixed(tmat.tx),
			       float2fixed(tmat.ty)) :
	     0);
    }
    code = pdf_append_chars(pdev, str.data + pte->index,
			    str.size - pte->index);
    pte->index = 0;
    if (code < 0)
	goto dflt;
    /*
     * If we don't need the widths, return now.  If the widths are
     * available directly from the font, compute and return the total
     * width now.  Otherwise, call the default implementation.
     */
    if (!(text->operation & TEXT_RETURN_WIDTH))
	return 0;
    {
	int i, w;
	double scale = (font->FontType == ft_TrueType ? 0.001 : 1.0);
	gs_point dpt;
	int num_spaces = 0;

	for (i = pte->index, w = 0; i < str.size; ++i) {
	    int cw;
	    int code =
		pdf_char_width(text_state.pdfont, str.data[i], font, &cw);

	    if (code < 0)
		goto dflt_w;
	    w += cw;
	    if (str.data[i] == ' ')
		++num_spaces;
	}
	gs_distance_transform(w * scale, 0.0, &font->FontMatrix, &dpt);
	if (text->operation & TEXT_ADD_TO_ALL_WIDTHS) {
	    int num_chars = str.size - pte->index;

	    dpt.x += penum->text.delta_all.x * num_chars;
	    dpt.y += penum->text.delta_all.y * num_chars;
	}
	if (text->operation & TEXT_ADD_TO_SPACE_WIDTH) {
	    dpt.x += penum->text.delta_space.x * num_spaces;
	    dpt.y += penum->text.delta_space.y * num_spaces;
	}
	pte->returned.total_width = dpt;
	gs_distance_transform(dpt.x, dpt.y, &ctm_only(pte->pis), &dpt);
	return gx_path_add_point(pte->path,
				 penum->origin.x + float2fixed(dpt.x),
				 penum->origin.y + float2fixed(dpt.y));
    }
 dflt_w:
    alt_text = *text;
    alt_text.operation ^= TEXT_DO_DRAW | TEXT_DO_CHARWIDTH;
    text = &alt_text;
 dflt:
    code = gx_default_text_begin(pte->dev, pte->pis, text, pte->current_font,
				 pte->path, pte->pdcolor, pte->pcpath,
				 pte->memory, &penum->pte_default);
    if (code < 0)
	return code;
    pte_default = penum->pte_default;
    gs_text_enum_copy_dynamic(pte_default, pte, false);
    goto top;
}

/* ---------------- Text and font utilities ---------------- */

/* Forward declarations */
private int assign_char_code(P1(gx_device_pdf * pdev));

/*
 * Set the current font and size, writing a Tf command if needed.
 */
private int
pdf_set_font_and_size(gx_device_pdf * pdev, pdf_font_t * font, floatp size)
{
    if (font != pdev->text.font || size != pdev->text.size) {
	int code = pdf_open_page(pdev, PDF_IN_TEXT);
	stream *s = pdev->strm;

	if (code < 0)
	    return code;
	pprints1(s, "/%s ", font->frname);
	pprintg1(s, "%g Tf\n", size);
	pdev->text.font = font;
	pdev->text.size = size;
    }
    font->used_on_page = true;
    return 0;
}

/*
 * Set the text matrix for writing text.
 * The translation component of the matrix is the text origin.
 * If the non-translation components of the matrix differ from the
 * current ones, write a Tm command; otherwise, write either a Td command
 * or a Tj command using space pseudo-characters.
 */
private int
set_text_distance(gs_point *pdist, const gs_point *ppt, const gs_matrix *pmat)
{
    double rounded;

    gs_distance_transform_inverse(pmat->tx - ppt->x, pmat->ty - ppt->y,
				  pmat, pdist);
    /* If the distance is very close to integers, round it. */
    if (fabs(pdist->x - (rounded = floor(pdist->x + 0.5))) < 0.0005)
	pdist->x = rounded;
    if (fabs(pdist->y - (rounded = floor(pdist->y + 0.5))) < 0.0005)
	pdist->y = rounded;
    return 0;
}
private int
pdf_set_text_matrix(gx_device_pdf * pdev, const gs_matrix * pmat)
{
    stream *s = pdev->strm;
    double sx = 72.0 / pdev->HWResolution[0],
	sy = 72.0 / pdev->HWResolution[1];
    int code;

    if (pmat->xx == pdev->text.matrix.xx &&
	pmat->xy == pdev->text.matrix.xy &&
	pmat->yx == pdev->text.matrix.yx &&
	pmat->yy == pdev->text.matrix.yy &&
    /*
     * If we aren't already in text context, BT will reset
     * the text matrix.
     */
	(pdev->context == PDF_IN_TEXT || pdev->context == PDF_IN_STRING)
	) {
	/* Use Td or a pseudo-character. */
	gs_point dist;

	set_text_distance(&dist, &pdev->text.current, pmat);
	if (dist.y == 0 && dist.x >= X_SPACE_MIN &&
	    dist.x <= X_SPACE_MAX &&
	    pdev->text.font != 0 &&
	    PDF_FONT_IS_SYNTHESIZED(pdev->text.font)
	    ) {			/* Use a pseudo-character. */
	    int dx = (int)dist.x;
	    int dx_i = dx - X_SPACE_MIN;
	    byte space_char = pdev->text.font->spaces[dx_i];

	    if (space_char == 0) {
		if (pdev->text.font != pdev->open_font)
		    goto td;
		code = assign_char_code(pdev);
		if (code <= 0)
		    goto td;
		space_char =
		    pdev->open_font->spaces[dx_i] =
		    (byte) code;
		if (pdev->space_char_ids[dx_i] == 0) {
		    /* Create the space char_proc now. */
		    char spstr[3 + 14 + 1];
		    stream *s;

		    sprintf(spstr, "%d 0 0 0 0 0 d1\n", dx);
		    pdev->space_char_ids[dx_i] = pdf_begin_separate(pdev);
		    s = pdev->strm;
		    pprintd1(s, "<</Length %d>>\nstream\n", strlen(spstr));
		    pprints1(s, "%sendstream\n", spstr);
		    pdf_end_separate(pdev);
		}
	    }
	    pdf_append_chars(pdev, &space_char, 1);
	    pdev->text.current.x += dist.x * pmat->xx;
	    return 0;
	}
      td:			/* Use Td. */
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	set_text_distance(&dist, &pdev->text.line_start, pmat);
	pprintg2(s, "%g %g Td\n", dist.x, dist.y);
    } else {			/* Use Tm. */
	code = pdf_open_page(pdev, PDF_IN_TEXT);
	if (code < 0)
	    return code;
	/*
	 * See stream_to_text in gdevpdf.c for why we need the following
	 * matrix adjustments.
	 */
	pprintg6(pdev->strm, "%g %g %g %g %g %g Tm\n",
		 pmat->xx * sx, pmat->xy * sy,
		 pmat->yx * sx, pmat->yy * sy,
		 pmat->tx * sx, pmat->ty * sy);
	pdev->text.matrix = *pmat;
    }
    pdev->text.line_start.x = pmat->tx;
    pdev->text.line_start.y = pmat->ty;
    pdev->text.current.x = pmat->tx;
    pdev->text.current.y = pmat->ty;
    return 0;
}

/* Append characters to a string being accumulated. */
private int
pdf_append_chars(gx_device_pdf * pdev, const byte * str, uint size)
{
    const byte *p = str;
    uint left = size;

    while (left)
	if (pdev->text.buffer_count == max_text_buffer) {
	    int code = pdf_open_page(pdev, PDF_IN_TEXT);

	    if (code < 0)
		return code;
	} else {
	    int code = pdf_open_page(pdev, PDF_IN_STRING);
	    uint copy;

	    if (code < 0)
		return code;
	    copy = min(max_text_buffer - pdev->text.buffer_count, left);
	    memcpy(pdev->text.buffer + pdev->text.buffer_count, p, copy);
	    pdev->text.buffer_count += copy;
	    p += copy;
	    left -= copy;
	}
    return 0;
}

/* ---------------- Synthesized fonts ---------------- */

/* Assign a code for a char_proc. */
private int
assign_char_code(gx_device_pdf * pdev)
{
    pdf_font_t *font = pdev->open_font;

    if (pdev->embedded_encoding_id == 0) {
	long id = pdf_begin_separate(pdev);
	stream *s = pdev->strm;
	int i;

	/*
	 * Even though the PDF reference documentation says that a
	 * BaseEncoding key is required unless the encoding is
	 * "based on the base font's encoding" (and there is no base
	 * font in this case), Acrobat 2.1 gives an error if the
	 * BaseEncoding key is present.
	 */
	pputs(s, "<</Type/Encoding/Differences[0");
	for (i = 0; i < 256; ++i) {
	    if (!(i & 15))
		pputs(s, "\n");
	    pprintd1(s, "/a%d", i);
	}
	pputs(s, "\n] >>\n");
	pdf_end_separate(pdev);
	pdev->embedded_encoding_id = id;
    }
    if (font == 0 || font->num_chars == 256) {
	/* Start a new synthesized font. */
	int code = pdf_alloc_font(pdev, gs_no_id, &font, gs_no_id);
	char *pc;

	if (code < 0)
	    return code;
	strcpy(font->frname, pdev->open_font_name);
	for (pc = font->frname; *pc == 'Z'; ++pc)
	    *pc = '@';
	if ((*pc)++ == 0)
	    *pc = 'A', pc[1] = 0;
	pdev->open_font = font;
	strcpy(pdev->open_font_name, font->frname);
    }
    return font->num_chars++;
}

/* Begin a CharProc for a synthesized (bitmap) font. */
int
pdf_begin_char_proc(gx_device_pdf * pdev, int w, int h, int x_width,
  int y_offset, gs_id id, pdf_char_proc_t ** ppcp, pdf_stream_position_t * ppos)
{
    pdf_resource_t *pres;
    pdf_char_proc_t *pcp;
    int char_code = assign_char_code(pdev);
    pdf_font_t *font = pdev->open_font;
    int code;

    if (char_code < 0)
	return char_code;
    code = pdf_begin_resource(pdev, resourceCharProc, id, &pres);
    if (code < 0)
	return code;
    pcp = (pdf_char_proc_t *) pres;
    pcp->font = font;
    pcp->char_next = font->char_procs;
    font->char_procs = pcp;
    pcp->char_code = char_code;
    pcp->width = w;
    pcp->height = h;
    pcp->x_width = x_width;
    pcp->y_offset = y_offset;
    font->max_y_offset = max(font->max_y_offset, h + (h >> 2));
    *ppcp = pcp;
    {
	stream *s = pdev->strm;

	/*
	 * The resource file is positionable, so rather than use an
	 * object reference for the length, we'll go back and fill it in
	 * at the end of the definition.  Take 10K as the longest
	 * definition we can handle.
	 */
	pputs(s, "<</Length     >>\nstream\n");
	ppos->start_pos = stell(s);
    }
    return 0;
}

/* End a CharProc. */
int
pdf_end_char_proc(gx_device_pdf * pdev, pdf_stream_position_t * ppos)
{
    stream *s = pdev->strm;
    long start_pos = ppos->start_pos;
    long end_pos = stell(s);
    long length = end_pos - start_pos;

    if (length > 9999)
	return_error(gs_error_limitcheck);
    sseek(s, start_pos - 14);
    pprintd1(s, "%d", length);
    sseek(s, end_pos);
    pputs(s, "endstream\n");
    pdf_end_separate(pdev);
    return 0;
}

/* Put out a reference to an image as a character in a synthesized font. */
int
pdf_do_char_image(gx_device_pdf * pdev, const pdf_char_proc_t * pcp,
		  const gs_matrix * pimat)
{
    pdf_set_font_and_size(pdev, pcp->font, 1.0);
    {
	gs_matrix tmat;

	tmat = *pimat;
	tmat.ty -= pcp->y_offset;
	pdf_set_text_matrix(pdev, &tmat);
    }
    pdf_append_chars(pdev, &pcp->char_code, 1);
    pdev->text.current.x += pcp->x_width * pdev->text.matrix.xx;
    return 0;
}
