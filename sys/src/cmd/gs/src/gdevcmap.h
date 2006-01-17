/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevcmap.h,v 1.5 2002/06/16 07:25:26 lpd Exp $ */
/* Interface to special color mapping device */

#ifndef gdevcmap_INCLUDED
#  define gdevcmap_INCLUDED

/* Define the color mapping algorithms. */
typedef enum {

    /* Don't change the color. */

    device_cmap_identity = 0,

    /* Snap each RGB primary component to 0 or 1 individually. */

    device_cmap_snap_to_primaries,

    /* Snap black to white, other colors to black. */

    device_cmap_color_to_black_over_white,

    /* Convert to a gray shade of the correct brightness. */

    device_cmap_monochrome

} gx_device_color_mapping_method_t;

#define device_cmap_max_method device_cmap_monochrome

/* Define the color mapping forwarding device. */
typedef struct gx_device_cmap_s {
    gx_device_forward_common;
    gx_device_color_mapping_method_t mapping_method;
} gx_device_cmap;

extern_st(st_device_cmap);
#define public_st_device_cmap()	/* in gdevcmap.c */\
  gs_public_st_suffix_add0_final(st_device_cmap, gx_device_cmap,\
    "gx_device_cmap", device_cmap_enum_ptrs, device_cmap_reloc_ptrs,\
    gx_device_finalize, st_device_forward)

/* Initialize a color mapping device.  Do this just once after allocation. */
int gdev_cmap_init(gx_device_cmap * dev, gx_device * target,
		   gx_device_color_mapping_method_t mapping_method);

/*
 * Clients can change the color mapping method at any time by setting
 * the ColorMappingMethod device parameter, but they must then call
 *	gs_setdevice_no_init(pgs, dev);
 * for each graphics state that may reference the device.
 */

#endif /* gdevcmap_INCLUDED */
