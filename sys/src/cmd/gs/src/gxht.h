/* Copyright (C) 1993, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxht.h,v 1.9 2004/08/04 19:36:12 stefan Exp $ */
/* Rest of (client) halftone definitions */

#ifndef gxht_INCLUDED
#  define gxht_INCLUDED

#include "gsht1.h"
#include "gsrefct.h"
#include "gxhttype.h"
#include "gxtmap.h"
#include "gscspace.h"

/*
 * Halftone types. Note that for this implementation there are only
 * spot functions, thresholds, and multi-component halftones; the peculiar
 * colored halftones supported by PostScript (HalftoneType's 2 and 4) are
 * not supported.
 *
 * NB1: While this code supports relocation of the client data, it will not
 *      free that data when the halftone is released. The client must handle
 *      that task directly.
 *
 * NB2: The garbage collection code will deal with the user provided data as
 *      a structure pointer allocated on the heap. The client must make
 *      certain this is the case.
 *
 * There is, somewhat unfortunately, no identifier applied to these
 * halftones. This reflects the origin of this graphics library as a set
 * of routines for use by a PostScript interpreter.
 *
 * In PostScript, halftone objects do not exist in an identified form outside
 * of the graphic state. Though Level 2 and PostScript 3 support halftone
 * dictionaries, these are neither read-only structures nor tagged
 * by a unique identifier. Hence, they are not suitable for use as cache keys.
 * Caching of halftones for PostScript is confined to the graphic state,
 * and this holds true for the graphic library as well.
 *
 * Note also that implementing a generalized halftone cache is not trivial,
 * as the device-specific representation of spot halftones depends on the 
 * default transformation for the device, and more generally the device
 * specific representation of halftones may depend on the sense of the device
 * (additive or subtract). Hence, a halftone cache would need to be keyed
 * by device. (This is not an issue when caching halftones in the graphic
 * state as the device is also a component of the graphic state).
 */

/*
 * Note that the transfer_closure members will replace transfer sometime
 * in the future.  For the moment, transfer_closure is only used if
 * transfer = 0.
 */

/* Type 1 halftone.  This is just a Level 1 halftone with */
/* a few extra members. */
typedef struct gs_spot_halftone_s {
    gs_screen_halftone screen;
    bool accurate_screens;
    gs_mapping_proc transfer;	/* OBSOLETE */
    gs_mapping_closure_t transfer_closure;
} gs_spot_halftone;

#define st_spot_halftone_max_ptrs st_screen_halftone_max_ptrs + 1

/* Define common elements for Type 3 and extended Type 3 halftones. */
#define GS_THRESHOLD_HALFTONE_COMMON\
    int width;\
    int height;\
    gs_mapping_closure_t transfer_closure
typedef struct gs_threshold_halftone_common_s {
    GS_THRESHOLD_HALFTONE_COMMON;
} gs_threshold_halftone_common;

/* Type 3 halftone. */
typedef struct gs_threshold_halftone_s {
    GS_THRESHOLD_HALFTONE_COMMON; /* must be first */
    gs_const_string thresholds;
    gs_mapping_proc transfer;	/* OBSOLETE */
} gs_threshold_halftone;

#define st_threshold_halftone_max_ptrs 2

/* Extended Type 3 halftone. */
typedef struct gs_threshold2_halftone_s {
    GS_THRESHOLD_HALFTONE_COMMON; /* must be first */
    int width2;
    int height2;
    int bytes_per_sample;	/* 1 or 2 */
    gs_const_bytestring thresholds; /* nota bene */
} gs_threshold2_halftone;

/* Client-defined halftone that generates a halftone order. */
typedef struct gs_client_order_halftone_s gs_client_order_halftone;

#ifndef gx_ht_order_DEFINED
#  define gx_ht_order_DEFINED
typedef struct gx_ht_order_s gx_ht_order;
#endif
typedef struct gs_client_order_ht_procs_s {

    /*
     * Allocate and fill in the order.  gx_ht_alloc_client_order
     * (see gzht.h) does everything but fill in the actual data.
     */

    int (*create_order) (gx_ht_order * porder,
			 gs_state * pgs,
			 const gs_client_order_halftone * phcop,
			 gs_memory_t * mem);

} gs_client_order_ht_procs_t;
struct gs_client_order_halftone_s {
    int width;
    int height;
    int num_levels;
    const gs_client_order_ht_procs_t *procs;
    const void *client_data;
    gs_mapping_closure_t transfer_closure;
};

#define st_client_order_halftone_max_ptrs 2

/* Define the elements of a Type 5 halftone. */
typedef struct gs_halftone_component_s {
    int comp_number;
    int cname;
    gs_halftone_type type;
    union {
	gs_spot_halftone spot;	/* Type 1 */
	gs_threshold_halftone threshold;	/* Type 3 */
	gs_threshold2_halftone threshold2;	/* Extended Type 3 */
	gs_client_order_halftone client_order;	/* client order */
    } params;
} gs_halftone_component;

extern_st(st_halftone_component);
#define public_st_halftone_component()	/* in gsht1.c */\
  gs_public_st_composite(st_halftone_component, gs_halftone_component,\
    "gs_halftone_component", halftone_component_enum_ptrs,\
    halftone_component_reloc_ptrs)
extern_st(st_ht_component_element);
#define public_st_ht_component_element() /* in gsht1.c */\
  gs_public_st_element(st_ht_component_element, gs_halftone_component,\
    "gs_halftone_component[]", ht_comp_elt_enum_ptrs, ht_comp_elt_reloc_ptrs,\
    st_halftone_component)
#define st_halftone_component_max_ptrs\
  max(max(st_spot_halftone_max_ptrs, st_threshold_halftone_max_ptrs),\
      st_client_order_halftone_max_ptrs)

/* Define the Type 5 halftone itself. */
typedef struct gs_multiple_halftone_s {
    gs_halftone_component *components;
    uint num_comp;
    int (*get_colorname_string)(const gs_memory_t *mem, gs_separation_name colorname_index,
		unsigned char **ppstr, unsigned int *pname_size);
} gs_multiple_halftone;

#define st_multiple_halftone_max_ptrs 1

/*
 * The halftone stored in the graphics state is the union of
 * setscreen, setcolorscreen, Type 1, Type 3, and Type 5.
 *
 * NOTE: it is assumed that all subsidiary structures of halftones (the
 * threshold array(s) for Type 3 halftones or halftone components, and the
 * components array for Type 5 halftones) are allocated with the same
 * allocator as the halftone structure itself.
 */
struct gs_halftone_s {
    gs_halftone_type type;
    rc_header rc;
    union {
	gs_screen_halftone screen;	/* setscreen */
	gs_colorscreen_halftone colorscreen;	/* setcolorscreen */
	gs_spot_halftone spot;	/* Type 1 */
	gs_threshold_halftone threshold;	/* Type 3 */
	gs_threshold2_halftone threshold2;	/* Extended Type 3 */
	gs_client_order_halftone client_order;	/* client order */
	gs_multiple_halftone multiple;	/* Type 5 */
    } params;
};

extern_st(st_halftone);
#define public_st_halftone()	/* in gsht.c */\
  gs_public_st_composite(st_halftone, gs_halftone, "gs_halftone",\
    halftone_enum_ptrs, halftone_reloc_ptrs)
#define st_halftone_max_ptrs\
  max(max(st_screen_halftone_max_ptrs, st_colorscreen_halftone_max_ptrs),\
      max(max(st_spot_halftone_max_ptrs, st_threshold_halftone_max_ptrs),\
	  max(st_client_order_halftone_max_ptrs,\
	      st_multiple_halftone_max_ptrs)))

/* Procedural interface for AccurateScreens */

/*
 * Set/get the default AccurateScreens value (for set[color]screen).
 * Note that this value is stored in a static variable.
 */
void gs_setaccuratescreens(bool);
bool gs_currentaccuratescreens(void);

/*
 * Set/get the value for UseWTS. Also a static, but it's going away.
 */
void gs_setusewts(bool);
bool gs_currentusewts(void);

/* Initiate screen sampling with optional AccurateScreens. */
int gs_screen_init_memory(gs_screen_enum *, gs_state *,
			  gs_screen_halftone *, bool, gs_memory_t *);

#define gs_screen_init_accurate(penum, pgs, phsp, accurate)\
  gs_screen_init_memory(penum, pgs, phsp, accurate, pgs->memory)

/* Procedural interface for MinScreenLevels (a Ghostscript extension) */

/*
 * Set/get the MinScreenLevels value.
 *
 * Note that this value is stored in a static variable.
 */
void gs_setminscreenlevels(uint);
uint gs_currentminscreenlevels(void);

#endif /* gxht_INCLUDED */
