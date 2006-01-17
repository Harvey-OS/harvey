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

/* $Id: gxpageq.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Page queue implementation */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */
/* 7/17/98 L. Peter Deutsch (ghost@aladdin.com) edited to conform to
   Ghostscript coding standards */
/* 8/7/98 ghost@aladdin.com fixed bugs in #define st_... statements */
/* 11/3/98 ghost@aladdin.com further updates for coding standards,
   moves definition of page queue structure to gxpageq.c */
/* 12/1/98 soho@crl.com - Upgraded gx_page_queue_action_t comments */

#ifndef gxpageq_INCLUDED
# define gxpageq_INCLUDED

#include "gsmemory.h"
#include "gxband.h"
#include "gxsync.h"

/* --------------- Data type definitions --------------------- */

/*	Action codes for each page queue entry. Each page that the interpreter
	emits to the page queue can actually be broken down into a sequence of
	one or more page queue entries. The general form for a given page's
	sequence of page queue entries can be expressed as:
		[PARTIAL_PAGE]... [COPY_PAGE [PARTIAL_PAGE]...]... FULL_PAGE
	where elements in square brackets are optional, and ellipses show
	repetition. NOTE that a single ACTION_TERMINATE (followed by nothing) can
	also show up at any point in the page queue in lieu of page descriptions.


	PARTIAL_PAGE: The interpreter emits a partial page when the bandlist is
	too small to contain a page's full representation. Partial pages will
	be emitted in out-of-memory situations *only* after the interpreter
	has determined that no further page queue entries are in the page
	queue, indicating that no further memory can be reclaimed by merely
	waiting for queued pages to render and free their associated bandlist.

	Note that num_copies is undefined for partial pages: the actual
	number of pages to render will only be known when ...COPY_PAGE
	or FULL_PAGE is emitted.

	Partial pages are never imaged.


	FULL_PAGE: The interpreter emits a full page when a page description
	is complete (e.g. showpage), or trashed (e.g. setpagedevice). The
	page's complete description consists of the FULL_PAGE plus all
	PARTIAL_PAGEs that immediately precede it in the page queue (and
	possibly preceding COPY_PAGEs) all the way back to the previous
	FULL_PAGE (or up to the beginning of queue entries).

	In the case of a trashed page, the page count will be 0. The page
	queue may choose to not render the 0-count FULL_PAGE queue entry
	for efficiency. If they have not been rendered, the page queue
	may choose to also discard (and/or not render) any PARTIAL_PAGEs
	leading up to the trashed page. The page queue must however take
	care to not discard any entries leading up to a COPY_PAGE with
	a non-0 page count that may precede the FULL_PAGE, since COPY_PAGE
	must be rendered in that case. In any event, a 0-count page will
	not be imaged.

	In the case of a complete page, the page count will be 0 or greater.
	The 0-count page is equivalent to a trashed page -- see above. The
	renderer must ensure that all PARTIAL_PAGEs and COPY_PAGEs leading
	up to the FULL_PAGE are rendered sequentially before rendering
	and imaging the FULL_PAGE.


	COPY_PAGE: is similar to FULL_PAGE above, except that COPY_PAGE must
	keep the rendered results, instead of clearing them. COPY_PAGE
	differs from a partial page in that the page must be imaged, as well
	as rasterized. This is to support PostScript language "copypage"
	semantics.

	Note that a 0 page count here does not absolve the renderer from
	rendering the page queue entries (unless all subsequent COPY_PAGEs
	the the FULL_PAGE for this page also have a 0 page count), since
	the results of COPY_PAGE must be available for subsequent pages.


	TERMINATE: This entry can appear at any time in the page queue. It
	will be the last entry to ever appear in the queue. The semantics
	of this entry require all prior non-zero-count COPY_PAGEs and 
	FULL_PAGEs to be imaged. Any trailing PARTIAL_PAGEs may optionally
	be rendered, but should not be imaged.
 */
typedef enum {
    GX_PAGE_QUEUE_ACTION_PARTIAL_PAGE,
    GX_PAGE_QUEUE_ACTION_FULL_PAGE,
    GX_PAGE_QUEUE_ACTION_COPY_PAGE,
    GX_PAGE_QUEUE_ACTION_TERMINATE
} gx_page_queue_action_t;

/*
 * Define the structure used to manage a page queue.
 * A page queue is a monitor-locked FIFO which holds completed command
 * list files ready for rendering.
 */
#ifndef gx_page_queue_DEFINED
# define gx_page_queue_DEFINED
typedef struct gx_page_queue_s gx_page_queue_t;
#endif

/*
 * Define a page queue entry object.
 */
typedef struct gx_page_queue_entry_s gx_page_queue_entry_t;
struct gx_page_queue_entry_s {
    gx_band_page_info_t page_info;
    gx_page_queue_action_t action;	/* action code */
    int num_copies;		/* number of copies to render, only defined */
                                /* if action == ...FULL_PAGE or ...COPY_PAGE */
    gx_page_queue_entry_t *next;		/* link to next in queue */
    gx_page_queue_t *queue;	/* link to queue the entry is in */
};

#define private_st_gx_page_queue_entry()\
  gs_private_st_ptrs2(st_gx_page_queue_entry, gx_page_queue_entry_t,\
    "gx_page_queue_entry",\
    gx_page_queue_entry_enum_ptrs, gx_page_queue_entry_reloc_ptrs,\
    next, queue)

/* -------------- Public Procedure Declaraions --------------------- */

/* Allocate a page queue. */
gx_page_queue_t *gx_page_queue_alloc(gs_memory_t *mem);

/*
 * Allocate and initialize a page queue entry.
 * All page queue entries must be allocated by this routine.
 */
/* rets ptr to allocated object, 0 if VM error */
gx_page_queue_entry_t *
gx_page_queue_entry_alloc(
    gx_page_queue_t * queue	/* queue that entry is being alloc'd for */
    );

/*
 * Free a page queue entry.
 * All page queue entries must be destroyed by this routine.
 */
void gx_page_queue_entry_free(
    gx_page_queue_entry_t * entry	/* entry to free up */
    );

/*
 * Free the page_info resources held by the pageq entry.  Used to free
 * pages' clist, typically after rendering.  Note that this routine is NOT
 * called implicitly by gx_page_queue_entry_free, since page clist may be
 * managed separately from page queue entries.  However, unless you are
 * managing clist separately, you must call this routine before freeing the
 * pageq entry itself (via gx_page_queue_entry_free), or you will leak
 * memory (lots).
 */
void gx_page_queue_entry_free_page_info(
    gx_page_queue_entry_t * entry	/* entry to free up */
    );

/*
 * Initialize a page queue; this must be done before it can be used.
 * This routine allocates & inits various necessary structures and will
 * fail if insufficient memory is available.
 */
/* -ve error code, or 0 */
int gx_page_queue_init(
    gx_page_queue_t * queue,	/* page queue to init */
    gs_memory_t * memory	/* allocator for dynamic memory */
    );

/*
 * Destroy a page queue which was initialized by gx_page_queue_init.
 * Any page queue entries in the queue are released and destroyed;
 * dynamic allocations are released.
 */
void gx_page_queue_dnit(
    gx_page_queue_t * queue	/* page queue to dnit */
    );

/*
 * If there are any pages in queue, wait until one of them finishes
 * rendering.  Typically called by writer's out-of-memory error handlers
 * that want to wait until some memory has been freed.
 */
/* rets 0 if no pages were waiting for rendering, 1 if actually waited */
int gx_page_queue_wait_one_page(
    gx_page_queue_t * queue	/* queue to wait on */
    );

/*
 * Wait until all (if any) pages in queue have finished rendering. Typically
 * called by writer operations which need to drain the page queue before
 * continuing.
 */
void gx_page_queue_wait_until_empty(
    gx_page_queue_t * queue		/* page queue to wait on */
    );

/*
 * Add a pageq queue entry to the end of the page queue. If an unsatisfied
 * reader thread has an outstanding gx_page_queue_start_deque(), wake it up.
 */
void gx_page_queue_enqueue(
    gx_page_queue_entry_t * entry	/* entry to add */
    );

/*
 * Allocate & construct a pageq entry, then add to the end of the pageq as
 * in gx_page_queue_enqueue. If unable to allocate a new pageq entry, uses
 * the pre-allocated reserve entry held in the pageq. When using the reserve
 * pageq entry, wait until enough pages have been rendered to allocate a new
 * reserve for next time -- this should always succeed & returns eFatal if not.
 * Unless the reserve was used, does not wait for any rendering to complete.
 * Typically called by writer when it has a (partial) page ready for rendering.
 */
/* rets 0 ok, gs_error_Fatal if error */
int gx_page_queue_add_page(
    gx_page_queue_t * queue,		/* page queue to add to */
    gx_page_queue_action_t action,		/* action code to queue */
    const gx_band_page_info_t * page_info,	/* bandinfo incl. bandlist */
    int page_count		/* # of copies to print if final "print," */
				   /* 0 if partial page, -1 if cancel */
    );

/*
 * Retrieve the least-recently added queue entry from the pageq. If no
 * entry is available, waits on a signal from gx_page_queue_enqueue. Must
 * eventually be followed by a call to gx_page_queue_finish_dequeue for the
 * same pageq entry.
 * Even though the pageq is actually removed from the pageq, a mark is made in
 * the pageq to indicate that the pageq is not "empty" until the
 * gx_page_queue_finish_dequeue; this is for the benefit of
 * gx_page_queue_wait_???, since the completing the current page's rendering
 * may free more memory.
 * Typically called by renderer thread loop, which looks like:
    do {
	gx_page_queue_start_deqeueue(...);
	render_retrieved_entry(...);
	gx_page_queue_finish_dequeue(...);
    } while (some condition);
 */
gx_page_queue_entry_t *		/* removed entry */
gx_page_queue_start_dequeue(
    gx_page_queue_t * queue	/* page queue to retrieve from */
    );

/*
 * Free the pageq entry and its associated band list data, then signal any
 * waiting threads.  Typically used to indicate completion of rendering the
 * pageq entry.  Note that this is different from gx_page_queue_entry_free,
 * which does not free the band list data (a separate call of
 * gx_page_queue_entry_free_page_info is required).
 */
void gx_page_queue_finish_dequeue(
    gx_page_queue_entry_t * entry  /* entry that was retrieved to delete */
    );

#endif /*!defined(gxpageq_INCLUDED) */
