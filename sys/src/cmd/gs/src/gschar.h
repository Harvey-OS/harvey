/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gschar.h,v 1.2 2000/03/10 04:41:47 lpd Exp $ */
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
gs_show_enum *gs_show_enum_alloc(P3(gs_memory_t *, gs_state *, client_name_t));

/* Release the contents of an enumerator. */
/* (This happens automatically if the enumeration finishes normally.) */
/* If the second argument is not NULL, also free the enumerator. */
void gs_show_enum_release(P2(gs_show_enum *, gs_memory_t *));

/* Initialize a text enumeration. */
int
    /* PostScript operators */
    gs_show_n_init(P4(gs_show_enum *, gs_state *, const char *, uint)),
    gs_ashow_n_init(P6(gs_show_enum *, gs_state *, floatp, floatp, const char *, uint)),
    gs_widthshow_n_init(P7(gs_show_enum *, gs_state *, floatp, floatp, gs_char, const char *, uint)),
    gs_awidthshow_n_init(P9(gs_show_enum *, gs_state *, floatp, floatp, gs_char, floatp, floatp, const char *, uint)),
    gs_kshow_n_init(P4(gs_show_enum *, gs_state *, const char *, uint)),
    gs_xyshow_n_init(P4(gs_show_enum *, gs_state *, const char *, uint)),
    gs_glyphshow_init(P3(gs_show_enum *, gs_state *, gs_glyph)), gs_cshow_n_init(P4(gs_show_enum *, gs_state *, const char *, uint)),
    gs_stringwidth_n_init(P4(gs_show_enum *, gs_state *, const char *, uint)),
    gs_charpath_n_init(P5(gs_show_enum *, gs_state *, const char *, uint, bool)),
    /* Extensions */
    gs_glyphpath_init(P4(gs_show_enum *, gs_state *, gs_glyph, bool)),
    gs_glyphwidth_init(P3(gs_show_enum *, gs_state *, gs_glyph)),
    gs_charboxpath_n_init(P5(gs_show_enum *, gs_state *, const char *, uint, bool));
int gs_show_use_glyph(P2(gs_show_enum *, gs_glyph));

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

int gs_show_next(P1(gs_show_enum *));

gs_char
    gs_show_current_char(P1(const gs_show_enum *)),
    gs_kshow_previous_char(P1(const gs_show_enum *)),
    gs_kshow_next_char(P1(const gs_show_enum *));
gs_font *
    gs_show_current_font(P1(const gs_show_enum *));

gs_glyph
    gs_show_current_glyph(P1(const gs_show_enum *));
int gs_show_current_width(P2(const gs_show_enum *, gs_point *));
void gs_show_width(P2(const gs_show_enum *, gs_point *));  /* cumulative width */

gs_char_path_mode
    gs_show_in_charpath(P1(const gs_show_enum *));  /* return charpath flag */

/* Character cache and metrics operators. */
/* gs_setcachedevice* return 1 iff the cache device was just installed. */
int gs_setcachedevice_float(P3(gs_show_enum *, gs_state *, const float * /*[6] */ ));
int gs_setcachedevice_double(P3(gs_show_enum *, gs_state *, const double * /*[6] */ ));
#define gs_setcachedevice(penum, pgs, pw)\
  gs_setcachedevice_float(penum, pgs, pw)
int gs_setcachedevice2_float(P3(gs_show_enum *, gs_state *, const float * /*[10] */ ));
int gs_setcachedevice2_double(P3(gs_show_enum *, gs_state *, const double * /*[10] */ ));
#define gs_setcachedevice2(penum, pgs, pw2)\
  gs_setcachedevice2_float(penum, pgs, pw2)
int gs_setcharwidth(P4(gs_show_enum *, gs_state *, floatp, floatp));

/* Return true if we only need the width from the rasterizer */
/* and can short-circuit the full rendering of the character, */
/* false if we need the actual character bits. */
/* This is only meaningful just before calling gs_setcharwidth or */
/* gs_setcachedevice[2]. */
bool gs_show_width_only(P1(const gs_show_enum *));

#endif /* gschar_INCLUDED */
