/* Copyright (C) 1993, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxcolor2.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internal definitions for Level 2 color routines */
/* Requires gsstruct.h, gxfixed.h */

#ifndef gxcolor2_INCLUDED
#  define gxcolor2_INCLUDED

#include "gscolor2.h"
#include "gsrefct.h"
#include "gxbitmap.h"

/* Cache for Indexed color with procedure, or Separation color. */
struct gs_indexed_map_s {
    rc_header rc;
    union {
	int (*lookup_index)(P3(const gs_indexed_params *, int, float *));
	int (*tint_transform)(P3(const gs_separation_params *, floatp, float *));
    } proc;
    uint num_values;	/* base_space->type->num_components * (hival + 1) */
    float *values;	/* actually [num_values] */
};

extern_st(st_indexed_map);
#define public_st_indexed_map() /* in gscolor2.c */\
  gs_public_st_ptrs1(st_indexed_map, gs_indexed_map, "gs_indexed_map",\
    indexed_map_enum_ptrs, indexed_map_reloc_ptrs, values)

/* Define a lookup_index procedure that just returns the map values. */
int lookup_indexed_map(P3(const gs_indexed_params *, int, float *));

/* Allocate an indexed map and its values. */
int alloc_indexed_map(P4(gs_indexed_map ** ppmap, int num_values,
			 gs_memory_t * mem, client_name_t cname));

/* Free an indexed map and its values when the reference count goes to 0. */
rc_free_proc(free_indexed_map);

/**************** TO gxptype1.h ****************/

/*
 * We define 'tiling space' as the space in which (0,0) is the origin of
 * the key pattern cell and in which coordinate (i,j) is displaced by
 * i * XStep + j * YStep from the origin.  In this space, it is easy to
 * compute a (rectangular) set of tile copies that cover a (rectangular)
 * region to be tiled.  Note that since all we care about is that the
 * stepping matrix (the transformation from tiling space to device space)
 * yield the right set of coordinates for integral X and Y values, we can
 * adjust it to make the tiling computation easier; in particular, we can
 * arrange it so that all 4 transformation factors are non-negative.
 */

typedef struct gs_pattern1_instance_s {
    gs_pattern_instance_common;	/* must be first */
    gs_pattern1_template_t template;
    /* Following are created by makepattern */
    gs_matrix step_matrix;	/* tiling space -> device space */
    gs_rect bbox;		/* bbox of tile in tiling space */
    bool is_simple;		/* true if xstep/ystep = tile size */
    /*
     * uses_mask is always true for PostScript patterns, but is false
     * for bitmap patterns that don't have explicit transparent pixels.
     */
    bool uses_mask;	        /* if true, pattern mask must be created */
    gs_int_point size;		/* in device coordinates */
    gx_bitmap_id id;		/* key for cached bitmap (= id of mask) */
} gs_pattern1_instance_t;

#define private_st_pattern1_instance() /* in gsptype1.c */\
  gs_private_st_composite(st_pattern1_instance, gs_pattern1_instance_t,\
    "gs_pattern1_instance_t", pattern1_instance_enum_ptrs,\
    pattern1_instance_reloc_ptrs)

#endif /* gxcolor2_INCLUDED */
