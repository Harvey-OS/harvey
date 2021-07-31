/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpng.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
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
const gx_device_printer gs_png16_device =
prn_device(png16_procs, "png16",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   4, png_print_page);

/* 8-bit (SuperVGA-style) color. */
/* (Uses a fixed palette of 3,3,2 bits.) */

private const gx_device_procs png256_procs =
prn_color_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		pc_8bit_map_rgb_color, pc_8bit_map_color_rgb);
const gx_device_printer gs_png256_device =
prn_device(png256_procs, "png256",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   8, png_print_page);

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
    int code = 0;		/* return code */
    const char *software_key = "Software";
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
    sprintf(software_text, "%s %d.%02d", gs_product,
	    (int)(gs_revision / 100), (int)(gs_revision % 100));
    text_png.compression = -1;	/* uncompressed */
    text_png.key = (char *)software_key;	/* not const, unfortunately */
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
