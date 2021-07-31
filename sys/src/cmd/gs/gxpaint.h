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

/* gxpaint.h */
/* Device coordinate painting interface for Ghostscript library */
/* Requires gsstate.h, gxpath.h */

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

/* The band mask is -1<<N, where 1<<N is the size of the band */
/* that a single multi-trapezoid fill must not exceed. */
int gx_fill_path_banded(P6(gx_path *, gx_device_color *, gs_state *,
			   int rule, fixed adjust, fixed band_mask));
#define fill_path_no_band_mask ((fixed)(-1) << (sizeof(fixed) * 8 - 1))
#define gx_fill_path(ppath, pdevc, pgs, rule, adjust)\
  gx_fill_path_banded(ppath, pdevc, pgs, rule, adjust, fill_path_no_band_mask)
int gx_stroke_fill(P2(const gx_path *, gs_state *));
int gx_stroke_add(P3(const gx_path *, gx_path *, gs_state *));
