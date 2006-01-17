/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpbm.c,v 1.11 2004/05/26 04:10:58 dan Exp $ */
/* Portable Bit/Gray/PixMap drivers */
#include "gdevprn.h"
#include "gscdefs.h"
#include "gscspace.h" /* For pnm_begin_typed_image(..) */
#include "gxgetbit.h"
#include "gxlum.h"
#include "gxiparam.h" /* For pnm_begin_typed_image(..) */
#include "gdevmpla.h"
#include "gdevplnx.h"
#include "gdevppla.h"

/*
 * Thanks are due to Jos Vos (jos@bull.nl) for an earlier P*M driver,
 * on which this one is based; to Nigel Roles (ngr@cotswold.demon.co.uk),
 * for the plan9bm changes; and to Leon Bottou (leonb@research.att.com)
 * for the color detection code in pnm_begin_typed_image.
 */

/*
 * There are 8 (families of) drivers here, plus one less related one:
 *      pbm[raw] - outputs PBM (black and white).
 *      pgm[raw] - outputs PGM (gray-scale).
 *      pgnm[raw] - outputs PBM if the page contains only black and white,
 *        otherwise PGM.
 *      ppm[raw] - outputs PPM (RGB).
 *      pnm[raw] - outputs PBM if the page contains only black and white,
 *        otherwise PGM if the page contains only gray shades,
 *        otherwise PPM.
 *      pkm[raw] - computes internally in CMYK, outputs PPM (RGB).
 *      pksm[raw] - computes internally in CMYK, outputs 4 PBM pages.
 *      pam - outputs CMYK as PAM
 *      plan9bm - outputs Plan 9 bitmap format.
 */

/*
 * The code here is designed to work with variable depths for PGM and PPM.
 * The code will work with any of the values in brackets, but the
 * Ghostscript imager requires that depth be a power of 2 or be 24,
 * so the actual allowed values are more limited.
 *      pgm, pgnm: 1, 2, 4, 8, 16.  [1-16]
 *      pgmraw, pgnmraw: 1, 2, 4, 8.  [1-8]
 *      ppm, pnm: 4(3x1), 8(3x2), 16(3x5), 24(3x8), 32(3x10).  [3-32]
 *      ppmraw, pnmraw: 4(3x1), 8(3x2), 16(3x5), 24(3x8).  [3-24]
 *      pkm, pkmraw: 4(4x1), 8(4x2), 16(4x4), 32(4x8).  [4-32]
 *	pksm, pksmraw: ibid.
 *      pam: 32 (CMYK)
 */

/* Structure for P*M devices, which extend the generic printer device. */

#define MAX_COMMENT 70		/* max user-supplied comment */
struct gx_device_pbm_s {
    gx_device_common;
    gx_prn_device_common;
    /* Additional state for P*M devices */
    char magic;			/* n for "Pn" */
    char comment[MAX_COMMENT + 1];	/* comment for head of file */
    byte is_raw;		/* 1 if raw format, 0 if plain */
    byte optimize;		/* 1 if optimization OK, 0 if not */
    byte uses_color;		/* 0 if image is black and white, */
				/* 1 if gray (PGM or PPM only), */
				/* 2 or 3 if colored (PPM only) */
    bool UsePlanarBuffer;	/* 0 if chunky buffer, 1 if planar */
    dev_proc_copy_alpha((*save_copy_alpha));
    dev_proc_begin_typed_image((*save_begin_typed_image));
};
typedef struct gx_device_pbm_s gx_device_pbm;

/* ------ The device descriptors ------ */

/*
 * Default X and Y resolution.
 */
#define X_DPI 72
#define Y_DPI 72

/* Macro for generating P*M device descriptors. */
#define pbm_prn_device(procs, dev_name, magic, is_raw, num_comp, depth, max_gray, max_rgb, optimize, x_dpi, y_dpi, print_page)\
{	prn_device_body(gx_device_pbm, procs, dev_name,\
	  DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS, x_dpi, y_dpi,\
	  0, 0, 0, 0,\
	  num_comp, depth, max_gray, max_rgb, max_gray + 1, max_rgb + 1,\
	  print_page),\
	magic,\
	 { 0 },\
	is_raw,\
	optimize,\
	0, 0, 0\
}

/* For all but PBM, we need our own color mapping and alpha procedures. */
private dev_proc_map_rgb_color(pgm_map_rgb_color);
private dev_proc_map_rgb_color(ppm_map_rgb_color);
private dev_proc_map_color_rgb(pgm_map_color_rgb);
private dev_proc_map_color_rgb(ppm_map_color_rgb);
private dev_proc_map_cmyk_color(pkm_map_cmyk_color);
private dev_proc_map_color_rgb(pkm_map_color_rgb);
private dev_proc_get_params(ppm_get_params);
private dev_proc_put_params(ppm_put_params);
private dev_proc_copy_alpha(pnm_copy_alpha);
private dev_proc_begin_typed_image(pnm_begin_typed_image);

/* We need to initialize uses_color when opening the device, */
/* and after each showpage. */
private dev_proc_open_device(ppm_open);
private dev_proc_output_page(ppm_output_page);

/* And of course we need our own print-page routines. */
private dev_proc_print_page(pbm_print_page);
private dev_proc_print_page(pgm_print_page);
private dev_proc_print_page(ppm_print_page);
private dev_proc_print_page(pkm_print_page);
private dev_proc_print_page(psm_print_page);
private dev_proc_print_page(psm_print_page);
private dev_proc_print_page(pam_print_page);

private int pam_print_row(gx_device_printer * pdev, byte * data, int depth,
	       FILE * pstream);
private int pam_print_page(gx_device_printer * pdev, FILE * pstream);

/* The device procedures */

/* See gdevprn.h for the template for the following. */
#define pgpm_procs(p_open, p_get_params, p_map_rgb_color, p_map_color_rgb, p_map_cmyk_color) {\
	p_open, NULL, NULL, ppm_output_page, gdev_prn_close,\
	p_map_rgb_color, p_map_color_rgb, NULL, NULL, NULL, NULL, NULL, NULL,\
	p_get_params, ppm_put_params,\
	p_map_cmyk_color, NULL, NULL, NULL, gx_page_device_get_page_device\
}

private const gx_device_procs pbm_procs =
    pgpm_procs(gdev_prn_open, gdev_prn_get_params,
	       gdev_prn_map_rgb_color, gdev_prn_map_color_rgb, NULL);
private const gx_device_procs pgm_procs =
    pgpm_procs(ppm_open, gdev_prn_get_params,
	       pgm_map_rgb_color, pgm_map_color_rgb, NULL);
private const gx_device_procs ppm_procs =
    pgpm_procs(ppm_open, ppm_get_params,
	       gx_default_rgb_map_rgb_color, ppm_map_color_rgb, NULL);
private const gx_device_procs pnm_procs =
    pgpm_procs(ppm_open, ppm_get_params,
	       ppm_map_rgb_color, ppm_map_color_rgb, NULL);
private const gx_device_procs pkm_procs =
    pgpm_procs(ppm_open, ppm_get_params,
	       NULL, cmyk_1bit_map_color_rgb, cmyk_1bit_map_cmyk_color);
private const gx_device_procs pam_procs =
    pgpm_procs(ppm_open, ppm_get_params,
	       NULL, cmyk_8bit_map_color_rgb, cmyk_8bit_map_cmyk_color);

/* The device descriptors themselves */
const gx_device_pbm gs_pbm_device =
pbm_prn_device(pbm_procs, "pbm", '1', 0, 1, 1, 1, 0, 0,
	       X_DPI, Y_DPI, pbm_print_page);
const gx_device_pbm gs_pbmraw_device =
pbm_prn_device(pbm_procs, "pbmraw", '4', 1, 1, 1, 1, 1, 0,
	       X_DPI, Y_DPI, pbm_print_page);
const gx_device_pbm gs_pgm_device =
pbm_prn_device(pgm_procs, "pgm", '2', 0, 1, 8, 255, 0, 0,
	       X_DPI, Y_DPI, pgm_print_page);
const gx_device_pbm gs_pgmraw_device =
pbm_prn_device(pgm_procs, "pgmraw", '5', 1, 1, 8, 255, 0, 0,
	       X_DPI, Y_DPI, pgm_print_page);
const gx_device_pbm gs_pgnm_device =
pbm_prn_device(pgm_procs, "pgnm", '2', 0, 1, 8, 255, 0, 1,
	       X_DPI, Y_DPI, pgm_print_page);
const gx_device_pbm gs_pgnmraw_device =
pbm_prn_device(pgm_procs, "pgnmraw", '5', 1, 1, 8, 255, 0, 1,
	       X_DPI, Y_DPI, pgm_print_page);
const gx_device_pbm gs_ppm_device =
pbm_prn_device(ppm_procs, "ppm", '3', 0, 3, 24, 255, 255, 0,
	       X_DPI, Y_DPI, ppm_print_page);
const gx_device_pbm gs_ppmraw_device =
pbm_prn_device(ppm_procs, "ppmraw", '6', 1, 3, 24, 255, 255, 0,
	       X_DPI, Y_DPI, ppm_print_page);
const gx_device_pbm gs_pnm_device =
pbm_prn_device(pnm_procs, "pnm", '3', 0, 3, 24, 255, 255, 1,
	       X_DPI, Y_DPI, ppm_print_page);
const gx_device_pbm gs_pnmraw_device =
pbm_prn_device(pnm_procs, "pnmraw", '6', 1, 3, 24, 255, 255, 1,
	       X_DPI, Y_DPI, ppm_print_page);
const gx_device_pbm gs_pkm_device =
pbm_prn_device(pkm_procs, "pkm", '3', 0, 4, 4, 1, 1, 0,
	       X_DPI, Y_DPI, pkm_print_page);
const gx_device_pbm gs_pkmraw_device =
pbm_prn_device(pkm_procs, "pkmraw", '6', 1, 4, 4, 1, 1, 0,
	       X_DPI, Y_DPI, pkm_print_page);
const gx_device_pbm gs_pksm_device =
pbm_prn_device(pkm_procs, "pksm", '1', 0, 4, 4, 1, 1, 0,
	       X_DPI, Y_DPI, psm_print_page);
const gx_device_pbm gs_pksmraw_device =
pbm_prn_device(pkm_procs, "pksmraw", '4', 1, 4, 4, 1, 1, 0,
	       X_DPI, Y_DPI, psm_print_page);
const gx_device_pbm gs_pam_device =
pbm_prn_device(pam_procs, "pam", '7', 1, 4, 32, 255, 255, 0,
	       X_DPI, Y_DPI, pam_print_page);

/* Plan 9 bitmaps default to 100 dpi. */
const gx_device_pbm gs_plan9bm_device =
pbm_prn_device(pbm_procs, "plan9bm", '9', 1, 1, 1, 1, 1, 1,
	       100, 100, pbm_print_page);

/* ------ Initialization ------ */

/* Set the copy_alpha and color mapping procedures if necessary. */
private void
ppm_set_dev_procs(gx_device * pdev)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    if (dev_proc(pdev, copy_alpha) != pnm_copy_alpha) {
	bdev->save_copy_alpha = dev_proc(pdev, copy_alpha);
	if (pdev->color_info.depth > 4)
	    set_dev_proc(pdev, copy_alpha, pnm_copy_alpha);
    }
    if (dev_proc(pdev, begin_typed_image) != pnm_begin_typed_image) {
        bdev->save_begin_typed_image = dev_proc(pdev, begin_typed_image);
	set_dev_proc(pdev, begin_typed_image, pnm_begin_typed_image);
    }
    if (bdev->color_info.num_components == 4) {
	if (bdev->magic == 7) {
	    set_dev_proc(pdev, map_color_rgb, cmyk_8bit_map_color_rgb);
	    set_dev_proc(pdev, map_cmyk_color, cmyk_8bit_map_cmyk_color);
	} else if (bdev->color_info.depth == 4) {
	    set_dev_proc(pdev, map_color_rgb, cmyk_1bit_map_color_rgb);
	    set_dev_proc(pdev, map_cmyk_color, cmyk_1bit_map_cmyk_color);
	} else {
	    set_dev_proc(pdev, map_color_rgb, pkm_map_color_rgb);
	    set_dev_proc(pdev, map_cmyk_color, pkm_map_cmyk_color);
	}
    }
}

/*
 * Define a special open procedure that changes create_buf_device to use
 * a planar device.
 */

private int
ppm_open(gx_device * pdev)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    int code = gdev_prn_open_planar(pdev, bdev->UsePlanarBuffer);

    if (code < 0)
	return code;
    pdev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
    set_linear_color_bits_mask_shift(pdev);
    bdev->uses_color = 0;
    ppm_set_dev_procs(pdev);
    return code;
}

/* Print a page, and reset uses_color if this is a showpage. */
private int
ppm_output_page(gx_device * pdev, int num_copies, int flush)
{
    int code = gdev_prn_output_page(pdev, num_copies, flush);
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    if (code < 0)
	return code;
    if (flush)
	bdev->uses_color = 0;
    return code;
}

/* ------ Color mapping routines ------ */

/* Map an RGB color to a PGM gray value. */
/* Keep track of whether the image is black-and-white or gray. */
private gx_color_index
pgm_map_rgb_color(gx_device * pdev, const gx_color_value cv[])
{				/* We round the value rather than truncating it. */
    gx_color_value gray;
    /* TO_DO_DEVICEN  - Kludge to emulate pre DeviceN math errors */
#if 1
    gx_color_value r, g, b;

    r = cv[0]; g = cv[0]; b = cv[0];
    gray = ((r * (ulong) lum_red_weight) +
     (g * (ulong) lum_green_weight) +
     (b * (ulong) lum_blue_weight) +
     (lum_all_weights / 2)) / lum_all_weights
    * pdev->color_info.max_gray / gx_max_color_value;
#else	    /* Should be ... */
    gray = cv[0] * pdev->color_info.max_gray / gx_max_color_value;
#endif

    if (!(gray == 0 || gray == pdev->color_info.max_gray)) {
	gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

	bdev->uses_color = 1;
    }
    return gray;
}

/* Map a PGM gray value back to an RGB color. */
private int
pgm_map_color_rgb(gx_device * dev, gx_color_index color,
		  gx_color_value prgb[3])
{
    gx_color_value gray =
    color * gx_max_color_value / dev->color_info.max_gray;

    prgb[0] = gray;
    prgb[1] = gray;
    prgb[2] = gray;
    return 0;
}

/*
 * Pre gs8.00 version of RGB mapping for 24-bit true (RGB) color devices
 * It is kept here for backwards comparibility since the gs8.00 version
 * has changed in functionality.  The new one requires that the device be
 * 'separable'.  This routine is logically separable but does not require
 * the various color_info fields associated with separability (comp_shift,
 * comp_bits, and comp_mask) be setup.
 */

private gx_color_index
gx_old_default_rgb_map_rgb_color(gx_device * dev,
		       gx_color_value r, gx_color_value g, gx_color_value b)
{
    if (dev->color_info.depth == 24)
	return gx_color_value_to_byte(b) +
	    ((uint) gx_color_value_to_byte(g) << 8) +
	    ((ulong) gx_color_value_to_byte(r) << 16);
    else {
	int bpc = dev->color_info.depth / 3;
	int drop = sizeof(gx_color_value) * 8 - bpc;

	return ((((r >> drop) << bpc) + (g >> drop)) << bpc) + (b >> drop);
    }
}

/* Map an RGB color to a PPM color tuple. */
/* Keep track of whether the image is black-and-white, gray, or colored. */
private gx_color_index
ppm_map_rgb_color(gx_device * pdev, const gx_color_value cv[])
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    gx_color_index color = 
	    gx_old_default_rgb_map_rgb_color(pdev, cv[0], cv[1], cv[2]);
    uint bpc = pdev->color_info.depth / 3;
    gx_color_index mask =
	((gx_color_index)1 << (pdev->color_info.depth - bpc)) - 1;
    if (!(((color >> bpc) ^ color) & mask)) { /* gray shade */
	if (color != 0 && (~color & mask))
	    bdev->uses_color |= 1;
    } else			/* color */
	bdev->uses_color = 2;
    return color;
}

/* Map a PPM color tuple back to an RGB color. */
private int
ppm_map_color_rgb(gx_device * dev, gx_color_index color,
		  gx_color_value prgb[3])
{
    uint bitspercolor = dev->color_info.depth / 3;
    uint colormask = (1 << bitspercolor) - 1;
    uint max_rgb = dev->color_info.max_color;

    prgb[0] = ((color >> (bitspercolor * 2)) & colormask) *
	(ulong) gx_max_color_value / max_rgb;
    prgb[1] = ((color >> bitspercolor) & colormask) *
	(ulong) gx_max_color_value / max_rgb;
    prgb[2] = (color & colormask) *
	(ulong) gx_max_color_value / max_rgb;
    return 0;
}

/* Map a CMYK color to a pixel value. */
private gx_color_index
pkm_map_cmyk_color(gx_device * pdev, const gx_color_value cv[])
{
    uint bpc = pdev->color_info.depth >> 2;
    uint max_value = pdev->color_info.max_color;
    uint cc = cv[0] * max_value / gx_max_color_value;
    uint mc = cv[1] * max_value / gx_max_color_value;
    uint yc = cv[2] * max_value / gx_max_color_value;
    uint kc = cv[3] * max_value / gx_max_color_value;
    gx_color_index color =
	(((((cc << bpc) + mc) << bpc) + yc) << bpc) + kc;

    return (color == gx_no_color_index ? color ^ 1 : color);
}

/* Map a CMYK pixel value to RGB. */
private int
pkm_map_color_rgb(gx_device * dev, gx_color_index color, gx_color_value rgb[3])
{
    int bpc = dev->color_info.depth >> 2;
    gx_color_index cshift = color;
    uint mask = (1 << bpc) - 1;
    uint k = cshift & mask;
    uint y = (cshift >>= bpc) & mask;
    uint m = (cshift >>= bpc) & mask;
    uint c = cshift >> bpc;
    uint max_value = dev->color_info.max_color;
    uint not_k = max_value - k;

#define CVALUE(c)\
  ((gx_color_value)((ulong)(c) * gx_max_color_value / max_value))
	/* We use our improved conversion rule.... */
	rgb[0] = CVALUE((max_value - c) * not_k / max_value);
    rgb[1] = CVALUE((max_value - m) * not_k / max_value);
    rgb[2] = CVALUE((max_value - y) * not_k / max_value);
#undef CVALUE
    return 0;
}

/* Augment get/put_params to add UsePlanarBuffer */

private int
ppm_get_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    return gdev_prn_get_params_planar(pdev, plist, &bdev->UsePlanarBuffer);
}

private int
ppm_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    gx_device_color_info save_info;
    int ncomps = pdev->color_info.num_components;
    int bpc = pdev->color_info.depth / ncomps;
    int ecode = 0;
    int code;
    long v;
    const char *vname;

    save_info = pdev->color_info;
    if ((code = param_read_long(plist, (vname = "GrayValues"), &v)) != 1 ||
	(code = param_read_long(plist, (vname = "RedValues"), &v)) != 1 ||
	(code = param_read_long(plist, (vname = "GreenValues"), &v)) != 1 ||
	(code = param_read_long(plist, (vname = "BlueValues"), &v)) != 1
	) {
	if (code < 0)
	    ecode = code;
	else if (v < 2 || v > (bdev->is_raw || ncomps > 1 ? 256 : 65536L))
	    param_signal_error(plist, vname,
			       ecode = gs_error_rangecheck);
	else if (v == 2)
	    bpc = 1;
	else if (v <= 4)
	    bpc = 2;
	else if (v <= 16)
	    bpc = 4;
	else if (v <= 32 && ncomps == 3)
	    bpc = 5;
	else if (v <= 256)
	    bpc = 8;
	else
	    bpc = 16;
	if (ecode >= 0) {
	    static const byte depths[4][16] =
	    {
		{1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16},
		{0},
		{4, 8, 0, 16, 16, 0, 0, 24},
		{4, 8, 0, 16, 0, 0, 0, 32},
	    };

	    pdev->color_info.depth = depths[ncomps - 1][bpc - 1];
	    pdev->color_info.max_gray = pdev->color_info.max_color =
		(pdev->color_info.dither_grays =
		 pdev->color_info.dither_colors = (int)v) - 1;
	}
    }
    if ((code = ecode) < 0 ||
	(code = gdev_prn_put_params_planar(pdev, plist, &bdev->UsePlanarBuffer)) < 0
	)
	pdev->color_info = save_info;
    ppm_set_dev_procs(pdev);
    return code;
}

/* Copy an alpha map, noting whether we may generate some non-black/white */
/* colors through blending. */
private int
pnm_copy_alpha(gx_device * pdev, const byte * data, int data_x,
	   int raster, gx_bitmap_id id, int x, int y, int width, int height,
	       gx_color_index color, int depth)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    if (pdev->color_info.depth < 24 ||
	(color >> 8) == (color & 0xffff)
	)
	bdev->uses_color |= 1;
    else
	bdev->uses_color |= 2;
    return (*bdev->save_copy_alpha) (pdev, data, data_x, raster, id,
				     x, y, width, height, color, depth);
}

/* Begin processing an image, noting whether we may generate some */
/* non-black/white colors in the process. */
private int 
pnm_begin_typed_image(gx_device *dev,
                      const gs_imager_state *pis, const gs_matrix *pmat,
                      const gs_image_common_t *pim, const gs_int_rect *prect,
                      const gx_drawing_color *pdcolor,
		      const gx_clip_path *pcpath,
                      gs_memory_t *memory, gx_image_enum_common_t **pinfo)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)dev;

    /* Conservatively guesses whether this operation causes color usage 
       that might not be otherwise captured by ppm_map_color_rgb. */
    if (pim && pim->type) {
	switch (pim->type->index) {
	case 1: case 3: case 4: {
	    /* Use colorspace to handle image types 1,3,4 */
	    const gs_pixel_image_t *pim1 = (const gs_pixel_image_t *)pim;

	    if (pim1->ColorSpace) {
		if (gs_color_space_get_index(pim1->ColorSpace) == gs_color_space_index_DeviceGray) {
		    if (pim1->BitsPerComponent > 1)
			bdev->uses_color |= 1;
		} else 
		    bdev->uses_color = 2;
	    }
	    break;
	}
	default:
	    /* Conservatively handles other image types */
	    bdev->uses_color = 2;
	}
    }
    /* Forward to saved routine */
    return (*bdev->save_begin_typed_image)(dev, pis, pmat, pim, prect, 
					   pdcolor, pcpath, memory, pinfo);
}


/* ------ Internal routines ------ */

/* Print a page using a given row printing routine. */
private int
pbm_print_page_loop(gx_device_printer * pdev, char magic, FILE * pstream,
	     int (*row_proc) (gx_device_printer *, byte *, int, FILE *))
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    uint raster = gdev_prn_raster(pdev);
    byte *data = gs_alloc_bytes(pdev->memory, raster, "pbm_print_page_loop");
    int lnum = 0;
    int code = 0;

    if (data == 0)
	return_error(gs_error_VMerror);
    /* Hack.  This should be done in the callers.  */
    if (magic == '9')
	fprintf(pstream, "%11d %11d %11d %11d %11d ",
		0, 0, 0, pdev->width, pdev->height);
    else if (magic == '7') {
	int ncomps = pdev->color_info.num_components;
	fprintf(pstream, "P%c\n", magic);
	fprintf(pstream, "WIDTH %d\n", pdev->width);
	fprintf(pstream, "HEIGHT %d\n", pdev->height);
	fprintf(pstream, "DEPTH %d\n", ncomps);
	fprintf(pstream, "MAXVAL %d\n", pdev->color_info.max_gray);
	fprintf(pstream, "TUPLTYPE %s\n",
	    (ncomps == 4) ? "CMYK" :
	    ((ncomps == 3) ? "RGB" : "GRAYSCALE"));
	if (bdev->comment[0])
	    fprintf(pstream, "# %s\n", bdev->comment);
	else
	    fprintf(pstream, "# Image generated by %s (device=%s)\n",
		    gs_product, pdev->dname);
	fprintf(pstream, "ENDHDR\n");
    }
    else {
	fprintf(pstream, "P%c\n", magic);
	if (bdev->comment[0])
	    fprintf(pstream, "# %s\n", bdev->comment);
	else
	    fprintf(pstream, "# Image generated by %s (device=%s)\n",
		    gs_product, pdev->dname);
	fprintf(pstream, "%d %d\n", pdev->width, pdev->height);
    }
    switch (magic) {
	case '1':		/* pbm */
	case '4':		/* pbmraw */
	case '7':		/* pam */
	case '9':		/* plan9bm */
	    break;
	default:
	    fprintf(pstream, "%d\n", pdev->color_info.max_gray);
    }
    for (; lnum < pdev->height; lnum++) {
	byte *row;

	code = gdev_prn_get_bits(pdev, lnum, data, &row);
	if (code < 0)
	    break;
	code = (*row_proc) (pdev, row, pdev->color_info.depth, pstream);
	if (code < 0)
	    break;
    }
    gs_free_object(pdev->memory, data, "pbm_print_page_loop");
    return (code < 0 ? code : 0);
}

/* ------ Individual page printing routines ------ */

/* Print a monobit page. */
private int
pbm_print_row(gx_device_printer * pdev, byte * data, int depth,
	      FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    if (bdev->is_raw)
	fwrite(data, 1, (pdev->width + 7) >> 3, pstream);
    else {
	byte *bp;
	uint x, mask;

	for (bp = data, x = 0, mask = 0x80; x < pdev->width;) {
	    putc((*bp & mask ? '1' : '0'), pstream);
	    if (++x == pdev->width || !(x & 63))
		putc('\n', pstream);
	    if ((mask >>= 1) == 0)
		bp++, mask = 0x80;
	}
    }
    return 0;
}
private int
pbm_print_page(gx_device_printer * pdev, FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    return pbm_print_page_loop(pdev, bdev->magic, pstream, pbm_print_row);
}

/* Print a gray-mapped page. */
private int
pgm_print_row(gx_device_printer * pdev, byte * data, int depth,
	      FILE * pstream)
{				/* Note that bpp <= 8 for raw format, bpp <= 16 for plain. */
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    uint mask = (1 << depth) - 1;
    /*
     * If we're writing planes for a CMYK device, we have 0 = white,
     * mask = black, which is the opposite of the pgm convention.
     */
    uint invert = (pdev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE);
    byte *bp;
    uint x;
    int shift;

    if (bdev->is_raw && depth == 8) {
	if (invert) {
	    for (bp = data, x = 0; x < pdev->width; bp++, x++)
		putc((byte)~*bp, pstream);
	} else
	    fwrite(data, 1, pdev->width, pstream);
    } else
	for (bp = data, x = 0, shift = 8 - depth; x < pdev->width;) {
	    uint pixel;

	    if (shift < 0) {	/* bpp = 16 */
		pixel = ((uint) * bp << 8) + bp[1];
		bp += 2;
	    } else {
		pixel = (*bp >> shift) & mask;
		if ((shift -= depth) < 0)
		    bp++, shift += 8;
	    }
	    ++x;
	    pixel ^= invert;
	    if (bdev->is_raw)
		putc(pixel, pstream);
	    else
		fprintf(pstream, "%d%c", pixel,
			(x == pdev->width || !(x & 15) ? '\n' : ' '));
	}
    return 0;
}
private int
pxm_pbm_print_row(gx_device_printer * pdev, byte * data, int depth,
		  FILE * pstream)
{				/* Compress a PGM or PPM row to a PBM row. */
    /* This doesn't have to be very fast. */
    /* Note that we have to invert the data as well. */
    int delta = (depth + 7) >> 3;
    byte *src = data + delta - 1;	/* always big-endian */
    byte *dest = data;
    int x;
    byte out_mask = 0x80;
    byte out = 0;

    if (depth >= 8) {		/* One or more bytes per source pixel. */
	for (x = 0; x < pdev->width; x++, src += delta) {
	    if (!(*src & 1))
		out |= out_mask;
	    out_mask >>= 1;
	    if (!out_mask)
		out_mask = 0x80,
		    *dest++ = out,
		    out = 0;
	}
    } else {			/* Multiple source pixels per byte. */
	byte in_mask = 0x100 >> depth;

	for (x = 0; x < pdev->width; x++) {
	    if (!(*src & in_mask))
		out |= out_mask;
	    in_mask >>= depth;
	    if (!in_mask)
		in_mask = 0x100 >> depth,
		    src++;
	    out_mask >>= 1;
	    if (!out_mask)
		out_mask = 0x80,
		    *dest++ = out,
		    out = 0;
	}
    }
    if (out_mask != 0x80)
	*dest = out;
    return pbm_print_row(pdev, data, 1, pstream);
}
private int
pgm_print_page(gx_device_printer * pdev, FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    return (bdev->uses_color == 0 && bdev->optimize ?
	    pbm_print_page_loop(pdev, (char)((int)bdev->magic - 1), pstream,
				pxm_pbm_print_row) :
	    pbm_print_page_loop(pdev, bdev->magic, pstream,
				pgm_print_row));
}

/* Print a color-mapped page. */
private int
ppgm_print_row(gx_device_printer * pdev, byte * data, int depth,
	       FILE * pstream, bool color)
{				/* If color=false, write only one value per pixel; */
    /* if color=true, write 3 values per pixel. */
    /* Note that depth <= 24 for raw format, depth <= 32 for plain. */
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    uint bpe = depth / 3;	/* bits per r/g/b element */
    uint mask = (1 << bpe) - 1;
    byte *bp;
    uint x;
    uint eol_mask = (color ? 7 : 15);
    int shift;

    if (bdev->is_raw && depth == 24 && color)
	fwrite(data, 1, pdev->width * (depth / 8), pstream);
    else
	for (bp = data, x = 0, shift = 8 - depth; x < pdev->width;) {
	    bits32 pixel = 0;
	    uint r, g, b;

	    switch (depth >> 3) {
		case 4:
		    pixel = (bits32) * bp << 24;
		    bp++;
		    /* falls through */
		case 3:
		    pixel += (bits32) * bp << 16;
		    bp++;
		    /* falls through */
		case 2:
		    pixel += (uint) * bp << 8;
		    bp++;
		    /* falls through */
		case 1:
		    pixel += *bp;
		    bp++;
		    break;
		case 0:	/* bpp == 4, bpe == 1 */
		    pixel = *bp >> shift;
		    if ((shift -= depth) < 0)
			bp++, shift += 8;
		    break;
	    }
	    ++x;
	    b = pixel & mask;
	    pixel >>= bpe;
	    g = pixel & mask;
	    pixel >>= bpe;
	    r = pixel & mask;
	    if (bdev->is_raw) {
		if (color) {
		    putc(r, pstream);
		    putc(g, pstream);
		}
		putc(b, pstream);
	    } else {
		if (color)
		    fprintf(pstream, "%d %d ", r, g);
		fprintf(pstream, "%d%c", b,
			(x == pdev->width || !(x & eol_mask) ?
			 '\n' : ' '));
	    }
	}
    return 0;
}
private int
ppm_print_row(gx_device_printer * pdev, byte * data, int depth,
	      FILE * pstream)
{
    return ppgm_print_row(pdev, data, depth, pstream, true);
}
private int
ppm_pgm_print_row(gx_device_printer * pdev, byte * data, int depth,
		  FILE * pstream)
{
    return ppgm_print_row(pdev, data, depth, pstream, false);
}
private int
ppm_print_page(gx_device_printer * pdev, FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    return (bdev->uses_color >= 2 || !bdev->optimize ?
	    pbm_print_page_loop(pdev, bdev->magic, pstream,
				ppm_print_row) :
	    bdev->uses_color == 1 ?
	    pbm_print_page_loop(pdev, (char)((int)bdev->magic - 1), pstream,
				ppm_pgm_print_row) :
	    pbm_print_page_loop(pdev, (char)((int)bdev->magic - 2), pstream,
				pxm_pbm_print_row));
}

private int
pam_print_row(gx_device_printer * pdev, byte * data, int depth,
	       FILE * pstream)
{
    if (depth == 32)
	fwrite(data, 1, pdev->width * (depth / 8), pstream);
    return 0;
}

private int
pam_print_page(gx_device_printer * pdev, FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    return pbm_print_page_loop(pdev, bdev->magic, pstream,
				pam_print_row);
}

/* Print a faux CMYK page. */
/* Print a row where each pixel occupies 4 bits (depth == 4). */
/* In this case, we also know pdev->color_info.max_color == 1. */
private int
pkm_print_row_4(gx_device_printer * pdev, byte * data, int depth,
		FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    byte *bp;
    uint x;
    byte rv[16], gv[16], bv[16], i;

    /* Precompute all the possible pixel values. */
    for (i = 0; i < 16; ++i) {
	gx_color_value rgb[3];

	cmyk_1bit_map_color_rgb((gx_device *)pdev, (gx_color_index)i, rgb);
	rv[i] = rgb[0] / gx_max_color_value;
	gv[i] = rgb[1] / gx_max_color_value;
	bv[i] = rgb[2] / gx_max_color_value;
    }
    /*
     * Contrary to what the documentation implies, gcc compiles putc
     * as a procedure call.  This is ridiculous, but since we can't
     * change it, we buffer groups of pixels ourselves and use fwrite.
     */
    if (bdev->is_raw) {
	for (bp = data, x = 0; x < pdev->width;) {
	    byte raw[50 * 3];	/* 50 is arbitrary, but must be even */
	    int end = min(x + sizeof(raw) / 3, pdev->width);
	    byte *outp = raw;

	    for (; x < end; bp++, outp += 6, x += 2) {
		uint b = *bp;
		uint pixel = b >> 4;

		outp[0] = rv[pixel], outp[1] = gv[pixel], outp[2] = bv[pixel];
		pixel = b & 0xf;
		outp[3] = rv[pixel], outp[4] = gv[pixel], outp[5] = bv[pixel];
	    }
	    /* x might overshoot the width by 1 pixel. */
	    if (x > end)
		outp -= 3;
	    fwrite(raw, 1, outp - raw, pstream);
	}
    } else {
	int shift;

	for (bp = data, x = 0, shift = 4; x < pdev->width;) {
	    int pixel = (*bp >> shift) & 0xf;

	    shift ^= 4;
	    bp += shift >> 2;
	    ++x;
	    fprintf(pstream, "%d %d %d%c", rv[pixel], gv[pixel], bv[pixel],
		    (x == pdev->width || !(x & 7) ?
		     '\n' : ' '));
	}
    }
    return 0;
}
/* Print a row where each pixel occupies 1 or more bytes (depth >= 8). */
private int
pkm_print_row(gx_device_printer * pdev, byte * data, int depth,
	      FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    byte *bp;
    uint x;
    ulong max_value = pdev->color_info.max_color;

    for (bp = data, x = 0; x < pdev->width;) {
	bits32 pixel = 0;
	gx_color_value rgb[3];
	uint r, g, b;

	switch (depth >> 3) {
	    case 4:
		pixel = (bits32) * bp << 24;
		bp++;
		/* falls through */
	    case 3:
		pixel += (bits32) * bp << 16;
		bp++;
		/* falls through */
	    case 2:
		pixel += (uint) * bp << 8;
		bp++;
		/* falls through */
	    case 1:
		pixel += *bp;
		bp++;
	}
	++x;
	pkm_map_color_rgb((gx_device *) pdev, pixel, rgb);
	r = rgb[0] * max_value / gx_max_color_value;
	g = rgb[1] * max_value / gx_max_color_value;
	b = rgb[2] * max_value / gx_max_color_value;
	if (bdev->is_raw) {
	    putc(r, pstream);
	    putc(g, pstream);
	    putc(b, pstream);
	} else {
	    fprintf(pstream, "%d %d %d%c", r, g, b,
		    (x == pdev->width || !(x & 7) ?
		     '\n' : ' '));
	}
    }
    return 0;
}
private int
pkm_print_page(gx_device_printer * pdev, FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;

    return pbm_print_page_loop(pdev, bdev->magic, pstream,
			       (pdev->color_info.depth < 8 ?
				pkm_print_row_4 :
				pkm_print_row));
}

/* Print individual separations on a single file. */
private int
psm_print_page(gx_device_printer * pdev, FILE * pstream)
{
    gx_device_pbm * const bdev = (gx_device_pbm *)pdev;
    /*
     * Allocate a large enough buffer for full pixels, on the theory that we
     * don't know how many bits will be allocated to each component.  (This
     * is for didactic purposes only: we know perfectly well that each
     * component will have 1/N of the bits.)
     */
    uint max_raster = bitmap_raster(pdev->width * pdev->color_info.depth);
    byte *data = gs_alloc_bytes(pdev->memory, max_raster, "pksm_print_page");
    int code = 0;
    int plane;

    if (data == 0)
	return_error(gs_error_VMerror);
    for (plane = 0; plane < pdev->color_info.num_components; ++plane) {
	int lnum, band_end;
	/*
	 * The following initialization is unnecessary: lnum == band_end on
	 * the first pass through the loop below, so marked will always be
	 * set before it is used.  We initialize marked solely to suppress
	 * bogus warning messages from certain compilers.
	 */
	gx_color_index marked = 0;
	gx_render_plane_t render_plane;
	int plane_depth;
	int plane_shift;
	gx_color_index plane_mask;
	int raster;

	gx_render_plane_init(&render_plane, (gx_device *)pdev, plane);
	plane_depth = render_plane.depth;
	plane_shift = render_plane.shift;
	plane_mask = (1 << plane_depth) - 1;
	raster = bitmap_raster(pdev->width * plane_depth);
	fprintf(pstream, "P%c\n", bdev->magic + (plane_depth > 1));
	if (bdev->comment[0])
	    fprintf(pstream, "# %s\n", bdev->comment);
	else
	    fprintf(pstream, "# Image generated by %s (device=%s)\n",
		    gs_product, pdev->dname);
	fprintf(pstream, "%d %d\n", pdev->width, pdev->height);
	if (plane_depth > 1)
	    fprintf(pstream, "%d\n", pdev->color_info.max_gray);
	for (lnum = band_end = 0; lnum < pdev->height; lnum++) {
	    byte *row;

	    if (lnum == band_end) {
		gx_colors_used_t colors_used;
		int band_start;
		int band_height =
		    gdev_prn_colors_used((gx_device *)pdev, lnum, 1,
					 &colors_used, &band_start);

		band_end = band_start + band_height;
		marked = colors_used.or & (plane_mask << plane_shift);
		if (!marked)
		    memset(data, 0, raster);
#ifdef DEBUG
		if (plane == 0)
		    if_debug4(':',
			      "[:]%4d - %4d mask = 0x%lx, slow_rop = %d\n",
			      lnum, band_end - 1, (ulong)colors_used.or,
			      colors_used.slow_rop);
#endif
	    }
	    if (marked) {
		gx_render_plane_t render_plane;
		uint actual_raster;

		render_plane.index = plane;
		code = gdev_prn_get_lines(pdev, lnum, 1, data, raster,
					  &row, &actual_raster,
					  &render_plane);
		if (code < 0)
		    break;
	    } else
		row = data;
	    code =
		(plane_depth == 1 ?
		 pbm_print_row(pdev, row, plane_depth, pstream) :
		 pgm_print_row(pdev, row, plane_depth, pstream));
	    if (code < 0)
		break;
	}
    }
    gs_free_object(pdev->memory, data, "pksm_print_page");
    return (code < 0 ? code : 0);
}
