/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevplnx.h,v 1.1 2000/03/09 08:40:41 lpd Exp $*/
/* Definitions and API for plane extraction device */
/* Requires gxdevcli.h */

#ifndef gdevplnx_INCLUDED
#  define gdevplnx_INCLUDED

#include "gxrplane.h"

/*
 * A plane-extraction device appears to its client to be a color-capable
 * device, like its target; but it actually extracts a single color plane
 * for rendering to yet another device, called the plane device (normally,
 * but not necessarily, a memory device).  Clients must know the pixel
 * representation in detail, since the plane is specified as a particular
 * group of bits within the pixel.
 *
 * The original purpose of plane-extraction devices is for band list
 * rendering for plane-oriented color printers.  Each per-plane rasterizing
 * pass over the band list sends the output to a plane-extraction device for
 * the plane being printed.  There is one special optimization to support
 * this: on the theory that even bands containing output for multiple bands
 * are likely to have many objects that only write white into that band, we
 * remember whether any (non-white) marks have been made on the page so far,
 * and if not, we simply skip any rendering operations that write white.
 *
 * The depth of the plane_extract device and its target are limited to 32
 * bits; the depth of each plane is limited to 8 bits.  We could raise these
 * without too much trouble if necessary, as long as each plane didn't
 * exceed 32 bits.
 */

typedef struct gx_device_plane_extract_s {
    gx_device_forward_common;
	/* The following are set by the client before opening the device. */
    gx_device *plane_dev;		/* the drawing device for the plane */
    gx_render_plane_t plane;
	/* The following are set by open_device. */
    gx_color_index plane_white;
    uint plane_mask;
    bool plane_dev_is_memory;
	/* The following change dynamically. */
    bool any_marks;
} gx_device_plane_extract;
extern_st(st_device_plane_extract);
#define public_st_device_plane_extract()	/* in gdevplnx.c */\
  gs_public_st_complex_only(st_device_plane_extract, gx_device_plane_extract,\
    "gx_device_plane_extract", 0, device_plane_extract_enum_ptrs,\
    device_plane_extract_reloc_ptrs, gx_device_finalize)

/* Initialize a plane extraction device. */
int plane_device_init(P5(gx_device_plane_extract *edev, gx_device *target,
			 gx_device *plane_dev,
			 const gx_render_plane_t *render_plane,
			 bool clear));

#endif /* gdevplnx_INCLUDED */
