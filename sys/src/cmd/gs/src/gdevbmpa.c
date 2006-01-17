/* Copyright (C) 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevbmpa.c,v 1.6 2002/06/16 05:48:54 lpd Exp $ */
/* .BMP file format output drivers: Demo of ASYNC rendering */

/* 2000-04-20 ghost@aladdin.com - Makes device structures const, changing
   makefile entry from DEV to DEV2. */
/* 1998/12/29 ghost@aladdin.com - Modified to use gdev_prn_render_lines,
   which replaces the former "overlay" calls */
/* 1998/11/23 ghost@aladdin.com - Removed pointless restriction to
   single-page output */
/* 1998/7/28 ghost@aladdin.com - Factored out common BMP format code
   to gdevbmpc.c */
/* Initial version 2/2/98 by John Desrosiers (soho@crl.com) */

#include "stdio_.h"
#include "gserrors.h"
#include "gdevprna.h"
#include "gdevpccm.h"
#include "gdevbmp.h"
#include "gdevppla.h"
#include "gpsync.h"

/*
 * The original version of this driver was restricted to producing a single
 * page per file.  If for some reason you want to reinstate this
 * restriction, uncomment the next line. 
 * NOTE: Even though the logic for multi-page files is straightforward,
 * it results in a file that most programs that process BMP format cannot
 * handle. Most programs will only display the first page.
 */
/*************** #define SINGLE_PAGE ****************/

/* ------ The device descriptors ------ */

/* Define data type for this device based on prn_device */
typedef struct gx_device_async_s {
    gx_device_common;
    gx_prn_device_common;
    bool UsePlanarBuffer;
    int buffered_page_exists;
    long file_offset_to_data[4];
} gx_device_async;

/* Define initializer for device */
#define async_device(procs, dname, w10, h10, xdpi, ydpi, lm, bm, rm, tm, color_bits, print_page)\
{ prn_device_std_margins_body(gx_device_async, procs, dname,\
    w10, h10, xdpi, ydpi, lm, tm, lm, bm, rm, tm, color_bits, print_page),\
    0, 0, { 0, 0, 0, 0 }\
}

private dev_proc_open_device(bmpa_writer_open);
private dev_proc_open_device(bmpa_cmyk_writer_open);
private prn_dev_proc_open_render_device(bmpa_reader_open_render_device);
private dev_proc_print_page_copies(bmpa_reader_print_page_copies);
/* VMS limits procedure names to 31 characters. */
private dev_proc_print_page_copies(bmpa_cmyk_reader_print_copies);
private prn_dev_proc_buffer_page(bmpa_reader_buffer_page);
private prn_dev_proc_buffer_page(bmpa_cmyk_reader_buffer_page);
private dev_proc_output_page(bmpa_reader_output_page);
private dev_proc_get_params(bmpa_get_params);
private dev_proc_put_params(bmpa_put_params);
private dev_proc_get_hardware_params(bmpa_get_hardware_params);
private prn_dev_proc_start_render_thread(bmpa_reader_start_render_thread);
private prn_dev_proc_get_space_params(bmpa_get_space_params);
#define default_print_page 0	/* not needed becoz print_page_copies def'd */

/* Monochrome. */

private const gx_device_procs bmpamono_procs =
  prn_procs(bmpa_writer_open, gdev_prn_output_page, gdev_prn_close);
const gx_device_async gs_bmpamono_device =
  async_device(bmpamono_procs, "bmpamono",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, default_print_page);

/* 1-bit-per-plane separated CMYK color. */

#define bmpa_cmyk_procs(p_open, p_map_color_rgb, p_map_cmyk_color)\
    p_open, NULL, NULL, gdev_prn_output_page, gdev_prn_close,\
    NULL, p_map_color_rgb, NULL, NULL, NULL, NULL, NULL, NULL,\
    bmpa_get_params, bmpa_put_params,\
    p_map_cmyk_color, NULL, NULL, NULL, gx_page_device_get_page_device

private const gx_device_procs bmpasep1_procs = {
    bmpa_cmyk_procs(bmpa_cmyk_writer_open, cmyk_1bit_map_color_rgb,
		    cmyk_1bit_map_cmyk_color)
};
const gx_device_async gs_bmpasep1_device = {
  prn_device_body(gx_device_async, bmpasep1_procs, "bmpasep1",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	4, 4, 1, 1, 2, 2, default_print_page)
};

/* 8-bit-per-plane separated CMYK color. */

private const gx_device_procs bmpasep8_procs = {
    bmpa_cmyk_procs(bmpa_cmyk_writer_open, cmyk_8bit_map_color_rgb,
		    cmyk_8bit_map_cmyk_color)
};
const gx_device_async gs_bmpasep8_device = {
  prn_device_body(gx_device_async, bmpasep8_procs, "bmpasep8",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	4, 32, 255, 255, 256, 256, default_print_page)
};

/* 4-bit (EGA/VGA-style) color. */

private const gx_device_procs bmpa16_procs =
  prn_color_procs(bmpa_writer_open, gdev_prn_output_page, gdev_prn_close,
    pc_4bit_map_rgb_color, pc_4bit_map_color_rgb);
const gx_device_async gs_bmpa16_device =
  async_device(bmpa16_procs, "bmpa16",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	4, default_print_page);

/* 8-bit (SuperVGA-style) color. */
/* (Uses a fixed palette of 3,3,2 bits.) */

private const gx_device_procs bmpa256_procs =
  prn_color_procs(bmpa_writer_open, gdev_prn_output_page, gdev_prn_close,
    pc_8bit_map_rgb_color, pc_8bit_map_color_rgb);
const gx_device_async gs_bmpa256_device =
  async_device(bmpa256_procs, "bmpa256",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	8, default_print_page);

/* 24-bit color. */

private const gx_device_procs bmpa16m_procs =
  prn_color_procs(bmpa_writer_open, gdev_prn_output_page, gdev_prn_close,
    bmp_map_16m_rgb_color, bmp_map_16m_color_rgb);
const gx_device_async gs_bmpa16m_device =
  async_device(bmpa16m_procs, "bmpa16m",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	24, default_print_page);

/* 32-bit CMYK color (outside the BMP specification). */

private const gx_device_procs bmpa32b_procs = {
    bmpa_cmyk_procs(bmpa_writer_open, gx_default_map_color_rgb,
		    gx_default_cmyk_map_cmyk_color)
};
const gx_device_async gs_bmpa32b_device =
  async_device(bmpa32b_procs, "bmpa32b",
	       DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	       X_DPI, Y_DPI,
	       0, 0, 0, 0,		/* margins */
	       32, default_print_page);

/* --------- Forward declarations ---------- */

private void bmpa_reader_thread(void *);

/* ------------ Writer Instance procedures ---------- */

/* Writer's open procedure */
private int
bmpa_open_writer(gx_device *pdev  /* Driver instance to open */,
		 dev_proc_print_page_copies((*reader_print_page_copies)),
		 prn_dev_proc_buffer_page((*reader_buffer_page)))
{
    gx_device_async * const pwdev = (gx_device_async *)pdev;
    int max_width;
    int max_raster;
    int min_band_height;
    int max_src_image_row;

    /*
     * Set up device's printer proc vector to point to this driver, since
     * there are no convenient macros for setting them up in static template.
     */
    init_async_render_procs(pwdev, bmpa_reader_start_render_thread,
			    reader_buffer_page,
			    reader_print_page_copies);
    set_dev_proc(pdev, get_params, bmpa_get_params);	/* because not all device-init macros allow this to be defined */
    set_dev_proc(pdev, put_params, bmpa_put_params);	/* ibid. */
    set_dev_proc(pdev, get_hardware_params, bmpa_get_hardware_params);
    set_dev_proc(pdev, output_page, bmpa_reader_output_page);	/* hack */
    pwdev->printer_procs.get_space_params = bmpa_get_space_params;
    pwdev->printer_procs.open_render_device =
	bmpa_reader_open_render_device;	/* Included for tutorial value */

    /*
     * Determine MAXIMUM parameters this device will have to support over
     * lifetime.  See comments for bmpa_get_space_params().
     */
    max_width = DEFAULT_WIDTH_10THS * 60;   /* figure max wid = default @ 600dpi */
    min_band_height = max(1, (DEFAULT_HEIGHT_10THS * 60) / 100);
    max_raster = bitmap_raster(max_width * pwdev->color_info.depth);	/* doesn't need to be super accurate */
    max_src_image_row = max_width * 4 * 2;

    /* Set to planar buffering mode if appropriate. */
    if (pwdev->UsePlanarBuffer)
	gdev_prn_set_procs_planar(pdev);

    /* Special writer open routine for async interpretation */
    /* Starts render thread */
    return gdev_prn_async_write_open((gx_device_printer *)pdev,
				     max_raster, min_band_height,
				     max_src_image_row);
}
private int
bmpa_writer_open(gx_device *pdev  /* Driver instance to open */)
{
    return bmpa_open_writer(pdev, bmpa_reader_print_page_copies,
			    bmpa_reader_buffer_page);
}
private int
bmpa_cmyk_writer_open(gx_device *pdev  /* Driver instance to open */)
{
    return bmpa_open_writer(pdev, bmpa_cmyk_reader_print_copies,
			    bmpa_cmyk_reader_buffer_page);
}

/* -------------- Renderer instance procedures ----------*/

/* Forward declarations */
private int
    bmpa_reader_buffer_planes(gx_device_printer *pdev, FILE *prn_stream,
			      int num_copies, int first_plane,
			      int last_plane, int raster);

/* Thread to do rendering, started by bmpa_reader_start_render_thread */
private void 
bmpa_reader_thread(void *params)
{
    gdev_prn_async_render_thread((gdev_prn_start_render_params *)params);
}

private int	/* rets 0 ok, -ve error if couldn't start thread */
bmpa_reader_start_render_thread(gdev_prn_start_render_params *params)
{
    return gp_create_thread(bmpa_reader_thread, params);
}

private int
bmpa_reader_open_render_device(gx_device_printer *ppdev)
{
    /*
     * Do anything that needs to be done at open time here.
     * Since this implementation doesn't do anything, we don't need to
     * cast the device argument to the more specific type.
     */
    /*gx_device_async * const prdev = (gx_device_async *)ppdev;*/

    /* Cascade down to the default handler */
    return gdev_prn_async_render_open(ppdev);
}

/* Generic routine to send the page to the printer. */
private int
bmpa_reader_output_page(gx_device *pdev, int num_copies, int flush)
{
    /*
     * HACK: open the printer page with the positionable attribute since
     * we need to seek back & forth to support partial rendering.
     */
    if ( num_copies > 0 || !flush ) {
	int code = gdev_prn_open_printer_positionable(pdev, 1, 1);

	if ( code < 0 )
	    return code;
    }
    return gdev_prn_output_page(pdev, num_copies, flush);
}

private int
bmpa_reader_print_planes(gx_device_printer *pdev, FILE *prn_stream,
			 int num_copies, int first_plane, int last_plane,
			 int raster)
{
    gx_device_async * const prdev = (gx_device_async *)pdev;
    /* BMP scan lines are padded to 32 bits. */
    uint bmp_raster = raster + (-raster & 3);
    int code = 0;
    int y;
    byte *row = 0;
    byte *raster_data;
    int plane;

    /* If there's data in buffer, need to process w/overlays */
    if (prdev->buffered_page_exists) {
	code = bmpa_reader_buffer_planes(pdev, prn_stream, num_copies,
					 first_plane, last_plane, raster);
	goto done;
    }
#ifdef SINGLE_PAGE
    /* BMP format is single page, so discard all but 1st printable page */
    /* Since the OutputFile may have a %d, we use ftell to determine if */
    /* this is a zero length file, which is legal to write		*/
    if (ftell(prn_stream) != 0)
	return 0;
#endif
    row = gs_alloc_bytes(pdev->memory, bmp_raster, "bmp file buffer");
    if (row == 0)		/* can't allocate row buffer */
	return_error(gs_error_VMerror);

    for (plane = first_plane; plane <= last_plane; ++plane) {
	gx_render_plane_t render_plane;

	/* Write header & seek to its end */
	code =
	    (first_plane < 0 ? write_bmp_header(pdev, prn_stream) :
	     write_bmp_separated_header(pdev, prn_stream));
	if (code < 0)
	    goto done;
	/* Save the file offset where data begins */
	if ((prdev->file_offset_to_data[plane - first_plane] =
	     ftell(prn_stream)) == -1L) {
	    code = gs_note_error(gs_error_ioerror);
	    goto done;
	}

	/*
	 * Write out the bands top to bottom.  Finish the job even if
	 * num_copies == 0, to avoid invalid output file.
	 */
	if (plane >= 0)
	    gx_render_plane_init(&render_plane, (gx_device *)pdev, plane);
	for (y = prdev->height - 1; y >= 0; y--) {
	    uint actual_raster;

	    code = gdev_prn_get_lines(pdev, y, 1, row, bmp_raster,
				      &raster_data, &actual_raster,
				      (plane < 0 ? NULL : &render_plane));
	    if (code < 0)
		goto done;
	    if (fwrite((const char *)raster_data, actual_raster, 1, prn_stream) < 1) {
		code = gs_error_ioerror;
		goto done;
	    }
	}
    }
done:
    gs_free_object(pdev->memory, row, "bmp file buffer");
    prdev->buffered_page_exists = 0;
    return code;
}
private int
bmpa_reader_print_page_copies(gx_device_printer *pdev, FILE *prn_stream,
			      int num_copies)
{
    return bmpa_reader_print_planes(pdev, prn_stream, num_copies, -1, -1,
				    gdev_prn_raster(pdev));
}
private int
bmpa_cmyk_plane_raster(gx_device_printer *pdev)
{
    return bitmap_raster(pdev->width * (pdev->color_info.depth / 4));
}
private int
bmpa_cmyk_reader_print_copies(gx_device_printer *pdev, FILE *prn_stream,
			      int num_copies)
{
    return bmpa_reader_print_planes(pdev, prn_stream, num_copies, 0, 3,
				    bmpa_cmyk_plane_raster(pdev));
}

/* Buffer a (partial) rasterized page & optionally print result multiple times. */
private int
bmpa_reader_buffer_planes(gx_device_printer *pdev, FILE *file, int num_copies,
			  int first_plane, int last_plane, int raster)
{
    gx_device_async * const prdev = (gx_device_async *)pdev;
    gx_device * const dev = (gx_device *)pdev;
    int code = 0;

    /* If there's no data in buffer, no need to do any overlays */
    if (!prdev->buffered_page_exists) {
	code = bmpa_reader_print_planes(pdev, file, num_copies,
					first_plane, last_plane, raster);
	goto done;
    }

    /*
     * Continue rendering on top of the existing file. This requires setting
     * up a buffer of the existing bits in GS's format (except for optional
     * extra padding bytes at the end of each scan line, provided the scan
     * lines are still correctly memory-aligned) and then calling
     * gdev_prn_render_lines.  If the device already provides a band buffer
     * -- which currently is always the case -- we can use it if we want;
     * but if a device stores partially rendered pages in memory in a
     * compatible format (e.g., a printer with a hardware page buffer), it
     * can render directly on top of the stored bits.
     *
     * If we can render exactly one band (or N bands) at a time, this is
     * more efficient, since otherwise (a) band(s) will have to be rendered
     * more than once.
     */

    {
	byte *raster_data;
	gx_device_clist_reader *const crdev =
	    (gx_device_clist_reader *)pdev;
	int raster = gx_device_raster(dev, 1);
	int padding = -raster & 3; /* BMP scan lines are padded to 32 bits. */
	int bmp_raster = raster + padding;
	int plane;

	/*
	 * Get the address of the renderer's band buffer.  In the future,
	 * it will be possible to suppress the allocation of this buffer,
	 * and to use only buffers provided the driver itself (e.g., a
	 * hardware buffer).
	 */
	if (!pdev->buffer_space) {
	    /* Not banding.  Can't happen. */
	    code = gs_note_error(gs_error_Fatal);
	    goto done;
	}
	raster_data = crdev->data;

	for (plane = first_plane; plane <= last_plane; ++plane) {
	    gx_render_plane_t render_plane;
	    gx_device *bdev;
	    int y, band_base_line;

	    /* Seek to beginning of data portion of file */
	    if (fseek(file, prdev->file_offset_to_data[plane - first_plane],
		      SEEK_SET)) {
		code = gs_note_error(gs_error_ioerror);
		goto done;
	    }

	    if (plane >= 0)
		gx_render_plane_init(&render_plane, (gx_device *)pdev, plane);
	    else
		render_plane.index = -1;

	    /* Set up the buffer device. */
	    code = gdev_create_buf_device(crdev->buf_procs.create_buf_device,
					  &bdev, crdev->target, &render_plane,
					  dev->memory, true);
	    if (code < 0)
		goto done;

	    /*
	     * Iterate thru bands from top to bottom.  As noted above, we
	     * do this an entire band at a time for efficiency.
	     */
	    for (y = dev->height - 1; y >= 0; y = band_base_line - 1) {
		int band_height =
		    dev_proc(dev, get_band)(dev, y, &band_base_line);
		int line;
		gs_int_rect band_rect;

		/* Set up the buffer device for this band. */
		code = crdev->buf_procs.setup_buf_device
		    (bdev, raster_data, bmp_raster, NULL, 0, band_height,
		     band_height);
		if (code < 0)
		    goto done;

		/* Fill in the buffer with a band from the BMP file. */
		/* Need to do this backward since BMP is top to bottom. */
		for (line = band_height - 1; line >= 0; --line)
		    if (fread(raster_data + line * bmp_raster,
			      raster, 1, file) < 1 ||
			fseek(file, padding, SEEK_CUR)
			) {
			code = gs_note_error(gs_error_ioerror);
			goto done;
		    }

		/* Continue rendering on top of the existing bits. */
		band_rect.p.x = 0;
		band_rect.p.y = band_base_line;
		band_rect.q.x = pdev->width;
		band_rect.q.y = band_base_line + band_height;
		if ((code = clist_render_rectangle((gx_device_clist *)pdev,
						   &band_rect, bdev,
						   &render_plane, false)) < 0)
		    goto done;

		/* Rewind & write out the updated buffer. */
		if (fseek(file, -bmp_raster * band_height, SEEK_CUR)) {
		    code = gs_note_error(gs_error_ioerror);
		    goto done;
		}
		for (line = band_height - 1; line >= 0; --line) {
		    if (fwrite(raster_data + line * bmp_raster,
			       bmp_raster, 1, file) < 1 ||
			fseek(file, padding, SEEK_CUR)
			) {
			code = gs_note_error(gs_error_ioerror);
			goto done;
		    }
		}
	    }
	    crdev->buf_procs.destroy_buf_device(bdev);
	}
    }

 done:
    prdev->buffered_page_exists = (code >= 0);
    return code;
}
private int
bmpa_reader_buffer_page(gx_device_printer *pdev, FILE *prn_stream,
			int num_copies)
{
    return bmpa_reader_buffer_planes(pdev, prn_stream, num_copies, -1, -1,
				     gdev_prn_raster(pdev));
}
private int
bmpa_cmyk_reader_buffer_page(gx_device_printer *pdev, FILE *prn_stream,
			     int num_copies)
{
    return bmpa_reader_buffer_planes(pdev, prn_stream, num_copies, 0, 3,
				     bmpa_cmyk_plane_raster(pdev));
}

/*------------ Procedures common to writer & renderer -------- */

/* Compute space parameters */
private void
bmpa_get_space_params(const gx_device_printer *pdev,
 gdev_prn_space_params *space_params)
{
    /* Plug params into device before opening it
     *
     * You ask "How did you come up with these #'s?" You asked, so...
     *
     * To answer clearly, let me begin by recapitulating how command list
     * (clist) device memory allocation works in the non-overlapped case:
     * When the device is opened, a buffer is allocated. How big? For
     * starters, it must be >= PRN_MIN_BUFFER_SPACE, and as we'll see, must
     * be sufficient to satisfy the rest of the band params. If you don't
     * specify a size for it in space_params.band.BandBufferSpace, the open
     * routine will use a heuristic where it tries to use PRN_BUFFER_SPACE,
     * then works its way down by factors of 2 if that much memory isn't
     * available.
     *
     * The device proceeds to divide the buffer into several parts: one of
     * them is used for the same thing during writing & rasterizing; the
     * other parts are redivided and used differently writing and
     * rasterizing. The limiting factor dictating memory requirements is the
     * rasterizer's render buffer.  This buffer needs to be able to contain
     * a pixmap that covers an entire band. Memory consumption is whatever
     * is needed to hold N rows of data aligned on word boundaries, +
     * sizeof(pointer) for each of N rows. Whatever is left over in the
     * rasterized is allocated to a tile cache. You want to make sure that
     * cache is at least 50KB.
     *
     * For example, take a 600 dpi b/w device at 8.5 x 11 inches.  For the
     * whole device, that's 6600 rows @ 638 bytes = ~4.2 MB total.  If the
     * device is divided into 100 bands, each band's rasterizer buffer is
     * 62K. Add on a 50K tile cache, and you get a 112KB (+ add a little
     * slop) total device buffer size.
     *
     * Now that we've covered the rasterizer, let's switch back to the
     * writer. The writer must have a tile cache *exactly* the same size as
     * the reader. This means that the space to divide up for the writer is
     * equal is size to the rasterizer's band buffer.  This space is divided
     * into 2 sections: per-band bookeeping info and a command buffer. The
     * bookeeping info currently uses ~72 bytes for each band. The rest is
     * the command buffer.
     *
     * To continue the same 112KB example, we have 62KB to slice up.
     * We need 72 bytes * 100 bands = 7.2KB, leaving a 55K command buffer.
     *
     * A larger command buffer has some performance (see gxclmem.c comments)
     * advantages in the general case, but is critical in one special case:
     * high-level images. Whenever possible, images are transmitted across
     * the band buffer in their original resolution and bits/pixel. The
     * alternative fallback behavior can be very slow.  Here, the relevant
     * restriction is that at least one entire source image row must fit
     * into the command buffer. This means that, in our example, an RGB
     * source image would have to be <= 18K pixels wide. If the image is
     * sampled at the same resolution as the hardware (600 dpi), that means
     * the row would be limited to a very reasonable 30 inches. However, if
     * the source image is sampled at 2400 dpi, that limit is only 7.5
     * inches. The situation gets worse as bands get smaller, but the
     * implementor must decide on the tradeoff point.
     *
     * The moral of the story is that you should never make a band
     * so small that its buffer limits the command buffer excessively.
     * Again, Max image row bytes = band buffer size - # bands * 72. 
     *
     * In the overlapped case, everything is exactly as above, except that
     * two identical devices, each with an identical buffer, are allocated:
     * one for the writer, and one for the rasterizer. Because it's critical
     * to allocate identical buffers, I *strongly* recommend setting these
     * params in the writer's open routine:
     * space_params.band.BandBufferSpace, .BandWidth and .BandHeight.  If
     * you don't force these values to a known value, the memory allocation
     * heuristic may not come to the same result for both copies of the
     * device, since the first allocation will diminish the amount of free
     * memory.
     *
     * There is room for an important optimization here: allocate the
     * writer's space with enough memory for a generous command buffer, but
     * allocate the reader with only enough memory for a band rasterization
     * buffer and the tile cache.  To do this, observe that the space_params
     * struct has two sizes: BufferSpace vs. BandBufferSpace.  To start,
     * BandBufferSpace is always <= BufferSpace. On the reader side,
     * BandBufferSpace is divided between the tile cache and the rendering
     * buffer -- that's all the memory that's needed to rasterize. On the
     * writer's side, BandBufferSpace is divided the same way: the tile
     * cache (which must be identical to the reader's) is carved out, and
     * the space that would have been used for a rasterizing buffer is used
     * as a command buffer. However, you can further increase the cmd buf
     * further by setting BufferSize (not BandBufferSize) to a higher number
     * than BandBufferSize. In that case, the command buffer is increased by
     * the difference (BufferSize - BandBufferSize). There is logic in the
     * memory allocation for printers that will automatically use BufferSize
     * for writers (or non-async printers), and BandBufferSize for readers.
     *
     * Note: per the comments in gxclmem.c, the banding logic will perform
     * better with 1MB or better for the command list.
     */
    
    /* This will give us a very "ungenerous" buffer. */
    /* Here, my arbitrary rule for min image row is: twice the dest width */
    /* in full CMYK. */
    int render_space;
    int writer_space;
    const int tile_cache_space = 50 * 1024;
    const int min_image_rows = 2;
    int min_row_space =
	min_image_rows * (  4 * ( pdev->width + sizeof(int) - 1 )  );
    int min_band_height = max(1, pdev->height / 100);	/* make bands >= 1% of total */

    space_params->band.BandWidth = pdev->width;
    space_params->band.BandHeight = min_band_height;

    render_space = gdev_mem_data_size( (const gx_device_memory *)pdev,
				       space_params->band.BandWidth,
				       space_params->band.BandHeight );
    /* need to include minimal writer requirements to satisfy rasterizer init */
    writer_space = 	/* add 5K slop for good measure */
	5000 + (72 + 8) * ( (pdev->height / space_params->band.BandHeight) + 1 );
    space_params->band.BandBufferSpace =
	max(render_space, writer_space) + tile_cache_space;
    space_params->BufferSpace =
	max(render_space, writer_space + min_row_space) + tile_cache_space;
    /**************** HACK HACK HACK ****************/
    /* Override this computation to force reader & writer to match */
    space_params->BufferSpace = space_params->band.BandBufferSpace;
}

/* Get device parameters. */
private int
bmpa_get_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_async * const bdev = (gx_device_async *)pdev;

    return gdev_prn_get_params_planar(pdev, plist, &bdev->UsePlanarBuffer);
}

/* Put device parameters. */
/* IMPORTANT: async drivers must NOT CLOSE the device while doing put_params.*/
/* IMPORTANT: async drivers must NOT CLOSE the device while doing put_params.*/
/* IMPORTANT: async drivers must NOT CLOSE the device while doing put_params.*/
/* IMPORTANT: async drivers must NOT CLOSE the device while doing put_params.*/
private int
bmpa_put_params(gx_device *pdev, gs_param_list *plist)
{
    /*
     * This driver does nothing interesting except cascade down to
     * gdev_prn_put_params_planar, which is something it would have to do
     * even if it did do something interesting here.
     *
     * Note that gdev_prn_put_params[_planar] does not close the device.
     */
    gx_device_async * const bdev = (gx_device_async *)pdev;

    return gdev_prn_put_params_planar(pdev, plist, &bdev->UsePlanarBuffer);
}

/* Get hardware-detected parameters. */
/* This proc defines a only one param: a useless value for testing */
private int
bmpa_get_hardware_params(gx_device *dev, gs_param_list *plist)
{
    static const char *const test_value = "Test value";
    static const char *const test_name = "TestValue";
    int code = 0;

    if ( param_requested(plist, test_name) ) {
	gs_param_string param_str;

	param_string_from_string(param_str, test_value); /* value must be persistent to use this macro */
	code = param_write_string(plist, test_name, &param_str);
    }
    return code;
}
