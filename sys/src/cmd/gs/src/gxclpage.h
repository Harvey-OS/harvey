/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxclpage.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
int gdev_prn_save_page(P3(gx_device_printer * pdev, gx_saved_page * page,
			  int num_copies));

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
int gdev_prn_render_pages(P3(gx_device_printer * pdev,
			     const gx_placed_page * ppages, int count));

#endif /* gxclpage_INCLUDED */
