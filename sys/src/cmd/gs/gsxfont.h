/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gsxfont.h */
/* External font client definitions for Ghostscript library */

#ifndef gsxfont_INCLUDED
#  define gsxfont_INCLUDED

/* Define a character glyph identifier.  This is opaque, probably an index */
/* into the font.  Glyph identifiers are font-specific. */
typedef ulong gx_xglyph;
#define gx_no_xglyph ((gx_xglyph)~0L)

/* Structure for xfont procedures. */
struct gx_xfont_procs_s;
typedef struct gx_xfont_procs_s gx_xfont_procs;

/* A generic xfont. */
struct gx_xfont_s;
typedef struct gx_xfont_s gx_xfont;

#endif					/* gsxfont_INCLUDED */
