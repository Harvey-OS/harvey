/* Copyright (C) 1989, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxdither.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
#include "gx.h"
#include "gsstruct.h"
#include "gsdcolor.h"
#include "gxdevice.h"
#include "gxlum.h"
#include "gxcmap.h"
#include "gxdither.h"
#include "gzht.h"

/*
 * Binary halftoning algorithms.
 *
 * The procedures in this file use halftoning (if necessary)
 * to implement a given device color that has already gone through
 * the transfer function.  There are two major cases: gray and color.
 * Gray halftoning always uses a binary screen.  Color halftoning
 * uses either a fast algorithm with a binary screen that produces
 * relatively poor approximations, or a very slow algorithm with a
 * general colored screen (or screens) that faithfully implements
 * the Adobe specifications.
 */

/* Tables for fast computation of fractional color levels. */
/* We have to put the table before any uses of it because of a bug */
/* in the VAX C compiler. */
/* We have to split up the definition of the table itself because of a bug */
/*  in the IBM AIX 3.2 C compiler. */
private const gx_color_value q0[] = {
    0
};
private const gx_color_value q1[] = {
    0, frac_color_(1, 1)
};
private const gx_color_value q2[] = {
    0, frac_color_(1, 2), frac_color_(2, 2)
};
private const gx_color_value q3[] = {
    0, frac_color_(1, 3), frac_color_(2, 3), frac_color_(3, 3)
};
private const gx_color_value q4[] = {
    0, frac_color_(1, 4), frac_color_(2, 4), frac_color_(3, 4),
    frac_color_(4, 4)
};
private const gx_color_value q5[] = {
    0, frac_color_(1, 5), frac_color_(2, 5), frac_color_(3, 5),
    frac_color_(4, 5), frac_color_(5, 5)
};
private const gx_color_value q6[] = {
    0, frac_color_(1, 6), frac_color_(2, 6), frac_color_(3, 6),
    frac_color_(4, 6), frac_color_(5, 6), frac_color_(6, 6)
};
private const gx_color_value q7[] = {
    0, frac_color_(1, 7), frac_color_(2, 7), frac_color_(3, 7),
    frac_color_(4, 7), frac_color_(5, 7), frac_color_(6, 7), frac_color_(7, 7)
};

/* We export fc_color_quo for the fractional_color macro in gzht.h. */
const gx_color_value *const fc_color_quo[8] = {
    q0, q1, q2, q3, q4, q5, q6, q7
};

/* Render a gray, possibly by halftoning. */
int
gx_render_device_gray(frac gray, gx_color_value alpha, gx_device_color * pdevc,
		      gx_device * dev, gx_device_halftone * pdht,
		      const gs_int_point * ht_phase)
{
    bool cmyk = dev->color_info.num_components == 4;

/* Make a special check for black and white. */
    if (alpha == gx_max_color_value) {
	gx_color_value lum;

	switch (gray) {
	    case frac_0:
		lum = 0;
		goto bw;
	    case frac_1:
		lum = gx_max_color_value;
	      bw:color_set_pure(pdevc,
			       (cmyk ?
				gx_map_cmyk_color(dev, 0, 0, 0,
						  gx_max_color_value - lum) :
				gx_map_rgb_color(dev, lum, lum, lum)));
		return 0;
	    default:
		;
	}
    }
/* get a few handy values */
    {
	uint max_value = dev->color_info.dither_grays - 1;
	unsigned long hsize = (unsigned)pdht->order.num_levels;
	unsigned long nshades = hsize * max_value + 1;
	unsigned long lx = (nshades * gray) / (frac_1_long + 1);
	uint v = lx / hsize;
	gx_color_value lum = fractional_color(v, max_value);
	gx_color_index color1;
	int level = lx % hsize;

	/* The following should be a conditional expression, */
	/* but the DECStation 3100 Ultrix 4.3 compiler */
	/* generates bad code for it. */
#define SET_COLOR_LUM(col, lum)\
  if ( cmyk )\
    col = gx_map_cmyk_color(dev, 0, 0, 0,\
			       gx_max_color_value - lum);\
  else if ( alpha == gx_max_color_value )\
    col = gx_map_rgb_color(dev, lum, lum, lum);\
  else\
    col = gx_map_rgb_alpha_color(dev, lum, lum, lum, alpha)
	SET_COLOR_LUM(color1, lum);
	if_debug5('c', "[c]gray=0x%x --> (%d+%d/%lu)/%d\n",
		  (unsigned)gray, v, level, hsize, max_value + 1);
	if (level == 0) {	/* Close enough to a pure color, */
	    /* no dithering needed. */
	    color_set_pure(pdevc, color1);
	    return 0;
	} else {
	    gx_color_index color2;

	    v++;
	    lum = fractional_color(v, max_value);
	    SET_COLOR_LUM(color2, lum);
	    color_set_binary_halftone(pdevc, pdht,
				      color1, color2, level);
	    color_set_phase_mod(pdevc, ht_phase->x, ht_phase->y,
				pdht->order.width,
				pdht->order.full_height);
	    return 1;
	}
#undef SET_COLOR_LUM
    }
}

/* 
 *    Color dithering for Ghostscript.  The underlying device imaging model
 *      supports dithering between two colors to generate intermediate shades.
 *      
 *      If the device has high quality colors (at least 32 values
 *      per axis), we ask it to map the color directly.
 *
 *      Otherwise, things are a bit more complicated.  If the device 
 *      supports N shades of each R, G and B independently, there are a total 
 *      of N*N*N colors.  These colors form a 3-D grid in a cubical color 
 *      space.  The following dithering technique works by locating the 
 *      color we want in this 3-D color grid and finding the eight colors 
 *      that surround it.  In the case of dithering into 8 colors with 1 
 *      bit for each red, green and blue, these eight colors will always 
 *      be the same.
 *
 *      Now we consider all possible diagonal paths between the eight colors
 *      and chose the path that runs closest to our desired color in 3-D
 *      color space.  There are 28 such paths.  Then we find the position
 *      on the path that is closest to our color.
 *
 *      The search is made faster by always reflecting our color into
 *      the bottom octant of the cube and comparing it to 7 paths.
 *      After the best path and the best position on that path are found,
 *      the results are reflected back into the original color space.
 *
 *      NOTE: This code has been tested for B/W and Color imaging with
 *      1, 2, 3 and 8 bits per component.
 *
 *      --- original code by Paul Haeberli @ Silicon Graphics - 1990
 *      --- extensively revised by L. Peter Deutsch, Aladdin Enterprises
 *
 *      lpd 3/14/94: added support for CMYK.
 */

/*
 * The weights are arbitrary, as long as their ratios are correct
 * and they will fit into the difference between a ulong and a frac
 * with room to spare.  By making WEIGHT1 and WEIGHT4 powers of 2,
 * we can turn some multiplies into shifts.
 */
#define WNUM 128000
#define	WEIGHT1		(ulong)(WNUM/1000)	/* 1.0                    */
#define	WEIGHT2		(ulong)(WNUM/1414)	/* 1/sqrt(2.0)            */
#define	WEIGHT3		(ulong)(WNUM/1732)	/* 1/sqrt(3.0)            */
#define WEIGHT4		(ulong)(WNUM/2000)	/* 1/sqrt(4.0)            */

#define	DIAG_R		(0x1)
#define	DIAG_G		(0x2)
#define	DIAG_B		(0x4)
#define DIAG_W		(0x8)
#define	DIAG_RG		(0x3)
#define	DIAG_GB		(0x6)
#define	DIAG_BR		(0x5)
#define	DIAG_RGB	(0x7)
#define DIAG_RGBW	(0xf)

/* What should we do about the W/K component?  For the moment, */
/* we ignore it in the luminance computation. */
#define lum_white_weight 0
private const unsigned short lum_w[16] =
{
    (0 * lum_blue_weight + 0 * lum_green_weight + 0 * lum_red_weight + 0 * lum_white_weight),
    (0 * lum_blue_weight + 0 * lum_green_weight + 1 * lum_red_weight + 0 * lum_white_weight),
    (0 * lum_blue_weight + 1 * lum_green_weight + 0 * lum_red_weight + 0 * lum_white_weight),
    (0 * lum_blue_weight + 1 * lum_green_weight + 1 * lum_red_weight + 0 * lum_white_weight),
    (1 * lum_blue_weight + 0 * lum_green_weight + 0 * lum_red_weight + 0 * lum_white_weight),
    (1 * lum_blue_weight + 0 * lum_green_weight + 1 * lum_red_weight + 0 * lum_white_weight),
    (1 * lum_blue_weight + 1 * lum_green_weight + 0 * lum_red_weight + 0 * lum_white_weight),
    (1 * lum_blue_weight + 1 * lum_green_weight + 1 * lum_red_weight + 0 * lum_white_weight),
    (0 * lum_blue_weight + 0 * lum_green_weight + 0 * lum_red_weight + 1 * lum_white_weight),
    (0 * lum_blue_weight + 0 * lum_green_weight + 1 * lum_red_weight + 1 * lum_white_weight),
    (0 * lum_blue_weight + 1 * lum_green_weight + 0 * lum_red_weight + 1 * lum_white_weight),
    (0 * lum_blue_weight + 1 * lum_green_weight + 1 * lum_red_weight + 1 * lum_white_weight),
    (1 * lum_blue_weight + 0 * lum_green_weight + 0 * lum_red_weight + 1 * lum_white_weight),
    (1 * lum_blue_weight + 0 * lum_green_weight + 1 * lum_red_weight + 1 * lum_white_weight),
    (1 * lum_blue_weight + 1 * lum_green_weight + 0 * lum_red_weight + 1 * lum_white_weight),
    (1 * lum_blue_weight + 1 * lum_green_weight + 1 * lum_red_weight + 1 * lum_white_weight)
};

/*
 * Render RGB or CMYK, possibly by halftoning.  If we are rendering RGB,
 * white is ignored.  If we are rendering CMYK, red/green/blue/white are
 * actually cyan/magenta/yellow/black.
 */
int
gx_render_device_color(frac red, frac green, frac blue, frac white, bool cmyk,
	gx_color_value alpha, gx_device_color * pdevc, gx_device * dev,
	gx_device_halftone * pdht, const gs_int_point * ht_phase)
{
    uint max_value = dev->color_info.dither_colors - 1;
    uint num_levels = pdht->order.num_levels;
    frac rem_r, rem_g, rem_b, rem_w;
    uint r, g, b, w;
    gx_color_value vr, vg, vb, vw;

#define MAP_COLOR_RGB()\
  (alpha == gx_max_color_value ?\
   gx_map_rgb_color(dev, vr, vg, vb) :\
   gx_map_rgb_alpha_color(dev, vr, vg, vb, alpha))
#define MAP_COLOR_CMYK()\
  gx_map_cmyk_color(dev, vr, vg, vb, vw)
#define MAP_COLOR()\
  (cmyk ? MAP_COLOR_CMYK() : MAP_COLOR_RGB())

    /* Compute the quotient and remainder of each color component */
    /* with the actual number of available colors. */
    switch (max_value) {
	case 1:		/* 8 or 16 colors */
	    if (red == frac_1)
		rem_r = 0, r = 1;
	    else
		rem_r = red, r = 0;
	    if (green == frac_1)
		rem_g = 0, g = 1;
	    else
		rem_g = green, g = 0;
	    if (blue == frac_1)
		rem_b = 0, b = 1;
	    else
		rem_b = blue, b = 0;
	    if (white == frac_1)
		rem_w = 0, w = 1;
	    else
		rem_w = white, w = 0;
	    break;
	default:
	    {
		ulong want_r, want_g, want_b, want_w;

		want_r = (ulong) max_value * red;
		r = frac_1_quo(want_r);
		rem_r = frac_1_rem(want_r, r);

		want_g = (ulong) max_value * green;
		g = frac_1_quo(want_g);
		rem_g = frac_1_rem(want_g, g);

		want_b = (ulong) max_value * blue;
		b = frac_1_quo(want_b);
		rem_b = frac_1_rem(want_b, b);

		want_w = (ulong) max_value * white;
		w = frac_1_quo(want_w);
		rem_w = frac_1_rem(want_w, w);
	    }
    }

    /* Check for no dithering required */
    if (!(rem_r | rem_g | rem_b | rem_w)) {
	vr = fractional_color(r, max_value);
	vg = fractional_color(g, max_value);
	vb = fractional_color(b, max_value);
	vw = fractional_color(w, max_value);
	color_set_pure(pdevc, MAP_COLOR());
	return 0;
    }
    if_debug12('c', "[c]rgbw=0x%x,0x%x,0x%x,0x%x -->\n   %x+0x%x,%x+0x%x,%x+0x%x,%x+0x%x -->\n",
	    (unsigned)red, (unsigned)green, (unsigned)blue, (unsigned)white,
	       (unsigned)r, (unsigned)rem_r, (unsigned)g, (unsigned)rem_g,
	       (unsigned)b, (unsigned)rem_b, (unsigned)w, (unsigned)rem_w);

    /* Dithering is required.  Choose between two algorithms. */

    if (dev->color_info.num_components >= 4) {
	/* This is a CMYK device. */
	/* Use the slow, general colored halftone algorithm. */
#define RGB_REM(rem_v, i)\
  (rem_v * (ulong)(pdht->components ? pdht->components[pdht->color_indices[i]].corder.num_levels : num_levels) / frac_1)
	uint lr = RGB_REM(rem_r, 0), lg = RGB_REM(rem_g, 1),
	    lb = RGB_REM(rem_b, 2);

	if (cmyk)
	    color_set_cmyk_halftone(pdevc, pdht, r, lr, g, lg, b, lb,
				    w, RGB_REM(rem_w, 3));
	else
	    color_set_rgb_halftone(pdevc, pdht, r, lr, g, lg, b, lb, alpha);
#undef RGB_REM
	color_set_phase_mod(pdevc, ht_phase->x, ht_phase->y,
			    pdht->lcm_width, pdht->lcm_height);
	if (!(pdevc->colors.colored.plane_mask &
	      (pdevc->colors.colored.plane_mask - 1))) {
	    /* We can reduce this color to a binary halftone or pure color. */
	    return gx_reduce_colored_halftone(pdevc, dev, cmyk);
	}
	return 1;
    }
    /* Fast, approximate binary halftone algorithm. */

    {
	ulong hsize = num_levels;
	int adjust_r, adjust_b, adjust_g, adjust_w;
	gx_color_index color1;
	frac amax, amin;
	ulong fmax, cmax;
	int axisc, facec, cubec, diagc;
	unsigned short lum_invert;
	ulong dot1, dot2, dot3, dot4;
	int level;
	int code;

/* Flip the remainder color into the 0, 0, 0 octant. */
	lum_invert = 0;
#define half (frac_1/2)
	if (rem_r > half)
	    rem_r = frac_1 - rem_r,
		adjust_r = -1, r++, lum_invert += lum_red_weight * 2;
	else
	    adjust_r = 1;
	if (rem_g > half)
	    rem_g = frac_1 - rem_g,
		adjust_g = -1, g++, lum_invert += lum_green_weight * 2;
	else
	    adjust_g = 1;
	if (rem_b > half)
	    rem_b = frac_1 - rem_b,
		adjust_b = -1, b++, lum_invert += lum_blue_weight * 2;
	else
	    adjust_b = 1;
	vr = fractional_color(r, max_value);
	vg = fractional_color(g, max_value);
	vb = fractional_color(b, max_value);
	if (cmyk) {
	    if (rem_w > half)
		rem_w = frac_1 - rem_w,
		    adjust_w = -1, w++, lum_invert += lum_white_weight * 2;
	    else
		adjust_w = 1;
	    vw = fractional_color(w, max_value);
	    color1 = MAP_COLOR_CMYK();
	} else
	    color1 = MAP_COLOR_RGB();

/* 
 * Dot the color with each axis to find the best one of 15;
 * find the color at the end of the axis chosen.
 */
	cmax = (ulong) rem_r + rem_g + rem_b;
	dot4 = cmax + rem_w;
	if (rem_g > rem_r) {
	    if (rem_b > rem_g)
		amax = rem_b, axisc = DIAG_B;
	    else
		amax = rem_g, axisc = DIAG_G;
	    if (rem_b > rem_r)
		amin = rem_r, fmax = (ulong) rem_g + rem_b, facec = DIAG_GB;
	    else
		amin = rem_b, fmax = (ulong) rem_r + rem_g, facec = DIAG_RG;
	} else {
	    if (rem_b > rem_r)
		amax = rem_b, axisc = DIAG_B;
	    else
		amax = rem_r, axisc = DIAG_R;
	    if (rem_b > rem_g)
		amin = rem_g, fmax = (ulong) rem_b + rem_r, facec = DIAG_BR;
	    else
		amin = rem_b, fmax = (ulong) rem_r + rem_g, facec = DIAG_RG;
	}
	if (rem_w > amin) {
	    cmax = fmax + rem_w, cubec = facec + DIAG_W;
	    if (rem_w > amax)
		fmax = (ulong) amax + rem_w, facec = axisc + DIAG_W,
		    amax = rem_w, axisc = DIAG_W;
	    else if (rem_w > fmax - amax)
		fmax = (ulong) amax + rem_w, facec = axisc + DIAG_W;
	} else
	    cubec = DIAG_RGB;

	dot1 = amax * WEIGHT1;
	dot2 = fmax * WEIGHT2;
	dot3 = cmax * WEIGHT3;
	/*dot4 see above */
#define use_axis()\
  diagc = axisc, level = (hsize * amax + (frac_1_long / 2)) / frac_1_long
#define use_face()\
  diagc = facec, level = (hsize * fmax + frac_1_long) / (2 * frac_1_long)
#define use_cube()\
  diagc = cubec, level = (hsize * cmax + (3 * frac_1_long / 2)) / (3 * frac_1_long)
#define use_tesseract()\
  diagc = DIAG_RGBW, level = (hsize * dot4 + (2 * frac_1_long)) / (4 * frac_1_long)
	if (dot1 > dot2) {
	    if (dot3 > dot1) {
		if (dot4 * WEIGHT4 > dot3)
		    use_tesseract();
		else
		    use_cube();
	    } else {
		if (dot4 * WEIGHT4 > dot1)
		    use_tesseract();
		else
		    use_axis();
	    }
	} else {
	    if (dot3 > dot2) {
		if (dot4 * WEIGHT4 > dot3)
		    use_tesseract();
		else
		    use_cube();
	    } else {
		if (dot4 * WEIGHT4 > dot2)
		    use_tesseract();
		else
		    use_face();
	    }
	};

	if_debug12('c', "   %x+0x%x,%x+0x%x,%x+0x%x,%x+0x%x; adjust=%d,%d,%d,%d\n",
		 (unsigned)r, (unsigned)rem_r, (unsigned)g, (unsigned)rem_g,
		 (unsigned)b, (unsigned)rem_b, (unsigned)w, (unsigned)rem_w,
		   adjust_r, adjust_g, adjust_b, adjust_w);

	if (level == 0) {
	    color_set_pure(pdevc, color1);
	    code = 0;
	} else {
	    gx_color_index color2;

/* construct the second color, inverting back to original space if needed */
	    if (diagc & DIAG_R)
		r += adjust_r;
	    if (diagc & DIAG_G)
		g += adjust_g;
	    if (diagc & DIAG_B)
		b += adjust_b;
/* get the second device color, sorting by luminance */
	    vr = fractional_color(r, max_value);
	    vg = fractional_color(g, max_value);
	    vb = fractional_color(b, max_value);
	    if (cmyk) {
		if (diagc & DIAG_W)
		    w += adjust_w;
		vw = fractional_color(w, max_value);
		color2 = MAP_COLOR_CMYK();
	    } else
		color2 = MAP_COLOR_RGB();
	    if (level == num_levels) {	/* This can only happen through rounding.... */
		color_set_pure(pdevc, color2);
		code = 0;
	    } else {
		if (lum_w[diagc] < lum_invert)
		    color_set_binary_halftone(pdevc, pdht, color2, color1, hsize - level);
		else
		    color_set_binary_halftone(pdevc, pdht, color1, color2, level);
		color_set_phase_mod(pdevc, ht_phase->x, ht_phase->y,
				    pdht->order.width,
				    pdht->order.full_height);
		code = 1;
	    }
	}

	if_debug7('c', "[c]diagc=%d; colors=0x%lx,0x%lx; level=%d/%d; lum=%d,diag=%d\n",
		  diagc, (ulong) pdevc->colors.binary.color[0],
		  (ulong) pdevc->colors.binary.color[1],
		  level, (unsigned)hsize, lum_invert, lum_w[diagc]);
	return code;
    }
}

/* Reduce a colored halftone to a binary halftone or pure color. */
int
gx_reduce_colored_halftone(gx_device_color *pdevc, gx_device *dev, bool cmyk)
{
    int planes = pdevc->colors.colored.plane_mask;
    gx_color_value max_color = dev->color_info.dither_colors - 1;
    uint b[4];
    gx_color_value v[4];
    gx_color_index c0, c1;

    b[0] = pdevc->colors.colored.c_base[0];
    v[0] = fractional_color(b[0], max_color);
    b[1] = pdevc->colors.colored.c_base[1];
    v[1] = fractional_color(b[1], max_color);
    b[2] = pdevc->colors.colored.c_base[2];
    v[2] = fractional_color(b[2], max_color);
    if (cmyk) {
	b[3] = pdevc->colors.colored.c_base[3];
	v[3] = fractional_color(b[3], max_color);
	c0 = dev_proc(dev, map_cmyk_color)(dev, v[0], v[1], v[2], v[3]);
    } else
	c0 = dev_proc(dev, map_rgb_color)(dev, v[0], v[1], v[2]);
    if (planes == 0) {
	/*
	 * Use a pure color.  This case is unlikely, but it can occur if
	 * (and only if) the difference of each component from the nearest
	 * device color is less than one halftone level.
	 */
	color_set_pure(pdevc, c0);
	return 0;
    } else {
	/* Use a binary color. */
	int i = (planes >> 1) - (planes >> 3);  /* log2 for 1,2,4,8 */
	uint bi = b[i] + 1;
	const gx_device_halftone *pdht = pdevc->colors.colored.c_ht;
	int index =
	    (pdht->components == 0 ? -1 : pdht->color_indices[i]);
	/*
	 * NB: the halftone orders are all set up for an additive color
	 *     space.  To use these work with a cmyk color space, it is
	 *     necessary to invert both the color level and the color
	 *     pair. Note that if the original color was provided an
	 *     additive space, this will reverse (in an approximate sense)
	 *     the color conversion performed to express the color in cmyk
	 *     space.
	 */
	bool invert = dev->color_info.num_components == 4; /****** HACK ******/
	uint level = pdevc->colors.colored.c_level[i];

	v[i] = fractional_color(bi, max_color);
	c1 = (cmyk ?
	      dev_proc(dev, map_cmyk_color)(dev, v[0], v[1], v[2], v[3]) :
	      dev_proc(dev, map_rgb_color)(dev, v[0], v[1], v[2]));
	if (invert) {
	    level =
		(index < 0 ? &pdht->order : &pdht->components[index].corder)
		  ->num_levels - level;
	    color_set_binary_halftone_component(pdevc, pdht, index, c1, c0,
						level);
	} else
	    color_set_binary_halftone_component(pdevc, pdht, index, c0, c1, level);
					    
	return 1;
    }
}

