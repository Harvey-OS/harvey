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

/* $Id: gxtext.h,v 1.16 2004/12/22 18:52:23 igor Exp $ */
/* Driver text interface implementation support */

#ifndef gxtext_INCLUDED
#  define gxtext_INCLUDED

#include "gstext.h"
#include "gsrefct.h"

/* Define the abstract type for the object procedures. */
typedef struct gs_text_enum_procs_s gs_text_enum_procs_t;

#ifndef cached_fm_pair_DEFINED
#  define cached_fm_pair_DEFINED
typedef struct cached_fm_pair_s cached_fm_pair;
#endif

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
    gx_device *imaging_dev;	/* see note below */\
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
    gs_glyph outer_CID; /* When a Type 3 is a FMapType 9 descendent. */\
    bool is_pure_color; /* The text is painted with a pure color. */\
    gs_log2_scale_point log2_scale;	/* for oversampling */\
    cached_fm_pair *pair; /* corresponds to the current_font and CTM*(1<<log2_scale) */\
    uint index;			/* index within string */\
    uint xy_index;		/* index within X/Y widths */\
    gx_font_stack_t fstack;\
    int cmap_code;		/* hack for FMapType 9 composite fonts, */\
				/* the value returned by decode_next */\
    gs_point FontBBox_as_Metrics2;  /* used with FontType 9,11 && WMode 1 */\
    /* The following is controlled by a device. */\
    bool device_disabled_grid_fitting;\
    /* The following are used to return information to the client. */\
    gs_text_returned_t returned
/* The typedef is in gstext.h. */
struct gs_text_enum_s {
    gs_text_enum_common;
};

/*
 * Notes on the imaging_dev field of device enumeration structures:
 *
 * This field is added as a hack to make the bbox device work
 * correctly as a forwarding device in some cases, particularly the X
 * driver. When the X driver is configured to use a memory device for
 * rendering (ie the MaxBitmap parameter is large enough to hold the
 * buffer), it sets up a pipeline where the bbox device forwards to
 * the memory device. The bbox device is used to determine which areas
 * of the buffer have been drawn on, so that the screen can be
 * appropriately updated.
 *
 * This works well for low-level operations such as filling
 * rectangles, because the bbox device can easily determine the bbox
 * of the drawing operation before forwarding it to the target device.
 * However, for higher level operations, such as those that require
 * enumerators, the approach is fundamentally broken. Essentially, the
 * execution of the drawing operation is the responsibility of the
 * target device, and the bbox device doesn't really have any way to
 * determine the bounding box.
 *
 * The approach taken here is to add an additional field to the
 * enumerations, imaging_dev. In the common case where the target
 * device implements the high level drawing operation in terms of
 * lower level operations, setting the imaging_dev field to non-NULL
 * requests that these lower level imaging operations be directed to
 * the imaging_dev rather than dev. The bbox device sets the
 * imaging_dev field to point to itself. Thus, the low level drawing
 * operations are intercepted by the bbox device, so that the bbox is
 * accounted for.
 *
 * Note that, if the target device implements higher level operations
 * by itself, ie not by breaking it into lower level operations, this
 * approach will fail.
 */

#define st_gs_text_enum_max_ptrs (st_gs_text_params_max_ptrs + 8)
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
int gs_text_enum_init(gs_text_enum_t *pte,
		      const gs_text_enum_procs_t *procs,
		      gx_device *dev, gs_imager_state *pis,
		      const gs_text_params_t *text,
		      gs_font *font, gx_path *path,
		      const gx_device_color *pdcolor,
		      const gx_clip_path *pcpath,
		      gs_memory_t *mem);

/*
 * Copy the dynamically changing elements from one enumerator to another.
 * This is useful primarily for enumerators that sometimes pass the
 * operation to a subsidiary enumerator.  Note that `returned' is copied
 * iff for_return is true.
 */
void gs_text_enum_copy_dynamic(gs_text_enum_t *pto,
			       const gs_text_enum_t *pfrom,
			       bool for_return);

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
  int proc(gs_text_enum_t *pte, const gs_text_enum_t *pfrom)

    text_enum_proc_resync((*resync));

    /*
     * Process the text.  The client should call this repeatedly until
     * it returns <= 0.  (> 0 means the client must take action: see
     * gstext.h.)
     *
     * Note that a default implementation of this procedure can't simply do
     * nothing and return:
     *
     *   - If TEXT_DO_CHARWIDTH or TEXT_DO_*PATH is set, the procedure must
     *   append the appropriate elements to the path.
     *
     *   - If TEXT_INTERVENE is set, the procedure must return to the client
     *   after each character except the last one in the string, setting
     *   returned.current_char and returned.current_glyph appropriately;
     *   also, it must reset the current font in the graphics state to its
     *   original value each time each time (after the first) that the
     *   procedure is called to process further characters of the string.
     *
     *   - If TEXT_RETURN_WIDTH is set, the procedure must set
     *   returned.total_width when(ever) it returns.
     *
     * We should provide a default implementation that makes all these
     * things simple, but currently we don't.
     */

#define text_enum_proc_process(proc)\
  int proc(gs_text_enum_t *pte)

    text_enum_proc_process((*process));

    /*
     * After the implementation returned TEXT_PROCESS_RENDER, determine
     * whether it needs the entire character description, or only the width
     * (escapement).
     */

#define text_enum_proc_is_width_only(proc)\
  bool proc(const gs_text_enum_t *pte)

    text_enum_proc_is_width_only((*is_width_only));

    /*
     * Return the width of the current character (in user space coordinates).
     */

#define text_enum_proc_current_width(proc)\
  int proc(const gs_text_enum_t *pte, gs_point *pwidth)

    text_enum_proc_current_width((*current_width));

    /*
     * Set the character width and optionally the bounding box,
     * and optionally enable caching.
     */

#define text_enum_proc_set_cache(proc)\
  int proc(gs_text_enum_t *pte, const double *values,\
    gs_text_cache_control_t control)

    text_enum_proc_set_cache((*set_cache));

    /*
     * Prepare to retry processing the current character by uninstalling the
     * cache device.
     */

#define text_enum_proc_retry(proc)\
  int proc(gs_text_enum_t *pte)

    text_enum_proc_retry((*retry));

    /*
     * Release the contents of the structure at the end of processing,
     * but don't free the structure itself.  (gs_text_release also does
     * the latter.)
     */

#define text_enum_proc_release(proc)\
  void proc(gs_text_enum_t *pte, client_name_t cname)

    text_enum_proc_release((*release));

};

/* Define the default release procedure. */
text_enum_proc_release(gx_default_text_release);

#endif /* gxtext_INCLUDED */
