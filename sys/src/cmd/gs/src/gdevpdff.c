/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdff.c,v 1.22 2001/10/05 05:40:41 lpd Exp $ */
/* Font handling for pdfwrite driver. */
#include "ctype_.h"
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include <stdlib.h>
#include "gx.h"
#include "gserrors.h"
#include "gsmalloc.h"		/* for patching font memory */
#include "gsmatrix.h"
#include "gspath.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfixed.h"		/* for gxfcache.h */
#include "gxfont.h"
#include "gxfcache.h"		/* for orig_fonts list */
#include "gxfcid.h"
#include "gxfont1.h"
#include "gxfont42.h"
#include "gxpath.h"		/* for getting current point */
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "gdevpdfo.h"
#include "gdevpsf.h"
#include "scommon.h"

/*
 * In our quest to work around Acrobat Reader quirks, we're resorting to
 * making all font names in the output unique by adding a suffix derived
 * from the PDF object number.  We hope to get rid of this someday....
 */
private const bool MAKE_FONT_NAMES_UNIQUE = true;

/* GC descriptors */
public_st_pdf_font();
public_st_pdf_char_proc();
public_st_pdf_font_descriptor();
private_st_pdf_encoding_element();
private ENUM_PTRS_WITH(pdf_encoding_elt_enum_ptrs, pdf_encoding_element_t *pe) {
    uint count = size / (uint)sizeof(*pe);

    if (index >= count)
	return 0;
    return ENUM_CONST_STRING(&pe[index].str);
} ENUM_PTRS_END
private RELOC_PTRS_WITH(pdf_encoding_elt_reloc_ptrs, pdf_encoding_element_t *pe)
    uint count = size / (uint)sizeof(*pe);
    uint i;

    for (i = 0; i < count; ++i)
	RELOC_CONST_STRING_VAR(pe[i].str);
RELOC_PTRS_END

/* Define the 14 standard built-in fonts. */
const pdf_standard_font_t pdf_standard_fonts[] = {
#define m(name, enc) {name, enc},
    pdf_do_std_fonts(m)
#undef m
    {0}
};

/* ---------------- Embedding status ---------------- */

/* Return the index of a standard font name, or -1 if missing. */
int
pdf_find_standard_font(const byte *str, uint size)
{
    const pdf_standard_font_t *ppsf;

    for (ppsf = pdf_standard_fonts; ppsf->fname; ++ppsf)
	if (strlen(ppsf->fname) == size &&
	    !strncmp(ppsf->fname, (const char *)str, size)
	    )
	    return ppsf - pdf_standard_fonts;
    return -1;
}

/*
 * If there is a standard font with the same appearance (CharStrings,
 * Private, WeightVector) as the given font, set *psame to the mask of
 * identical properties, and return the standard-font index; otherwise,
 * set *psame to 0 and return -1.
 */
private int
find_std_appearance(const gx_device_pdf *pdev, const gs_font_base *bfont,
		    int mask, int *psame)
{
    bool has_uid = uid_is_UniqueID(&bfont->UID) && bfont->UID.id != 0;
    const pdf_std_font_t *psf = pdev->std_fonts;
    int i;

    mask |= FONT_SAME_OUTLINES;
    for (i = 0; i < PDF_NUM_STD_FONTS; ++psf, ++i) {
	if (has_uid) {
	    if (!uid_equal(&bfont->UID, &psf->uid))
		continue;
	    if (!psf->font) {
		/*
		 * Identity of UIDs is supposed to guarantee that the
		 * fonts have the same outlines and metrics.
		 */
		*psame = FONT_SAME_OUTLINES | FONT_SAME_METRICS;
		return i;
	    }
	}
	if (psf->font) {
	    int same = *psame =
		bfont->procs.same_font((const gs_font *)bfont, psf->font,
				       mask);

	    if (same & FONT_SAME_OUTLINES)
		return i;
	}
    }
    *psame = 0;
    return -1;
}

/*
 * We register the fonts in pdev->std_fonts so that the pointers can
 * be weak (get set to 0 when the font is freed).
 */
private GS_NOTIFY_PROC(pdf_std_font_notify_proc);
typedef struct pdf_std_font_notify_s {
    gx_device_pdf *pdev;
    int index;			/* in std_fonts */
    gs_font *font;	/* for checking */
} pdf_std_font_notify_t;
gs_private_st_ptrs2(st_pdf_std_font_notify, pdf_std_font_notify_t,
		    "pdf_std_font_notify_t",
		    pdf_std_font_notify_enum_ptrs,
		    pdf_std_font_notify_reloc_ptrs,
		    pdev, font);
private int
pdf_std_font_notify_proc(void *vpsfn /*proc_data*/, void *event_data)
{
    pdf_std_font_notify_t *const psfn = vpsfn;
    gx_device_pdf *const pdev = psfn->pdev;
    gs_font *const font = psfn->font;

    if (event_data)
	return 0;		/* unknown event */
    if_debug4('_',
	      "[_]  notify 0x%lx: gs_font 0x%lx, id %ld, index=%d\n",
	      (ulong)psfn, (ulong)font, font->id, psfn->index);
#ifdef DEBUG
    if (pdev->std_fonts[psfn->index].font != font)
	lprintf3("pdf_std_font_notify font = 0x%lx, std_fonts[%d] = 0x%lx\n",
		 (ulong)font, psfn->index,
		 (ulong)pdev->std_fonts[psfn->index].font);
    else
#endif
	pdev->std_fonts[psfn->index].font = 0;
    gs_font_notify_unregister(font, pdf_std_font_notify_proc, vpsfn);
    gs_free_object(pdev->pdf_memory, vpsfn, "pdf_std_font_notify_proc");
    return 0;
}

/* Unregister the standard fonts when cleaning up. */
private void
pdf_std_font_unreg_proc(void *vpsfn /*proc_data*/)
{
    pdf_std_font_notify_proc(vpsfn, NULL);
}
void
pdf_unregister_fonts(gx_device_pdf *pdev)
{
    int j;

    for (j = 0; j < PDF_NUM_STD_FONTS; ++j)
	if (pdev->std_fonts[j].font != 0)
	    gs_notify_unregister_calling(&pdev->std_fonts[j].font->notify_list,
					 pdf_std_font_notify_proc, NULL,
					 pdf_std_font_unreg_proc);
}

/*
 * Scan a font directory for standard fonts.  Return true if any new ones
 * were found.
 */
private bool
scan_for_standard_fonts(gx_device_pdf *pdev, const gs_font_dir *dir)
{
    bool found = false;
    gs_font *orig = dir->orig_fonts;

    for (; orig; orig = orig->next) {
	gs_font_base *obfont;

	if (orig->FontType == ft_composite || !orig->is_resource)
	    continue;
	obfont = (gs_font_base *)orig;
	if (uid_is_UniqueID(&obfont->UID)) {
	    /* Is it one of the standard fonts? */
	    int i = pdf_find_standard_font(orig->key_name.chars,
					   orig->key_name.size);
	    pdf_std_font_t *psf;

	    if (i >= 0 && (psf = &pdev->std_fonts[i])->font == 0) {
		pdf_std_font_notify_t *psfn =
		    gs_alloc_struct(pdev->pdf_memory, pdf_std_font_notify_t,
				    &st_pdf_std_font_notify,
				    "scan_for_standard_fonts");

		if (psfn == 0)
		    continue;	/* can't register */
		psfn->pdev = pdev;
		psfn->index = i;
		psfn->font = orig;
		if_debug4('_',
			  "[_]register 0x%lx: gs_font 0x%lx, id %ld, index=%d\n",
			  (ulong)psfn, (ulong)orig, orig->id, i);
		gs_font_notify_register(orig, pdf_std_font_notify_proc, psfn);
		psf->font = orig;
		psf->orig_matrix = obfont->FontMatrix;
		psf->uid = obfont->UID;
		found = true;
	    }
	}
    }
    return found;
}

/*
 * Determine the embedding status of a font.  If the font is in the base
 * 14, store its index (0..13) in *pindex, otherwise store -1 there.
 */
private bool
font_is_symbolic(const gs_font *font)
{
    if (font->FontType == ft_composite)
	return true;		/* arbitrary */
    switch (((const gs_font_base *)font)->nearest_encoding_index) {
    case ENCODING_INDEX_STANDARD:
    case ENCODING_INDEX_ISOLATIN1:
    case ENCODING_INDEX_WINANSI:
    case ENCODING_INDEX_MACROMAN:
	return false;
    default:
	return true;
    }
}
private bool
embed_list_includes(const gs_param_string_array *psa, const byte *chars,
		    uint size)
{
    uint i;

    for (i = 0; i < psa->size; ++i)
	if (!bytes_compare(psa->data[i].data, psa->data[i].size, chars, size))
	    return true;
    return false;
}
pdf_font_embed_t
pdf_font_embed_status(gx_device_pdf *pdev, gs_font *font, int *pindex,
		      int *psame)
{
    const byte *chars = font->font_name.chars;
    uint size = font->font_name.size;

    /*
     * The behavior of Acrobat Distiller changed between 3.0 (PDF 1.2),
     * which will never embed the base 14 fonts, and 4.0 (PDF 1.3), which
     * doesn't treat them any differently from any other fonts.
     */
    if (pdev->CompatibilityLevel < 1.3) {
	/* Check whether the font is in the base 14. */
	int index = pdf_find_standard_font(chars, size);

	if (index >= 0) {
	    *pindex = index;
	    if (font->is_resource) {
		*psame = ~0;
		return FONT_EMBED_STANDARD;
	    } else if (font->FontType != ft_composite &&
		       find_std_appearance(pdev, (gs_font_base *)font, -1,
					   psame) == index)
		return FONT_EMBED_STANDARD;
	}
    }
    *pindex = -1;
    *psame = 0;
    /* Check the Embed lists. */
    if (embed_list_includes(&pdev->params.NeverEmbed, chars, size))
	return FONT_EMBED_NO;
    if (pdev->params.EmbedAllFonts || font_is_symbolic(font) ||
	embed_list_includes(&pdev->params.AlwaysEmbed, chars, size))
	return FONT_EMBED_YES;
    return FONT_EMBED_NO;
}

/* ---------------- Everything else ---------------- */

/*
 * Compute and return the orig_matrix of a font.
 */
int
pdf_font_orig_matrix(const gs_font *font, gs_matrix *pmat)
{
    switch (font->FontType) {
    case ft_composite:		/* subfonts have their own FontMatrix */
    case ft_TrueType:
    case ft_CID_TrueType:
	/* The TrueType FontMatrix is 1 unit per em, which is what we want. */
	gs_make_identity(pmat);
	break;
    case ft_encrypted:
    case ft_encrypted2:
    case ft_CID_encrypted:
	/*
	 * Type 1 fonts are supposed to use a standard FontMatrix of
	 * [0.001 0 0 0.001 0 0], with a 1000-unit cell.  However,
	 * Windows NT 4.0 creates Type 1 fonts, apparently derived from
	 * TrueType fonts, that use a 2048-unit cell and corresponding
	 * FontMatrix.  Detect and correct for this here.
	 */
	{
	    const gs_font *base_font = font;
	    double scale;

	    while (base_font->base != base_font)
		base_font = base_font->base;
	    if (base_font->FontMatrix.xx == 1.0/2048 &&
		base_font->FontMatrix.xy == 0 &&
		base_font->FontMatrix.yx == 0 &&
		base_font->FontMatrix.yy == 1.0/2048
		)
		scale = 1.0/2048;
	    else
		scale = 0.001;
	    gs_make_scaling(scale, scale, pmat);
	}
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    return 0;
}

/*
 * Find the original (unscaled) standard font corresponding to an
 * arbitrary font, if any.  Return its index in standard_fonts, or -1.
 */
int
pdf_find_orig_font(gx_device_pdf *pdev, gs_font *font, gs_matrix *pfmat)
{
    bool scan = true;
    int i;

    if (font->FontType == ft_composite)
	return -1;
    for (;; font = font->base) {
	gs_font_base *bfont = (gs_font_base *)font;
	int same;

	/* Look for a standard font with the same appearance. */
	i = find_std_appearance(pdev, bfont, 0, &same);
	if (i >= 0)
	    break;
	if (scan) {
	    /* Scan for fonts with any of the standard names that */
	    /* have a UID. */
	    bool found = scan_for_standard_fonts(pdev, font->dir);

	    scan = false;
	    if (found) {
		i = find_std_appearance(pdev, bfont, 0, &same);
		if (i >= 0)
		    break;
	    }
	}
	if (font->base == font)
	    return -1;
    }
    *pfmat = pdev->std_fonts[i].orig_matrix;
    return i;
}

/*
 * Determine the resource type (resourceFont or resourceCIDFont) for
 * a font.
 */
pdf_resource_type_t
pdf_font_resource_type(const gs_font *font)
{
    switch (font->FontType) {
    case ft_CID_encrypted:	/* CIDFontType 0 */
    case ft_CID_user_defined:	/* CIDFontType 1 */
    case ft_CID_TrueType:	/* CIDFontType 2 */
    case ft_CID_bitmap:		/* CIDFontType 4 */
	return resourceCIDFont;
    default:
	return resourceFont;
    }
}

/*
 * Determine the chars_count (number of entries in chars_used and Widths)
 * for a font.  For CIDFonts, this is CIDCount.
 */
private int
pdf_font_chars_count(const gs_font *font)
{
    switch (font->FontType) {
    case ft_composite:
	/* Composite fonts don't use chars_used or Widths. */
	return 0;
    case ft_CID_encrypted:
	return ((const gs_font_cid0 *)font)->cidata.common.CIDCount;
    case ft_CID_TrueType:
	return ((const gs_font_cid2 *)font)->cidata.common.CIDCount;
    default:
	return 256;		/* Encoding size */
    }
}

/*
 * Determine the glyphs_count (number of entries in glyphs_used) for a font.
 * This is only non-zero for TrueType-based fonts.
 */
private int
pdf_font_glyphs_count(const gs_font *font)
{
    switch (font->FontType) {
    case ft_TrueType:
    case ft_CID_TrueType:
	return ((const gs_font_type42 *)font)->data.numGlyphs;
    default:
	return 0;
    }
}

/*
 * Allocate a font resource.  If pfd != 0, a FontDescriptor is allocated,
 * with its id, values, and chars_count taken from *pfd_in.
 * If font != 0, its FontType is used to determine whether the resource
 * is of type Font or of (pseudo-)type CIDFont; in this case, pfres->font
 * and pfres->FontType are also set.
 */
int
pdf_alloc_font(gx_device_pdf *pdev, gs_id rid, pdf_font_t **ppfres,
	       const pdf_font_descriptor_t *pfd_in, gs_font *font)
{
    gs_memory_t *mem = pdev->v_memory;
    pdf_font_descriptor_t *pfd = 0;
    pdf_resource_type_t rtype = resourceFont;
    gs_string chars_used, glyphs_used;
    int *Widths = 0;
    byte *widths_known = 0;
    ushort *CIDToGIDMap = 0;
    int code;
    pdf_font_t *pfres;

    chars_used.data = 0;
    glyphs_used.data = 0;
    if (pfd_in) {
	uint chars_count = pfd_in->chars_count;
	uint glyphs_count = pfd_in->glyphs_count;

	code = pdf_alloc_resource(pdev, resourceFontDescriptor,
				  pfd_in->rid, (pdf_resource_t **)&pfd, 0L);
	if (code < 0)
	    return code;
	chars_used.size = (chars_count + 7) >> 3;
	chars_used.data = gs_alloc_string(mem, chars_used.size,
					  "pdf_alloc_font(chars_used)");
	if (chars_used.data == 0)
	    goto vmfail;
	if (glyphs_count) {
	    glyphs_used.size = (glyphs_count + 7) >> 3;
	    glyphs_used.data = gs_alloc_string(mem, glyphs_used.size,
					       "pdf_alloc_font(glyphs_used)");
	    if (glyphs_used.data == 0)
		goto vmfail;
	    memset(glyphs_used.data, 0, glyphs_used.size);
	}
	memset(chars_used.data, 0, chars_used.size);
	pfd->values = pfd_in->values;
	pfd->chars_count = chars_count;
	pfd->chars_used = chars_used;
	pfd->glyphs_count = glyphs_count;
	pfd->glyphs_used = glyphs_used;
	pfd->do_subset = FONT_SUBSET_OK;
	pfd->FontFile_id = 0;
	pfd->base_font = 0;
	pfd->notified = false;
	pfd->written = false;
    }
    if (font) {
	uint chars_count = pdf_font_chars_count(font);
	uint widths_known_size = (chars_count + 7) >> 3;

	Widths = (void *)gs_alloc_byte_array(mem, chars_count, sizeof(*Widths),
					     "pdf_alloc_font(Widths)");
	widths_known = gs_alloc_bytes(mem, widths_known_size,
				      "pdf_alloc_font(widths_known)");
	if (Widths == 0 || widths_known == 0)
	    goto vmfail;
	if (font->FontType == ft_CID_TrueType) {
	    CIDToGIDMap = (void *)
		gs_alloc_byte_array(mem, chars_count, sizeof(*CIDToGIDMap),
				    "pdf_alloc_font(CIDToGIDMap)");
	    if (CIDToGIDMap == 0)
		goto vmfail;
	    memset(CIDToGIDMap, 0, chars_count * sizeof(*CIDToGIDMap));
	}
	memset(widths_known, 0, widths_known_size);
	rtype = pdf_font_resource_type(font);
    }
    code = pdf_alloc_resource(pdev, rtype, rid, (pdf_resource_t **)ppfres, 0L);
    if (code < 0)
	goto fail;
    pfres = *ppfres;
    memset((byte *)pfres + sizeof(pdf_resource_t), 0,
	   sizeof(*pfres) - sizeof(pdf_resource_t));
    pfres->font = font;
    if (font)
	pfres->FontType = font->FontType;
    pfres->index = -1;
    pfres->is_MM_instance = false;
    pfres->FontDescriptor = pfd;
    pfres->write_Widths = false;
    pfres->Widths = Widths;
    pfres->widths_known = widths_known;
    pfres->BaseEncoding = ENCODING_INDEX_UNKNOWN;
    pfres->Differences = 0;
    pfres->DescendantFont = 0;
    pfres->glyphshow_font = 0;
    pfres->CIDToGIDMap = CIDToGIDMap;
    pfres->char_procs = 0;
    return 0;
 vmfail:
    code = gs_note_error(gs_error_VMerror);
 fail:
    gs_free_object(mem, CIDToGIDMap, "pdf_alloc_font(CIDToGIDMap)");
    gs_free_object(mem, widths_known, "pdf_alloc_font(widths_known)");
    gs_free_object(mem, Widths, "pdf_alloc_font(Widths)");
    if (glyphs_used.data)
	gs_free_string(mem, glyphs_used.data, glyphs_used.size,
		       "pdf_alloc_font(glyphs_used)");
    if (chars_used.data)
	gs_free_string(mem, chars_used.data, chars_used.size,
		       "pdf_alloc_font(chars_used)");
    gs_free_object(mem, pfd, "pdf_alloc_font(descriptor)");
    return code;
}

/*
 * Create a new pdf_font for a gs_font.  This procedure is only intended
 * to be called from a few places in gdevpdft.c.
 */
int
pdf_create_pdf_font(gx_device_pdf *pdev, gs_font *font, const gs_matrix *pomat,
		    pdf_font_t **pppf)
{
    int index = -1;
    int ftemp_Widths[256];
    byte ftemp_widths_known[256/8];
    int BaseEncoding = ENCODING_INDEX_UNKNOWN;
    int same = 0, base_same = 0;
    pdf_font_embed_t embed =
	pdf_font_embed_status(pdev, font, &index, &same);
    bool have_widths = false;
    bool is_standard = false;
    long ffid = 0;
    pdf_font_descriptor_t *pfd = 0;
    pdf_std_font_t *psf = 0;
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
    /*
     * set_base is the head of a logical loop; we return here if we
     * decide to change the base_font to one registered as a resource.
     */
 set_base:
    if (base_font == font)
	base_same = same;
    else
	embed = pdf_font_embed_status(pdev, base_font, &index, &base_same);
    if (embed == FONT_EMBED_STANDARD) {
	psf = &pdev->std_fonts[index];
	if (psf->font != 0 || psf->pfd != 0) {
	    /*
	     * Use the standard font as the base font.  Either base_font
	     * or pfd may be zero, but not both.
	     */
	    base_font = psf->font;
	    pfd = psf->pfd;
	}
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
	 
    /* Composite fonts don't have descriptors. */
    if (font->FontType == ft_composite) {
	code = pdf_alloc_font(pdev, font->id, &ppf, NULL, font);
	if (code < 0)
	    return code;
	if_debug2('_',
		  "[_]created ft_composite pdf_font_t 0x%lx, id %ld\n",
		  (ulong)ppf, pdf_resource_id((pdf_resource_t *)ppf));
	ppf->index = -1;
	code = pdf_register_font(pdev, font, ppf);
	*pppf = ppf;
	return code;
    }

    /* See if we already have a descriptor for this base font. */
    if (pfd == 0)		/* if non-zero, was standard font */
	pfd = (pdf_font_descriptor_t *)
	    pdf_find_resource_by_gs_id(pdev, resourceFontDescriptor,
				       base_font->id);
    if (pfd != 0 && pfd->base_font != base_font && pfd->base_font != 0)
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
	if ((font->FontType == ft_TrueType ||
	     font->FontType == ft_CID_TrueType) &&
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
	if (!pfd) {
	    code = pdf_compute_font_descriptor(pdev, &fdesc, font, NULL);
	    if (code < 0)
		return code;
	    ffid = pdf_obj_ref(pdev);
	}
	/* The font isn't standard: make sure we write the Widths. */
	same &= ~FONT_SAME_METRICS;
	break;
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
	/* The font isn't standard: make sure we write the Widths. */
	same &= ~FONT_SAME_METRICS;
	break;
    case FONT_EMBED_STANDARD:
	break;
    }
    if (~same & (FONT_SAME_METRICS | FONT_SAME_ENCODING)) {
	/*
	 * Before allocating the font resource, check that we can
	 * get all the widths for non-CID-keyed fonts.
	 */
	switch (font->FontType) {
	case ft_composite:
	case ft_CID_encrypted:
	case ft_CID_TrueType:
	    break;
	default:
	    {
		pdf_font_t ftemp;
		int i;

		memset(&ftemp, 0, sizeof(ftemp));
		pdf_font_orig_matrix(font, &ftemp.orig_matrix);
		ftemp.Widths = ftemp_Widths;
		ftemp.widths_known = ftemp_widths_known;
		memset(ftemp.widths_known, 0, sizeof(ftemp_widths_known));
		for (i = 0; i <= 255; ++i) {
		    code = pdf_char_width(&ftemp, i, font, NULL);
		    if (code < 0 && code != gs_error_undefined)
			return code;
		}
		have_widths = true;
	    }
	}
    }
    if (pfd) {
	code = pdf_alloc_font(pdev, font->id, &ppf, NULL, font);
	if (code < 0)
	    return code;
	if_debug4('_',
		  "[_]created pdf_font_t 0x%lx, id %ld, FontDescriptor 0x%lx, id %ld (old)\n",
		  (ulong)ppf, pdf_resource_id((pdf_resource_t *)ppf),
		  (ulong)pfd, pdf_resource_id((pdf_resource_t *)pfd));
	ppf->FontDescriptor = pfd;
    } else {
	int name_index = index;

	fdesc.rid = base_font->id;
	fdesc.chars_count = pdf_font_chars_count(base_font);
	fdesc.glyphs_count = pdf_font_glyphs_count(base_font);
	code = pdf_alloc_font(pdev, font->id, &ppf, &fdesc, font);
	if (code < 0)
	    return code;
	pfd = ppf->FontDescriptor;
	if_debug4('_',
		  "[_]created pdf_font_t 0x%lx, id %ld, FontDescriptor 0x%lx, id %ld (new)\n",
		  (ulong)ppf, pdf_resource_id((pdf_resource_t *)ppf),
		  (ulong)pfd, pdf_resource_id((pdf_resource_t *)pfd));
	if (index < 0) {
	    int ignore_same;
	    const gs_font_name *pfname = &base_font->font_name;

	    /* CIDFonts may have a key_name but no font_name. */
	    if (pfname->size == 0)
		pfname = &base_font->key_name;
	    memcpy(pfd->FontName.chars, pfname->chars, pfname->size);
	    pfd->FontName.size = pfname->size;
	    pfd->FontFile_id = ffid;
	    pfd->base_font = base_font;
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
	    memset(&pfd->values, 0, sizeof(&pfd->values));
	}
	pfd->orig_matrix = *pomat;
	if (is_standard) {
	    psf->pfd = pfd;
	    if (embed == FONT_EMBED_STANDARD)
		pfd->base_font = 0;
	} else {
	    code = pdf_adjust_font_name(pdev, pfd, name_index >= 0);
	    if (code < 0)
		return code;
	}
    } /* end else (!pfd) */
    ppf->index = index;

    switch (font->FontType) {
    case ft_encrypted:
    case ft_encrypted2:
	ppf->is_MM_instance =
	    ((const gs_font_type1 *)font)->data.WeightVector.count > 0;
	break;
    case ft_CID_encrypted:
    case ft_CID_TrueType:
	/*
	 * Write the CIDSystemInfo now, so we don't try to access it after
	 * the font may no longer be available.
	 */
	{
	    long cidsi_id = pdf_begin_separate(pdev);

	    pdf_write_CIDFont_system_info(pdev, font);
	    pdf_end_separate(pdev);
	    ppf->CIDSystemInfo_id = cidsi_id;
	}
    default:
	DO_NOTHING;
    }

    ppf->BaseEncoding = BaseEncoding;
    ppf->fname = pfd->FontName;
    ppf->orig_matrix = *pomat;
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
	/*
	 * If the font is being downloaded incrementally, the range we
	 * determine here will be too small.  The character encoding
	 * loop in pdf_process_string takes care of expanding it.
	 */
	pdf_find_char_range(font, &ppf->FirstChar, &ppf->LastChar);
    }
    if (have_widths) {
	memcpy(ppf->Widths, ftemp_Widths, sizeof(ftemp_Widths));
	memcpy(ppf->widths_known, ftemp_widths_known,
	       sizeof(ftemp_widths_known));
    }
    code = pdf_register_font(pdev, font, ppf);

    *pppf = ppf;
    return code;
}

/*
 * Determine whether a font is a subset font by examining the name.
 */
bool
pdf_has_subset_prefix(const byte *str, uint size)
{
    int i;

    if (size < SUBSET_PREFIX_SIZE || str[SUBSET_PREFIX_SIZE - 1] != '+')
	return false;
    for (i = 0; i < SUBSET_PREFIX_SIZE - 1; ++i)
	if ((uint)(str[i] - 'A') >= 26)
	    return false;
    return true;
}

/*
 * Make the prefix for a subset font from the font's resource ID.
 */
void
pdf_make_subset_prefix(const gx_device_pdf *pdev, byte *str, ulong id)
{
    int i;
    ulong v;

    /* We disregard the id and generate a random prefix. */
    v = (ulong)(pdev->random_offset + rand());
    for (i = 0; i < SUBSET_PREFIX_SIZE - 1; ++i, v /= 26)
	str[i] = 'A' + (v % 26);
    str[SUBSET_PREFIX_SIZE - 1] = '+';
}

/*
 * Adjust the FontName of a newly created FontDescriptor so that it is
 * unique if necessary.
 */
int
pdf_adjust_font_name(const gx_device_pdf *pdev, pdf_font_descriptor_t *pfd,
		     bool is_standard)
{
    int code = 0;

    if (MAKE_FONT_NAMES_UNIQUE) {
	/* Check whether this name has already been used. */
	int j = 0;
	pdf_font_descriptor_t *old;
	byte *chars = pfd->FontName.chars;
	uint size = pfd->FontName.size;

#define SUFFIX_CHAR '~'
	/*
	 * If the name looks as though it has one of our unique suffixes,
	 * remove the suffix.
	 */
	{
	    int i;

	    for (i = size;
		 i > 0 && isxdigit(chars[i - 1]);
		 --i)
		DO_NOTHING;
	    if (i < size && i > 0 && chars[i - 1] == SUFFIX_CHAR) {
		do {
		    --i;
		} while (i > 0 && chars[i - 1] == SUFFIX_CHAR);
		size = i + 1;
	    }
	    code = size != pfd->FontName.size;
	}
	/*
	 * Non-standard fonts with standard names must always use a suffix
	 * to avoid being confused with the standard fonts.
	 */
	if (!is_standard)
	    for (; j < NUM_RESOURCE_CHAINS; ++j)
		for (old = (pdf_font_descriptor_t *)pdev->resources[resourceFontDescriptor].chains[j];
		     old != 0; old = old->next
		     ) {
		    const byte *old_chars = old->FontName.chars;
		    uint old_size = old->FontName.size;

		    if (old == pfd)
			continue;
		    if (pdf_has_subset_prefix(old_chars, old_size))
			old_chars += SUBSET_PREFIX_SIZE,
			    old_size -= SUBSET_PREFIX_SIZE;
		    if (!bytes_compare(old_chars, old_size, chars, size))
			goto found; /* do a double 'break' */
		}
    found:			/* only used for double 'break' above */
	if (j < NUM_RESOURCE_CHAINS) {
	    /* Create a unique name. */
	    char suffix[sizeof(long) * 2 + 2];
	    uint suffix_size;

	    sprintf(suffix, "%c%lx", SUFFIX_CHAR,
		    pdf_resource_id((pdf_resource_t *)pfd));
	    suffix_size = strlen(suffix);
	    if (size + suffix_size > sizeof(pfd->FontName.chars))
		return_error(gs_error_rangecheck);
	    memcpy(chars + size, (const byte *)suffix, suffix_size);
	    size += suffix_size;
	    code = 1;
	}
	pfd->FontName.size = size;
#undef SUFFIX_CHAR
    }
    return code;
}

/* Add an encoding difference to a font. */
int
pdf_add_encoding_difference(gx_device_pdf *pdev, pdf_font_t *ppf, int chr,
			    gs_font_base *bfont, gs_glyph glyph)
{
    pdf_encoding_element_t *pdiff = ppf->Differences;
    /*
     * Since the font Widths are indexed by character code, changing the
     * encoding also changes the Widths.
     */
    int width;
    int code = pdf_glyph_width(ppf, glyph, (gs_font *)bfont, &width);

    if (code < 0)
	return code;
    if (pdiff == 0) {
	pdiff = gs_alloc_struct_array(pdev->pdf_memory, 256,
				      pdf_encoding_element_t,
				      &st_pdf_encoding_element,
				      "Differences");
	if (pdiff == 0)
	    return_error(gs_error_VMerror);
	memset(pdiff, 0, sizeof(pdf_encoding_element_t) * 256);
	ppf->Differences = pdiff;
    }
    pdiff[chr].glyph = glyph;
    pdiff[chr].str.data = (const byte *)
	bfont->procs.callbacks.glyph_name(glyph, &pdiff[chr].str.size);
    ppf->Widths[chr] = width;
    if (code == 0)
	ppf->widths_known[chr >> 3] |= 0x80 >> (chr & 7);
    else
	ppf->widths_known[chr >> 3] &= ~(0x80 >> (chr & 7));
    return 0;
}

/*
 * Get the width of a given character in a (base) font.  May add the width
 * to the widths cache (ppf->Widths).
 */
int
pdf_char_width(pdf_font_t *ppf, int ch, gs_font *font,
	       int *pwidth /* may be NULL */)
{
    if (ch < 0 || ch > 255)
	return_error(gs_error_rangecheck);
    if (!(ppf->widths_known[ch >> 3] & (0x80 >> (ch & 7)))) {
	gs_font_base *bfont = (gs_font_base *)font;
	gs_glyph glyph = bfont->procs.encode_char(font, (gs_char)ch,
						  GLYPH_SPACE_INDEX);
	int width = 0;
	int code = pdf_glyph_width(ppf, glyph, font, &width);

	if (code < 0)
	    return code;
	ppf->Widths[ch] = width;
	if (code == 0)
	    ppf->widths_known[ch >> 3] |= 0x80 >> (ch & 7);
    }
    if (pwidth)
	*pwidth = ppf->Widths[ch];
    return 0;
}

/*
 * Get the width of a glyph in a (base) font.  Return 1 if the width was
 * defaulted to MissingWidth.
 */
int
pdf_glyph_width(pdf_font_t *ppf, gs_glyph glyph, gs_font *font,
		int *pwidth /* must not be NULL */)
{
    int wmode = font->WMode;
    gs_glyph_info_t info;
    /*
     * orig_matrix.xx is 1.0 for TrueType, 0.001 or 1.0/2048 for Type 1.
     */
    double scale = ppf->orig_matrix.xx * 1000.0;
    int code;

    if (glyph != gs_no_glyph &&
	(code = font->procs.glyph_info(font, glyph, NULL,
				       GLYPH_INFO_WIDTH0 << wmode,
				       &info)) >= 0
	) {
	double w, v;

	if (wmode && (w = info.width[wmode].y) != 0)
	    v = info.width[wmode].x;
	else
	    w = info.width[wmode].x, v = info.width[wmode].y;
	if (v != 0)
	    return_error(gs_error_rangecheck);
	*pwidth = (int)(w * scale);
	/*
	 * If the character is .notdef, don't cache the width,
	 * just in case this is an incrementally defined font.
	 */
	return (gs_font_glyph_is_notdef((gs_font_base *)font, glyph) ? 1 : 0);
    } else {
	/* Try for MissingWidth. */
	gs_point scale2;
	const gs_point *pscale = 0;
	gs_font_info_t finfo;

	if (scale != 1)
	    scale2.x = scale2.y = scale, pscale = &scale2;
	code = font->procs.font_info(font, pscale, FONT_INFO_MISSING_WIDTH,
				     &finfo);
	if (code < 0)
	    return code;
	*pwidth = finfo.MissingWidth;
	/*
	 * Don't mark the width as known, just in case this is an
	 * incrementally defined font.
	 */
	return 1;
    }
}

/*
 * Find the range of character codes that includes all the defined
 * characters in a font.  This is a separate procedure only for
 * readability: it is only called from one place in pdf_update_text_state.
 */
void
pdf_find_char_range(gs_font *font, int *pfirst, int *plast)
{
    gs_glyph notdef = gs_no_glyph;
    int first = 0, last = 255;
    gs_glyph glyph;

    switch (font->FontType) {
    case ft_encrypted:
    case ft_encrypted2: {
	/* Scan the Encoding vector looking for .notdef. */
	gs_font_base *const bfont = (gs_font_base *)font;
	int ch;

	for (ch = 0; ch <= 255; ++ch) {
	    gs_glyph glyph =
		font->procs.encode_char(font, (gs_char)ch,
					GLYPH_SPACE_INDEX);

	    if (glyph == gs_no_glyph)
		continue;
	    if (gs_font_glyph_is_notdef(bfont, glyph)) {
		notdef = glyph;
		break;
	    }
	}
	break;
    }
    default:
	DO_NOTHING;
    }
    while (last >= first &&
	   ((glyph =
	     font->procs.encode_char(font, (gs_char)last,
				     GLYPH_SPACE_INDEX))== gs_no_glyph ||
	    glyph == notdef || glyph == gs_min_cid_glyph)
	   )
	--last;
    while (first <= last &&
	   ((glyph =
	     font->procs.encode_char(font, (gs_char)first,
				     GLYPH_SPACE_INDEX))== gs_no_glyph ||
	    glyph == notdef || glyph == gs_min_cid_glyph)
	   )
	++first;
    if (first > last)
	last = first;	/* no characters used */
    *pfirst = first;
    *plast = last;
}

/* Compute the FontDescriptor.values for a font. */
private int
font_char_bbox(gs_rect *pbox, gs_glyph *pglyph, gs_font *font, int ch,
	       const gs_matrix *pmat)
{
    gs_glyph glyph;
    gs_glyph_info_t info;
    int code;

    glyph = font->procs.encode_char(font, (gs_char)ch, GLYPH_SPACE_INDEX);
    if (glyph == gs_no_glyph)
	return gs_error_undefined;
    code = font->procs.glyph_info(font, glyph, pmat, GLYPH_INFO_BBOX, &info);
    if (code < 0)
	return code;
    *pbox = info.bbox;
    if (pglyph)
	*pglyph = glyph;
    return 0;
}
int
pdf_compute_font_descriptor(gx_device_pdf *pdev, pdf_font_descriptor_t *pfd,
			    gs_font *font, const byte *used /*[32]*/)
{
    gs_font_base *bfont = (gs_font_base *)font;
    gs_glyph glyph, notdef;
    int index;
    int wmode = font->WMode;
    int members = (GLYPH_INFO_WIDTH0 << wmode) |
	GLYPH_INFO_BBOX | GLYPH_INFO_NUM_PIECES;
    gs_glyph letters[52];
    int num_letters = 0;
    pdf_font_descriptor_values_t desc;
    gs_matrix smat;
    gs_matrix *pmat = NULL;
    int fixed_width = 0;
    int small_descent = 0, small_height = 0;
    int code;

    memset(&desc, 0, sizeof(desc));
    desc.FontType = font->FontType;
    desc.FontBBox.p.x = desc.FontBBox.p.y = max_int;
    desc.FontBBox.q.x = desc.FontBBox.q.y = min_int;
    /*
     * Embedded TrueType fonts use a 1000-unit character space, but the
     * font itself uses a 1-unit space.  Compensate for this here.
     */
    switch (font->FontType) {
    case ft_TrueType:
    case ft_CID_TrueType:
	gs_make_scaling(1000.0, 1000.0, &smat);
	pmat = &smat;
    default:
	break;
    }
    /*
     * See the note on FONT_IS_ADOBE_ROMAN / FONT_USES_STANDARD_ENCODING
     * in gdevpdff.h for why the following substitution is made.
     */
#if 0
#  define CONSIDER_FONT_SYMBOLIC(font) font_is_symbolic(font)
#else
#  define CONSIDER_FONT_SYMBOLIC(font)\
  ((font)->FontType == ft_composite ||\
   ((const gs_font_base *)(font))->encoding_index != ENCODING_INDEX_STANDARD)
#endif
    if (CONSIDER_FONT_SYMBOLIC(font))
	desc.Flags |= FONT_IS_SYMBOLIC;
    else {
	/*
	 * Look at various specific characters to guess at the remaining
	 * descriptor values (CapHeight, ItalicAngle, StemV, XHeight,
	 * and flags SERIF, SCRIPT, ITALIC, ALL_CAPS, and SMALL_CAPS).
	 * The algorithms are pretty crude.
	 */
	/*
	 * Look at the glyphs for the lower-case letters.  If they are
	 * all missing, this is an all-cap font; if any is present, check
	 * the relative heights to determine whether this is a small-cap font.
	 */
	bool small_present = false;
	int ch;
	int x_height = min_int;
	int cap_height = 0;
	gs_rect bbox, bbox2;

	desc.Flags |= FONT_IS_ADOBE_ROMAN; /* required if not symbolic */
	for (ch = 'a'; ch <= 'z'; ++ch) {
	    int y0, y1;

	    code =
		font_char_bbox(&bbox, &letters[num_letters], font, ch, pmat);
	    if (code < 0)
		continue;
	    ++num_letters;
	    rect_merge(desc.FontBBox, bbox);
	    small_present = true;
	    y0 = (int)bbox.p.y;
	    y1 = (int)bbox.q.y;
	    switch (ch) {
	    case 'b': case 'd': case 'f': case 'h':
	    case 'k': case 'l': case 't': /* ascender */
		small_height = max(small_height, y1);
	    case 'i':		/* anomalous ascent */
		break;
	    case 'j':		/* descender with anomalous ascent */
		small_descent = min(small_descent, y0);
		break;
	    case 'g': case 'p': case 'q': case 'y': /* descender */
		small_descent = min(small_descent, y0);
	    default:		/* no ascender or descender */
		x_height = max(x_height, y1);		
	    }
	}
	desc.XHeight = (int)x_height;
	if (!small_present)
	    desc.Flags |= FONT_IS_ALL_CAPS;
	for (ch = 'A'; ch <= 'Z'; ++ch) {
	    code =
		font_char_bbox(&bbox, &letters[num_letters], font, ch, pmat);
	    if (code < 0)
		continue;
	    ++num_letters;
	    rect_merge(desc.FontBBox, bbox);
	    cap_height = max(cap_height, (int)bbox.q.y);
	}
	desc.CapHeight = cap_height;
	/*
	 * Look at various glyphs to determine ItalicAngle, StemV,
	 * SERIF, SCRIPT, and ITALIC.
	 */
	if ((code = font_char_bbox(&bbox, NULL, font, ':', pmat)) >= 0 &&
	    (code = font_char_bbox(&bbox2, NULL, font, '.', pmat)) >= 0
	    ) {
	    /* Calculate the dominant angle. */
	    int angle = 
		(int)(atan2((bbox.q.y - bbox.p.y) - (bbox2.q.y - bbox2.p.y),
			    (bbox.q.x - bbox.p.x) - (bbox2.q.x - bbox2.p.x)) *
		      radians_to_degrees) - 90;

	    /* Normalize to [-90..90]. */
	    while (angle > 90)
		angle -= 180;
	    while (angle < -90)
		angle += 180;
	    if (angle < -30)
		angle = -30;
	    else if (angle > 30)
		angle = 30;
	    /*
	     * For script or embellished fonts, we can get an angle that is
	     * slightly off from zero even for non-italic fonts.
	     * Compensate for this now.
	     */
	    if (angle <= 2 && angle >= -2)
		angle = 0;
	    desc.ItalicAngle = angle;
	}
	if (desc.ItalicAngle)
	    desc.Flags |= FONT_IS_ITALIC;
	if (code >= 0) {
	    double wdot = bbox2.q.x - bbox2.p.x;

	    if ((code = font_char_bbox(&bbox2, NULL, font, 'I', pmat)) >= 0) {
		double wcolon = bbox.q.x - bbox.p.x;
		double wI = bbox2.q.x - bbox2.p.x;

		desc.StemV = (int)wdot;
		if (wI > wcolon * 2.5 || wI > (bbox2.q.y - bbox2.p.y) * 0.25)
		    desc.Flags |= FONT_IS_SERIF;
	    }
	}
    }
    /*
     * Scan the entire glyph space to compute Ascent, Descent, FontBBox,
     * and the fixed width if any.  Avoid computing the bounding box of
     * letters a second time.
     */
    num_letters = psf_sort_glyphs(letters, num_letters);
    desc.Ascent = desc.FontBBox.q.y;
    notdef = gs_no_glyph;
    for (index = 0;
	 (code = font->procs.enumerate_glyph(font, &index, GLYPH_SPACE_INDEX, &glyph)) >= 0 &&
	     index != 0;
	 ) {
	gs_glyph_info_t info;

	if (psf_sorted_glyphs_include(letters, num_letters, glyph)) {
	    /* We don't need the bounding box. */
	    code = font->procs.glyph_info(font, glyph, pmat,
					  members - GLYPH_INFO_BBOX, &info);
	    if (code < 0)
		return code;
	} else {
	    code = font->procs.glyph_info(font, glyph, pmat, members, &info);
	    if (code < 0)
		return code;
	    rect_merge(desc.FontBBox, info.bbox);
	    if (!info.num_pieces)
		desc.Ascent = max(desc.Ascent, info.bbox.q.y);
	}
	if (notdef == gs_no_glyph && gs_font_glyph_is_notdef(bfont, glyph)) {
	    notdef = glyph;
	    desc.MissingWidth = info.width[wmode].x;
	}
	if (info.width[wmode].y != 0)
	    fixed_width = min_int;
	else if (fixed_width == 0)
	    fixed_width = info.width[wmode].x;
	else if (info.width[wmode].x != fixed_width)
	    fixed_width = min_int;
    }
    if (code < 0)
	return code;
    if (desc.Ascent == 0)
	desc.Ascent = desc.FontBBox.q.y;
    desc.Descent = desc.FontBBox.p.y;
    if (!(desc.Flags & (FONT_IS_SYMBOLIC | FONT_IS_ALL_CAPS)) &&
	(small_descent > desc.Descent / 3 || desc.XHeight > small_height * 0.9)
	)
	desc.Flags |= FONT_IS_SMALL_CAPS;
    if (fixed_width > 0) {
	desc.Flags |= FONT_IS_FIXED_WIDTH;
	desc.AvgWidth = desc.MaxWidth = desc.MissingWidth = fixed_width;
    }
    if (desc.CapHeight == 0)
	desc.CapHeight = desc.Ascent;
    if (desc.StemV == 0)
	desc.StemV = (int)(desc.FontBBox.q.x * 0.15);
    pfd->values = desc;
    return 0;
}
