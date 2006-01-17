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

/* $Id: gdevxcmp.c,v 1.9 2004/08/04 19:36:12 stefan Exp $ */
/* X Windows color mapping */
#include "math_.h"
#include "x_.h"
#include "gx.h"			/* for gx_bitmap; includes std.h */
#include "gserrors.h"
#include "gxdevice.h"
#include "gdevx.h"

/* ---------------- Utilities ---------------- */

private void
gs_x_free(gs_memory_t *mem, void *obj, client_name_t cname)
{
    gs_free(mem, obj, 0 /*ignored*/, 0 /*ignored*/, cname);
}

/* ---------------- Color mapping setup / cleanup ---------------- */

#if HaveStdCMap

/* Install a standard color map in the device. */
/* Sets std_cmap.* except for free_map. */
private bool
set_cmap_values(x11_cmap_values_t *values, int maxv, int mult)
{
    int i;

    if (maxv < 1 || maxv > 63 || (maxv & (maxv + 1)) ||
	(mult & (mult - 1))
	)
	return false;
    values->cv_shift = 16 - small_exact_log2(maxv + 1);
    for (i = 0; i <= maxv; ++i)
	values->nearest[i] = X_max_color_value * i / maxv;
    for (i = 0; mult != (1 << i); ++i)
	DO_NOTHING;
    values->pixel_shift = i;
    return true;
}
private void
set_std_cmap(gx_device_X *xdev, XStandardColormap *map)
{
    xdev->cman.std_cmap.map = map;
    xdev->cman.std_cmap.fast =
	set_cmap_values(&xdev->cman.std_cmap.red, map->red_max, map->red_mult) &&
	set_cmap_values(&xdev->cman.std_cmap.green, map->green_max, map->green_mult) &&
	set_cmap_values(&xdev->cman.std_cmap.blue, map->blue_max, map->blue_mult);
}

/* Get the Standard colormap if available. */
/* Uses: dpy, scr, cmap. */
private XStandardColormap *
x_get_std_cmap(gx_device_X * xdev, Atom prop)
{
    int i;
    XStandardColormap *scmap, *sp;
    int nitems;

    if (XGetRGBColormaps(xdev->dpy, RootWindowOfScreen(xdev->scr),
			 &scmap, &nitems, prop))
	for (i = 0, sp = scmap; i < nitems; i++, sp++)
	    if (xdev->cmap == sp->colormap)
		return sp;

    return NULL;
}

/* Create a Standard colormap for a TrueColor or StaticGray display. */
/* Return true if the allocation was successful. */
/* Uses: vinfo.  Sets: std_cmap.*. */
private bool
alloc_std_cmap(gx_device_X *xdev, bool colored)
{
    XStandardColormap *cmap = XAllocStandardColormap();

    if (cmap == 0)
	return false;		/* can't allocate */
    /*
     * Some buggy X servers (including XFree86) don't set any of the
     * _mask values for StaticGray visuals.  Compensate for that here.
     */
    if ((cmap->red_max = xdev->vinfo->red_mask) == 0) {
	cmap->red_max = (1 << xdev->vinfo->depth) - 1;
	cmap->red_mult = 1;
    } else {
	for (cmap->red_mult = 1; (cmap->red_max & 1) == 0;) {
	    cmap->red_max >>= 1;
	    cmap->red_mult <<= 1;
	}
    }
    if (colored) {
	for (cmap->green_max = xdev->vinfo->green_mask, cmap->green_mult = 1;
	     (cmap->green_max & 1) == 0;
	     ) {
	    cmap->green_max >>= 1;
	    cmap->green_mult <<= 1;
	}
	for (cmap->blue_max = xdev->vinfo->blue_mask, cmap->blue_mult = 1;
	     (cmap->blue_max & 1) == 0;
	     ) {
	    cmap->blue_max >>= 1;
	    cmap->blue_mult <<= 1;
	}
    } else {
        cmap->green_max = cmap->blue_max = cmap->red_max;
        cmap->green_mult = cmap->blue_mult = cmap->red_mult;
    }
    set_std_cmap(xdev, cmap);
    xdev->cman.std_cmap.free_map = true;
    return true;
}

#endif

/* Allocate the dynamic color table, if needed and possible. */
/* Uses: vinfo, cman.num_rgb.  Sets: cman.dynamic.*. */
private void
alloc_dynamic_colors(gx_device_X * xdev, int num_colors)
{
    if (num_colors > 0) {
	xdev->cman.dynamic.colors = (x11_color_t **)
	    gs_malloc(xdev->memory, sizeof(x11_color_t *), xdev->cman.num_rgb,
		      "x11 cman.dynamic.colors");
	if (xdev->cman.dynamic.colors) {
	    int i;

	    xdev->cman.dynamic.size = xdev->cman.num_rgb;
	    xdev->cman.dynamic.shift = 16 - xdev->vinfo->bits_per_rgb;
	    for (i = 0; i < xdev->cman.num_rgb; i++)
		xdev->cman.dynamic.colors[i] = NULL;
	    xdev->cman.dynamic.max_used = min(256, num_colors);
	    xdev->cman.dynamic.used = 0;
	}
    }
}

/* Allocate an X color, updating the reverse map. */
/* Return true if the allocation was successful. */
private bool
x_alloc_color(gx_device_X *xdev, XColor *xcolor)
{
    x11_rgb_t rgb;

    rgb.rgb[0] = xcolor->red;
    rgb.rgb[1] = xcolor->green;
    rgb.rgb[2] = xcolor->blue;
    if (!XAllocColor(xdev->dpy, xdev->cmap, xcolor))
	return false;
    if (xcolor->pixel < xdev->cman.color_to_rgb.size) {
	x11_rgb_t *pxrgb = &xdev->cman.color_to_rgb.values[xcolor->pixel];

	memcpy(pxrgb->rgb, rgb.rgb, sizeof(rgb.rgb));
	pxrgb->defined = true;
    }
    return true;
}

/* Free X colors, updating the reverse map. */
private void
x_free_colors(gx_device_X *xdev, x_pixel *pixels /*[count]*/, int count)
{
    int i;
    x_pixel pixel;

    XFreeColors(xdev->dpy, xdev->cmap, pixels, count, 0);
    for (i = 0; i < count; ++i)
	if ((pixel = pixels[i]) < xdev->cman.color_to_rgb.size)
	    xdev->cman.color_to_rgb.values[pixel].defined = false;
}

/* Free a partially filled color cube or ramp. */
/* Uses: dpy, cmap.  Uses and sets: cman.dither_ramp. */
private void
free_ramp(gx_device_X * xdev, int num_used, int size)
{
    if (num_used - 1 > 0)
	x_free_colors(xdev, xdev->cman.dither_ramp + 1, num_used - 1);
    gs_x_free(xdev->memory, xdev->cman.dither_ramp, "x11_setup_colors");
    xdev->cman.dither_ramp = NULL;
}

/* Allocate and fill in a color cube or ramp. */
/* Return true if the operation succeeded. */
/* Uses: dpy, cmap, foreground, background, cman.color_mask. */
/* Sets: cman.dither_ramp. */
private bool
setup_cube(gx_device_X * xdev, int ramp_size, bool colors)
{
    int step, num_entries;
    int max_rgb = ramp_size - 1;
    int index;

    if (colors) {
	num_entries = ramp_size * ramp_size * ramp_size;
	step = 1;		/* all colors */
    } else {
	num_entries = ramp_size;
	step = (ramp_size + 1) * ramp_size + 1;		/* gray only */
    }

    xdev->cman.dither_ramp =
	(x_pixel *) gs_malloc(xdev->memory, sizeof(x_pixel), num_entries,
			      "gdevx setup_cube");
    if (xdev->cman.dither_ramp == NULL)
	return false;

    xdev->cman.dither_ramp[0] = xdev->foreground;
    xdev->cman.dither_ramp[num_entries - 1] = xdev->background;
    for (index = 1; index < num_entries - 1; index++) {
	int rgb_index = index * step;
	int q = rgb_index / ramp_size,
	    r = q / ramp_size,
	    g = q % ramp_size,
	    b = rgb_index % ramp_size;
	XColor xc;

	xc.red = (X_max_color_value * r / max_rgb) & xdev->cman.color_mask.red;
	xc.green = (X_max_color_value * g / max_rgb) & xdev->cman.color_mask.green;
	xc.blue = (X_max_color_value * b / max_rgb) & xdev->cman.color_mask.blue;
	if (!x_alloc_color(xdev, &xc)) {
	    free_ramp(xdev, index, num_entries);
	    return false;
	}
	xdev->cman.dither_ramp[index] = xc.pixel;
    }

    return true;
}

/* Setup color mapping. */
int
gdev_x_setup_colors(gx_device_X * xdev)
{
    char palette =
	((xdev->vinfo->class != StaticGray) &&
	 (xdev->vinfo->class != GrayScale) ? 'C' :	/* Color */
	 (xdev->vinfo->colormap_size > 2) ? 'G' :		/* GrayScale */
	 'M');		/* MonoChrome */

    if (xdev->ghostview) {
	Atom gv_colors = XInternAtom(xdev->dpy, "GHOSTVIEW_COLORS", False);
	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	char *buf;

	/* Delete property if explicit dest is given */
	if (XGetWindowProperty(xdev->dpy, xdev->win, gv_colors, 0,
			       256, (xdev->dest != 0), XA_STRING,
			       &type, &format, &nitems, &bytes_after,
			       (unsigned char **)&buf) == 0 &&
	    type == XA_STRING) {
	    nitems = sscanf(buf, "%*s %ld %ld", &(xdev->foreground),
			    &(xdev->background));
	    if (nitems != 2 || (*buf != 'M' && *buf != 'G' && *buf != 'C')) {
		eprintf("Malformed GHOSTVIEW_COLOR property.\n");
		return_error(gs_error_rangecheck);
	    }
	    palette = max(palette, *buf);
	}
    } else {
	if (xdev->palette[0] == 'c')
	    xdev->palette[0] = 'C';
	else if (xdev->palette[0] == 'g')
	    xdev->palette[0] = 'G';
	else if (xdev->palette[0] == 'm')
	    xdev->palette[0] = 'M';
	palette = max(palette, xdev->palette[0]);
    }

    /* set up color mappings here */
    xdev->cman.color_mask.red = xdev->cman.color_mask.green =
	xdev->cman.color_mask.blue = X_max_color_value -
	  (X_max_color_value >> xdev->vinfo->bits_per_rgb);
    xdev->cman.match_mask = xdev->cman.color_mask; /* default */
    xdev->cman.num_rgb = 1 << xdev->vinfo->bits_per_rgb;

#if HaveStdCMap
    xdev->cman.std_cmap.map = NULL;
    xdev->cman.std_cmap.free_map = false;
#endif
    xdev->cman.dither_ramp = NULL;
    xdev->cman.dynamic.colors = NULL;
    xdev->cman.dynamic.size = 0;
    xdev->cman.dynamic.used = 0;
    switch (xdev->vinfo->depth) {
    case 1: case 2: case 4: case 8: case 16: case 24: case 32:
	xdev->color_info.depth = xdev->vinfo->depth;
	break;
    case 15:
	xdev->color_info.depth = 16;
	break;
    default:
	eprintf1("Unsupported X visual depth: %d\n", xdev->vinfo->depth);
	return_error(gs_error_rangecheck);
    }
    {	/* Set up the reverse map from pixel values to RGB. */
	int count = 1 << min(xdev->color_info.depth, 8);

	xdev->cman.color_to_rgb.values =
	    (x11_rgb_t *)gs_malloc(xdev->memory, sizeof(x11_rgb_t), count,
				   "gdevx color_to_rgb");
	if (xdev->cman.color_to_rgb.values) {
	    int i;

	    for (i = 0; i < count; ++i)
		xdev->cman.color_to_rgb.values[i].defined = false;
	    xdev->cman.color_to_rgb.size = count;
	} else
	    xdev->cman.color_to_rgb.size = 0;
    }
    switch ((int)palette) {
    case 'C':
	xdev->color_info.num_components = 3;
	xdev->color_info.max_gray =
	    xdev->color_info.max_color = xdev->cman.num_rgb - 1;
#if HaveStdCMap
	/* Get a standard color map if available */
	if (xdev->vinfo->visual == DefaultVisualOfScreen(xdev->scr)) {
	    xdev->cman.std_cmap.map = x_get_std_cmap(xdev, XA_RGB_DEFAULT_MAP);
	} else {
	    xdev->cman.std_cmap.map = x_get_std_cmap(xdev, XA_RGB_BEST_MAP);
	}
	if (xdev->cman.std_cmap.map ||
	    (xdev->vinfo->class == TrueColor && alloc_std_cmap(xdev, true))
	    ) {
	    xdev->color_info.dither_grays = xdev->color_info.dither_colors =
		min(xdev->cman.std_cmap.map->red_max,
		    min(xdev->cman.std_cmap.map->green_max,
			xdev->cman.std_cmap.map->blue_max)) + 1;
	    if (xdev->cman.std_cmap.map)
		set_std_cmap(xdev, xdev->cman.std_cmap.map);
	} else
#endif
	    /* Otherwise set up a rgb cube of our own */
	    /* The color cube is limited to about 1/2 of the available */
	    /* colormap, the user specified maxRGBRamp (usually 5), */
	    /* or the number of representable colors */
#define CUBE(r) (r*r*r)
#define CBRT(r) pow(r, 1.0/3.0)
	{
	    int ramp_size =
		min((int)CBRT(xdev->vinfo->colormap_size / 2.0),
		    min(xdev->maxRGBRamp, xdev->cman.num_rgb));

	    while (!xdev->cman.dither_ramp && ramp_size >= 2) {
		xdev->color_info.dither_grays =
		    xdev->color_info.dither_colors = ramp_size;
		if (!setup_cube(xdev, ramp_size, true)) {
#ifdef DEBUG
		    eprintf3("Warning: failed to allocate %dx%dx%d RGB cube.\n",
			     ramp_size, ramp_size, ramp_size);
#endif
		    ramp_size--;
		    continue;
		}
	    }

	    if (!xdev->cman.dither_ramp) {
		goto grayscale;
	    }
	}

	/* Allocate the dynamic color table. */
	alloc_dynamic_colors(xdev, CUBE(xdev->cman.num_rgb) -
			     CUBE(xdev->color_info.dither_colors));
#undef CUBE
#undef CBRT
	break;
    case 'G':
grayscale:
	xdev->color_info.num_components = 1;
	xdev->color_info.max_gray = xdev->cman.num_rgb - 1;
#if HaveStdCMap
	/* Get a standard color map if available */
	xdev->cman.std_cmap.map = x_get_std_cmap(xdev, XA_RGB_GRAY_MAP);
	if (xdev->cman.std_cmap.map ||
	    (xdev->vinfo->class == StaticGray && alloc_std_cmap(xdev, false))
	    ) {
	    xdev->color_info.dither_grays =
		xdev->cman.std_cmap.map->red_max + 1;
	    if (xdev->cman.std_cmap.map)
		set_std_cmap(xdev, xdev->cman.std_cmap.map);
	} else
#endif
	    /* Otherwise set up a gray ramp of our own */
	    /* The gray ramp is limited to about 1/2 of the available */
	    /* colormap, the user specified maxGrayRamp (usually 128), */
	    /* or the number of representable grays */
	{
	    int ramp_size = min(xdev->vinfo->colormap_size / 2,
				min(xdev->maxGrayRamp, xdev->cman.num_rgb));

	    while (!xdev->cman.dither_ramp && ramp_size >= 3) {
		xdev->color_info.dither_grays = ramp_size;
		if (!setup_cube(xdev, ramp_size, false)) {
#ifdef DEBUG
		    eprintf1("Warning: failed to allocate %d level gray ramp.\n",
			     ramp_size);
#endif
		    ramp_size /= 2;
		    continue;
		}
	    }
	    if (!xdev->cman.dither_ramp) {
		goto monochrome;
	    }
	}

	/* Allocate the dynamic color table. */
	alloc_dynamic_colors(xdev, xdev->cman.num_rgb -
			     xdev->color_info.dither_grays);
	break;
    case 'M':
monochrome:
	xdev->color_info.num_components = 1;
	xdev->color_info.max_gray = 1;
	xdev->color_info.dither_grays = 2;
	break;
    default:
	eprintf1("Unknown palette: %s\n", xdev->palette);
	if (xdev->cman.color_to_rgb.values) {
	    gs_x_free(xdev->memory, xdev->cman.color_to_rgb.values, "gdevx color_to_rgb");
	    xdev->cman.color_to_rgb.values = 0;
	}
	return_error(gs_error_rangecheck);
    }

#if HaveStdCMap
    /*
     * When comparing colors, if not halftoning, we must only compare as
     * many bits as actually fit in a pixel, even if the hardware has more.
     */
    if (!gx_device_must_halftone(xdev)) {
	if (xdev->cman.std_cmap.map) {
	    xdev->cman.match_mask.red &=
		X_max_color_value << xdev->cman.std_cmap.red.cv_shift;
	    xdev->cman.match_mask.green &=
		X_max_color_value << xdev->cman.std_cmap.green.cv_shift;
	    xdev->cman.match_mask.blue &=
		X_max_color_value << xdev->cman.std_cmap.blue.cv_shift;
	}
    }
#endif

    return 0;
}

/* Free the dynamic colors when doing an erasepage. */
/* Uses: cman.dynamic.*.  Sets: cman.dynamic.used. */
void
gdev_x_free_dynamic_colors(gx_device_X *xdev)
{
    if (xdev->cman.dynamic.colors) {
	int i;
	x11_color_t *xcp;
	x11_color_t *next;

	for (i = 0; i < xdev->cman.dynamic.size; i++) {
	    for (xcp = xdev->cman.dynamic.colors[i]; xcp; xcp = next) {
		next = xcp->next;
		if (xcp->color.pad)
		    x_free_colors(xdev, &xcp->color.pixel, 1);
		gs_x_free(xdev->memory, xcp, "x11_dynamic_color");
	    }
	    xdev->cman.dynamic.colors[i] = NULL;
	}
	xdev->cman.dynamic.used = 0;
    }
}

/*
 * Free storage and color map entries when closing the device.
 * Uses and sets: cman.{std_cmap.map, dither_ramp, dynamic.colors,
 * color_to_rgb}.  Uses: cman.std_cmap.free_map.
 */
void
gdev_x_free_colors(gx_device_X *xdev)
{
    if (xdev->cman.std_cmap.free_map) {
	/* XFree is declared as taking a char *, not a void *! */
	XFree((void *)xdev->cman.std_cmap.map);
	xdev->cman.std_cmap.free_map = false;
    }
    xdev->cman.std_cmap.map = 0;
    if (xdev->cman.dither_ramp)
	gs_x_free(xdev->memory, xdev->cman.dither_ramp, "x11 dither_colors");
    if (xdev->cman.dynamic.colors) {
	gdev_x_free_dynamic_colors(xdev);
	gs_x_free(xdev->memory, xdev->cman.dynamic.colors, "x11 cman.dynamic.colors");
	xdev->cman.dynamic.colors = NULL;
    }
    if (xdev->cman.color_to_rgb.values) {
	gs_x_free(xdev->memory, xdev->cman.color_to_rgb.values, "x11 color_to_rgb");
	xdev->cman.color_to_rgb.values = NULL;
	xdev->cman.color_to_rgb.size = 0;
    }
}

/* ---------------- Driver color mapping calls ---------------- */

/* Define a table for computing N * X_max_color_value / D for 0 <= N <= D, */
/* 1 <= D <= 7. */
/* This requires a multiply and a divide otherwise; */
/* integer multiply and divide are slow on all platforms. */
#define CV_FRACTION(n, d) ((X_color_value)(X_max_color_value * (n) / (d)))
#define ND(n, d) CV_FRACTION(n, d)
private const X_color_value cv_tab1[] = {
    ND(0,1), ND(1,1)
};
private const X_color_value cv_tab2[] = {
    ND(0,2), ND(1,2), ND(2,2)
};
private const X_color_value cv_tab3[] = {
    ND(0,3), ND(1,3), ND(2,3), ND(3,3)
};
private const X_color_value cv_tab4[] = {
    ND(0,4), ND(1,4), ND(2,4), ND(3,4), ND(4,4)
};
private const X_color_value cv_tab5[] = {
    ND(0,5), ND(1,5), ND(2,5), ND(3,5), ND(4,5), ND(5,5)
};
private const X_color_value cv_tab6[] = {
    ND(0,6), ND(1,6), ND(2,6), ND(3,6), ND(4,6), ND(5,6), ND(6,6)
};
private const X_color_value cv_tab7[] = {
    ND(0,7), ND(1,7), ND(2,7), ND(3,7), ND(4,7), ND(5,7), ND(6,7), ND(7,7)
};
#undef ND
private const X_color_value *const cv_tables[] =
{
    0, cv_tab1, cv_tab2, cv_tab3, cv_tab4, cv_tab5, cv_tab6, cv_tab7
};

/* Some C compilers don't declare the abs function in math.h. */
/* Provide one of our own. */
private inline int
iabs(int x)
{
    return (x < 0 ? -x : x);
}

/* Map RGB values to a pixel value. */
gx_color_index
gdev_x_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    gx_device_X *const xdev = (gx_device_X *) dev;
    gx_color_value r = cv[0];
    gx_color_value g = cv[1];
    gx_color_value b = cv[2];

    /* X and ghostscript both use shorts for color values. */
    /* Set drgb to the nearest color that the device can represent. */
    X_color_value dr = r & xdev->cman.color_mask.red;
    X_color_value dg = g & xdev->cman.color_mask.green;
    X_color_value db = b & xdev->cman.color_mask.blue;

    {
	/* Foreground and background get special treatment: */
	/* They may be mapped to other colors. */
	/* Set mrgb to the color to be used for match testing. */
	X_color_value mr = r & xdev->cman.match_mask.red;
	X_color_value mg = g & xdev->cman.match_mask.green;
	X_color_value mb = b & xdev->cman.match_mask.blue;

	if ((mr | mg | mb) == 0) {	/* i.e., all 0 */
	    if_debug4('C', "[cX]%u,%u,%u => foreground = %lu\n",
		      r, g, b, (ulong) xdev->foreground);
	    return xdev->foreground;
	}
	if (mr == xdev->cman.match_mask.red &&
	    mg == xdev->cman.match_mask.green &&
	    mb == xdev->cman.match_mask.blue
	    ) {
	    if_debug4('C', "[cX]%u,%u,%u => background = %lu\n",
		      r, g, b, (ulong) xdev->background);
	    return xdev->background;
	}
    }

#define CV_DENOM (gx_max_color_value + 1)

#if HaveStdCMap
    /* check the standard colormap first */
    if (xdev->cman.std_cmap.map) {
	const XStandardColormap *cmap = xdev->cman.std_cmap.map;

	if (gx_device_has_color(xdev)) {
	    uint cr, cg, cb;	/* rgb cube indices */
	    X_color_value cvr, cvg, cvb;	/* color value on cube */

	    if (xdev->cman.std_cmap.fast) {
		cr = r >> xdev->cman.std_cmap.red.cv_shift;
		cvr = xdev->cman.std_cmap.red.nearest[cr];
		cg = g >> xdev->cman.std_cmap.green.cv_shift;
		cvg = xdev->cman.std_cmap.green.nearest[cg];
		cb = b >> xdev->cman.std_cmap.blue.cv_shift;
		cvb = xdev->cman.std_cmap.blue.nearest[cb];
	    } else {
		cr = r * (cmap->red_max + 1) / CV_DENOM;
		cg = g * (cmap->green_max + 1) / CV_DENOM;
		cb = b * (cmap->blue_max + 1) / CV_DENOM;
		cvr = X_max_color_value * cr / cmap->red_max;
		cvg = X_max_color_value * cg / cmap->green_max;
		cvb = X_max_color_value * cb / cmap->blue_max;
	    }
	    if ((iabs((int)r - (int)cvr) & xdev->cman.match_mask.red) == 0 &&
		(iabs((int)g - (int)cvg) & xdev->cman.match_mask.green) == 0 &&
		(iabs((int)b - (int)cvb) & xdev->cman.match_mask.blue) == 0) {
		gx_color_index pixel =
		    (xdev->cman.std_cmap.fast ?
		     (cr << xdev->cman.std_cmap.red.pixel_shift) +
		     (cg << xdev->cman.std_cmap.green.pixel_shift) +
		     (cb << xdev->cman.std_cmap.blue.pixel_shift) :
		     cr * cmap->red_mult + cg * cmap->green_mult +
		     cb * cmap->blue_mult) + cmap->base_pixel;

		if_debug4('C', "[cX]%u,%u,%u (std cmap) => %lu\n",
			  r, g, b, pixel);
		return pixel;
	    }
	    if_debug3('C', "[cX]%u,%u,%u (std cmap fails)\n", r, g, b);
	} else {
	    uint cr;
	    X_color_value cvr;

	    cr = r * (cmap->red_max + 1) / CV_DENOM;
	    cvr = X_max_color_value * cr / cmap->red_max;
	    if ((iabs((int)r - (int)cvr) & xdev->cman.match_mask.red) == 0) {
		gx_color_index pixel = cr * cmap->red_mult + cmap->base_pixel;

		if_debug2('C', "[cX]%u (std cmap) => %lu\n", r, pixel);
		return pixel;
	    }
	    if_debug1('C', "[cX]%u (std cmap fails)\n", r);
	}
    } else
#endif

	/* If there is no standard colormap, check the dither cube/ramp */
    if (xdev->cman.dither_ramp) {
	if (gx_device_has_color(xdev)) {
	    uint cr, cg, cb;	/* rgb cube indices */
	    X_color_value cvr, cvg, cvb;	/* color value on cube */
	    int dither_rgb = xdev->color_info.dither_colors;
	    uint max_rgb = dither_rgb - 1;

	    cr = r * dither_rgb / CV_DENOM;
	    cg = g * dither_rgb / CV_DENOM;
	    cb = b * dither_rgb / CV_DENOM;
	    if (max_rgb < countof(cv_tables)) {
		const ushort *cv_tab = cv_tables[max_rgb];

		cvr = cv_tab[cr];
		cvg = cv_tab[cg];
		cvb = cv_tab[cb];
	    } else {
		cvr = CV_FRACTION(cr, max_rgb);
		cvg = CV_FRACTION(cg, max_rgb);
		cvb = CV_FRACTION(cb, max_rgb);
	    }
	    if ((iabs((int)r - (int)cvr) & xdev->cman.match_mask.red) == 0 &&
		(iabs((int)g - (int)cvg) & xdev->cman.match_mask.green) == 0 &&
		(iabs((int)b - (int)cvb) & xdev->cman.match_mask.blue) == 0) {
		gx_color_index pixel =
		    xdev->cman.dither_ramp[CUBE_INDEX(cr, cg, cb)];

		if_debug4('C', "[cX]%u,%u,%u (dither cube) => %lu\n",
			  r, g, b, pixel);
		return pixel;
	    }
	    if_debug3('C', "[cX]%u,%u,%u (dither cube fails)\n", r, g, b);
	} else {
	    uint cr;
	    X_color_value cvr;
	    int dither_grays = xdev->color_info.dither_grays;
	    uint max_gray = dither_grays - 1;

	    cr = r * dither_grays / CV_DENOM;
	    cvr = (X_max_color_value * cr / max_gray);
	    if ((iabs((int)r - (int)cvr) & xdev->cman.match_mask.red) == 0) {
		gx_color_index pixel = xdev->cman.dither_ramp[cr];

		if_debug2('C', "[cX]%u (dither ramp) => %lu\n", r, pixel);
		return pixel;
	    }
	    if_debug1('C', "[cX]%u (dither ramp fails)\n", r);
	}
    }

    /* Finally look through the list of dynamic colors */
    if (xdev->cman.dynamic.colors) {
	int i = (dr ^ dg ^ db) >> xdev->cman.dynamic.shift;
	x11_color_t *xcp = xdev->cman.dynamic.colors[i];
	x11_color_t *prev = NULL;
	XColor xc;

	for (; xcp; prev = xcp, xcp = xcp->next)
	    if (xcp->color.red == dr && xcp->color.green == dg &&
		xcp->color.blue == db) {
		/* Promote the found entry to the front of the list. */
		if (prev) {
		    prev->next = xcp->next;
		    xcp->next = xdev->cman.dynamic.colors[i];
		    xdev->cman.dynamic.colors[i] = xcp;
		}
		if (xcp->color.pad) {
		    if_debug4('C', "[cX]%u,%u,%u (dynamic) => %lu\n",
			      r, g, b, (ulong) xcp->color.pixel);
		    return xcp->color.pixel;
		} else {
		    if_debug3('C', "[cX]%u,%u,%u (dynamic) => missing\n",
			      r, g, b);
		    return gx_no_color_index;
		}
	    }

	/* If not in our list of dynamic colors, */
	/* ask the X server and add an entry. */
	/* First check if dynamic table is exhausted */
	if (xdev->cman.dynamic.used > xdev->cman.dynamic.max_used) {
	    if_debug3('C', "[cX]%u,%u,%u (dynamic) => full\n", r, g, b);
	    return gx_no_color_index;
	}
	xcp = (x11_color_t *)
	    gs_malloc(xdev->memory, sizeof(x11_color_t), 1, "x11_dynamic_color");
	if (!xcp)
	    return gx_no_color_index;
	xc.red = xcp->color.red = dr;
	xc.green = xcp->color.green = dg;
	xc.blue = xcp->color.blue = db;
	xcp->next = xdev->cman.dynamic.colors[i];
	xdev->cman.dynamic.colors[i] = xcp;
	xdev->cman.dynamic.used++;
	if (x_alloc_color(xdev, &xc)) {
	    xcp->color.pixel = xc.pixel;
	    xcp->color.pad = true;
	    if_debug5('c', "[cX]0x%x,0x%x,0x%x (dynamic) => added [%d]%lu\n",
		      dr, dg, db, xdev->cman.dynamic.used - 1,
		      (ulong)xc.pixel);
	    return xc.pixel;
	} else {
	    xcp->color.pad = false;
	    if_debug3('c', "[cX]0x%x,0x%x,0x%x (dynamic) => can't alloc\n",
		      dr, dg, db);
	    return gx_no_color_index;
	}
    }
    if_debug3('C', "[cX]%u,%u,%u fails\n", r, g, b);
    return gx_no_color_index;
#undef CV_DENOM
}


/* Map a pixel value back to r-g-b. */
int
gdev_x_map_color_rgb(gx_device * dev, gx_color_index color,
		     gx_color_value prgb[3])
{
    const gx_device_X *const xdev = (const gx_device_X *) dev;
#if HaveStdCMap
    const XStandardColormap *cmap = xdev->cman.std_cmap.map;
#endif

    if (color == xdev->foreground) {
	prgb[0] = prgb[1] = prgb[2] = 0;
	return 0;
    }
    if (color == xdev->background) {
	prgb[0] = prgb[1] = prgb[2] = gx_max_color_value;
	return 0;
    }
    if (color < xdev->cman.color_to_rgb.size) {
	const x11_rgb_t *pxrgb = &xdev->cman.color_to_rgb.values[color];

	if (pxrgb->defined) {
	    prgb[0] = pxrgb->rgb[0];
	    prgb[1] = pxrgb->rgb[1];
	    prgb[2] = pxrgb->rgb[2];
	    return 0;
	}
#if HaveStdCMap
    }

    /* Check the standard colormap. */
    if (cmap) {
	if (color >= cmap->base_pixel) {
	    x_pixel value = color - cmap->base_pixel;
	    uint r = (value / cmap->red_mult) % (cmap->red_max + 1);
	    uint g = (value / cmap->green_mult) % (cmap->green_max + 1);
	    uint b = (value / cmap->blue_mult) % (cmap->blue_max + 1);

	    if (value == r * cmap->red_mult + g * cmap->green_mult +
		b * cmap->blue_mult) {
		/* When mapping color buckets back to specific colors,
		 * we can choose to map them to the darkest shades
		 * (e.g., 0, 1/3, 2/3), to the lightest shades (e.g.,
		 * 1/3-epsilon, 2/3-epsilon, 1-epsilon), to the middle
		 * shades (e.g., 1/6, 1/2, 5/6), or for maximum range
		 * (e.g., 0, 1/2, 1).  The last of these matches the
		 * assumptions of the halftoning code, so that is what
		 * we choose.
		 */
		prgb[0] = r * gx_max_color_value / cmap->red_max;
		prgb[1] = g * gx_max_color_value / cmap->green_max;
		prgb[2] = b * gx_max_color_value / cmap->blue_max;
		return 0;
	    }
	}
    }
    if (color < xdev->cman.color_to_rgb.size) {
#endif
	/* Error -- undefined pixel value. */
	return_error(gs_error_unknownerror);
    }
    /*
     * Check the dither cube/ramp.  This is hardly ever used, since if
     * there are few enough colors to require dithering, the pixel values
     * are likely to be small enough to index color_to_rgb.
     */
    if (xdev->cman.dither_ramp) {
	if (gx_device_has_color(xdev)) {
	    int size = xdev->color_info.dither_colors;
	    int size3 = size * size * size;
	    int i;

	    for (i = 0; i < size3; ++i)
		if (xdev->cman.dither_ramp[i] == color) {
		    uint max_rgb = size - 1;
		    uint q = i / size,
			r = q / size,
			g = q % size,
			b = i % size;

		    /*
		     * See above regarding the choice of color mapping
		     * algorithm.
		     */
		    prgb[0] = r * gx_max_color_value / max_rgb;
		    prgb[1] = g * gx_max_color_value / max_rgb;
		    prgb[2] = b * gx_max_color_value / max_rgb;
		    return 0;
		}
	} else {
	    int size = xdev->color_info.dither_grays;
	    int i;

	    for (i = 0; i < size; ++i)
		if (xdev->cman.dither_ramp[i] == color) {
		    prgb[0] = prgb[1] = prgb[2] =
			i * gx_max_color_value / (size - 1);
		    return 0;
		}
	}
    }

    /* Finally, search the list of dynamic colors. */
    if (xdev->cman.dynamic.colors) {
	int i;
	const x11_color_t *xcp;

	for (i = xdev->cman.dynamic.size; --i >= 0;)
	    for (xcp = xdev->cman.dynamic.colors[i]; xcp; xcp = xcp->next)
		if (xcp->color.pixel == color && xcp->color.pad) {
		    prgb[0] = xcp->color.red;
		    prgb[1] = xcp->color.green;
		    prgb[2] = xcp->color.blue;
		    return 0;
		}
    }

    /* Not found -- not possible! */
    return_error(gs_error_unknownerror);
}
