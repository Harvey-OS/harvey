/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gspaint.c */
/* Painting procedures for Ghostscript library */
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxmatrix.h"			/* for gs_state */
#include "gspaint.h"
#include "gzpath.h"
#include "gxpaint.h"
#include "gzstate.h"
#include "gxcpath.h"
#include "gxdevmem.h"
#include "gximage.h"

/* Define the nominal size for alpha buffers. */
#if !arch_ints_are_short	/*** Small memory machines ***/
#  define abuf_nominal 500
#else				/*** Big memory machines ***/
#  define abuf_nominal 2000
#endif

/* Erase the page */
int
gs_erasepage(gs_state *pgs)
{	/* We can't just fill with device white; we must take the */
	/* transfer function into account. */
	int code;
	if ( (code = gs_gsave(pgs)) < 0 )
	  return code;
	if ( (code = gs_setgray(pgs, 1.0)) >= 0 )
	{	/* Fill the page directly, ignoring clipping. */
		code = gs_fillpage(pgs);
	}
	gs_grestore(pgs);
	return code;
}

/* Fill the page with the current color. */
int
gs_fillpage(gs_state *pgs)
{	gx_device *dev;
	int code;
	gx_set_dev_color(pgs);
	dev = gs_currentdevice(pgs);
	/* Fill the page directly, ignoring clipping. */
	code = gx_fill_rectangle(0, 0, dev->width, dev->height,
				 pgs->dev_color, pgs);
	if ( code < 0 )
	  return code;
	return (*dev_proc(dev, sync_output))(dev);
}

/*
 * If appropriate, set up an alpha buffer for a stroke or fill operation.
 * Return 0 if no buffer was required, 1 if a buffer was installed,
 * or the usual negative error code.
 *
 * The fill/stroke code sets up a clipping device if needed; however,
 * since we scale up all the path coordinates, we either need to scale up
 * the clipping region, or do clipping after, rather than before,
 * alpha buffering.  Either of these is a little inconvenient, but
 * the former is less inconvenient.
 */
private int near
alpha_buffer_init(gs_state *pgs, fixed extra, fixed *pmask)
{	gx_device *dev;
	int alpha_bits;
	int log2_alpha_bits;
	gs_fixed_rect bbox;
	gs_int_rect ibox;
	uint width, raster, band_space;
	uint height;
	gs_log2_scale_point log2_scale;
	gs_memory_t *mem;
	gx_device_memory *mdev;

	if ( !color_is_pure(pgs->dev_color) )
	  return 0;
	dev = gs_currentdevice_inline(pgs);
	alpha_bits = (*dev_proc(dev, get_alpha_bits))(dev, go_graphics);
	if ( alpha_bits <= 1 )
	  return 0;
	log2_alpha_bits = alpha_bits >> 1;	/* works for 1,2,4 */
	log2_scale.x = log2_scale.y = log2_alpha_bits;
	gx_path_bbox(pgs->path, &bbox);
	ibox.p.x = fixed2int(bbox.p.x - extra) - 1;
	ibox.p.y = fixed2int(bbox.p.y - extra) - 1;
	ibox.q.x = fixed2int_ceiling(bbox.q.x + extra) + 1;
	ibox.q.y = fixed2int_ceiling(bbox.q.y + extra) + 1;
	width = (ibox.q.x - ibox.p.x) << log2_scale.x;
	raster = bitmap_raster(width);
	band_space = raster << log2_scale.y;
	height = (abuf_nominal / band_space) << log2_scale.y;
	if ( height == 0 )
	  height = 1 << log2_scale.y;
	mem = pgs->memory;
	mdev = gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
			       "alpha_buffer_init");
	if ( mdev == 0 )
	  return 0;		/* if no room, don't buffer */
	gs_make_mem_abuf_device(mdev, mem, dev, &log2_scale,
				alpha_bits, ibox.p.x << log2_scale.x);
	mdev->width = width;
	mdev->height = height;
	mdev->bitmap_memory = mem;
	if ( (*dev_proc(mdev, open_device))((gx_device *)mdev) < 0 )
	  {	/* No room for bits, punt. */
		gs_free_object(mem, mdev, "alpha_buffer_init");
		return 0;
	  }
	gx_set_device_only(pgs, (gx_device *)mdev);
	gx_path_scale_exp2(pgs->path, log2_scale.x, log2_scale.y);
	gx_cpath_scale_exp2(pgs->clip_path, log2_scale.x, log2_scale.y);
	*pmask = int2fixed(-1 << log2_scale.y);
	return 1;
}

/* Release an alpha buffer. */
private void near
alpha_buffer_release(gs_state *pgs, bool newpath)
{	gx_device_memory *mdev =
	  (gx_device_memory *)gs_currentdevice_inline(pgs);
	gx_device *target = mdev->target;

	(*dev_proc(mdev, close_device))((gx_device *)mdev);
	gs_free_object(mdev->memory, mdev, "alpha_buffer_release");
	gx_set_device_only(pgs, target);
	gx_cpath_scale_exp2(pgs->clip_path,
			    -mdev->log2_scale.x, -mdev->log2_scale.y);
	if ( !(newpath && !pgs->path->shares_segments) )
	  gx_path_scale_exp2(pgs->path,
			     -mdev->log2_scale.x, -mdev->log2_scale.y);
}

/* Fill the current path using a specified rule. */
private int near
fill_with_rule(gs_state *pgs, int rule)
{	int code;
	/* If we're inside a charpath, just merge the current path */
	/* into the parent's path. */
	if ( pgs->in_charpath )
	  code = gx_path_add_path(pgs->show_gstate->path, pgs->path);
	else
	{	int acode;
		fixed band_mask = fill_path_no_band_mask;
		gx_set_dev_color(pgs);
		code = gx_color_load(pgs->dev_color, pgs);
		if ( code < 0 )
		  return code;
		acode = alpha_buffer_init(pgs, pgs->fill_adjust, &band_mask);
		if ( acode < 0 )
		  return acode;
		code = gx_fill_path_banded(pgs->path, pgs->dev_color, pgs,
					   rule, pgs->fill_adjust, band_mask);
		if ( acode > 0 )
		  alpha_buffer_release(pgs, code >= 0);
		if ( code >= 0 )
		  gs_newpath(pgs);
		
	}
	return code;
}
/* Fill using the winding number rule */
int
gs_fill(gs_state *pgs)
{	return fill_with_rule(pgs, gx_rule_winding_number);
}
/* Fill using the even/odd rule */
int
gs_eofill(gs_state *pgs)
{	return fill_with_rule(pgs, gx_rule_even_odd);
}

/* Stroke the current path */
int
gs_stroke(gs_state *pgs)
{	int code;
	/* If we're inside a charpath, just merge the current path */
	/* into the parent's path. */
	if ( pgs->in_charpath )
	  code = gx_path_add_path(pgs->show_gstate->path, pgs->path);
	else
	{	int acode;
		fixed band_mask = fill_path_no_band_mask;
		gx_set_dev_color(pgs);
		code = gx_color_load(pgs->dev_color, pgs);
		if ( code < 0 )
		  return code;
		/****** pgs->fill_adjust IS WRONG, NEED TO ADD ******/
		/****** LINE WIDTH ******/
		acode = alpha_buffer_init(pgs, pgs->fill_adjust, &band_mask);
		if ( acode < 0 )
		  return acode;
		code = gx_stroke_fill(pgs->path, pgs);
		if ( acode > 0 )
		  alpha_buffer_release(pgs, code >= 0);
		if ( code >= 0 )
		  gs_newpath(pgs);
	}
	return code;
}

/* Compute the stroked outline of the current path */
int
gs_strokepath(gs_state *pgs)
{	gx_path spath;
	int code;
	gx_path_init(&spath, pgs->memory);
	code = gx_stroke_add(pgs->path, &spath, pgs);
	if ( code < 0 )
	  return code;
	gx_path_release(pgs->path);
	*pgs->path = spath;
	return 0;
}
