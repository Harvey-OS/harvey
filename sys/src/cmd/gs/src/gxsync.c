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

/*$Id: gxsync.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Interface to platform-based synchronization primitives */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */

#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmemory.h"
#include "gxsync.h"

/* This module abstracts the platform-specific synchronization primitives. */
/* Since these routines will see heavy use, performance is important. */


/* ----- Semaphore interface ----- */
/* These have the usual queued, counting semaphore semantics: at init time, */
/* the event count is set to 0 ('wait' will wait until 1st signal). */

/* Allocate & initialize a semaphore */
gx_semaphore_t *		/* returns a new semaphore, 0 if error */
gx_semaphore_alloc(
		      gs_memory_t * memory	/* memory allocator to use */
)
{
    gx_semaphore_t *sema;

    /* sizeof decl'd sema struct, minus semaphore placeholder's size, + actual semaphore size */
    unsigned semaSizeof
    = sizeof(*sema) - sizeof(sema->native) + gp_semaphore_sizeof();

    if (gp_semaphore_open(0) == 0)	/* see if gp_semaphores are movable */
	/* movable */
	sema = (gx_semaphore_t *) gs_alloc_bytes(memory, semaSizeof,
						 "gx_semaphore (create)");
    else
	/* unmovable */
	sema = (gx_semaphore_t *) gs_alloc_bytes_immovable(memory, semaSizeof,
						   "gx_semaphore (create)");
    if (sema == 0)
	return 0;

    /* Make sema remember which allocator was used to allocate it */
    sema->memory = memory;

    if (gp_semaphore_open(&sema->native) < 0) {
	gs_free_object(memory, sema, "gx_semaphore (alloc)");
	return 0;
    }
    return sema;
}

/* Deinit & free a semaphore */
void
gx_semaphore_free(
		     gx_semaphore_t * sema	/* semaphore to delete */
)
{
    if (sema) {
	gp_semaphore_close(&sema->native);
	gs_free_object(sema->memory, sema, "gx_semaphore (free)");
    }
}

/* Macros defined in gxsync.h, but redefined here so compiler chex consistency */
#define gx_semaphore_wait(sema)  gp_semaphore_wait(&(sema)->native)
#define gx_semaphore_signal(sema)  gp_semaphore_signal(&(sema)->native)


/* ----- Monitor interface ----- */
/* These have the usual monitor semantics: at init time, */
/* the event count is set to 1 (1st 'enter' succeeds immediately). */

/* Allocate & Init a monitor */
gx_monitor_t *			/* returns a new monitor, 0 if error */
gx_monitor_alloc(
		    gs_memory_t * memory	/* memory allocator to use */
)
{
    gx_monitor_t *mon;

    /* sizeof decl'd mon struct, minus monitor placeholder's size, + actual monitor size */
    unsigned monSizeof
    = sizeof(*mon) - sizeof(mon->native) + gp_monitor_sizeof();

    if (gp_monitor_open(0) == 0)	/* see if gp_monitors are movable */
	/* movable */
	mon = (gx_monitor_t *) gs_alloc_bytes(memory, monSizeof,
					      "gx_monitor (create)");
    else
	/* unmovable */
	mon = (gx_monitor_t *) gs_alloc_bytes_immovable(memory, monSizeof,
						     "gx_monitor (create)");
    if (mon == 0)
	return 0;

    /* Make monitor remember which allocator was used to allocate it */
    mon->memory = memory;

    if (gp_monitor_open(&mon->native) < 0) {
	gs_free_object(memory, mon, "gx_monitor (alloc)");
	return 0;
    }
    return mon;
}

/* Dnit & free a monitor */
void
gx_monitor_free(
		   gx_monitor_t * mon	/* monitor to delete */
)
{
    if (mon) {
	gp_monitor_close(&mon->native);
	gs_free_object(mon->memory, mon, "gx_monitor (free)");
    }
}

/* Macros defined in gxsync.h, but redefined here so compiler chex consistency */
#define gx_monitor_enter(sema)  gp_monitor_enter(&(sema)->native)
#define gx_monitor_leave(sema)  gp_monitor_leave(&(sema)->native)
