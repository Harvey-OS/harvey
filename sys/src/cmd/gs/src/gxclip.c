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

/* $Id: gxclip.c,v 1.15 2004/06/24 05:03:36 dan Exp $ */
/* Implementation of (path-based) clipping */
#include "gx.h"
#include "gxdevice.h"
#include "gxclip.h"
#include "gxpath.h"
#include "gxcpath.h"

/* Define whether to look for vertical clipping regions. */
#define CHECK_VERTICAL_CLIPPING

/* ------ Rectangle list clipper ------ */

/* Device for clipping with a region. */
/* We forward non-drawing operations, but we must be sure to intercept */
/* all drawing operations. */
private dev_proc_open_device(clip_open);
private dev_proc_fill_rectangle(clip_fill_rectangle);
private dev_proc_copy_mono(clip_copy_mono);
private dev_proc_copy_color(clip_copy_color);
private dev_proc_copy_alpha(clip_copy_alpha);
private dev_proc_fill_mask(clip_fill_mask);
private dev_proc_strip_tile_rectangle(clip_strip_tile_rectangle);
private dev_proc_strip_copy_rop(clip_strip_copy_rop);
private dev_proc_get_clipping_box(clip_get_clipping_box);
private dev_proc_get_bits_rectangle(clip_get_bits_rectangle);

/* The device descriptor. */
private const gx_device_clip gs_clip_device =
{std_device_std_body(gx_device_clip, 0, "clipper",
		     0, 0, 1, 1),
 {clip_open,
  gx_forward_get_initial_matrix,
  gx_default_sync_output,
  gx_default_output_page,
  gx_default_close_device,
  gx_forward_map_rgb_color,
  gx_forward_map_color_rgb,
  clip_fill_rectangle,
  gx_default_tile_rectangle,
  clip_copy_mono,
  clip_copy_color,
  gx_default_draw_line,
  gx_default_get_bits,
  gx_forward_get_params,
  gx_forward_put_params,
  gx_forward_map_cmyk_color,
  gx_forward_get_xfont_procs,
  gx_forward_get_xfont_device,
  gx_forward_map_rgb_alpha_color,
  gx_forward_get_page_device,
  gx_forward_get_alpha_bits,
  clip_copy_alpha,
  gx_forward_get_band,
  gx_default_copy_rop,
  gx_default_fill_path,
  gx_default_stroke_path,
  clip_fill_mask,
  gx_default_fill_trapezoid,
  gx_default_fill_parallelogram,
  gx_default_fill_triangle,
  gx_default_draw_thin_line,
  gx_default_begin_image,
  gx_default_image_data,
  gx_default_end_image,
  clip_strip_tile_rectangle,
  clip_strip_copy_rop,
  clip_get_clipping_box,
  gx_default_begin_typed_image,
  clip_get_bits_rectangle,
  gx_forward_map_color_rgb_alpha,
  gx_no_create_compositor,
  gx_forward_get_hardware_params,
  gx_default_text_begin,
  gx_default_finish_copydevice,
  NULL,			/* begin_transparency_group */
  NULL,			/* end_transparency_group */
  NULL,			/* begin_transparency_mask */
  NULL,			/* end_transparency_mask */
  NULL,			/* discard_transparency_layer */
  gx_forward_get_color_mapping_procs,
  gx_forward_get_color_comp_index,
  gx_forward_encode_color,
  gx_forward_decode_color,
  gx_forward_pattern_manage,
  gx_forward_fill_rectangle_hl_color,
  gx_forward_include_color_space,
  gx_default_fill_linear_color_scanline,
  gx_default_fill_linear_color_trapezoid,
  gx_default_fill_linear_color_triangle,
  gx_forward_update_spot_equivalent_colors
 }
};

/* Make a clipping device. */
void
gx_make_clip_translate_device(gx_device_clip * dev, const gx_clip_list * list,
			      int tx, int ty, gs_memory_t *mem)
{
    gx_device_init((gx_device *)dev, (const gx_device *)&gs_clip_device,
		   mem, true);
    dev->list = *list;
    dev->translation.x = tx;
    dev->translation.y = ty;
}
void
gx_make_clip_path_device(gx_device_clip * dev, const gx_clip_path * pcpath)
{
    gx_make_clip_device(dev, gx_cpath_list(pcpath));
}

/* Define debugging statistics for the clipping loops. */
#ifdef DEBUG
struct stats_clip_s {
    long
         loops, out, in_y, in, in1, down, up, x, no_x;
} stats_clip;

private const uint clip_interval = 10000;

# define INCR(v) (++(stats_clip.v))
# define INCR_THEN(v, e) (INCR(v), (e))
#else
# define INCR(v) DO_NOTHING
# define INCR_THEN(v, e) (e)
#endif

/*
 * Enumerate the rectangles of the x,w,y,h argument that fall within
 * the clipping region.
 */
private int
clip_enumerate_rest(gx_device_clip * rdev,
		    int x, int y, int xe, int ye,
		    int (*process)(clip_callback_data_t * pccd,
				   int xc, int yc, int xec, int yec),
		    clip_callback_data_t * pccd)
{
    gx_clip_rect *rptr = rdev->current;		/* const within algorithm */
    int yc;
    int code;

#ifdef DEBUG
    if (INCR(loops) % clip_interval == 0 && gs_debug_c('q')) {
	dprintf5("[q]loops=%ld out=%ld in_y=%ld in=%ld in1=%ld\n",
		 stats_clip.loops, stats_clip.out, stats_clip.in,
		 stats_clip.in_y, stats_clip.in1);
	dprintf4("[q]   down=%ld up=%ld x=%ld no_x=%ld\n",
		 stats_clip.down, stats_clip.up, stats_clip.x,
		 stats_clip.no_x);
    }
#endif
    pccd->x = x, pccd->y = y;
    pccd->w = xe - x, pccd->h = ye - y;
    /*
     * Warp the cursor forward or backward to the first rectangle row
     * that could include a given y value.  Assumes rptr is set, and
     * updates it.  Specifically, after this loop, either rptr == 0 (if
     * the y value is greater than all y values in the list), or y <
     * rptr->ymax and either rptr->prev == 0 or y >= rptr->prev->ymax.
     * Note that y <= rptr->ymin is possible.
     *
     * In the first case below, the while loop is safe because if there
     * is more than one rectangle, there is a 'stopper' at the end of
     * the list.
     */
    if (y >= rptr->ymax) {
	if ((rptr = rptr->next) != 0)
	    while (INCR_THEN(up, y >= rptr->ymax))
		rptr = rptr->next;
    } else
	while (rptr->prev != 0 && y < rptr->prev->ymax)
	    INCR_THEN(down, rptr = rptr->prev);
    if (rptr == 0 || (yc = rptr->ymin) >= ye) {
	INCR(out);
	if (rdev->list.count > 1)
	    rdev->current =
		(rptr != 0 ? rptr :
		 y >= rdev->current->ymax ? rdev->list.tail :
		 rdev->list.head);
	return 0;
    }
    rdev->current = rptr;
    if (yc < y)
	yc = y;

    do {
	const int ymax = rptr->ymax;
	int yec = min(ymax, ye);

	if_debug2('Q', "[Q]yc=%d yec=%d\n", yc, yec);
	do {
	    int xc = rptr->xmin;
	    int xec = rptr->xmax;

	    if (xc < x)
		xc = x;
	    if (xec > xe)
		xec = xe;
	    if (xec > xc) {
		clip_rect_print('Q', "match", rptr);
		if_debug2('Q', "[Q]xc=%d xec=%d\n", xc, xec);
		INCR(x);
/*
 * Conditionally look ahead to detect unclipped vertical strips.  This is
 * really only valuable for 90 degree rotated images or (nearly-)vertical
 * lines with convex clipping regions; if we ever change images to use
 * source buffering and destination-oriented enumeration, we could probably
 * take out the code here with no adverse effects.
 */
#ifdef CHECK_VERTICAL_CLIPPING
		if (xec - xc == pccd->w) {	/* full width */
		    /* Look ahead for a vertical swath. */
		    while ((rptr = rptr->next) != 0 &&
			   rptr->ymin == yec &&
			   rptr->ymax <= ye &&
			   rptr->xmin <= x &&
			   rptr->xmax >= xe
			   )
			yec = rptr->ymax;
		} else
		    rptr = rptr->next;
#else
		rptr = rptr->next;
#endif
		code = process(pccd, xc, yc, xec, yec);
		if (code < 0)
		    return code;
	    } else {
		INCR_THEN(no_x, rptr = rptr->next);
	    }
	    if (rptr == 0)
		return 0;
	}
	while (rptr->ymax == ymax);
    } while ((yc = rptr->ymin) < ye);
    return 0;
}

private int
clip_enumerate(gx_device_clip * rdev, int x, int y, int w, int h,
	       int (*process)(clip_callback_data_t * pccd,
			      int xc, int yc, int xec, int yec),
	       clip_callback_data_t * pccd)
{
    int xe, ye;
    const gx_clip_rect *rptr = rdev->current;

    if (w <= 0 || h <= 0)
	return 0;
    pccd->tdev = rdev->target;
    x += rdev->translation.x;
    xe = x + w;
    y += rdev->translation.y;
    ye = y + h;
    /* Check for the region being entirely within the current rectangle. */
    if (y >= rptr->ymin && ye <= rptr->ymax &&
	x >= rptr->xmin && xe <= rptr->xmax
	) {
	pccd->x = x, pccd->y = y, pccd->w = w, pccd->h = h;
	return INCR_THEN(in, process(pccd, x, y, xe, ye));
    }
    return clip_enumerate_rest(rdev, x, y, xe, ye, process, pccd);
}

/* Open a clipping device */
private int
clip_open(gx_device * dev)
{
    gx_device_clip *const rdev = (gx_device_clip *) dev;
    gx_device *tdev = rdev->target;

    /* Initialize the cursor. */
    rdev->current =
	(rdev->list.head == 0 ? &rdev->list.single : rdev->list.head);
    rdev->color_info = tdev->color_info;
    rdev->cached_colors = tdev->cached_colors;
    rdev->width = tdev->width;
    rdev->height = tdev->height;
    gx_device_copy_color_procs(dev, tdev);
    rdev->clipping_box_set = false;
    rdev->memory = tdev->memory;
    return 0;
}

/* Fill a rectangle */
int
clip_call_fill_rectangle(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, fill_rectangle))
	(pccd->tdev, xc, yc, xec - xc, yec - yc, pccd->color[0]);
}
private int
clip_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		    gx_color_index color)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;
    /* We handle the fastest cases in-line here. */
    gx_device *tdev = rdev->target;
    /*const*/ gx_clip_rect *rptr = rdev->current;
    int xe, ye;

    if (w <= 0 || h <= 0)
	return 0;
    x += rdev->translation.x;
    xe = x + w;
    y += rdev->translation.y;
    ye = y + h;
    /* We open-code the most common cases here. */
    if ((y >= rptr->ymin && ye <= rptr->ymax) ||
	((rptr = rptr->next) != 0 &&
	 y >= rptr->ymin && ye <= rptr->ymax)
	) {
	rdev->current = rptr;	/* may be redundant, but awkward to avoid */
	INCR(in_y);
	if (x >= rptr->xmin && xe <= rptr->xmax) {
	    INCR(in);
	    return dev_proc(tdev, fill_rectangle)(tdev, x, y, w, h, color);
	}
	else if ((rptr->prev == 0 || rptr->prev->ymax != rptr->ymax) &&
		 (rptr->next == 0 || rptr->next->ymax != rptr->ymax)
		 ) {
	    INCR(in1);
	    if (x < rptr->xmin)
		x = rptr->xmin;
	    if (xe > rptr->xmax)
		xe = rptr->xmax;
	    return
		(x >= xe ? 0 :
		 dev_proc(tdev, fill_rectangle)(tdev, x, y, xe - x, h, color));
	}
    }
    ccdata.tdev = tdev;
    ccdata.color[0] = color;
    return clip_enumerate_rest(rdev, x, y, xe, ye,
			       clip_call_fill_rectangle, &ccdata);
}

/* Copy a monochrome rectangle */
int
clip_call_copy_mono(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, copy_mono))
	(pccd->tdev, pccd->data + (yc - pccd->y) * pccd->raster,
	 pccd->sourcex + xc - pccd->x, pccd->raster, gx_no_bitmap_id,
	 xc, yc, xec - xc, yec - yc, pccd->color[0], pccd->color[1]);
}
private int
clip_copy_mono(gx_device * dev,
	       const byte * data, int sourcex, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h,
	       gx_color_index color0, gx_color_index color1)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;
    /* We handle the fastest case in-line here. */
    gx_device *tdev = rdev->target;
    const gx_clip_rect *rptr = rdev->current;
    int xe, ye;

    if (w <= 0 || h <= 0)
	return 0;
    x += rdev->translation.x;
    xe = x + w;
    y += rdev->translation.y;
    ye = y + h;
    if (y >= rptr->ymin && ye <= rptr->ymax) {
	INCR(in_y);
	if (x >= rptr->xmin && xe <= rptr->xmax) {
	    INCR(in);
	    return dev_proc(tdev, copy_mono)
		(tdev, data, sourcex, raster, id, x, y, w, h, color0, color1);
	}
    }
    ccdata.tdev = tdev;
    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.color[0] = color0, ccdata.color[1] = color1;
    return clip_enumerate_rest(rdev, x, y, xe, ye,
			       clip_call_copy_mono, &ccdata);
}

/* Copy a color rectangle */
int
clip_call_copy_color(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, copy_color))
	(pccd->tdev, pccd->data + (yc - pccd->y) * pccd->raster,
	 pccd->sourcex + xc - pccd->x, pccd->raster, gx_no_bitmap_id,
	 xc, yc, xec - xc, yec - yc);
}
private int
clip_copy_color(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		int x, int y, int w, int h)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    return clip_enumerate(rdev, x, y, w, h, clip_call_copy_color, &ccdata);
}

/* Copy a rectangle with alpha */
int
clip_call_copy_alpha(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, copy_alpha))
	(pccd->tdev, pccd->data + (yc - pccd->y) * pccd->raster,
	 pccd->sourcex + xc - pccd->x, pccd->raster, gx_no_bitmap_id,
	 xc, yc, xec - xc, yec - yc, pccd->color[0], pccd->depth);
}
private int
clip_copy_alpha(gx_device * dev,
		const byte * data, int sourcex, int raster, gx_bitmap_id id,
		int x, int y, int w, int h,
		gx_color_index color, int depth)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.color[0] = color, ccdata.depth = depth;
    return clip_enumerate(rdev, x, y, w, h, clip_call_copy_alpha, &ccdata);
}

/* Fill a region defined by a mask. */
int
clip_call_fill_mask(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, fill_mask))
	(pccd->tdev, pccd->data + (yc - pccd->y) * pccd->raster,
	 pccd->sourcex + xc - pccd->x, pccd->raster, gx_no_bitmap_id,
	 xc, yc, xec - xc, yec - yc, pccd->pdcolor, pccd->depth,
	 pccd->lop, NULL);
}
private int
clip_fill_mask(gx_device * dev,
	       const byte * data, int sourcex, int raster, gx_bitmap_id id,
	       int x, int y, int w, int h,
	       const gx_drawing_color * pdcolor, int depth,
	       gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;

    if (pcpath != 0)
	return gx_default_fill_mask(dev, data, sourcex, raster, id,
				    x, y, w, h, pdcolor, depth, lop,
				    pcpath);
    ccdata.data = data, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.pdcolor = pdcolor, ccdata.depth = depth, ccdata.lop = lop;
    return clip_enumerate(rdev, x, y, w, h, clip_call_fill_mask, &ccdata);
}

/* Strip-tile a rectangle. */
int
clip_call_strip_tile_rectangle(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, strip_tile_rectangle))
	(pccd->tdev, pccd->tiles, xc, yc, xec - xc, yec - yc,
	 pccd->color[0], pccd->color[1], pccd->phase.x, pccd->phase.y);
}
private int
clip_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
			  int x, int y, int w, int h,
     gx_color_index color0, gx_color_index color1, int phase_x, int phase_y)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.tiles = tiles;
    ccdata.color[0] = color0, ccdata.color[1] = color1;
    ccdata.phase.x = phase_x, ccdata.phase.y = phase_y;
    return clip_enumerate(rdev, x, y, w, h, clip_call_strip_tile_rectangle, &ccdata);
}

/* Copy a rectangle with RasterOp and strip texture. */
int
clip_call_strip_copy_rop(clip_callback_data_t * pccd, int xc, int yc, int xec, int yec)
{
    return (*dev_proc(pccd->tdev, strip_copy_rop))
	(pccd->tdev, pccd->data + (yc - pccd->y) * pccd->raster,
	 pccd->sourcex + xc - pccd->x, pccd->raster, gx_no_bitmap_id,
	 pccd->scolors, pccd->textures, pccd->tcolors,
	 xc, yc, xec - xc, yec - yc, pccd->phase.x, pccd->phase.y,
	 pccd->lop);
}
private int
clip_strip_copy_rop(gx_device * dev,
	      const byte * sdata, int sourcex, uint raster, gx_bitmap_id id,
		    const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
		    int x, int y, int w, int h,
		    int phase_x, int phase_y, gs_logical_operation_t lop)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    clip_callback_data_t ccdata;

    ccdata.data = sdata, ccdata.sourcex = sourcex, ccdata.raster = raster;
    ccdata.scolors = scolors, ccdata.textures = textures,
	ccdata.tcolors = tcolors;
    ccdata.phase.x = phase_x, ccdata.phase.y = phase_y, ccdata.lop = lop;
    return clip_enumerate(rdev, x, y, w, h, clip_call_strip_copy_rop, &ccdata);
}

/* Get the (outer) clipping box, in client coordinates. */
private void
clip_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    gx_device_clip *const rdev = (gx_device_clip *) dev;

    if (!rdev->clipping_box_set) {
	gx_device *tdev = rdev->target;
	gs_fixed_rect tbox;

	(*dev_proc(tdev, get_clipping_box)) (tdev, &tbox);
	if (rdev->list.count != 0) {
	    gs_fixed_rect cbox;

	    if (rdev->list.count == 1) {
		cbox.p.x = int2fixed(rdev->list.single.xmin);
		cbox.p.y = int2fixed(rdev->list.single.ymin);
		cbox.q.x = int2fixed(rdev->list.single.xmax);
		cbox.q.y = int2fixed(rdev->list.single.ymax);
	    } else {
		/* The head and tail elements are dummies.... */
		cbox.p.x = int2fixed(rdev->list.xmin);
		cbox.p.y = int2fixed(rdev->list.head->next->ymin);
		cbox.q.x = int2fixed(rdev->list.xmax);
		cbox.q.y = int2fixed(rdev->list.tail->prev->ymax);
	    }
	    rect_intersect(tbox, cbox);
	}
	if (rdev->translation.x | rdev->translation.y) {
	    fixed tx = int2fixed(rdev->translation.x),
		ty = int2fixed(rdev->translation.y);

	    if (tbox.p.x != min_fixed)
		tbox.p.x -= tx;
	    if (tbox.p.y != min_fixed)
		tbox.p.y -= ty;
	    if (tbox.q.x != max_fixed)
		tbox.q.x -= tx;
	    if (tbox.q.y != max_fixed)
		tbox.q.y -= ty;
	}
	rdev->clipping_box = tbox;
	rdev->clipping_box_set = true;
    }
    *pbox = rdev->clipping_box;
}

/* Get bits back from the device. */
private int
clip_get_bits_rectangle(gx_device * dev, const gs_int_rect * prect,
			gs_get_bits_params_t * params, gs_int_rect ** unread)
{
    gx_device_clip *rdev = (gx_device_clip *) dev;
    gx_device *tdev = rdev->target;
    int tx = rdev->translation.x, ty = rdev->translation.y;
    gs_int_rect rect;
    int code;

    rect.p.x = prect->p.x - tx, rect.p.y = prect->p.y - ty;
    rect.q.x = prect->q.x - tx, rect.q.y = prect->q.y - ty;
    code = (*dev_proc(tdev, get_bits_rectangle))
	(tdev, &rect, params, unread);
    if (code > 0) {
	/* Adjust unread rectangle coordinates */
	gs_int_rect *list = *unread;
	int i;

	for (i = 0; i < code; ++list, ++i) {
	    list->p.x += tx, list->p.y += ty;
	    list->q.x += tx, list->q.y += ty;
	}
    }
    return code;
}
