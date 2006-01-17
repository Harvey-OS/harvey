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

/* $Id: gdevpdtt.h,v 1.25 2004/10/15 18:24:31 igor Exp $ */
/* Internal text processing interface for pdfwrite */

#ifndef gdevpdtt_INCLUDED
#  define gdevpdtt_INCLUDED

/*
 * This file is only used internally to define the interface between
 * gdevpdtt.c, gdevpdtc.c, and gdevpdte.c.
 */

/* ---------------- Coordinate systems ---------------- */

/*

  The current text code has to deal with 6 different coordinate systems.
  This situation is complex, confusing, and fragile, but despite literally
  years of struggle we have been unable to understand how to simplify it.

  1) PostScript user space at the time a text operator is invoked.  The
     width values for ashow, xshow, etc. are specified in this space: these
     values appear in the delta_all, delta_space, x_widths, and y_widths
     members of the text member of the common part of the text enumerator
     (pdf_text_enum_t).

  2) Device space.  For the pdfwrite device, this is a straightforward space
     with (0,0) in the lower left corner.  The usual resolution is 720 dpi,
     but this is user-settable to provide a tradeoff between file size and
     bitmap resolution (for rendered Patterns and fonts that have to be
     converted to bitmaps).

  3) PDF user space.  During the processing of text operators, this is
     always the same as the default PDF user space, in which 1 unit = 1/72",
     because an Acrobat quirk apparently requires this.  (See stream_to_text
     in gdevpdfu.c.)

  4) Font design space.  This is the space in which the font's character
     outlines and advance widths are specified, including the width values
     returned by font->procs.glyph_info and by pdf_char_widths.

  5) PDF unscaled text space.  This space is used for the PDF width
     adjustment values (Tc, Tw, and TL parameters, and " operator) and
     positioning coordinates (Td and TD operators).

  6) PDF text space.  This space is used for the width adjustments for the
     TJ operator.

  The following convert between these spaces:

  - The PostScript CTM (pte->pis->ctm) maps #1 to #2.

  - The mapping from #3 to #2 is a scaling by pdev->HWResolution / 72.

  - The mapping from #5 to #6 is a scaling by the size member of
    pdf_text_state_values_t, which is the size value for the Tf operator.

  - The matrix member of pdf_text_state_values_t maps #5 to #2, which is the
    matrix for the Tm operator or its abbreviations.

  - The FontMatrix of a font maps #4 to #1.

  Note that the PDF text matrix (set by the Tm operator) maps #5 to #3.
  However, this is not actually stored anywhere: it is computed by
  multiplying the #5->#2 matrix by the #2->#3 scaling.

*/

/* ---------------- Types and structures ---------------- */

#ifndef pdf_char_glyph_pair_DEFINED
#  define pdf_char_glyph_pair_DEFINED
typedef struct pdf_char_glyph_pair_s pdf_char_glyph_pair_t;
#endif

#ifndef pdf_char_glyph_pairs_DEFINED
#  define pdf_char_glyph_pairs_DEFINED
typedef struct pdf_char_glyph_pairs_s pdf_char_glyph_pairs_t;
#endif

/* Define a structure for a text characters list. */
/* It must not contain pointers due to variable length. */
struct pdf_char_glyph_pairs_s {
    int num_all_chars;
    int num_unused_chars;
    int unused_offset;  /* The origin of the unused character table.*/
    pdf_char_glyph_pair_t s[1];  /* Variable length. */
};

/* Define the text enumerator. */
typedef struct pdf_text_enum_s {
    gs_text_enum_common;
    gs_text_enum_t *pte_default;
    gs_fixed_point origin;
    bool charproc_accum;
    bool cdevproc_callout;
    double cdevproc_result[10];
    pdf_char_glyph_pairs_t *cgp;
} pdf_text_enum_t;
#define private_st_pdf_text_enum()\
  extern_st(st_gs_text_enum);\
  gs_private_st_suffix_add2(st_pdf_text_enum, pdf_text_enum_t,\
    "pdf_text_enum_t", pdf_text_enum_enum_ptrs, pdf_text_enum_reloc_ptrs,\
    st_gs_text_enum, pte_default, cgp)

/*
 * Define quantities derived from the current font and CTM, used within
 * the text processing loop.  NOTE: This structure has no GC descriptor
 * and must therefore only be used locally (allocated on the C stack).
 */
typedef struct pdf_text_process_state_s {
    pdf_text_state_values_t values;
    gs_font *font;
} pdf_text_process_state_t;

/*
 * Define the structure used to return glyph width information.  Note that
 * there are two different sets of width information: real-number (x,y)
 * values, which give the true advance width, and an integer value, which
 * gives an X advance width for WMode = 0 or a Y advance width for WMode = 1.
 * The return value from pdf_glyph_width() indicates which of these is/are
 * valid.
 */
typedef struct pdf_glyph_width_s {
    double w;
    gs_point xy;
    gs_point v;				/* glyph origin shift */
} pdf_glyph_width_t;
typedef struct pdf_glyph_widths_s {
    pdf_glyph_width_t Width;		/* unmodified, for Widths */
    pdf_glyph_width_t real_width;	/* possibly modified, for rendering */
    bool replaced_v;
} pdf_glyph_widths_t;

/* ---------------- Procedures ---------------- */

/*
 * Declare the procedures for processing different species of text.
 * These procedures may, but need not, copy pte->text into buf
 * (a caller-supplied buffer large enough to hold the string).
 */
#define PROCESS_TEXT_PROC(proc)\
  int proc(gs_text_enum_t *pte, void *vbuf, uint bsize)

/* ------ gdevpdtt.c ------ */

/*
 * Compute and return the orig_matrix of a font.
 */
int pdf_font_orig_matrix(const gs_font *font, gs_matrix *pmat);
int font_orig_scale(const gs_font *font, double *sx);

/* 
 * Check the Encoding compatibility 
 */
bool pdf_check_encoding_compatibility(const pdf_font_resource_t *pdfont, 
		const pdf_char_glyph_pair_t *pairs, int num_chars);

/*
 * Create or find a font resource object for a text.
 */
int
pdf_obtain_font_resource(pdf_text_enum_t *penum, 
		const gs_string *pstr, pdf_font_resource_t **ppdfont);

/*
 * Create or find a font resource object for a glyphshow text.
 */
int pdf_obtain_font_resource_unencoded(pdf_text_enum_t *penum, 
	    const gs_string *pstr, pdf_font_resource_t **ppdfont, const gs_glyph *gdata);

/*
 * Create or find a CID font resource object for a glyph set.
 */
int pdf_obtain_cidfont_resource(gx_device_pdf *pdev, gs_font *subfont, 
			    pdf_font_resource_t **ppdsubf, 
			    pdf_char_glyph_pairs_t *cgp);

/*
 * Create or find a parent Type 0 font resource object for a CID font resource.
 */
int pdf_obtain_parent_type0_font_resource(gx_device_pdf *pdev, pdf_font_resource_t *pdsubf, 
		const gs_const_string *CMapName, pdf_font_resource_t **pdfont);

/*
 * Retrive font resource attached to a font.
 * allocating glyph_usage and real_widths on request.
 */
int pdf_attached_font_resource(gx_device_pdf *pdev, gs_font *font,  
			   pdf_font_resource_t **pdfont, byte **glyph_usage,
			   double **real_widths, int *num_chars, int *num_widths);

/*
 * Attach font resource to a font.
 */
int pdf_attach_font_resource(gx_device_pdf *pdev, gs_font *font, 
			 pdf_font_resource_t *pdfont); 

/*
 * Create a font resource object for a gs_font of Type 3.
 */
int pdf_make_font3_resource(gx_device_pdf *pdev, gs_font *font,
		       pdf_font_resource_t **ppdfont);

/*
 * Compute the cached values in the text processing state from the text
 * parameters, pdfont, and pis->ctm.  Return either an error code (< 0) or a
 * mask of operation attributes that the caller must emulate.  Currently the
 * only such attributes are TEXT_ADD_TO_ALL_WIDTHS and
 * TEXT_ADD_TO_SPACE_WIDTH.
 */
int pdf_update_text_state(pdf_text_process_state_t *ppts,
			  const pdf_text_enum_t *penum,
			  pdf_font_resource_t *pdfont,
			  const gs_matrix *pfmat);

/*
 * Set up commands to make the output state match the processing state.
 * General graphics state commands are written now; text state commands
 * are written later.
 */
int pdf_set_text_process_state(gx_device_pdf *pdev,
			       const gs_text_enum_t *pte, /*for pdcolor, pis*/
			       pdf_text_process_state_t *ppts);

/*
 * Get the widths (unmodified and possibly modified) of a glyph in a (base)
 * font.  If the width is cachable (implying that the w values area valid),
 * return 0; if only the xy values are valid, or the width is not cachable
 * for some other reason, return 1.
 * Return TEXT_PROCESS_CDEVPROC if a CDevProc callout is needed.
 * cdevproc_result != NULL if we restart after a CDevProc callout.
 */
int pdf_glyph_widths(pdf_font_resource_t *pdfont, int wmode, gs_glyph glyph,
		     gs_font *font, pdf_glyph_widths_t *pwidths,
		     const double cdevproc_result[10]);

/*
 * Fall back to the default text processing code when needed.
 */
int pdf_default_text_begin(gs_text_enum_t *pte, const gs_text_params_t *text,
			   gs_text_enum_t **ppte);

/*
 * Check for simple font.
 */
bool pdf_is_simple_font(gs_font *font);

/*
 * Check for CID font.
 */
bool pdf_is_CID_font(gs_font *font);

/* Get a synthesized Type 3 font scale. */
void pdf_font3_scale(gx_device_pdf *pdev, gs_font *font, double *scale);

/* Release a text characters colloction. */
void pdf_text_release_cgp(pdf_text_enum_t *penum);


/* ------ gdevpdtc.c ------ */

PROCESS_TEXT_PROC(process_composite_text);
PROCESS_TEXT_PROC(process_cmap_text);
PROCESS_TEXT_PROC(process_cid_text);

/* ------ gdevpdte.c ------ */

PROCESS_TEXT_PROC(process_plain_text);

/*
 * Encode and process a string with a simple gs_font.
 */
int pdf_encode_process_string(pdf_text_enum_t *penum, gs_string *pstr,
			      const gs_glyph *gdata, const gs_matrix *pfmat,
			      pdf_text_process_state_t *ppts);

/*
 * Emulate TEXT_ADD_TO_ALL_WIDTHS and/or TEXT_ADD_TO_SPACE_WIDTH,
 * and implement TEXT_REPLACE_WIDTHS if requested.
 * Uses and updates ppts->values.matrix; uses ppts->values.pdfont.
 */
int process_text_modify_width(pdf_text_enum_t *pte, gs_font *font,
			  pdf_text_process_state_t *ppts,
			  const gs_const_string *pstr,
			  gs_point *pdpt);

/* 
 * Add char code pair to ToUnicode CMap,
 * creating the CMap on neccessity.
 */
int
pdf_add_ToUnicode(gx_device_pdf *pdev, gs_font *font, pdf_font_resource_t *pdfont, 
		  gs_glyph glyph, gs_char ch);

/*
 * Get character code from a glyph code.
 * An usage of this function is very undesirable,
 * because a glyph may be unlisted in Encoding.
 */
int pdf_encode_glyph(gs_font_base *bfont, gs_glyph glyph0,
	    byte *buf, int buf_size, int *char_code_length);

int pdf_shift_text_currentpoint(pdf_text_enum_t *penum, gs_point *wpt);

#endif /* gdevpdtt_INCLUDED */
