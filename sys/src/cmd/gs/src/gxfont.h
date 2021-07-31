/* Copyright (C) 1989, 1995, 1996, 1997, 1999, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxfont.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Font object structure */
/* Requires gsmatrix.h, gxdevice.h */

#ifndef gxfont_INCLUDED
#  define gxfont_INCLUDED

#include "gsccode.h"
#include "gsfont.h"
#include "gsnotify.h"
#include "gsuid.h"
#include "gsstype.h"		/* for extern_st */
#include "gxftype.h"

/* A font object as seen by clients. */
/* See the PostScript Language Reference Manual for details. */

#ifndef gs_text_enum_DEFINED
#  define gs_text_enum_DEFINED
typedef struct gs_text_enum_s gs_text_enum_t;
#endif

#ifndef gx_path_DEFINED
#  define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif

/*
 * Define flags for font properties (Flags* members in the structure below.)
 *  Currently these must match the ones defined for PDF.
 */
#define FONT_IS_FIXED_WIDTH (1<<0)
/*
 * Define the structure used to return information about the font as a
 * whole.  Currently this structure almost exactly parallels the PDF
 * FontDescriptor.
 *
 * Unless otherwise specified, values are in the font's native character
 * space, scaled as requested by the call to the font_info procedure.
 * Note that Type 1 fonts typically have 1000 units per em, while
 * TrueType fonts typically have 1 unit per em.
 */
typedef struct gs_font_info_s {
    int members;

    /* The following members exactly parallel the PDF FontDescriptor flags. */

#define FONT_INFO_ASCENT 0x0001
    int Ascent;
#define FONT_INFO_AVG_WIDTH 0x0002
    int AvgWidth;
#define FONT_INFO_BBOX 0x0004
    gs_int_rect BBox;		/* computed from outlines, not FontBBox */
#define FONT_INFO_CAP_HEIGHT 0x0008
    int CapHeight;
#define FONT_INFO_DESCENT 0x0010
    int Descent;
#define FONT_INFO_FLAGS 0x0020
    uint Flags;			/* a mask of FONT_IS_ bits */
    uint Flags_requested;	/* client must set this if FLAGS requested */
    uint Flags_returned;
#define FONT_INFO_ITALIC_ANGLE 0x0100
    float ItalicAngle;		/* degrees CCW from vertical */
#define FONT_INFO_LEADING 0x0200
    int Leading;
#define FONT_INFO_MAX_WIDTH 0x0400
    int MaxWidth;
#define FONT_INFO_MISSING_WIDTH 0x0800
    int MissingWidth;
#define FONT_INFO_STEM_H 0x00010000
    int StemH;
#define FONT_INFO_STEM_V 0x00020000
    int StemV;
#define FONT_INFO_UNDERLINE_POSITION 0x00040000
    int UnderlinePosition;
#define FONT_INFO_UNDERLINE_THICKNESS 0x00080000
    int UnderlineThickness;
#define FONT_INFO_X_HEIGHT 0x00100000
    int XHeight;

    /* The following members do NOT appear in the PDF FontDescriptor. */

#define FONT_INFO_COPYRIGHT 0x0040
    gs_const_string Copyright;
#define FONT_INFO_NOTICE 0x0080
    gs_const_string Notice;
#define FONT_INFO_FAMILY_NAME 0x1000
    gs_const_string FamilyName;
#define FONT_INFO_FULL_NAME 0x2000
    gs_const_string FullName;

} gs_font_info_t;

/*
 * Define the structure used to return information about a glyph.
 */
typedef struct gs_glyph_info_s {
    int members;		/* mask of which members are valid */
#define GLYPH_INFO_WIDTH0 1
#define GLYPH_INFO_WIDTH GLYPH_INFO_WIDTH0
#define GLYPH_INFO_WIDTH1 2	/* must be WIDTH0 << 1 */
#define GLYPH_INFO_WIDTHS (GLYPH_INFO_WIDTH0 | GLYPH_INFO_WIDTH1)
    gs_point width[2];		/* width for WMode 0/1 */
#define GLYPH_INFO_BBOX 4
    gs_rect bbox;
#define GLYPH_INFO_NUM_PIECES 8
    int num_pieces;		/* # of pieces if composite, 0 if not */
#define GLYPH_INFO_PIECES 16
    gs_glyph *pieces;		/* pieces are stored here: the caller must */
				/* preset pieces if INFO_PIECES is set. */
} gs_glyph_info_t;

/* Define the "object" procedures of fonts. */

typedef struct gs_font_procs_s {

    /* ------ Font-level procedures ------ */

    /*
     * Define any special handling of gs_definefont.
     * We break this out so it can be different for composite fonts.
     */

#define font_proc_define_font(proc)\
  int proc(P2(gs_font_dir *, gs_font *))
    font_proc_define_font((*define_font));

    /*
     * Define any special handling of gs_makefont.
     * We break this out so it can be different for composite fonts.
     */

#define font_proc_make_font(proc)\
  int proc(P4(gs_font_dir *, const gs_font *, const gs_matrix *, gs_font **))
    font_proc_make_font((*make_font));

    /*
     * Get information about the font, as specified by the members mask.
     * Disregard the FontMatrix: scale the font as indicated by *pscale
     * (pscale=NULL means no scaling).  Set info->members to the members
     * that are actually returned.  Note that some member options require
     * the caller to preset some of the elements of info.  Note also that
     * this procedure may return more information than was requested.
     */

#define font_proc_font_info(proc)\
  int proc(P4(gs_font *font, const gs_point *pscale, int members,\
	      gs_font_info_t *info))
    font_proc_font_info((*font_info));

    /*
     * Determine whether this font is the "same as" another font in the ways
     * specified by the mask.  The returned value is a subset of the mask.
     * This procedure is allowed to be conservative (return false in cases
     * of uncertainty).  Note that this procedure does not test the UniqueID
     * or FontMatrix.
     */

#define FONT_SAME_OUTLINES 1
#define FONT_SAME_METRICS 2
#define FONT_SAME_ENCODING 4

#define font_proc_same_font(proc)\
  int proc(P3(const gs_font *font, const gs_font *ofont, int mask))
    font_proc_same_font((*same_font));

    /* ------ Glyph-level procedures ------ */

    /* Map a character code to a glyph.  Some PostScript fonts use both
     * names and indices to identify glyphs: for example, in PostScript Type
     * 42 fonts, the Encoding array maps character codes to glyph names, and
     * the CharStrings dictionary maps glyph names to glyph indices.  In
     * such fonts, the glyph_space argument determines which type of glyph
     * is returned.
     */

#define font_proc_encode_char(proc)\
  gs_glyph proc(P3(gs_font *, gs_char, gs_glyph_space_t))
    font_proc_encode_char((*encode_char));

    /*
     * Get the next glyph in an enumeration of all glyphs defined by the
     * font.  index = 0 means return the first one; a returned index of 0
     * means the enumeration is finished.  The glyphs are enumerated in
     * an unpredictable order.
     */

#define font_proc_enumerate_glyph(proc)\
  int proc(P4(gs_font *font, int *pindex, gs_glyph_space_t glyph_space,\
	      gs_glyph *pglyph))
    font_proc_enumerate_glyph((*enumerate_glyph));

    /*
     * Get information about a glyph, as specified by the members mask.
     * pmat = NULL means get the scalable values; non-NULL pmat means get
     * the scaled values.  Set info->members to the members that are
     * actually returned.  Return gs_error_undefined if the glyph doesn't
     * exist in the font; calling glyph_info with members = 0 is an
     * efficient way to find out whether a given glyph exists.  Note that
     * some member options require the caller to preset some of the elements
     * of info.  Note also that this procedure may return more information
     * than was requested.
     */

#define font_proc_glyph_info(proc)\
  int proc(P5(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,\
	      int members, gs_glyph_info_t *info))
    font_proc_glyph_info((*glyph_info));

    /*
     * Append the outline for a glyph to a path, with the glyph origin
     * at the current point.  pmat is as for glyph_width.  The outline
     * does include a final moveto for the advance width.
     */

#define font_proc_glyph_outline(proc)\
  int proc(P4(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,\
	      gx_path *ppath))
    font_proc_glyph_outline((*glyph_outline));

    /* ------ Glyph rendering procedures ------ */

    /*
     * Define any needed procedure for initializing the composite
     * font stack in a show enumerator.  This is a no-op for
     * all but composite fonts.
     */

#define font_proc_init_fstack(proc)\
  int proc(P2(gs_text_enum_t *, gs_font *))
    font_proc_init_fstack((*init_fstack));

    /*
     * Define the font's algorithm for getting the next character or glyph
     * from a string being shown.  This is trivial, except for composite
     * fonts.  Returns 0 if the current (base) font didn't change, 1 if it
     * did change, 2 if there are no more characters, or an error code.
     *
     * This procedure may set either *pchar to gs_no_char or *pglyph to
     * gs_no_glyph, but not both.
     */

#define font_proc_next_char_glyph(proc)\
  int proc(P3(gs_text_enum_t *pte, gs_char *pchar, gs_glyph *pglyph))
    font_proc_next_char_glyph((*next_char_glyph));

    /*
     * Define a client-supplied BuildChar/BuildGlyph procedure.
     * The gs_char may be gs_no_char (for BuildGlyph), or the gs_glyph
     * may be gs_no_glyph (for BuildChar), but not both.  Return 0 for
     * success, 1 if the procedure was unable to render the character
     * (but no error occurred), <0 for error.
     */

#define font_proc_build_char(proc)\
  int proc(P5(gs_text_enum_t *, gs_state *, gs_font *, gs_char, gs_glyph))
    font_proc_build_char((*build_char));

    /* Callback procedures for external font rasterizers */
    /* (see gsccode.h for details.) */

    gx_xfont_callbacks callbacks;

} gs_font_procs;

/* Default font-level font procedures in gsfont.c */
font_proc_define_font(gs_no_define_font);
font_proc_make_font(gs_no_make_font);
font_proc_make_font(gs_base_make_font);
font_proc_font_info(gs_default_font_info);
font_proc_same_font(gs_default_same_font);
font_proc_same_font(gs_base_same_font);
/* Default glyph-level font procedures in gsfont.c */
font_proc_encode_char(gs_no_encode_char);
font_proc_enumerate_glyph(gs_no_enumerate_glyph);
font_proc_glyph_info(gs_default_glyph_info);
font_proc_glyph_outline(gs_no_glyph_outline);
/* Default glyph rendering procedures in gstext.c */
font_proc_init_fstack(gs_default_init_fstack);
font_proc_next_char_glyph(gs_default_next_char_glyph);
font_proc_build_char(gs_no_build_char);
/* Default procedure vector in gsfont.c */
extern const gs_font_procs gs_font_procs_default;

/* The font names are only needed for xfont lookup and high-level output. */
typedef struct gs_font_name_s {
#define gs_font_name_max 47	/* must be >= 40 */
    /* The +1 is so we can null-terminate for debugging printout. */
    byte chars[gs_font_name_max + 1];
    uint size;
} gs_font_name;

/*
 * Define a generic font.  We include PaintType and StrokeWidth here because
 * they affect rendering algorithms outside the Type 1 font machinery.
 *
 * ****** NOTE: If you define any subclasses of gs_font, you *must* define
 * ****** the finalization procedure as gs_font_finalize.  Finalization
 * ****** procedures are not automatically inherited.
 */
#define gs_font_common\
	gs_font *next, *prev;		/* chain for original font list or */\
					/* scaled font cache */\
	gs_memory_t *memory;		/* allocator for this font */\
	gs_font_dir *dir;		/* directory where registered */\
	bool is_resource;\
	gs_notify_list_t notify_list;	/* clients to notify when freeing */\
	gs_id id;			/* internal ID (no relation to UID) */\
	gs_font *base;			/* original (unscaled) base font */\
	void *client_data;		/* additional client data */\
	gs_matrix FontMatrix;\
	font_type FontType;\
	bool BitmapWidths;\
	fbit_type ExactSize, InBetweenSize, TransformedChar;\
	int WMode;			/* 0 or 1 */\
	int PaintType;			/* PaintType for Type 1/4/42 fonts, */\
					/* 0 for others */\
	float StrokeWidth;		/* StrokeWidth for Type 1/4/42 */\
					/* fonts (if present), 0 for others */\
	gs_font_procs procs;\
	/* We store both the FontDirectory key (key_name) and, */\
	/* if present, the FontName (font_name). */\
	gs_font_name key_name, font_name
/*typedef struct gs_font_s gs_font; *//* in gsfont.h and other places */
struct gs_font_s {
    gs_font_common;
};

extern_st(st_gs_font);		/* (abstract) */
struct_proc_finalize(gs_font_finalize);  /* public for concrete subclasses */
#define public_st_gs_font()	/* in gsfont.c */\
  gs_public_st_complex_only(st_gs_font, gs_font, "gs_font",\
    0, font_enum_ptrs, font_reloc_ptrs, gs_font_finalize)
#define st_gs_font_max_ptrs (st_gs_notify_list_max_ptrs + 5)

#define private_st_gs_font_ptr()	/* in gsfont.c */\
  gs_private_st_ptr(st_gs_font_ptr, gs_font *, "gs_font *",\
    font_ptr_enum_ptrs, font_ptr_reloc_ptrs)
#define st_gs_font_ptr_max_ptrs 1

extern_st(st_gs_font_ptr_element);
#define public_st_gs_font_ptr_element()	/* in gsfont.c */\
  gs_public_st_element(st_gs_font_ptr_element, gs_font *, "gs_font *[]",\
    font_ptr_element_enum_ptrs, font_ptr_element_reloc_ptrs, st_gs_font_ptr)

/* Allocate and minimally initialize a font. */
/* Does not set: FontMatrix, FontType, key_name, font_name. */
gs_font *
  gs_font_alloc(P5(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
		   const gs_font_procs *procs, gs_font_dir *dir,
		   client_name_t cname));

/*
 * Register/unregister a client for notification by a font.  Currently
 * the clients are only notified when a font is freed.  Note that any
 * such client must unregister itself when *it* is freed.
 */
int gs_font_notify_register(P3(gs_font *font, gs_notify_proc_t proc,
			       void *proc_data));
int gs_font_notify_unregister(P3(gs_font *font, gs_notify_proc_t proc,
				 void *proc_data));

/* Define a base (not composite) font. */
#define gs_font_base_common\
	gs_font_common;\
	gs_rect FontBBox;\
	gs_uid UID;\
	gs_encoding_index_t encoding_index;\
	gs_encoding_index_t nearest_encoding_index  /* (may be >= 0 even if */\
						/* encoding_index = -1) */
#ifndef gs_font_base_DEFINED
#  define gs_font_base_DEFINED
typedef struct gs_font_base_s gs_font_base;
#endif
struct gs_font_base_s {
    gs_font_base_common;
};

extern_st(st_gs_font_base);
#define public_st_gs_font_base()	/* in gsfont.c */\
  gs_public_st_suffix_add1_final(st_gs_font_base, gs_font_base,\
    "gs_font_base", font_base_enum_ptrs, font_base_reloc_ptrs,\
    gs_font_finalize, st_gs_font, UID.xvalues)
#define st_gs_font_base_max_ptrs (st_gs_font_max_ptrs + 1)

/* Allocate and minimally initialize a base font. */
/* Does not set: same elements as gs_alloc_font. */
gs_font_base *
  gs_font_base_alloc(P5(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
			const gs_font_procs *procs, gs_font_dir *dir,
			client_name_t cname));

#endif /* gxfont_INCLUDED */
