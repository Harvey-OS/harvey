/* Copyright (C) 1992, 1993, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscsepr.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Client interface to Separation color */

#ifndef gscsepr_INCLUDED
#  define gscsepr_INCLUDED

#include "gscspace.h"

/* Graphics state */
bool gs_currentoverprint(P1(const gs_state *));
void gs_setoverprint(P2(gs_state *, bool));

/*
 * Separation color spaces.
 *
 * The API for creating Separation color space objects exposes the fact that
 * they normally cache the results of sampling the tint_transform procedure,
 * and use the cache to convert colors when necessary.  When a language
 * interpreter sets up a Separation space, it may either provide a
 * tint_tranform procedure that will be called each time (specifying the
 * cache size as 0), or it may fill in the cache directly and provide a
 * dummy procedure.
 *
 * By default, the tint transformation procedure will simple return the
 * entries in the cache. If this function is called when the cache size is
 * 0, all color components in the alternative color space will be set to 0.
 */
extern int gs_cspace_build_Separation(P5(
					 gs_color_space ** ppcspace,
					 gs_separation_name sname,
					 const gs_color_space * palt_cspace,
					 int cache_size,
					 gs_memory_t * pmem
					 ));

/* Get the cached value array for a Separation color space. */
/* VMS limits procedure names to 31 characters. */
extern float *gs_cspace_get_sepr_value_array(P1(
						const gs_color_space * pcspace
						));
/* BACKWARD COMPATIBILITY */
#define gs_cspace_get_separation_value_array gs_cspace_get_sepr_value_array

/* Set the tint transformation procedure for a separation color space. */
/* VMS limits procedure names to 31 characters. */
extern int gs_cspace_set_tint_xform_proc(P2(
					    gs_color_space * pcspace,
			       int (*proc) (P3(const gs_separation_params *,
					       floatp,
					       float *
					       ))
					    ));
/* BACKWARD COMPATIBILITY */
#define gs_cspace_set_tint_transform_proc gs_cspace_set_tint_xform_proc

#endif /* gscsepr_INCLUDED */
