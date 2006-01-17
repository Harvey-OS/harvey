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

/* $Id: gxsync.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Interface to synchronization primitives */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */

#if !defined(gxsync_INCLUDED)
#  define gxsync_INCLUDED

#include "gpsync.h"
#include "gsmemory.h"

/* This module abstracts the platform-specific synchronization primitives. */
/* Since these routines will see heavy use, performance is important. */

/* ----- Semaphore interface ----- */
/* These have the usual queued, counting semaphore semantics: at init time, */
/* the event count is set to 0 ('wait' will wait until 1st signal). */
typedef struct gx_semaphore_s {
    gs_memory_t *memory;	/* allocator to free memory */
    gp_semaphore native;	/* MUST BE LAST last since length is undef'd */
    /*  platform-dep impl, len is gp_semaphore_sizeof() */
} gx_semaphore_t;

gx_semaphore_t *		/* returns a new semaphore, 0 if error */
    gx_semaphore_alloc(
		       gs_memory_t * memory	/* memory allocator to use */
		       );
void
    gx_semaphore_free(
		      gx_semaphore_t * sema	/* semaphore to delete */
		      );

#define gx_semaphore_wait(sema)  gp_semaphore_wait(&(sema)->native)
#define gx_semaphore_signal(sema)  gp_semaphore_signal(&(sema)->native)


/* ----- Monitor interface ----- */
/* These have the usual monitor semantics: at init time, */
/* the event count is set to 1 (1st 'enter' succeeds immediately). */
typedef struct gx_monitor_s {
    gs_memory_t *memory;	/* allocator to free memory */
    gp_monitor native;		/* platform-dep impl, len is gp_monitor_sizeof() */
} gx_monitor_t;

gx_monitor_t *			/* returns a new monitor, 0 if error */
    gx_monitor_alloc(
		     gs_memory_t * memory	/* memory allocator to use */
		     );
void
    gx_monitor_free(
		    gx_monitor_t * mon	/* monitor to delete */
		    );

#define gx_monitor_enter(sema)  gp_monitor_enter(&(sema)->native)
#define gx_monitor_leave(sema)  gp_monitor_leave(&(sema)->native)

#endif /* !defined(gxsync_INCLUDED) */
