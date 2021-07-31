/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevppla.c,v 1.2 2000/09/19 19:00:21 lpd Exp $ */
/* Support for printer devices with planar buffering. */
#include "gdevprn.h"
#include "gdevmpla.h"
#include "gdevppla.h"


/* Set the buf_procs in a printer device to planar mode. */
int
gdev_prn_set_procs_planar(gx_device *dev)
{
    gx_device_printer * const pdev = (gx_device_printer *)dev;

    pdev->printer_procs.buf_procs.create_buf_device =
	gdev_prn_create_buf_planar;
    pdev->printer_procs.buf_procs.size_buf_device =
	gdev_prn_size_buf_planar;
    return 0;
}

/* Open a printer device, conditionally setting it to be planar. */
int
gdev_prn_open_planar(gx_device *dev, bool upb)
{
    if (upb)
	gdev_prn_set_procs_planar(dev);
    return gdev_prn_open(dev);
}

/* Augment get/put_params to add UsePlanarBuffer. */
int
gdev_prn_get_params_planar(gx_device * pdev, gs_param_list * plist,
			   bool *pupb)
{
    int ecode = gdev_prn_get_params(pdev, plist);

    if (ecode < 0)
	return ecode;
    return param_write_bool(plist, "UsePlanarBuffer", pupb);
}
int
gdev_prn_put_params_planar(gx_device * pdev, gs_param_list * plist,
			   bool *pupb)
{
    bool upb = *pupb;
    int ecode = 0, code;

    if (pdev->color_info.num_components > 1)
	ecode = param_read_bool(plist, "UsePlanarBuffer", &upb);
    code = gdev_prn_put_params(pdev, plist);
    if (ecode >= 0)
	ecode = code;
    if (ecode >= 0)
	*pupb = upb;
    return ecode;
}

/* Set the buffer device to planar mode. */
private int
gdev_prn_set_planar(gx_device_memory *mdev, const gx_device *tdev)
{
    int num_comp = tdev->color_info.num_components;
    gx_render_plane_t planes[4];
    int depth = tdev->color_info.depth / num_comp;

    if (num_comp < 3 || num_comp > 4)
	return_error(gs_error_rangecheck);
    /* Round up the depth per plane to a power of 2. */
    while (depth & (depth - 1))
	--depth, depth = (depth | (depth >> 1)) + 1;
    planes[3].depth = planes[2].depth = planes[1].depth = planes[0].depth =
	depth;
    /* We want the most significant plane to come out first. */
    planes[0].shift = depth * (num_comp - 1);
    planes[1].shift = planes[0].shift - depth;
    planes[2].shift = planes[1].shift - depth;
    planes[3].shift = 0;
    return gdev_mem_set_planar(mdev, num_comp, planes);
}

/* Create a planar buffer device. */
int
gdev_prn_create_buf_planar(gx_device **pbdev, gx_device *target,
			   const gx_render_plane_t *render_plane,
			   gs_memory_t *mem, bool for_band)
{
    int code = gx_default_create_buf_device(pbdev, target, render_plane, mem,
					    for_band);

    if (code < 0)
	return code;
    if (gs_device_is_memory(*pbdev) /* == render_plane->index < 0 */) {
	code = gdev_prn_set_planar((gx_device_memory *)*pbdev, *pbdev);
    }
    return code;
}

/* Determine the space needed by a planar buffer device. */
int
gdev_prn_size_buf_planar(gx_device_buf_space_t *space, gx_device *target,
			 const gx_render_plane_t *render_plane,
			 int height, bool for_band)
{
    gx_device_memory mdev;

    if (render_plane && render_plane->index >= 0)
	return gx_default_size_buf_device(space, target, render_plane,
					  height, for_band);
    mdev.color_info = target->color_info;
    gdev_prn_set_planar(&mdev, target);
    space->bits = gdev_mem_bits_size(&mdev, target->width, height);
    space->line_ptrs = gdev_mem_line_ptrs_size(&mdev, target->width, height);
    space->raster = bitmap_raster(target->width * mdev.planes[0].depth);
    return 0;
}
