/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxclpage.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Command list procedures for saved pages */
/* Requires gdevprn.h, gxclist.h */

#ifndef gxclpage_INCLUDED
#  define gxclpage_INCLUDED

#include "gxclio.h"

/* ---------------- Procedures ---------------- */

/*
 * Package up the current page in a banding device as a page object.
 * The client must provide storage for the page object.
 * The client may retain the object in memory, or may write it on a file
 * for later retrieval; in the latter case, the client should free the
 * in-memory structure.
 */
int gdev_prn_save_page(gx_device_printer * pdev, gx_saved_page * page,
		       int num_copies);

/*
 * Render an array of saved pages by setting up a modified get_bits
 * procedure and then calling the device's normal output_page procedure.
 * Any current page in the device's buffers is lost.
 * The (0,0) point of each saved page is translated to the corresponding
 * specified offset on the combined page.  (Currently the Y offset must be 0.)
 * The client is responsible for freeing the saved and placed pages.
 *
 * Note that the device instance for rendering need not be, and normally is
 * not, the same as the device from which the pages were saved, but it must
 * be an instance of the same device.  The client is responsible for
 * ensuring that the rendering device's buffer size (BufferSpace value) is
 * the same as the BandBufferSpace value of all the saved pages, and that
 * the device width is the same as the BandWidth value of the saved pages.
 */
int gdev_prn_render_pages(gx_device_printer * pdev,
			  const gx_placed_page * ppages, int count);

#endif /* gxclpage_INCLUDED */
