/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gscpixel.h,v 1.2 2000/09/19 19:00:26 lpd Exp $ */
/* Interface to DevicePixel color space */
/* Requires gscspace.h */

#ifndef gscpixel_INCLUDED
#  define gscpixel_INCLUDED

/* Initialize a DevicePixel color space. */
int gs_cspace_init_DevicePixel(P2(gs_color_space * pcs, int depth));

#endif /* gscpixel_INCLUDED */
