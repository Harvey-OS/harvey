/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxtext.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Driver text interface implementation support */

#ifndef gxtext_INCLUDED
#  define gxtext_INCLUDED

#include "gstext.h"
#include "gsrefct.h"

/*
 * WARNING: The APIs and structures in this file are UNSTABLE.
 * Do not try to use them.
 */

/* Define the abstract type for the object procedures. */
typedef struct gs_text_enum_procs_s gs_text_enum_procs_t;

/*
 * Define values returned by text_process to the client.
 */
typedef struct gs_text_returned_s {
    gs_char current_char;	/* INTERVENE */
    gs_glyph current_glyph;	/* INTERVENE */
    gs_point total_width;	/* RETURN_WIDTH */
} gs_text_returned_t;

/*
 * Define the stack for composite fonts.
 * If the current font is not composite, depth = -1.
 * If the current font is composite, 0 <= depth <= MAX_FONT_STACK.
 * items[0] through items[depth] are occupied.
 * items[0].font is the root font; items[0].index = 0.
 * The root font must be composite, but may be of any map type.
 * items[0..N-1] are modal composite fonts, for some N <= depth.
 * items[N..depth-1] are non-modal composite fonts.
 * items[depth] is a base (non-composite) font.
 * Note that if depth >= 0, the font member of the graphics state
 * for a base font BuildChar/Glyph is the same as items[depth].font.
 */
#define MAX_FONT_STACK 5
typedef struct gx_font_stack_item_s {
    gs_font *font;		/* font at this level */
    uint index;			/* index of this font in parent's Encoding */
} gx_font_stack_item_t;
typedef struct gx_font_stack_s {
    int depth;
    gx_font_stack_item_t items[1 + MAX_FONT_STACK];
} gx_font_stack_t;

/*
 * Define the common part of the structure that tracks the state of text
 * processing.  All implementations of text_begin must allocate one of these
 * using rc_alloc_struct_1; implementations may subclass and extend it.
 * Note that it includes a copy of the text parameters.
 *
 * The freeing procedure (rc.free) must call rc_free_text_enum, which
 * calls the enumerator's release procedure.  This is required in order to
 * properly decrement the reference count(s) of the referenced structures
 * (in the common part of the structure, only the device).
 */
rc_free_proc(rc_free_text_enum);
#define gs_text_enum_common\
    /*\
     * The following copies of the arguments of text_begin are set at\
     * initialization, and const thereafter.\
     */\
    gs_text_params_t text;	/* must be first for subclassing */\
    gx_device *dev;\
    gs_imager_state *pis;\
    gs_font *orig_font;\
    gx_path *path;			/* unless DO_NONE & !RETURN_WIDTH */\
    const gx_device_color *pdcolor;	/* if DO_DRAW */\
    const gx_clip_path *pcpath;		/* if DO_DRAW */\
    gs_memory_t *memory;\
    /* The following additional members are set at initialization. */\
    const gs_text_enum_procs_t *procs;\
    /* The following change dynamically.  NOTE: gs_text_enum_copy_dynamic */\
    /* knows the entire list of dynamically changing elements. */\
    rc_header rc;\
    gs_font *current_font; /* changes for composite fonts */\
    gs_log2_scale_point log2_scale;	/* for oversampling */\
    uint index;			/* index within string */\
    uint xy_index;		/* index within X/Y widths */\
    gx_font_stack_t fstack;\
    int cmap_code;		/* hack for FMapType 9 composite fonts, */\
				/* the value returned by decode_next */\
    /* The following are used to return information to the client. */\
    gs_text_returned_t returned
/* The typedef is in gstext.h. */
/*typedef*/ struct gs_text_enum_s {
    gs_text_enum_common;
} /*gs_text_enum_t*/;

#define st_gs_text_enum_max_ptrs (st_gs_text_params_max_ptrs + 7)
/*extern_st(st_gs_text_enum); */
#define public_st_gs_text_enum()	/* in gstext.c */\
  gs_public_st_composite(st_gs_text_enum, gs_text_enum_t, "gs_text_enum_t",\
    text_enum_enum_ptrs, text_enum_reloc_ptrs)

/* 
 * Initialize a newly created text enumerator.  Implementations of
 * text_begin must call this just after allocating the enumerator.
 * Note that this procedure can return an error, e.g., if attempting
 * a glyph-based operation with a composite font.
 */
int gs_text_enum_init(P10(gs_text_enum_t *pte,
			  const gs_text_enum_procs_t *procs,
			  gx_device *dev, gs_imager_state *pis,
			  const gs_text_params_t *text,
			  gs_font *font, gx_path *path,
			  const gx_device_color *pdcolor,
			  const gx_clip_path *pcpath,
			  gs_memory_t *mem));

/*
 * Copy the dynamically changing elements from one enumerator to another.
 * This is useful primarily for enumerators that sometimes pass the
 * operation to a subsidiary enumerator.  Note that `returned' is copied
 * iff for_return is true.
 */
void gs_text_enum_copy_dynamic(P3(gs_text_enum_t *pto,
				  const gs_text_enum_t *pfrom,
				  bool for_return));

/*
 * Define some convenience macros for testing aspects of a text
 * enumerator.
 */

#define SHOW_IS(penum, op_mask)\
  (((penum)->text.operation & (op_mask)) != 0)
#define SHOW_IS_ALL_OF(penum, op_mask)\
  (((penum)->text.operation & (op_mask)) == (op_mask))
    /*
     * The comments next to the following macros indicate the
     * corresponding test on gs_show_enum structures in pre-5.24 filesets.
     */
#define SHOW_IS_ADD_TO_ALL(penum)	/* add */\
  SHOW_IS(penum, TEXT_ADD_TO_ALL_WIDTHS)
#define SHOW_IS_ADD_TO_SPACE(penum)	/* wchr != no_char */\
  SHOW_IS(penum, TEXT_ADD_TO_SPACE_WIDTH)
#define SHOW_IS_DO_KERN(penum)		/* do_kern */\
  SHOW_IS(penum, TEXT_INTERVENE)
#define SHOW_IS_SLOW(penum)		/* slow_show */\
  SHOW_IS(penum, TEXT_REPLACE_WIDTHS | TEXT_ADD_TO_ALL_WIDTHS | TEXT_ADD_TO_SPACE_WIDTH | TEXT_INTERVENE)
#define SHOW_IS_DRAWING(penum)		/* !stringwidth_flag */\
  !SHOW_IS(penum, TEXT_DO_NONE)
#define SHOW_IS_STRINGWIDTH(penum)	/* stringwidth_flag > 0 */\
  SHOW_IS_ALL_OF(penum, TEXT_DO_NONE | TEXT_RETURN_WIDTH)

/*
 * Define the procedures associated with text processing.
 */
struct gs_text_enum_procs_s {

    /*
     * Resync processing from an enumerator that may have different
     * parameters and may be partway through processing the string.  Note
     * that this may only be implemented for certain kinds of changes, and
     * will fail for other kinds.  (We may reconsider this.)  We require
     * pfrom != pte.
     */

#define text_enum_proc_resync(proc)\
  int proc(P2(gs_text_enum_t *pte, const gs_text_enum_t *pfrom))

    text_enum_proc_resync((*resync));

    /*
     * Process the text.  Then client should call this repeatedly until
     * it returns <= 0.  (> 0 means the client must take action: see
     * gstext.h.)
     */

#define text_enum_proc_process(proc)\
  int proc(P1(gs_text_enum_t *pte))

    text_enum_proc_process((*process));

    /*
     * After the implementation returned TEXT_PROCESS_RENDER, determine
     * whether it needs the entire character description, or only the width
     * (escapement).
     */

#define text_enum_proc_is_width_only(proc)\
  int proc(P1(const gs_text_enum_t *pte))

    text_enum_proc_is_width_only((*is_width_only));

    /*
     * Return the width of the current character (in user space coordinates).
     */

#define text_enum_proc_current_width(proc)\
  int proc(P2(const gs_text_enum_t *pte, gs_point *pwidth))

    text_enum_proc_current_width((*current_width));

    /*
     * Set the character width and optionally the bounding box,
     * and optionally enable caching.
     */

#define text_enum_proc_set_cache(proc)\
  int proc(P3(gs_text_enum_t *pte, const double *values,\
    gs_text_cache_control_t control))

    text_enum_proc_set_cache((*set_cache));

    /*
     * Prepare to retry processing the current character by uninstalling the
     * cache device.
     */

#define text_enum_proc_retry(proc)\
  int proc(P1(gs_text_enum_t *pte))

    text_enum_proc_retry((*retry));

    /*
     * Release the contents of the structure at the end of processing,
     * but don't free the structure itself.  (gs_text_release also does
     * the latter.)
     */

#define text_enum_proc_release(proc)\
  void proc(P2(gs_text_enum_t *pte, client_name_t cname))

    text_enum_proc_release((*release));

};

/* Define the default release procedure. */
text_enum_proc_release(gx_default_text_release);

#endif /* gxtext_INCLUDED */
