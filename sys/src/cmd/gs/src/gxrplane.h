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

/*$Id: gxrplane.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Definitions for planar rendering */

#ifndef gxrplane_INCLUDED
#  define gxrplane_INCLUDED

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/*
 * Define the parameters for extracting a single plane from chunky pixels.
 * This structure should be considered opaque, and should only be
 * initialized with the procedure.
 */
typedef struct gx_render_plane_s {
    int depth;
    int shift;			/* bit position of l.s.b. from low end */
    int index;			/* index within multi-screen halftone */
} gx_render_plane_t;

/*
 * Initialize a rendering plane specification for a device.  Note that it is
 * up to the device to decide which bits constitute a given plane identified
 * by index.  (Currently this is done with a fixed procedure, but eventually
 * it will be made a property of the device somehow, perhaps in the
 * color_info.)
 */
int gx_render_plane_init(P3(gx_render_plane_t *render_plane,
			    const gx_device *dev, int index));

#endif /* gxrplane_INCLUDED */
