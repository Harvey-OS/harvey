/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxdither.h */
/* Interface to gxdither.c */

/* Render a gray, possibly by halftoning. */
int gx_render_gray(P3(frac, gx_device_color *, const gs_state *));

/* Render a color, possibly by halftoning. */
int gx_render_color(P7(frac, frac, frac, frac, bool, gx_device_color *, const gs_state *));
#define gx_render_rgb(r, g, b, pdevc, pgs)\
  gx_render_color(r, g, b, frac_0, false, pdevc, pgs)
#define gx_render_cmyk(c, m, y, k, pdevc, pgs)\
  gx_render_color(c, m, y, k, true, pdevc, pgs)
