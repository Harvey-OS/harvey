/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdff.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Font-related definitions for PDF-writing driver. */

#ifndef gdevpdff_INCLUDED
#  define gdevpdff_INCLUDED

/* ================ Types and structures ================ */

/*
 * The PDF writer creates several different kinds of font resources.
 * The key differences between them are the values of num_chars, index,
 * and descriptor.
 *
 *	- Synthesized Type 3 bitmap fonts are identified by num_chars != 0 (or
 *	equivalently PDF_FONT_IS_SYNTHESIZED = true).  They have index < 0,
 *	FontDescriptor == 0.  All other fonts have num_chars == 0 and
 *	(currently) FontDescriptor != 0.
 *
 *	- The base 14 fonts (when not embedded) have num_chars == 0, index
 *	>= 0, FontDescriptor != 0, FontDescriptor->base_font == 0.  All
 *	other fonts have index < 0.
 *
 *	- All other fonts (Type 1 or TrueType, embedded or not) have
 *	num_chars == 0, index < 0, FontDescriptor != 0,
 *	FontDescriptor->base_font != 0.  A font is embedded iff
 *	FontDescriptor->FontFile_id != 0.
 *
 * For non-synthesized fonts, the structure representation is designed to
 * represent directly the information that will be written in the font
 * resource, Encoding, and FontDescriptor dictionaries.  See the comments
 * on the pdf_font_t structure below for more detail.
 */

/*
 * A FontDescriptor refers to an unscaled, possibly built-in base_font.
 * Multiple pdf_fonts with the same outlines (but possibly different
 * encodings, metrics, and scaling) may share a single FontDescriptor.
 * Each such pdf_font refers to a corresponding gs_font object, and
 * the gs_fonts of all such pdf_fonts will have the base_font of the
 * FontDescriptor as their eventual base font (through a chain of 0 or
 * more base pointers).
 *
 * Since gs_font objects may be freed at any time, we register a procedure
 * to be called when that happens.  The (gs_)font of a pdf_font may be freed
 * either before or after the base_font in the FontDescriptor.  Therefore, a
 * pdf_font copies enough information from its gs_font that when the gs_font
 * is freed, the pdf_font still has enough information to write the Font
 * resource dictionary at a later time.  When a gs_font referenced from a
 * pdf_font, we only clear the pointer to it from its referencing pdf_font.
 * However, when the base_font of a FontDescriptor is about to be freed, we
 * must write the FontDescriptor, and, if relevant, the FontFile data.
 */

/*
 * PDF font names must be large enough for the 14 built-in fonts,
 * and also large enough for any reasonable font name + 7 characters
 * for the subsetting prefix + a suffix derived from the PDF object number.
 */
#define SUBSET_PREFIX_SIZE 7
#define MAX_PDF_FONT_NAME\
  (SUBSET_PREFIX_SIZE + gs_font_name_max + 1 + 1 + sizeof(long) * 2)
typedef struct pdf_font_name_s {
    byte chars[MAX_PDF_FONT_NAME];
    uint size;
} pdf_font_name_t;

/*
 * Font descriptors (handled as pseudo-resources).  Multiple pdf_fonts
 * may share a descriptor.  We don't need to use reference counting to
 * keep track of this, since all descriptors persist until the device
 * is closed, even though the base_font they reference may have been
 * freed.
 */
typedef struct pdf_font_descriptor_values_s {
    font_type FontType;		/* copied from font */
    /* Required elements */
    int Ascent, CapHeight, Descent, ItalicAngle, StemV;
    gs_int_rect FontBBox;
    uint Flags;
    /* Optional elements (default to 0) */
    int AvgWidth, Leading, MaxWidth, MissingWidth, StemH, XHeight;
} pdf_font_descriptor_values_t;
typedef struct pdf_font_descriptor_s pdf_font_descriptor_t;
struct pdf_font_descriptor_s {
    pdf_resource_common(pdf_font_descriptor_t);
    pdf_font_name_t FontName;
    pdf_font_descriptor_values_t values;
    gs_matrix orig_matrix;	/* unscaled font matrix */
    byte chars_used[32];	/* 1 bit per character code */
    bool subset_ok;		/* if false, don't subset the font -- */
				/* see gdevpdft.c */
    long FontFile_id;		/* non-0 iff the font is embedded */
    gs_font *base_font;		/* unscaled font defining the base encoding, */
				/* matrix, and character set, 0 iff */
				/* non-embedded standard font */
    bool notified;		/* if true, base_font will notify on freeing */
    bool written;		/* if true, descriptor has been written out */
};
/* Flag bits */
/*#define FONT_IS_FIXED_WIDTH (1<<0)*/  /* define in gxfont.h */
#define FONT_IS_SERIF (1<<1)
#define FONT_IS_SYMBOLIC (1<<2)
#define FONT_IS_SCRIPT (1<<3)
/*
 * There is confusion over the meaning of the 1<<5 bit.  According to the
 * published PDF documentation, in PDF 1.1, it meant "font uses
 * StandardEncoding", and as of PDF 1.2, it means "font uses (a subset of)
 * the Adobe standard Latin character set"; however, Acrobat Reader 3 and 4
 * seem to use the former interpretation, even if the font is embedded and
 * the file is identified as a PDF 1.2 file.  We have to use the former
 * interpretation in order to produce output that Acrobat will handle
 * correctly.
 */
#define FONT_USES_STANDARD_ENCODING (1<<5) /* always used */
#define FONT_IS_ADOBE_ROMAN (1<<5) /* never used */
#define FONT_IS_ITALIC (1<<6)
#define FONT_IS_ALL_CAPS (1<<16)
#define FONT_IS_SMALL_CAPS (1<<17)
#define FONT_IS_FORCE_BOLD (1<<18)
/*
 * Font descriptors are pseudo-resources, so their GC descriptors
 * must be public.
 */
#define public_st_pdf_font_descriptor()\
  gs_public_st_suffix_add1(st_pdf_font_descriptor, pdf_font_descriptor_t,\
    "pdf_font_descriptor_t", pdf_font_desc_enum_ptrs,\
    pdf_font_desc_reloc_ptrs, st_pdf_resource, base_font)

/* Font resources */
typedef struct pdf_char_proc_s pdf_char_proc_t;	/* forward reference */
/*typedef struct pdf_font_s pdf_font_t;*/
typedef struct pdf_encoding_element_s {
    gs_glyph glyph;
    gs_const_string str;
} pdf_encoding_element_t;
#define private_st_pdf_encoding_element()\
  gs_private_st_composite(st_pdf_encoding_element, pdf_encoding_element_t,\
    "pdf_encoding_element_t[]", pdf_encoding_elt_enum_ptrs,\
    pdf_encoding_elt_reloc_ptrs)
/*
 * Structure elements beginning with a capital latter correspond directly
 * to keys in the font or Encoding dictionary.  Currently these are:
 *	(font) FontType, FirstChar, LastChar, Widths, FontDescriptor
 *	(Encoding) BaseEncoding, Differences
 */
struct pdf_font_s {
    pdf_resource_common(pdf_font_t);
    pdf_font_name_t fname;
    font_type FontType;
    gs_font *font;		/* non-0 iff font will notify us; */
				/* should be a weak pointer */
    int index;			/* in pdf_standard_fonts, -1 if not base 14 */
    gs_matrix orig_matrix;	/* FontMatrix of unscaled font for embedding */
    bool is_MM_instance;	/* for Type 1/2 fonts, true iff the font */
				/* is a Multiple Master instance */
    /*
     * For synthesized fonts, frname is A, B, ...; for other fonts,
     * frname is R<id>.  The string is null-terminated.
     */
    char frname[1/*R*/ + (sizeof(long) * 8 / 3 + 1) + 1/*\0*/];
    /* Encoding for base fonts. */
    gs_encoding_index_t BaseEncoding;
    pdf_encoding_element_t *Differences; /* [256] */
    /* Bookkeeping for non-synthesized fonts. */
    pdf_font_descriptor_t *FontDescriptor;
    int FirstChar, LastChar;
    bool write_Widths;
    int Widths[256];
    byte widths_known[32];	/* 1 bit per character code */
    bool skip;			/* font was already written, skip it */
    /* Bookkeeping for synthesized fonts. */
    int num_chars;
#define PDF_FONT_IS_SYNTHESIZED(pdfont) ((pdfont)->num_chars != 0)
    pdf_char_proc_t *char_procs;
    int max_y_offset;
    /* Pseudo-characters for spacing. */
    /* The range should be determined by the device resolution.... */
/*#define X_SPACE_MIN xxx*/ /* in gdevpdfx.h */
/*#define X_SPACE_MAX nnn*/ /* in gdevpdfx.h */
    byte spaces[X_SPACE_MAX - X_SPACE_MIN + 1];
};

/* The descriptor is public for pdf_resource_type_structs. */
#define public_st_pdf_font()\
  gs_public_st_suffix_add4(st_pdf_font, pdf_font_t, "pdf_font_t",\
    pdf_font_enum_ptrs, pdf_font_reloc_ptrs, st_pdf_resource,\
    font, Differences, FontDescriptor, char_procs)

/* CharProc pseudo-resources for synthesized fonts */
struct pdf_char_proc_s {
    pdf_resource_common(pdf_char_proc_t);
    pdf_font_t *font;
    pdf_char_proc_t *char_next;	/* next char_proc for same font */
    int width, height;
    int x_width;		/* X escapement */
    int y_offset;		/* of character (0,0) */
    byte char_code;
};

/* The descriptor is public for pdf_resource_type_structs. */
#define public_st_pdf_char_proc()\
  gs_public_st_suffix_add2(st_pdf_char_proc, pdf_char_proc_t,\
    "pdf_char_proc_t", pdf_char_proc_enum_ptrs,\
    pdf_char_proc_reloc_ptrs, st_pdf_resource, font, char_next)

/* ================ Procedures ================ */

/* ---------------- Exported by gdevpdft.c ---------------- */

/* Begin a CharProc for an embedded (bitmap) font. */
int pdf_begin_char_proc(P8(gx_device_pdf * pdev, int w, int h, int x_width,
			   int y_offset, gs_id id, pdf_char_proc_t **ppcp,
			   pdf_stream_position_t * ppos));

/* End a CharProc. */
int pdf_end_char_proc(P2(gx_device_pdf * pdev, pdf_stream_position_t * ppos));

/* Put out a reference to an image as a character in an embedded font. */
int pdf_do_char_image(P3(gx_device_pdf * pdev, const pdf_char_proc_t * pcp,
			 const gs_matrix * pimat));

/* ---------------- Exported by gdevpdff.c ---------------- */

typedef enum {
    FONT_EMBED_STANDARD,	/* 14 standard fonts */
    FONT_EMBED_NO,
    FONT_EMBED_YES
} pdf_font_embed_t;

typedef struct pdf_standard_font_s {
    const char *fname;
    gs_encoding_index_t base_encoding;
} pdf_standard_font_t;
extern const pdf_standard_font_t pdf_standard_fonts[];

/* Return the index of a standard font name, or -1 if missing. */
int pdf_find_standard_font(P2(const byte *str, uint size));

/*
 * Find the original (unscaled) standard font corresponding to an
 * arbitrary font, if any.  Return its index in standard_fonts, or -1.
 */
int pdf_find_orig_font(P3(gx_device_pdf *pdev, gs_font *font,
			  gs_matrix *pfmat));

/*
 * Determine the embedding status of a font.  If the font is in the base
 * 14, store its index (0..13) in *pindex and its similarity to the base
 * font (as determined by the font's same_font procedure) in *psame.
 */
pdf_font_embed_t pdf_font_embed_status(P4(gx_device_pdf *pdev, gs_font *font,
					  int *pindex, int *psame));

/*
 * Allocate a font resource.  If descriptor_id is gs_no_id, no
 * FontDescriptor is allocated.
 */
int pdf_alloc_font(P4(gx_device_pdf *pdev, gs_id rid, pdf_font_t **ppfres,
		      gs_id descriptor_id));

/*
 * Determine whether a font is a subset font by examining the name.
 */
bool pdf_has_subset_prefix(P2(const byte *str, uint size));

/*
 * Make the prefix for a subset font from the font's resource ID.
 */
void pdf_make_subset_prefix(P2(byte *str, ulong id));

/*
 * Adjust the FontName of a newly created FontDescriptor so that it is
 * unique if necessary.  If the name was changed, return 1.
 */
int pdf_adjust_font_name(P3(const gx_device_pdf *pdev,
			    pdf_font_descriptor_t *pfd,
			    bool is_standard));

/* Add an encoding difference to a font. */
int pdf_add_encoding_difference(P5(gx_device_pdf *pdev, pdf_font_t *ppf,
				   int chr, const gs_font_base *bfont,
				   gs_glyph glyph));

/* Get the width of a given character in a (base) font. */
int pdf_char_width(P4(pdf_font_t *ppf, int ch, gs_font *font,
		      int *pwidth /* may be NULL */));

/*
 * Find the range of character codes that includes all the defined
 * characters in a font.
 */
void pdf_find_char_range(P3(gs_font *font, int *pfirst, int *plast));

/* Compute the FontDescriptor for a font or a font subset. */
int pdf_compute_font_descriptor(P4(gx_device_pdf *pdev,
				   pdf_font_descriptor_t *pfd, gs_font *font,
				   const byte *used /*[32]*/));

/* Unregister the standard fonts when cleaning up. */
void pdf_unregister_fonts(P1(gx_device_pdf *pdev));

/* ---------------- Exported by gdevpdfw.c ---------------- */

/* Register a font for eventual writing (embedded or not). */
int pdf_register_font(P3(gx_device_pdf *pdev, gs_font *font, pdf_font_t *ppf));

/* Write out the font resources when wrapping up the output. */
int pdf_write_font_resources(P1(gx_device_pdf *pdev));

#endif /* gdevpdff_INCLUDED */
