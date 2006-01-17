/* Copyright (C) 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxdevice.h,v 1.21 2005/03/14 18:08:36 dan Exp $ */
/* Definitions for device implementors */

#ifndef gxdevice_INCLUDED
#  define gxdevice_INCLUDED

#include "stdio_.h"		/* for FILE */
#include "gxdevcli.h"
#include "gsfname.h"
#include "gsparam.h"
/*
 * Many drivers still use gs_malloc and gs_free, so include the interface
 * for these.  (Eventually they should go away.)
 */
#include "gsmalloc.h"
/*
 * Similarly, quite a few drivers reference stdout and/or stderr.
 * (Eventually these references must go away.)
 */
#include "gxstdio.h"

/*
 * NOTE: if you write code that creates device instances (either with
 * gs_copydevice or by allocating them explicitly), allocates device
 * instances as either local or static variables (actual instances, not
 * pointers to instances), or sets the target of forwarding devices, please
 * read the documentation in gxdevcli.h about memory management for devices.
 * The rules for doing these things changed substantially in release 5.68,
 * in a non-backward-compatible way, and unfortunately we could not find a
 * way to make the compiler give an error at places that need changing.
 */

/* ---------------- Auxiliary types and structures ---------------- */

/* Define default pages sizes. */
/* U.S. letter paper (8.5" x 11"). */
#define DEFAULT_WIDTH_10THS_US_LETTER 85
#define DEFAULT_HEIGHT_10THS_US_LETTER 110
/* A4 paper (210mm x 297mm), we use 595pt x 842pt. */
#define DEFAULT_WIDTH_10THS_A4 82.6389
#define DEFAULT_HEIGHT_10THS_A4 116.9444
/* Choose a default.  A4 may be set in the makefile. */
#ifdef A4
#  define DEFAULT_WIDTH_10THS DEFAULT_WIDTH_10THS_A4
#  define DEFAULT_HEIGHT_10THS DEFAULT_HEIGHT_10THS_A4
#else
#  define DEFAULT_WIDTH_10THS DEFAULT_WIDTH_10THS_US_LETTER
#  define DEFAULT_HEIGHT_10THS DEFAULT_HEIGHT_10THS_US_LETTER
#endif

/* ---------------- Device structure ---------------- */

/*
 * To insulate statically defined device templates from the
 * consequences of changes in the device structure, the following macros
 * must be used for generating initialized device structures.
 *
 * The computations of page width and height in pixels should really be
 *      ((int)(page_width_inches*x_dpi))
 * but some compilers (the Ultrix 3.X pcc compiler and the HPUX compiler)
 * can't cast a computed float to an int.  That's why we specify
 * the page width and height in inches/10 instead of inches.
 * This has been now been changed to use floats.
 *
 * Note that the macro is broken up so as to be usable for devices that
 * add further initialized state to the generic device.
 * Note also that the macro does not initialize procs, which is
 * the next element of the structure.
 */
#define std_device_part1_(devtype, ptr_procs, dev_name, stype, open_init)\
	sizeof(devtype), ptr_procs, dev_name,\
	0 /*memory*/, stype, 0 /*stype_is_dynamic*/, 0 /*finalize*/,\
	{ 0 } /*rc*/, 0 /*retained*/, open_init() /*is_open, max_fill_band*/
	/* color_info goes here */
/*
 * The MetroWerks compiler has some bizarre bug that produces a spurious
 * error message if the width and/or height are defined as 0 below,
 * unless we use the +/- workaround in the next macro.
 */
#define std_device_part2_(width, height, x_dpi, y_dpi)\
	{ gx_no_color_index, gx_no_color_index },\
          width, height, 0/*TrayOrientation*/,\
	{ (float)((((width) * 72.0 + 0.5) - 0.5) / (x_dpi))/*MediaSize[0]*/,\
	  (float)((((height) * 72.0 + 0.5) - 0.5) / (y_dpi))/*MediaSize[1]*/},\
	{ 0, 0, 0, 0 }/*ImagingBBox*/, 0/*ImagingBBox_set*/,\
	{ x_dpi, y_dpi }/*HWResolution*/, { x_dpi, y_dpi }/*MarginsHWResolution*/

/* offsets and margins go here */
#define std_device_part3_()\
	0/*PageCount*/, 0/*ShowpageCount*/, 1/*NumCopies*/, 0/*NumCopies_set*/,\
	0/*IgnoreNumCopies*/, 0/*UseCIEColor*/, 0/*LockSafetyParams*/,\
	{ gx_default_install, gx_default_begin_page, gx_default_end_page }
/*
 * We need a number of different variants of the std_device_ macro simply
 * because we can't pass the color_info or offsets/margins
 * as macro arguments, which in turn is because of the early macro
 * expansion issue noted in stdpre.h.  The basic variants are:
 *      ...body_with_macros_, which uses 0-argument macros to supply
 *        open_init, color_info, and offsets/margins;
 *      ...full_body, which takes 12 values (6 for dci_values,
 *        6 for offsets/margins);
 *      ...color_full_body, which takes 9 values (3 for dci_color,
 *        6 for margins/offset).
 *      ...std_color_full_body, which takes 7 values (1 for dci_std_color,
 *        6 for margins/offset).
 *      
 */
#define std_device_body_with_macros_(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, open_init, dci_macro, margins_macro)\
	std_device_part1_(dtype, pprocs, dname, stype, open_init),\
	dci_macro(),\
	std_device_part2_(w, h, xdpi, ydpi),\
	margins_macro(),\
	std_device_part3_()

#define std_device_std_body_type(dtype, pprocs, dname, stype, w, h, xdpi, ydpi)\
	std_device_body_with_macros_(dtype, pprocs, dname, stype,\
	  w, h, xdpi, ydpi,\
	  open_init_closed, dci_black_and_white_, no_margins_)

#define std_device_std_body(dtype, pprocs, dname, w, h, xdpi, ydpi)\
	std_device_std_body_type(dtype, pprocs, dname, 0, w, h, xdpi, ydpi)

#define std_device_std_body_type_open(dtype, pprocs, dname, stype, w, h, xdpi, ydpi)\
	std_device_body_with_macros_(dtype, pprocs, dname, stype,\
	  w, h, xdpi, ydpi,\
	  open_init_open, dci_black_and_white_, no_margins_)

#define std_device_std_body_open(dtype, pprocs, dname, w, h, xdpi, ydpi)\
	std_device_std_body_type_open(dtype, pprocs, dname, 0, w, h, xdpi, ydpi)

#define std_device_full_body_type(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, ncomp, depth, mg, mc, dg, dc, xoff, yoff, lm, bm, rm, tm)\
	std_device_part1_(dtype, pprocs, dname, stype, open_init_closed),\
	dci_values(ncomp, depth, mg, mc, dg, dc),\
	std_device_part2_(w, h, xdpi, ydpi),\
	offset_margin_values(xoff, yoff, lm, bm, rm, tm),\
	std_device_part3_()

#define std_device_full_body_type_extended(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, mcomp, ncomp, pol, depth, gi, mg, mc, dg, dc, ef, cn, xoff, yoff, lm, bm, rm, tm)\
	std_device_part1_(dtype, pprocs, dname, stype, open_init_closed),\
        dci_extended_alpha_values(mcomp, ncomp, pol, depth, gi, mg, mc, dg, dc, 1, 1, ef, cn), \
	std_device_part2_(w, h, xdpi, ydpi),\
	offset_margin_values(xoff, yoff, lm, bm, rm, tm),\
	std_device_part3_()

#define std_device_full_body(dtype, pprocs, dname, w, h, xdpi, ydpi, ncomp, depth, mg, mc, dg, dc, xoff, yoff, lm, bm, rm, tm)\
	std_device_full_body_type(dtype, pprocs, dname, 0, w, h, xdpi, ydpi,\
	    ncomp, depth, mg, mc, dg, dc, xoff, yoff, lm, bm, rm, tm)

#define std_device_dci_alpha_type_body(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, ncomp, depth, mg, mc, dg, dc, ta, ga)\
	std_device_part1_(dtype, pprocs, dname, stype, open_init_closed),\
	dci_alpha_values(ncomp, depth, mg, mc, dg, dc, ta, ga),\
	std_device_part2_(w, h, xdpi, ydpi),\
	offset_margin_values(0, 0, 0, 0, 0, 0),\
	std_device_part3_()

#define std_device_dci_type_body(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, ncomp, depth, mg, mc, dg, dc)\
	std_device_dci_alpha_type_body(dtype, pprocs, dname, stype, w, h,\
	  xdpi, ydpi, ncomp, depth, mg, mc, dg, dc, 1, 1)

#define std_device_dci_body(dtype, pprocs, dname, w, h, xdpi, ydpi, ncomp, depth, mg, mc, dg, dc)\
	std_device_dci_type_body(dtype, pprocs, dname, 0,\
	  w, h, xdpi, ydpi, ncomp, depth, mg, mc, dg, dc)

#define std_device_color_full_body(dtype, pprocs, dname, w, h, xdpi, ydpi, depth, max_value, dither, xoff, yoff, lm, bm, rm, tm)\
	std_device_part1_(dtype, pprocs, dname, 0, open_init_closed),\
	dci_color(depth, max_value, dither),\
	std_device_part2_(w, h, xdpi, ydpi),\
	offset_margin_values(xoff, yoff, lm, bm, rm, tm),\
	std_device_part3_()

#define std_device_color_body(dtype, pprocs, dname, w, h, xdpi, ydpi, depth, max_value, dither)\
	std_device_color_full_body(dtype, pprocs, dname,\
	  w, h, xdpi, ydpi,\
	  depth, max_value, dither,\
	  0, 0, 0, 0, 0, 0)

#define std_device_color_stype_body(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, depth, max_value, dither)\
	std_device_part1_(dtype, pprocs, dname, stype, open_init_closed),\
	dci_color(depth, max_value, dither),\
	std_device_part2_(w, h, xdpi, ydpi),\
	offset_margin_values(0, 0, 0, 0, 0, 0),\
	std_device_part3_()

#define std_device_std_color_full_body_type(dtype, pprocs, dname, stype, w, h, xdpi, ydpi, depth, xoff, yoff, lm, bm, rm, tm)\
	std_device_part1_(dtype, pprocs, dname, stype, open_init_closed),\
	dci_std_color(depth),\
	std_device_part2_(w, h, xdpi, ydpi),\
	offset_margin_values(xoff, yoff, lm, bm, rm, tm),\
	std_device_part3_()

#define std_device_std_color_full_body(dtype, pprocs, dname, w, h, xdpi, ydpi, depth, xoff, yoff, lm, bm, rm, tm)\
	std_device_std_color_full_body_type(dtype, pprocs, dname, 0,\
	    w, h, xdpi, ydpi, depth, xoff, yoff, lm, bm, rm, tm)

/* ---------------- Default implementations ---------------- */

/* Default implementations of optional procedures. */
/* Note that the default map_xxx_color routines assume white_on_black. */
dev_proc_open_device(gx_default_open_device);
dev_proc_get_initial_matrix(gx_default_get_initial_matrix);
dev_proc_get_initial_matrix(gx_upright_get_initial_matrix);
dev_proc_sync_output(gx_default_sync_output);
dev_proc_output_page(gx_default_output_page);
dev_proc_close_device(gx_default_close_device);
dev_proc_map_rgb_color(gx_default_w_b_map_rgb_color);
dev_proc_map_color_rgb(gx_default_w_b_map_color_rgb);
#define gx_default_map_rgb_color gx_default_w_b_map_rgb_color
#define gx_default_map_color_rgb gx_default_w_b_map_color_rgb
dev_proc_tile_rectangle(gx_default_tile_rectangle);
dev_proc_copy_mono(gx_default_copy_mono);
dev_proc_copy_color(gx_default_copy_color);
dev_proc_draw_line(gx_default_draw_line);
dev_proc_get_bits(gx_no_get_bits);	/* gives error */
dev_proc_get_bits(gx_default_get_bits);
dev_proc_get_params(gx_default_get_params);
dev_proc_put_params(gx_default_put_params);
dev_proc_map_cmyk_color(gx_default_map_cmyk_color);
dev_proc_get_xfont_procs(gx_default_get_xfont_procs);
dev_proc_get_xfont_device(gx_default_get_xfont_device);
dev_proc_map_rgb_alpha_color(gx_default_map_rgb_alpha_color);
dev_proc_get_page_device(gx_default_get_page_device);	/* returns NULL */
dev_proc_get_page_device(gx_page_device_get_page_device);	/* returns dev */
dev_proc_get_alpha_bits(gx_default_get_alpha_bits);
dev_proc_copy_alpha(gx_no_copy_alpha);	/* gives error */
dev_proc_copy_alpha(gx_default_copy_alpha);
dev_proc_get_band(gx_default_get_band);
dev_proc_copy_rop(gx_no_copy_rop);	/* gives error */
dev_proc_copy_rop(gx_default_copy_rop);
dev_proc_fill_path(gx_default_fill_path);
dev_proc_stroke_path(gx_default_stroke_path);
dev_proc_fill_mask(gx_default_fill_mask);
dev_proc_fill_trapezoid(gx_default_fill_trapezoid);
dev_proc_fill_parallelogram(gx_default_fill_parallelogram);
dev_proc_fill_triangle(gx_default_fill_triangle);
dev_proc_draw_thin_line(gx_default_draw_thin_line);
dev_proc_begin_image(gx_default_begin_image);
dev_proc_image_data(gx_default_image_data);
dev_proc_end_image(gx_default_end_image);
dev_proc_strip_tile_rectangle(gx_default_strip_tile_rectangle);
dev_proc_strip_copy_rop(gx_no_strip_copy_rop);	/* gives error */
dev_proc_strip_copy_rop(gx_default_strip_copy_rop);
dev_proc_get_clipping_box(gx_default_get_clipping_box);
dev_proc_get_clipping_box(gx_get_largest_clipping_box);
dev_proc_begin_typed_image(gx_default_begin_typed_image);
dev_proc_get_bits_rectangle(gx_no_get_bits_rectangle);	/* gives error */
dev_proc_get_bits_rectangle(gx_default_get_bits_rectangle);
dev_proc_map_color_rgb_alpha(gx_default_map_color_rgb_alpha);
dev_proc_create_compositor(gx_no_create_compositor);
/* default is for ordinary "leaf" devices, null is for */
/* devices that only care about coverage and not contents. */
dev_proc_create_compositor(gx_default_create_compositor);
dev_proc_create_compositor(gx_null_create_compositor);
dev_proc_get_hardware_params(gx_default_get_hardware_params);
dev_proc_text_begin(gx_default_text_begin);
dev_proc_finish_copydevice(gx_default_finish_copydevice);
dev_proc_pattern_manage(gx_default_pattern_manage);
dev_proc_fill_rectangle_hl_color(gx_default_fill_rectangle_hl_color);
dev_proc_include_color_space(gx_default_include_color_space);
dev_proc_fill_linear_color_scanline(gx_default_fill_linear_color_scanline);
dev_proc_fill_linear_color_trapezoid(gx_default_fill_linear_color_trapezoid);
dev_proc_fill_linear_color_triangle(gx_default_fill_linear_color_triangle);
dev_proc_update_spot_equivalent_colors(gx_default_update_spot_equivalent_colors);
/* BACKWARD COMPATIBILITY */
#define gx_non_imaging_create_compositor gx_null_create_compositor

/* Color mapping routines for black-on-white, gray scale, true RGB, */
/* true CMYK, and 1-bit CMYK color. */
dev_proc_map_rgb_color(gx_default_b_w_map_rgb_color);
dev_proc_map_color_rgb(gx_default_b_w_map_color_rgb);
dev_proc_map_rgb_color(gx_default_gray_map_rgb_color);
dev_proc_map_color_rgb(gx_default_gray_map_color_rgb);
dev_proc_map_color_rgb(gx_default_rgb_map_color_rgb);
#define gx_default_cmyk_map_cmyk_color cmyk_8bit_map_cmyk_color /*see below*/
/*
 * The following are defined as "standard" color mapping procedures
 * that can be propagated through device pipelines and that color
 * processing code can test for.
 */
dev_proc_map_rgb_color(gx_default_rgb_map_rgb_color);
dev_proc_map_cmyk_color(cmyk_1bit_map_cmyk_color);
dev_proc_map_color_rgb(cmyk_1bit_map_color_rgb);
dev_proc_decode_color(cmyk_1bit_map_color_cmyk);
dev_proc_map_cmyk_color(cmyk_8bit_map_cmyk_color);
dev_proc_map_color_rgb(cmyk_8bit_map_color_rgb);
dev_proc_decode_color(cmyk_8bit_map_color_cmyk);
dev_proc_encode_color(gx_default_8bit_map_gray_color);
dev_proc_decode_color(gx_default_8bit_map_color_gray);

/* Default implementations for forwarding devices */
dev_proc_close_device(gx_forward_close_device);
dev_proc_get_initial_matrix(gx_forward_get_initial_matrix);
dev_proc_sync_output(gx_forward_sync_output);
dev_proc_output_page(gx_forward_output_page);
dev_proc_map_rgb_color(gx_forward_map_rgb_color);
dev_proc_map_color_rgb(gx_forward_map_color_rgb);
dev_proc_fill_rectangle(gx_forward_fill_rectangle);
dev_proc_tile_rectangle(gx_forward_tile_rectangle);
dev_proc_copy_mono(gx_forward_copy_mono);
dev_proc_copy_color(gx_forward_copy_color);
dev_proc_get_bits(gx_forward_get_bits);
dev_proc_get_params(gx_forward_get_params);
dev_proc_put_params(gx_forward_put_params);
dev_proc_map_cmyk_color(gx_forward_map_cmyk_color);
dev_proc_get_xfont_procs(gx_forward_get_xfont_procs);
dev_proc_get_xfont_device(gx_forward_get_xfont_device);
dev_proc_map_rgb_alpha_color(gx_forward_map_rgb_alpha_color);
dev_proc_get_page_device(gx_forward_get_page_device);
#define gx_forward_get_alpha_bits gx_default_get_alpha_bits
dev_proc_copy_alpha(gx_forward_copy_alpha);
dev_proc_get_band(gx_forward_get_band);
dev_proc_copy_rop(gx_forward_copy_rop);
dev_proc_fill_path(gx_forward_fill_path);
dev_proc_stroke_path(gx_forward_stroke_path);
dev_proc_fill_mask(gx_forward_fill_mask);
dev_proc_fill_trapezoid(gx_forward_fill_trapezoid);
dev_proc_fill_parallelogram(gx_forward_fill_parallelogram);
dev_proc_fill_triangle(gx_forward_fill_triangle);
dev_proc_draw_thin_line(gx_forward_draw_thin_line);
dev_proc_begin_image(gx_forward_begin_image);
#define gx_forward_image_data gx_default_image_data
#define gx_forward_end_image gx_default_end_image
dev_proc_strip_tile_rectangle(gx_forward_strip_tile_rectangle);
dev_proc_strip_copy_rop(gx_forward_strip_copy_rop);
dev_proc_get_clipping_box(gx_forward_get_clipping_box);
dev_proc_begin_typed_image(gx_forward_begin_typed_image);
dev_proc_get_bits_rectangle(gx_forward_get_bits_rectangle);
dev_proc_map_color_rgb_alpha(gx_forward_map_color_rgb_alpha);
/* There is no forward_create_compositor (see Drivers.htm). */
dev_proc_get_hardware_params(gx_forward_get_hardware_params);
dev_proc_text_begin(gx_forward_text_begin);
dev_proc_get_color_mapping_procs(gx_forward_get_color_mapping_procs);
dev_proc_get_color_comp_index(gx_forward_get_color_comp_index);
dev_proc_encode_color(gx_forward_encode_color);
dev_proc_decode_color(gx_forward_decode_color);
dev_proc_pattern_manage(gx_forward_pattern_manage);
dev_proc_fill_rectangle_hl_color(gx_forward_fill_rectangle_hl_color);
dev_proc_include_color_space(gx_forward_include_color_space);
dev_proc_fill_linear_color_scanline(gx_forward_fill_linear_color_scanline);
dev_proc_fill_linear_color_trapezoid(gx_forward_fill_linear_color_trapezoid);
dev_proc_fill_linear_color_triangle(gx_forward_fill_linear_color_triangle);
dev_proc_update_spot_equivalent_colors(gx_forward_update_spot_equivalent_colors);

/* ---------------- Implementation utilities ---------------- */

/* Convert the device procedures to the proper form (see above). */
void gx_device_set_procs(gx_device *);

/* Fill in defaulted procedures in a device procedure record. */
void gx_device_fill_in_procs(gx_device *);
void gx_device_forward_fill_in_procs(gx_device_forward *);

/* Forward the color mapping procedures from a device to its target. */
void gx_device_forward_color_procs(gx_device_forward *);

/*
 * Check if the device's encode_color routine uses 'separable' bit encodings
 * for each colorant.  For more info see the routine's header.
 */
void check_device_separable(gx_device * dev);
/*
 * If a device has a linear and separable encode color function then
 * set up the comp_bits, comp_mask, and comp_shift fields.
 */
void set_linear_color_bits_mask_shift(gx_device * dev);
/*
 * Copy the color mapping procedures from the target if they are
 * standard ones (saving a level of procedure call at mapping time).
 */
void gx_device_copy_color_procs(gx_device *dev, const gx_device *target);

/* Get the black and white pixel values of a device. */
gx_color_index gx_device_black(gx_device *dev);
#define gx_device_black_inline(dev)\
  ((dev)->cached_colors.black == gx_no_color_index ?\
   gx_device_black(dev) : (dev)->cached_colors.black)
gx_color_index gx_device_white(gx_device *dev);
#define gx_device_white_inline(dev)\
  ((dev)->cached_colors.white == gx_no_color_index ?\
   gx_device_white(dev) : (dev)->cached_colors.white)

/* Clear the black/white pixel cache. */
void gx_device_decache_colors(gx_device *dev);

/*
 * Copy the color-related device parameters back from the target:
 * color_info and color mapping procedures.
 */
void gx_device_copy_color_params(gx_device *dev, const gx_device *target);

/*
 * Copy device parameters back from a target.  This copies all standard
 * parameters related to page size and resolution, plus color_info
 * and (if appropriate) color mapping procedures.
 */
void gx_device_copy_params(gx_device *dev, const gx_device *target);

/*
 * Parse the output file name for a device, recognizing "-" and "|command",
 * and also detecting and validating any %nnd format for inserting the
 * page count.  If a format is present, store a pointer to its last
 * character in *pfmt, otherwise store 0 there.  Note that an empty name
 * is currently allowed.
 */
int gx_parse_output_file_name(gs_parsed_file_name_t *pfn,
			      const char **pfmt, const char *fname,
			      uint len);

/*
 * Open the output file for a device.  Note that if the file name is empty,
 * it may be replaced with the name of a scratch file.
 */
int gx_device_open_output_file(const gx_device * dev, char *fname,
			       bool binary, bool positionable,
			       FILE ** pfile);

/* Close the output file for a device. */
int gx_device_close_output_file(const gx_device * dev, const char *fname,
				FILE *file);

/*
 * Define the number of levels for a colorant above which we do not halftone.
 */
#define MIN_CONTONE_LEVELS 31

/*
 * Determine whether a given device needs to halftone.  Eventually this
 * should take an imager state as an additional argument.
 */
#define gx_device_must_halftone(dev)\
  ((gx_device_has_color(dev) ? (dev)->color_info.max_color :\
    (dev)->color_info.max_gray) < MIN_CONTONE_LEVELS)

/*
 * Do generic work for output_page.  All output_page procedures must call
 * this as the last thing they do, unless an error has occurred earlier.
 */
dev_proc_output_page(gx_finish_output_page);

/*
 * Device procedures that draw into rectangles need to clip the coordinates
 * to the rectangle ((0,0),(dev->width,dev->height)).  The following macros
 * do the clipping.  They assume that the arguments of the procedure are
 * named dev, x, y, w, and h, and may modify the arguments (other than dev).
 *
 * For procedures that fill a region, dev, x, y, w, and h are the only
 * relevant arguments.  For procedures that copy bitmaps, see below.
 *
 * The following group of macros for region-filling procedures clips
 * specific edges of the supplied rectangle, as indicated by the macro name.
 */
#define fit_fill_xy(dev, x, y, w, h)\
  BEGIN\
	if ( (x | y) < 0 ) {\
	  if ( x < 0 )\
	    w += x, x = 0;\
	  if ( y < 0 )\
	    h += y, y = 0;\
	}\
  END
#define fit_fill_y(dev, y, h)\
  BEGIN\
	if ( y < 0 )\
	  h += y, y = 0;\
  END
#define fit_fill_w(dev, x, w)\
  BEGIN\
	if ( w > (dev)->width - x )\
	  w = (dev)->width - x;\
  END
#define fit_fill_h(dev, y, h)\
  BEGIN\
	if ( h > (dev)->height - y )\
	  h = (dev)->height - y;\
  END
#define fit_fill_xywh(dev, x, y, w, h)\
  BEGIN\
	fit_fill_xy(dev, x, y, w, h);\
	fit_fill_w(dev, x, w);\
	fit_fill_h(dev, y, h);\
  END
/*
 * Clip all edges, and return from the procedure if the result is empty.
 */
#define fit_fill(dev, x, y, w, h)\
  BEGIN\
	fit_fill_xywh(dev, x, y, w, h);\
	if ( w <= 0 || h <= 0 )\
	  return 0;\
  END

/*
 * For driver procedures that copy bitmaps (e.g., copy_mono, copy_color),
 * clipping the destination region also may require adjusting the pointer to
 * the source data.  In addition to dev, x, y, w, and h, the clipping macros
 * for these procedures reference data, data_x, raster, and id; they may
 * modify the values of data, data_x, and id.
 *
 * Clip the edges indicated by the macro name.
 */
#define fit_copy_xyw(dev, data, data_x, raster, id, x, y, w, h)\
  BEGIN\
	if ( (x | y) < 0 ) {\
	  if ( x < 0 )\
	    w += x, data_x -= x, x = 0;\
	  if ( y < 0 )\
	    h += y, data -= y * raster, id = gx_no_bitmap_id, y = 0;\
	}\
	if ( w > (dev)->width - x )\
	  w = (dev)->width - x;\
  END
/*
 * Clip all edges, and return from the procedure if the result is empty.
 */
#define fit_copy(dev, data, data_x, raster, id, x, y, w, h)\
  BEGIN\
	fit_copy_xyw(dev, data, data_x, raster, id, x, y, w, h);\
	if ( h > (dev)->height - y )\
	  h = (dev)->height - y;\
	if ( w <= 0 || h <= 0 )\
	  return 0;\
  END

/* ---------------- Media parameters ---------------- */

/* Define the InputAttributes and OutputAttributes of a device. */
/* The device get_params procedure would call these. */

typedef struct gdev_input_media_s {
    float PageSize[4];		/* nota bene */
    const char *MediaColor;
    float MediaWeight;
    const char *MediaType;
} gdev_input_media_t;

#define gdev_input_media_default_values { 0, 0, 0, 0 }, 0, 0, 0
extern const gdev_input_media_t gdev_input_media_default;

void gdev_input_media_init(gdev_input_media_t * pim);

int gdev_begin_input_media(gs_param_list * mlist, gs_param_dict * pdict,
			   int count);

int gdev_write_input_page_size(int index, gs_param_dict * pdict,
			       floatp width_points, floatp height_points);

int gdev_write_input_media(int index, gs_param_dict * pdict,
			   const gdev_input_media_t * pim);

int gdev_end_input_media(gs_param_list * mlist, gs_param_dict * pdict);

typedef struct gdev_output_media_s {
    const char *OutputType;
} gdev_output_media_t;

#define gdev_output_media_default_values 0
extern const gdev_output_media_t gdev_output_media_default;

int gdev_begin_output_media(gs_param_list * mlist, gs_param_dict * pdict,
			    int count);

int gdev_write_output_media(int index, gs_param_dict * pdict,
			    const gdev_output_media_t * pom);

int gdev_end_output_media(gs_param_list * mlist, gs_param_dict * pdict);

#endif /* gxdevice_INCLUDED */
