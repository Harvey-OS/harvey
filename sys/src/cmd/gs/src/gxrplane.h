/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxrplane.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
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
int gx_render_plane_init(gx_render_plane_t *render_plane,
			 const gx_device *dev, int index);

#endif /* gxrplane_INCLUDED */
