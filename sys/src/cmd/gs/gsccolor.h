/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gsccolor.h */
/* Client color structure definition */

#ifndef gsccolor_INCLUDED
#  define gsccolor_INCLUDED

#include "gsstruct.h"			/* for extern_st */

/* Pattern instance, usable in color. */
typedef struct gs_pattern_instance_s gs_pattern_instance;

/* Paint (non-pattern) colors (Device, CIE, Indexed, Separation) */
typedef struct gs_paint_color_s {
	float values[4];
} gs_paint_color;

/* General colors */
typedef struct gs_client_color_s {
	gs_paint_color paint;		/* also color for uncolored pattern */
	gs_pattern_instance *pattern;
} gs_client_color;
extern_st(st_client_color);
#define public_st_client_color() /* in gscolor.c */\
  gs_public_st_ptrs1(st_client_color, gs_client_color, "gs_client_color",\
    client_color_enum_ptrs, client_color_reloc_ptrs, pattern)
#define st_client_color_max_ptrs 1

#endif					/* gsccolor_INCLUDED */
