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

/*$Id: gscssub.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Color space substitution "operators" */
#include "gx.h"
#include "gserrors.h"
#include "gscssub.h"
#include "gxcspace.h"		/* for st_color_space */
#include "gxdevcli.h"
#include "gzstate.h"

/* .setsubstitutecolorspace */
int
gs_setsubstitutecolorspace(gs_state *pgs, gs_color_space_index csi,
			   const gs_color_space *pcs)
{
    int index = (int)csi;
    static const uint masks[3] = {
	(1 << gs_color_space_index_DeviceGray) |
	  (1 << gs_color_space_index_CIEA),
	(1 << gs_color_space_index_DeviceRGB) |
	  (1 << gs_color_space_index_CIEABC) |
	  (1 << gs_color_space_index_CIEDEF),
	(1 << gs_color_space_index_DeviceCMYK) |
	  (1 << gs_color_space_index_CIEDEFG)
    };
    const gs_color_space *pcs_old;

    if (index < 0 || index > 2)
	return_error(gs_error_rangecheck);
    if (pcs) {
	if (!masks[index] & (1 << gs_color_space_get_index(pcs)))
	    return_error(gs_error_rangecheck);
    }
    pcs_old = pgs->device_color_spaces.indexed[index];
    if (pcs_old == 0) {
	gs_color_space *pcs_new;

	if (pcs == 0 || gs_color_space_get_index(pcs) == csi)
	    return 0;
	pcs_new = gs_alloc_struct(pgs->memory, gs_color_space, &st_color_space,
				  "gs_setsubstitutecolorspace");
	if (pcs_new == 0)
	    return_error(gs_error_VMerror);
	gs_cspace_init_from(pcs_new, pcs);
	pgs->device_color_spaces.indexed[index] = pcs_new;
    } else {
	gs_cspace_assign(pgs->device_color_spaces.indexed[index],
			 (pcs ? pcs :
			  pgs->shared->device_color_spaces.indexed[index]));
    }
    return 0;
}

/* Possibly-substituted color space accessors. */
const gs_color_space *
gs_current_DeviceGray_space(const gs_state *pgs)
{
    const gs_color_space *pcs;

    return (!pgs->device->UseCIEColor ||
	    (pcs = pgs->device_color_spaces.named.Gray) == 0 ?
	    pgs->shared->device_color_spaces.named.Gray : pcs);
}
const gs_color_space *
gs_current_DeviceRGB_space(const gs_state *pgs)
{
    const gs_color_space *pcs;

    return (!pgs->device->UseCIEColor ||
	    (pcs = pgs->device_color_spaces.named.RGB) == 0 ?
	    pgs->shared->device_color_spaces.named.RGB : pcs);
}
const gs_color_space *
gs_current_DeviceCMYK_space(const gs_state *pgs)
{
    const gs_color_space *pcs;

    return (!pgs->device->UseCIEColor ||
	    (pcs = pgs->device_color_spaces.named.CMYK) == 0 ?
	    pgs->shared->device_color_spaces.named.CMYK : pcs);
}

/* .currentsubstitutecolorspace */
const gs_color_space *
gs_currentsubstitutecolorspace(const gs_state *pgs, gs_color_space_index csi)
{
    switch (csi) {
    case gs_color_space_index_DeviceGray:
	return gs_current_DeviceGray_space(pgs);
    case gs_color_space_index_DeviceRGB:
	return gs_current_DeviceRGB_space(pgs);
    case gs_color_space_index_DeviceCMYK:
	return gs_current_DeviceCMYK_space(pgs);
    default:
	return 0;
    }
}
