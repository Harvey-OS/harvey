/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscpixel.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* DevicePixel color space and operation definition */
#include "gx.h"
#include "gserrors.h"
#include "gsrefct.h"
#include "gxcspace.h"
#include "gscpixel.h"
#include "gxdevice.h"

/* Define the DevicePixel color space type. */
private cs_proc_restrict_color(gx_restrict_DevicePixel);
private cs_proc_remap_concrete_color(gx_remap_concrete_DevicePixel);
private cs_proc_concretize_color(gx_concretize_DevicePixel);
private const gs_color_space_type gs_color_space_type_DevicePixel = {
    gs_color_space_index_DevicePixel, true, false,
    &st_base_color_space, gx_num_components_1,
    gx_no_base_space,
    gx_init_paint_1, gx_restrict_DevicePixel,
    gx_same_concrete_space,
    gx_concretize_DevicePixel, gx_remap_concrete_DevicePixel,
    gx_default_remap_color, gx_no_install_cspace,
    gx_no_adjust_cspace_count, gx_no_adjust_color_count
};

/* Initialize a DevicePixel color space. */
int
gs_cspace_init_DevicePixel(gs_color_space * pcs, int depth)
{
    switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
	case 24:
	case 32:
	    break;
	default:
	    return_error(gs_error_rangecheck);
    }
    gs_cspace_init(pcs, &gs_color_space_type_DevicePixel, NULL);
    pcs->params.pixel.depth = depth;
    return 0;
}

/* ------ Internal routines ------ */

/* Force a DevicePixel color into legal range. */
private void
gx_restrict_DevicePixel(gs_client_color * pcc, const gs_color_space * pcs)
{
    /****** NOT ENOUGH BITS IN float OR frac ******/
    floatp pixel = pcc->paint.values[0];
    ulong max_value = (1L << pcs->params.pixel.depth) - 1;

    pcc->paint.values[0] = (pixel < 0 ? 0 : min(pixel, max_value));
}


/* Remap a DevicePixel color. */

private int
gx_concretize_DevicePixel(const gs_client_color * pc, const gs_color_space * pcs,
			  frac * pconc, const gs_imager_state * pis)
{
    /****** NOT ENOUGH BITS IN float OR frac ******/
    pconc[0] = (frac) (ulong) pc->paint.values[0];
    return 0;
}

private int
gx_remap_concrete_DevicePixel(const frac * pconc,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
			      gs_color_select_t select)
{
    color_set_pure(pdc, pconc[0] & ((1 << dev->color_info.depth) - 1));
    return 0;
}
