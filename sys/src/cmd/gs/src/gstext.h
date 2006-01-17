/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gstext.h,v 1.9 2003/09/11 21:12:18 igor Exp $ */
/* Driver interface for text */

#ifndef gstext_INCLUDED
#  define gstext_INCLUDED

#include "gsccode.h"
#include "gscpm.h"

/*
 * Note that text display must return information to the generic code:
 *	If TEXT_RETURN_WIDTH or TEXT_DO_CHARWIDTH, the string escapement
 *	  (a.k.a. "width");
 *	If TEXT_DO_*_CHARPATH, the entire character description;
 *	If TEXT_DO_*_CHARBOXPATH, the character bounding box.
 */

/*
 * Define the set of possible text operations.  While we define this as
 * a bit mask for convenience in testing, only certain combinations are
 * meaningful.  Specifically, the following are errors:
 *      - No FROM or DO.
 *      - More than one FROM or DO.
 *	- FROM_SINGLE with size != 1.
 *      - Both ADD_TO and REPLACE.
 */
#define TEXT_HAS_MORE_THAN_ONE_(op, any)\
  ( ((op) & any) & (((op) & any) - 1) )
#define TEXT_OPERATION_IS_INVALID(op)\
  (!((op) & TEXT_FROM_ANY) ||\
   !((op) & TEXT_DO_ANY) ||\
   TEXT_HAS_MORE_THAN_ONE_(op, TEXT_FROM_ANY) ||\
   TEXT_HAS_MORE_THAN_ONE_(op, TEXT_DO_ANY) ||\
   (((op) & TEXT_ADD_ANY) && ((op) & TEXT_REPLACE_WIDTHS))\
   )
#define TEXT_PARAMS_ARE_INVALID(params)\
  (TEXT_OPERATION_IS_INVALID((params)->operation) ||\
   ( ((params)->operation & TEXT_FROM_ANY_SINGLE) && ((params)->size != 1) )\
   )

	/* Define the representation of the text itself. */
#define TEXT_FROM_STRING          0x00001
#define TEXT_FROM_BYTES           0x00002
#define TEXT_FROM_CHARS           0x00004
#define TEXT_FROM_GLYPHS          0x00008
#define TEXT_FROM_SINGLE_CHAR     0x00010
#define TEXT_FROM_SINGLE_GLYPH    0x00020
#define TEXT_FROM_ANY_SINGLE	/* only for testing and masking */\
  (TEXT_FROM_SINGLE_CHAR | TEXT_FROM_SINGLE_GLYPH)
#define TEXT_FROM_ANY	/* only for testing and masking */\
  (TEXT_FROM_STRING | TEXT_FROM_BYTES | TEXT_FROM_CHARS | TEXT_FROM_GLYPHS |\
   TEXT_FROM_ANY_SINGLE)
	/* Define how to compute escapements. */
#define TEXT_ADD_TO_ALL_WIDTHS    0x00040
#define TEXT_ADD_TO_SPACE_WIDTH   0x00080
#define TEXT_ADD_ANY	/* only for testing and masking */\
  (TEXT_ADD_TO_ALL_WIDTHS | TEXT_ADD_TO_SPACE_WIDTH)
#define TEXT_REPLACE_WIDTHS       0x00100
	/* Define what result should be produced. */
#define TEXT_DO_NONE              0x00200	/* stringwidth or cshow only */
#define TEXT_DO_DRAW              0x00400
#define TEXT_DO_CHARWIDTH         0x00800	/* rmoveto by width */
#define TEXT_DO_FALSE_CHARPATH    0x01000
#define TEXT_DO_TRUE_CHARPATH     0x02000
#define TEXT_DO_FALSE_CHARBOXPATH 0x04000
#define TEXT_DO_TRUE_CHARBOXPATH  0x08000
#define TEXT_DO_ANY_CHARPATH	/* only for testing and masking */\
  (TEXT_DO_CHARWIDTH | TEXT_DO_FALSE_CHARPATH | TEXT_DO_TRUE_CHARPATH |\
   TEXT_DO_FALSE_CHARBOXPATH | TEXT_DO_TRUE_CHARBOXPATH)
#define TEXT_DO_ANY	/* only for testing and masking */\
  (TEXT_DO_NONE | TEXT_DO_DRAW | TEXT_DO_ANY_CHARPATH)
	/* Define whether the client intervenes between characters. */
#define TEXT_INTERVENE            0x10000
	/* Define whether to return the width. */
#define TEXT_RETURN_WIDTH         0x20000

/*
 * Define the structure of parameters passed in for text display.
 * Note that the implementation does not modify any of these; the client
 * must not modify them after initialization.
 */
typedef struct gs_text_params_s {
    /* The client must set the following in all cases. */
    uint operation;		/* TEXT_xxx mask */
    union sd_ {
	const byte *bytes;	/* FROM_STRING, FROM_BYTES */
	const gs_char *chars;	/* FROM_CHARS */
	const gs_glyph *glyphs;	/* FROM_GLYPHS */
	gs_char d_char;		/* FROM_SINGLE_CHAR */
	gs_glyph d_glyph;	/* FROM_SINGLE_GLYPH */
    } data;
    uint size;			/* number of data elements, */
				/* must be 1 if FROM_SINGLE */
    /* The following are used only in the indicated cases. */
    gs_point delta_all;		/* ADD_TO_ALL_WIDTHS */
    gs_point delta_space;	/* ADD_TO_SPACE_WIDTH */
    union s_ {
	gs_char s_char;		/* ADD_TO_SPACE_WIDTH & !FROM_GLYPHS */
	gs_glyph s_glyph;	/* ADD_TO_SPACE_WIDTH & FROM_GLYPHS */
    } space;
    /* If x_widths == y_widths, widths are taken in pairs. */
    /* Either one may be NULL, meaning widths = 0. */
    const float *x_widths;	/* REPLACE_WIDTHS */
    const float *y_widths;	/* REPLACE_WIDTHS */
    uint widths_size;		/****** PROBABLY NOT NEEDED ******/
} gs_text_params_t;

#define st_gs_text_params_max_ptrs 3
/*extern_st(st_gs_text_params); */
#define public_st_gs_text_params() /* in gstext.c */\
  gs_public_st_composite(st_gs_text_params, gs_text_params_t,\
    "gs_text_params", text_params_enum_ptrs, text_params_reloc_ptrs)

/* Assuming REPLACE_WIDTHS is set, return the width of the i'th character. */
int gs_text_replaced_width(const gs_text_params_t *text, uint index,
			   gs_point *pwidth);

/*
 * Define the abstract type for the structure that tracks the state of text
 * processing.
 */
#ifndef gs_text_enum_DEFINED
#  define gs_text_enum_DEFINED
typedef struct gs_text_enum_s gs_text_enum_t;
#endif

/* Abstract types */
#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif
#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif
#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif
#ifndef gx_path_DEFINED
#  define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif
#ifndef gx_clip_path_DEFINED
#  define gx_clip_path_DEFINED
typedef struct gx_clip_path_s gx_clip_path;
#endif

/*
 * Define the driver procedure for text.  This procedure must allocate
 * the enumerator (see gxtext.h) and initialize the procs and rc members.
 */
#define dev_t_proc_text_begin(proc, dev_t)\
  int proc(dev_t *dev,\
    gs_imager_state *pis,\
    const gs_text_params_t *text,\
    gs_font *font,\
    gx_path *path,			/* unless DO_NONE */\
    const gx_device_color *pdcolor,	/* if DO_DRAW */\
    const gx_clip_path *pcpath,		/* if DO_DRAW */\
    gs_memory_t *memory,\
    gs_text_enum_t **ppte)
#define dev_proc_text_begin(proc)\
  dev_t_proc_text_begin(proc, gx_device)

/*
 * Begin processing text.  This calls the device procedure, and also
 * initializes the common parts of the enumerator.
 */
dev_proc_text_begin(gx_device_text_begin);

/* Begin processing text with a graphics state. */
#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;
#endif
int gs_text_begin(gs_state * pgs, const gs_text_params_t * text,
		  gs_memory_t * mem, gs_text_enum_t ** ppenum);

/*
 * Update the device color to be used with text (because a kshow or
 * cshow procedure may have changed the current color).
 */
int gs_text_update_dev_color(gs_state * pgs, gs_text_enum_t * pte);


/* Begin the PostScript-equivalent text operators. */
int
gs_show_begin(gs_state *, const byte *, uint,
	      gs_memory_t *, gs_text_enum_t **),
    gs_ashow_begin(gs_state *, floatp, floatp, const byte *, uint,
		   gs_memory_t *, gs_text_enum_t **),
    gs_widthshow_begin(gs_state *, floatp, floatp, gs_char,
		       const byte *, uint,
		       gs_memory_t *, gs_text_enum_t **),
    gs_awidthshow_begin(gs_state *, floatp, floatp, gs_char,
			floatp, floatp, const byte *, uint,
			gs_memory_t *, gs_text_enum_t **),
    gs_kshow_begin(gs_state *, const byte *, uint,
		   gs_memory_t *, gs_text_enum_t **),
    gs_xyshow_begin(gs_state *, const byte *, uint,
		    const float *, const float *, uint,
		    gs_memory_t *, gs_text_enum_t **),
    gs_glyphshow_begin(gs_state *, gs_glyph,
		       gs_memory_t *, gs_text_enum_t **),
    gs_cshow_begin(gs_state *, const byte *, uint,
		   gs_memory_t *, gs_text_enum_t **),
    gs_stringwidth_begin(gs_state *, const byte *, uint,
			 gs_memory_t *, gs_text_enum_t **),
    gs_charpath_begin(gs_state *, const byte *, uint, bool,
		      gs_memory_t *, gs_text_enum_t **),
    gs_glyphpath_begin(gs_state *, gs_glyph, bool,
		       gs_memory_t *, gs_text_enum_t **),
    gs_glyphwidth_begin(gs_state *, gs_glyph,
			gs_memory_t *, gs_text_enum_t **),
    gs_charboxpath_begin(gs_state *, const byte *, uint, bool,
			 gs_memory_t *, gs_text_enum_t **);

/*
 * Restart text processing with new parameters.
 */
int gs_text_restart(gs_text_enum_t *pte, const gs_text_params_t *text);

/*
 * Resync text processing with new parameters and string position.
 */
int gs_text_resync(gs_text_enum_t *pte, const gs_text_enum_t *pfrom);

/*
 * Define the possible return values from gs_text_process.  The client
 * should call text_process until it returns 0 (successful completion) or a
 * negative (error) value.
 */

	/*
	 * The client must render a character: obtain the code from
	 * gs_text_current_char/glyph, do whatever is necessary, and then
	 * call gs_text_process again.
	 */
#define TEXT_PROCESS_RENDER 1

	/*
	 * The client has asked to intervene between characters.
	 * Obtain the current and next codes from gs_text_current_char/glyph
	 * and gs_text_next_char, do whatever is necessary, and then
	 * call gs_text_process again.
	 */
#define TEXT_PROCESS_INTERVENE 2

	/*
	 * The device has asked to execute CDevProc.
	 * Obtain the current codes from gs_text_current_char/glyph,
	 * do whatever is necessary and put CDevProc results to pte->cdevproc_result, 
	 * and then call gs_text_process again with pte->cdevproc_result_valid=true.
	 */
#define TEXT_PROCESS_CDEVPROC 3

/* Process text after 'begin'. */
int gs_text_process(gs_text_enum_t *pte);

/* Access elements of the enumerator. */
gs_font *gs_text_current_font(const gs_text_enum_t *pte);
gs_char gs_text_current_char(const gs_text_enum_t *pte);
gs_char gs_text_next_char(const gs_text_enum_t *pte);
gs_glyph gs_text_current_glyph(const gs_text_enum_t *pte);
int gs_text_total_width(const gs_text_enum_t *pte, gs_point *pwidth);

/*
 * After the implementation returned TEXT_PROCESS_RENDER, determine
 * whether it needs the entire character description, or only the width
 * (escapement).
 */
bool gs_text_is_width_only(const gs_text_enum_t *pte);

/*
 * Return the width of the current character (in user space coordinates).
 */
int gs_text_current_width(const gs_text_enum_t *pte, gs_point *pwidth);

/*
 * Set text metrics and optionally enable caching.  Return 1 iff the
 * cache device was just installed.
 */
typedef enum {
    TEXT_SET_CHAR_WIDTH,	/* wx wy */
    TEXT_SET_CACHE_DEVICE,	/* wx wy llx lly urx ury */
    TEXT_SET_CACHE_DEVICE2	/* w0x w0y llx lly urx ury w1x w1y vx vy */
} gs_text_cache_control_t;
int
    gs_text_set_cache(gs_text_enum_t *pte, const double *values,
		      gs_text_cache_control_t control),
    gs_text_setcharwidth(gs_text_enum_t *pte, const double wxy[2]),
    gs_text_setcachedevice(gs_text_enum_t *pte, const double wbox[6]),
    gs_text_setcachedevice2(gs_text_enum_t *pte, const double wbox2[10]);

/* Retry processing of the last character. */
int gs_text_retry(gs_text_enum_t *pte);

/* Release the text processing structures. */
void gs_text_release(gs_text_enum_t *pte, client_name_t cname);

#endif /* gstext_INCLUDED */
