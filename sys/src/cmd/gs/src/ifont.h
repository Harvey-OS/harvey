/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ifont.h,v 1.14 2004/08/04 19:36:12 stefan Exp $ */
/* Interpreter internal font representation */

#ifndef ifont_INCLUDED
#  define ifont_INCLUDED

#include "gsccode.h"		/* for gs_glyph, NUM_KNOWN_ENCODINGS */
#include "gsstype.h"		/* for extern_st */

/* The external definition of fonts is given in the PostScript manual, */
/* pp. 91-93. */

/* The structure given below is 'client data' from the viewpoint */
/* of the library.  font-type objects (t_struct/st_font, "t_fontID") */
/* point directly to a gs_font.  */

typedef struct font_data_s {
    ref dict;			/* font dictionary object */
    ref BuildChar;
    ref BuildGlyph;
    ref Encoding;
    ref CharStrings;
    ref GlyphNames2Unicode;
    union _fs {
	struct _f1 {
	    ref OtherSubrs;	/* from Private dictionary */
	    ref Subrs;		/* from Private dictionary */
	    ref GlobalSubrs;	/* from Private dictionary, */
	    /* for Type 2 charstrings */
	} type1;
	struct _f42 {
	    ref sfnts;
	    ref CIDMap;		/* for CIDFontType 2 fonts */
	    ref GlyphDirectory;
	} type42;
	struct _fc0 {
	    ref GlyphDirectory;
	    ref GlyphData;	/* (if preloaded) string or array of strings */
	    ref DataSource;	/* (if not preloaded) reusable stream */
	} cid0;
    } u;
} font_data;

/*
 * Even though the interpreter's part of the font data actually
 * consists of refs, allocating it as refs tends to create sandbars;
 * since it is always allocated and freed as a unit, we can treat it
 * as an ordinary structure.
 */
/* st_font_data is exported for zdefault_make_font in zfont.c. */
extern_st(st_font_data);
#define public_st_font_data()	/* in zbfont.c */\
  gs_public_st_ref_struct(st_font_data, font_data, "font_data")
#define pfont_data(pfont) ((font_data *)((pfont)->client_data))
#define pfont_dict(pfont) (&pfont_data(pfont)->dict)

/* ================Internal procedures shared across files ================ */

/* ---------------- Exported by zchar.c ---------------- */

/*
 * Get the FontBBox from a font dictionary, if any; if none, or if invalid,
 * return 4 zeros.
 */
int font_bbox_param(const gs_memory_t *mem, const ref *pfdict, double bbox[4]);

/* ---------------- Exported by zfont.c ---------------- */

#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif

/*
 * Check a parameter that should be a valid font dictionary, and return
 * the gs_font stored in its FID entry.
 */
int font_param(const ref * pfdict, gs_font ** ppfont);

/*
 * Mark a glyph as a PostScript name (if it isn't a CID) for the garbage
 * collector.  Return true if a mark was just added.  This procedure is
 * intended to be used as the mark_glyph procedure in the character cache.
 */
bool zfont_mark_glyph_name(const gs_memory_t *mem, gs_glyph glyph, void *ignore_data);

/*
 * Return information about a font, including information from the FontInfo
 * dictionary.  This procedure is intended to be used as the font_info
 * procedure in all PostScript fonts.
 */
font_proc_font_info(zfont_info);

#endif /* ifont_INCLUDED */
