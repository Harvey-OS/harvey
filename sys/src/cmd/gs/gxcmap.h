/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxcmap.h */
/* Private definition of color mapping for Ghostscript */
/* Requires gxdcolor.h, gxdevice.h. */
#include "gxfmap.h"

/* Procedures for rendering colors specified by fractions. */

#define cmap_proc_gray(proc)\
  void proc(P3(frac, gx_device_color *, const gs_state *))
#define cmap_proc_rgb(proc)\
  void proc(P5(frac, frac, frac, gx_device_color *, const gs_state *))
#define cmap_proc_cmyk(proc)\
  void proc(P6(frac, frac, frac, frac, gx_device_color *, const gs_state *))

typedef struct gx_color_map_procs_s {
	cmap_proc_gray((*map_gray));
	cmap_proc_rgb((*map_rgb));
	cmap_proc_cmyk((*map_cmyk));
} gx_color_map_procs;

/* Set the color mapping procedures in the graphics state. */
/* This is only needed when switching devices. */
void gx_set_cmap_procs(P1(gs_state *));

/* Map a color, with optional tracing if we are debugging. */
#ifdef DEBUG
/* Use procedures in gxcmap.c */
gx_color_index gx_proc_map_rgb_color(P4(gx_device *,
  gx_color_value, gx_color_value, gx_color_value));
gx_color_index gx_proc_map_rgb_alpha_color(P5(gx_device *,
  gx_color_value, gx_color_value, gx_color_value, gx_color_value));
gx_color_index gx_proc_map_cmyk_color(P5(gx_device *,
  gx_color_value, gx_color_value, gx_color_value, gx_color_value));
#  define gx_map_rgb_color(dev, vr, vg, vb)\
     gx_proc_map_rgb_color(dev, vr, vg, vb)
#  define gx_map_rgb_alpha_color(dev, vr, vg, vb, va)\
     gx_proc_map_rgb_alpha_color(dev, vr, vg, vb, va)
#  define gx_map_cmyk_color(dev, vc, vm, vy, vk)\
     gx_proc_map_cmyk_color(dev, vc, vm, vy, vk)
#else
#  define gx_map_rgb_color(dev, vr, vg, vb)\
     (*dev_proc(dev, map_rgb_color))(dev, vr, vg, vb)
#  define gx_map_rgb_alpha_color(dev, vr, vg, vb, va)\
     (*dev_proc(dev, map_rgb_alpha_color))(dev, vr, vg, vb, va)
#  define gx_map_cmyk_color(dev, vc, vm, vy, vk)\
     (*dev_proc(dev, map_cmyk_color))(dev, vc, vm, vy, vk)
#endif
