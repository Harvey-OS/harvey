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

/*$Id: gxclpage.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Page object management */
#include "gdevprn.h"
#include "gxcldev.h"
#include "gxclpage.h"

/* Save a page. */
int
gdev_prn_save_page(gx_device_printer * pdev, gx_saved_page * page,
		   int num_copies)
{
    /* Make sure we are banding. */
    if (!pdev->buffer_space)
	return_error(gs_error_rangecheck);
    if (strlen(pdev->dname) >= sizeof(page->dname))
	return_error(gs_error_limitcheck);
    {
	gx_device_clist_writer * const pcldev =
	    (gx_device_clist_writer *)pdev;
	int code;

	if ((code = clist_end_page(pcldev)) < 0 ||
	    (code = clist_fclose(pcldev->page_cfile, pcldev->page_cfname, false)) < 0 ||
	    (code = clist_fclose(pcldev->page_bfile, pcldev->page_bfname, false)) < 0
	    )
	    return code;
	/* Save the device information. */
	memcpy(&page->device, pdev, sizeof(gx_device));
	strcpy(page->dname, pdev->dname);
	/* Save the page information. */
	page->info = pcldev->page_info;
	page->info.cfile = 0;
	page->info.bfile = 0;
    }
    /* Save other information. */
    page->num_copies = num_copies;
    return (*gs_clist_device_procs.open_device) ((gx_device *) pdev);
}

/* Render an array of saved pages. */
int
gdev_prn_render_pages(gx_device_printer * pdev,
		      const gx_placed_page * ppages, int count)
{
    gx_device_clist_reader * const pcldev =
	(gx_device_clist_reader *)pdev;

    /* Check to make sure the pages are compatible with the device. */
    {
	int i;
	gx_band_params_t params;

	for (i = 0; i < count; ++i) {
	    const gx_saved_page *page = ppages[i].page;

	    /* We would like to fully check the color representation, */
	    /* but we don't have enough information to do that. */
	    if (strcmp(page->dname, pdev->dname) != 0 ||
		memcmp(&page->device.color_info, &pdev->color_info,
		       sizeof(pdev->color_info)) != 0
		)
		return_error(gs_error_rangecheck);
	    /* Currently we don't allow translation in Y. */
	    if (ppages[i].offset.y != 0)
		return_error(gs_error_rangecheck);
	    /* Make sure the band parameters are compatible. */
	    if (page->info.band_params.BandBufferSpace !=
		pdev->buffer_space ||
		page->info.band_params.BandWidth !=
		pdev->width
		)
		return_error(gs_error_rangecheck);
	    /* Currently we require all band heights to be the same. */
	    if (i == 0)
		params = page->info.band_params;
	    else if (page->info.band_params.BandHeight !=
		     params.BandHeight
		)
		return_error(gs_error_rangecheck);
	}
    }
    /* Set up the page list in the device. */
    /****** SHOULD FACTOR THIS OUT OF clist_render_init ******/
    pcldev->ymin = pcldev->ymax = 0;
    pcldev->pages = ppages;
    pcldev->num_pages = count;
    /* Render the pages. */
    {
	int code = (*dev_proc(pdev, output_page))
	    ((gx_device *) pdev, ppages[0].page->num_copies, true);

	/* Delete the temporary files. */
	int i;

	for (i = 0; i < count; ++i) {
	    const gx_saved_page *page = ppages[i].page;

	    clist_unlink(page->info.cfname);
	    clist_unlink(page->info.bfname);
	}
	return code;
    }
}
