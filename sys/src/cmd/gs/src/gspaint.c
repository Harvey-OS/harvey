/* Copyright (C) 1989, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gspaint.c,v 1.10 2005/10/12 17:59:55 leonardo Exp $ */
/* Painting procedures for Ghostscript library */
#include "math_.h"		/* for fabs */
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsropt.h"		/* for gxpaint.h */
#include "gxfixed.h"
#include "gxmatrix.h"		/* for gs_state */
#include "gspaint.h"
#include "gspath.h"
#include "gzpath.h"
#include "gxpaint.h"
#include "gzstate.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gzcpath.h"
#include "gxhldevc.h"

/* Define the nominal size for alpha buffers. */
#define abuf_nominal_SMALL 500
#define abuf_nominal_LARGE 2000
#if arch_small_memory
#  define abuf_nominal abuf_nominal_SMALL
#else
#  define abuf_nominal\
     (gs_debug_c('.') ? abuf_nominal_SMALL : abuf_nominal_LARGE)
#endif

/* Erase the page */
int
gs_erasepage(gs_state * pgs)
{
    /*
     * We can't just fill with device white; we must take the
     * transfer function into account.
     */
    int code;

    if ((code = gs_gsave(pgs)) < 0)
	return code;
    if ((code = gs_setgray(pgs, 1.0)) >= 0) {
	/* Fill the page directly, ignoring clipping. */
	code = gs_fillpage(pgs);
    }
    gs_grestore(pgs);
    return code;
}

/* Fill the page with the current color. */
int
gs_fillpage(gs_state * pgs)
{
    gx_device *dev;
    int code = 0;
    gs_logical_operation_t save_lop;
    bool hl_color_available = gx_hld_is_hl_color_available((gs_imager_state *)pgs, 
						    pgs->dev_color);
    gx_set_dev_color(pgs);
    dev = gs_currentdevice(pgs);
    /* Fill the page directly, ignoring clipping. */
    /* Use the default RasterOp. */
    save_lop = pgs->log_op;
    gs_init_rop(pgs);
    if (hl_color_available) {
	gs_fixed_rect rect;

	rect.p.x = rect.p.y = 0;
	rect.q.x = int2fixed(dev->width);
	rect.q.y = int2fixed(dev->height);
	code = dev_proc(pgs->device, fill_rectangle_hl_color)(pgs->device, 
		&rect, (const gs_imager_state *)pgs, pgs->dev_color, NULL);
    }
    if (!hl_color_available || code == gs_error_rangecheck)
	code = gx_fill_rectangle(0, 0, dev->width, dev->height,
				 pgs->dev_color, pgs);
    pgs->log_op = save_lop;
    if (code < 0)
	return code;
    return (*dev_proc(dev, sync_output)) (dev);
}

/*
 * Determine the number of bits of alpha buffer for a stroke or fill.
 * We should do alpha buffering iff this value is >1.
 */
private int
alpha_buffer_bits(gs_state * pgs)
{
    gx_device *dev;

    if (!color_is_pure(pgs->dev_color))
	return 0;
    dev = gs_currentdevice_inline(pgs);
    if (gs_device_is_abuf(dev)) {
	/* We're already writing into an alpha buffer. */
	return 0;
    }
    return (*dev_proc(dev, get_alpha_bits))
	(dev, (pgs->in_cachedevice ? go_text : go_graphics));
}
/*
 * Set up an alpha buffer for a stroke or fill operation.  Return 0
 * if no buffer could be allocated, 1 if a buffer was installed,
 * or the usual negative error code.
 *
 * The fill/stroke code sets up a clipping device if needed; however,
 * since we scale up all the path coordinates, we either need to scale up
 * the clipping region, or do clipping after, rather than before,
 * alpha buffering.  Either of these is a little inconvenient, but
 * the former is less inconvenient.
 */
private int
scale_paths(gs_state * pgs, int log2_scale_x, int log2_scale_y, bool do_path)
{
    /*
     * Because of clip and clippath, any of path, clip_path, and view_clip
     * may be aliases for each other.  The only reliable way to detect
     * this is by comparing the segments pointers.  Note that we must
     * scale the non-segment parts of the paths even if the segments are
     * aliased.
     */
    const gx_path_segments *seg_clip =
	(pgs->clip_path->path_valid ? pgs->clip_path->path.segments : 0);
    const gx_clip_rect_list *list_clip = pgs->clip_path->rect_list;
    const gx_path_segments *seg_view_clip;
    const gx_clip_rect_list *list_view_clip;
    const gx_path_segments *seg_effective_clip =
	(pgs->effective_clip_path->path_valid ?
	 pgs->effective_clip_path->path.segments : 0);
    const gx_clip_rect_list *list_effective_clip =
	pgs->effective_clip_path->rect_list;

    gx_cpath_scale_exp2_shared(pgs->clip_path, log2_scale_x, log2_scale_y,
			       false, false);
    if (pgs->view_clip != 0 && pgs->view_clip != pgs->clip_path) {
	seg_view_clip =
	    (pgs->view_clip->path_valid ? pgs->view_clip->path.segments : 0);
	list_view_clip = pgs->view_clip->rect_list;
	gx_cpath_scale_exp2_shared(pgs->view_clip, log2_scale_x, log2_scale_y,
				   list_view_clip == list_clip,
				   seg_view_clip && seg_view_clip == seg_clip);
    } else
	seg_view_clip = 0, list_view_clip = 0;
    if (pgs->effective_clip_path != pgs->clip_path &&
	pgs->effective_clip_path != pgs->view_clip
	)
	gx_cpath_scale_exp2_shared(pgs->effective_clip_path, log2_scale_x,
				   log2_scale_y,
				   list_effective_clip == list_clip ||
				   list_effective_clip == list_view_clip,
				   seg_effective_clip &&
				   (seg_effective_clip == seg_clip ||
				    seg_effective_clip == seg_view_clip));
    if (do_path) {
	const gx_path_segments *seg_path = pgs->path->segments;

	gx_path_scale_exp2_shared(pgs->path, log2_scale_x, log2_scale_y,
				  seg_path == seg_clip ||
				  seg_path == seg_view_clip ||
				  seg_path == seg_effective_clip);
    }
    return 0;
}
private void
scale_dash_pattern(gs_state * pgs, floatp scale)
{
    int i;

    for (i = 0; i < pgs->line_params.dash.pattern_size; ++i)
	pgs->line_params.dash.pattern[i] *= scale;
    pgs->line_params.dash.offset *= scale;
    pgs->line_params.dash.pattern_length *= scale;
    pgs->line_params.dash.init_dist_left *= scale;
    if (pgs->line_params.dot_length_absolute)
	pgs->line_params.dot_length *= scale;
}
private int
alpha_buffer_init(gs_state * pgs, fixed extra_x, fixed extra_y, int alpha_bits)
{
    gx_device *dev = gs_currentdevice_inline(pgs);
    int log2_alpha_bits = ilog2(alpha_bits);
    gs_fixed_rect bbox;
    gs_int_rect ibox;
    uint width, raster, band_space;
    uint height;
    gs_log2_scale_point log2_scale;
    gs_memory_t *mem;
    gx_device_memory *mdev;

    log2_scale.x = log2_scale.y = log2_alpha_bits;
    gx_path_bbox(pgs->path, &bbox);
    ibox.p.x = fixed2int(bbox.p.x - extra_x) - 1;
    ibox.p.y = fixed2int(bbox.p.y - extra_y) - 1;
    ibox.q.x = fixed2int_ceiling(bbox.q.x + extra_x) + 1;
    ibox.q.y = fixed2int_ceiling(bbox.q.y + extra_y) + 1;
    width = (ibox.q.x - ibox.p.x) << log2_scale.x;
    raster = bitmap_raster(width);
    band_space = raster << log2_scale.y;
    height = (abuf_nominal / band_space) << log2_scale.y;
    if (height == 0)
	height = 1 << log2_scale.y;
    mem = pgs->memory;
    mdev = gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
			   "alpha_buffer_init");
    if (mdev == 0)
	return 0;		/* if no room, don't buffer */
    gs_make_mem_abuf_device(mdev, mem, dev, &log2_scale,
			    alpha_bits, ibox.p.x << log2_scale.x);
    mdev->width = width;
    mdev->height = height;
    mdev->bitmap_memory = mem;
    if ((*dev_proc(mdev, open_device)) ((gx_device *) mdev) < 0) {
	/* No room for bits, punt. */
	gs_free_object(mem, mdev, "alpha_buffer_init");
	return 0;
    }
    gx_set_device_only(pgs, (gx_device *) mdev);
    scale_paths(pgs, log2_scale.x, log2_scale.y, true);
    return 1;
}

/* Release an alpha buffer. */
private void
alpha_buffer_release(gs_state * pgs, bool newpath)
{
    gx_device_memory *mdev =
    (gx_device_memory *) gs_currentdevice_inline(pgs);

    (*dev_proc(mdev, close_device)) ((gx_device *) mdev);
    scale_paths(pgs, -mdev->log2_scale.x, -mdev->log2_scale.y,
		!(newpath && !gx_path_is_shared(pgs->path)));
    /* Reference counting will free mdev. */
    gx_set_device_only(pgs, mdev->target);
}

/* Fill the current path using a specified rule. */
private int
fill_with_rule(gs_state * pgs, int rule)
{
    int code;

    /* If we're inside a charpath, just merge the current path */
    /* into the parent's path. */
    if (pgs->in_charpath)
	code = gx_path_add_char_path(pgs->show_gstate->path, pgs->path,
				     pgs->in_charpath);
    else if (gs_is_null_device(pgs->device)) {
	/* Handle separately to prevent gs_state_color_load - bug 688308. */
	gs_newpath(pgs);
	code = 0;
    } else {
	int abits, acode;

	gx_set_dev_color(pgs);
	code = gs_state_color_load(pgs);
	if (code < 0)
	    return code;
	abits = alpha_buffer_bits(pgs);
	if (abits > 1) {
	    acode = alpha_buffer_init(pgs, pgs->fill_adjust.x,
				      pgs->fill_adjust.y, abits);
	    if (acode < 0)
		return acode;
	} else
	    acode = 0;
	code = gx_fill_path(pgs->path, pgs->dev_color, pgs, rule,
			    pgs->fill_adjust.x, pgs->fill_adjust.y);
	if (acode > 0)
	    alpha_buffer_release(pgs, code >= 0);
	if (code >= 0)
	    gs_newpath(pgs);

    }
    return code;
}
/* Fill using the winding number rule */
int
gs_fill(gs_state * pgs)
{
    return fill_with_rule(pgs, gx_rule_winding_number);
}
/* Fill using the even/odd rule */
int
gs_eofill(gs_state * pgs)
{
    return fill_with_rule(pgs, gx_rule_even_odd);
}

/* Stroke the current path */
int
gs_stroke(gs_state * pgs)
{
    int code;

    /*
     * If we're inside a charpath, just merge the current path
     * into the parent's path.
     */
    if (pgs->in_charpath) {
	if (pgs->in_charpath == cpm_true_charpath) {
	    /*
	     * A stroke inside a true charpath should do the
	     * equivalent of strokepath.
	     */
	    code = gs_strokepath(pgs);
	    if (code < 0)
		return code;
	}
	code = gx_path_add_char_path(pgs->show_gstate->path, pgs->path,
				     pgs->in_charpath);
    } else if (gs_is_null_device(pgs->device)) {
	/* Handle separately to prevent gs_state_color_load. */
	gs_newpath(pgs);
	code = 0;
    } else {
	int abits, acode;

	gx_set_dev_color(pgs);
	code = gs_state_color_load(pgs);
	if (code < 0)
	    return code;
	abits = alpha_buffer_bits(pgs);
	if (abits > 1) {
	    /*
	     * Expand the bounding box by the line width.
	     * This is expensive to compute, so we only do it
	     * if we know we're going to buffer.
	     */
	    float xxyy = fabs(pgs->ctm.xx) + fabs(pgs->ctm.yy);
	    float xyyx = fabs(pgs->ctm.xy) + fabs(pgs->ctm.yx);
	    float scale = (float)(1 << (abits / 2));
	    float orig_width = gs_currentlinewidth(pgs);
	    float new_width = orig_width * scale;
	    fixed extra_adjust =
		float2fixed(max(xxyy, xyyx) * new_width / 2);
	    float orig_flatness = gs_currentflat(pgs);
	    gx_path spath;

	    /* Scale up the line width, dash pattern, and flatness. */
	    if (extra_adjust < fixed_1)
		extra_adjust = fixed_1;
	    acode = alpha_buffer_init(pgs,
				      pgs->fill_adjust.x + extra_adjust,
				      pgs->fill_adjust.y + extra_adjust,
				      abits);
	    if (acode < 0)
		return acode;
	    gs_setlinewidth(pgs, new_width);
	    scale_dash_pattern(pgs, scale);
	    gs_setflat(pgs, orig_flatness * scale);
	    /*
	     * The alpha-buffer device requires that we fill the
	     * entire path as a single unit.
	     */
	    gx_path_init_local(&spath, pgs->memory);
	    code = gx_stroke_add(pgs->path, &spath, pgs);
	    gs_setlinewidth(pgs, orig_width);
	    scale_dash_pattern(pgs, 1.0 / scale);
	    if (code >= 0)
		code = gx_fill_path(&spath, pgs->dev_color, pgs,
				    gx_rule_winding_number,
				    pgs->fill_adjust.x,
				    pgs->fill_adjust.y);
	    gs_setflat(pgs, orig_flatness);
	    gx_path_free(&spath, "gs_stroke");
	    if (acode > 0)
		alpha_buffer_release(pgs, code >= 0);
	} else
	    code = gx_stroke_fill(pgs->path, pgs);
	if (code >= 0)
	    gs_newpath(pgs);
    }
    return code;
}

/* Compute the stroked outline of the current path */
int
gs_strokepath(gs_state * pgs)
{
    gx_path spath;
    int code;

    gx_path_init_local(&spath, pgs->path->memory);
    code = gx_stroke_add(pgs->path, &spath, pgs);
    if (code < 0) {
	gx_path_free(&spath, "gs_strokepath");
	return code;
    }
    code = gx_path_assign_free(pgs->path, &spath);
    if (code < 0)
	return code;
    gx_setcurrentpoint(pgs, fixed2float(spath.position.x), fixed2float(spath.position.y));
    return 0;
}
