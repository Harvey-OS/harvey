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

/* $Id: gxpageq.c,v 1.5 2002/06/16 05:48:56 lpd Exp $ */
/* Page queue implementation */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */

#include "gx.h"
#include "gxdevice.h"
#include "gxclist.h"
#include "gxpageq.h"
#include "gserrors.h"
#include "gsstruct.h"

/* Define the structure implementation for a page queue. */
struct gx_page_queue_s {
    gs_memory_t *memory;	/* allocator used to allocate entries */
    gx_monitor_t *monitor;	/* used to serialize access to this structure */
    int entry_count;		/* # elements in page_queue */
    bool dequeue_in_progress;	/* true between start/ & end_dequeue */
    gx_semaphore_t *render_req_sema;	/* sema signalled when page queued */
    bool enable_render_done_signal;	/* enable signals to render_done_sema */
    gx_semaphore_t *render_done_sema;	/* semaphore signaled when (partial) page rendered */
    gx_page_queue_entry_t *last_in;	/* if <> 0, Last-in queue entry */
    gx_page_queue_entry_t *first_in;	/* if <> 0, First-in queue entry */
    gx_page_queue_entry_t *reserve_entry;	/* spare allocation */
};

/*
 * Null initializer for entry page_info (used by gx_page_queue_add_page() ).
 */
private const gx_band_page_info_t null_page_info = { PAGE_INFO_NULL_VALUES };

#define private_st_gx_page_queue()\
  gs_private_st_ptrs4(st_gx_page_queue, gx_page_queue_t, "gx_page_queue",\
    gx_page_queue_enum_ptrs, gx_page_queue_reloc_ptrs,\
    monitor, first_in, last_in, reserve_entry);

/* ------------------- Global Data ----------------------- */

/* Structure descriptor for GC */
private_st_gx_page_queue_entry();
private_st_gx_page_queue();

/* ------------ Forward Decl's --------------------------- */
private gx_page_queue_entry_t *	/* removed entry, 0 if none avail */
    gx_page_queue_remove_first(
			       gx_page_queue_t * queue	/* page queue to retrieve from */
			       );


/* --------------------Procedures------------------------- */

/* Allocate a page queue. */
gx_page_queue_t *
gx_page_queue_alloc(gs_memory_t *mem)
{
    return gs_alloc_struct(mem, gx_page_queue_t, &st_gx_page_queue,
			   "gx_page_queue_alloc");
}

/* ------- page_queue_entry alloc/free --------- */

/* Allocate & init a gx_page_queue_entry */
gx_page_queue_entry_t *		/* rets ptr to allocated object, 0 if VM error */
gx_page_queue_entry_alloc(
			  gx_page_queue_t * queue	/* queue that entry is being alloc'd for */
)
{
    gx_page_queue_entry_t *entry
    = gs_alloc_struct(queue->memory, gx_page_queue_entry_t,
		      &st_gx_page_queue_entry, "gx_page_queue_entry_alloc");

    if (entry != 0) {
	entry->next = 0;
	entry->queue = queue;
    }
    return entry;
}

/* Free a gx_page_queue_entry allocated w/gx_page_queue_entry_alloc */
void
gx_page_queue_entry_free(
			    gx_page_queue_entry_t * entry	/* entry to free up */
)
{
    gs_free_object(entry->queue->memory, entry, "gx_page_queue_entry_free");
}
  
/* Free the clist resources held by a gx_page_queue_entry_t */
void
gx_page_queue_entry_free_page_info(
			    gx_page_queue_entry_t * entry	/* entry to free up */
)
{
    clist_close_page_info( &entry->page_info );
}

/* -------- page_queue init/dnit ---------- */

/* Initialize a gx_page_queue object */
int				/* -ve error code, or 0 */
gx_page_queue_init(
		   gx_page_queue_t * queue,	/* page queue to init */
		   gs_memory_t * memory	/* allocator for dynamic memory */
)
{
    queue->memory = memory;
    queue->monitor = gx_monitor_alloc(memory);	/* alloc monitor to serialize */
    queue->entry_count = 0;
    queue->dequeue_in_progress = false;
    queue->render_req_sema = gx_semaphore_alloc(memory);
    queue->enable_render_done_signal = false;
    queue->render_done_sema = gx_semaphore_alloc(memory);
    queue->first_in = queue->last_in = 0;
    queue->reserve_entry = gx_page_queue_entry_alloc(queue);

    if (queue->monitor && queue->render_req_sema && queue->render_done_sema
	&& queue->reserve_entry)
	return 0;
    else {
	gx_page_queue_dnit(queue);
	return gs_error_VMerror;
    }
}

/* Dnitialize a gx_page_queue object */
void
gx_page_queue_dnit(
		      gx_page_queue_t * queue	/* page queue to dnit */
)
{
    /* Deallocate any left-over queue entries */
    gx_page_queue_entry_t *entry;

    while ((entry = gx_page_queue_remove_first(queue)) != 0) {
	gx_page_queue_entry_free_page_info(entry);
	gx_page_queue_entry_free(entry);
    }

    /* Free dynamic objects */
    if (queue->monitor) {
	gx_monitor_free(queue->monitor);
	queue->monitor = 0;
    }
    if (queue->render_req_sema) {
	gx_semaphore_free(queue->render_req_sema);
	queue->render_req_sema = 0;
    }
    if (queue->render_done_sema) {
	gx_semaphore_free(queue->render_done_sema);
	queue->render_done_sema = 0;
    }
    if (queue->reserve_entry) {
	gx_page_queue_entry_free(queue->reserve_entry);
	queue->reserve_entry = 0;
    }
}

/* -------- low-level queue add/remove ---------- */

/* Retrieve & remove firstin queue entry */
private gx_page_queue_entry_t *	/* removed entry, 0 if none avail */
gx_page_queue_remove_first(
			      gx_page_queue_t * queue	/* page queue to retrieve from */
)
{
    gx_page_queue_entry_t *entry = 0;	/* assume failure */

    /* Enter monitor */
    gx_monitor_enter(queue->monitor);

    /* Get the goods */
    if (queue->entry_count) {
	entry = queue->first_in;
	queue->first_in = entry->next;
	if (queue->last_in == entry)
	    queue->last_in = 0;
	--queue->entry_count;
    }
    /* exit monitor */
    gx_monitor_leave(queue->monitor);

    return entry;
}

/* Add entry to queue at end */
private void
gx_page_queue_add_last(
			  gx_page_queue_entry_t * entry	/* entry to add */
)
{
    gx_page_queue_t *queue = entry->queue;

    /* Enter monitor */
    gx_monitor_enter(queue->monitor);

    /* Add the goods */
    entry->next = 0;
    if (queue->last_in != 0)
	queue->last_in->next = entry;
    queue->last_in = entry;
    if (queue->first_in == 0)
	queue->first_in = entry;
    ++queue->entry_count;

    /* exit monitor */
    gx_monitor_leave(queue->monitor);
}

/* --------- low-level synchronization ---------- */

/* Wait for a single page to finish rendering (if any pending) */
int				/* rets 0 if no pages were waiting for rendering, 1 if actually waited */
gx_page_queue_wait_one_page(
			       gx_page_queue_t * queue	/* queue to wait on */
)
{
    int code;

    gx_monitor_enter(queue->monitor);
    if (!queue->entry_count && !queue->dequeue_in_progress) {
	code = 0;
	gx_monitor_leave(queue->monitor);
    } else {
	/* request acknowledgement on render done */
	queue->enable_render_done_signal = true;

	/* exit monitor & wait for acknowlegement */
	gx_monitor_leave(queue->monitor);
	gx_semaphore_wait(queue->render_done_sema);
	code = 1;
    }
    return code;
}

/* Wait for page queue to become empty */
void
gx_page_queue_wait_until_empty(
				  gx_page_queue_t * queue	/* page queue to wait on */
)
{
    while (gx_page_queue_wait_one_page(queue));
}

/* -----------  Synchronized page_queue get/put routines ------ */

/* Add an entry to page queue for rendering w/sync to renderer */
void
gx_page_queue_enqueue(
			 gx_page_queue_entry_t * entry	/* entry to add */
)
{
    gx_page_queue_t *queue = entry->queue;

    /* Add the goods to queue, & signal it */
    gx_page_queue_add_last(entry);
    gx_semaphore_signal(queue->render_req_sema);
}

/* Add page to a page queue */
/* Even if an error is returned, entry will have been added to queue! */
int				/* rets 0 ok, gs_error_VMerror if error */
gx_page_queue_add_page(
			  gx_page_queue_t * queue,	/* page queue to add to */
			  gx_page_queue_action_t action,	/* action code to queue */
			  const gx_band_page_info_t * page_info,  /* bandinfo incl. bandlist (or 0) */
			  int page_count	/* see comments in gdevprna.c */
)
{
    int code = 0;

    /* Allocate a new page queue entry */
    gx_page_queue_entry_t *entry
    = gx_page_queue_entry_alloc(queue);

    if (!entry) {
	/* Use reserve page queue entry */
	gx_monitor_enter(queue->monitor);	/* not strictly necessary */
	entry = queue->reserve_entry;
	queue->reserve_entry = 0;
	gx_monitor_leave(queue->monitor);
    }
    /* Fill in page queue entry with info from device */
    entry->action = action;
    if (page_info != 0)
	entry->page_info = *page_info;
    else
	entry->page_info = null_page_info;
    entry->num_copies = page_count;

    /* Stick onto page queue & signal */
    gx_page_queue_enqueue(entry);

    /* If a new reserve entry is needed, wait till enough mem is avail */
    while (!queue->reserve_entry) {
	queue->reserve_entry = gx_page_queue_entry_alloc(queue);
	if (!queue->reserve_entry && !gx_page_queue_wait_one_page(queue)) {
	    /* Should never happen: all pages rendered & still can't get memory: give up! */
	    code = gs_note_error(gs_error_Fatal);
	    break;
	}
    }
    return code;
}

/* Wait for & get next page queue entry */
gx_page_queue_entry_t *		/* removed entry */
gx_page_queue_start_dequeue(
			       gx_page_queue_t * queue	/* page queue to retrieve from */
)
{
    gx_semaphore_wait(queue->render_req_sema);
    queue->dequeue_in_progress = true;
    return gx_page_queue_remove_first(queue);
}

/* After rendering page gotten w/gx_page_queue_dequeue, call this to ack */
void
gx_page_queue_finish_dequeue(
				gx_page_queue_entry_t * entry	/* entry that was retrieved to delete */
)
{
    gx_page_queue_t *queue = entry->queue;

    gx_monitor_enter(queue->monitor);
    if (queue->enable_render_done_signal) {
	queue->enable_render_done_signal = false;
	gx_semaphore_signal(queue->render_done_sema);
    }
    queue->dequeue_in_progress = false;

    /*
     * Delete the previously-allocated entry, do inside monitor in case
     * this is the reserve entry & is the only memory in the universe;
     * in that case gx_page_queue_add_page won't be looking for this
     * until the monitor is exited.
     * In this implementation of the page queue, clist and queue entries
     * are managed together, so free the clist just before freeing the entry.
     */
    gx_page_queue_entry_free_page_info(entry);
    gx_page_queue_entry_free(entry);

    gx_monitor_leave(queue->monitor);
}
