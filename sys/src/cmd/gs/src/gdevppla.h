/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevppla.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Support for printer devices with planar buffering. */
/* Requires gdevprn.h */

#ifndef gdevppla_INCLUDED
#  define gdevppla_INCLUDED

/* Set the buf_procs in a printer device to planar mode. */
int gdev_prn_set_procs_planar(P1(gx_device *pdev));

/* Open a printer device, conditionally setting it to be planar. */
int gdev_prn_open_planar(P2(gx_device *pdev, bool upb));

/* Augment get/put_params to add UsePlanarBuffer. */
int gdev_prn_get_params_planar(P3(gx_device * pdev, gs_param_list * plist,
				  bool *pupb));
int gdev_prn_put_params_planar(P3(gx_device * pdev, gs_param_list * plist,
				  bool *pupb));

/* Create a planar buffer device. */
/* Use this instead of the default if UsePlanarBuffer is true. */
int gdev_prn_create_buf_planar(P5(gx_device **pbdev, gx_device *target,
				  const gx_render_plane_t *render_plane,
				  gs_memory_t *mem, bool for_band));

/* Determine the space needed by a planar buffer device. */
/* Use this instead of the default if UsePlanarBuffer is true. */
int gdev_prn_size_buf_planar(P5(gx_device_buf_space_t *space,
				gx_device *target,
				const gx_render_plane_t *render_plane,
				int height, bool for_band));

#endif /* gdevppla_INCLUDED */
