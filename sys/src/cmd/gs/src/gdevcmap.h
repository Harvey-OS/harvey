/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevcmap.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
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
int gdev_cmap_init(P3(gx_device_cmap * dev, gx_device * target,
		      gx_device_color_mapping_method_t mapping_method));

/*
 * Clients can change the color mapping method at any time by setting
 * the ColorMappingMethod device parameter, but they must then call
 *	gs_setdevice_no_init(pgs, dev);
 * for each graphics state that may reference the device.
 */

#endif /* gdevcmap_INCLUDED */
