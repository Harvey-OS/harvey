/* Copyright (C) 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gsptype2.h,v 1.4 2001/05/11 23:12:11 jackiem Exp $ */
/* Client interface to PatternType 2 Patterns */

#ifndef gsptype2_INCLUDED
#  define gsptype2_INCLUDED

#include "gspcolor.h"
#include "gxfixed.h"

/* ---------------- Types and structures ---------------- */

/* PatternType 2 template */

#ifndef gs_shading_t_DEFINED
#  define gs_shading_t_DEFINED
typedef struct gs_shading_s gs_shading_t;
#endif

typedef struct gs_pattern2_template_s {
    gs_pattern_template_common;
    const gs_shading_t *Shading;
} gs_pattern2_template_t;

#define private_st_pattern2_template() /* in gsptype2.c */\
  gs_private_st_suffix_add1(st_pattern2_template,\
    gs_pattern2_template_t, "gs_pattern2_template_t",\
    pattern2_template_enum_ptrs, pattern2_template_reloc_ptrs,\
    st_pattern_template, Shading)
#define st_pattern2_template_max_ptrs (st_pattern_template_max_ptrs + 1)

/* PatternType 2 instance */

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif

typedef struct gs_pattern2_instance_s {
    gs_pattern_instance_common;
    gs_pattern2_template_t template;
} gs_pattern2_instance_t;

#define private_st_pattern2_instance() /* in gsptype2.c */\
  gs_private_st_composite(st_pattern2_instance, gs_pattern2_instance_t,\
    "gs_pattern2_instance_t", pattern2_instance_enum_ptrs,\
    pattern2_instance_reloc_ptrs)

#ifndef gx_path_DEFINED
#define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif

#ifndef gx_device_DEFINED
#define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/* ---------------- Procedures ---------------- */

/*
 * We should provide a gs_cspace_build_Pattern2 procedure to construct
 * the color space, but we don't.
 */

/* Initialize a PatternType 2 pattern. */
void gs_pattern2_init(P1(gs_pattern2_template_t *));

/* Check device color for Pattern Type 2. */
bool gx_dc_is_pattern2_color(const gx_device_color *pdevc);

/* Fill path or rect, with adjustment, and with a PatternType 2 color. */
int gx_dc_pattern2_fill_path_adjusted(const gx_device_color * pdevc, 
			      gx_path * ppath, gs_fixed_rect * rect, 
			      gx_device * dev);


#endif /* gsptype2_INCLUDED */
