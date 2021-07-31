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

/* gxdraw.h */
/* Interface to drawing routines in gxdraw.c */
/* Requires gxfixed.h, gxdcolor.h */

extern int gx_fill_trapezoid_fixed(P9(fixed fx0, fixed fw0, fixed fy0,
				       fixed fx1, fixed fw1, fixed fh,
				       bool swap_axes,
				       const gx_device_color *pdevc,
				       const gs_state *pgs));
extern int gx_fill_pgram_fixed(P8(fixed px, fixed py, fixed ax, fixed ay,
				  fixed bx, fixed by,
				  const gx_device_color *pdevc,
				  const gs_state *pgs));
extern int gx_draw_line_fixed(P6(fixed ixf, fixed iyf,
				 fixed itoxf, fixed itoyf,
				 const gx_device_color *pdevc,
				 const gs_state *pgs));
