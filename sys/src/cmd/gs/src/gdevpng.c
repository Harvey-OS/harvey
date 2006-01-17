/* Copyright (C) 1995-2003, artofcode LLC.  All rights reserved.
  
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

/* $Id: gdevpng.c,v 1.12 2005/07/15 05:23:43 giles Exp $ */
/* PNG (Portable Network Graphics) Format.  Pronounced "ping". */
/* lpd 1999-09-24: changes PNG_NO_STDIO to PNG_NO_CONSOLE_IO for libpng
   versions 1.0.3 and later. */
/* lpd 1999-07-01: replaced remaining uses of gs_malloc and gs_free with
   gs_alloc_bytes and gs_free_object. */
/* lpd 1999-03-08: changed png.h to png_.h to allow compiling with only
   headers in /usr/include, no source code. */
/* lpd 1997-07-20: changed from using gs_malloc/png_xxx_int to png_create_xxx
 * for allocating structures, and from gs_free to png_write_destroy for
 * freeing them. */
/* lpd 1997-5-7: added PNG_LIBPNG_VER conditional for operand types of
 * dummy png_push_fill_buffer. */
/* lpd 1997-4-13: Added PNG_NO_STDIO to remove library access to stderr. */
/* lpd 1997-3-14: Added resolution (pHYs) to output. */
/* lpd 1996-6-24: Added #ifdef for compatibility with old libpng versions. */
/* lpd 1996-6-11: Edited to remove unnecessary color mapping code. */
/* lpd (L. Peter Deutsch) 1996-4-7: Modified for libpng 0.88. */
/* Original version by Russell Lang 1995-07-04 */

#include "gdevprn.h"
#include "gdevmem.h"
#include "gdevpccm.h"
#include "gscdefs.h"

#define PNG_INTERNAL
/*
 * libpng versions 1.0.3 and later allow disabling access to the stdxxx
 * files while retaining support for FILE * I/O.
 */
#define PNG_NO_CONSOLE_IO
/*
 * Earlier libpng versions require disabling FILE * I/O altogether.
 * This produces a compiler warning about no prototype for png_init_io.
 * The right thing will happen at link time, since the library itself
 * is compiled with stdio support.  Unfortunately, we can't do this
 * conditionally depending on PNG_LIBPNG_VER, because this is defined
 * in png.h.
 */
/*#define PNG_NO_STDIO*/
#include "png_.h"

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

private dev_proc_print_page(png_print_page);
private dev_proc_open_device(pngalpha_open);
private dev_proc_encode_color(pngalpha_encode_color);
private dev_proc_decode_color(pngalpha_decode_color);
private dev_proc_copy_alpha(pngalpha_copy_alpha);
private dev_proc_fill_rectangle(pngalpha_fill_rectangle);
private dev_proc_get_params(pngalpha_get_params);
private dev_proc_put_params(pngalpha_put_params);
private dev_proc_create_buf_device(pngalpha_create_buf_device);

/* Monochrome. */

const gx_device_printer gs_pngmono_device =
prn_device(prn_std_procs, "pngmono",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   1, png_print_page);

/* 4-bit planar (EGA/VGA-style) color. */

private const gx_device_procs png16_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		pc_4bit_map_rgb_color, pc_4bit_map_color_rgb);
const gx_device_printer gs_png16_device = {
  prn_device_body(gx_device_printer, png16_procs, "png16",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   3, 4, 1, 1, 2, 2, png_print_page)
};

/* 8-bit (SuperVGA-style) color. */
/* (Uses a fixed palette of 3,3,2 bits.) */

private const gx_device_procs png256_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		pc_8bit_map_rgb_color, pc_8bit_map_color_rgb);
const gx_device_printer gs_png256_device = {
  prn_device_body(gx_device_printer, png256_procs, "png256",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   3, 8, 5, 5, 6, 6, png_print_page)
};

/* 8-bit gray */

private const gx_device_procs pnggray_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
	      gx_default_gray_map_rgb_color, gx_default_gray_map_color_rgb);
const gx_device_printer gs_pnggray_device =
{prn_device_body(gx_device_printer, pnggray_procs, "pnggray",
		 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		 X_DPI, Y_DPI,
		 0, 0, 0, 0,	/* margins */
		 1, 8, 255, 0, 256, 0, png_print_page)
};

/* 24-bit color. */

private const gx_device_procs png16m_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);
const gx_device_printer gs_png16m_device =
prn_device(png16m_procs, "png16m",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   24, png_print_page);

/* 48 bit color. */

private const gx_device_procs png48_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb);
const gx_device_printer gs_png48_device =
prn_device(png48_procs, "png48",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   48, png_print_page);


/* 32-bit RGBA */
/* pngalpha device is 32-bit RGBA, with the alpha channel
 * indicating pixel coverage, not true transparency.  
 * Anti-aliasing is enabled by default.
 * An erasepage will erase to transparent, not white.
 * It is intended to be used for creating web graphics with
 * a transparent background.
 */
typedef struct gx_device_pngalpha_s gx_device_pngalpha;
struct gx_device_pngalpha_s {
    gx_device_common;
    gx_prn_device_common;
    dev_t_proc_fill_rectangle((*orig_fill_rectangle), gx_device);
    int background;
};
private const gx_device_procs pngalpha_procs =
{
	pngalpha_open,
	NULL,	/* get_initial_matrix */
	NULL,	/* sync_output */
	gdev_prn_output_page,
	gdev_prn_close,
	pngalpha_encode_color,	/* map_rgb_color */
	pngalpha_decode_color,  /* map_color_rgb */
	pngalpha_fill_rectangle,
	NULL,	/* tile_rectangle */
	NULL,	/* copy_mono */
	NULL,	/* copy_color */
	NULL,	/* draw_line */
	NULL,	/* get_bits */
	pngalpha_get_params,
	pngalpha_put_params,
	NULL,	/* map_cmyk_color */
	NULL,	/* get_xfont_procs */
	NULL,	/* get_xfont_device */
	NULL,	/* map_rgb_alpha_color */
	gx_page_device_get_page_device,
	NULL,	/* get_alpha_bits */
	pngalpha_copy_alpha,
	NULL,	/* get_band */
	NULL,	/* copy_rop */
	NULL,	/* fill_path */
	NULL,	/* stroke_path */
	NULL,	/* fill_mask */
	NULL,	/* fill_trapezoid */
	NULL,	/* fill_parallelogram */
	NULL,	/* fill_triangle */
	NULL,	/* draw_thin_line */
	NULL,	/* begin_image */
	NULL,	/* image_data */
	NULL,	/* end_image */
	NULL,	/* strip_tile_rectangle */
	NULL,	/* strip_copy_rop, */
	NULL,	/* get_clipping_box */
	NULL,	/* begin_typed_image */
	NULL,	/* get_bits_rectangle */
	NULL,	/* map_color_rgb_alpha */
	NULL,	/* create_compositor */
	NULL,	/* get_hardware_params */
	NULL,	/* text_begin */
	NULL,	/* finish_copydevice */
	NULL, 	/* begin_transparency_group */
	NULL, 	/* end_transparency_group */
	NULL, 	/* begin_transparency_mask */
	NULL, 	/* end_transparency_mask */
	NULL, 	/* discard_transparency_layer */
	gx_default_DevRGB_get_color_mapping_procs,
	gx_default_DevRGB_get_color_comp_index, 
	pngalpha_encode_color,
	pngalpha_decode_color
};

const gx_device_pngalpha gs_pngalpha_device = {
	std_device_part1_(gx_device_pngalpha, &pngalpha_procs, "pngalpha",
		&st_device_printer, open_init_closed),
	/* color_info */ 
	{3 /* max components */,
	 3 /* number components */,
	 GX_CINFO_POLARITY_ADDITIVE /* polarity */,
	 32 /* depth */,
	 -1 /* gray index */,
	 255 /* max gray */,
	 255 /* max color */,
	 256 /* dither grays */,
	 256 /* dither colors */,
	 { 4, 4 } /* antialias info text, graphics */,
	 GX_CINFO_SEP_LIN_NONE /* separable_and_linear */,
	 { 0 } /* component shift */,
	 { 0 } /* component bits */,
	 { 0 } /* component mask */,
	 "DeviceRGB" /* process color name */,
	 GX_CINFO_OPMODE_UNKNOWN /* opmode */,
	 0 /* process_cmps */
	},
	std_device_part2_(
	  (int)((float)(DEFAULT_WIDTH_10THS) * (X_DPI) / 10 + 0.5),
	  (int)((float)(DEFAULT_HEIGHT_10THS) * (Y_DPI) / 10 + 0.5),
	   X_DPI, Y_DPI),
	offset_margin_values(0, 0, 0, 0, 0, 0),
	std_device_part3_(),
	prn_device_body_rest_(png_print_page),
	NULL, 
	0xffffff	/* white background */
};


/* ------ Private definitions ------ */

/* Write out a page in PNG format. */
/* This routine is used for all formats. */
private int
png_print_page(gx_device_printer * pdev, FILE * file)
{
    gs_memory_t *mem = pdev->memory;
    int raster = gdev_prn_raster(pdev);

    /* PNG structures */
    byte *row = gs_alloc_bytes(mem, raster, "png raster buffer");
    png_struct *png_ptr =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_info *info_ptr =
    png_create_info_struct(png_ptr);
    int height = pdev->height;
    int depth = pdev->color_info.depth;
    int y;
    int code;			/* return code */
    char software_key[80];
    char software_text[256];
    png_text text_png;

    if (row == 0 || png_ptr == 0 || info_ptr == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    /* set error handling */
    if (setjmp(png_ptr->jmpbuf)) {
	/* If we get here, we had a problem reading the file */
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    code = 0;			/* for normal path */
    /* set up the output control */
    png_init_io(png_ptr, file);

    /* set the file information here */
    info_ptr->width = pdev->width;
    info_ptr->height = pdev->height;
    /* resolution is in pixels per meter vs. dpi */
    info_ptr->x_pixels_per_unit =
	(png_uint_32) (pdev->HWResolution[0] * (100.0 / 2.54));
    info_ptr->y_pixels_per_unit =
	(png_uint_32) (pdev->HWResolution[1] * (100.0 / 2.54));
    info_ptr->phys_unit_type = PNG_RESOLUTION_METER;
    info_ptr->valid |= PNG_INFO_pHYs;
    switch (depth) {
	case 32:
	    info_ptr->bit_depth = 8;
	    info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	    png_set_invert_alpha(png_ptr);
	    {   gx_device_pngalpha *ppdev = (gx_device_pngalpha *)pdev;
		png_color_16 background;
		background.index = 0;
		background.red =   (ppdev->background >> 16) & 0xff;
		background.green = (ppdev->background >> 8)  & 0xff;
		background.blue =  (ppdev->background)       & 0xff;
		background.gray = 0;
		png_set_bKGD(png_ptr, info_ptr, &background);
	    }
	    break;
	case 48:
	    info_ptr->bit_depth = 16;
	    info_ptr->color_type = PNG_COLOR_TYPE_RGB;
#if defined(ARCH_IS_BIG_ENDIAN) && (!ARCH_IS_BIG_ENDIAN) 
	    png_set_swap(png_ptr);
#endif
	    break;
	case 24:
	    info_ptr->bit_depth = 8;
	    info_ptr->color_type = PNG_COLOR_TYPE_RGB;
	    break;
	case 8:
	    info_ptr->bit_depth = 8;
	    if (gx_device_has_color(pdev))
		info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
	    else
		info_ptr->color_type = PNG_COLOR_TYPE_GRAY;
	    break;
	case 4:
	    info_ptr->bit_depth = 4;
	    info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
	    break;
	case 1:
	    info_ptr->bit_depth = 1;
	    info_ptr->color_type = PNG_COLOR_TYPE_GRAY;
	    /* invert monocrome pixels */
	    png_set_invert_mono(png_ptr);
	    break;
    }

    /* set the palette if there is one */
    if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
	int i;
	int num_colors = 1 << depth;
	gx_color_value rgb[3];

	info_ptr->palette =
	    (void *)gs_alloc_bytes(mem, 256 * sizeof(png_color),
				   "png palette");
	if (info_ptr->palette == 0) {
	    code = gs_note_error(gs_error_VMerror);
	    goto done;
	}
	info_ptr->num_palette = num_colors;
	info_ptr->valid |= PNG_INFO_PLTE;
	for (i = 0; i < num_colors; i++) {
	    (*dev_proc(pdev, map_color_rgb)) ((gx_device *) pdev,
					      (gx_color_index) i, rgb);
	    info_ptr->palette[i].red = gx_color_value_to_byte(rgb[0]);
	    info_ptr->palette[i].green = gx_color_value_to_byte(rgb[1]);
	    info_ptr->palette[i].blue = gx_color_value_to_byte(rgb[2]);
	}
    }
    /* add comment */
    strncpy(software_key, "Software", sizeof(software_key));
    sprintf(software_text, "%s %d.%02d", gs_product,
	    (int)(gs_revision / 100), (int)(gs_revision % 100));
    text_png.compression = -1;	/* uncompressed */
    text_png.key = software_key;
    text_png.text = software_text;
    text_png.text_length = strlen(software_text);
    info_ptr->text = &text_png;
    info_ptr->num_text = 1;

    /* write the file information */
    png_write_info(png_ptr, info_ptr);

    /* don't write the comments twice */
    info_ptr->num_text = 0;
    info_ptr->text = NULL;

    /* Write the contents of the image. */
    for (y = 0; y < height; y++) {
	gdev_prn_copy_scan_lines(pdev, y, row, raster);
	png_write_rows(png_ptr, &row, 1);
    }

    /* write the rest of the file */
    png_write_end(png_ptr, info_ptr);

    /* if you alloced the palette, free it here */
    gs_free_object(mem, info_ptr->palette, "png palette");

  done:
    /* free the structures */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    gs_free_object(mem, row, "png raster buffer");

    return code;
}

/*
 * Patch around a static reference to a never-used procedure.
 * This could be avoided if we were willing to edit pngconf.h to
 *      #undef PNG_PROGRESSIVE_READ_SUPPORTED
 */
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
#  if PNG_LIBPNG_VER >= 95
#    define PPFB_LENGTH_T png_size_t
#  else
#    define PPFB_LENGTH_T png_uint_32
#  endif
void
png_push_fill_buffer(png_structp png_ptr, png_bytep buffer,
		     PPFB_LENGTH_T length)
{
}
#endif

private int
pngalpha_open(gx_device * pdev)
{
    gx_device_pngalpha *ppdev = (gx_device_pngalpha *)pdev;
    int code;
    /* We replace create_buf_device so we can replace copy_alpha 
     * for memory device, but not clist.
     */
    ppdev->printer_procs.buf_procs.create_buf_device = 
	pngalpha_create_buf_device;
    code = gdev_prn_open(pdev);
    /* We intercept fill_rectangle to translate "fill page with white"
     * into "fill page with transparent".  We then call the original
     * implementation of fill_page.
     */
    if ((ppdev->procs.fill_rectangle != pngalpha_fill_rectangle) &&
	(ppdev->procs.fill_rectangle != NULL)) {
	ppdev->orig_fill_rectangle = ppdev->procs.fill_rectangle;
        ppdev->procs.fill_rectangle = pngalpha_fill_rectangle;
    }
    return code;
}

private int 
pngalpha_create_buf_device(gx_device **pbdev, gx_device *target,
   const gx_render_plane_t *render_plane, gs_memory_t *mem,
   bool for_band)
{
    gx_device_printer *ptarget = (gx_device_printer *)target;
    int code = gx_default_create_buf_device(pbdev, target, 
	render_plane, mem, for_band);
    /* Now set copy_alpha to one that handles RGBA */
    set_dev_proc(*pbdev, copy_alpha, ptarget->orig_procs.copy_alpha);
    return code;
}

private int
pngalpha_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_pngalpha *ppdev = (gx_device_pngalpha *)pdev;
    int background;
    int code;

    /* BackgroundColor in format 16#RRGGBB is used for bKGD chunk */
    switch(code = param_read_int(plist, "BackgroundColor", &background)) {
	case 0:
	    ppdev->background = background & 0xffffff;
	    break;
	case 1:		/* not found */
	    code = 0;
	    break;
	default:
	    param_signal_error(plist, "BackgroundColor", code);
	    break;
    }

    if (code == 0) {
	code = gdev_prn_put_params(pdev, plist);
	if ((ppdev->procs.fill_rectangle != pngalpha_fill_rectangle) &&
	    (ppdev->procs.fill_rectangle != NULL)) {
	    /* Get current implementation of fill_rectangle and restore ours.
	     * Implementation is either clist or memory and can change 
	     * during put_params.
	     */
	    ppdev->orig_fill_rectangle = ppdev->procs.fill_rectangle;
	    ppdev->procs.fill_rectangle = pngalpha_fill_rectangle;
	}
    }
    return code;
}

/* Get device parameters */
private int
pngalpha_get_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_pngalpha *ppdev = (gx_device_pngalpha *)pdev;
    int code = gdev_prn_get_params(pdev, plist);
    if (code >= 0)
	code = param_write_int(plist, "BackgroundColor",
				&(ppdev->background));
    return code;
}


/* RGB mapping for 32-bit RGBA color devices */

private gx_color_index
pngalpha_encode_color(gx_device * dev, const gx_color_value cv[])
{
    /* bits 0-7 are alpha, stored inverted to avoid white/opaque 
     * being 0xffffffff which is also gx_no_color_index.
     * So 0xff is transparent and 0x00 is opaque.
     * We always return opaque colors (bits 0-7 = 0).
     * Return value is 0xRRGGBB00.
     */
    return
	((uint) gx_color_value_to_byte(cv[2]) << 8) +
	((ulong) gx_color_value_to_byte(cv[1]) << 16) +
	((ulong) gx_color_value_to_byte(cv[0]) << 24);
}

/* Map a color index to a r-g-b color. */
private int
pngalpha_decode_color(gx_device * dev, gx_color_index color,
			     gx_color_value prgb[3])
{
    prgb[0] = gx_color_value_from_byte((color >> 24) & 0xff);
    prgb[1] = gx_color_value_from_byte((color >> 16) & 0xff);
    prgb[2] = gx_color_value_from_byte((color >> 8)  & 0xff);
    return 0;
}

private int
pngalpha_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		  gx_color_index color)
{
    gx_device_pngalpha *pdev = (gx_device_pngalpha *)dev;
    if ((color == 0xffffff00) && (x==0) && (y==0) 
	&& (w==dev->width) && (h==dev->height)) {
	/* If filling whole page with white, make it transparent */
        return pdev->orig_fill_rectangle(dev, x, y, w, h, 0xfefefeff);
    }
    return pdev->orig_fill_rectangle(dev, x, y, w, h, color);
}

/* Implementation for 32-bit RGBA in a memory buffer */
/* Derived from gx_default_copy_alpha, but now maintains alpha channel. */
private int
pngalpha_copy_alpha(gx_device * dev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
		      gx_color_index color, int depth)
{				/* This might be called with depth = 1.... */
    if (depth == 1)
	return (*dev_proc(dev, copy_mono)) (dev, data, data_x, raster, id,
					    x, y, width, height,
					    gx_no_color_index, color);
    /*
     * Simulate alpha by weighted averaging of RGB values.
     * This is very slow, but functionally correct.
     */
    {
	const byte *row;
	gs_memory_t *mem = dev->memory;
	int bpp = dev->color_info.depth;
	int ncomps = dev->color_info.num_components;
	uint in_size = gx_device_raster(dev, false);
	byte *lin;
	uint out_size;
	byte *lout;
	int code = 0;
	gx_color_value color_cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
	int ry;

	fit_copy(dev, data, data_x, raster, id, x, y, width, height);
	row = data;
	out_size = bitmap_raster(width * bpp);
	lin = gs_alloc_bytes(mem, in_size, "copy_alpha(lin)");
	lout = gs_alloc_bytes(mem, out_size, "copy_alpha(lout)");
	if (lin == 0 || lout == 0) {
	    code = gs_note_error(gs_error_VMerror);
	    goto out;
	}
	(*dev_proc(dev, decode_color)) (dev, color, color_cv);
	for (ry = y; ry < y + height; row += raster, ++ry) {
	    byte *line;
	    int sx, rx;

	    DECLARE_LINE_ACCUM_COPY(lout, bpp, x);

	    code = (*dev_proc(dev, get_bits)) (dev, ry, lin, &line);
	    if (code < 0)
		break;
	    for (sx = data_x, rx = x; sx < data_x + width; ++sx, ++rx) {
		gx_color_index previous = gx_no_color_index;
		gx_color_index composite;
		int alpha2, alpha;

		if (depth == 2)	/* map 0 - 3 to 0 - 15 */
		    alpha = ((row[sx >> 2] >> ((3 - (sx & 3)) << 1)) & 3) * 5;
		else
		    alpha2 = row[sx >> 1],
			alpha = (sx & 1 ? alpha2 & 0xf : alpha2 >> 4);
		if (alpha == 15) {	/* Just write the new color. */
		    composite = color;
		} else {
		    if (previous == gx_no_color_index) {	/* Extract the old color. */
			const byte *src = line + (rx * (bpp >> 3));
			previous = 0;
			previous += (gx_color_index) * src++ << 24;
			previous += (gx_color_index) * src++ << 16;
			previous += (gx_color_index) * src++ << 8;
			previous += *src++;
		    }
		    if (alpha == 0) {	/* Just write the old color. */
			composite = previous;
		    } else {	/* Blend values. */
			gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];
			int i;
			int old_coverage;
			int new_coverage;

			(*dev_proc(dev, decode_color)) (dev, previous, cv);
			/* decode color doesn't give us coverage */
			cv[3] = previous & 0xff;
			old_coverage = 255 - cv[3];
			new_coverage = 
			    (255 * alpha + old_coverage * (15 - alpha)) / 15;
			for (i=0; i<ncomps; i++)
			    cv[i] = min(((255 * alpha * color_cv[i]) + 
				(old_coverage * (15 - alpha ) * cv[i]))
				/ (new_coverage * 15), gx_max_color_value);
			composite =
			    (*dev_proc(dev, encode_color)) (dev, cv);
			/* encode color doesn't include coverage */
			composite |= (255 - new_coverage) & 0xff;
			
			/* composite can never be gx_no_color_index
			 * because pixel is never completely transparent
			 * (low byte != 0xff).
			 */
		    }
		}
		LINE_ACCUM(composite, bpp);
	    }
	    LINE_ACCUM_COPY(dev, lout, bpp, x, rx, raster, ry);
	}
      out:gs_free_object(mem, lout, "copy_alpha(lout)");
	gs_free_object(mem, lin, "copy_alpha(lin)");
	return code;
    }
}

