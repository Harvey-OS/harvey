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

/* icolor.h */
/* Declarations for transfer function & similar cache remapping */

/*
 * All caches use the same mapping function for the library layer;
 * it simply looks up the value in the cache.
 */
float gs_mapped_transfer(P2(floatp, const gx_transfer_map *));

/* Define the number of stack slots needed for zcolor_remap_one. */
/* The client is responsible for doing check_e/ostack or the equivalent */
/* before calling zcolor_remap_one. */
extern const int zcolor_remap_one_ostack;
extern const int zcolor_remap_one_estack;

/* Schedule the sampling and reloading of a cache. */
int zcolor_remap_one(P5(const ref *, os_ptr, gx_transfer_map *,
			const gs_state *, int (*)(P1(os_ptr))));

/* Reload a cache with entries in [0..1] after sampling. */
int zcolor_remap_one_finish(P1(os_ptr));

/* Reload a cache with entries in [-1..1] after sampling. */
int zcolor_remap_one_signed_finish(P1(os_ptr));

/* Recompute the effective transfer functions and invalidate the current */
/* color after cache reloading. */
int zcolor_reset_transfer(P1(os_ptr));

/* Invalidate the current color after cache reloading. */
int zcolor_remap_color(P1(os_ptr));
