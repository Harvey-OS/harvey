/* Copyright (C) 1993, 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsccode.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Types for character codes */

#ifndef gsccode_INCLUDED
#  define gsccode_INCLUDED

/*
 * Define a character code.  Normally this is just a single byte from a
 * string, but because of composite fonts, character codes must be
 * at least 32 bits.
 */
typedef ulong gs_char;

#define gs_no_char ((gs_char)~0L)

/*
 * Define a character glyph code, a.k.a. character name.
 * gs_glyphs from 0 to 2^31-1 are (PostScript) names; gs_glyphs 2^31 and
 * above are CIDs, biased by 2^31.
 */
typedef ulong gs_glyph;

#define gs_no_glyph ((gs_glyph)0x7fffffff)
#if arch_sizeof_long > 4
#  define gs_min_cid_glyph ((gs_glyph)0x80000000L)
#else
/* Avoid compiler warnings about signed/unsigned constants. */
#  define gs_min_cid_glyph ((gs_glyph)~0x7fffffff)
#endif
#define gs_max_glyph max_ulong

/* Define a procedure for marking a gs_glyph during garbage collection. */
typedef bool(*gs_glyph_mark_proc_t) (P2(gs_glyph glyph, void *proc_data));

/* Define a procedure for mapping a gs_glyph to its (string) name. */
#define gs_proc_glyph_name(proc)\
  const char *proc(P2(gs_glyph, uint *))
/* The following typedef is needed because ansi2knr can't handle */
/* gs_proc_glyph_name((*procname)) in a formal argument list. */
typedef gs_proc_glyph_name((*gs_proc_glyph_name_t));

/* Define the indices for known encodings. */
typedef enum {
    ENCODING_INDEX_UNKNOWN = -1,
	/* Real encodings.  These must come first. */
    ENCODING_INDEX_STANDARD = 0,
    ENCODING_INDEX_ISOLATIN1,
    ENCODING_INDEX_SYMBOL,
    ENCODING_INDEX_DINGBATS,
    ENCODING_INDEX_WINANSI,
    ENCODING_INDEX_MACROMAN,
    ENCODING_INDEX_MACEXPERT,
#define NUM_KNOWN_REAL_ENCODINGS 7
	/* Pseudo-encodings (glyph sets). */
    ENCODING_INDEX_MACGLYPH,	/* a pseudo-encoding */
    ENCODING_INDEX_ALOGLYPH,	/* ditto */
    ENCODING_INDEX_ALXGLYPH	/* ditto */
#define NUM_KNOWN_ENCODINGS 10
} gs_encoding_index_t;
#define KNOWN_REAL_ENCODING_NAMES\
  "StandardEncoding", "ISOLatin1Encoding", "SymbolEncoding",\
  "DingbatsEncoding", "WinAnsiEncoding", "MacRomanEncoding",\
  "MacExpertEncoding"

/*
 * For fonts that use more than one method to identify glyphs, define the
 * glyph space for the values returned by procedures that return glyphs.
 * Note that if a font uses only one method (such as Type 1 fonts, which
 * only use names, or TrueType fonts, which only use indexes), the
 * glyph_space argument is ignored.
 */
typedef enum gs_glyph_space_s {
    GLYPH_SPACE_NAME,		/* names (if available) */
    GLYPH_SPACE_INDEX		/* indexes (if available) */
} gs_glyph_space_t;

/*
 * Define a procedure for accessing the known encodings.  Note that if
 * there is a choice, this procedure always returns a glyph name, not a
 * glyph index.
 */
#define gs_proc_known_encode(proc)\
  gs_glyph proc(P2(gs_char, int))
typedef gs_proc_known_encode((*gs_proc_known_encode_t));

/* Define the callback procedure vector for character to xglyph mapping. */
typedef struct gx_xfont_callbacks_s {
    gs_proc_glyph_name((*glyph_name));
    gs_proc_known_encode((*known_encode));
} gx_xfont_callbacks;

#endif /* gsccode_INCLUDED */
