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

/* $Id: gschar.h,v 1.6 2002/06/16 08:45:42 lpd Exp $ */
/* Client interface to character operations */

#ifndef gschar_INCLUDED
#  define gschar_INCLUDED

#include "gsccode.h"
#include "gscpm.h"

/* String display, like image display, uses an enumeration structure */
/* to keep track of what's going on (aka 'poor man's coroutine'). */
#ifndef gs_show_enum_DEFINED
#  define gs_show_enum_DEFINED
typedef struct gs_show_enum_s gs_show_enum;
#endif

/* Define an opaque type for fonts if necessary. */
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif

/* Allocate an enumerator. */
gs_show_enum *gs_show_enum_alloc(gs_memory_t *, gs_state *, client_name_t);

/* Release the contents of an enumerator. */
/* (This happens automatically if the enumeration finishes normally.) */
/* If the second argument is not NULL, also free the enumerator. */
void gs_show_enum_release(gs_show_enum *, gs_memory_t *);

/* Initialize a text enumeration. */
int
    /* PostScript operators */
    gs_show_n_init(gs_show_enum *, gs_state *, const char *, uint),
    gs_ashow_n_init(gs_show_enum *, gs_state *, floatp, floatp, const char *, uint),
    gs_widthshow_n_init(gs_show_enum *, gs_state *, floatp, floatp, gs_char, const char *, uint),
    gs_awidthshow_n_init(gs_show_enum *, gs_state *, floatp, floatp, gs_char, floatp, floatp, const char *, uint),
    gs_kshow_n_init(gs_show_enum *, gs_state *, const char *, uint),
    gs_xyshow_n_init(gs_show_enum *, gs_state *, const char *, uint),
    gs_glyphshow_init(gs_show_enum *, gs_state *, gs_glyph), gs_cshow_n_init(gs_show_enum *, gs_state *, const char *, uint),
    gs_stringwidth_n_init(gs_show_enum *, gs_state *, const char *, uint),
    gs_charpath_n_init(gs_show_enum *, gs_state *, const char *, uint, bool),
    /* Extensions */
    gs_glyphpath_init(gs_show_enum *, gs_state *, gs_glyph, bool),
    gs_glyphwidth_init(gs_show_enum *, gs_state *, gs_glyph),
    gs_charboxpath_n_init(gs_show_enum *, gs_state *, const char *, uint, bool);
int gs_show_use_glyph(gs_show_enum *, gs_glyph);

/* After setting up the enumeration, all the string-related routines */
/* work the same way.  The client calls gs_show_next until it returns */
/* a zero (successful completion) or negative (error) value. */
/* Other values indicate the following situations: */

	/* The client must render a character: obtain the code from */
	/* gs_show_current_char, do whatever is necessary, and then */
	/* call gs_show_next again. */
#define gs_show_render TEXT_PROCESS_RENDER

	/* The client has asked to intervene between characters (kshow). */
	/* Obtain the previous and next codes from gs_kshow_previous_char */
	/* and gs_kshow_next_char, do whatever is necessary, and then */
	/* call gs_show_next again. */
#define gs_show_kern TEXT_PROCESS_INTERVENE

	/* The client has asked to handle characters individually */
	/* (xshow, yshow, xyshow, cshow).  Obtain the current code */
	/* from gs_show_current_char, do whatever is necessary, and then */
	/* call gs_show_next again. */
#define gs_show_move TEXT_PROCESS_INTERVENE

int gs_show_next(gs_show_enum *);

gs_char
    gs_show_current_char(const gs_show_enum *),
    gs_kshow_previous_char(const gs_show_enum *),
    gs_kshow_next_char(const gs_show_enum *);
gs_font *
    gs_show_current_font(const gs_show_enum *);

gs_glyph
    gs_show_current_glyph(const gs_show_enum *);
int gs_show_current_width(const gs_show_enum *, gs_point *);
void gs_show_width(const gs_show_enum *, gs_point *);  /* cumulative width */

gs_char_path_mode
    gs_show_in_charpath(const gs_show_enum *);  /* return charpath flag */

/* Character cache and metrics operators. */
/* gs_setcachedevice* return 1 iff the cache device was just installed. */
int gs_setcachedevice_float(gs_show_enum *, gs_state *, const float * /*[6] */ );
int gs_setcachedevice_double(gs_show_enum *, gs_state *, const double * /*[6] */ );
#define gs_setcachedevice(penum, pgs, pw)\
  gs_setcachedevice_float(penum, pgs, pw)
int gs_setcachedevice2_float(gs_show_enum *, gs_state *, const float * /*[10] */ );
int gs_setcachedevice2_double(gs_show_enum *, gs_state *, const double * /*[10] */ );
#define gs_setcachedevice2(penum, pgs, pw2)\
  gs_setcachedevice2_float(penum, pgs, pw2)
int gs_setcharwidth(gs_show_enum *, gs_state *, floatp, floatp);

/* Return true if we only need the width from the rasterizer */
/* and can short-circuit the full rendering of the character, */
/* false if we need the actual character bits. */
/* This is only meaningful just before calling gs_setcharwidth or */
/* gs_setcachedevice[2]. */
bool gs_show_width_only(const gs_show_enum *);

#endif /* gschar_INCLUDED */
