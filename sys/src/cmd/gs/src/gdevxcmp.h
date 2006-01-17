/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevxcmp.h,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* X driver color mapping structure */

#ifndef gdevxcmp_INCLUDED
#  define gdevxcmp_INCLUDED

/*
 * The structure defined in this file is used in only one place, in the
 * gx_device_X structure defined in gdevx.h.  We define it as a separate
 * structure because its function is logically separate from the rest of the
 * X driver, and because this function (color mapping / management) accounts
 * for the single largest piece of the driver.
 */

/* Define pixel value to RGB mapping */
typedef struct x11_rgb_s {
    gx_color_value rgb[3];
    bool defined;
} x11_rgb_t;

/* Define dynamic color hash table structure */
typedef struct x11_color_s x11_color_t;
struct x11_color_s {
    XColor color;
    x11_color_t *next;
};

/*
 * Define X color values.  Fortuitously, these are the same as Ghostscript
 * color values; in gdevxcmp.c, we are pretty sloppy about aliasing the
 * two.
 */
typedef ushort X_color_value;
#define X_max_color_value 0xffff

#if HaveStdCMap  /* Standard colormap stuff is only in X11R4 and later. */

/* Define the structure for values computed from a standard cmap component. */
typedef struct x11_cmap_values_s {
    int cv_shift;	/* 16 - log2(max_value + 1) */
    X_color_value nearest[64]; /* [i] = i * 0xffff / max_value */
    int pixel_shift;	/* log2(mult) */
} x11_cmap_values_t;

#endif

typedef struct x11_cman_s {

    /*
     * num_rgb is the number of possible R/G/B values, i.e.,
     * 1 << the bits_per_rgb of the X visual.
     */
    int num_rgb;

    /*
     * color_mask is a mask that selects the high-order N bits of an
     * X color value, where N may be the mask width for TrueColor or
     * StaticGray and is bits_per_rgb for the other visual classes.
     *
     * match_mask is the mask used for comparing colors.  It may have
     * fewer bits than color_mask if the device is not using halftones.
     */
    struct cmm_ {
	X_color_value red, green, blue;
    } color_mask, match_mask;

#if HaveStdCMap  /* Standard colormap stuff is only in X11R4 and later. */

    struct {

	/*
	 * map is the X standard colormap for the display and screen,
	 * if one is available.
	 */
	XStandardColormap *map;

	/*
	 * When possible, we precompute shift values and tables that replace
	 * some multiplies and divides.
	 */
	bool fast;
	x11_cmap_values_t red, green, blue;

	/*
	 * If free_map is true, we allocated the map ourselves (to
	 * represent a TrueColor or Static Gray visual), and must free it
	 * when closing the device.
	 */
	bool free_map;

    } std_cmap;

#endif /* HaveStdCmap */

    /*
     * color_to_rgb is a reverse map from pixel values to RGB values.  It
     * only maps pixels values up to 255: pixel values above this must go
     * through the standard colormap or query the server.
     */
    struct cmc_ {
	int size;		/* min(1 << depth, 256) */
	x11_rgb_t *values;	/* [color_to_rgb.size] */
    } color_to_rgb;

    /*
     * For systems with writable colormaps and no suitable standard
     * colormap, dither_ramp is a preallocated ramp or cube used for
     * dithering.
     */
#define CUBE_INDEX(r,g,b) (((r) * xdev->color_info.dither_colors + (g)) * \
				  xdev->color_info.dither_colors + (b))
    x_pixel *dither_ramp;	/* [color_info.dither_colors^3] if color,
				   [color_info.dither_grays] if gray */

    /*
     * For systems with writable colormaps, dynamic.colors is a chained
     * hash table that maps RGB values (masked with color_mask) to
     * pixel values.  Entries are added dynamically.
     */
    struct cmd_ {
	int size;
	x11_color_t **colors;	/* [size] */
	int shift;		/* 16 - log2(size) */
	int used;
	int max_used;
    } dynamic;

} x11_cman_t;

#endif /* gdevxcmp_INCLUDED */
