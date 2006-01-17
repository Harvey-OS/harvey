/* Copyright (C) 1991, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxclread.c,v 1.13 2004/10/07 05:18:34 ray Exp $ */
/* Command list reading for Ghostscript. */
#include "memory_.h"
#include "gx.h"
#include "gp.h"			/* for gp_fmode_rb */
#include "gpcheck.h"
#include "gserrors.h"
#include "gxdevice.h"
#include "gscoord.h"		/* requires gsmatrix.h */
#include "gsdevice.h"		/* for gs_deviceinitialmatrix */
#include "gxdevmem.h"		/* must precede gxcldev.h */
#include "gxcldev.h"
#include "gxgetbit.h"
#include "gxhttile.h"
#include "gdevplnx.h"
/*
 * We really don't like the fact that gdevprn.h is included here, since
 * command lists are supposed to be usable for purposes other than printer
 * devices; but gdev_prn_colors_used and gdev_create_buf_device are
 * currently only applicable to printer devices.
 */
#include "gdevprn.h"
#include "stream.h"
#include "strimpl.h"

/* ------ Band file reading stream ------ */

/*
 * To separate banding per se from command list interpretation,
 * we make the command list interpreter simply read from a stream.
 * When we are actually doing banding, the stream filters the band file
 * and only passes through the commands for the current bands (or band
 * ranges that include a current band).
 */
typedef struct stream_band_read_state_s {
    stream_state_common;
    gx_band_page_info_t page_info;
    int band_first, band_last;
    uint left;			/* amount of data left in this run */
    cmd_block b_this;
} stream_band_read_state;

private int
s_band_read_init(stream_state * st)
{
    stream_band_read_state *const ss = (stream_band_read_state *) st;

    ss->left = 0;
    ss->b_this.band_min = 0;
    ss->b_this.band_max = 0;
    ss->b_this.pos = 0;
    clist_rewind(ss->page_bfile, false, ss->page_bfname);
    return 0;
}

private int
s_band_read_process(stream_state * st, stream_cursor_read * ignore_pr,
		    stream_cursor_write * pw, bool last)
{
    stream_band_read_state *const ss = (stream_band_read_state *) st;
    register byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    clist_file_ptr cfile = ss->page_cfile;
    clist_file_ptr bfile = ss->page_bfile;
    uint left = ss->left;
    int status = 1;
    uint count;

    while ((count = wlimit - q) != 0) {
	if (left) {		/* Read more data for the current run. */
	    if (count > left)
		count = left;
	    clist_fread_chars(q + 1, count, cfile);
	    if (clist_ferror_code(cfile) < 0) {
		status = ERRC;
		break;
	    }
	    q += count;
	    left -= count;
	    process_interrupts(st->memory);
	    continue;
	}
rb:
	/*
	 * Scan for the next run for the current bands (or a band range
	 * that includes a current band).
	 */
	if (ss->b_this.band_min == cmd_band_end &&
	    clist_ftell(bfile) == ss->page_bfile_end_pos
	    ) {
	    status = EOFC;
	    break;
	} {
	    int bmin = ss->b_this.band_min;
	    int bmax = ss->b_this.band_max;
	    long pos = ss->b_this.pos;

	    clist_fread_chars(&ss->b_this, sizeof(ss->b_this), bfile);
	    if (!(ss->band_last >= bmin && ss->band_first <= bmax))
		goto rb;
	    clist_fseek(cfile, pos, SEEK_SET, ss->page_cfname);
	    left = (uint) (ss->b_this.pos - pos);
	    if_debug5('l', "[l]reading for bands (%d,%d) at bfile %ld, cfile %ld, length %u\n",
		      bmin, bmax,
		      clist_ftell(bfile) - 2 * sizeof(ss->b_this),
		      pos, left);
	}
    }
    pw->ptr = q;
    ss->left = left;
    return status;
}

/* Stream template */
private const stream_template s_band_read_template = {
    &st_stream_state, s_band_read_init, s_band_read_process, 1, cbuf_size
};


/* ------ Reading/rendering ------ */

/* Forward references */

private int clist_render_init(gx_device_clist *);
private int clist_playback_file_bands(clist_playback_action action,
				      gx_device_clist_reader *cdev,
				      gx_band_page_info_t *page_info,
				      gx_device *target,
				      int band_first, int band_last,
				      int x0, int y0);
private int clist_rasterize_lines(gx_device *dev, int y, int lineCount,
				  gx_device *bdev,
				  const gx_render_plane_t *render_plane,
				  int *pmy);

/* Calculate the raster for a chunky or planar device. */
private int
clist_plane_raster(const gx_device *dev, const gx_render_plane_t *render_plane)
{
    return bitmap_raster(dev->width *
			 (render_plane && render_plane->index >= 0 ?
			  render_plane->depth : dev->color_info.depth));
}

/* Select full-pixel rendering if required for RasterOp. */
private void
clist_select_render_plane(gx_device *dev, int y, int height,
			  gx_render_plane_t *render_plane, int index)
{
    if (index >= 0) {
	gx_colors_used_t colors_used;
	int ignore_start;

	gdev_prn_colors_used(dev, y, height, &colors_used,  &ignore_start);
	if (colors_used.slow_rop)
	    index = -1;
    }
    if (index < 0)
	render_plane->index = index;
    else
	gx_render_plane_init(render_plane, dev, index);
}

/*
 * Do device setup from params stored in command list. This is only for
 * async rendering & assumes that the first command in every command list
 * is a put_params command which sets all space-related parameters to the
 * value they will have for the duration of that command list.
 */
int
clist_setup_params(gx_device *dev)
{
    gx_device_clist_reader * const crdev =
	&((gx_device_clist *)dev)->reader;
    int code = clist_render_init((gx_device_clist *)dev);
    if (code < 0)
	return code;

    code = clist_playback_file_bands(playback_action_setup,
				     crdev, &crdev->page_info, 0, 0, 0, 0, 0);

    /* put_params may have reinitialized device into a writer */
    clist_render_init((gx_device_clist *)dev);

    return code;
}

/* Copy a rasterized rectangle to the client, rasterizing if needed. */
int
clist_get_bits_rectangle(gx_device *dev, const gs_int_rect * prect,
			 gs_get_bits_params_t *params, gs_int_rect **unread)
{
    gx_device_clist_common *cdev = (gx_device_clist_common *)dev;
    gs_get_bits_options_t options = params->options;
    int y = prect->p.y;
    int end_y = prect->q.y;
    int line_count = end_y - y;
    gs_int_rect band_rect;
    int lines_rasterized;
    gx_device *bdev;
    int num_planes =
	(options & GB_PACKING_CHUNKY ? 1 :
	 options & GB_PACKING_PLANAR ? dev->color_info.num_components :
	 options & GB_PACKING_BIT_PLANAR ? dev->color_info.depth :
	 0 /****** NOT POSSIBLE ******/);
    gx_render_plane_t render_plane;
    int plane_index;
    int my;
    int code;

    if (prect->p.x < 0 || prect->q.x > dev->width ||
	y < 0 || end_y > dev->height
	)
	return_error(gs_error_rangecheck);
    if (line_count <= 0 || prect->p.x >= prect->q.x)
	return 0;

    /*
     * Calculate the render_plane from the params.  There are two cases:
     * full pixels, or a single plane.
     */
    plane_index = -1;
    if (options & GB_SELECT_PLANES) {
	/* Look for the one selected plane. */
	int i;

	for (i = 0; i < num_planes; ++i)
	    if (params->data[i]) {
		if (plane_index >= 0)  /* >1 plane requested */
		    return gx_default_get_bits_rectangle(dev, prect, params,
							 unread);
		plane_index = i;
	    }
    }
    clist_select_render_plane(dev, y, line_count, &render_plane, plane_index);
    code = gdev_create_buf_device(cdev->buf_procs.create_buf_device,
				  &bdev, cdev->target, &render_plane,
				  dev->memory, true);
    if (code < 0)
	return code;
    code = clist_rasterize_lines(dev, y, line_count, bdev, &render_plane, &my);
    if (code < 0)
	return code;
    lines_rasterized = min(code, line_count);
    /* Return as much of the rectangle as falls within the rasterized lines. */
    band_rect = *prect;
    band_rect.p.y = my;
    band_rect.q.y = my + lines_rasterized;
    code = dev_proc(bdev, get_bits_rectangle)
	(bdev, &band_rect, params, unread);
    cdev->buf_procs.destroy_buf_device(bdev);
    if (code < 0 || lines_rasterized == line_count)
	return code;
    /*
     * We'll have to return the rectangle in pieces.  Force GB_RETURN_COPY
     * rather than GB_RETURN_POINTER, and require all subsequent pieces to
     * use the same values as the first piece for all of the other format
     * options.  If copying isn't allowed, or if there are any unread
     * rectangles, punt.
     */
    if (!(options & GB_RETURN_COPY) || code > 0)
	return gx_default_get_bits_rectangle(dev, prect, params, unread);
    options = params->options;
    if (!(options & GB_RETURN_COPY)) {
	/* Redo the first piece with copying. */
	params->options = options =
	    (params->options & ~GB_RETURN_ALL) | GB_RETURN_COPY;
	lines_rasterized = 0;
    }
    {
	gs_get_bits_params_t band_params;
	uint raster = gx_device_raster(bdev, true);

	code = gdev_create_buf_device(cdev->buf_procs.create_buf_device,
				      &bdev, cdev->target, &render_plane,
				      dev->memory, true);
	if (code < 0)
	    return code;
	band_params = *params;
	while ((y += lines_rasterized) < end_y) {
	    int i;

	    /* Increment data pointers by lines_rasterized. */
	    for (i = 0; i < num_planes; ++i)
		if (band_params.data[i])
		    band_params.data[i] += raster * lines_rasterized;
	    line_count = end_y - y;
	    code = clist_rasterize_lines(dev, y, line_count, bdev,
					 &render_plane, &my);
	    if (code < 0)
		break;
	    lines_rasterized = min(code, line_count);
	    band_rect.p.y = my;
	    band_rect.q.y = my + lines_rasterized;
	    code = dev_proc(bdev, get_bits_rectangle)
		(bdev, &band_rect, &band_params, unread);
	    if (code < 0)
		break;
	    params->options = options = band_params.options;
	    if (lines_rasterized == line_count)
		break;
	}
	cdev->buf_procs.destroy_buf_device(bdev);
    }
    return code;
}

/* Copy scan lines to the client.  This is where rendering gets done. */
/* Processes min(requested # lines, # lines available thru end of band) */
private int	/* returns -ve error code, or # scan lines copied */
clist_rasterize_lines(gx_device *dev, int y, int line_count,
		      gx_device *bdev, const gx_render_plane_t *render_plane,
		      int *pmy)
{
    gx_device_clist * const cdev = (gx_device_clist *)dev;
    gx_device_clist_reader * const crdev = &cdev->reader;
    gx_device *target = crdev->target;
    uint raster = clist_plane_raster(target, render_plane);
    byte *mdata = crdev->data + crdev->page_tile_cache_size;
    int plane_index = (render_plane ? render_plane->index : -1);
    int code;

    /* Render a band if necessary, and copy it incrementally. */
    if (crdev->ymin < 0 || crdev->yplane.index != plane_index ||
	!(y >= crdev->ymin && y < crdev->ymax)
	) {
	int band_height = crdev->page_band_height;
	int band = y / band_height;
	int band_begin_line = band * band_height;
	int band_end_line = band_begin_line + band_height;
	int band_num_lines;
	gs_int_rect band_rect;

	if (band_end_line > dev->height)
	    band_end_line = dev->height;
	/* Clip line_count to current band */
	if (line_count > band_end_line - y)
	    line_count = band_end_line - y;
	band_num_lines = band_end_line - band_begin_line;

	if (y < 0 || y > dev->height)
	    return_error(gs_error_rangecheck);
	code = crdev->buf_procs.setup_buf_device
	    (bdev, mdata, raster, NULL, 0, band_num_lines, band_num_lines);
	band_rect.p.x = 0;
	band_rect.p.y = band_begin_line;
	band_rect.q.x = dev->width;
	band_rect.q.y = band_end_line;
	if (code >= 0)
	    code = clist_render_rectangle(cdev, &band_rect, bdev, render_plane,
					  true);
	/* Reset the band boundaries now, so that we don't get */
	/* an infinite loop. */
	crdev->ymin = band_begin_line;
	crdev->ymax = band_end_line;
	if (code < 0)
	    return code;
    }

    if (line_count > crdev->ymax - y)
	line_count = crdev->ymax - y;
    code = crdev->buf_procs.setup_buf_device
	(bdev, mdata, raster, NULL, y - crdev->ymin, line_count,
	 crdev->ymax - crdev->ymin);
    if (code < 0)
	return code;

    *pmy = 0;
    return line_count;
}

/* Initialize for reading. */
private int
clist_render_init(gx_device_clist *dev)
{
    gx_device_clist_reader * const crdev = &dev->reader;

    crdev->ymin = crdev->ymax = 0;
    crdev->yplane.index = -1;
    /* For normal rasterizing, pages and num_pages are zero. */
    crdev->pages = 0;
    crdev->num_pages = 0;
    return 0;
}

/*
 * Render a rectangle to a client-supplied device.  There is no necessary
 * relationship between band boundaries and the region being rendered.
 */
int
clist_render_rectangle(gx_device_clist *cdev, const gs_int_rect *prect,
		       gx_device *bdev,
		       const gx_render_plane_t *render_plane, bool clear)
{
    gx_device_clist_reader * const crdev = &cdev->reader;
    const gx_placed_page *ppages;
    int num_pages = crdev->num_pages;
    int band_height = crdev->page_band_height;
    int band_first = prect->p.y / band_height;
    int band_last = (prect->q.y - 1) / band_height;
    gx_saved_page current_page;
    gx_placed_page placed_page;
    int code = 0;
    int i;

    /* Initialize for rendering if we haven't done so yet. */
    if (crdev->ymin < 0) {
	code = clist_end_page(&cdev->writer);
	if (code < 0)
	    return code;
	code = clist_render_init(cdev);
	if (code < 0)
	    return code;
    }
    if (render_plane)
	crdev->yplane = *render_plane;
    else
	crdev->yplane.index = -1;
    if_debug2('l', "[l]rendering bands (%d,%d)\n", band_first, band_last);
    if (clear)
	dev_proc(bdev, fill_rectangle)
	    (bdev, 0, 0, bdev->width, bdev->height, gx_device_white(bdev));

    /*
     * If we aren't rendering saved pages, do the current one.
     * Note that this is the only case in which we may encounter
     * a gx_saved_page with non-zero cfile or bfile.
     */
    ppages = crdev->pages;
    if (ppages == 0) {
	current_page.info = crdev->page_info;
	placed_page.page = &current_page;
	placed_page.offset.x = placed_page.offset.y = 0;
	ppages = &placed_page;
	num_pages = 1;
    }
    for (i = 0; i < num_pages && code >= 0; ++i) {
	const gx_placed_page *ppage = &ppages[i];

	code = clist_playback_file_bands(playback_action_render,
					 crdev, &ppage->page->info,
					 bdev, band_first, band_last,
					 prect->p.x - ppage->offset.x,
					 prect->p.y);
    }
    return code;
}

/* Playback the band file, taking the indicated action w/ its contents. */
private int
clist_playback_file_bands(clist_playback_action action, 
			  gx_device_clist_reader *cdev,
			  gx_band_page_info_t *page_info, gx_device *target,
			  int band_first, int band_last, int x0, int y0)
{
    int code = 0;
    bool opened_bfile = false;
    bool opened_cfile = false;

    /* We have to pick some allocator for rendering.... */
    gs_memory_t *mem =cdev->memory;
 
    stream_band_read_state rs;

    /* setup stream */
    s_init_state((stream_state *)&rs, &s_band_read_template,
		 (gs_memory_t *)0);
    rs.band_first = band_first;
    rs.band_last = band_last;
    rs.page_info = *page_info;

    /* If this is a saved page, open the files. */
    if (rs.page_cfile == 0) {
	code = clist_fopen(rs.page_cfname,
			   gp_fmode_rb, &rs.page_cfile, cdev->bandlist_memory,
			   cdev->bandlist_memory, true);
	opened_cfile = (code >= 0);
    }
    if (rs.page_bfile == 0 && code >= 0) {
	code = clist_fopen(rs.page_bfname,
			   gp_fmode_rb, &rs.page_bfile, cdev->bandlist_memory,
			   cdev->bandlist_memory, false);
	opened_bfile = (code >= 0);
    }
    if (rs.page_cfile != 0 && rs.page_bfile != 0) {
	stream s;
	byte sbuf[cbuf_size];
	static const stream_procs no_procs = {
	    s_std_noavailable, s_std_noseek, s_std_read_reset,
	    s_std_read_flush, s_std_close, s_band_read_process
	};

	s_band_read_init((stream_state *)&rs);
	  /* The stream doesn't need a memory, but we'll need to access s.memory->gs_lib_ctx. */
	s_init(&s, mem);
	s_std_init(&s, sbuf, cbuf_size, &no_procs, s_mode_read);
	s.foreign = 1;
	s.state = (stream_state *)&rs;
	code = clist_playback_band(action, cdev, &s, target, x0, y0, mem);
    }

    /* Close the files if we just opened them. */
    if (opened_bfile && rs.page_bfile != 0)
	clist_fclose(rs.page_bfile, rs.page_bfname, false);
    if (opened_cfile && rs.page_cfile != 0)
	clist_fclose(rs.page_cfile, rs.page_cfname, false);

    return code;
}
