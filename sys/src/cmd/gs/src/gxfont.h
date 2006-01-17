/* Copyright (C) 1989, 1995, 1996, 1997, 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfont.h,v 1.23 2004/10/13 15:31:58 igor Exp $ */
/* Font object structure */
/* Requires gsmatrix.h, gxdevice.h */

#ifndef gxfont_INCLUDED
#  define gxfont_INCLUDED

#include "gsccode.h"
#include "gsfont.h"
#include "gsgdata.h"
#include "gsmatrix.h"
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

#define public_st_gs_font_info() /* in gsfont.c */\
  BASIC_PTRS(gs_font_info_ptrs) {\
    GC_CONST_STRING_ELT(gs_font_info_t, Copyright),\
    GC_CONST_STRING_ELT(gs_font_info_t, Notice),\
    GC_CONST_STRING_ELT(gs_font_info_t, FamilyName),\
    GC_CONST_STRING_ELT(gs_font_info_t, FullName)\
  };\
  gs_public_st_basic(st_gs_font_info, gs_font_info_t, "gs_font_info_t",\
		     gs_font_info_ptrs, gs_font_info_data)

/*
 * Define the structure used to return information about a glyph.
 *
 * Note that a glyph may have two different sets of widths: those stored in
 * the outline (which includes hmtx for TrueType fonts), and those actually
 * used when rendering the glyph.  Currently, these differ only for Type 1
 * fonts that use Metrics or CDevProc to change the widths.  The glyph_info
 * procedure normally returns the rendering widths: to get the outline
 * widths, clients use GLYPH_INFO_OUTLINE_WIDTHS.  The glyph_info procedure
 * should set GLYPH_INFO_OUTLINE_WIDTHS in the response (the `members' in
 * the gs_glyph_info_t structure) iff it was set in the request *and* the
 * glyph actually has two different sets of widths.  With this arrangement,
 * fonts that don't distinguish the two sets of widths don't need to do
 * anything special, and don't need to test for GLYPH_INFO_OUTLINE_WIDTHS.
 */
typedef struct gs_glyph_info_s {
    int members;		/* mask of which members are valid */
#define GLYPH_INFO_WIDTH0 1
#define GLYPH_INFO_WIDTH GLYPH_INFO_WIDTH0
#define GLYPH_INFO_WIDTH1 2	/* must be WIDTH0 << 1 */
#define GLYPH_INFO_WIDTHS (GLYPH_INFO_WIDTH0 | GLYPH_INFO_WIDTH1)
    gs_point width[2];		/* width for WMode 0/1 */
    gs_point v;			/* glyph origin for WMode 1 */
#define GLYPH_INFO_BBOX 4
    gs_rect bbox;
#define GLYPH_INFO_NUM_PIECES 8
    int num_pieces;		/* # of pieces if composite, 0 if not */
#define GLYPH_INFO_PIECES 16
    gs_glyph *pieces;		/* pieces are stored here: the caller must */
				/* preset pieces if INFO_PIECES is set. */
#define GLYPH_INFO_OUTLINE_WIDTHS 32 /* return unmodified widths, see above */
#define GLYPH_INFO_VVECTOR0 64	
#define GLYPH_INFO_VVECTOR1 128	/* must be VVECTOR0 << 1 */
#define GLYPH_INFO_CDEVPROC 256	/* Allow CDevProc callout. */
} gs_glyph_info_t;

/* Define the "object" procedures of fonts. */

typedef struct gs_font_procs_s {

    /* ------ Font-level procedures ------ */

    /*
     * Define any special handling of gs_definefont.
     * We break this out so it can be different for composite fonts.
     */

#define font_proc_define_font(proc)\
  int proc(gs_font_dir *, gs_font *)
    font_proc_define_font((*define_font));

    /*
     * Define any special handling of gs_makefont.
     * We break this out so it can be different for composite fonts.
     */

#define font_proc_make_font(proc)\
  int proc(gs_font_dir *, const gs_font *, const gs_matrix *, gs_font **)
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
  int proc(gs_font *font, const gs_point *pscale, int members,\
	   gs_font_info_t *info)
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
  int proc(const gs_font *font, const gs_font *ofont, int mask)
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
  gs_glyph proc(gs_font *, gs_char, gs_glyph_space_t)
    font_proc_encode_char((*encode_char));

    /* Map a glyph name to Unicode UTF-16.
     */

#define font_proc_decode_glyph(proc)\
  gs_char proc(gs_font *, gs_glyph)
    font_proc_decode_glyph((*decode_glyph));

    /*
     * Get the next glyph in an enumeration of all glyphs defined by the
     * font.  index = 0 means return the first one; a returned index of 0
     * means the enumeration is finished.  The glyphs are enumerated in
     * an unpredictable order.
     */

#define font_proc_enumerate_glyph(proc)\
  int proc(gs_font *font, int *pindex, gs_glyph_space_t glyph_space,\
	   gs_glyph *pglyph)
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
     *
     * Implementations of this method must not access font->WMode,
     * because for font descendents it is inherited from an upper font.
     * Implementatios must derive WMode from requested flags specified
     * in 'members' argument.
     *
     * Currently we do not handle requests, in which GLYPH_INFO_VVECTOR0
     * is set, but GLYPH_INFO_WIDTH0 is not. Same for GLYPH_INFO_VVECTOR1
     * and GLYPH_INFO_WIDTH1. Also requests, in which both GLYPH_INFO_WIDTH0 and
     * GLYPH_INFO_WIDTH1 are set, may work wrongly. Such requests look never used 
     * and debugged, and the implementation code requires improvements.
     */

#define font_proc_glyph_info(proc)\
  int proc(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,\
	   int members, gs_glyph_info_t *info)
    font_proc_glyph_info((*glyph_info));

    /*
     * Append the outline for a glyph to a path, with the glyph origin
     * at the current point.  pmat is as for glyph_width.  The outline
     * does include a final moveto for the advance width.
     *
     * Implementations of this method must not access font->WMode,
     * because for font descendents it is inherited from an upper font.
     * This is especially important for Type 42 fonts with hmtx and vmtx.
     */
    /* Currently glyph_outline retrieves sbw only with type 1,2,9 fonts. */

#define font_proc_glyph_outline(proc)\
  int proc(gs_font *font, int WMode, gs_glyph glyph, const gs_matrix *pmat,\
	   gx_path *ppath, double sbw[4])
    font_proc_glyph_outline((*glyph_outline));

    /*
     * Return the (string) name of a glyph.
     */

#define font_proc_glyph_name(proc)\
  int proc(gs_font *font, gs_glyph glyph, gs_const_string *pstr)
    font_proc_glyph_name((*glyph_name));

    /* ------ Glyph rendering procedures ------ */

    /*
     * Define any needed procedure for initializing the composite
     * font stack in a show enumerator.  This is a no-op for
     * all but composite fonts.
     */

#define font_proc_init_fstack(proc)\
  int proc(gs_text_enum_t *, gs_font *)
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
  int proc(gs_text_enum_t *pte, gs_char *pchar, gs_glyph *pglyph)
    font_proc_next_char_glyph((*next_char_glyph));

    /*
     * Define a client-supplied BuildChar/BuildGlyph procedure.
     * The gs_char may be gs_no_char (for BuildGlyph), or the gs_glyph
     * may be gs_no_glyph (for BuildChar), but not both.  Return 0 for
     * success, 1 if the procedure was unable to render the character
     * (but no error occurred), <0 for error.
     */

#define font_proc_build_char(proc)\
  int proc(gs_text_enum_t *, gs_state *, gs_font *, gs_char, gs_glyph)
    font_proc_build_char((*build_char));

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
font_proc_decode_glyph(gs_no_decode_glyph);
font_proc_enumerate_glyph(gs_no_enumerate_glyph);
font_proc_glyph_info(gs_default_glyph_info);
font_proc_glyph_outline(gs_no_glyph_outline);
font_proc_glyph_name(gs_no_glyph_name);
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
	gs_matrix orig_FontMatrix;      /* The original font matrix or zeros. */\
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
  gs_font_alloc(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
		const gs_font_procs *procs, gs_font_dir *dir,
		client_name_t cname);

/* Initialize the notification list for a font. */
void gs_font_notify_init(gs_font *font);

/*
 * Register/unregister a client for notification by a font.  Currently
 * the clients are only notified when a font is freed.  Note that any
 * such client must unregister itself when *it* is freed.
 */
int gs_font_notify_register(gs_font *font, gs_notify_proc_t proc,
			    void *proc_data);
int gs_font_notify_unregister(gs_font *font, gs_notify_proc_t proc,
			      void *proc_data);

#ifndef FAPI_server_DEFINED
#define FAPI_server_DEFINED
typedef struct FAPI_server_s FAPI_server;
#endif

/* Define a base (not composite) font. */
#define gs_font_base_common\
	gs_font_common;\
	gs_rect FontBBox;\
	gs_uid UID;\
	FAPI_server *FAPI; \
        void *FAPI_font_data; \
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
  gs_font_base_alloc(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
		     const gs_font_procs *procs, gs_font_dir *dir,
		     client_name_t cname);

/* Define a string to interact with unique_name in lib/pdf_font.ps .
   The string is used to resolve glyph name conflict while
   converting PDF Widths into Metrics.
 */
extern const char gx_extendeg_glyph_name_separator[];

/*
 * Test whether a glyph is the notdef glyph for a base font.
 * The test is somewhat adhoc: perhaps this should be a virtual procedure.
 */
bool gs_font_glyph_is_notdef(gs_font_base *bfont, gs_glyph glyph);

/* Get font parent (a Type 9 font). */
const gs_font_base *gs_font_parent(const gs_font_base *pbfont);

#endif /* gxfont_INCLUDED */
