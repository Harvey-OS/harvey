/* Copyright (C) 1994, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxdither.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Interface to gxdither.c */

#ifndef gxdither_INCLUDED
#  define gxdither_INCLUDED

#include "gxfrac.h"

#ifndef gx_device_halftone_DEFINED
#  define gx_device_halftone_DEFINED
typedef struct gx_device_halftone_s gx_device_halftone;
#endif

/*
 * Note that in the procedures below, the colors are specified by fracs,
 * but the alpha value is a gx_color_value.  This is a design flaw that
 * we might be able to fix eventually.
 */

/* Render a gray, possibly by halftoning. */
/* Return 0 if complete, 1 if caller must do gx_color_load, <0 on error. */
int gx_render_device_gray(P6(frac gray, gx_color_value alpha,
			     gx_device_color * pdevc, gx_device * dev,
			     gx_device_halftone * dev_ht,
			     const gs_int_point * ht_phase));

#define gx_render_gray_alpha(gray, alpha, pdevc, pis, dev, select)\
  gx_render_device_gray(gray, alpha, pdevc, dev, pis->dev_ht,\
			&pis->screen_phase[select])
#define gx_render_gray(gray, pdevc, pis, dev, select)\
  gx_render_gray_alpha(gray, pis->alpha, pdevc, pis, dev, select)

/* Render a color, possibly by halftoning. */
/* Return as for gx_render_[device_]gray. */
int gx_render_device_color(P10(frac red, frac green, frac blue, frac white,
			       bool cmyk, gx_color_value alpha,
			       gx_device_color * pdevc, gx_device * dev,
			       gx_device_halftone * pdht,
			       const gs_int_point * ht_phase));

#define gx_render_color_alpha(r, g, b, w, a, cmyk, pdevc, pis, dev, select)\
  gx_render_device_color(r, g, b, w, cmyk, a, pdevc, dev,\
			 pis->dev_ht, &pis->screen_phase[select])
#define gx_render_color(r, g, b, w, cmyk, pdevc, pis, dev, select)\
  gx_render_color_alpha(r, g, b, w, pis->alpha, cmyk, pdevc, pis, dev, select)
#define gx_render_rgb(r, g, b, pdevc, pis, dev, select)\
  gx_render_color(r, g, b, frac_0, false, pdevc, pis, dev, select)
#define gx_render_cmyk(c, m, y, k, pdevc, pis, dev, select)\
  gx_render_color(c, m, y, k, true, pdevc, pis, dev, select)
#define gx_render_rgb_alpha(r, g, b, a, pdevc, pis, dev, select)\
  gx_render_color_alpha(r, g, b, frac_0, a, false, pdevc, pis, dev, select)

/*
 * Reduce a colored halftone with 0 or 1 varying plane(s) to a pure color
 * or a binary halftone.
 */
int gx_reduce_colored_halftone(P3(gx_device_color *pdevc, gx_device *dev,
				  bool cmyk));

#endif /* gxdither_INCLUDED */
