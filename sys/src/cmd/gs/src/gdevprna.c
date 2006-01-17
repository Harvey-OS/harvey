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

/* $Id: gdevprna.c,v 1.6 2004/08/04 19:36:12 stefan Exp $ */
/* Generic asynchronous printer driver support */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */
/* Revised 8/7/98 by L. Peter Deutsch (ghost@aladdin.com) for */
/*   memory manager changes */
/* 12/1/98 soho@crl.com - Removed unnecessary flush & reopen in */
/*         gdev_prn_async_write_get_hardware_params */
#include "gdevprna.h"
#include "gsalloc.h"
#include "gsdevice.h"
#include "gsmemlok.h"
#include "gsmemret.h"
#include "gsnogc.h"
#include "gxcldev.h"
#include "gxclpath.h"
#include "gxpageq.h"
#include "gzht.h"		/* for gx_ht_cache_default_bits */

/* ----------------- Constants ----------------------- */
/*
 * Fixed overhead # bytes to run renderer in (+ driver-spec'd variable bytes):
 * empirical & still very subject to change.
 */
#define RendererAllocationOverheadBytes 503000 /* minimum is 503,000 as of 4/26/99 */

#ifdef DEBUG
/* 196000 is pretty much the minimum, given 16K phys memfile blocks */
/*# define DebugBandlistMemorySize 196000*/ /* comment out to disable fixed (debug) bandlist size */
#endif /* defined(DEBUG) */

/* ---------------- Standard device procedures ---------------- */
private dev_proc_close_device(gdev_prn_async_write_close_device);
private dev_proc_output_page(gdev_prn_async_write_output_page);
private dev_proc_put_params(gdev_prn_async_write_put_params);
private dev_proc_get_hardware_params(gdev_prn_async_write_get_hardware_params);
private dev_proc_put_params(gdev_prn_async_render_put_params);

/* ---------------- Forward Declarations ---------------------- */
private void gdev_prn_dealloc(gx_device_printer *);
private proc_free_up_bandlist_memory(gdev_prn_async_write_free_up_bandlist_memory);
private int flush_page(gx_device_printer *, bool);
private int reopen_clist_after_flush(gx_device_printer *);
private void reinit_printer_into_printera(gx_device_printer * const);
private int alloc_bandlist_memory(gs_memory_t **, gs_memory_t *);
private void free_bandlist_memory(gs_memory_t *);
private int alloc_render_memory(gs_memory_t **, gs_memory_t *, long);
private void free_render_memory(gs_memory_t *);
private gs_memory_recover_status_t
    prna_mem_recover(gs_memory_retrying_t *rmem, void *proc_data);

/* ------ Open/close ------ */

/*
 * Open this printer device in ASYNC (overlapped) mode.
 * This routine must always called by the concrete device's xx_open routine
 * in lieu of gdev_prn_open.
 */
int
gdev_prn_async_write_open(gx_device_printer * pwdev, int max_raster,
			  int min_band_height, int max_src_image_row)
{
    gx_device *const pdev = (gx_device *) pwdev;
    int code;
    bool writer_is_open = false;
    gx_device_clist_writer *const pcwdev =
	&((gx_device_clist *) pwdev)->writer;
    gx_device_clist_reader *pcrdev = 0;
    gx_device_printer *prdev = 0;
    gs_memory_t *render_memory = 0;	/* renderer's mem allocator */

    pwdev->page_queue = 0;
    pwdev->bandlist_memory = 0;
    pwdev->async_renderer = 0;

    /* allocate & init render memory */
    /* The big memory consumers are: */
    /* - the buffer used to read images from the command list */
    /* - buffer used by gx_real_default_strip_copy_rop() */
    /* - line pointer tables for memory devices used in plane extraction */
    /* - the halftone cache */
    /* - the band rendering buffer */
    /* The * 2's in the next statement are a ****** HACK ****** to deal with */
    /* sandbars in the memory manager. */
    if ((code = alloc_render_memory(&render_memory,
	    pwdev->memory->non_gc_memory, RendererAllocationOverheadBytes + max_raster
				    /* the first * 2 is not a hack */
		   + (max_raster + sizeof(void *) * 2) * min_band_height
		   + max_src_image_row + gx_ht_cache_default_bits() * 2)) < 0)
	     goto open_err;

    /* Alloc & init bandlist allocators */
    /* Bandlist mem is threadsafe & common to rdr/wtr, so it's used */
    /* for page queue & cmd list buffers. */
    if ((code = alloc_bandlist_memory
	 (&pwdev->bandlist_memory, pwdev->memory->non_gc_memory)) < 0)
	goto open_err;

    /* Dictate banding parameters for both renderer & writer */
    /* Protect from user change, since user changing these won't be */
    /* detected, ergo the necessary close/reallocate/open wouldn't happen. */
    pwdev->space_params.banding_type = BandingAlways;
    pwdev->space_params.params_are_read_only = true;

    /* Make a copy of device for use as rendering device b4 opening writer */
    code = gs_copydevice((gx_device **) & prdev, pdev, render_memory);
    pcrdev = &((gx_device_clist *) prdev)->reader;
    if (code < 0)
	goto open_err;

    /* -------------- Open cmd list WRITER instance of device ------- */
    /* --------------------------------------------------------------- */
    /* This is wrong, because it causes the same thing in the renderer */
    pwdev->OpenOutputFile = 0;	/* Don't open output file in writer */

    /* Hack: set this vector to tell gdev_prn_open to allocate for async rendering */
    pwdev->free_up_bandlist_memory = &gdev_prn_async_write_free_up_bandlist_memory;

    /* prevent clist writer from queuing path graphics & force it to split images */
    pwdev->clist_disable_mask |= clist_disable_fill_path |
	clist_disable_stroke_path | clist_disable_complex_clip |
	clist_disable_nonrect_hl_image | clist_disable_pass_thru_params;

    if ((code = gdev_prn_open(pdev)) >= 0) {
	writer_is_open = true;

	/* set up constant async-specific fields in device */
	reinit_printer_into_printera(pwdev);

	/* keep ptr to renderer device */
	pwdev->async_renderer = prdev;

	/* Allocate the page queue, then initialize it */
	/* Use bandlist memory since it's shared between rdr & wtr */
	if ((pwdev->page_queue = gx_page_queue_alloc(pwdev->bandlist_memory)) == 0)
	    code = gs_note_error(gs_error_VMerror);
	else
	    /* Allocate from clist allocator since it is thread-safe */
	    code = gx_page_queue_init(pwdev->page_queue, pwdev->bandlist_memory);
    }
    /* ------------ Open cmd list RENDERER instance of device ------- */
    /* --------------------------------------------------------------- */
    if (code >= 0) {
	gx_semaphore_t *open_semaphore;

	/* Force writer's actual band params into reader's requested params */
	prdev->space_params.band = pcwdev->page_info.band_params;

	/* copydevice has already set up prdev->memory = render_memory */
	/* prdev->bandlist_memory = pwdev->bandlist_memory; */
	prdev->buffer_memory = prdev->memory;

	/* enable renderer to accept changes to params computed by writer */
	prdev->space_params.params_are_read_only = false;

	/* page queue is common to both devices */
	prdev->page_queue = pwdev->page_queue;

	/* Start renderer thread & wait for its successful open of device */
	if (!(open_semaphore = gx_semaphore_alloc(prdev->memory)))
	    code = gs_note_error(gs_error_VMerror);
	else {
	    gdev_prn_start_render_params thread_params;

	    thread_params.writer_device = pwdev;
	    thread_params.open_semaphore = open_semaphore;
	    thread_params.open_code = 0;
	    code = (*pwdev->printer_procs.start_render_thread)
		(&thread_params);
	    if (code >= 0)
		gx_semaphore_wait(open_semaphore);
	    code = thread_params.open_code;
	    gx_semaphore_free(open_semaphore);
	}
    }
    /* ----- Set the recovery procedure for the mem allocator ----- */
    if (code >= 0) {
	gs_memory_retrying_set_recover(
		(gs_memory_retrying_t *)pwdev->memory->non_gc_memory,
		prna_mem_recover,
		(void *)pcwdev
	    );
    }
    /* --------------------- Wrap up --------------------------------- */
    /* --------------------------------------------------------------- */
    if (code < 0) {
open_err:
	/* error mop-up */
	if (render_memory && !prdev)
	    free_render_memory(render_memory);

	gdev_prn_dealloc(pwdev);
	if (writer_is_open) {
	    gdev_prn_close(pdev);
	    pwdev->free_up_bandlist_memory = 0;
	}
    }
    return code;
}

/* This procedure is called from within the memory allocator when regular */
/* malloc's fail -- this procedure tries to free up pages from the queue  */
/* and returns a status code indicating whether any more can be freed.    */
private gs_memory_recover_status_t
prna_mem_recover(gs_memory_retrying_t *rmem, void *proc_data)
{
    int pages_remain = 0;
    gx_device_clist_writer *cldev = proc_data;

    if (cldev->free_up_bandlist_memory != NULL)
	pages_remain =
	    (*cldev->free_up_bandlist_memory)( (gx_device *)cldev, false );
    return (pages_remain > 0) ? RECOVER_STATUS_RETRY_OK : RECOVER_STATUS_NO_RETRY;
}

/* (Re)set printer device fields which get trampled by gdevprn_open & put_params */
private void
reinit_printer_into_printera(
			     gx_device_printer * const pdev	/* printer to convert */
)
{
    /* Change some of the procedure vector to point at async procedures */
    /* Originals were already saved by gdev_prn_open */
    if (dev_proc(pdev, close_device) == gdev_prn_close)
	set_dev_proc(pdev, close_device, gdev_prn_async_write_close_device);
    set_dev_proc(pdev, output_page, gdev_prn_async_write_output_page);
    set_dev_proc(pdev, put_params, gdev_prn_async_write_put_params);
    set_dev_proc(pdev, get_xfont_procs, gx_default_get_xfont_procs);
    set_dev_proc(pdev, get_xfont_device, gx_default_get_xfont_device);
    set_dev_proc(pdev, get_hardware_params, gdev_prn_async_write_get_hardware_params);

    /* clist writer calls this if it runs out of memory & wants to retry */
    pdev->free_up_bandlist_memory = &gdev_prn_async_write_free_up_bandlist_memory;
}

/* Generic closing for the writer device. */
private int
gdev_prn_async_write_close_device(gx_device * pdev)
{
    gx_device_printer *const pwdev = (gx_device_printer *) pdev;

    /* Signal render thread to close & terminate when done */
    gx_page_queue_add_page(pwdev->page_queue,
			   GX_PAGE_QUEUE_ACTION_TERMINATE, 0, 0);

    /* Wait for renderer to finish all pages & terminate req */
    gx_page_queue_wait_until_empty(pwdev->page_queue);

    /* Cascade down to original close rtn */
    gdev_prn_close(pdev);
    pwdev->free_up_bandlist_memory = 0;

    /* Deallocte dynamic stuff */
    gdev_prn_dealloc(pwdev);
    return 0;
}

/* Deallocte dynamic memory attached to device. Aware of possible imcomplete open */
private void
gdev_prn_dealloc(gx_device_printer * pwdev)
{
    gx_device_printer *const prdev = pwdev->async_renderer;

    /* Delete renderer device & its memory allocator */
    if (prdev) {
	gs_memory_t *render_alloc = prdev->memory;

	gs_free_object(render_alloc, prdev, "gdev_prn_dealloc");
	free_render_memory(render_alloc);
    }
    /* Free page queue */
    if (pwdev->page_queue) {
	gx_page_queue_dnit(pwdev->page_queue);
	gs_free_object(pwdev->bandlist_memory, pwdev->page_queue,
		       "gdev_prn_dealloc");
	pwdev->page_queue = 0;
    }
    /* Free memory bandlist allocators */
    if (pwdev->bandlist_memory)
	free_bandlist_memory(pwdev->bandlist_memory);
}

/* Open the render portion of a printer device in ASYNC (overlapped) mode.

 * This routine is always called by concrete device's xx_open_render_device
 * in lieu of gdev_prn_open.
 */
int
gdev_prn_async_render_open(gx_device_printer * prdev)
{
    gx_device *const pdev = (gx_device *) prdev;

    prdev->is_async_renderer = true;
    return gdev_prn_open(pdev);
}

/* Generic closing for the rendering device. */
int
gdev_prn_async_render_close_device(gx_device_printer * prdev)
{
    gx_device *const pdev = (gx_device *) prdev;

    return gdev_prn_close(pdev);
}

/* (Re)set renderer device fields which get trampled by gdevprn_open & put_params */
private void
reinit_printer_into_renderer(
			     gx_device_printer * const pdev	/* printer to convert */
)
{
    set_dev_proc(pdev, put_params, gdev_prn_async_render_put_params);
}

/* ---------- Start rasterizer thread ------------ */
/*
 * Must be called by async device driver implementation (see gdevprna.h
 * under "Synchronizing the Instances"). This is the rendering loop, which
 * requires its own thread for as long as the device is open. This proc only
 * returns after the device is closed, or if the open failed. NB that an
 * open error leaves things in a state where the writer thread will not be
 * able to close since it's expecting the renderer to acknowledge its
 * requests before the writer can close.  Ergo, if this routine fails you'll
 * crash unless the caller fixes the problem & successfully retries this.
 */
int				/* rets 0 ok, -ve error code if open failed */
gdev_prn_async_render_thread(
			     gdev_prn_start_render_params * params
)
{
    gx_device_printer *const pwdev = params->writer_device;
    gx_device_printer *const prdev = pwdev->async_renderer;
    gx_page_queue_entry_t *entry;
    int code;

    /* Open device, but don't use default if user didn't override */
    if (prdev->printer_procs.open_render_device ==
	  gx_default_open_render_device)
	code = gdev_prn_async_render_open(prdev);
    else
	code = (*prdev->printer_procs.open_render_device) (prdev);
    reinit_printer_into_renderer(prdev);

    /* The cmd list logic assumes reader's & writer's tile caches are same size */
    if (code >= 0 &&
	  ((gx_device_clist *) pwdev)->writer.page_tile_cache_size !=
	    ((gx_device_clist *) prdev)->writer.page_tile_cache_size) {
	gdev_prn_async_render_close_device(prdev);
	code = gs_note_error(gs_error_VMerror);
    }
    params->open_code = code;
    gx_semaphore_signal(params->open_semaphore);
    if (code < 0)
	return code;

    /* fake open, since not called by gs_opendevice */
    prdev->is_open = true;

    /* Successful open */
    while ((entry = gx_page_queue_start_dequeue(prdev->page_queue))
	   && entry->action != GX_PAGE_QUEUE_ACTION_TERMINATE) {
	/* Force printer open again if it mysteriously closed. */
	/* This shouldn't ever happen, but... */
	if (!prdev->is_open) {
	    if (prdev->printer_procs.open_render_device ==
		  gx_default_open_render_device)
		code = gdev_prn_async_render_open(prdev);
	    else
		code = (*prdev->printer_procs.open_render_device) (prdev);
	    reinit_printer_into_renderer(prdev);

	    if (code >= 0) {
		prdev->is_open = true;
		gdev_prn_output_page((gx_device *) prdev, 0, true);
	    }
	}
	if (prdev->is_open) {
	    /* Force retrieved entry onto render device */
	    ((gx_device_clist *) prdev)->common.page_info = entry->page_info;

	    /* Set up device geometry */
	    if (clist_setup_params((gx_device *) prdev) >= 0)
		/* Go this again, since setup_params may have trashed it */
		((gx_device_clist *) prdev)->common.page_info = entry->page_info;

	    /* Call appropriate renderer routine to deal w/buffer */
	    /* Ignore status, since we don't know how to deal w/errors! */
	    switch (entry->action) {

		case GX_PAGE_QUEUE_ACTION_FULL_PAGE:
		    (*dev_proc(prdev, output_page))((gx_device *) prdev,
						    entry->num_copies, true);
		    break;

		case GX_PAGE_QUEUE_ACTION_PARTIAL_PAGE:
		case GX_PAGE_QUEUE_ACTION_COPY_PAGE:
		    (*dev_proc(prdev, output_page))((gx_device *) prdev,
						    entry->num_copies, false);
		    break;
	    }
	    /*
	     * gx_page_queue_finish_dequeue will close and free the band
	     * list files, so we don't need to call clist_close_output_file.
	     */
	}
	/* Finalize dequeue & free retrieved queue entry */
	gx_page_queue_finish_dequeue(entry);
    }

    /* Close device, but don't use default if user hasn't overriden. */
    /* Ignore status, since returning bad status means open failed */
    if (prdev->printer_procs.close_render_device ==
	  gx_default_close_render_device)
	gdev_prn_async_render_close_device(prdev);
    else
	(*prdev->printer_procs.close_render_device)(prdev);

    /* undo fake open, since not called by gs_closedevice */
    prdev->is_open = false;

    /* Now that device is closed, acknowledge gx_page_queue_terminate */
    gx_page_queue_finish_dequeue(entry);

    return 0;
}

/* ------ Get/put parameters ------ */

/* Put parameters. */
private int
gdev_prn_async_write_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_clist_writer *const pclwdev =
	&((gx_device_clist *) pdev)->writer;
    gx_device_printer *const pwdev = (gx_device_printer *) pdev;
    gdev_prn_space_params save_sp = pwdev->space_params;
    int save_height = pwdev->height;
    int save_width = pwdev->width;
    int code, ecode;

    if (!pwdev->is_open)
	return (*pwdev->orig_procs.put_params) (pdev, plist);

    /*
     * First, cascade to real device's put_params.
     * If that put_params made any changes that require re-opening
     * the device, just flush the page; the parameter block at the
     * head of the next page will reflect the changes just made.
     * If the put_params requires no re-open, just slip it into the
     * stream in the command buffer. This way, the
     * writer device should parallel the renderer status at the same point
     * in their respective executions.
     *
     * NB. that all this works only because we take the position that
     * put_params can make no change that actually affects hardware's state
     * before the final output_page on the RASTERIZER.
     */
    /* Call original procedure, but "closed" to prevent closing device */
    pwdev->is_open = false;	/* prevent put_params from closing device */
    code = (*pwdev->orig_procs.put_params) (pdev, plist);
    pwdev->is_open = true;
    pwdev->OpenOutputFile = 0;	/* This is wrong, because it causes the same thing in the renderer */

    /* Flush device or emit to command list, depending if device changed geometry */
    if (memcmp(&pwdev->space_params, &save_sp, sizeof(save_sp)) != 0 ||
	pwdev->width != save_width || pwdev->height != save_height
	) {
	int pageq_remaining;
	int new_width = pwdev->width;
	int new_height = pwdev->height;
	gdev_prn_space_params new_sp = pwdev->space_params;

	/* Need to start a new page, reallocate clist memory */
	pwdev->width = save_width;
	pwdev->height = save_height;
	pwdev->space_params = save_sp;

	/* First, get rid of any pending partial pages */
	code = flush_page(pwdev, false);

	/* Free and reallocate the printer memory. */
	pageq_remaining = 1;	/* assume there are pages left in queue */
	do {
	    ecode =
		gdev_prn_reallocate_memory(pdev,
					   &new_sp, new_width, new_height);
	    if (ecode >= 0)
		break;		/* managed to recover enough memory */
	    if (!pdev->is_open) {
		/* Disaster! Device was forced closed, which async drivers */
		/* aren't suppsed to do. */
		gdev_prn_async_write_close_device(pdev);
		return ecode;	/* caller 'spozed to know could be closed now */
	    }
	    pclwdev->error_is_retryable = (ecode == gs_error_VMerror);
	}
	while (pageq_remaining >= 1 &&
	       (pageq_remaining = ecode =
		clist_VMerror_recover(pclwdev, ecode)) >= 0);
	if (ecode < 0) {
	    gdev_prn_free_memory(pdev);
	    pclwdev->is_open = false;
	    code = ecode;
	}
    } else if (code >= 0) {
	do
	    if ((ecode = cmd_put_params(pclwdev, plist)) >= 0)
		break;
	while ((ecode = clist_VMerror_recover(pclwdev, ecode)) >= 0);
	if (ecode < 0 && pclwdev->error_is_retryable &&
	    pclwdev->driver_call_nesting == 0
	    )
	    ecode = clist_VMerror_recover_flush(pclwdev, ecode);
	if (ecode < 0)
	    code = ecode;
    }
    /* Reset fields that got trashed by gdev_prn_put_params and/or gdev_prn_open */
    reinit_printer_into_printera(pwdev);

    return code;
}

/* Get hardware-detected params. Drain page queue, then call renderer version */
private int
gdev_prn_async_write_get_hardware_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_printer *const pwdev = (gx_device_printer *) pdev;
    gx_device_printer *const prdev = pwdev->async_renderer;

    if (!pwdev->is_open || !prdev)
	/* if not open, just use device's get hw params */
	return (dev_proc(pwdev, get_hardware_params))(pdev, plist);
    else {
	/* wait for empty pipeline */
	gx_page_queue_wait_until_empty(pwdev->page_queue);

	/* get reader's h/w params, now that writer & reader are sync'ed */
	return (dev_proc(prdev, get_hardware_params))
	    ((gx_device *) prdev, plist);
    }
}

/* Put parameters on RENDERER. */
private int		/* returns -ve err code only if FATAL error (can't keep rendering) */
gdev_prn_async_render_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_printer *const prdev = (gx_device_printer *) pdev;
    bool save_is_open = prdev->is_open;

    /* put_parms from clist are guaranteed to never re-init device */
    /* They're also pretty much guaranteed to always succeed */
    (*prdev->orig_procs.put_params) (pdev, plist);

    /* If device closed itself, try to open & clear it */
    if (!prdev->is_open && save_is_open) {
	int code;

	if (prdev->printer_procs.open_render_device ==
	      gx_default_open_render_device)
	    code = gdev_prn_async_render_open(prdev);
	else
	    code = (*prdev->printer_procs.open_render_device) (prdev);
	reinit_printer_into_renderer(prdev);
	if (code >= 0)
	    /****** CLEAR PAGE SOMEHOW ******/;
	else
	    return code;	/* this'll cause clist to stop processing this band! */
    }
    return 0;			/* return this unless FATAL status */
}


/* ------ Others ------ */

/* Output page causes file to get added to page queue for later rasterizing */
private int
gdev_prn_async_write_output_page(gx_device * pdev, int num_copies, int flush)
{
    gx_device_printer *const pwdev = (gx_device_printer *) pdev;
    gx_device_clist_writer *const pcwdev =
	&((gx_device_clist *) pdev)->writer;
    int flush_code;
    int add_code;
    int open_code;
    int one_last_time = 1;

    /* do NOT close files before sending to page queue */
    flush_code = clist_end_page(pcwdev);
    add_code = gx_page_queue_add_page(pwdev->page_queue,
				(flush ? GX_PAGE_QUEUE_ACTION_FULL_PAGE :
				 GX_PAGE_QUEUE_ACTION_COPY_PAGE),
				      &pcwdev->page_info, num_copies);
    if (flush && (flush_code >= 0) && (add_code >= 0)) {
	/* This page is finished */
	gx_finish_output_page(pdev, num_copies, flush);
    }

    /* Open new band files to take the place of ones added to page queue */
    while ((open_code = (*gs_clist_device_procs.open_device)
	    ((gx_device *) pdev)) == gs_error_VMerror) {
	/* Open failed, try after a page gets rendered */
	if (!gx_page_queue_wait_one_page(pwdev->page_queue)
	    && one_last_time-- <= 0)
	    break;
    }

    return
  	(flush_code < 0 ? flush_code : open_code < 0 ? open_code :
	 add_code < 0 ? add_code : 0);
}

/* Free bandlist memory waits until the rasterizer runs enough to free some mem */
private int			/* -ve err,  0 if no pages remain to rasterize, 1 if more pages to go */
gdev_prn_async_write_free_up_bandlist_memory(gx_device * pdev, bool flush_current)
{
    gx_device_printer *const pwdev = (gx_device_printer *) pdev;

    if (flush_current) {
	int code = flush_page(pwdev, true);

	if (code < 0)
	    return code;
    }
    return gx_page_queue_wait_one_page(pwdev->page_queue);
}

/* -------- Utility Routines --------- */

/* Flush out any partial pages accumulated in device */
/* LEAVE DEVICE in a state where it must be re-opened/re-init'd */
private int			/* ret 0 ok no flush, -ve error code */
flush_page(
	   gx_device_printer * pwdev,	/* async writer device to flush */
	   bool partial	/* true if only partial page */
)
{
    gx_device_clist *const pcldev = (gx_device_clist *) pwdev;
    gx_device_clist_writer *const pcwdev = &pcldev->writer;
    int flush_code = 0;
    int add_code = 0;

    /* do NOT close files before sending to page queue */
    flush_code = clist_end_page(pcwdev);
    add_code = gx_page_queue_add_page(pwdev->page_queue,
				(partial ? GX_PAGE_QUEUE_ACTION_PARTIAL_PAGE :
				 GX_PAGE_QUEUE_ACTION_FULL_PAGE),
				      &pcwdev->page_info, 0);

    /* Device no longer has BANDFILES, so it must be re-init'd by caller */
    pcwdev->page_info.bfile = pcwdev->page_info.cfile = 0;

    /* return the worst of the status. */
    if (flush_code < 0)
	return flush_code;
    return add_code;
}

/* Flush any pending partial pages, re-open device */
private int
reopen_clist_after_flush(
			 gx_device_printer * pwdev	/* async writer device to flush */
)
{
    int open_code;
    int one_last_time = 1;

    /* Open new band files to take the place of ones added to page queue */
    while ((open_code = (*gs_clist_device_procs.open_device)
	    ((gx_device *) pwdev)) == gs_error_VMerror) {
	/* Open failed, try after a page gets rendered */
	if (!gx_page_queue_wait_one_page(pwdev->page_queue)
	    && one_last_time-- <= 0)
	    break;
    }
    return open_code;
}

/*
 * The bandlist does allocations on the writer's thread & deallocations on
 * the reader's thread, so it needs to have mutual exclusion from itself, as
 * well as from other memory allocators since the reader can run at the same
 * time as the interpreter.  The bandlist allocator therefore consists of
 * a monitor-locking wrapper around either a direct heap allocator or (for
 * testing) a fixed-limit allocator.
 */

/* Create a bandlist allocator. */
private int
alloc_bandlist_memory(gs_memory_t ** final_allocator,
		      gs_memory_t * base_allocator)
{
    gs_memory_t *data_allocator = 0;
    gs_memory_locked_t *locked_allocator = 0;
    int code = 0;

#if defined(DEBUG) && defined(DebugBandlistMemorySize)
    code = alloc_render_memory(&data_allocator, base_allocator,
			       DebugBandlistMemorySize);
    if (code < 0)
	return code;
#else
    data_allocator = (gs_memory_t *)gs_malloc_memory_init();
    if (!data_allocator)
	return_error(gs_error_VMerror);
#endif
    locked_allocator = (gs_memory_locked_t *)
	gs_alloc_bytes_immovable(data_allocator, sizeof(gs_memory_locked_t),
				 "alloc_bandlist_memory(locked allocator)");
    if (!locked_allocator)
	goto alloc_err;
    code = gs_memory_locked_init(locked_allocator, data_allocator);
    if (code < 0)
	goto alloc_err;
    *final_allocator = (gs_memory_t *)locked_allocator;
    return 0;
alloc_err:
    if (locked_allocator)
	free_bandlist_memory((gs_memory_t *)locked_allocator);
    else if (data_allocator)
	gs_memory_free_all(data_allocator, FREE_ALL_EVERYTHING,
			   "alloc_bandlist_memory(data allocator)");
    return (code < 0 ? code : gs_note_error(gs_error_VMerror));
}

/* Free a bandlist allocator. */
private void
free_bandlist_memory(gs_memory_t *bandlist_allocator)
{
    gs_memory_locked_t *const lmem = (gs_memory_locked_t *)bandlist_allocator;
    gs_memory_t *data_mem = gs_memory_locked_target(lmem);

    gs_memory_free_all(bandlist_allocator,
		       FREE_ALL_STRUCTURES | FREE_ALL_ALLOCATOR,
		       "free_bandlist_memory(locked allocator)");
    if (data_mem)
	gs_memory_free_all(data_mem, FREE_ALL_EVERYTHING,
			   "free_bandlist_memory(data allocator)");
}

/* Create an allocator with a fixed memory limit. */
private int
alloc_render_memory(gs_memory_t **final_allocator,
		    gs_memory_t *base_allocator, long space)
{
    gs_ref_memory_t *rmem =
	ialloc_alloc_state((gs_memory_t *)base_allocator, space);
    vm_spaces spaces;
    int i, code;

    if (rmem == 0)
	return_error(gs_error_VMerror);
    code = ialloc_add_chunk(rmem, space, "alloc_render_memory");
    if (code < 0) {
	gs_memory_free_all((gs_memory_t *)rmem, FREE_ALL_EVERYTHING,
			   "alloc_render_memory");
	return code;
    }
    *final_allocator = (gs_memory_t *)rmem;

    /* Call the reclaim procedure to delete the string marking tables	*/
    /* Only need this once since no other chunks will ever exist	*/

    for ( i = 0; i < countof(spaces_indexed); ++i )
	spaces_indexed[i] = 0;
    space_local = space_global = (gs_ref_memory_t *)rmem;
    spaces.vm_reclaim = gs_nogc_reclaim;	/* no real GC on this chunk */
    GS_RECLAIM(&spaces, false);

    return 0;
}

/* Free an allocator with a fixed memory limit. */
private void
free_render_memory(gs_memory_t *render_allocator)
{
    if (render_allocator)
	gs_memory_free_all(render_allocator, FREE_ALL_EVERYTHING,
			   "free_render_memory");
}
