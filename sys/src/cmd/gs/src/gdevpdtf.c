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

/* $Id: gdevpdtf.c,v 1.46 2005/09/12 11:34:50 leonardo Exp $ */
/* Font and CMap resource implementation for pdfwrite text */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gxfcache.h"		/* for orig_fonts list */
#include "gxfcid.h"
#include "gxfcmap.h"
#include "gxfcopy.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gdevpsf.h"
#include "gdevpdfx.h"
#include "gdevpdtb.h"
#include "gdevpdtd.h"
#include "gdevpdtf.h"
#include "gdevpdtw.h"

/* GC descriptors */
public_st_pdf_font_resource();
private_st_pdf_encoding1();
private_st_pdf_encoding_element();
private_st_pdf_standard_font();
private_st_pdf_standard_font_element();
private_st_pdf_outline_fonts();

private
ENUM_PTRS_WITH(pdf_font_resource_enum_ptrs, pdf_font_resource_t *pdfont)
ENUM_PREFIX(st_pdf_resource, 12);
case 0: return ENUM_STRING(&pdfont->BaseFont);
case 1: ENUM_RETURN(pdfont->FontDescriptor);
case 2: ENUM_RETURN(pdfont->base_font);
case 3: ENUM_RETURN(pdfont->Widths);
case 4: ENUM_RETURN(pdfont->used);
case 5: ENUM_RETURN(pdfont->res_ToUnicode);
case 6: ENUM_RETURN(pdfont->cmap_ToUnicode);
case 7: switch (pdfont->FontType) {
 case ft_composite:
     ENUM_RETURN(pdfont->u.type0.DescendantFont);
 case ft_CID_encrypted:
 case ft_CID_TrueType:
     ENUM_RETURN(pdfont->u.cidfont.Widths2);
 default:
     ENUM_RETURN(pdfont->u.simple.Encoding);
}
case 8: switch (pdfont->FontType) {
 case ft_composite:
     return (pdfont->u.type0.cmap_is_standard ? ENUM_OBJ(0) :
	     ENUM_CONST_STRING(&pdfont->u.type0.CMapName));
 case ft_encrypted:
 case ft_encrypted2:
 case ft_TrueType:
 case ft_user_defined:
     ENUM_RETURN(pdfont->u.simple.v);
 case ft_CID_encrypted:
 case ft_CID_TrueType:
     ENUM_RETURN(pdfont->u.cidfont.v);
 default:
     ENUM_RETURN(0);
}
case 9: switch (pdfont->FontType) {
 case ft_user_defined:
     ENUM_RETURN(pdfont->u.simple.s.type3.char_procs);
 case ft_CID_encrypted:
 case ft_CID_TrueType:
     ENUM_RETURN(pdfont->u.cidfont.CIDToGIDMap);
 default:
     ENUM_RETURN(0);
}
case 10: switch (pdfont->FontType) {
 case ft_user_defined:
     ENUM_RETURN(pdfont->u.simple.s.type3.cached);
 case ft_CID_encrypted:
 case ft_CID_TrueType:
     ENUM_RETURN(pdfont->u.cidfont.parent);
 default:
     ENUM_RETURN(0);
}
case 11: switch (pdfont->FontType) {
 case ft_user_defined:
     ENUM_RETURN(pdfont->u.simple.s.type3.used_resources);
 case ft_CID_encrypted:
 case ft_CID_TrueType:
     ENUM_RETURN(pdfont->u.cidfont.used2);
 default:
     ENUM_RETURN(0);
}
ENUM_PTRS_END
private
RELOC_PTRS_WITH(pdf_font_resource_reloc_ptrs, pdf_font_resource_t *pdfont)
{
    RELOC_PREFIX(st_pdf_resource);
    RELOC_STRING_VAR(pdfont->BaseFont);
    RELOC_VAR(pdfont->FontDescriptor);
    RELOC_VAR(pdfont->base_font);
    RELOC_VAR(pdfont->Widths);
    RELOC_VAR(pdfont->used);
    RELOC_VAR(pdfont->res_ToUnicode);
    RELOC_VAR(pdfont->cmap_ToUnicode);
    switch (pdfont->FontType) {
    case ft_composite:
	if (!pdfont->u.type0.cmap_is_standard)
	    RELOC_CONST_STRING_VAR(pdfont->u.type0.CMapName);
	RELOC_VAR(pdfont->u.type0.DescendantFont);
	break;
    case ft_user_defined:
	RELOC_VAR(pdfont->u.simple.Encoding);
	RELOC_VAR(pdfont->u.simple.v);
	RELOC_VAR(pdfont->u.simple.s.type3.char_procs);
	RELOC_VAR(pdfont->u.simple.s.type3.cached);
	RELOC_VAR(pdfont->u.simple.s.type3.used_resources);
	break;
    case ft_CID_encrypted:
    case ft_CID_TrueType:
	RELOC_VAR(pdfont->u.cidfont.Widths2);
	RELOC_VAR(pdfont->u.cidfont.v);
	RELOC_VAR(pdfont->u.cidfont.CIDToGIDMap);
	RELOC_VAR(pdfont->u.cidfont.parent);
	RELOC_VAR(pdfont->u.cidfont.used2);
	break;
    default:
	RELOC_VAR(pdfont->u.simple.Encoding);
	RELOC_VAR(pdfont->u.simple.v);
	break;
    }
}
RELOC_PTRS_END

/* ---------------- Standard fonts ---------------- */

/* ------ Private ------ */

/* Define the 14 standard built-in fonts. */
#define PDF_NUM_STANDARD_FONTS 14
typedef struct pdf_standard_font_info_s {
    const char *fname;
    int size;
    gs_encoding_index_t base_encoding;
} pdf_standard_font_info_t;
private const pdf_standard_font_info_t standard_font_info[] = {
    {"Courier",                7, ENCODING_INDEX_STANDARD},
    {"Courier-Bold",          12, ENCODING_INDEX_STANDARD},
    {"Courier-Oblique",       15, ENCODING_INDEX_STANDARD},
    {"Courier-BoldOblique",   19, ENCODING_INDEX_STANDARD},
    {"Helvetica",              9, ENCODING_INDEX_STANDARD},
    {"Helvetica-Bold",        14, ENCODING_INDEX_STANDARD},
    {"Helvetica-Oblique",     17, ENCODING_INDEX_STANDARD},
    {"Helvetica-BoldOblique", 21, ENCODING_INDEX_STANDARD},
    {"Symbol",                 6, ENCODING_INDEX_SYMBOL},
    {"Times-Roman",           11, ENCODING_INDEX_STANDARD},
    {"Times-Bold",            10, ENCODING_INDEX_STANDARD},
    {"Times-Italic",          12, ENCODING_INDEX_STANDARD},
    {"Times-BoldItalic",      16, ENCODING_INDEX_STANDARD},
    {"ZapfDingbats",          12, ENCODING_INDEX_DINGBATS},
    {0}
};

/* Return the index of a standard font name, or -1 if missing. */
private int
pdf_find_standard_font_name(const byte *str, uint size)
{
    const pdf_standard_font_info_t *ppsf;

    for (ppsf = standard_font_info; ppsf->fname; ++ppsf)
	if (ppsf->size == size &&
	    !memcmp(ppsf->fname, (const char *)str, size)
	    )
	    return ppsf - standard_font_info;
    return -1;
}

/*
 * If there is a standard font with the same appearance (CharStrings,
 * Private, WeightVector) as the given font, set *psame to the mask of
 * identical properties, and return the standard-font index; otherwise,
 * set *psame to 0 and return -1.
 */
private int
find_std_appearance(const gx_device_pdf *pdev, gs_font_base *bfont,
		    int mask, pdf_char_glyph_pair_t *pairs, int num_glyphs)
{
    bool has_uid = uid_is_UniqueID(&bfont->UID) && bfont->UID.id != 0;
    const pdf_standard_font_t *psf = pdf_standard_fonts(pdev);
    int i;

    switch (bfont->FontType) {
    case ft_encrypted:
    case ft_encrypted2:
    case ft_TrueType:
	break;
    default:
	return -1;
    }

    mask |= FONT_SAME_OUTLINES;
    for (i = 0; i < PDF_NUM_STANDARD_FONTS; ++psf, ++i) {
	gs_font_base *cfont;
	int code;

	if (!psf->pdfont)
	    continue;
	cfont = pdf_font_resource_font(psf->pdfont, false);
	if (has_uid) {
	    /*
	     * Require the UIDs to match.  The PostScript spec says this
	     * is the case iff the outlines are the same.
	     */
	    if (!uid_equal(&bfont->UID, &cfont->UID))
		continue;
	}
	/*
	 * Require the actual outlines to match (within the given subset).
	 */
	code = gs_copied_can_copy_glyphs((const gs_font *)cfont,
					 (const gs_font *)bfont,
					 &pairs[0].glyph, num_glyphs, 
					 sizeof(pdf_char_glyph_pair_t), true);
	if (code == gs_error_unregistered) /* Debug purpose only. */
	    return code;
	/* Note: code < 0 means an error. Skip it here. */
	if (code > 0)
	    return i;
    }
    return -1;
}

/*
 * Scan a font directory for standard fonts.  Return true if any new ones
 * were found.  A font is recognized as standard if it was loaded as a
 * resource, it has a UniqueId, and it has a standard name.
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
	    int i = pdf_find_standard_font_name(orig->key_name.chars,
						orig->key_name.size);

	    if (i >= 0 && pdf_standard_fonts(pdev)[i].pdfont == 0) {
		pdf_font_resource_t *pdfont;
		int code = pdf_font_std_alloc(pdev, &pdfont, true, orig->id, obfont,
					      i);

		if (code < 0)
		    continue;
		found = true;
	    }
	}
    }
    return found;
}

/* ---------------- Initialization ---------------- */

/*
 * Allocate and initialize bookkeeping for outline fonts.
 */
pdf_outline_fonts_t *
pdf_outline_fonts_alloc(gs_memory_t *mem)
{
    pdf_outline_fonts_t *pofs =
	gs_alloc_struct(mem, pdf_outline_fonts_t, &st_pdf_outline_fonts,
			"pdf_outline_fonts_alloc(outline_fonts)");
    pdf_standard_font_t *ppsf =
	gs_alloc_struct_array(mem, PDF_NUM_STANDARD_FONTS,
			      pdf_standard_font_t,
			      &st_pdf_standard_font_element,
			      "pdf_outline_fonts_alloc(standard_fonts)");

    if (pofs == 0 || ppsf == 0)
	return 0;
    memset(ppsf, 0, PDF_NUM_STANDARD_FONTS * sizeof(*ppsf));
    memset(pofs, 0, sizeof(*pofs));
    pofs->standard_fonts = ppsf;
    return pofs;
}

/*
 * Return the standard fonts array.
 */
pdf_standard_font_t *
pdf_standard_fonts(const gx_device_pdf *pdev)
{
    return pdev->text->outline_fonts->standard_fonts;
}

/*
 * Clean the standard fonts array.
 */
void
pdf_clean_standard_fonts(const gx_device_pdf *pdev)
{
    pdf_standard_font_t *ppsf = pdf_standard_fonts(pdev);

    memset(ppsf, 0, PDF_NUM_STANDARD_FONTS * sizeof(*ppsf));
}


/* ---------------- Font resources ---------------- */

/* ------ Private ------ */


private int pdf_resize_array(gs_memory_t *mem, void **p, int elem_size, int old_size, int new_size)
{
    void *q = gs_alloc_byte_array(mem, new_size, elem_size, "pdf_resize_array");

    if (q == NULL)
	return_error(gs_error_VMerror);
    memset((char *)q + elem_size * old_size, 0, elem_size * (new_size - old_size));
    memcpy(q, *p, elem_size * old_size);
    gs_free_object(mem, *p, "pdf_resize_array");
    *p = q;
    return 0;
}

/*
 * Allocate and (minimally) initialize a font resource.
 */
private int
font_resource_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
		    pdf_resource_type_t rtype, gs_id rid, font_type ftype,
		    int chars_count,
		    pdf_font_write_contents_proc_t write_contents)
{
    gs_memory_t *mem = pdev->pdf_memory;
    pdf_font_resource_t *pfres;
    double *widths = 0;
    byte *used = 0;
    int code;
    bool is_CID_font = (ftype == ft_CID_encrypted || ftype == ft_CID_TrueType);

    if (chars_count != 0) {
	uint size = (chars_count + 7) / 8;

	if (!is_CID_font) {
    	    widths = (void *)gs_alloc_byte_array(mem, chars_count, sizeof(*widths),
						"font_resource_alloc(Widths)");
	} else {
	    /*  Delay allocation because we don't know which WMode will be used. */
	}
	used = gs_alloc_bytes(mem, size, "font_resource_alloc(used)");
	if ((!is_CID_font && widths == 0) || used == 0) {
	    code = gs_note_error(gs_error_VMerror);
	    goto fail;
	}
	if (!is_CID_font)
	    memset(widths, 0, chars_count * sizeof(*widths));
	memset(used, 0, size);
    }
    code = pdf_alloc_resource(pdev, rtype, rid, (pdf_resource_t **)&pfres, 0L);
    if (code < 0)
	goto fail;
    memset((byte *)pfres + sizeof(pdf_resource_t), 0,
	   sizeof(*pfres) - sizeof(pdf_resource_t));
    pfres->FontType = ftype;
    pfres->count = chars_count;
    pfres->Widths = widths;
    pfres->used = used;
    pfres->write_contents = write_contents;
    pfres->res_ToUnicode = NULL;
    pfres->cmap_ToUnicode = NULL;
    *ppfres = pfres;
    return 0;
 fail:
    gs_free_object(mem, used, "font_resource_alloc(used)");
    gs_free_object(mem, widths, "font_resource_alloc(Widths)");
    return code;
}
private int
font_resource_simple_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
			   gs_id rid, font_type ftype, int chars_count,
			   pdf_font_write_contents_proc_t write_contents)
{
    pdf_font_resource_t *pfres;
    int code = font_resource_alloc(pdev, &pfres, resourceFont, rid, ftype,
				   chars_count, write_contents);

    if (code < 0)
	return code;
    pfres->u.simple.FirstChar = 256;
    pfres->u.simple.LastChar = -1;
    pfres->u.simple.BaseEncoding = -1;
    *ppfres = pfres;
    return 0;
}
int
font_resource_encoded_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
			    gs_id rid, font_type ftype,
			    pdf_font_write_contents_proc_t write_contents)
{
    pdf_encoding_element_t *Encoding =
	gs_alloc_struct_array(pdev->pdf_memory, 256, pdf_encoding_element_t,
			      &st_pdf_encoding_element,
			      "font_resource_encoded_alloc");
    gs_point *v = (gs_point *)gs_alloc_byte_array(pdev->pdf_memory, 
		    256, sizeof(gs_point), "pdf_font_simple_alloc");
    pdf_font_resource_t *pdfont;
    int code, i;


    if (v == 0 || Encoding == 0) {
	gs_free_object(pdev->pdf_memory, Encoding, 
		       "font_resource_encoded_alloc");
	gs_free_object(pdev->pdf_memory, v, 
	               "font_resource_encoded_alloc");
	return_error(gs_error_VMerror);
    }
    code = font_resource_simple_alloc(pdev, &pdfont, rid, ftype,
				      256, write_contents);
    if (code < 0) {
	gs_free_object(pdev->pdf_memory, Encoding, 
	               "font_resource_encoded_alloc");
	gs_free_object(pdev->pdf_memory, v, 
	               "font_resource_encoded_alloc");
	return_error(gs_error_VMerror);
    }
    if (code < 0) {
	return code;
    }
    memset(v, 0, 256 * sizeof(*v));
    memset(Encoding, 0, 256 * sizeof(*Encoding));
    for (i = 0; i < 256; ++i)
	Encoding[i].glyph = GS_NO_GLYPH;
    pdfont->u.simple.Encoding = Encoding;
    pdfont->u.simple.v = v;
    *ppfres = pdfont;
    return 0;
}

/*
 * Record whether a Type 1 or Type 2 font is a Multiple Master instance.
 */
private void
set_is_MM_instance(pdf_font_resource_t *pdfont, const gs_font_base *pfont)
{
    switch (pfont->FontType) {
    case ft_encrypted:
    case ft_encrypted2:
	pdfont->u.simple.s.type1.is_MM_instance =
	    ((const gs_font_type1 *)pfont)->data.WeightVector.count > 0;
    default:
	break;
    }
}

/* ------ Generic public ------ */

/* Resize font resource arrays. */
int 
pdf_resize_resource_arrays(gx_device_pdf *pdev, pdf_font_resource_t *pfres, int chars_count)
{
    /* This function fixes CID fonts that provide a lesser CIDCount than
       CIDs used in a document. Rather PS requires to print CID=0,
       we need to provide a bigger CIDCount since we don't 
       re-encode the text. The text should look fine if the 
       viewer application substitutes the font. */
    gs_memory_t *mem = pdev->pdf_memory;
    int code;
    
    if (chars_count < pfres->count)
	return 0;
    if (pfres->Widths != NULL) {
	code = pdf_resize_array(mem, (void **)&pfres->Widths, sizeof(*pfres->Widths), 
		    pfres->count, chars_count);    
	if (code < 0)
	    return code;
    }
    code = pdf_resize_array(mem, (void **)&pfres->used, sizeof(*pfres->used), 
		    (pfres->count + 7) / 8, (chars_count + 7) / 8);    
    if (code < 0)
	return code;
    if (pfres->FontType == ft_CID_encrypted || pfres->FontType == ft_CID_TrueType) {
	if (pfres->u.cidfont.v != NULL) {
	    code = pdf_resize_array(mem, (void **)&pfres->u.cidfont.v, 
		    sizeof(*pfres->u.cidfont.v), pfres->count * 2, chars_count * 2);    
	    if (code < 0)
		return code;
	}
	if (pfres->u.cidfont.Widths2 != NULL) {
	    code = pdf_resize_array(mem, (void **)&pfres->u.cidfont.Widths2, 
		    sizeof(*pfres->u.cidfont.Widths2), pfres->count, chars_count);    
	    if (code < 0)
		return code;
	}
    }
    if (pfres->FontType == ft_CID_TrueType) {
	if (pfres->u.cidfont.CIDToGIDMap != NULL) {
	    code = pdf_resize_array(mem, (void **)&pfres->u.cidfont.CIDToGIDMap, 
		    sizeof(*pfres->u.cidfont.CIDToGIDMap), pfres->count, chars_count);
	    if (code < 0)
		return code;
	}
    }
    if (pfres->FontType == ft_CID_encrypted || pfres->FontType == ft_CID_TrueType) {
	if (pfres->u.cidfont.used2 != NULL) {
	    code = pdf_resize_array(mem, (void **)&pfres->u.cidfont.used2, 
		    sizeof(*pfres->u.cidfont.used2), 
		    (pfres->count + 7) / 8, (chars_count + 7) / 8);
	    if (code < 0)
		return code;
	}
    }
    pfres->count = chars_count;
    return 0;
}

/* Get the object ID of a font resource. */
long
pdf_font_id(const pdf_font_resource_t *pdfont)
{
    return pdf_resource_id((const pdf_resource_t *)pdfont);
}

/*
 * Return the (copied, subset) font associated with a font resource.
 * If this font resource doesn't have one (Type 0 or Type 3), return 0.
 */
gs_font_base *
pdf_font_resource_font(const pdf_font_resource_t *pdfont, bool complete)
{
    if (pdfont->base_font != NULL)
	return pdf_base_font_font(pdfont->base_font, complete);
    if (pdfont->FontDescriptor == 0)
	return 0;
    return pdf_font_descriptor_font(pdfont->FontDescriptor, complete);
}

/*
 * Determine the embedding status of a font.  If the font is in the base
 * 14, store its index (0..13) in *pindex and its similarity to the base
 * font (as determined by the font's same_font procedure) in *psame.
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
private bool
embed_as_standard(gx_device_pdf *pdev, gs_font *font, int index,
		  pdf_char_glyph_pair_t *pairs, int num_glyphs)
{
    if (font->is_resource) {
	return true;
    }
    if (find_std_appearance(pdev, (gs_font_base *)font, -1,
			    pairs, num_glyphs) == index)
	return true;
    if (!scan_for_standard_fonts(pdev, font->dir))
	return false;
    return (find_std_appearance(pdev, (gs_font_base *)font, -1,
				pairs, num_glyphs) == index);
}
private bool
has_extension_glyphs(gs_font *pfont)
{
    psf_glyph_enum_t genum;
    gs_glyph glyph;
    gs_const_string str;
    int code, j, l;
    const int sl = strlen(gx_extendeg_glyph_name_separator);

    psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont, NULL, 0, GLYPH_SPACE_NAME);
    for (glyph = gs_no_glyph; (code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1; ) {
	code = pfont->procs.glyph_name(pfont, glyph, &str);
	if (code < 0)
	    return code;
	l = str.size - sl, j;
	for (j = 0; j < l; j ++)
	    if (!memcmp(gx_extendeg_glyph_name_separator, str.data + j, sl))
		return true;
    }
    psf_enumerate_glyphs_reset(&genum);
    return false;
}

/*
 * Choose a name for embedded font.
 */
const gs_font_name *pdf_choose_font_name(gs_font *font, bool key_name)
{
    return key_name ? (font->key_name.size != 0 ? &font->key_name : &font->font_name)
	             : (font->font_name.size != 0 ? &font->font_name : &font->key_name);
}
pdf_font_embed_t
pdf_font_embed_status(gx_device_pdf *pdev, gs_font *font, int *pindex,
		      pdf_char_glyph_pair_t *pairs, int num_glyphs)
{
    const gs_font_name *fn = pdf_choose_font_name(font, false);
    const byte *chars = fn->chars;
    uint size = fn->size;
    int index = pdf_find_standard_font_name(chars, size);
    bool embed_as_standard_called = false;
    bool do_embed_as_standard = false; /* Quiet compiler. */

    /*
     * The behavior of Acrobat Distiller changed between 3.0 (PDF 1.2),
     * which will never embed the base 14 fonts, and 4.0 (PDF 1.3), which
     * doesn't treat them any differently from any other fonts.  However,
     * if any of the base 14 fonts is not embedded, it still requires
     * special treatment.
     */
    if (pindex)
	*pindex = index;
    if (pdev->PDFX)
	return FONT_EMBED_YES;
    if (pdev->CompatibilityLevel < 1.3) {
	if (index >= 0 && 
	    (embed_as_standard_called = true,
		do_embed_as_standard = embed_as_standard(pdev, font, index, pairs, num_glyphs))) {
	    if (pdev->ForOPDFRead && has_extension_glyphs(font))
		return FONT_EMBED_YES;
	    return FONT_EMBED_STANDARD;
	}
    }
    /* Check the Embed lists. */
    if (!embed_list_includes(&pdev->params.NeverEmbed, chars, size) ||
 	(index >= 0 && 
	    !(embed_as_standard_called ? do_embed_as_standard :
	     (embed_as_standard_called = true,
	      (do_embed_as_standard = embed_as_standard(pdev, font, index, pairs, num_glyphs)))))
 	/* Ignore NeverEmbed for a non-standard font with a standard name */
 	) {
	if (pdev->params.EmbedAllFonts || font_is_symbolic(font) ||
	    embed_list_includes(&pdev->params.AlwaysEmbed, chars, size))
	    return FONT_EMBED_YES;
    }
    if (index >= 0 && 
	(embed_as_standard_called ? do_embed_as_standard :
	 embed_as_standard(pdev, font, index, pairs, num_glyphs)))
	return FONT_EMBED_STANDARD;
    return FONT_EMBED_NO;
}

/*
 * Compute the BaseFont of a font according to the algorithm described
 * in gdevpdtf.h.
 */
int
pdf_compute_BaseFont(gx_device_pdf *pdev, pdf_font_resource_t *pdfont, bool finish)
{
    pdf_font_resource_t *pdsubf = pdfont;
    gs_string fname;
    uint size, extra = 0;
    byte *data;

    if (pdfont->FontType == ft_composite) {
	int code;

	pdsubf = pdfont->u.type0.DescendantFont;
	code = pdf_compute_BaseFont(pdev, pdsubf, finish);
	if (code < 0)
	    return code;
	fname = pdsubf->BaseFont;
	if (pdsubf->FontType == ft_CID_encrypted || pdsubf->FontType == ft_CID_TrueType)
	    extra = 1 + pdfont->u.type0.CMapName.size;
    }
    else if (pdfont->FontDescriptor == 0) {
	/* Type 3 font, or has its BaseFont computed in some other way. */
	return 0;
    } else
	fname = *pdf_font_descriptor_base_name(pdsubf->FontDescriptor);
    size = fname.size;
    data = gs_alloc_string(pdev->pdf_memory, size + extra,
			   "pdf_compute_BaseFont");
    if (data == 0)
	return_error(gs_error_VMerror);
    memcpy(data, fname.data, size);
    switch (pdfont->FontType) {
    case ft_composite:
	if (extra) {
	    data[size] = '-';
	    memcpy(data + size + 1, pdfont->u.type0.CMapName.data, extra - 1);
	    size += extra;
	}
	break;
    case ft_encrypted:
    case ft_encrypted2:
	if (pdfont->u.simple.s.type1.is_MM_instance &&
	    !pdf_font_descriptor_embedding(pdfont->FontDescriptor)
	    ) {
	    /* Replace spaces by underscores in the base name. */
	    uint i;

	    for (i = 0; i < size; ++i)
		if (data[i] == ' ')
		    data[i] = '_';
	}
	break;
    case ft_TrueType:
    case ft_CID_TrueType: {
	/* Remove spaces from the base name. */
	uint i, j;

	for (i = j = 0; i < size; ++i)
	    if (data[i] != ' ')
		data[j++] = data[i];
	data = gs_resize_string(pdev->pdf_memory, data, i, j,
				"pdf_compute_BaseFont");
	size = j;
	break;
    }
    default:
	break;
    }
    pdfont->BaseFont.data = fname.data = data;
    pdfont->BaseFont.size = fname.size = size;
    /* Compute names for subset fonts. */
    if (finish && pdfont->FontDescriptor != NULL &&
	pdf_font_descriptor_is_subset(pdfont->FontDescriptor) &&
	!pdf_has_subset_prefix(fname.data, fname.size) &&
	pdf_font_descriptor_embedding(pdfont->FontDescriptor)
	) {
	int code = pdf_add_subset_prefix(pdev, &fname, pdfont->used, pdfont->count);

	if (code < 0)
	    return code;
        pdfont->BaseFont = fname;
	/* Don't write a UID for subset fonts. */
	uid_set_invalid(&pdf_font_resource_font(pdfont, false)->UID);
    }
    if (pdfont->FontType != ft_composite && pdsubf->FontDescriptor)
	*pdf_font_descriptor_name(pdsubf->FontDescriptor) = fname;
    return 0;
}

/* ------ Type 0 ------ */

/* Allocate a Type 0 font resource. */
int
pdf_font_type0_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
		     gs_id rid, pdf_font_resource_t *DescendantFont, 
		     const gs_const_string *CMapName)
{
    int code = font_resource_alloc(pdev, ppfres, resourceFont, rid,
				   ft_composite, 0, pdf_write_contents_type0);

    if (code >= 0) {
	(*ppfres)->u.type0.DescendantFont = DescendantFont;
	(*ppfres)->u.type0.CMapName = *CMapName;
	code = pdf_compute_BaseFont(pdev, *ppfres, false);
    }
    return code;    
}

/* ------ Type 3 ------ */

/* Allocate a Type 3 font resource for sinthesyzed bitmap fonts. */
int
pdf_font_type3_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
		     pdf_font_write_contents_proc_t write_contents)
{
    return font_resource_simple_alloc(pdev, ppfres, gs_no_id, ft_user_defined,
				      256, write_contents);
}

/* ------ Standard (base 14) Type 1 or TrueType ------ */

/* Allocate a standard (base 14) font resource. */
int
pdf_font_std_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
		   bool is_original, gs_id rid, gs_font_base *pfont, int index)
{
    pdf_font_resource_t *pdfont;
    int code = font_resource_encoded_alloc(pdev, &pdfont, rid, pfont->FontType,
					   pdf_write_contents_std);
    const pdf_standard_font_info_t *psfi = &standard_font_info[index];
    pdf_standard_font_t *psf = &pdf_standard_fonts(pdev)[index];
    gs_matrix *orig_matrix = (is_original ? &pfont->FontMatrix : &psf->orig_matrix);

    if (code < 0 ||
	(code = pdf_base_font_alloc(pdev, &pdfont->base_font, pfont, orig_matrix, true, true)) < 0
	)
	return code;
    pdfont->BaseFont.data = (byte *)psfi->fname; /* break const */
    pdfont->BaseFont.size = strlen(psfi->fname);
    set_is_MM_instance(pdfont, pfont);
    if (is_original) {
	psf->pdfont = pdfont;
	psf->orig_matrix = pfont->FontMatrix;
    }
    *ppfres = pdfont;
    return 0;
}

/* ------ Other Type 1 or TrueType ------ */

/* Allocate a Type 1 or TrueType font resource. */
int
pdf_font_simple_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
		      gs_id rid, pdf_font_descriptor_t *pfd)
{
    pdf_font_resource_t *pdfont;
    int code;

    code = font_resource_encoded_alloc(pdev, &pdfont, rid,
					   pdf_font_descriptor_FontType(pfd),
					   pdf_write_contents_simple);

    pdfont->FontDescriptor = pfd;
    set_is_MM_instance(pdfont, pdf_font_descriptor_font(pfd, false));
    *ppfres = pdfont;
    return pdf_compute_BaseFont(pdev, pdfont, false);
}

/* ------ CID-keyed ------ */

/* Allocate a CIDFont resource. */
int
pdf_font_cidfont_alloc(gx_device_pdf *pdev, pdf_font_resource_t **ppfres,
		       gs_id rid, pdf_font_descriptor_t *pfd)
{
    font_type FontType = pdf_font_descriptor_FontType(pfd);
    gs_font_base *font = pdf_font_descriptor_font(pfd, false);
    int chars_count;
    int code;
    pdf_font_write_contents_proc_t write_contents;
    const gs_cid_system_info_t *pcidsi;
    ushort *map = 0;
    pdf_font_resource_t *pdfont;

    switch (FontType) {
    case ft_CID_encrypted:
	chars_count = ((const gs_font_cid0 *)font)->cidata.common.CIDCount;
	pcidsi = &((const gs_font_cid0 *)font)->cidata.common.CIDSystemInfo;
	write_contents = pdf_write_contents_cid0;
	break;
    case ft_CID_TrueType:
	chars_count = ((const gs_font_cid2 *)font)->cidata.common.CIDCount;
	pcidsi = &((const gs_font_cid2 *)font)->cidata.common.CIDSystemInfo;
	map = (void *)gs_alloc_byte_array(pdev->pdf_memory, chars_count,
					  sizeof(*map), "CIDToGIDMap");
	if (map == 0)
	    return_error(gs_error_VMerror);
	memset(map, 0, chars_count * sizeof(*map));
	write_contents = pdf_write_contents_cid2;
	break;
    default:
	return_error(gs_error_rangecheck);
    }
    code = font_resource_alloc(pdev, &pdfont, resourceCIDFont, rid, FontType,
			       chars_count, write_contents);
    if (code < 0)
	return code;
    pdfont->FontDescriptor = pfd;
    pdfont->u.cidfont.CIDToGIDMap = map;
    /* fixme : Likely pdfont->u.cidfont.CIDToGIDMap duplicates 
       pdfont->FontDescriptor->base_font->copied->client_data->CIDMap.
       Only difference is 0xFFFF designates unmapped CIDs.
     */
    pdfont->u.cidfont.Widths2 = NULL;
    pdfont->u.cidfont.v = NULL;
    pdfont->u.cidfont.parent = NULL;
    /* Don' know whether the font will use WMode 1,
       so reserve it now. */
    pdfont->u.cidfont.used2 = gs_alloc_bytes(pdev->pdf_memory, 
		(chars_count + 7) / 8, "pdf_font_cidfont_alloc");
    if (pdfont->u.cidfont.used2 == NULL)
        return_error(gs_error_VMerror);
    memset(pdfont->u.cidfont.used2, 0, (chars_count + 7) / 8);
    /*
     * Write the CIDSystemInfo now, so we don't try to access it after
     * the font may no longer be available.
     */
    {
	long cidsi_id = pdf_begin_separate(pdev);

	code = pdf_write_cid_system_info(pdev, pcidsi, cidsi_id);
	if (code < 0)
	    return code;
	pdf_end_separate(pdev);
	pdfont->u.cidfont.CIDSystemInfo_id = cidsi_id;
    }
    *ppfres = pdfont;
    return pdf_compute_BaseFont(pdev, pdfont, false);
}

int
pdf_obtain_cidfont_widths_arrays(gx_device_pdf *pdev, pdf_font_resource_t *pdfont, 
                                 int wmode, double **w, double **w0, double **v)
{
    gs_memory_t *mem = pdev->pdf_memory;
    double *ww, *vv = 0, *ww0 = 0;
    int chars_count = pdfont->count;

    *w0 = (wmode ? pdfont->Widths : NULL);
    *v = (wmode ? pdfont->u.cidfont.v : NULL);
    *w = (wmode ? pdfont->u.cidfont.Widths2 : pdfont->Widths);
    if (*w == NULL) {
	ww = (double *)gs_alloc_byte_array(mem, chars_count, sizeof(*ww),
						    "pdf_obtain_cidfont_widths_arrays");
	if (wmode) {
	    vv = (double *)gs_alloc_byte_array(mem, chars_count, sizeof(*vv) * 2,
						    "pdf_obtain_cidfont_widths_arrays");
	    if (pdfont->Widths == 0) {
		ww0 = (double *)gs_alloc_byte_array(mem, chars_count, sizeof(*ww0),
						    "pdf_obtain_cidfont_widths_arrays");
		pdfont->Widths = *w0 = ww0;
		if (ww0 != 0)
		    memset(ww0, 0, chars_count * sizeof(*ww));
	    } else
		*w0 = ww0 = pdfont->Widths;
	}
	if (ww == 0 || (wmode && vv == 0) || (wmode && ww0 == 0)) {
	    gs_free_object(mem, ww, "pdf_obtain_cidfont_widths_arrays");
	    gs_free_object(mem, vv, "pdf_obtain_cidfont_widths_arrays");
	    gs_free_object(mem, ww0, "pdf_obtain_cidfont_widths_arrays");
	    return_error(gs_error_VMerror);
	}
	if (wmode)
	    memset(vv, 0, chars_count * 2 * sizeof(*vv));
	memset(ww, 0, chars_count * sizeof(*ww));
	if (wmode) {
	    pdfont->u.cidfont.Widths2 = *w = ww;	
	    pdfont->u.cidfont.v = *v = vv;	
	} else {
	    pdfont->Widths = *w = ww;
	    *v = NULL;
	}
    }
    return 0;
}

/* ---------------- CMap resources ---------------- */

/*
 * Allocate a CMap resource.
 */
int
pdf_cmap_alloc(gx_device_pdf *pdev, const gs_cmap_t *pcmap,
	       pdf_resource_t **ppres, int font_index_only)
{
    return pdf_write_cmap(pdev, pcmap, ppres, font_index_only);
}
