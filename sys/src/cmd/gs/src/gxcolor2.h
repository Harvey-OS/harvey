/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxcolor2.h,v 1.8 2003/04/21 15:39:47 igor Exp $ */
/* Internal definitions for Level 2 color routines */
/* Requires gsstruct.h, gxfixed.h */

#ifndef gxcolor2_INCLUDED
#  define gxcolor2_INCLUDED

#include "gscolor2.h"
#include "gsmatrix.h"		/* for step_matrix */
#include "gsrefct.h"
#include "gxbitmap.h"

/* Cache for Indexed color with procedure, or Separation color. */
struct gs_indexed_map_s {
    rc_header rc;
    union {
	int (*lookup_index)(const gs_indexed_params *, int, float *);
	int (*tint_transform)(const gs_separation_params *, floatp, float *);
    } proc;
    void *proc_data;
    uint num_values;	/* base_space->type->num_components * (hival + 1) */
    float *values;	/* actually [num_values] */
};
#define private_st_indexed_map() /* in gscolor2.c */\
  gs_private_st_ptrs2(st_indexed_map, gs_indexed_map, "gs_indexed_map",\
    indexed_map_enum_ptrs, indexed_map_reloc_ptrs, proc_data, values)

/* Define a lookup_index procedure that just returns the map values. */
int lookup_indexed_map(const gs_indexed_params *, int, float *);

/* Allocate an indexed map and its values. */
/* The initial reference count is 1. */
int alloc_indexed_map(gs_indexed_map ** ppmap, int num_values,
		      gs_memory_t * mem, client_name_t cname);

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

#ifndef gs_pattern1_instance_t_DEFINED
#  define gs_pattern1_instance_t_DEFINED
typedef struct gs_pattern1_instance_s gs_pattern1_instance_t;
#endif

struct gs_pattern1_instance_s {
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
};

#define private_st_pattern1_instance() /* in gsptype1.c */\
  gs_private_st_composite(st_pattern1_instance, gs_pattern1_instance_t,\
    "gs_pattern1_instance_t", pattern1_instance_enum_ptrs,\
    pattern1_instance_reloc_ptrs)

#endif /* gxcolor2_INCLUDED */
