/* Copyright (C) 1993, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsccode.h,v 1.14 2004/08/19 19:33:09 stefan Exp $ */
/* Types for character codes */

#ifndef gsccode_INCLUDED
#  define gsccode_INCLUDED

/*
 * Define a character code.  Normally this is just a single byte from a
 * string, but because of composite fonts, character codes must be
 * at least 32 bits.
 */
typedef ulong gs_char;

#define GS_NO_CHAR ((gs_char)~0L)
/* Backward compatibility */
#define gs_no_char GS_NO_CHAR

/*
 * Define a character glyph code, a.k.a. character name.  The space of
 * glyph codes is divided into five sections:
 *
 *	- Codes >= GS_MIN_GLYPH_INDEX represent (non-negative) 
 *	  integers biased by GS_MIN_CID_GLYPH.  They represent glyph indices
 *	  of a specific font.
 *
 *	- Codes within [GS_MIN_CID_GLYPH, GS_MIN_GLYPH_INDEX) represent (non-negative) 
 *	  integers biased by GS_MIN_CID_GLYPH.  They represent PostScript CIDs
 *        of a specific Ordering.
 *
 *	- Codes < GS_MIN_CID_GLYPH represent named glyphs.  There are
 *	  three sub-sections:
 *
 *	  - GS_NO_GLYPH, which means "no known glyph value".  Note that
 *	    it is not the same as /.notdef or CID 0 or GID 0: it means
 *	    that the identity of the glyph is unknown, as opposed to a
 *	    known glyph that is used for rendering an unknown character
 *	    code.
 *
 *	  - Codes < gs_c_min_std_encoding_glyph represent names in some
 *	    global space that the graphics library doesn't attempt to
 *	    interpret.  (When the client is the PostScript interpreter,
 *	    these codes are PostScript name indices, but the graphics
 *	    library doesn't know or rely on this.)  The graphics library
 *	    *does* assume that such codes denote the same names across
 *	    all fonts, and that they can be converted to a string name
 *	    by the font's glyph_name virtual procedure.
 *
 *	  - Codes >= gs_c_min_std_encoding_glyph (and < GS_MIN_CID_GLYPH)
 *	    represent names in a special space used for the 11 built-in
 *	    Encodings.  The API is defined in gscencs.h.  The only
 *	    procedures that generate or recognize such codes are the ones
 *	    declared in that file: clients must be careful not to mix
 *	    such codes with codes in the global space.
 *
 * Client code may assume that GS_NO_GLYPH < GS_MIN_CID_GLYPH (i.e., it is a
 * "name", not an integer), but should not make assumptions about whether
 * GS_NO_GLYPH is less than or greater than gs_c_min_std_encoding_glyph.
 */
typedef ulong gs_glyph;

#define GS_NO_GLYPH ((gs_glyph)0x7fffffff)
#if arch_sizeof_long > 4
#  define GS_MIN_CID_GLYPH ((gs_glyph)0x80000000L)
#else
/* Avoid compiler warnings about signed/unsigned constants. */
#  define GS_MIN_CID_GLYPH ((gs_glyph)~0x7fffffff)
#endif
#define GS_MIN_GLYPH_INDEX (GS_MIN_CID_GLYPH | (GS_MIN_CID_GLYPH >> 1))
#define GS_GLYPH_TAG (gs_glyph)(GS_MIN_CID_GLYPH | GS_MIN_GLYPH_INDEX)
#define GS_MAX_GLYPH max_ulong
/* Backward compatibility */
#define gs_no_glyph GS_NO_GLYPH
#define gs_min_cid_glyph GS_MIN_CID_GLYPH
#define gs_max_glyph GS_MAX_GLYPH

/* Define a procedure for marking a gs_glyph during garbage collection. */
typedef bool (*gs_glyph_mark_proc_t)(const gs_memory_t *mem, gs_glyph glyph, void *proc_data);

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
    ENCODING_INDEX_MACGLYPH,	/* Mac glyphs */
    ENCODING_INDEX_ALOGLYPH,	/* Adobe Latin glyph set */
    ENCODING_INDEX_ALXGLYPH,	/* Adobe Latin Extended glyph set */
    ENCODING_INDEX_CFFSTRINGS	/* CFF StandardStrings */
#define NUM_KNOWN_ENCODINGS 11
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
    GLYPH_SPACE_INDEX,		/* indexes (if available) */
    GLYPH_SPACE_NOGEN		/* don't generate new names (Type 3 only) */
} gs_glyph_space_t;

/*
 * Define a procedure for mapping a glyph to its (string) name.  This is
 * currently used only for CMaps: it is *not* the same as the glyph_name
 * procedure in fonts.
 */
typedef int (*gs_glyph_name_proc_t)(const gs_memory_t *mem, 
				    gs_glyph glyph, gs_const_string *pstr,
				    void *proc_data);

#endif /* gsccode_INCLUDED */
